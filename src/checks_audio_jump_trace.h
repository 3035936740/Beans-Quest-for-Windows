#pragma once

// Regression coverage for audio jump trace. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct AudioJumpTraceTestSnapshot {
    AudioJumpTrace traces[AUDIO_JUMP_TRACE_CAPACITY];
    u32 sequence, voiceTrace[32];
    AudioVoice voices[32];
    SoundClip clips[2];
    int clipCount;
    u32 serial, started, mixed, nonzero;
    bool enabled, music;
    MMRESULT error;
    AudioJumpTraceContextFn context;
};

void audioJumpTraceTestContext(AudioJumpTrace &trace) {
    trace.stageId = 976871008727ULL;
    trace.x100 = 680012;
    trace.y100 = 12345;
    trace.jumps = 9;
    trace.scene = PLAY;
    trace.contextFlags = 17;
}

void audioJumpTraceChecks() {
    // The self-test uses initAudio(...,false). Never swap mixer state while
    // a hardware worker is running, and restore every diagnostic/audio field.
    if (!audioInitialized || audioRunning) {
        check(false, "Jump trace regression requires the isolated offline mixer");
        return;
    }
    AudioJumpTraceTestSnapshot *saved = new AudioJumpTraceTestSnapshot;
    memcpy(saved->traces, audioJumpTraces, sizeof(audioJumpTraces));
    saved->sequence = audioJumpTraceSequence;
    memcpy(saved->voiceTrace, audioJumpVoiceTrace, sizeof(audioJumpVoiceTrace));
    memcpy(saved->voices, audioVoices, sizeof(audioVoices));
    memcpy(saved->clips, soundClips, sizeof(saved->clips));
    saved->clipCount = soundClipCount;
    saved->serial = voiceSerial;
    saved->started = audioVoicesStarted;
    saved->mixed = audioSamplesMixed;
    saved->nonzero = audioNonzeroSamples;
    saved->enabled = soundEnabled;
    saved->music = musicEnabled;
    saved->error = audioError;
    saved->context = audioJumpTraceContext;
    memset(audioVoices, 0, sizeof(audioVoices));
    resetAudioJumpTrace();
    soundEnabled = true;
    musicEnabled = false;
    audioError = MMSYSERR_NOERROR;
    audioJumpTraceContext = audioJumpTraceTestContext;
    short pcm[] = {1000, -1000, 0, 0, 2, 0, 0, -2};
    AudioClip clip = {0, pcm, 4};
    soundClips[0].name = "jump";
    soundClips[0].clip = clip;
    soundClips[1].name = "trace-other";
    soundClips[1].clip = clip;
    soundClipCount = 2;
    short output[16] = {};
    sound("jump");
    mixAudio(output, 4);
    AudioJumpTrace &full = audioJumpTraces[0];
    check(
        full.sequence == 1 && full.rejectReason == AUDIO_JUMP_ACCEPTED &&
            full.finishReason == AUDIO_JUMP_COMPLETE && full.clipFrames == 4 &&
            full.mixedFrames == 4 && full.nonzeroFrames == 3 && full.slot >= 0 &&
            full.voiceSerial && full.stageId == 976871008727ULL && full.x100 == 680012 &&
            full.y100 == 12345 && full.jumps == 9 && full.contextFlags == 17,
        "Jump trace records exact per-voice mixed frames and nonzero contribution plus caller context without changing PCM");
    check(output[0] == 750 && output[1] == -750 && output[2] == 0 && output[4] == 1 &&
              output[7] == -1,
          "Jump trace instrumentation preserves existing three-quarter-volume mixer samples");
    sound("jump");
    mixAudio(output, 1);
    stopSounds();
    AudioJumpTrace &stopped = audioJumpTraces[1];
    check(stopped.finishReason == AUDIO_JUMP_STOPPED && stopped.mixedFrames == 1 &&
              stopped.nonzeroFrames == 1,
          "Jump trace distinguishes stopSounds truncation from natural voice completion");
    sound("jump");
    int jumpSlot = audioJumpTraces[2].slot;
    for (int i = 0; i < 32; i++)
        if (i != jumpSlot) {
            audioVoices[i].clip = &soundClips[1].clip;
            audioVoices[i].gain = 1;
            audioVoices[i].position = 0;
            audioVoices[i].serial = ++voiceSerial;
        }
    sound("trace-other");
    check(
        audioJumpTraces[2].finishReason == AUDIO_JUMP_PREEMPTED &&
            audioJumpTraces[2].mixedFrames == 0 && audioJumpVoiceTrace[jumpSlot] == 0,
        "A non-jump voice stealing the oldest slot records jump preemption before any mixed frames");
    stopSounds();
    soundEnabled = false;
    sound("jump");
    soundEnabled = true;
    audioInitialized = false;
    sound("jump");
    audioInitialized = true;
    soundClipCount = 0;
    sound("jump");
    soundClipCount = 2;
    check(
        audioJumpTraces[3].rejectReason == AUDIO_JUMP_DISABLED &&
            audioJumpTraces[4].rejectReason == AUDIO_JUMP_UNINITIALIZED &&
            audioJumpTraces[5].rejectReason == AUDIO_JUMP_CLIP_MISSING &&
            audioJumpTraces[3].finishReason == AUDIO_JUMP_REJECTED &&
            audioJumpTraces[4].mixedFrames == 0 && audioJumpTraces[5].slot == -1,
        "Rejected jumps separately record sound toggle, uninitialized audio and absent cached clip");
    resetAudioJumpTrace();
    sound("jump");
    u32 stale = audioJumpVoiceTrace[audioJumpTraces[0].slot];
    for (int i = 0; i < AUDIO_JUMP_TRACE_CAPACITY; i++)
        audioJumpTraceBegin(AUDIO_JUMP_DISABLED);
    mixAudio(output, 1);
    bool wrapped = audioJumpTraceSequence == AUDIO_JUMP_TRACE_CAPACITY + 1 &&
                   audioJumpTraces[0].sequence == AUDIO_JUMP_TRACE_CAPACITY + 1 &&
                   audioJumpTraces[0].mixedFrames == 0 && stale == 1;
    check(
        wrapped && flushAudioJumpTrace("test-output/audio-jump-trace-isolated.csv"),
        "The 2048-entry jump ring wraps without attributing a stale live voice to a newer event and flushes only from the offline main thread");
    memcpy(audioJumpTraces, saved->traces, sizeof(audioJumpTraces));
    audioJumpTraceSequence = saved->sequence;
    memcpy(audioJumpVoiceTrace, saved->voiceTrace, sizeof(audioJumpVoiceTrace));
    memcpy(audioVoices, saved->voices, sizeof(audioVoices));
    memcpy(soundClips, saved->clips, sizeof(saved->clips));
    soundClipCount = saved->clipCount;
    voiceSerial = saved->serial;
    audioVoicesStarted = saved->started;
    audioSamplesMixed = saved->mixed;
    audioNonzeroSamples = saved->nonzero;
    soundEnabled = saved->enabled;
    musicEnabled = saved->music;
    audioError = saved->error;
    audioJumpTraceContext = saved->context;
    delete saved;
    Screen oldScreen = screen;
    bool oldBean = page.hasBean;
    u64 oldId = page.level.id;
    State oldState = page.bean.state;
    screen = SELECT;
    page.hasBean = true;
    page.level.id = 12038123762473955738ULL;
    page.bean.state.x = 336;
    page.bean.state.y = 340;
    AudioJumpTrace selector = {};
    runtimeJumpAudioContext(selector);
    check(
        selector.scene == SELECT && selector.stageId == page.level.id && selector.x100 == 33600 &&
            selector.y100 == 34000 && (selector.contextFlags & 16),
        "Live selector sound diagnostics identify the actual playable selector and miniature Bean instead of the suspended game stage");
    screen = oldScreen;
    page.hasBean = oldBean;
    page.level.id = oldId;
    page.bean.state = oldState;
}
