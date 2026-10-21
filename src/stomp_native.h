#pragma once

// Native stomp direction and launch calculations.
#include "assets.h"

// TomPlayerBehaviour::KilledEnemyFromJump 0x312b50 delegates to Jump
// 0x312a8c. The enemy's jump-boost is added to HEIGHT, then converted to
// launch velocity through UFromSVA(height, 0, gravity), native 0x312794.
float nativeStompLaunch(float jumpHeight, float bonusHeight, float enemyBoost, float gravity) {
    return root(maxf(0, -2 * gravity * (jumpHeight + bonusHeight + enemyBoost)));
}

// Jump 0x312af0..0x312b1c keeps existing horizontal motion, adds only the
// normal's X contribution, and replaces vertical velocity. KillableEnemy
// passes (0,1), so falling momentum never increases an ordinary stomp.
Vec nativeStompVelocity(Vec incoming, Vec normal, float jumpHeight, float bonusHeight,
                        float enemyBoost, float gravity, float xJumpFactor) {
    float speed = nativeStompLaunch(jumpHeight, bonusHeight, enemyBoost, gravity),
          x = normal.x * xJumpFactor;
    return Vec{incoming.x + speed * x, speed * root(maxf(0, 1 - x * x))};
}
