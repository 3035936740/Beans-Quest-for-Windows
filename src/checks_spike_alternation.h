#pragma once

// Regression coverage for spike alternation. Runs only in the isolated BQ_ENABLE_TESTS executable.

void spikeAlternationChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = deathTimer = 0;
    int groups = 0, cells = 0;
    u64 seen[6] = {};
    bool initialOff = true, lengths = true, unrelated = true, opening = true, endIdle = true,
         closing = true, endOff = true, phaseHeld = true;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind != SPIKE || e.mask != 16)
            continue;
        cells++;
        bool found = false;
        for (int j = 0; j < groups; j++)
            if (seen[j] == e.source->id)
                found = true;
        if (!found && groups < 6)
            seen[groups++] = e.source->id;
        resetSpikeAnimation(e, 0);
        initialOff &=
            e.state.hp == 0 && e.state.active == SPIKE_OFF && eq(spikeAnimationName(e), "off");
        lengths &= absf(animationLength(animation(e.art, "open")) - 0.5f) < 0.0001f &&
                   absf(animationLength(animation(e.art, "close")) - 0.5f) < 0.0001f &&
                   absf(animationLength(animation(e.art, "idle")) - 0.4f) < 0.0001f;
        State before = e.state;
        alternateSpikeAnimation(e, 8);
        unrelated &= e.state.active == before.active && e.state.timer == before.timer &&
                     e.state.hp == before.hp;
        alternateSpikeAnimation(e, 16);
        advanceSpikeAnimation(e, 0.499f);
        opening &=
            e.state.hp == 1 && e.state.active == SPIKE_OPENING && eq(spikeAnimationName(e), "open");
        float phase = e.state.aux;
        advanceSpikeAnimation(e, 0.0011f);
        endIdle &= e.state.active == SPIKE_IDLE && eq(spikeAnimationName(e), "idle") &&
                   absf(e.state.timer - phase) < 0.0001f;
        advanceSpikeAnimation(e, 0.073f);
        float held = e.state.timer;
        alternateSpikeAnimation(e, 16);
        advanceSpikeAnimation(e, 0.499f);
        closing &= e.state.hp == 0 && e.state.active == SPIKE_CLOSING &&
                   eq(spikeAnimationName(e), "close");
        advanceSpikeAnimation(e, 0.0011f);
        endOff &= e.state.active == SPIKE_OFF && eq(spikeAnimationName(e), "off");
        advanceSpikeAnimation(e, 2.0f);
        alternateSpikeAnimation(e, 16);
        advanceSpikeAnimation(e, 0.5f);
        phaseHeld &= e.state.active == SPIKE_IDLE && absf(e.state.timer - held) < 0.0001f;
    }
    check(
        groups == 6 && cells > 6,
        "All six original Wizard spike groups and their actual split actlets are present for the native alternation animation regression");
    check(
        initialOff && lengths,
        "Wizard mask16 spikes initially use original off art and retain native half-second opening/closing plus0.4-second idle resources");
    check(
        unrelated && opening && endIdle,
        "Only a matching alternation event restarts every real spike actlet's open STOP animation and selects idle at its end");
    check(
        closing && endOff && phaseHeld,
        "Original spike close STOP reaches off and later reopening resumes the idle instance's held per-cell phase");
    // Render the original atlas, without relying on a changed drawEntity hook.
    Entity sample;
    Object &source = *classById(17084625859160385508ULL);
    initializeEntity(sample, source, Rect{240, 180, 256, 196});
    sample.mask = 16;
    sample.alternation = 0;
    resetSpikeAnimation(sample, 0);
    cloudRenderBegin();
    drawActor(sample.art, entityRect(sample), spikeAnimationName(sample), sample.state.timer);
    u32 off = cloudRenderPixels(Rect{240, 180, 256, 196}, false);
    alternateSpikeAnimation(sample, 16);
    advanceSpikeAnimation(sample, 0.21f);
    cloudRenderBegin();
    drawActor(sample.art, entityRect(sample), spikeAnimationName(sample), sample.state.timer, 0,
              false, -1, 1, ANIMATION_STOP);
    u32 open = cloudRenderPixels(Rect{240, 180, 256, 196}, false);
    advanceSpikeAnimation(sample, 0.3f);
    cloudRenderBegin();
    drawActor(sample.art, entityRect(sample), spikeAnimationName(sample), sample.state.timer);
    u32 idle = cloudRenderPixels(Rect{240, 180, 256, 196}, false);
    alternateSpikeAnimation(sample, 16);
    advanceSpikeAnimation(sample, 0.21f);
    cloudRenderBegin();
    drawActor(sample.art, entityRect(sample), spikeAnimationName(sample), sample.state.timer, 0,
              false, -1, 1, ANIMATION_STOP);
    u32 close = cloudRenderPixels(Rect{240, 180, 256, 196}, false);
    check(
        off != idle && open != off && open != idle && close != off && close != idle,
        "Actual OpenGL spike atlas distinguishes original off, partially opening, extended idle and partially closing states");
    sample.state.hp = 3;
    sample.state.active = 1;
    sample.state.timer = 12;
    resetSpikeAnimation(sample, 0);
    bool oldOff = sample.state.hp == 0 && sample.state.active == SPIKE_OFF;
    resetSpikeAnimation(sample, 16);
    check(
        oldOff && sample.state.hp == 1 && sample.state.active == SPIKE_IDLE,
        "Legacy checkpoint spike state can be reset from saved global alternation bits without misreading the former generic hp3 as an opening animation");
    Entity neighbour = sample;
    neighbour.originX += 16;
    resetSpikeAnimation(sample, 16);
    resetSpikeAnimation(neighbour, 16);
    check(
        absf(spikeWrappedAnimationTime(neighbour.state.timer - sample.state.timer, 0.4f) - 0.32f) <
            0.0001f,
        "Original neighbouring16px spike actlets retain their negative0.08-second spatial idle phase difference");
    startStage(0);
}
