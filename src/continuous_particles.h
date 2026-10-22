#pragma once

// Advances authored continuous emitters independently of short-lived burst effects.
#include "particle_random_native.h"

// Stateful port of ParticleEffect::Update 0x2ae894 and ParticleInstance
// Spawn/Update. Original effects share a level MT19937; per-emitter seeds
// here preserve the distribution, not the original cross-object RNG order.
struct NativeContinuousDef {
    const Props *props;
    const char *animation;
    float weight, lifeMean, lifeStd, velocityMean, velocityStd, depthLo, depthHi, thetaLo, thetaHi,
        oxLo, oxHi, oyLo, oyHi, startLo, startHi, angleLo, angleHi, spinLo, spinHi, delay, alpha,
        fadeIn, fadeOut, animationScale;
    Vec gravity, size;
    bool bounded;
    NativeParticleNormal lifeNormal, velocityNormal;
};

struct NativeContinuousParticle {
    int definition;
    float age, life, animationStart, angle, spin;
    Vec initial, velocity, gravity, size;
    u32 generation;
};

struct NativeContinuousPool {
    Object *source;
    u64 id;
    NativeContinuousParticle *particles;
    int capacity, definitionCount;
    NativeContinuousDef definitions[32];
    NativeParticleRng random;
    float budget, lastTime, totalWeight;
    u32 births, deaths, used;
    bool configured, updated;
};

NativeContinuousPool nativeContinuousPools[64];
u32 nativeContinuousUse;

void nativeContinuousConfigure(NativeContinuousPool &pool, Object &source) {
    if (pool.particles)
        HeapFree(GetProcessHeap(), 0, pool.particles);
    memset(&pool, 0, sizeof(pool));
    pool.source = &source;
    pool.id = source.id;
    pool.configured = true;
    pool.capacity = (int)clamp(source.props.num("max-particles", 200), 0, MAX_EFFECT_PARTICLES);
    pool.budget = source.props.num("initial-time");
    nativeParticleSeed(pool.random, (u32)source.id ^ (u32)(source.id >> 32) ^ 0x6182ab3u);
    if (pool.capacity) {
        pool.particles = (NativeContinuousParticle *)HeapAlloc(
            GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NativeContinuousParticle) * pool.capacity);
        if (!pool.particles)
            fatal("Particle pool allocation failed.");
        for (int i = 0; i < pool.capacity; i++)
            pool.particles[i].definition = -1;
    }
    for (int i = 0; i < source.behaviorCount; i++)
        if (eq(source.behaviors[i].type, "ParticleDefSettings")) {
            if (pool.definitionCount == 32)
                fatal("Too many particle definitions.");
            NativeContinuousDef &d = pool.definitions[pool.definitionCount++];
            const Props &p = source.behaviors[i].props;
            d.props = &p;
            d.animation = p.get("animation");
            d.weight = maxf(0, p.num("spawn-priority", 1));
            pool.totalWeight += d.weight;
            d.lifeMean = p.num("lifetime-mean", 1);
            d.lifeStd = p.num("lifetime-stddev", .1f);
            d.velocityMean = p.num("velocity-mean", 1);
            d.velocityStd = p.num("velocity-stddev", .1f);
            d.depthLo = p.num("depth-min");
            d.depthHi = p.num("depth-max");
            d.thetaLo = p.num("theta-min");
            d.thetaHi = p.num("theta-max", 360);
            d.oxLo = p.num("offset-min-x", p.num("offset-x", 0.5f));
            d.oxHi = p.num("offset-max-x", p.num("offset-x", 0.5f));
            d.oyLo = p.num("offset-min-y", p.num("offset-y", 0.5f));
            d.oyHi = p.num("offset-max-y", p.num("offset-y", 0.5f));
            d.startLo = p.num("animation-start-min");
            d.startHi = p.num("animation-start-max", 1);
            d.angleLo = p.num("start-angle-min");
            d.angleHi = p.num("start-angle-max", 360);
            d.spinLo = p.num("angular-velocity-min", -360);
            d.spinHi = p.num("angular-velocity-max", 360);
            d.delay = p.num("start-time");
            d.alpha = p.num("alpha", 1);
            d.fadeIn = p.num("fadein-end");
            d.fadeOut = p.num("fadeout-start", 1);
            d.animationScale = p.num("animation-scale", 1);
            d.gravity = Vec{p.num("gravity-x"), p.num("gravity-y")};
            d.size = Vec{p.num("size-x", 16), p.num("size-y", 16)};
            d.bounded = p.flag("bounded");
        }
}

