#pragma once

// Regression coverage for ending object mute native. Runs only in the isolated BQ_ENABLE_TESTS executable.
class EndingObjectMuteObserverNative : public CreditsContactsNative {
  public:
    bool observed, preference;
    int sampled;
    short output[16];

    explicit EndingObjectMuteObserverNative(UiScene &s)
        : CreditsContactsNative(s), observed(false), preference(false), sampled(0) {
        memset(output, 0, sizeof(output));
    }

    void PreSolve(b2Contact *contact, const b2Manifold *old) {
        CreditsContactsNative::PreSolve(contact, old);
        if (!observed) {
            observed = true;
            preference = soundEnabled;
            mixAudio(output, 8);
            sampled = 8;
        }
    }
};

void endingObjectMuteNativeChecks() {
    bool oldSound = soundEnabled, oldMusic = musicEnabled;
    interfaceEnabled = true;
    musicEnabled = false;
    stopMusic();
    stopSounds();
    startStage(0);
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    static short pcm[512];
    for (int i = 0; i < 256; i++) {
        pcm[i * 2] = 12000;
        pcm[i * 2 + 1] = -8000;
    }
    AudioClip clip = {0, pcm, 256};
    for (int enabled = 0; enabled < 2; enabled++) {
        navigate(137502285691ULL);
        int bird = -1;
        page.level.geometryCount = 0;
        for (int i = 0; i < page.playfieldCount; i++) {
            if (page.playfield[i].kind == HELPER)
                bird = i;
            else
                page.playfield[i].state.gone = 1;
        }
        if (bird < 0) {
            check(false, "Original ending helper exists for interleaved object-mute/mixer control");
            continue;
        }
        Entity &helper = page.playfield[bird];
        page.bean.state.x = helper.state.x;
        page.bean.state.y = helper.state.y;
        page.bean.state.vx = page.bean.state.vy = 0;
        resetUiPhysics(page);
        delete page.contacts;
        EndingObjectMuteObserverNative *observer = new EndingObjectMuteObserverNative(page);
        page.contacts = observer;
        page.physics->SetContactListener(observer);
        clearEffects(1);
        stopSounds();
        audioVoices[0] = AudioVoice{&clip, 0, ++voiceSerial, 0, 65536, 1};
        soundEnabled = enabled != 0;
        u32 voices = audioVoicesStarted;
        tickCreditsRuntime(page, 1.0f / 120);
        char label[230];
        wsprintfA(
            label,
            "Actual ending pickup suppresses new object sounds while still producing its native particles with user sounds%d",
            enabled);
        check(observer->observed && helper.state.active == 1 &&
                  helperPickupParticleCountNative(
                      1, idnum(helper.source->behavior("HelperBirdBehaviour")->get("effect"))) >
                      0 &&
                  audioVoicesStarted == voices,
              label);
        wsprintfA(
            label,
            "Audio mix interleaved inside real ending PreSolve retains the user's sound preference%d",
            enabled);
        check(observer->observed && observer->preference == (enabled != 0) &&
                  soundEnabled == (enabled != 0),
              label);
        bool samples = true;
        for (int i = 0; i < 8; i++)
            samples &= observer->output[i * 2] == (enabled ? 9000 : 0) &&
                       observer->output[i * 2 + 1] == (enabled ? -6000 : 0);
        wsprintfA(
            label,
            "Audio mix inside ending object mute%s an existing voice without allowing new requests",
            enabled ? " advances" : " preserves disabled");
        check(observer->sampled == 8 && audioVoices[0].clip == &clip &&
                  audioVoices[0].position == (enabled ? 8u : 0u) && samples,
              label);
        stopSounds();
        navigate(610822198339107ULL);
        voices = audioVoicesStarted;
        sound("bird");
        wsprintfA(
            label,
            "Leaving ending object mute restores ordinary sound requests according to user preference%d",
            enabled);
        check(soundEnabled == (enabled != 0) && audioVoicesStarted == voices + (enabled ? 1u : 0u),
              label);
        stopSounds();
    }
    soundEnabled = true;
    stopSounds();
    u32 voices = audioVoicesStarted, sequence = audioJumpTraceSequence;
    bool nested = false;
    {
        ScopedObjectSoundMuteNative outer;
        {
            ScopedObjectSoundMuteNative inner;
            sound("jump");
        }
        sound("jump");
        nested = audioVoicesStarted == voices && audioJumpTraceSequence == sequence + 2 &&
                 audioJumpTraces[(sequence) & (AUDIO_JUMP_TRACE_CAPACITY - 1)].rejectReason ==
                     AUDIO_JUMP_OBJECT_MUTED &&
                 audioJumpTraces[(sequence + 1) & (AUDIO_JUMP_TRACE_CAPACITY - 1)].rejectReason ==
                     AUDIO_JUMP_OBJECT_MUTED;
    }
    sound("jump");
    check(
        nested && audioVoicesStarted == voices + 1 && soundEnabled,
        "Nested object mute suppresses only new requests, reports object mute separately from user disable and releases at the correct scope");
    stopSounds();
    AudioSilenceKeepAlive previousKeepalive = audioSilenceKeepAlive;
    audioSilenceKeepAlive = AudioSilenceKeepAlive{};
    audioSilenceKeepAlive.matched = true;
    static short silent[4410];
    {
        ScopedObjectSoundMuteNative muted;
        mixAudio(silent, 2205);
    }
    int peak = 0;
    for (int i = 0; i < 4410; i++)
        peak = (int)maxf((float)peak, absf((float)silent[i]));
    check(
        soundEnabled && audioSilenceKeepAlive.frames == 2205 && peak >= 831 && peak <= 832,
        "Native object mute leaves verified10Hz silent-output headphone compatibility active without creating any sound voice");
    audioSilenceKeepAlive = previousKeepalive;
    check(
        campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "Ending object mute and interleaved mixer checks do not alter campaign actors or checkpoint bytes");
    interfaceEnabled = false;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    stopSounds();
    startStage(0);
}
