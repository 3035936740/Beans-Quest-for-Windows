#pragma once

// Native world-selector pointer drag, target position and focus calculations.

// WorldSelectController::OnUpdate 0x339060: six224-unit slots,2048units/s,
// a five-camera-unit drag threshold,50*log(1+.02*overflow) edge resistance,
// and a <=.2second flick when rounding would otherwise retain the same slot.
// Keep pointer positions in physical pixels: the native threshold is scaled,
// but the one-time anchor correction is exactly five physical pixels.
struct WorldSelectPointerNative {
    float anchorX, x, age;
    bool down, justDown, dragged;
};

WorldSelectPointerNative worldSelectPointerNative;

void worldSelectBeginNative(WorldSelectPointerNative &p, float x) {
    p = WorldSelectPointerNative{};
    p.anchorX = p.x = x;
    p.down = p.justDown = true;
}

float worldSelectDragNative(WorldSelectPointerNative &p, float dt, float scale) {
    if (!p.down)
        return 0;
    if (p.justDown) {
        p.justDown = false;
        return 0;
    }
    p.age += maxf(0, dt);
    float displacement = (p.anchorX - p.x) / scale;
    if (!p.dragged && absf(displacement) > 5) {
        if (p.anchorX < p.x)
            p.anchorX += 5;
        else if (p.anchorX > p.x)
            p.anchorX -= 5;
        p.dragged = true;
    }
    return p.dragged ? (p.anchorX - p.x) / scale : 0;
}

int worldSelectReleaseNative(WorldSelectPointerNative &p, int slot, float x, float scale) {
    int result = slot;
    if (p.dragged) {
        float displacement = (p.anchorX - x) / scale / 224;
        int delta = (int)(displacement >= 0 ? displacement + .5f : displacement - .5f);
        result = (int)clamp((float)(slot + delta), 1, 6);
        if (result == slot && p.age <= .2f) {
            if (x > p.anchorX)
                result = (int)maxf(1, (float)slot - 1);
            else if (x < p.anchorX)
                result = (int)minf(6, (float)slot + 1);
        }
    }
    p.down = p.justDown = false;
    return result;
}

double worldSelectLogNative(double x) {
    double result;
    __asm {fldln2}
    __asm {
        fld x
    }
    __asm {fyl2x}
    __asm {
        fstp result
    }
    return result;
}

float worldSelectTargetNative(int slot, float drag) {
    float position = (slot - 1) * 224.0f + drag;
    if (position < 0) {
        float argument = 1 - position * .02f;
        return (float)(-50.0 * worldSelectLogNative(argument));
    }
    if (position > 1120) {
        float argument = 1 + (position - 1120) * .02f;
        return (float)(1120.0 + 50.0 * worldSelectLogNative(argument));
    }
    return position;
}
