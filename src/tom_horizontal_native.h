#pragma once

// Shared tom horizontal behavior used by the Windows reconstruction.

// Tom::PrePhysics0x311864(right),0x3123c4(left),0x311c9c(no input).
// A launch/contact can leave vx beyond the requested run speed. Native uses
// stop-acceleration for that excess, move-acceleration otherwise.
float nativeTomHorizontalVelocity(float incoming, int direction, float maximum, float moving,
                                  float stopping, float dt) {
    float target = direction * maximum;
    bool excess = direction > 0 ? incoming > target : direction < 0 ? incoming < target : true;
    return approach(incoming, target, (excess ? stopping : moving) * dt);
}