Vec nativeContinuousPosition(const NativeContinuousParticle &particle,
                             const NativeContinuousDef &d) {
    float age = particle.age - d.delay;
    return Vec{
        particle.initial.x + particle.velocity.x * age + 0.5f * particle.gravity.x * age * age,
        particle.initial.y + particle.velocity.y * age + 0.5f * particle.gravity.y * age * age};
}

bool nativeContinuousAlive(NativeContinuousPool &pool, NativeContinuousParticle &p, Rect bounds) {
    if (p.definition < 0)
        return false;
    NativeContinuousDef &d = pool.definitions[p.definition];
    float age = p.age - d.delay;
    if (age < 0)
        return true;
    if (age > p.life ||
        !nativeParticleInsideBounds(nativeContinuousPosition(p, d), bounds, d.bounded)) {
        p.definition = -1;
        pool.deaths++;
        return false;
    }
    return true;
}

void nativeContinuousSpawn(NativeContinuousPool &pool, NativeContinuousParticle &p, Rect bounds,
                           float scale, float rotation) {
    float choice = nativeParticleUniform(pool.random) * pool.totalWeight;
    int index = -1;
    for (int i = 0; i < pool.definitionCount; i++)
        if (pool.definitions[i].weight > 0) {
            index = i;
            choice -= pool.definitions[i].weight;
            if (choice < 0)
                break;
        }
    if (index < 0)
        return;
    NativeContinuousDef &d = pool.definitions[index];
    p.definition = index;
    p.generation = ++pool.births;
    float ox = nativeParticleRange(pool.random, d.oxLo, d.oxHi),
          oy = nativeParticleRange(pool.random, d.oyLo, d.oyHi),
          cos = bqCos(rotation * 0.017453293f), sin = bqSin(rotation * 0.017453293f);
    float nx = ox * 2 - 1, ny = oy * 2 - 1;
    ox = (cos * nx - sin * ny + 1) * 0.5f;
    oy = (sin * nx + cos * ny + 1) * 0.5f;
    p.initial = Vec{bounds.l + (bounds.r - bounds.l) * ox, bounds.b + (bounds.t - bounds.b) * oy};
    p.life = d.lifeMean + d.lifeStd * nativeParticleGaussian(pool.random, d.lifeNormal);
    float depth = nativeParticleRange(pool.random, d.depthLo, d.depthHi),
          theta = nativeParticleRange(pool.random, d.thetaLo, d.thetaHi) * 0.017453293f,
          speed = (d.velocityMean +
                   d.velocityStd * nativeParticleGaussian(pool.random, d.velocityNormal)) *
                  root(maxf(0, 1 - depth * depth));
    Vec velocity = {bqCos(theta) * speed * scale, bqSin(theta) * speed * scale};
    p.velocity = Vec{cos * velocity.x - sin * velocity.y, sin * velocity.x + cos * velocity.y};
    p.gravity = Vec{d.gravity.x * scale, d.gravity.y * scale};
    p.size = Vec{maxf(2, d.size.x * scale), maxf(2, d.size.y * scale)};
    p.animationStart = nativeParticleRange(pool.random, d.startLo, d.startHi);
    p.angle = nativeParticleRange(pool.random, d.angleLo, d.angleHi) + rotation;
    p.spin = nativeParticleRange(pool.random, d.spinLo, d.spinHi);
    p.age = 0;
}

void nativeContinuousStep(NativeContinuousPool &pool, Rect bounds, float scale, float rotation,
                          float dt) {
    float rate = pool.source->props.num("particles-per-second", 10);
    bool emitting = pool.source->props.flag("emitting", true) && rate > 0 && pool.totalWeight > 0;
    float interval = emitting ? 1 / rate : 0;
    bool weatherBirds = pool.source->klass == 697074819893ULL &&
                        pool.source->props.flag("maker-weather");
    if (emitting)
        pool.budget += dt;
    else
        pool.budget = 0;
    // Update every occupied slot first. A dead slot can be re-used in this
    // same pass; a bounded death is permanent even if its curve re-enters.
    for (int i = 0; i < pool.capacity; i++) {
        NativeContinuousParticle &p = pool.particles[i];
        if (p.definition >= 0)
            p.age += dt;
        // Recycle custom-weather birds once they fly beyond the window's sides. The original
        // unbounded eight-second lifetime otherwise keeps a small pool occupied offscreen.
        if (weatherBirds && p.definition >= 0) {
            Vec position = nativeContinuousPosition(p, pool.definitions[p.definition]);
            if (position.x < bounds.l - 16 || position.x > bounds.r + 16) {
                p.definition = -1;
                pool.deaths++;
            }
        }
        if (nativeContinuousAlive(pool, p, bounds))
            continue;
        if (emitting && pool.budget > 0) {
            nativeContinuousSpawn(pool, p, bounds, scale, rotation);
            p.age = pool.budget;
            nativeContinuousAlive(pool, p, bounds);
            pool.budget -= interval;
        }
    }
    // Native 0x2aeb70..0x2aeb88 discards overdue births when the pool is
    // full. It never releases a burst of backlog after the next death.
    if (emitting && pool.budget > 0) {
        pool.budget -= (int)(pool.budget / interval) * interval;
        pool.budget -= interval;
    }
}

