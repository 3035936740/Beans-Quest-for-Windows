#pragma once

// Saveable projectile-source phase data, separate from transient projectile bodies.

// angle is the retained CURRENT aim in radians. initialized:0 absent,
// 1 pending/legacy unused angle,2 native aiming state (same wire layout).
struct BulletSourceState {
    float clock, interval, angle;
    int initialized;
};

BulletSourceState bulletSourceStates[2048];

// ARM OnUpdate0x146198 adds in binary32 before its strict expiry comparison.
// The optimized x87 build otherwise compares a retained extended result.
void advanceBulletSourceClockNative(BulletSourceState &state, float dt) {
    volatile float clock = state.clock + dt;
    state.clock = clock;
}

struct SourceRandomState {
    u32 values[624];
    int index;
};

SourceRandomState sourceRandom;

void seedSourceRandom(u32 seed) {
    sourceRandom.values[0] = seed;
    for (u32 i = 1; i < 624; i++)
        sourceRandom.values[i] =
            1812433253u * (sourceRandom.values[i - 1] ^ (sourceRandom.values[i - 1] >> 30)) + i;
    sourceRandom.index = 624;
}

u32 sourceRandomWord() {
    if (sourceRandom.index >= 624) {
        for (int i = 0; i < 624; i++) {
            u32 y = (sourceRandom.values[i] & 0x80000000u) |
                    (sourceRandom.values[(i + 1) % 624] & 0x7fffffffu);
            sourceRandom.values[i] =
                sourceRandom.values[(i + 397) % 624] ^ (y >> 1) ^ ((y & 1) ? 0x9908b0dfu : 0);
        }
        sourceRandom.index = 0;
    }
    u32 x = sourceRandom.values[sourceRandom.index++];
    x ^= x >> 11;
    x ^= (x << 7) & 0x9d2c5680u;
    x ^= (x << 15) & 0xefc60000u;
    x ^= x >> 18;
    return x;
}

float sourceInterval(const Props &p) {
    // Original ConfigOptions stores both limits as binary32 before SafeGen
    // compares them. Force that boundary on the x87 build: otherwise parsing
    // the same decimal twice can compare an extended register with a rounded
    // float and consume an MT word for an authored fixed interval (e.g.1.2).
    volatile float parsedLow = p.num("timeMin", 3), parsedHigh = p.num("timeMax", 5);
    float low = parsedLow, high = parsedHigh;
    if (high < low) {
        float swap = low;
        low = high;
        high = swap;
    }
    if (high == low)
        return maxf(0.001f, low);
    float unit = (float)((double)sourceRandomWord() / 4294967296.0);
    return maxf(0.001f, low + (high - low) * unit);
}

void resetBulletSourceAnimation(int index);

void resetBulletSource(int index, const Props &p) {
    resetBulletSourceAnimation(index);
    BulletSourceState &s = bulletSourceStates[index];
    s = BulletSourceState{p.num("timeOffset"), sourceInterval(p), 0, 1};
}
