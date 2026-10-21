#pragma once

// Regression coverage for jump audio followup. Runs only in the isolated BQ_ENABLE_TESTS executable.

AudioClip *jumpAudioClip() {
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, "jump"))
            return &soundClips[i].clip;
    return 0;
}

u32 newestJumpAudioSerial() {
    AudioClip *clip = jumpAudioClip();
    u32 serial = 0;
    for (int i = 0; i < 32; i++)
        if (audioVoices[i].clip == clip && audioVoices[i].serial > serial)
            serial = audioVoices[i].serial;
    return serial;
}

void jumpAudioFollowupChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = false;
    soundEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    check(
        boss >= 0 && jumpAudioClip() && jumpAudioClip()->frames > 0,
        "Original Wizard finale and cached ordinary jump effect exist for the reported post-boss silent stair bounce");
    if (boss < 0)
        return;
    soundEnabled = true;
    stopSounds();
    stopMusic();
    Entity &enemy = entities[boss];
    enemy.state.hp = 1;
    enemy.state.active = 3;
    enemy.state.timer = 2.1f;
    wizardHurt(enemy);
    bool deathSound = false;
    for (int i = 0; i < 32; i++)
        if (audioVoices[i].clip)
            for (int n = 0; n < soundClipCount; n++)
                if (eq(soundClips[n].name, "malmo-death") &&
                    audioVoices[i].clip == &soundClips[n].clip)
                    deathSound = true;
    check(
        deathSound && soundEnabled && audioInitialized && playingMusic == 0,
        "Wizard's final hit stops only music and leaves its death effect and sound-effect mixer active");
    short pcm[384];
    for (int i = 0; i < 380; i++) {
        updateWizard(enemy, 1.0f / 120.0f);
        mixAudio(pcm, 192);
    }
    check(
        enemy.state.gone && !wizardInputLocked(),
        "The actual final Wizard death sequence retires its actor and releases gameplay input before the potion stairs");
    // Exact authored final staircase: x7328..7392 has top192, next step
    // x7392..7552 has top224. x7360 is left of the UnbeanPotion sensor.
    State &bean = entities[playerIndex].state;
    bean.x = 7360;
    bean.y = 192 - entities[playerIndex].hit.b + 0.1f;
    bean.vx = 0;
    bean.vy = -20;
    restorePhysics();
    u32 reads = audioFileReads;
    int bounces = 0, sounded = 0;
    bool mixed = false;
    u32 seen = newestJumpAudioSerial();
    for (int frame = 0; frame < 960; frame++) {
        float before = bean.vy;
        tick(1.0f / 120.0f, false, false);
        if (ordinaryJumpLaunchedNative && bean.vy > 100) {
            bounces++;
            u32 now = newestJumpAudioSerial();
            if (now > seen) {
                sounded++;
                seen = now;
            }
        }
        mixAudio(pcm, 192);
        for (int n = 0; n < 384; n++)
            mixed |= pcm[n] != 0;
    }
    check(
        bounces >= 4 && sounded == bounces && mixed && audioFileReads == reads && soundEnabled &&
            playingMusic == 0,
        "Every real potion-stair landing after the final Wizard death starts and mixes its own cached jump sound without BGM or file IO");
    // The user screenshot has DEBUG enabled. Native AddJump suppression must
    // neither suppress the ordinary sound nor retain a stomp suppression flag.
    debugMode = true;
    jumps = 0;
    stopSounds();
    bean.x = 7360;
    bean.y = 192 - entities[playerIndex].hit.b + 0.1f;
    bean.vx = 0;
    bean.vy = -20;
    restorePhysics();
    seen = newestJumpAudioSerial();
    bool debugBounce = false, debugSound = false;
    for (int frame = 0; frame < 30; frame++) {
        float before = bean.vy;
        tick(1.0f / 120.0f, false, false);
        if (ordinaryJumpLaunchedNative && bean.vy > 100) {
            debugBounce = true;
            debugSound = newestJumpAudioSerial() > seen;
            break;
        }
    }
    check(
        debugBounce && debugSound && jumps == 1,
        "Session unlock mode counts the post-boss stair bounce and retains its normal jump effect");
    char trace[240];
    wsprintfA(
        trace,
        "scene=976871008727 stair-x=7360 top=192 bounces=%d jump-voices=%d mixed=%d death-sound=%d debug-bounce=%d debug-sound=%d reads-delta=%u\r\n",
        bounces, sounded, mixed, deathSound, debugBounce, debugSound, audioFileReads - reads);
    atomicWrite("test-output/post-wizard-jump-audio.txt", trace, lstrlenA(trace));
    debugMode = false;
    musicEnabled = soundEnabled = false;
    stopSounds();
    startStage(stageById(976871008727ULL));
}