NativeContinuousPool &nativeContinuousPoolFor(Object &source, float time) {
    int index = -1, oldest = 0;
    for (int i = 0; i < 64; i++) {
        if (nativeContinuousPools[i].configured && nativeContinuousPools[i].id == source.id &&
            nativeContinuousPools[i].source == &source) {
            index = i;
            break;
        }
        if (nativeContinuousPools[i].used < nativeContinuousPools[oldest].used)
            oldest = i;
    }
    if (index < 0)
        index = oldest;
    NativeContinuousPool &pool = nativeContinuousPools[index];
    if (!pool.configured || pool.id != source.id || pool.source != &source || time < pool.lastTime)
        nativeContinuousConfigure(pool, source);
    pool.used = ++nativeContinuousUse;
    return pool;
}

void nativeContinuousResetScene(Level &scene) {
    for (int i = 0; i < 64; i++)
        if (nativeContinuousPools[i].configured)
            for (int j = 0; j < scene.objectCount; j++)
                if (nativeContinuousPools[i].source == &scene.objects[j]) {
                    nativeContinuousPools[i].lastTime = 1;
                    break;
                }
    for (int i = 0; i < scene.objectCount; i++)
        if (scene.objects[i].base == 5) {
            NativeContinuousPool &pool = nativeContinuousPoolFor(scene.objects[i], 0);
            pool.lastTime = 0;
            pool.updated = false;
        }
}

// ParticleEffect::Save/Load0x2ab614/0x2adbbc serialize visibility and
// transforms, not live slots, emission budget or RNG state. LevelTask's
// DoLoadState0x26e5a8..0x26e69c loads the existing objects in place.
// Rebase the port's scene-clock cursor without rewinding those unsaved pools.
void nativeContinuousRebaseScene(Level &scene, float time) {
    for (int i = 0; i < scene.objectCount; i++) {
        Object &source = scene.objects[i];
        if (source.base != 5)
            continue;
        NativeContinuousPool *retained = 0;
        for (int j = 0; j < 64; j++)
            if (nativeContinuousPools[j].configured && nativeContinuousPools[j].id == source.id &&
                nativeContinuousPools[j].source == &source) {
                retained = nativeContinuousPools + j;
                break;
            }
        NativeContinuousPool &pool = retained ? *retained : nativeContinuousPoolFor(source, time);
        pool.lastTime = time;
        pool.updated = true;
        pool.used = ++nativeContinuousUse;
    }
}

void nativeContinuousAdvanceScene(Level &scene, float scale, float time, float dt) {
    for (int i = 0; i < scene.objectCount; i++) {
        Object &source = scene.objects[i];
        if (source.base != 5)
            continue;
        if (source.props.flag("maker-weather"))
            source.rect = makerWeatherEmitterRect(source, viewW, viewH);
        NativeContinuousPool &pool = nativeContinuousPoolFor(source, time);
        if (pool.updated && pool.lastTime == time)
            continue;
        Rect bounds = nativeParticleEmitterBounds(source.rect, scale, source.angle);
        Vec origin = center(source.rect);
        bounds = shifted(bounds, -origin.x, -origin.y);
        if (dt > 0)
            nativeContinuousStep(pool, bounds, scale, source.angle, dt);
        pool.lastTime = time;
        pool.updated = true;
    }
}

NativeContinuousPool &nativeContinuousFor(Object &source, Rect bounds, float scale, float time) {
    NativeContinuousPool &pool = nativeContinuousPoolFor(source, time);
    // Normal runtime advances every emitter once per scene tick, including
    // offscreen ones. Drawing never ages an already updated scene. A direct
    // diagnostic draw without a scene tick uses one native Update(delta),
    // not a costly invented sixty-step replay of every elapsed second.
    if (!pool.updated || pool.lastTime != time) {
        float delta = maxf(0, time - pool.lastTime);
        if (delta > 0)
            nativeContinuousStep(pool, bounds, scale, source.angle, delta);
        pool.lastTime = time;
        pool.updated = true;
    }
    return pool;
}
