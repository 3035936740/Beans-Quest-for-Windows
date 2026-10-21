#pragma once

// Owns projectile slots, contact processing and their gameplay lifetime.

// EnemyBullet::PreSolve 0x1ba6c0 and PostSolve 0x1ba4a8: ignore the
// objects overlapped during launch, pass through one-way geometry, and
// deactivate after a later solid contact instead of reusing enemy stomps.
struct ShotContacts {
    u32 ignored[(MAX_ENTITIES + 31) / 32];
    u32 extraIgnored[PROJECTILE_EXTRA_IGNORE_WORDS_NATIVE];
    bool first, hit, hitBean;
    float lastTime;
    Vec point;
};

ShotContacts shotContacts[MAX_PROJECTILES];

// FinalBossBullet::PostPhysicsStep0x1e8c14 waits for Killable::IsDead,
// not IsDeadOrDying. A stomp therefore retains its occupied pool slot until
// the shared Killable squash completes. Bullet.aux is otherwise unused;
// keep its saved nonnegative death age there without changing the wire layout.
bool shotFlyingNative(const Bullet &b) {
    return b.life > 0 && !b.state.gone;
}

bool shotDeathVisibleNative(const Bullet &b) {
    return b.life > 0 && b.state.gone == 2 && classes[b.klass].has("KillableEnemyBehaviour");
}

float shotDeathDurationNative(const Bullet &b) {
    return classes[b.klass].behavior("KillableEnemyBehaviour")->num("die-time", .1f);
}

Rect shotDeathRectNative(const Bullet &b, Rect r) {
    if (!shotDeathVisibleNative(b))
        return r;
    float duration = shotDeathDurationNative(b),
          t = duration > 0 ? clamp(b.state.aux / duration, 0, 1) : 1;
    Vec c = center(r);
    return Rect{c.x + (r.l - c.x) * (1 - .5f * t), c.y + (r.b - c.y) * (1 - t),
                c.x + (r.r - c.x) * (1 - .5f * t), c.y + (r.t - c.y) * (1 - t)};
}

void beginShotDeathNative(int slot) {
    Bullet &b = bullets[slot];
    if (!shotFlyingNative(b) || !classes[b.klass].has("KillableEnemyBehaviour"))
        return;
    if (shotBodies[slot]) {
        b2Vec2 p = shotBodies[slot]->GetPosition(), v = shotBodies[slot]->GetLinearVelocity();
        b.state.x = p.x / metres;
        b.state.y = p.y / metres;
        b.state.vx = v.x / metres;
        b.state.vy = v.y / metres;
        shotBodies[slot]->SetActive(false);
    }
    b.state.gone = 2;
    b.state.aux = 0;
    sound("enemyexplosion");
}

void advanceShotDeathNative(int slot, float dt) {
    Bullet &b = bullets[slot];
    if (!shotDeathVisibleNative(b) || dt <= 0)
        return;
    b.state.aux += dt;
    if (b.state.aux > shotDeathDurationNative(b)) {
        const Props &p = *classes[b.klass].behavior("KillableEnemyBehaviour");
        int owner = b.state.active - 1, layer = owner >= 0 && owner < entityCount
                                                    ? entities[owner].layer
                                                    : entities[playerIndex].layer;
        b.life = 0;
        b.state.gone = 1;
        emitEffect(idnum(p.get("explosion-actor")), b.state.x, b.state.y, layer);
        if (p.flag("alternate-on-death"))
            alternateScene((int)p.num("alternation-mask", 15));
    }
}

bool projectileGravitated(const Object &source) {
    // Both bullet controllers override their CirclePhysics body on activation:
    // FinalBossBullet 0x1e6934 and EnemyBullet 0x1bd73c clear body flag0x80.
    if (source.has("FinalBossBulletBehaviour") || source.has("EnemyBulletBehaviour"))
        return false;
    const Props *physics = source.behavior("CirclePhysicsBehaviour");
    return physics && physics->flag("is-gravitated", true);
}

// Native PhysicsObject::GetId0x2c6498 identifies a persistent behaviour,
// not a disposable Box2D body/slot. Source members and oscillator proxies
// therefore need distinct stable identities in the launch-ignore set.
int projectilePhysicsIdentityNative(b2Body *body) {
    int index = (int)body->GetUserData() - 1;
    if (index >= 0 && index < entityCount) {
        if (body == bodies[index])
            return index;
        if (body == oscillatorProxies[index])
            return MAX_ENTITIES + MAX_PROJECTILES + index;
    }
    int slot = index - MAX_ENTITIES;
    if (slot >= 0 && slot < MAX_PROJECTILES && shotBodies[slot] == body) {
        int member = projectileAnimationsNative.binding[slot];
        if (projectileAnimationMatchesNative(slot, member))
            return MAX_ENTITIES + member;
    }
    return -1;
}

