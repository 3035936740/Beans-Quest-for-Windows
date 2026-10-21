#pragma once

// Projectile-source animation timing and state transitions.

// BulletSource::OnUpdate0x145f04; native actor animation advances first in
// Actor::PreUpdate0xe61e0..6214. Snapshot v12 retains these independent clocks.
enum BulletSourceVisualPhase {
    SOURCE_VISUAL_DEFAULT,
    SOURCE_VISUAL_IDLE,
    SOURCE_VISUAL_PRE,
    SOURCE_VISUAL_POST
};

struct BulletSourceAnimationNativeState {
    float idleAge, preAge, postAge;
    int phase, initialized, reconstructed;
};

BulletSourceAnimationNativeState bulletSourceAnimations[MAX_ENTITIES];

Animation *bulletSourceExactAnimation(Actor *art, const char *name) {
    if (!art)
        return 0;
    for (int i = 0; i < art->count; i++)
        if (eq(art->animations[i].name, name))
            return art->animations + i;
    return 0;
}

void resetBulletSourceAnimation(int index) {
    if (index < 0 || index >= MAX_ENTITIES)
        return;
    memset(bulletSourceAnimations + index, 0, sizeof(bulletSourceAnimations[index]));
    bulletSourceAnimations[index].initialized = 1;
}

void resetBulletSourceAnimations() {
    memset(bulletSourceAnimations, 0, sizeof(bulletSourceAnimations));
}

int bulletSourceVisualIndex(const Entity &e) {
    size_t address = (size_t)&e, base = (size_t)entities;
    if (address < base || address >= base + entityCount * sizeof(Entity) ||
        (address - base) % sizeof(Entity))
        return -1;
    return (int)((address - base) / sizeof(Entity));
}

void nativeBulletSourceAnimationBefore(BulletSourceAnimationNativeState &state, Actor *art,
                                       float clock, float interval, float dt) {
    if (!state.initialized) {
        memset(&state, 0, sizeof(state));
        state.initialized = 1;
    }
    Animation *idle = bulletSourceExactAnimation(art, "idle-shoot");
    Animation *pre = bulletSourceExactAnimation(art, "pre-shoot");
    Animation *post = bulletSourceExactAnimation(art, "post-shoot");
    // Only the current AnimationInstance receives Actor::PreUpdate's dt.
    // Idle instances are retained across shots, whereas post is Restarted.
    if (state.phase == SOURCE_VISUAL_IDLE) {
        float length = animationLength(idle);
        if (length > 0) {
            state.idleAge += dt;
            if (state.idleAge >= length)
                state.idleAge -= length;
        }
    }
    if (state.phase == SOURCE_VISUAL_POST) {
        state.postAge += dt;
        if (post && state.postAge >= animationLength(post)) {
            state.postAge = animationLength(post);
            if (idle)
                state.phase = SOURCE_VISUAL_IDLE;
        }
    }
    // 0x146104..146158 compares the OLD clock, before0x146188 advances it.
    // SetTime is a raw write; STOP determines the displayed endpoint.
    if (pre) {
        float length = animationLength(pre);
        if (clock >= interval - length) {
            state.preAge = clock - interval + length;
            state.phase = SOURCE_VISUAL_PRE;
        }
    }
}

void nativeBulletSourceAnimationFired(BulletSourceAnimationNativeState &state, Actor *art) {
    if (bulletSourceExactAnimation(art, "post-shoot")) {
        state.postAge = 0;
        state.phase = SOURCE_VISUAL_POST;
    } else if (bulletSourceExactAnimation(art, "idle-shoot"))
        state.phase = SOURCE_VISUAL_IDLE;
    state.initialized = 1;
    state.reconstructed = 0;
}

void bulletSourceAnimationBeforeUpdate(Entity &e, float dt) {
    int index = bulletSourceVisualIndex(e);
    if (index < 0)
        return;
    BulletSourceState &source = bulletSourceStates[index];
    nativeBulletSourceAnimationBefore(bulletSourceAnimations[index], e.art, source.clock,
                                      source.interval, dt);
}

void bulletSourceAnimationAfterFire(Entity &e) {
    int index = bulletSourceVisualIndex(e);
    if (index >= 0)
        nativeBulletSourceAnimationFired(bulletSourceAnimations[index], e.art);
}

bool bulletSourceAnimationPose(const Entity &e, const char **name, float *time,
                               AnimationEndAction *action) {
    if (!e.source->has("BulletSource"))
        return false;
    int index = bulletSourceVisualIndex(e);
    if (index < 0)
        return false;
    const BulletSourceAnimationNativeState &state = bulletSourceAnimations[index];
    if (!state.initialized)
        return false;
    if (state.phase == SOURCE_VISUAL_IDLE) {
        *name = "idle-shoot";
        *time = state.idleAge;
        *action = ANIMATION_LOOP;
        return true;
    }
    if (state.phase == SOURCE_VISUAL_PRE) {
        *name = "pre-shoot";
        *time = state.preAge;
        *action = ANIMATION_STOP;
        return true;
    }
    if (state.phase == SOURCE_VISUAL_POST) {
        *name = "post-shoot";
        *time = state.postAge;
        *action = ANIMATION_STOP;
        return true;
    }
    return false;
}

// Explicit best-effort visual fallback for existing snapshots, NOT exact
// original AnimationInstance restoration. Old clock alone cannot distinguish
// a paused stale pre-shoot from idle, nor recover arbitrary post/idle ages.
// Returning false makes that information loss visible to callers/tests.
bool reconstructBulletSourceAnimationFromClock(int index) {
    if (index < 0 || index >= entityCount)
        return false;
    resetBulletSourceAnimation(index);
    Entity &e = entities[index];
    BulletSourceAnimationNativeState &state = bulletSourceAnimations[index];
    BulletSourceState &source = bulletSourceStates[index];
    state.reconstructed = 1;
    Animation *pre = bulletSourceExactAnimation(e.art, "pre-shoot");
    if (pre && source.clock >= source.interval - animationLength(pre)) {
        state.phase = SOURCE_VISUAL_PRE;
        state.preAge = source.clock - source.interval + animationLength(pre);
    } else if (bulletSourceExactAnimation(e.art, "idle-shoot"))
        state.phase = SOURCE_VISUAL_IDLE;
    return false;
}
