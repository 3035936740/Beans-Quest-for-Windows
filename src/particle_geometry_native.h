#pragma once

// Computes authored particle emission geometry and coordinate transforms.
float nativeParticleRoundedPosition(float value) {
    volatile float adjusted = value + (value < 0 ? -.5f : .5f);
    return (float)(int)adjusted;
}

// GetAABB 0x2aa7c8 rounds the emitter position and takes the largest
// absolute rotated half-axis contribution on each axis (not their sum).
Rect nativeParticleEmitterBounds(Rect authored, float scale, float angle = 0) {
    Vec c = center(authored);
    c.x = nativeParticleRoundedPosition(c.x);
    c.y = nativeParticleRoundedPosition(c.y);
    float cosine = bqCos(angle * .017453293f), sine = bqSin(angle * .017453293f),
          x = (authored.r - authored.l) * .5f * scale, y = (authored.t - authored.b) * .5f * scale;
    float hx = maxf(absf(cosine * x), absf(sine * y)), hy = maxf(absf(sine * x), absf(cosine * y));
    return Rect{c.x - hx, c.y - hy, c.x + hx, c.y + hy};
}

bool nativeParticleInsideBounds(Vec position, Rect emitter, bool bounded) {
    return !bounded || (position.x >= emitter.l && position.x <= emitter.r &&
                        position.y >= emitter.b && position.y <= emitter.t);
}
