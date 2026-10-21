#pragma once

// Domain-local burst random streams; menu effects cannot disturb campaign random state.
#include "particle_random_native.h"
#include "burst_emitter_native.h"

struct BurstDefinitionRandomNative {
    const Props *definition;
    int emitter;
    NativeParticleNormal lifetime, velocity;
};

struct BurstRandomNative {
    NativeParticleRng engine;
    BurstDefinitionRandomNative definitions[128];
    int count;
    bool initialized;
};

// Campaign/interface lifetimes are isolated. The native level's complete
// cross-subsystem RNG ordering is still open; do not claim seed/frame parity.
BurstRandomNative burstRandomNative[2];

void resetBurstRandomNative(int domain, u32 seed) {
    BurstRandomNative &r = burstRandomNative[domain];
    memset(&r, 0, sizeof(r));
    nativeParticleSeed(r.engine, seed);
    r.initialized = true;
    resetBurstEmittersNative(domain);
}

BurstRandomNative &burstRandomForNative(int domain) {
    if (!burstRandomNative[domain].initialized)
        resetBurstRandomNative(domain, 0x6182ab3);
    return burstRandomNative[domain];
}

BurstDefinitionRandomNative &burstDefinitionRandomNative(BurstRandomNative &r, const Props &p,
                                                         int emitter) {
    for (int i = 0; i < r.count; i++)
        if (r.definitions[i].definition == &p && r.definitions[i].emitter == emitter)
            return r.definitions[i];
    if (r.count == 128)
        fatal("Too many burst particle definitions.");
    BurstDefinitionRandomNative &d = r.definitions[r.count++];
    memset(&d, 0, sizeof(d));
    d.definition = &p;
    d.emitter = emitter;
    return d;
}