bool shotIgnored(int slot, int identity) {
    if (identity < 0 || identity >= PROJECTILE_PHYSICS_ID_LIMIT_NATIVE)
        return false;
    if (identity < MAX_ENTITIES)
        return (shotContacts[slot].ignored[identity / 32] & (1u << (identity % 32))) != 0;
    int key = identity - MAX_ENTITIES;
    return (shotContacts[slot].extraIgnored[key / 32] & (1u << (key % 32))) != 0;
}

void rememberProjectileIgnoreNative(int slot, int identity) {
    if (identity < 0 || identity >= PROJECTILE_PHYSICS_ID_LIMIT_NATIVE)
        return;
    if (identity < MAX_ENTITIES)
        shotContacts[slot].ignored[identity / 32] |= 1u << (identity % 32);
    else {
        int key = identity - MAX_ENTITIES;
        shotContacts[slot].extraIgnored[key / 32] |= 1u << (key % 32);
    }
}

// Shoot clears the native identifier set before scheduling its first-tick
// callback (EnemyBullet0x1bef6c). Do this explicitly, including zero-age
// cross-class reuse whose freshly recreated body is already active.
void resetProjectileLaunchContactsNative(int slot, int owner) {
    memset(shotContacts + slot, 0, sizeof(shotContacts[slot]));
    shotContacts[slot].first = true;
    rememberProjectileIgnoreNative(slot, owner);
}

Vec projectileContactPointNative(b2Contact *contact) {
    b2WorldManifold world;
    contact->GetWorldManifold(&world);
    b2Vec2 point = world.points[0];
    if (contact->GetManifold()->pointCount == 2)
        point = .5f * (point + world.points[1]);
    return Vec{point.x / metres, point.y / metres};
}

void projectilePreSolve(b2Contact *contact) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int slot = (int)fixtures[side]->GetBody()->GetUserData() - 1 - MAX_ENTITIES;
        if (slot < 0 || slot >= MAX_PROJECTILES || bullets[slot].life <= 0)
            continue;
        b2Fixture *other = fixtures[1 - side];
        int identity = projectilePhysicsIdentityNative(other->GetBody());
        if (other->IsSensor() || other->GetUserData() == (void *)1 ||
            other->GetUserData() == (void *)2 || other->GetUserData() == (void *)15) {
            contact->SetEnabled(false);
            continue;
        }
        if (shotContacts[slot].first)
            rememberProjectileIgnoreNative(slot, identity);
        if (shotIgnored(slot, identity)) {
            contact->SetEnabled(false);
            continue;
        }
    }
}

// EnemyBullet::PostSolve 0x1ba4a8 / FinalBossBullet::PostSolve 0x1e3500.
// A KillableEnemy stomp disables PreSolve, so only contacts actually solved
// by Box2D may destroy a projectile as a wall/side hit.
void projectilePostSolve(b2Contact *contact) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (!contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    for (int side = 0; side < 2; side++) {
        int slot = (int)fixtures[side]->GetBody()->GetUserData() - 1 - MAX_ENTITIES;
        if (slot < 0 || slot >= MAX_PROJECTILES || bullets[slot].life <= 0)
            continue;
        Object &source = classes[bullets[slot].klass];
        if (!source.has("EnemyBulletBehaviour") && !source.has("FinalBossBulletBehaviour"))
            continue;
        ShotContacts &hit = shotContacts[slot];
        hit.hit = true;
        hit.hitBean |= fixtures[1 - side]->GetBody() == bodies[playerIndex];
        hit.point = projectileContactPointNative(contact);
    }
}

b2Body *createShotBody(int slot) {
    Bullet &bullet = bullets[slot];
    Object &source = classes[bullet.klass];
    const Props *physics = source.behavior("CirclePhysicsBehaviour");
    b2BodyDef definition;
    definition.type = b2_dynamicBody;
    definition.gravityScale = 0;
    definition.bullet = physics == 0;
    definition.fixedRotation = physics ? physics->flag("fixed-rotation", false) : true;
    definition.position.Set(bullet.state.x * metres, bullet.state.y * metres);
    definition.userData = (void *)(MAX_ENTITIES + slot + 1);
    b2Body *body = physicsWorld->CreateBody(&definition);
    b2CircleShape shape;
    float width = source.rect.r - source.rect.l, height = source.rect.t - source.rect.b;
    if (physics) {
        width += physics->num("adjust-right") - physics->num("adjust-left");
        height += physics->num("adjust-top") - physics->num("adjust-bottom");
    }
    if (physics)
        nativeCircleFixtureGeometry(shape, nativeCirclePrototypeLocal(source, physics));
    else
        shape.m_radius = maxf(1, minf(absf(width), absf(height)) * 0.5f) * metres;
    b2FixtureDef fixture;
    fixture.shape = &shape;
    fixture.density = 1;
    fixture.restitution = 0;
    fixture.friction = .2f;
    if (physics)
        nativeBoxCircleFixtureMaterial(fixture, *physics);
    fixture.filter.categoryBits = 4;
    fixture.filter.maskBits = nativePhysicsMask(physics);
    body->CreateFixture(&fixture);
    shotBodyClasses[slot] = bullet.klass;
    return body;
}

