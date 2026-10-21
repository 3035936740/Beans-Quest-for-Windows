#pragma once

// Advances enemy death presentation and body retirement through the normal killable-enemy lifecycle.

// Native Killable state1 is the visual death transition; state2 is hidden.
// The port's gone flag remains truthy throughout, so AI and physics stop
// immediately. Value2 distinguishes the still-visible transition from gone1.
// While gone2, the existing saved MotionState.turnCooldown slot stores age.
// Its ordinary nonnegative breakable debounce is inactive on a dead actor.
// This preserves the exact version14 wire layout and every live velocity,
// facing value and animation clock, while retaining the native saved age.
bool killableDeathVisibleNative(const Entity &e) {
    return e.state.gone == 2 && e.source && e.source->has("KillableEnemyBehaviour");
}

float killableDeathAgeNative(const Entity &e) {
    return e.turnCooldown;
}

float killableDeathDurationNative(const Entity &e) {
    const Props *p = e.source->behavior("KillableEnemyBehaviour");
    return p ? p->num("die-time", .1f) : .1f;
}

Rect killableDeathRectNative(const Entity &e, Rect rect) {
    if (!killableDeathVisibleNative(e))
        return rect;
    float duration = killableDeathDurationNative(e), age = killableDeathAgeNative(e);
    float t = duration > 0 ? clamp(age / duration, 0, 1) : 1;
    Vec c = center(rect);
    float x = 1 - .5f * t, y = 1 - t;
    return Rect{c.x + (rect.l - c.x) * x, c.y + (rect.b - c.y) * y, c.x + (rect.r - c.x) * x,
                c.y + (rect.t - c.y) * y};
}
