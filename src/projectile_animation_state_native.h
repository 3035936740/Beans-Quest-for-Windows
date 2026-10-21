#pragma once

// State records for projectile animation; live physics ownership stays with the calling scene.
struct ProjectileAnimationMemberNative {
    int owner, klass, ordinal;
    float age;
};

struct ProjectileAnimationStateNative {
    int count;
    ProjectileAnimationMemberNative members[MAX_PROJECTILES];
    int binding[MAX_PROJECTILES];
};

ProjectileAnimationStateNative projectileAnimationsNative;

enum {
    PROJECTILE_EXTRA_IGNORE_WORDS_NATIVE = (MAX_ENTITIES + MAX_PROJECTILES + 31) / 32,
    PROJECTILE_PHYSICS_ID_LIMIT_NATIVE = MAX_ENTITIES * 2 + MAX_PROJECTILES
};

struct ProjectileExtraIgnoresNative {
    u32 targets[MAX_PROJECTILES][PROJECTILE_EXTRA_IGNORE_WORDS_NATIVE];
};

bool validProjectileAnimationStateNative(const ProjectileAnimationStateNative &p, int entities) {
    if (p.count < 0 || p.count > MAX_PROJECTILES)
        return false;
    for (int i = 0; i < p.count; i++) {
        const ProjectileAnimationMemberNative &m = p.members[i];
        if (m.owner < 0 || m.owner >= entities || m.klass < 0 || m.klass >= classCount ||
            m.ordinal < 0 || m.ordinal >= MAX_PROJECTILES || !(m.age >= 0 && m.age < 100000000) ||
            (!classes[m.klass].has("EnemyBulletBehaviour") &&
             !classes[m.klass].has("FinalBossBulletBehaviour")))
            return false;
        for (int n = 0; n < i; n++)
            if (p.members[n].owner == m.owner && p.members[n].klass == m.klass &&
                p.members[n].ordinal == m.ordinal)
                return false;
    }
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (p.binding[i] < -1 || p.binding[i] >= p.count)
            return false;
    return true;
}
