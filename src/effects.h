#pragma once

// Allocates, advances and retires transient effect particles without changing authored gameplay entities.
#include "assets.h"
#include "burst_random_native.h"

struct EffectParticle {
    const Props *definition;
    Actor *art;
    int domain, layer, z;
    bool additive, bounded;
    int emitter;
    float x, y, vx, vy, gx, gy, age, life, delay, w, h, angle, spin, alpha, fadeIn, fadeOut,
        animationStart, animationScale;
    float initialX, initialY, initialAngle;
};

enum { MAX_EFFECT_PARTICLES = 2048 };

EffectParticle effectParticles[MAX_EFFECT_PARTICLES];
u32 effectSeed = 0x6182ab3, emittedParticles;

float particleRandom(u32 seed) {
    seed ^= seed >> 16;
    seed *= 0x7feb352d;
    seed ^= seed >> 15;
    seed *= 0x846ca68b;
    seed ^= seed >> 16;
    return (seed & 65535) / 65535.0f;
}

float effectRandom() {
    effectSeed = effectSeed * 1664525 + 1013904223;
    return (effectSeed >> 8) / 16777216.0f;
}

Object *effectClass(u64 id) {
    for (int i = 0; i < classCount; i++)
        if (classes[i].id == id)
            return classes + i;
    return 0;
}

void preloadEffects() {
    for (int i = 0; i < classCount; i++)
        if (classes[i].base == 5)
            actor(classes[i].id);
}

void clearEffects(int domain = 0) {
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].domain == domain)
            effectParticles[i].definition = 0;
}

int liveEffects(int domain = 0) {
    int count = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition && effectParticles[i].domain == domain)
            count++;
    return count;
}

void newEffectParticle(Object &source, const Props &p, float x, float y, int layer, float scale,
                       int domain, int instance = 0) {
    int slot = -1;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (!effectParticles[i].definition) {
            slot = i;
            break;
        }
    if (slot < 0)
        return;
    EffectParticle &q = effectParticles[slot];
    memset(&q, 0, sizeof(q));
    q.definition = &p;
    q.art = actor(source.id);
    q.domain = domain;
    q.layer = layer;
    q.z = source.z;
    q.additive = source.props.flag("additive");
    BurstRandomNative &random = burstRandomForNative(domain);
    q.emitter = positionBurstEmitterNative(source, x, y, scale, domain, instance);
    q.bounded = p.flag("bounded");
    BurstEmitterNative &emitter = burstEmittersNative[domain].emitters[q.emitter];
    BurstDefinitionRandomNative &definition = burstDefinitionRandomNative(random, p, q.emitter);
    float cosine = bqCos(emitter.angle * .017453293f), sine = bqSin(emitter.angle * .017453293f);
    float ox = nativeParticleRange(random.engine, p.num("offset-min-x", p.num("offset-x", .5f)),
                                   p.num("offset-max-x", p.num("offset-x", .5f))),
          oy = nativeParticleRange(random.engine, p.num("offset-min-y", p.num("offset-y", .5f)),
                                   p.num("offset-max-y", p.num("offset-y", .5f)));
    float nx = ox * 2 - 1, ny = oy * 2 - 1;
    ox = (cosine * nx - sine * ny + 1) * .5f;
    oy = (sine * nx + cosine * ny + 1) * .5f;
    q.x = emitter.bounds.l + (emitter.bounds.r - emitter.bounds.l) * ox;
    q.y = emitter.bounds.b + (emitter.bounds.t - emitter.bounds.b) * oy;
    // Spawn 0x2af530 samples lifetime first; GetRandomVelocityVector then
    // samples depth/theta and its own cached Gaussian. Signed values survive.
    q.life =
        p.num("lifetime-mean", 1) +
        p.num("lifetime-stddev", .1f) * nativeParticleGaussian(random.engine, definition.lifetime);
    q.delay = p.num("start-time");
    float depth = nativeParticleRange(random.engine, p.num("depth-min"), p.num("depth-max")),
          theta = nativeParticleRange(random.engine, p.num("theta-min"), p.num("theta-max", 360)) *
                  .017453293f;
    float speed = (p.num("velocity-mean", 1) +
                   p.num("velocity-stddev", .1f) *
                       nativeParticleGaussian(random.engine, definition.velocity)) *
                  root(1 - depth * depth) * scale;
    float vx = bqCos(theta) * speed, vy = bqSin(theta) * speed;
    q.vx = cosine * vx - sine * vy;
    q.vy = sine * vx + cosine * vy;
    q.gx = p.num("gravity-x") * scale;
    q.gy = p.num("gravity-y") * scale;
    q.w = maxf(2, p.num("size-x", 16) * scale);
    q.h = maxf(2, p.num("size-y", 16) * scale);
    q.animationStart = nativeParticleRange(random.engine, p.num("animation-start-min"),
                                           p.num("animation-start-max", 1));
    q.angle = nativeParticleRange(random.engine, p.num("start-angle-min"),
                                  p.num("start-angle-max", 360)) +
              emitter.angle - 90;
    q.spin = nativeParticleRange(random.engine, p.num("angular-velocity-min", -360),
                                 p.num("angular-velocity-max", 360));
    q.alpha = p.num("alpha", 1);
    q.fadeIn = p.num("fadein-end");
    q.fadeOut = p.num("fadeout-start", 1);
    q.animationScale = p.num("animation-scale", 1);
    emittedParticles++;
    q.initialX = q.x;
    q.initialY = q.y;
    q.initialAngle = q.angle;
}

