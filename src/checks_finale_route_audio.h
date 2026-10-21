#pragma once

// Regression coverage for finale route audio. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct FinaleRouteAudioStats {
    int rises, voices, silentRises, ordinarySilent, frames;
    bool reached, alive, pcm, deviceAdvanced;
};

u32 finaleRouteJumpSerial() {
    u32 serial = 0;
    AudioClip *clip = jumpAudioClip();
    EnterCriticalSection(&audioLock);
    for (int i = 0; i < 32; i++)
        if (audioVoices[i].clip == clip && audioVoices[i].serial > serial)
            serial = audioVoices[i].serial;
    LeaveCriticalSection(&audioLock);
    return serial;
}

int finaleRouteQueuedBuffers() {
    int n = 0;
    for (int i = 0; i < AUDIO_BUFFERS; i++)
        if (audioHeaders[i].dwFlags & WHDR_INQUEUE)
            n++;
    return n;
}

FinaleRouteAudioStats finaleRouteAudioRun(bool debug, bool device, bool airborne,
                                          const char *path) {
    const float dt = 1.0f / 120.0f;
    FinaleRouteAudioStats out = {};
    interfaceEnabled = false;
    debugMode = debug;
    musicEnabled = false;
    soundEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = deathTimer = 0;
    int boss = -1, potion = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == BOSS)
            boss = i;
        if (entities[i].kind == POTION)
            potion = i;
    }
    if (boss < 0 || potion < 0)
        return out;
    stopSounds();
    soundEnabled = true;
    Entity &wizard = entities[boss];
    wizard.state.hp = 1;
    wizard.state.active = 3;
    wizard.state.timer = 2.1f;
    wizardHurt(wizard);
    // Exercise the genuine last-hit/death tick path before the route. The
    // route itself never rewrites Bean's position, velocity or contact flags.
    State &bean = entities[playerIndex].state;
    bean.x = 7100;
    bean.y = 320;
    bean.vx = bean.vy = 0;
    restorePhysics();
    short samples[368];
    for (int frame = 0; frame < 390; frame++) {
        tick(dt, false, false);
        if (device)
            Sleep(8);
        else
            mixAudio(samples, 184);
    }
    bool finalDead = wizard.state.gone && !wizardInputLocked();
    bean.x = 7100;
    bean.y = airborne ? 260.0f : 96 - entities[playerIndex].hit.b + 1;
    bean.vx = 0;
    bean.vy = airborne ? -40.0f : -20.0f;
    restorePhysics();
    readyTimer = 0;
    u32 previousMixed = audioSamplesMixed;
    static char log[16000];
    lstrcpyA(
        log,
        "frame,x100,y100,vx100,beforeVy100,afterVy100,preStepVy100,ground,support,stomp,spring,magnet,walk,rise,newJumpVoice,serial,deviceQueued,mixedFrames\r\n");
    const Rect potionBounds = hitRect(entities[potion]);
    u32 seen = finaleRouteJumpSerial();
    for (int frame = 0; frame < 1500 && hitRect(entities[playerIndex]).r < potionBounds.l - 3;
         frame++) {
        float before = bean.vy;
        tick(dt, false, true);
        out.frames++;
        bool rise = before <= 0 && bean.vy > 100;
        u32 serial = finaleRouteJumpSerial();
        bool voice = serial > seen;
        if (voice)
            seen = serial;
        bool ordinary = rise && onGround && preStepVy <= 0 && !killablePlayerStompedThisStep() &&
                        playerSpringContact < 0 && !playerMagnetContact && !playerWalkMode;
        if (rise) {
            out.rises++;
            if (!voice)
                out.silentRises++;
        }
        if (voice)
            out.voices++;
        if (ordinary && !voice)
            out.ordinarySilent++;
        if (rise || voice) {
            char row[512];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d,%u\r\n", frame,
                      (int)(bean.x * 100), (int)(bean.y * 100), (int)(bean.vx * 100),
                      (int)(before * 100), (int)(bean.vy * 100), (int)(preStepVy * 100), onGround,
                      supportEntity, killablePlayerStompedThisStep(), playerSpringContact,
                      playerMagnetContact, playerWalkMode, rise, voice, serial,
                      finaleRouteQueuedBuffers(), audioSamplesMixed);
            if (lstrlenA(log) + lstrlenA(row) < 15000)
                lstrcatA(log, row);
        }
        if (device)
            Sleep(8);
        else {
            mixAudio(samples, 184);
            for (int n = 0; n < 368; n++)
                out.pcm |= samples[n] != 0;
        }
        if (deathTimer > 0 || respawnBusy(respawnTransition) || endingScene.active ||
            screen != PLAY)
            break;
    }
    out.reached = bean.x > 7392 && hitRect(entities[playerIndex]).r >= potionBounds.l - 3 &&
                  !entities[potion].state.gone && !endingScene.active;
    out.alive = finalDead && deathTimer == 0 && !respawnBusy(respawnTransition) && screen == PLAY &&
                !playerWalkMode;
    out.deviceAdvanced = audioSamplesMixed > previousMixed;
    char tail[512];
    wsprintfA(
        tail,
        "# debug=%d device=%d airborne=%d reached=%d alive=%d frames=%d rises=%d voices=%d silentRises=%d ordinarySilent=%d pcm=%d deviceAdvanced=%d waveOutError=%u\r\n",
        debug, device, airborne, out.reached, out.alive, out.frames, out.rises, out.voices,
        out.silentRises, out.ordinarySilent, out.pcm, out.deviceAdvanced, audioError);
    lstrcatA(log, tail);
    atomicWrite(path, log, lstrlenA(log));
    return out;
}

void finaleRouteAudioChecks() {
    for (int mode = 0; mode < 4; mode++) {
        bool debug = (mode & 1) != 0, airborne = (mode & 2) != 0;
        char file[128];
        wsprintfA(file, "test-output/finale-route-audio-%d.csv", mode);
        FinaleRouteAudioStats s = finaleRouteAudioRun(debug, false, airborne, file);
        check(
            s.reached && s.alive,
            "The real post-Wizard staircase route reaches the potion approach using only held-right input and original geometry");
        check(
            s.rises > 0 && s.voices >= s.rises && s.ordinarySilent == 0 && s.silentRises == 0 &&
                s.pcm,
            "Every actual upward relaunch along the potion staircase has a new cached jump voice and nonzero mixed PCM");
        if (debug)
            check(
                jumps > 0,
                "Session unlock staircase traversal retains normal jump effects and counts jumps");
    }
    debugMode = false;
    musicEnabled = soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
}

// Called only by --audio-test with a real worker/device. Its saves are isolated
// from the player's campaign; device logs distinguish enqueue and PCM progress
// from the offline route test, without claiming to measure speaker audibility.
bool finaleRouteDeviceSmoke() {
    char oldSaveRoot[MAX_PATH];
    lstrcpyA(oldSaveRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/audio-route-saves");
    CreateDirectoryA(saveRoot, 0);
    bool ok = true;
    for (int mode = 0; mode < 2; mode++) {
        char file[128];
        wsprintfA(file, "test-output/finale-route-device-%d.csv", mode);
        FinaleRouteAudioStats s = finaleRouteAudioRun(mode != 0, true, false, file);
        ok &= s.reached && s.alive && s.rises > 0 && s.voices >= s.rises && !s.silentRises &&
              !s.ordinarySilent && s.deviceAdvanced && audioError == 0;
    }
    debugMode = false;
    stopSounds();
    lstrcpyA(saveRoot, oldSaveRoot);
    return ok;
}
