#pragma once

// Authored animation state and timing for projectile.
// BulletSource creates its actor pool at activation, not at Shoot. Native
// Actor::PreUpdate0xe6150 advances the selected clip while invisible;
// EnemyBullet::Shoot0x1beb44 does not restart it. Preserve source/member
// identity independently from the shared reusable Box2D slot and flight age.
int projectileAnimationClassNative(const Props &p) {
    Object *source = classById(idnum(p.get("bulletClass")));
    return source && source->has("EnemyBulletBehaviour") ? (int)(source - classes) : -1;
}

int bossProjectileAnimationClassNative(const Props &p) {
    Object *source = classById(idnum(p.get("bullet-class")));
    return source && source->has("FinalBossBulletBehaviour") ? (int)(source - classes) : -1;
}

void appendProjectileAnimationMembersNative(int owner, int klass, int count) {
    if (klass < 0)
        return;
    for (int n = 0; n < count; n++) {
        if (projectileAnimationsNative.count >= MAX_PROJECTILES)
            fatal("Projectile animation pool is full.");
        projectileAnimationsNative.members[projectileAnimationsNative.count++] =
            ProjectileAnimationMemberNative{owner, klass, n, 0};
    }
}

void resetProjectileAnimationsNative() {
    memset(&projectileAnimationsNative, 0, sizeof(projectileAnimationsNative));
    for (int i = 0; i < MAX_PROJECTILES; i++)
        projectileAnimationsNative.binding[i] = -1;
    for (int owner = 0; owner < entityCount; owner++) {
        if (const Props *p = entities[owner].source->behavior("BulletSource"))
            appendProjectileAnimationMembersNative(owner, projectileAnimationClassNative(*p),
                                                   (int)p->num("bulletPoolSize", 10));
        if (const Props *p = entities[owner].source->behavior("FinalBossBehaviour"))
            appendProjectileAnimationMembersNative(owner, bossProjectileAnimationClassNative(*p),
                                                   (int)p->num("bullet-pool-size", 10));
    }
}

bool projectileAnimationMatchesNative(int slot, int member) {
    if (member < 0 || member >= projectileAnimationsNative.count)
        return false;
    const Bullet &b = bullets[slot];
    const ProjectileAnimationMemberNative &p = projectileAnimationsNative.members[member];
    return b.life > 0 && b.klass == p.klass && b.state.active == p.owner + 1;
}

int freeProjectileAnimationNative(int owner, int klass) {
    for (int i = 0; i < projectileAnimationsNative.count; i++)
        if (projectileAnimationsNative.members[i].owner == owner &&
            projectileAnimationsNative.members[i].klass == klass) {
            bool occupied = false;
            for (int slot = 0; slot < MAX_PROJECTILES; slot++)
                if (projectileAnimationsNative.binding[slot] == i &&
                    projectileAnimationMatchesNative(slot, i)) {
                    occupied = true;
                    break;
                }
            if (!occupied)
                return i;
        }
    return -1;
}

float projectileAnimationLegacyAgeNative(int klass, float age) {
    float length = animationLength(
        animation(actor(classes[klass].id), classes[klass].props.get("default-animation")));
    age = maxf(0, age);
    if (length > 0 && age >= length)
        age -= (int)(age / length) * length;
    return age;
}

void reconcileProjectileAnimationBindingsNative() {
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        if (!projectileAnimationMatchesNative(slot, projectileAnimationsNative.binding[slot]))
            projectileAnimationsNative.binding[slot] = -1;
    // Old files did not retain hidden phases or member identities. Preserve
    // each known live visual phase; initialize the unsaved instances at zero.
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        if (bullets[slot].life > 0 &&
            (classes[bullets[slot].klass].has("EnemyBulletBehaviour") ||
             classes[bullets[slot].klass].has("FinalBossBulletBehaviour")) &&
            projectileAnimationsNative.binding[slot] < 0) {
            int member =
                freeProjectileAnimationNative(bullets[slot].state.active - 1, bullets[slot].klass);
            if (member >= 0) {
                projectileAnimationsNative.binding[slot] = member;
                projectileAnimationsNative.members[member].age = projectileAnimationLegacyAgeNative(
                    bullets[slot].klass, bullets[slot].state.timer);
            }
        }
}

void advanceProjectileAnimationsNative(float dt, bool paused = false) {
    if (dt <= 0)
        return;
    for (int i = 0; i < projectileAnimationsNative.count; i++) {
        ProjectileAnimationMemberNative &p = projectileAnimationsNative.members[i];
        Object &source = classes[p.klass];
        if (source.props.flag("no-tick") ||
            ((paused || source.paused) && !source.props.flag("animate-while-paused")))
            continue;
        float length =
            animationLength(animation(actor(source.id), source.props.get("default-animation")));
        if (length > 0) {
            p.age += dt;
            if (p.age >= length)
                p.age -= length;
        }
    }
}

float projectileAnimationAgeNative(const Bullet &b) {
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        if (bullets + slot == &b) {
            int member = projectileAnimationsNative.binding[slot];
            if (projectileAnimationMatchesNative(slot, member))
                return projectileAnimationsNative.members[member].age;
            break;
        }
    return b.state.timer;
}

bool compatibleProjectileAnimationsNative(const ProjectileAnimationStateNative &saved,
                                          bool includeBoss = true) {
    int index = 0;
    for (int owner = 0; owner < entityCount; owner++)
        for (int kind = 0; kind < (includeBoss ? 2 : 1); kind++) {
            const Props *p =
                entities[owner].source->behavior(kind ? "FinalBossBehaviour" : "BulletSource");
            if (!p)
                continue;
            int klass =
                kind ? bossProjectileAnimationClassNative(*p) : projectileAnimationClassNative(*p);
            if (klass < 0)
                continue;
            int count = (int)p->num(kind ? "bullet-pool-size" : "bulletPoolSize", 10);
            for (int n = 0; n < count; n++) {
                if (index >= saved.count || saved.members[index].owner != owner ||
                    saved.members[index].klass != klass || saved.members[index].ordinal != n)
                    return false;
                index++;
            }
        }
    return index == saved.count;
}

void restoreVersion18ProjectileAnimationsNative(const ProjectileAnimationStateNative &saved) {
    resetProjectileAnimationsNative();
    for (int n = 0; n < saved.count; n++)
        for (int i = 0; i < projectileAnimationsNative.count; i++) {
            const ProjectileAnimationMemberNative &old = saved.members[n];
            ProjectileAnimationMemberNative &member = projectileAnimationsNative.members[i];
            if (old.owner == member.owner && old.klass == member.klass &&
                old.ordinal == member.ordinal) {
                member.age = old.age;
                for (int slot = 0; slot < MAX_PROJECTILES; slot++)
                    if (saved.binding[slot] == n)
                        projectileAnimationsNative.binding[slot] = i;
                break;
            }
        }
    reconcileProjectileAnimationBindingsNative();
}
