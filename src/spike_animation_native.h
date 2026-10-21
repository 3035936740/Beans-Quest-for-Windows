#pragma once

// Authored animation state and timing for spike.

// Native SpikeGroup +0x68 is the desired on bit. Four per-actlet instances
// are retained: idle/off/open/close; only the attached instance advances.
enum SpikeAnimationState { SPIKE_OFF = 0, SPIKE_IDLE = 1, SPIKE_OPENING = 2, SPIKE_CLOSING = 3 };

float spikeWrappedAnimationTime(float time, float length) {
    if (length <= 0)
        return 0;
    time -= (int)(time / length) * length;
    if (time < 0)
        time += length;
    return time;
}

const char *spikeAnimationName(const Entity &e) {
    return e.state.active == SPIKE_OPENING   ? "open"
           : e.state.active == SPIKE_CLOSING ? "close"
           : e.state.hp                      ? "idle"
                                             : "off";
}

void resetSpikeAnimation(Entity &e, int bits) {
    if (e.kind != SPIKE)
        return;
    bool on = e.alternation != 0;
    if (bits & e.mask)
        on = !on;
    float length = animationLength(animation(e.art, "idle"));
    // OnActivate0x2fa494..4fc phases each cell by its world centre y-x.
    e.state.aux = spikeWrappedAnimationTime((e.originY - e.originX) / 200, length);
    e.state.hp = on ? 1 : 0;
    e.state.active = on ? SPIKE_IDLE : SPIKE_OFF;
    e.state.timer = on ? e.state.aux : 0;
}

void alternateSpikeAnimation(Entity &e, int eventMask) {
    if (e.kind != SPIKE || !(e.mask & eventMask))
        return;
    if (e.state.active == SPIKE_IDLE)
        e.state.aux = e.state.timer;
    e.state.hp = e.state.hp ? 0 : 1;
    e.state.active = e.state.hp ? SPIKE_OPENING : SPIKE_CLOSING;
    e.state.timer = 0;
}

void advanceSpikeAnimation(Entity &e, float dt) {
    if (e.kind != SPIKE)
        return;
    e.state.timer += maxf(0, dt);
    float length = animationLength(animation(e.art, spikeAnimationName(e)));
    if (e.state.active == SPIKE_OPENING || e.state.active == SPIKE_CLOSING) {
        // Native STOP reaches its end at equality, then SpikeGroup::OnUpdate
        // swaps the instance without advancing the newly attached idle.
        if (length <= 0 || e.state.timer >= length) {
            e.state.active = e.state.hp ? SPIKE_IDLE : SPIKE_OFF;
            e.state.timer = e.state.hp ? e.state.aux : 0;
        }
    } else {
        e.state.timer = spikeWrappedAnimationTime(e.state.timer, length);
        if (e.state.active == SPIKE_IDLE)
            e.state.aux = e.state.timer;
    }
}
