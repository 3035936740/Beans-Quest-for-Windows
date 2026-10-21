#pragma once

// Regression coverage for selector deferred native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorDeferredNativeChecks() {
    const float dt = 1.0f / 120;
    interfaceEnabled = soundEnabled = true;
    musicEnabled = false;
    debugMode = true;
    defaultBindings();
    stopMusic();
    const u64 locations[][2] = {{12038123762473955738ULL, 6285546912625162820ULL},
                                {12038123762473955738ULL, 12499371799514561398ULL},
                                {71376317651ULL, 576871468884ULL},
                                {71376317651ULL, 701188307820ULL},
                                {799282380888ULL, 9064547182192831ULL},
                                {669776786229ULL, 6212760147216476ULL},
                                {137502285691ULL, 1323211665147700ULL},
                                {137502285691ULL, 9106860118099698ULL}};
    bool placements = true, queued = true, voices = true, launched = true, integrated = true,
         isolated = true;
    int pads = 0;
    char rows[7000] = "placement,first_vy100,first_jump,first_mega,second_jump,second_mega\r\n";
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint)),
        mega = hashBytes(&pinballPendingNative, sizeof(pinballPendingNative)),
        jelly = hashBytes(&trampolineStateNative, sizeof(trampolineStateNative));
    for (int c = 0; c < 8; c++) {
        int i = prepareUiJumpDetailNative(locations[c][0], locations[c][1]);
        if (i < 0) {
            placements = false;
            continue;
        }
        pads++;
        UiScene &s = page;
        bool trampoline = s.playfield[i].kind == TRAMPOLINE;
        float g = s.bean.physics->num("gravity", -200),
              height = s.bean.physics->num("jump-height", 37);
        stepUiPhysics(s, dt);
        bool contact = s.contacts->trampoline == i;
        for (int k = 0; k < s.contacts->incomingCount; k++)
            contact |= s.contacts->incoming[k].source == i &&
                       s.contacts->incoming[k].target == s.playfieldCount &&
                       s.contacts->incoming[k].solved;
        placements &= contact;
        queued &= s.bean.state.vy < 100;
        int firstJump = pinballDetailVoiceCount("jump"),
            firstMega = pinballDetailVoiceCount("megajump"), firstVy = (int)(s.bean.state.vy * 100);
        voices &= firstJump == 0 && (trampoline ? firstMega == 0 : firstMega >= 1);
        float contactY = s.bean.state.y;
        float speed = trampoline
                          ? root(-2 * g * (s.trampolineApex - nativeTrampolineRoundedY(contactY)))
                          : root(-2 * g * maxf(height, s.playfield[i].jumpPower));
        stopSounds();
        stepUiPhysics(s, dt);
        launched &= s.bean.state.vy > 100 && absf(s.bean.state.vy - (speed + g * dt)) < .02f &&
                    s.bean.state.y > contactY;
        integrated &= absf(s.bean.state.y - (contactY + (speed + g * dt) * dt)) < .01f &&
                      absf(s.bodies[s.playfieldCount]->GetLinearVelocity().y / metres -
                           s.bean.state.vy) < .001f;
        voices &= pinballDetailVoiceCount("jump") == (!trampoline ? 0 : 1) &&
                  pinballDetailVoiceCount("megajump") == 0;
        char row[180];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d\r\n", c, firstVy, firstJump, firstMega,
                  pinballDetailVoiceCount("jump"), pinballDetailVoiceCount("megajump"));
        lstrcatA(rows, row);
    }
    check(
        placements && pads == 8,
        "All2original selector jellies and6selector/ending spring placements participate in actual independent physics contacts");
    check(
        queued,
        "Original miniature jelly and spring contacts queue their jump rather than applying velocity in the same solved-contact tick");
    check(
        launched,
        "All8miniature special jumps launch on the following physical update with their own gravity integrated in that launch step");
    check(
        integrated,
        "Miniature special-jump body velocity and displacement match the independent semi-implicit launch-plus-gravity solution");
    check(
        voices,
        "Spring sound occurs at contact and jelly sound at deferred launch without duplicate ordinary-jump voices");
    bool rejected = true, boundary = true, separated = true, reset = true;
    for (int c = 0; c < 2; c++) {
        int i = prepareUiJumpDetailNative(locations[c ? 5 : 0][0], locations[c ? 5 : 0][1]);
        stepUiPhysics(page, dt);
        page.bean.state.timer = .049f;
        stopSounds();
        stepUiPhysics(page, dt);
        rejected &= i >= 0 && page.bean.state.vy < 100 && pinballDetailVoiceCount("jump") == 0;
        i = prepareUiJumpDetailNative(locations[c ? 5 : 0][0], locations[c ? 5 : 0][1]);
        stepUiPhysics(page, dt);
        page.bean.state.timer = .05f;
        stopSounds();
        stepUiPhysics(page, dt);
        boundary &= i >= 0 && page.bean.state.vy > 100;
        i = prepareUiJumpDetailNative(locations[c ? 5 : 0][0], locations[c ? 5 : 0][1]);
        stepUiPhysics(page, dt);
        page.bean.state.x += 120;
        page.bean.state.y += 100;
        page.bodies[page.playfieldCount]->SetTransform(
            b2Vec2(page.bean.state.x * metres, page.bean.state.y * metres), 0);
        stepUiPhysics(page, dt);
        separated &= i >= 0 && page.bean.state.vy > 100;
        i = prepareUiJumpDetailNative(locations[c ? 5 : 0][0], locations[c ? 5 : 0][1]);
        stepUiPhysics(page, dt);
        page.bean.state.x += 120;
        page.bean.state.y += 100;
        page.bean.state.vy = 0;
        resetUiPhysics(page);
        stepUiPhysics(page, dt);
        reset &= i >= 0 && page.bean.state.vy <= 0;
    }
    check(
        rejected,
        "Miniature special requests younger than the native0.05second old-age boundary are discarded without a delayed stale launch");
    check(
        boundary,
        "Both miniature MegaJump and jelly requests accept the exact native0.05second old-age boundary");
    check(
        separated,
        "A valid miniature special request survives separation from its contact before the following update consumes it");
    check(
        reset,
        "Resetting miniature physics clears old mechanism requests instead of launching a respawned Bean");
    isolated = campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
               saved == hashBytes(&checkpoint, sizeof(checkpoint)) &&
               mega == hashBytes(&pinballPendingNative, sizeof(pinballPendingNative)) &&
               jelly == hashBytes(&trampolineStateNative, sizeof(trampolineStateNative));
    check(
        isolated,
        "Selector and ending special-jump requests leave all campaign entities, snapshots and both campaign request records untouched");
    bool pinballs = true;
    const u64 pinballIds[] = {890074998190988ULL, 1105562976745860ULL};
    for (int c = 0; c < 2; c++) {
        navigate(137502285691ULL);
        int pad = -1;
        for (int k = 0; k < page.playfieldCount; k++)
            if (page.playfield[k].source->id == pinballIds[c])
                pad = k;
        bool ok = pad >= 0;
        if (ok) {
            destroyUiPhysics(page);
            page.level.geometryCount = 0;
            for (int k = 0; k < page.playfieldCount; k++)
                if (k != pad)
                    page.playfield[k].state.gone = 1;
            Entity &b = page.playfield[pad];
            page.bean.state.x = b.state.x + 32 - page.bean.hit.l - .1f;
            page.bean.state.y = b.state.y;
            page.bean.state.vx = -30;
            page.bean.state.vy = 0;
            page.bean.state.timer = .1f;
            createUiPhysics(page);
            stopSounds();
            stepUiPhysics(page, dt);
            PinballPendingNative request = page.specialJump.mega;
            float vx = page.bean.state.vx;
            ok = request.pending && request.source == pad &&
                 pinballDetailVoiceCount("pinball") > 0 && pinballDetailVoiceCount("jump") == 0;
            stepUiPhysics(page, dt);
            ok &= page.specialJump.launchedKind == 1 && page.specialJump.launchedSource == pad &&
                  absf(page.bean.state.vx - (vx + request.speed * request.normal.x)) < .02f &&
                  absf(page.bean.state.vy - (request.speed * request.normal.y +
                                             page.bean.physics->num("gravity", -200) * dt)) < .02f;
        }
        pinballs &= ok;
    }
    check(
        pinballs,
        "Both actual ending pinballs retain their real manifold-normal MegaJump through the next solver step instead of losing Tom impacts");
    prepareUiJumpDetailNative(locations[0][0], locations[0][1]);
    page.bean.state.x += 120;
    page.bean.state.y += 100;
    page.bean.state.vy = 0;
    resetUiPhysics(page);
    page.bean.state.timer = .05f;
    page.specialJump.mega = PinballPendingNative{1, 0, Vec{0, 1}, 200};
    page.specialJump.jelly = TrampolineStateNative{1, 1, 340, 300};
    stepUiPhysics(page, dt);
    check(page.specialJump.launchedKind == 1 &&
              absf(page.bean.state.vy - (200 + page.bean.physics->num("gravity", -200) * dt)) <
                  .02f &&
              !page.specialJump.jelly.pending,
          "Miniature MegaJump takes native precedence and clears the competing jelly request once");
    page.specialJump.mega = PinballPendingNative{1, 0, Vec{0, 1}, 200};
    page.miniDeath = .1f;
    stepUiPhysics(page, dt);
    check(page.specialJump.launchedKind == 0 && !page.specialJump.mega.pending,
          "A miniature death state cannot consume an old request into a live launch");
    atomicWrite("test-output/selector-deferred-contact-audio.csv", rows, lstrlenA(rows));
    stopSounds();
    soundEnabled = false;
    interfaceEnabled = false;
    debugMode = false;
    startStage(0);
    readyTimer = 0;
}
