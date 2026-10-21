#pragma once

// Particle random distributions and state used to preserve authored emission behavior.
// ParticleDef uses MT19937 words converted to binary32 before multiplication
// by 2^-32 (0x2a4978), retrying a rounded upper endpoint. The normal sampler
// caches the sine partner per definition, separately for lifetime and speed.
struct NativeParticleRng {
    u32 state[624];
    int index;
};

void nativeParticleSeed(NativeParticleRng &r, u32 seed) {
    r.state[0] = seed;
    for (int i = 1; i < 624; i++)
        r.state[i] = 1812433253u * (r.state[i - 1] ^ (r.state[i - 1] >> 30)) + (u32)i;
    r.index = 624;
}

u32 nativeParticleBits(NativeParticleRng &r) {
    if (r.index >= 624) {
        for (int i = 0; i < 624; i++) {
            u32 x = (r.state[i] & 0x80000000u) | (r.state[(i + 1) % 624] & 0x7fffffffu);
            r.state[i] = r.state[(i + 397) % 624] ^ (x >> 1) ^ ((x & 1) ? 0x9908b0dfu : 0);
        }
        r.index = 0;
    }
    u32 x = r.state[r.index++];
    x ^= x >> 11;
    x ^= (x << 7) & 0x9d2c5680u;
    x ^= (x << 15) & 0xefc60000u;
    return x ^ (x >> 18);
}

float nativeParticleUniform(NativeParticleRng &r) {
    for (;;) {
        volatile float word = (float)(double)nativeParticleBits(r);
        float value = word * (1.0f / 4294967296.0f);
        if (value < 1)
            return value;
    }
}

float nativeParticleRange(NativeParticleRng &rng, float a, float b) {
    if (b < a) {
        float old = a;
        a = b;
        b = old;
    }
    if (a == b)
        return a;
    for (;;) {
        volatile float value = a + (b - a) * nativeParticleUniform(rng);
        if (value < b)
            return value;
    }
}

float nativeParticleLog(float x) {
    float result;
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

struct NativeParticleNormal {
    float spare;
    bool ready;
};

float nativeParticleGaussian(NativeParticleRng &r, NativeParticleNormal &n) {
    if (n.ready) {
        n.ready = false;
        return n.spare;
    }
    float theta = 6.283185307f * nativeParticleUniform(r),
          radius = root(-2 * nativeParticleLog(1 - nativeParticleUniform(r)));
    n.spare = radius * bqSin(theta);
    n.ready = true;
    return radius * bqCos(theta);
}
