#pragma once

// Regression coverage for credits shot animation native. Runs only in the isolated BQ_ENABLE_TESTS executable.
u32 creditsShotAnimationPixelsNative(CreditsShotNative &shot) {
    Vec point = {shot.entity.state.x, shot.entity.state.y};
    float angle = shot.entity.state.angle;
    shot.entity.state.x = 256;
    shot.entity.state.y = 128;
    shot.entity.state.angle = 0;
    cloudRenderBegin();
    drawCreditsShotNative(shot);
    u32 pixels = cloudRenderPixels(Rect{232, 104, 280, 152}, false);
    shot.entity.state.x = point.x;
    shot.entity.state.y = point.y;
    shot.entity.state.angle = angle;
    return pixels;
}

void creditsShotAnimationNativeChecks() {
    bool oldInterface = interfaceEnabled, oldSound = soundEnabled, oldMusic = musicEnabled,
         oldDebug = debugMode;
    interfaceEnabled = true;
    soundEnabled = musicEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    int owner = prepareCreditsProjectileNative();
    CreditsShotNative &shot = creditsRuntime.shots[0];
    int pool = shot.animationPool;
    Object *fire = shot.entity.source;
    Animation *clip = animation(shot.entity.art, fire->props.get("default-animation"));
    int fireCount = 0, plasmaCount = 0;
    bool zero = true;
    for (int i = 0; i < creditsRuntime.shotAnimationCount; i++) {
        CreditsShotAnimationNative &clock = creditsRuntime.shotAnimations[i];
        fireCount += clock.source->id == 6430600862785566ULL;
        plasmaCount += clock.source->id == 12978618438667112518ULL;
        zero &= clock.age == 0;
    }
    check(
        owner >= 0 && creditsRuntime.shotAnimationCount == 30 && fireCount == 10 &&
            plasmaCount == 20 && zero && clip->count > 1,
        "The original ending creates ten fireball and twenty plasma animation instances before their first shots");
    u32 first = creditsShotAnimationPixelsNative(shot),
        campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        campaignShots = hashBytes(bullets, sizeof(bullets));
    float dt = 1.0f / 120;
    for (int i = 0; i < 19; i++)
        tickCreditsRuntime(page, dt);
    float before = creditsShotAnimationAgeNative(shot);
    bool hidden = true;
    for (int i = 0; i < creditsRuntime.shotAnimationCount; i++) {
        CreditsShotAnimationNative &clock = creditsRuntime.shotAnimations[i];
        float duration = animationLength(
                  animation(clock.art, clock.source->props.get("default-animation"))),
              reference = 0;
        if (duration > 0)
            for (int frame = 0; frame < 19; frame++) {
                reference += dt;
                if (reference >= duration)
                    reference -= duration;
            }
        hidden &= absf(clock.age - reference) < .000001f;
    }
    check(
        before > 0 && hidden,
        "Actual ending ticks advance every pooled animation including bullets that have never been visible");
    bool changed = creditsShotAnimationPixelsNative(shot) != first;
    for (int i = 0; i < 30 && !changed; i++) {
        tickCreditsRuntime(page, dt);
        changed = creditsShotAnimationPixelsNative(shot) != first;
    }
    check(changed,
          "Real OpenGL fireball frames advance through the retained original idle animation");
    shot.hit = true;
    shot.hitPoint = Vec{shot.entity.state.x, shot.entity.state.y};
    creditsProjectilePostStepNative(page, dt);
    before = creditsShotAnimationAgeNative(shot);
    for (int i = 0; i < 3; i++)
        tickCreditsRuntime(page, dt);
    float after = creditsRuntime.shotAnimations[pool].age, length = animationLength(clip),
          expected = before + 3 * dt;
    if (expected >= length)
        expected -= length;
    check(!shot.live && absf(after - expected) < .000001f,
          "A collided and hidden ending bullet keeps advancing its original selected animation");
    creditsFire(page, owner);
    check(
        shot.live && shot.animationPool == pool && shot.entity.state.timer == 0 &&
            creditsShotAnimationAgeNative(shot) == after,
        "Reusing the first free original pool member retains its animation phase while resetting flight age");
    u32 retained = creditsShotAnimationPixelsNative(shot);
    creditsRuntime.shotAnimations[pool].age = 0;
    u32 restarted = creditsShotAnimationPixelsNative(shot);
    creditsRuntime.shotAnimations[pool].age = after;
    check(
        retained != restarted,
        "Actual production OpenGL projectile rendering uses retained pool phase rather than the newly reset flight timer");
    creditsFire(page, owner);
    CreditsShotNative &second = creditsRuntime.shots[1];
    check(
        second.live && second.owner == owner && second.animationPool != pool &&
            creditsShotAnimationAgeNative(second) == after,
        "Simultaneous shots own distinct animation instances instead of sharing a reusable physical slot clock");
    u32 frozen = hashBytes(creditsRuntime.shotAnimations, sizeof(creditsRuntime.shotAnimations));
    tickCreditsRuntime(page, 0);
    check(frozen == hashBytes(creditsRuntime.shotAnimations, sizeof(creditsRuntime.shotAnimations)),
          "A suspended ending tick does not advance pooled animation clocks");
    creditsRuntime.shotAnimations[pool].age = length - dt;
    advanceCreditsShotAnimationsNative(dt);
    check(absf(creditsRuntime.shotAnimations[pool].age) < .000001f,
          "Pooled ending animation wraps at the native exact duration boundary");
    int plasmaOwner = -1;
    for (int i = 0; i < creditsRuntime.shotAnimationCount; i++)
        if (creditsRuntime.shotAnimations[i].source->id == 12978618438667112518ULL)
            plasmaOwner = creditsRuntime.shotAnimations[i].owner;
    shot.live = false;
    shot.body->SetActive(false);
    creditsFire(page, plasmaOwner);
    check(
        shot.live && shot.owner == plasmaOwner &&
            shot.entity.source->id == 12978618438667112518ULL && shot.animationPool != pool &&
            creditsRuntime.shotAnimations[pool].source == fire,
        "A physical slot reused by plasma retains the plasma emitter's own animation identity instead of inheriting fireball phase");
    check(campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
              campaignShots == hashBytes(bullets, sizeof(bullets)),
          "Ending animation lifecycle checks leave campaign actors and projectile state untouched");
    navigate(137502285691ULL);
    zero = true;
    for (int i = 0; i < creditsRuntime.shotAnimationCount; i++)
        zero &= creditsRuntime.shotAnimations[i].age == 0;
    check(zero && creditsRuntime.shotAnimationCount == 30,
          "Re-entering the ending constructs fresh original pooled animation instances");
    clearEffects(1);
    startStage(0);
    interfaceEnabled = oldInterface;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
}
