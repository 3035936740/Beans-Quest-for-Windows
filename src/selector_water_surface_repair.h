#pragma once

// Repairs the affected selector water surface while preserving campaign geometry.

// User-requested correction of an authored World4 (displayed chapter2)
// selector defect. Native selector particles use scale0.5 around their
// emitter centre. The wave's offset0.75..0.95 consequently puts its mean
// at128.2, above the actual WaterArea surface127. Translate only its draw
// origin; retain the original emitter distribution, RNG, and water physics.
Rect selectorWaterSurfaceVisualRect(const Level &level, const Object &object, Rect rect) {
    if (level.id != 799282380888ULL || object.id != 534211538701571ULL || object.base != 5 ||
        object.klass != 528389847068ULL || object.layer != 6 || object.angle != 0)
        return rect;
    if (object.rect.l != 368 || object.rect.b != 112 || object.rect.r != 640 ||
        object.rect.t != 136)
        return rect;
    const Props *definition = object.behavior("ParticleDefSettings");
    if (!definition || absf(definition->num("offset-min-y") - 0.75f) > 0.00001f ||
        absf(definition->num("offset-max-y") - 0.95f) > 0.00001f)
        return rect;
    for (int i = 0; i < level.objectCount; i++) {
        const Object &water = level.objects[i];
        if (water.id != 5677084289919524ULL || water.layer != object.layer ||
            !water.has("WaterAreaBehaviour") || water.rect.l != 432 || water.rect.b != -1 ||
            water.rect.r != 576 || water.rect.t != 127)
            continue;
        Rect emitter = nativeParticleEmitterBounds(object.rect, 0.5f);
        float mean = emitter.b +
                     (emitter.t - emitter.b) *
                         (definition->num("offset-min-y") + definition->num("offset-max-y")) * 0.5f;
        return shifted(rect, 0, water.rect.t - mean);
    }
    return rect;
}
