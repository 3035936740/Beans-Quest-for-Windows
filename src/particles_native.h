#pragma once

// Shared particle simulation and rendering parameters read from the original resources.
#include "particle_geometry_native.h"

Vec nativeParticleTrajectory(Rect emitter, float offsetX, float offsetY, Vec velocity, Vec gravity,
                             float age, float scale) {
    // Spawn 0x2af544..0x2af584 scales local velocity; Update
    // 0x2af004..0x2af044 scales each gravity axis by the stored scale too.
    return Vec{emitter.l + (emitter.r - emitter.l) * offsetX +
                   scale * (velocity.x * age + 0.5f * gravity.x * age * age),
               emitter.b + (emitter.t - emitter.b) * offsetY +
                   scale * (velocity.y * age + 0.5f * gravity.y * age * age)};
}

// Legacy deterministic approximation, retained only for old diagnostic callers.
// Runtime emission uses nativeContinuousStep below: any positive emission
// budget immediately occupies a free slot; capacity is not a target live count.
float continuousParticleAge(int slot, int capacity, float rate, float life, float time) {
    if (rate <= 0 || life <= 0)
        return -1;
    float birth = (slot + 1) / rate;
    if (time < birth)
        return -1;
    float cycle = maxf(life, capacity / rate), age = time - birth;
    age -= (int)(age / cycle) * cycle;
    return age < life ? age : -1;
}

#include "continuous_particles.h"