void updateEffectParticleNative(EffectParticle &p, float dt);

void emitEffect(u64 id, float x, float y, int layer, float scale = 1, int domain = 0,
                int requested = -1, int instance = 0) {
    Object *source = effectClass(id);
    if (!source || source->base != 5)
        return;
    burstRandomForNative(domain);
    int emitter = positionBurstEmitterNative(*source, x, y, scale, domain, instance);
    // LevelTask::FindOrSpawnObjectByClassId (0x26f5e8) shares these burst
    // effects by class within a scene, even when callers use different layers.
    // ParticleEffect::Explode (0x2aa9f4) skips living slots and drops overflow;
    // its maximum is the whole live pool, not a fresh allowance per explosion.
    int count = requested >= 0 ? requested : (int)source->props.num("explosion-size", 1),
        cap = (int)source->props.num("max-particles", 100), alive = 0, free = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
        const EffectParticle &q = effectParticles[i];
        if (!q.definition)
            free++;
        else if (q.domain == domain && q.art && q.art->id == id && q.emitter == emitter)
            alive++;
    }
    count = (int)clamp((float)count, 0, minf((float)free, maxf(0, (float)cap - alive)));
    int used = 0;
    float total = 0;
    for (int i = 0; i < source->behaviorCount; i++)
        if (eq(source->behaviors[i].type, "ParticleDefSettings")) {
            const Props &p = source->behaviors[i].props;
            total += maxf(0, p.num("spawn-priority", 1));
            int n = (int)p.num("explosion-min-spawn");
            for (int j = 0; j < n && used < count; j++, used++)
                newEffectParticle(*source, p, x, y, layer, scale, domain, instance);
        }
    while (used < count && total > 0) {
        float choice = nativeParticleRange(burstRandomForNative(domain).engine, 0, total);
        for (int i = 0; i < source->behaviorCount; i++)
            if (eq(source->behaviors[i].type, "ParticleDefSettings")) {
                const Props &p = source->behaviors[i].props;
                choice -= maxf(0, p.num("spawn-priority", 1));
                if (choice <= 0) {
                    newEffectParticle(*source, p, x, y, layer, scale, domain, instance);
                    break;
                }
            }
        used++;
    }
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
        EffectParticle &q = effectParticles[i];
        if (q.definition && q.domain == domain && q.art && q.art->id == id && q.emitter == emitter)
            updateEffectParticleNative(q, 0);
    }
}

void updateEffectParticleNative(EffectParticle &p, float dt) {
    p.age += dt;
    float time = p.age - p.delay;
    if (time < 0)
        return;
    if (time > p.life) {
        p.definition = 0;
        return;
    }
    p.x = p.initialX + time * (p.vx + 0.5f * p.gx * time);
    p.y = p.initialY + time * (p.vy + 0.5f * p.gy * time);
    if (p.bounded &&
        !nativeParticleInsideBounds(
            Vec{p.x, p.y}, burstEmittersNative[p.domain].emitters[p.emitter].bounds, true)) {
        p.definition = 0;
        return;
    }
    p.angle = p.initialAngle + p.spin * time;
}

void tickEffects(float dt, int domain = 0) {
    // Native launch state remains immutable, including signed Gaussian speed.
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
        EffectParticle &p = effectParticles[i];
        if (p.definition && p.domain == domain)
            updateEffectParticleNative(p, dt);
    }
}
