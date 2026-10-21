#pragma once

// Regression coverage for campaign trampoline placements. Runs only in the isolated BQ_ENABLE_TESTS executable.
int jellySoundVoiceNative();
bool jellySoundDrainNative(int slot, const char *wav);

// All25 authored campaign placements from the original resource inventory.
void campaignTrampolinePlacementChecks() {
    const u64 pads[][2] = {{15974012089803411243ULL, 12132029551037129326ULL},
                           {15974012089803411243ULL, 5796044792201048883ULL},
                           {15974012089803411243ULL, 8550981875067517896ULL},
                           {15974012089803411243ULL, 4425596266358620537ULL},
                           {15974012089803411243ULL, 10058737931524503676ULL},
                           {8315980062923458623ULL, 1055505038776570404ULL},
                           {8315980062923458623ULL, 1433549067415226519ULL},
                           {8315980062923458623ULL, 2467973747054197926ULL},
                           {8315980062923458623ULL, 11350837716537434851ULL},
                           {10756198405680061017ULL, 12403547100276307249ULL},
                           {10756198405680061017ULL, 5578283343803505588ULL},
                           {10756198405680061017ULL, 8564353475960485266ULL},
                           {13052395652604067555ULL, 10241860368736639943ULL},
                           {669939580618ULL, 930587990107ULL},
                           {806701038719ULL, 186898655684ULL},
                           {806701038719ULL, 414974238589ULL},
                           {789619714186ULL, 8811787717024908ULL},
                           {789619714186ULL, 91987635878108ULL},
                           {789619714186ULL, 6696044913517260ULL},
                           {739118707375ULL, 897717886617ULL},
                           {739118707375ULL, 34282487919ULL},
                           {976871008727ULL, 9568817419616918ULL},
                           {976871008727ULL, 5901992709801738ULL},
                           {5679669680221079ULL, 5533788617726170ULL},
                           {5679669680221079ULL, 9040708684420698ULL}};
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/campaign-pad-saves");
    CreateDirectoryA(saveRoot, 0);
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    char rows[16000] = {};
    for (int n = 0; n < 25; n++) {
        startStage(stageById(pads[n][0]));
        readyTimer = deathTimer = 0;
        readyOpening.showing = false;
        current.geometryCount = 0;
        int pad = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == TRAMPOLINE && entities[i].source->id == pads[n][1] &&
                boxGroupMatrixOwner(entities[i])) {
                pad = i;
                break;
            }
        bool ok = false;
        if (pad >= 0) {
            Object *source = entities[pad].source;
            for (int i = 0; i < entityCount; i++)
                if (i != playerIndex && entities[i].source != source)
                    entities[i].state.gone = 1;
            Entity &e = entities[pad];
            Entity &bean = entities[playerIndex];
            Rect r = boxGroupMatrixFixture(e);
            bean.state.x = e.state.x + (r.l + r.r) * .5f;
            bean.state.y = e.state.y + r.t - bean.hit.b + .2f;
            bean.state.vx = 0;
            bean.state.vy = -30;
            bean.state.hp = 1;
            bean.state.timer = .1f;
            jumps = 0;
            rebuildPhysics();
            stopSounds();
            for (int f = 0; f < 40 && !trampolineStateNative.pending; f++)
                tick(1.0f / 120, false, false);
            TrampolineStateNative request = trampolineStateNative;
            bool pending = request.pending && request.source == pad && jumps == 0 &&
                           pinballDetailVoiceCount("jump") == 0;
            tick(1.0f / 120, false, false);
            ok = pending && trampolineLaunchedThisStepNative && jumps == 1 &&
                 absf(bean.lastLaunchSpeed - request.speed) < .001f &&
                 pinballDetailVoiceCount("jump") == 1 && pinballDetailVoiceCount("megajump") == 0 &&
                 jellySoundDrainNative(jellySoundVoiceNative(), 0);
        }
        char scene[24], id[24], row[450];
        idstr(pads[n][0], scene);
        idstr(pads[n][1], id);
        wsprintfA(
            row,
            "Campaign jelly original scene=%s source=%s retains an independent real contact and one deferred launch",
            scene, id);
        check(ok, row);
        lstrcatA(rows, ok ? "PASS " : "FAIL ");
        lstrcatA(rows, row);
        lstrcatA(rows, "\r\n");
    }
    atomicWrite("test-output/campaign-jelly-all-placements.txt", rows, lstrlenA(rows));
    stopSounds();
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldRoot);
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
