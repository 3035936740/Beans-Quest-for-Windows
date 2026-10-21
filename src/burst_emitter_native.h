#pragma once

// Initializes authored burst particles and their per-particle runtime data.
#include "particle_geometry_native.h"

struct BurstEmitterNative {
    u64 klass;
    int instance;
    Rect bounds;
    float angle;
};

struct BurstEmittersNative {
    BurstEmitterNative emitters[64];
    int count;
};

BurstEmittersNative burstEmittersNative[2];

void resetBurstEmittersNative(int domain) {
    memset(burstEmittersNative + domain, 0, sizeof(BurstEmittersNative));
}

int positionBurstEmitterNative(Object &source, float x, float y, float scale, int domain,
                               int instance) {
    BurstEmittersNative &scene = burstEmittersNative[domain];
    int index = -1;
    for (int i = 0; i < scene.count; i++)
        if (scene.emitters[i].klass == source.id && scene.emitters[i].instance == instance) {
            index = i;
            break;
        }
    if (index < 0) {
        if (scene.count == 64)
            fatal("Too many burst emitters.");
        index = scene.count++;
        scene.emitters[index].klass = source.id;
        scene.emitters[index].instance = instance;
    }
    float hx = (source.rect.r - source.rect.l) * .5f, hy = (source.rect.t - source.rect.b) * .5f;
    BurstEmitterNative &e = scene.emitters[index];
    e.bounds =
        nativeParticleEmitterBounds(Rect{x - hx, y - hy, x + hx, y + hy}, scale, source.angle);
    e.angle = source.angle;
    return index;
}
