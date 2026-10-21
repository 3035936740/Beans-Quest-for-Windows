#pragma once

// Regression coverage for jelly sound native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int jellySoundVoiceNative() {
    int slot = -1;
    u32 newest = 0;
    for (int i = 0; i < 32; i++)
        if (audioVoices[i].clip && audioVoices[i].serial > newest) {
            newest = audioVoices[i].serial;
            slot = i;
        }
    return slot;
}

bool jellySoundDrainNative(int slot, const char *wav = 0) {
    if (slot < 0 || !audioVoices[slot].clip)
        return false;
    AudioVoice before = audioVoices[slot];
    u32 sequence = audioJumpVoiceTrace[slot];
    u32 cap = before.clip->frames * 2 + 4;
    u8 *bytes = (u8 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 44 + cap * 4);
    if (!bytes)
        return false;
    short *pcm = (short *)(bytes + 44);
    u32 frames = 0;
    while (audioVoices[slot].clip && frames < cap) {
        mixAudio(pcm + frames * 2, 1);
        frames++;
    }
    AudioJumpTrace &t = audioJumpTraces[(sequence - 1) & (AUDIO_JUMP_TRACE_CAPACITY - 1)];
    bool ok = sequence && t.sequence == sequence && t.finishReason == AUDIO_JUMP_COMPLETE &&
              t.nonzeroFrames > 100 && t.mixedFrames == frames;
    if (wav) {
        memcpy(bytes, "RIFF", 4);
        *(u32 *)(bytes + 4) = 36 + frames * 4;
        memcpy(bytes + 8, "WAVEfmt ", 8);
        *(u32 *)(bytes + 16) = 16;
        *(unsigned short *)(bytes + 20) = 1;
        *(unsigned short *)(bytes + 22) = 2;
        *(u32 *)(bytes + 24) = 22050;
        *(u32 *)(bytes + 28) = 88200;
        *(unsigned short *)(bytes + 32) = 4;
        *(unsigned short *)(bytes + 34) = 16;
        memcpy(bytes + 36, "data", 4);
        *(u32 *)(bytes + 40) = frames * 4;
        ok = atomicWrite(wav, bytes, 44 + frames * 4) && ok;
    }
    HeapFree(GetProcessHeap(), 0, bytes);
    return ok;
}