// Native BulletSource owns a fixed-class actor pool (OnActivate0x147dc4).
// The port shares slots between emitters, so its physical fixture must follow
// the slot's class identity as well as its position/velocity. This also lets
// checkpoint restoration rebuild a body before restoring saved contact state.
void ensureShotBodyClass(int slot) {
    if (shotBodies[slot] && shotBodyClasses[slot] != bullets[slot].klass) {
        physicsWorld->DestroyBody(shotBodies[slot]);
        shotBodies[slot] = 0;
    }
    if (!shotBodies[slot])
        shotBodies[slot] = createShotBody(slot);
}

void captureShotContacts(ShotSavedContacts *saved, ShotSavedContacts *extra) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        ShotSavedContacts &out = extra && i >= 64 ? extra[i - 64] : saved[i];
        memcpy(out.ignored, shotContacts[i].ignored, sizeof(out.ignored));
        out.first = shotContacts[i].first ? 1 : 0;
    }
}

void restoreShotContacts(const ShotSavedContacts *saved, const ShotSavedContacts *extra) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        // Construct class-correct fixtures before copying saved identity.
        // A missing body must not make prepareShot reclassify this as launch.
        if (bullets[i].life > 0) {
            ensureShotBodyClass(i);
            shotBodies[i]->SetActive(shotFlyingNative(bullets[i]));
        }
        memset(shotContacts + i, 0, sizeof(shotContacts[i]));
        if (bullets[i].life > 0) {
            const ShotSavedContacts &in = extra && i >= 64 ? extra[i - 64] : saved[i];
            memcpy(shotContacts[i].ignored, in.ignored, sizeof(in.ignored));
            shotContacts[i].first = in.first != 0;
            shotContacts[i].lastTime = bullets[i].state.timer;
        }
    }
}

void captureProjectileExtraIgnoresNative(ProjectileExtraIgnoresNative &saved) {
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        memcpy(saved.targets[slot], shotContacts[slot].extraIgnored,
               sizeof(shotContacts[slot].extraIgnored));
}

void restoreProjectileExtraIgnoresNative(const ProjectileExtraIgnoresNative &saved) {
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        memcpy(shotContacts[slot].extraIgnored, saved.targets[slot],
               sizeof(shotContacts[slot].extraIgnored));
}

void prepareShot(int slot) {
    Bullet &b = bullets[slot];
    if (!shotBodies[slot] || !shotBodies[slot]->IsActive() ||
        b.state.timer < shotContacts[slot].lastTime) {
        memset(&shotContacts[slot], 0, sizeof(shotContacts[slot]));
        shotContacts[slot].first = true;
        int owner = b.state.active - 1;
        if (owner >= 0 && owner < entityCount)
            shotContacts[slot].ignored[owner / 32] |= 1u << (owner % 32);
    }
    shotContacts[slot].lastTime = b.state.timer;
}

void destroyShot(int slot) {
    Bullet &b = bullets[slot];
    if (b.life <= 0)
        return;
    Object &source = classes[b.klass];
    const Props *behaviour = source.behavior("EnemyBulletBehaviour");
    if (!behaviour)
        behaviour = source.behavior("KillableEnemyBehaviour");
    if (behaviour) {
        u64 effect = idnum(
            behaviour->get(source.has("EnemyBulletBehaviour") ? "effect" : "explosion-actor"));
        emitEffect(effect, shotContacts[slot].point.x, shotContacts[slot].point.y,
                   entities[playerIndex].layer);
    }
    b.life = 0;
    if (shotBodies[slot])
        shotBodies[slot]->SetActive(false);
}

void projectilePostStep() {
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (bullets[i].life > 0) {
            Bullet &b = bullets[i];
            Object &source = classes[b.klass];
            if (shotFlyingNative(b)) {
                if (shotContacts[i].hitBean)
                    die();
                if (shotContacts[i].hit)
                    destroyShot(i);
            }
            if (source.has("FinalBossBulletBehaviour") && b.state.timer > 0 &&
                !overlaps(shifted(source.rect, b.state.x, b.state.y),
                          Rect{camX, camY, camX + viewW, camY + viewH})) {
                b.life = 0;
                if (shotBodies[i])
                    shotBodies[i]->SetActive(false);
            }
            const Props *behaviour = source.behavior("EnemyBulletBehaviour");
            if (behaviour && behaviour->flag("will-rotate"))
                b.state.angle = bqAtan2(b.state.vy, b.state.vx) * 57.29578f;
            shotContacts[i].first = false;
        }
}
