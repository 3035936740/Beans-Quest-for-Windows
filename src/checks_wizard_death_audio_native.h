#pragma once

// Regression coverage for wizard death audio native. Runs only in the isolated BQ_ENABLE_TESTS executable.
#include "checks_wizard_death_audio_vectors.h"

float wizardDeathAudioFloatNative(u32 bits) {
    float f;
    memcpy(&f, &bits, 4);
    return f;
}

AudioClip *wizardDeathAudioClipNative() {
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, "malmo-death"))
            return &soundClips[i].clip;
    return 0;
}

int wizardDeathAudioPrepareNative() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    stopMusic();
    stopSounds();
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    if (boss >= 0) {
        entities[boss].state.active = 4;
        entities[boss].state.hp = 0;
        entities[boss].state.timer = 0;
        entities[boss].state.gone = 0;
    }
    soundEnabled = true;
    return boss;
}

bool wizardDeathAudioPcmNative(AudioClip *clip, float gain, int frames = 512) {
    short output[1024];
    mixAudio(output, frames);
    bool same = clip && clip->frames >= (u32)frames, nonzero = false;
    for (int i = 0; i < frames * 2 && same; i++) {
        int expected = (int)((clip->pcm[i] * 3 / 4) * gain);
        same &= absf((float)output[i] - expected) <= 1;
        nonzero |= expected != 0;
    }
    return same && nonzero;
}

