#pragma once

// Native collectable event parameters and shared collection behavior.
#include "assets.h"

// CollectableObjectGroupBehaviour::OnActivate 0x16c6ec..0x16c734 uses
// ActletMatrix::GetActletSize().x, already the half-width, as circle radius.
// CollectableObjectBehaviour likewise inherits CirclePhysicsBehaviour.
float nativeCollectableRadius(Rect visualCell) {
    return (visualCell.r - visualCell.l) * 0.5f;
}

// Equivalent overlap for callers without a physics body. The optional skin
// is the player's polygon radius (native Box2D default 0.01m = 0.64px).
// Bean's adjusted main fixture is the box; its hat artwork is not a sensor.
bool nativeCollectableTouches(Rect playerFixture, Vec position, float radius,
                              float polygonSkin = 0.64f) {
    float x = clamp(position.x, playerFixture.l, playerFixture.r),
          y = clamp(position.y, playerFixture.b, playerFixture.t);
    float dx = position.x - x, dy = position.y - y, total = radius + polygonSkin;
    return dx * dx + dy * dy <= total * total;
}
