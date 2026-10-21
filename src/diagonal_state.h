#pragma once

// State records for diagonal; live physics ownership stays with the calling scene.

// DiagonalBehaviour SaveState 0x19da2c: float2 direction, effective speed,
// first-frame performance decision. No pointer or platform-sized field.
struct DiagonalRuntimeState {
    float directionX, directionY, effectiveSpeed;
    int selected;
};

DiagonalRuntimeState diagonalRuntime[MAX_ENTITIES];

// OscillatingBehaviour stores a clock independent of its solved body position.
// SaveState 0x28c48c persists origin, direction, bounds, speed and this clock;
// authored immutable fields remain in Entity, mutable fields live here.
struct OscillatorRuntimeState {
    float clock, effectiveSpeed;
    int selected, initialized;
};

OscillatorRuntimeState oscillatorRuntime[MAX_ENTITIES];
float nativeMotionRenderFPS = 60.0f;
