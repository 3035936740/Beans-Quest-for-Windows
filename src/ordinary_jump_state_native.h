#pragma once

// Transient ordinary-jump request data passed from solved contact to the following simulation boundary.

// Tom saves +bc pending, +c8 stomp/count exemption, +f4 magnet, +a8
// direction, +c0 dust point and +100 speed independently of its body state.
struct OrdinaryJumpStateNative {
    int pending, stomp, magnet;
    Vec direction, point;
    float speed;
};

bool validOrdinaryJumpStateNative(const OrdinaryJumpStateNative &s) {
    float norm = s.direction.x * s.direction.x + s.direction.y * s.direction.y;
    return (!s.pending || (norm > .999f && norm < 1.001f)) && s.pending >= 0 && s.pending <= 1 &&
           s.stomp >= 0 && s.stomp <= 1 && s.magnet >= 0 && s.magnet <= 1 && s.direction.x >= -1 &&
           s.direction.x <= 1 && s.direction.y >= 0 && s.direction.y <= 1 &&
           s.point.x >= -100000000 && s.point.x <= 100000000 && s.point.y >= -100000000 &&
           s.point.y <= 100000000 && s.speed >= 0 && s.speed <= 100000;
}