void wizardDeathAudioNativeChecks() {
    if (!audioInitialized || audioRunning) {
        check(false, "Wizard death envelope requires the isolated software mixer");
        return;
    }
    AudioSilenceKeepAlive keep = audioSilenceKeepAlive;
    audioSilenceKeepAlive = AudioSilenceKeepAlive{};
    int boss = wizardDeathAudioPrepareNative();
    AudioClip *clip = wizardDeathAudioClipNative();
    check(boss >= 0 && clip && clip->frames > 512,
          "Original Wizard and original cached death clip resolve for offline waveform controls");
    if (boss < 0 || !clip)
        return;
    Entity &b = entities[boss];
    b.state.active = 3;
    b.state.hp = 1;
    setWizardHitClockNative(b, 2);
    u32 before = voiceSerial;
    wizardHurt(b);
    check(
        b.state.active == 4 && !b.state.hp && voiceSerial == before + 1 &&
            wizardDeathAudioPcmNative(clip, 1),
        "Actual final head hit retains one full-volume initial death sound before the diminishing repeats");
    bool timing = true, wave = true;
    int pulses = 0;
    char trace[5000] = "fps,step,age_bits,expected_gain_bits,expected_voice,actual_voice\r\n";
    for (int rate = 0; rate < 3; rate++) {
        boss = wizardDeathAudioPrepareNative();
        Entity &death = entities[boss];
        int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240;
        for (int step = 1; step <= fps * 3; step++) {
            stopSounds();
            before = voiceSerial;
            updateWizard(death, 1.0f / fps);
            const WizardDeathAudioEventReference *expected = 0;
            for (unsigned int e = 0;
                 e < sizeof(wizardDeathAudioEvents) / sizeof(wizardDeathAudioEvents[0]); e++)
                if (wizardDeathAudioEvents[e].fps == fps && wizardDeathAudioEvents[e].step == step)
                    expected = &wizardDeathAudioEvents[e];
            bool emitted = voiceSerial != before;
            timing &= emitted == (expected != 0) && voiceSerial - before < 2;
            if (expected) {
                u32 age;
                memcpy(&age, &death.state.timer, 4);
                timing &= age == expected->age;
                wave &= emitted && wizardDeathAudioPcmNative(
                                       clip, wizardDeathAudioFloatNative(expected->gain));
                pulses++;
            }
            if (expected || emitted) {
                char row[180];
                u32 age;
                memcpy(&age, &death.state.timer, 4);
                wsprintfA(row, "%d,%d,%08x,%08x,%d,%d\r\n", fps, step, age,
                          expected ? expected->gain : 0, expected != 0, emitted);
                lstrcatA(trace, row);
            }
        }
    }
    check(
        timing && pulses == 15,
        "30/120/240Hz death timelines match independent binary32 reference pulses and omit the2.4/2.8second repeats");
    check(
        wave && pulses == 15,
        "Every actual death pulse mixes the original stereo samples at its independently computed diminishing gain");
    atomicWrite("test-output/wizard-death-audio-timeline.csv", trace, lstrlenA(trace));
    bool edges = true, large = true;
    for (unsigned int n = 0; n < sizeof(wizardDeathAudioCases) / sizeof(wizardDeathAudioCases[0]);
         n++) {
        const WizardDeathAudioCaseReference &ref = wizardDeathAudioCases[n];
        boss = wizardDeathAudioPrepareNative();
        Entity &death = entities[boss];
        death.state.timer = wizardDeathAudioFloatNative(ref.old);
        before = voiceSerial;
        updateWizard(death, wizardDeathAudioFloatNative(ref.dt));
        u32 now;
        memcpy(&now, &death.state.timer, 4);
        bool correct = now == ref.now && voiceSerial - before == (ref.gain ? 1 : 0);
        if (ref.gain)
            correct &= wizardDeathAudioPcmNative(clip, wizardDeathAudioFloatNative(ref.gain));
        if (n >= 7 && n <= 9)
            large &= correct;
        else
            edges &= correct;
    }
    check(
        edges,
        "Death audio preserves native binary32 cutoffs, exact multiples and negative-clock signed modulo behavior");
    check(
        large,
        "Large death-clock steps use the original modulo-wrap decision and emit at most one fading pulse");
    boss = wizardDeathAudioPrepareNative();
    Entity &restored = entities[boss];
    restored.state.timer = .39f;
    captureCheckpoint(false);
    float age = restored.state.timer;
    before = voiceSerial;
    Screen old = screen;
    screen = PAUSE;
    tick(.02f, false, false);
    check(restored.state.timer == age && voiceSerial == before,
          "Pausing gameplay preserves the saved death clock without queuing a repeat");
    screen = old;
    restored.state.timer = 2;
    restoreCheckpoint();
    before = voiceSerial;
    updateWizard(entities[boss], .02f);
    check(
        voiceSerial == before + 1 &&
            wizardDeathAudioPcmNative(clip,
                                      wizardDeathAudioFloatNative(wizardDeathAudioCases[0].gain)),
        "Restoring the death sequence resumes its saved fade phase without replaying the initial full-volume effect");
    boss = wizardDeathAudioPrepareNative();
    Entity &muted = entities[boss];
    muted.state.timer = .39f;
    soundEnabled = false;
    before = voiceSerial;
    updateWizard(muted, .02f);
    bool silent = voiceSerial == before;
    stopSounds();
    soundEnabled = true;
    updateWizard(muted, .01f);
    check(
        silent && voiceSerial == before && muted.state.timer > .41f,
        "Effects mute advances the death clock without voices and unmute does not replay missed pulses");
    boss = wizardDeathAudioPrepareNative();
    Entity &final = entities[boss];
    final.state.timer = 2.99f;
    before = voiceSerial;
    updateWizard(final, .02f);
    check(
        final.state.gone && final.state.active == 5 && !wizardInputLocked() &&
            voiceSerial == before,
        "The distinct three-second death completion still retires the Wizard and releases input without a late death pulse");
    stopSounds();
    sound("jump");
    AudioClip *jump = jumpAudioClip();
    check(
        jump && wizardDeathAudioPcmNative(jump, 1),
        "An ordinary jump following the last fading death pulse keeps the exact original default mixer gain");
    stopSounds();
    sound("malmo-death", 1, .25f);
    sound("jump");
    short mixed[1024];
    mixAudio(mixed, 512);
    bool independent = jump && jump->frames >= 512;
    for (int i = 0; i < 1024 && independent; i++) {
        int expected = (int)((clip->pcm[i] * 3 / 4) * .25f) + jump->pcm[i] * 3 / 4;
        expected = (int)clamp((float)expected, -32768, 32767);
        independent &= mixed[i] == expected;
    }
    check(
        independent,
        "Simultaneous faded death and default jump voices keep independent gains through actual stereo summation");
    stopSounds();
    sound("malmo-death", 1, 0);
    mixAudio(mixed, 512);
    bool zero = true;
    for (int i = 0; i < 1024; i++)
        zero &= mixed[i] == 0;
    stopSounds();
    sound("jump");
    check(
        zero && wizardDeathAudioPcmNative(jump, 1),
        "An explicitly zero-gain event is digitally silent and cannot attenuate the next ordinary jump");
    stopSounds();
    soundEnabled = musicEnabled = false;
    audioSilenceKeepAlive = keep;
    startStage(0);
}
