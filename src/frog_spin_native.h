#pragma once

// Shared frog spin behavior used by the Windows reconstruction.
#include "burst_random_native.h"

// FrogSpecialBall::Recalculate0x1f5db0..5e58 samples [-3*pi,+3*pi)
// from LevelTask's MT19937. Angular velocity is stored in radians/sec,
// with no degrees conversion after the draw (body+0x50 at0x1f6150).
float nativeFrogSpinNative(int domain) {
    NativeParticleRng &random = burstRandomForNative(domain).engine;
    volatile float high = 3.0f * 3.1415927410125732f, low = -3.0f * 3.1415927410125732f,
                   span = high - low;
    for (;;) {
        volatile float word = (float)(double)nativeParticleBits(random);
        volatile float unit = word * 2.3283064365386962890625e-10f;
        volatile float product = unit * span, value = low + product;
        if (value < high)
            return value;
    }
}
