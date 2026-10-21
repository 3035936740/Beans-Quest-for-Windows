#pragma once

// Preserves authored source animation and movement phase across state transitions.

// Angle::Normalize0x3a86cc / TurnTowards0x3a8850 use radians in[-pi,pi].
float sourceAngleNative(float angle) {
    const float pi = 3.14159265358979323846f;
    while (angle > pi)
        angle -= 2 * pi;
    while (angle < -pi)
        angle += 2 * pi;
    return angle;
}

float sourceTurnNative(float current, float desired, float dt, float speed) {
    volatile float radians = dt * .017453292519943295f;
    volatile float step = radians * speed;
    float delta = sourceAngleNative(current - desired);
    return absf(delta) <= step ? desired : sourceAngleNative(current + (delta < 0 ? step : -step));
}

bool sourceAimCloseNative(float a, float b) {
    // OnUpdate0x14629c compares signed float representations within100ULP.
    int x, y;
    memcpy(&x, &a, 4);
    memcpy(&y, &b, 4);
    if (x < 0)
        x = (int)(0x80000000u - (u32)x);
    if (y < 0)
        y = (int)(0x80000000u - (u32)y);
    u32 distance = x >= y ? (u32)x - (u32)y : (u32)y - (u32)x;
    return distance <= 100;
}

float sourceDesiredNative(const Entity &e, const Props &p, const State *target, float retained) {
    float x = 0, y = 0;
    const char *mode = p.get("targetingMode", "static");
    bool valid = true;
    if (eq(mode, "target")) {
        if (target) {
            x = target->x - e.state.x;
            y = target->y - e.state.y;
        } else
            valid = false;
    } else if (eq(mode, "velocity")) {
        x = e.state.vx;
        y = e.state.vy;
    } else if (eq(mode, "static")) {
        x = p.num("targetX");
        y = p.num("targetY");
        valid = x != 0 || y != 0;
    } else
        valid = false;
    float desired = valid ? bqAtan2(y, x) : retained;
    const char *allowed = p.get("targetValidAngles");
    if (*allowed) {
        float best = 3.402823466e38f, picked = desired;
        for (const char *value = allowed; *value;) {
            float candidate = sourceAngleNative(number(value) * .017453292519943295f),
                  difference = absf(sourceAngleNative(desired - candidate));
            if (difference < best) {
                best = difference;
                picked = candidate;
            }
            while (*value && *value != ',')
                value++;
            if (*value)
                value++;
        }
        desired = picked;
    }
    return desired;
}

const State *sourceTargetNative(const Props &p) {
    u64 id = idnum(p.get("targetClass"));
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->klass == id)
            return &entities[i].state;
    return 0;
}

void alignBulletSourceAimNative(Entity &e, const Props &p, BulletSourceState &state,
                                const State *target) {
    // OnActivate0x148f20..30 copies the initial desired angle into current.
    // Old checkpoints used initialized=1 and never wrote a meaningful angle.
    // Value2 identifies retained radians without changing the16byte wire layout.
    state.angle = sourceDesiredNative(e, p, target, 0);
    state.initialized = 2;
}

void alignBulletSourcesNative(bool pendingOnly = false) {
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        const Props *p = e.source->behavior("BulletSource");
        if (p && (!pendingOnly || bulletSourceStates[i].initialized < 2))
            alignBulletSourceAimNative(e, *p, bulletSourceStates[i], sourceTargetNative(*p));
    }
}

float trackBulletSourceAimNative(Entity &e, const Props &p, BulletSourceState &state,
                                 const State *target, float dt) {
    if (state.initialized < 2)
        alignBulletSourceAimNative(e, p, state, target);
    float desired = sourceDesiredNative(e, p, target, state.angle);
    state.angle = sourceTurnNative(state.angle, desired, dt, p.num("turnSpeed", 90));
    if (p.flag("faceTarget"))
        e.state.angle = state.angle * 57.29577951308232f;
    return desired;
}
