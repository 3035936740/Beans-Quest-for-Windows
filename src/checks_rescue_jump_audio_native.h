#pragma once

// Regression coverage for rescue jump audio native. Runs only in the isolated BQ_ENABLE_TESTS executable.
struct RescueAudioSweepStats {
    int cases, launches, rises, silentLaunches, silentRises, voiced, pcmCases, ceilings, stopped,
        errors;
};

RescueAudioSweepStats rescueJumpAudioSweep(bool device, bool allRates) {
    lstrcpyA(saveRoot, "test-output/rescue-audio-sweep-saves");
    CreateDirectoryA("test-output", 0);
    CreateDirectoryA(saveRoot, 0);
    memset(&progress, 0, sizeof(progress));
    progress.unlocked = 49;
    if (hudScene.level.id != 14897698531695279339ULL)
        initInterface();
    interfaceEnabled = false;
    if (device) {
        shutdownAudio();
        initAudio(soundFiles, true);
    }
    const int rates[] = {30, 75, 120, 144};
    const int starts[] = {6360, 6560, 6800, 7100, 7280, 7360};
    RescueAudioSweepStats total = {};
    HANDLE out = CreateFileA(device ? "test-output/rescue-audio-device-sweep.csv"
                                    : "test-output/rescue-jump-audio-native-sweep.csv",
                             GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    DWORD written;
    const char *head =
        "case,rate,start,pattern,debug,frame,x100,y100,preVy100,vy100,timer1000,ground,support,headContact,launch,rise,jumpVoice,newestSerial,mixedNonzero\r\n";
    WriteFile(out, head, lstrlenA(head), &written, 0);
    for (int ri = 0; ri < 4; ri++)
        for (int si = 0; si < 6; si++)
            for (int pattern = 0; pattern < 3; pattern++) {
                if (!allRates && ri != 2)
                    continue;
                if (device && (ri != 2 || pattern != 2 || (si != 0 && si != 3 && si != 5)))
                    continue;
                const float dt = 1.0f / rates[ri];
                musicEnabled = soundEnabled = false;
                debugMode = (pattern & 1) != 0;
                startStage(stageById(976871008727ULL));
                readyTimer = 0;
                int boss = -1;
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].kind == BOSS)
                        boss = i;
                if (boss < 0) {
                    total.errors++;
                    continue;
                }
                Entity &wizard = entities[boss];
                wizard.state.hp = 1;
                wizard.state.active = 3;
                wizard.state.timer = 2.1f;
                soundEnabled = true;
                stopSounds();
                wizardHurt(wizard);
                State &bean = entities[playerIndex].state;
                bean.x = 7100;
                bean.y = 320;
                bean.vx = bean.vy = 0;
                restorePhysics();
                short samples[1600];
                for (int frame = 0; frame < 390; frame++) {
                    tick(1.0f / 120, false, false);
                    if (device)
                        Sleep(8);
                    else
                        mixAudio(samples, 184);
                }
                if (!wizard.state.gone || wizardInputLocked()) {
                    total.errors++;
                    continue;
                }
                bean.x = (float)starts[si];
                float top = starts[si] >= 7328   ? 192
                            : starts[si] >= 7264 ? 160
                            : starts[si] >= 7200 ? 128
                                                 : 96;
                bean.y = top - entities[playerIndex].hit.b + 1;
                bean.vx = 0;
                bean.vy = -20;
                bean.hp = 0;
                entities[playerIndex].originX = bean.x;
                restorePhysics();
                stopSounds();
                u32 seen = voiceSerial;
                bool rightward = true, nonzero = false;
                int launches = 0, silent = 0;
                total.cases++;
                for (int frame = 0; frame < rates[ri] * 8; frame++) {
                    if (bean.x > 7410)
                        rightward = false;
                    if (bean.x < 6300)
                        rightward = true;
                    bool right = pattern == 1 ? true : pattern == 2 ? rightward : false;
                    bool left = pattern == 2 && !rightward;
                    float oldVy = bean.vy;
                    tick(dt, left, right);
                    bool launch = bean.timer == 0 && bean.vy > 0 && !playerWalkMode &&
                                  screen == PLAY && !endingScene.active;
                    bool rise = oldVy <= 0 && bean.vy > 30;
                    bool voice = false;
                    u32 newest = seen;
                    EnterCriticalSection(&audioLock);
                    for (int i = 0; i < 32; i++)
                        if (audioVoices[i].clip == jumpAudioClip() &&
                            audioVoices[i].serial > seen) {
                            voice = true;
                            if (audioVoices[i].serial > newest)
                                newest = audioVoices[i].serial;
                        }
                    LeaveCriticalSection(&audioLock);
                    seen = voiceSerial;
                    int headContact = 0;
                    for (b2ContactEdge *edge = bodies[playerIndex]->GetContactList(); edge;
                         edge = edge->next) {
                        b2Contact *c = edge->contact;
                        if (!c->IsTouching() || !c->IsEnabled() || !c->GetManifold()->pointCount ||
                            c->GetFixtureA()->IsSensor() || c->GetFixtureB()->IsSensor())
                            continue;
                        b2WorldManifold m;
                        c->GetWorldManifold(&m);
                        float ny = c->GetFixtureA()->GetBody() == bodies[playerIndex] ? -m.normal.y
                                                                                      : m.normal.y;
                        if (ny < -.5f)
                            headContact++;
                    }
                    int n = (int)(22050 * dt);
                    int pcm = 0;
                    if (device) {
                        u32 oldMixed = audioNonzeroSamples;
                        Sleep(8);
                        pcm = (int)(audioNonzeroSamples - oldMixed);
                    } else {
                        mixAudio(samples, n);
                        for (int n0 = 0; n0 < n * 2; n0++)
                            if (samples[n0])
                                pcm++;
                    }
                    nonzero |= pcm != 0;
                    total.ceilings += headContact != 0;
                    if (launch) {
                        launches++;
                        total.launches++;
                        if (!voice) {
                            silent++;
                            total.silentLaunches++;
                        }
                    }
                    if (rise) {
                        total.rises++;
                        if (!voice)
                            total.silentRises++;
                    }
                    if (voice)
                        total.voiced++;
                    if (launch || rise || voice || headContact) {
                        char row[512];
                        wsprintfA(row,
                                  "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d\r\n",
                                  total.cases, rates[ri], starts[si], pattern, debugMode, frame,
                                  (int)(bean.x * 100), (int)(bean.y * 100), (int)(preStepVy * 100),
                                  (int)(bean.vy * 100), (int)(bean.timer * 1000), onGround,
                                  supportEntity, headContact, launch, rise, voice, newest, pcm);
                        WriteFile(out, row, lstrlenA(row), &written, 0);
                    }
                    if (deathTimer > 0 || respawnBusy(respawnTransition) || endingScene.active ||
                        screen != PLAY || playerWalkMode) {
                        total.stopped++;
                        break;
                    }
                }
                total.pcmCases += nonzero;
                char msg[256];
                wsprintfA(
                    msg, "case=%d rate=%d start=%d pattern=%d launches=%d silent=%d nonzero=%d\r\n",
                    total.cases, rates[ri], starts[si], pattern, launches, silent, nonzero);
                OutputDebugStringA(msg);
            }
    CloseHandle(out);
    char result[700];
    wsprintfA(
        result,
        "cases=%d launches=%d rises=%d silentLaunches=%d silentRises=%d voiced=%d pcmCases=%d ceilingContactFrames=%d stopped=%d setupErrors=%d\r\n",
        total.cases, total.launches, total.rises, total.silentLaunches, total.silentRises,
        total.voiced, total.pcmCases, total.ceilings, total.stopped, total.errors);
    atomicWrite(device ? "test-output/rescue-audio-device-sweep-result.txt"
                       : "test-output/rescue-jump-audio-native-result.txt",
                result, lstrlenA(result));
    if (device) {
        char d[256];
        wsprintfA(d, "waveOutError=%u mixed=%u nonzero=%u queued=%d frames=%u\r\n", audioError,
                  audioSamplesMixed, audioNonzeroSamples, finaleRouteQueuedBuffers(),
                  jumpAudioClip() ? jumpAudioClip()->frames : 0);
        atomicWrite("test-output/rescue-audio-device-status.txt", d, lstrlenA(d));
    }
    return total;
}

void rescueJumpAudioNativeChecks() {
    char oldSaveRoot[MAX_PATH];
    lstrcpyA(oldSaveRoot, saveRoot);
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled;
    Progress oldProgress = progress;
    RescueAudioSweepStats s = rescueJumpAudioSweep(false, false);
    check(
        s.cases == 18 && s.errors == 0,
        "Rescue audio probe reaches the real defeated-Wizard corridor and all six princess-stair approach starts at the actual 120 Hz simulation step");
    check(
        s.launches >= 100 && s.rises <= s.launches && s.voiced == s.launches && !s.silentLaunches &&
            !s.silentRises,
        "Every real rescue-corridor stationary, rightward and return-route jump starts a new cached jump voice, including DEBUG and a reset count latch");
    check(
        s.pcmCases == s.cases,
        "All eighteen real rescue-corridor traversal cases produce nonzero mixed PCM after the Wizard music has stopped");
    debugMode = false;
    musicEnabled = soundEnabled = false;
    stopSounds();
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldSaveRoot);
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    progress = oldProgress;
}