void jellySoundNativeChecks() {
    if (audioRunning) {
        check(false, "Jelly PCM checks require the offline mixer");
        return;
    }
    bool oldSound = soundEnabled, oldMusic = musicEnabled, oldDebug = debugMode,
         oldInterface = interfaceEnabled;
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/jelly-sound-saves");
    CreateDirectoryA(saveRoot, 0);
    soundEnabled = true;
    musicEnabled = debugMode = interfaceEnabled = false;
    stopMusic();
    stopSounds();
    check(
        nativeTrampolineSoundRate(74, 74) == 1 && nativeTrampolineSoundRate(0, 74) == 1 &&
            nativeTrampolineSoundRate(370, 74) == 2 && nativeTrampolineSoundRate(740, 74) == 2,
        "Jelly native pitch stays audible at base height and clamps inherited height to rate1..2");
    check(
        absf(nativeTrampolineSoundRate(222, 74) - 1.5f) < .00001f &&
            absf(nativeTrampolineSoundRate(171, 37, 20) - 1.5f) < .00001f,
        "Jelly sound follows the native height and bonus-height modulation rather than a second megajump clip");
    check(
        absf(nativeTrampolineSoundRateFromSpeed(root(800 * 222), -400, 74) - 1.5f) < .00001f &&
            nativeTrampolineSoundRateFromSpeed(400, 0, 74) == 1,
        "Saved jelly speed reconstructs its sound pitch without changing checkpoint bytes or inventing silence");
    AudioJumpTraceTestSnapshot *saved = new AudioJumpTraceTestSnapshot;
    memcpy(saved->clips, soundClips, sizeof(saved->clips));
    saved->clipCount = soundClipCount;
    short ramp[] = {1000, -1000, 2000, -2000, 3000, -3000, 4000, -4000,
                    5000, -5000, 6000, -6000, 7000, -7000, 8000, -8000};
    AudioClip synthetic = {0, ramp, 8};
    soundClips[0].name = "jump";
    soundClips[0].clip = synthetic;
    soundClipCount = 1;
    short output[32] = {};
    sound("jump");
    mixAudio(output, 8);
    check(output[0] == 750 && output[2] == 1500 && output[14] == 6000 && output[15] == -6000 &&
              jellySoundVoiceNative() < 0,
          "Rate1 preserves every ordinary jump sample, stereo channel and original voice duration");
    sound("jump", 1.5f);
    mixAudio(output, 6);
    check(
        output[0] == 750 && output[2] == 1875 && output[4] == 3000 && output[6] == 4125 &&
            output[8] == 5250 && output[10] == 6000 && output[3] == -1875 &&
            jellySoundVoiceNative() < 0,
        "Fractional jelly pitch linearly resamples both channels and completes without dropping the voice");
    sound("jump", 2);
    mixAudio(output, 4);
    check(
        output[0] == 750 && output[2] == 2250 && output[4] == 3750 && output[6] == 5250 &&
            jellySoundVoiceNative() < 0,
        "Maximum native jelly pitch advances through the clip at twice the playback rate with nonzero PCM");
    sound("jump", 2);
    int first = jellySoundVoiceNative();
    sound("jump");
    int second = jellySoundVoiceNative();
    check(
        first != second && audioVoices[first].rate == 131072 && audioVoices[second].rate == 65536 &&
            audioVoices[second].fraction == 0,
        "A pitched jelly voice cannot change the next ordinary jump or another simultaneous voice");
    stopSounds();
    memcpy(soundClips, saved->clips, sizeof(saved->clips));
    soundClipCount = saved->clipCount;
    delete saved;
    const float dt = 1.0f / 120;
    for (int high = 0; high < 2; high++) {
        int pad = prepareCampaignTrampolineNative(high != 0);
        for (int n = 0; n < 500 && !trampolineStateNative.pending; n++)
            tick(dt, false, false);
        TrampolineStateNative request = trampolineStateNative;
        float expected = nativeTrampolineSoundRateFromSpeed(request.speed, -400, 74);
        u32 before = voiceSerial;
        tick(dt, false, false);
        int slot = jellySoundVoiceNative();
        bool launched = pad >= 0 && request.pending && trampolineLaunchedThisStepNative &&
                        voiceSerial == before + 1 && slot >= 0 &&
                        audioVoices[slot].rate == (u32)(int)(expected * 65536 + .5f) &&
                        pinballDetailVoiceCount("jump") == 1 &&
                        pinballDetailVoiceCount("megajump") == 0;
        check(
            launched && (high ? expected > 1 : expected == 1),
            high ? "An actual high campaign jelly contact emits one height-pitched jump voice"
                 : "An actual base-height campaign jelly contact emits one normal-rate jump voice");
        check(
            launched && jellySoundDrainNative(slot, high ? "test-output/jelly-campaign-high.wav"
                                                         : "test-output/jelly-campaign-base.wav"),
            high
                ? "The inherited-height campaign jelly sound mixes nonzero PCM through natural completion"
                : "The base-height campaign jelly sound mixes nonzero PCM through natural completion");
    }
    for (int n = 0; n < 2; n++) {
        interfaceEnabled = true;
        int pad = prepareUiJumpDetailNative(12038123762473955738ULL,
                                            n ? 12499371799514561398ULL : 6285546912625162820ULL);
        u32 before = voiceSerial;
        tickInterface(dt);
        tickInterface(dt);
        int slot = jellySoundVoiceNative();
        bool ok = pad >= 0 && page.specialJump.launchedKind == 2 &&
                  page.specialJump.launchedSource == pad && voiceSerial == before + 1 &&
                  slot >= 0 && audioVoices[slot].rate > 65536 &&
                  pinballDetailVoiceCount("jump") == 1 && pinballDetailVoiceCount("megajump") == 0;
        check(
            ok && jellySoundDrainNative(slot, n ? "test-output/jelly-selector-right.wav"
                                                : "test-output/jelly-selector-left.wav"),
            n ? "The right original playable-selector jelly emits its height-pitched voice and complete nonzero PCM"
              : "The left original playable-selector jelly emits its height-pitched voice and complete nonzero PCM");
    }
    stopSounds();
    lstrcpyA(saveRoot, oldRoot);
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    debugMode = oldDebug;
    interfaceEnabled = oldInterface;
}
