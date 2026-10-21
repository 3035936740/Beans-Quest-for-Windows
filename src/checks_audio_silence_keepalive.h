#pragma once

// Regression coverage for audio silence keepalive. Runs only in the isolated BQ_ENABLE_TESTS executable.

void audioSilenceKeepAliveChecks() {
    check(
        audioDeviceNeedsSilenceKeepAlive(L"Speakers (2- AB13X USB Audio)") &&
            audioDeviceNeedsSilenceKeepAlive(L"ab13x usb audio") &&
            !audioDeviceNeedsSilenceKeepAlive(L"Speakers (AB17X USB Audio)") &&
            !audioDeviceNeedsSilenceKeepAlive(L"Speakers (Realtek Audio)") &&
            !audioDeviceNeedsSilenceKeepAlive(L"XAB13X USB Audio") &&
            !audioDeviceNeedsSilenceKeepAlive(L"AB13X USB AudioExtra"),
        "Silence compatibility matches only the named AB13X USB Audio device family, preserving other outputs");
    short *pcm = (short *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 4410 * sizeof(short));
    AudioSilenceKeepAlive state = {};
    state.matched = true;
    bool filled = fillAudioOutputSilence(pcm, 2205, true, state), small = true, stereo = true;
    int sum = 0, peak = 0;
    for (int i = 0; i < 2205; i++) {
        small &= absf((float)pcm[i * 2]) <= 832;
        stereo &= pcm[i * 2] == pcm[i * 2 + 1];
        sum += pcm[i * 2];
        peak = (int)maxf((float)peak, absf((float)pcm[i * 2]));
    }
    check(
        filled && small && stereo && sum == 0 && peak >= 831 && state.frames == 2205 &&
            state.phase == 0,
        "A silent buffer receives the verified low-frequency shape: a zero-mean10Hz stereo sine with832-unit peak at22050Hz");
    short *split = (short *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 4410 * sizeof(short));
    AudioSilenceKeepAlive fragmented = {};
    fragmented.matched = true;
    fillAudioOutputSilence(split, 511, true, fragmented);
    fillAudioOutputSilence(split + 1022, 1694, true, fragmented);
    check(
        memcmp(pcm, split, 4410 * sizeof(short)) == 0 && fragmented.phase == state.phase,
        "The low-frequency waveform keeps continuous phase across irregular output blocks without restarting or drifting at buffer boundaries");
    HeapFree(GetProcessHeap(), 0, split);
    memset(pcm, 0, 2048 * sizeof(short));
    state.phase = 300;
    u32 frames = state.frames;
    bool muted = fillAudioOutputSilence(pcm, 1024, false, state);
    bool zero = true;
    for (int i = 0; i < 2048; i++)
        zero &= pcm[i] == 0;
    check(
        !muted && zero && state.frames == frames && state.phase == 0,
        "Turning sound effects off preserves exact digital silence and disables the endpoint keepalive");
    state.matched = false;
    bool inactive = fillAudioOutputSilence(pcm, 1024, true, state);
    zero = true;
    for (int i = 0; i < 2048; i++)
        zero &= pcm[i] == 0;
    check(
        !inactive && zero,
        "An unmatched audio device receives the original all-zero silent buffer without compatibility changes");
    state.matched = true;
    pcm[100] = 1;
    pcm[601] = -32768;
    u32 original = hashBytes(pcm, 2048 * sizeof(short));
    bool mixed = fillAudioOutputSilence(pcm, 1024, true, state);
    check(
        !mixed && hashBytes(pcm, 2048 * sizeof(short)) == original,
        "Any genuine mixed sound, including one-unit samples and clipping limits, keeps every stereo PCM sample byte-identical");
    HeapFree(GetProcessHeap(), 0, pcm);

    if (!audioInitialized || audioRunning) {
        check(false, "Authentic clip compatibility regression requires the offline mixer");
        return;
    }
    AudioSilenceKeepAlive savedKeep = audioSilenceKeepAlive;
    AudioVoice savedVoices[32];
    u32 savedTrace[32];
    memcpy(savedVoices, audioVoices, sizeof(savedVoices));
    memcpy(savedTrace, audioJumpVoiceTrace, sizeof(savedTrace));
    bool oldSound = soundEnabled, oldMusic = musicEnabled;
    u32 oldMixed = audioSamplesMixed, oldNonzero = audioNonzeroSamples;
    AudioClip *jump = jumpAudioClip();
    const int blocks = 14, samples = blocks * AUDIO_FRAMES * 2;
    short *reference =
        (short *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, samples * sizeof(short));
    short *candidate =
        (short *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, samples * sizeof(short));
    bool authentic = jump && jump->frames == 5112;
    int changedBlocks = 0;
    if (authentic) {
        soundEnabled = true;
        musicEnabled = false;
        memset(audioJumpVoiceTrace, 0, sizeof(audioJumpVoiceTrace));
        for (int pass = 0; pass < 2; pass++) {
            memset(audioVoices, 0, sizeof(audioVoices));
            audioVoices[0] = AudioVoice{jump, 0, 1, 0, 65536, 1};
            audioSilenceKeepAlive = AudioSilenceKeepAlive{};
            audioSilenceKeepAlive.matched = pass != 0;
            for (int block = 0; block < blocks; block++)
                mixAudio((pass ? candidate : reference) + block * AUDIO_FRAMES * 2, AUDIO_FRAMES);
        }
        for (int block = 0; block < blocks; block++) {
            short *a = reference + block * AUDIO_FRAMES * 2,
                  *b = candidate + block * AUDIO_FRAMES * 2;
            bool rawSilent = true, same = true;
            for (int i = 0; i < AUDIO_FRAMES * 2; i++) {
                rawSilent &= a[i] == 0;
                same &= a[i] == b[i];
            }
            if (!rawSilent)
                authentic &= same;
            else {
                changedBlocks += !same;
                for (int i = 0; i < AUDIO_FRAMES; i++)
                    authentic &= absf((float)b[i * 2]) <= 832 && b[i * 2] == b[i * 2 + 1];
            }
        }
    }
    check(
        authentic && changedBlocks == 4,
        "The original5112frame jump passes unchanged through the real mixer while only the four subsequent fully silent buffers gain keepalive");
    memcpy(audioVoices, savedVoices, sizeof(savedVoices));
    memcpy(audioJumpVoiceTrace, savedTrace, sizeof(savedTrace));
    audioSilenceKeepAlive = savedKeep;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    audioSamplesMixed = oldMixed;
    audioNonzeroSamples = oldNonzero;
    HeapFree(GetProcessHeap(), 0, candidate);
    HeapFree(GetProcessHeap(), 0, reference);
}
