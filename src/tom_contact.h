#pragma once

// Shared tom contact behavior used by the Windows reconstruction.

// TomPlayerBehaviour::PostSolve 0x312bf4..0x312c24. The velocity is the
// pre-solver body velocity, and normal points from the surface toward Bean.
bool nativeTomCanAutoJump(float incomingVy, float normalY) {
    return incomingVy <= 0 && normalY >= 0.5f;
}

// The original rejects collapsed two-point manifolds before the solver.
// Arguments are in Box2D metres, not pixels (PreSolve0x310108..0x31013c).
bool nativeTomCollapsedManifold(Vec a, Vec b) {
    float x = a.x - b.x, y = a.y - b.y;
    return x * x + y * y <= 0.0002f;
}

// The old b2LoopShape is type3, corresponding to b2ChainShape::e_chain.
// Keep the native branch ordering for its single-point corner suppression
// (0x310070..0x310168). Bounds/point must use the same coordinates, without
// expanding the adjusted core rectangle by the polygon skin radius.
bool nativeTomLoopCorner(Rect adjustedWorldBounds, Vec point, Vec normal) {
    if (point.x >= adjustedWorldBounds.l && point.x <= adjustedWorldBounds.r)
        return false;
    if (point.y <= adjustedWorldBounds.b && !(point.y < adjustedWorldBounds.t))
        return false;
    return normal.x == 0 || normal.y == 0;
}

// Jump()0x312ae4 scales the horizontal normal by authored x-jump-factor;
// a vertical wall never reaches Jump because its normalY fails the test.
Vec nativeTomJumpFactors(float normalX, float xJumpFactor) {
    float x = normalX * xJumpFactor;
    return Vec{x, root(maxf(0, 1 - x * x))};
}
