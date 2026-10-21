#pragma once

// Authored animation state and timing for enemy animation.

// animation() deliberately falls back to the first atlas entry. It cannot be
// used to ask whether an optional directional animation actually exists.
Animation *exactEnemyAnimation(Actor *art, const char *name) {
    if (!art)
        return 0;
    for (int i = 0; i < art->count; i++)
        if (eq(art->animations[i].name, name))
            return art->animations + i;
    return 0;
}

// OscillatingBehaviour::SetConfig 0x28f9ac / 0x28ff0c / 0x29042c /
// 0x290990 creates optional right/left/up/down instances for cardinal rails.
// Missing instances do not replace the actor's existing authored animation.
const char *oscillatorAnimationName(Actor *art, float axisX, float axisY, bool backwards,
                                    const char *authored) {
    float direction = backwards ? -1.0f : 1.0f, x = axisX * direction, y = axisY * direction;
    const char *candidate = x > 0.9f    ? "right"
                            : x < -0.9f ? "left"
                            : y > 0.9f  ? "up"
                            : y < -0.9f ? "down"
                                        : 0;
    return candidate && exactEnemyAnimation(art, candidate) ? candidate : authored;
}
