#pragma once

// Regression coverage for ui jump detail native. Runs only in the isolated BQ_ENABLE_TESTS executable.

int uiJumpDustCountNative() {
    int count = 0;
    u64 id = idnum(page.bean.physics->get("dust-actor"));
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition && effectParticles[i].domain == 1 &&
            effectParticles[i].art && effectParticles[i].art->id == id)
            count++;
    return count;
}

int prepareUiJumpDetailNative(u64 scene, u64 source) {
    if (!hudScene.level.id)
        initInterface();
    navigate(scene);
    page.minimized = true;
    page.minimizePhase = 1;
    page.miniDeath = 0;
    page.level.geometryCount = 0;
    int index = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        Entity &e = page.playfield[i];
        if (e.source->id == source && boxGroupMatrixOwner(e))
            index = i;
        else if (e.source->id != source)
            e.state.gone = 1;
    }
    if (index < 0)
        return -1;
    destroyUiPhysics(page);
    Entity &pad = page.playfield[index];
    Rect r = boxGroupMatrixFixture(pad);
    page.bean.state.x = pad.state.x + (r.l + r.r) * .5f;
    page.bean.state.y = pad.state.y + r.t - page.bean.hit.b + .2f;
    page.bean.state.vx = 0;
    page.bean.state.vy = -30;
    page.bean.state.timer = .1f;
    page.trampolineApex = 340;
    createUiPhysics(page);
    clearEffects(1);
    stopSounds();
    memset(keys, 0, sizeof(keys));
    return index;
}

void uiJumpDetailNativeChecks() {
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = soundEnabled = true;
    debugMode = musicEnabled = false;
    stopMusic();
    defaultBindings();
    u32 stage = hashBytes(entities, sizeof(Entity) * entityCount),
        save = hashBytes(&checkpoint, sizeof(checkpoint)),
        mega = hashBytes(&pinballPendingNative, sizeof(pinballPendingNative)),
        tramp = hashBytes(&trampolineStateNative, sizeof(trampolineStateNative));
    int oldJumps = jumps;
    const float dt = 1.0f / 120;
    const u64 springs[][2] = {
        {71376317651ULL, 576871468884ULL},      {71376317651ULL, 701188307820ULL},
        {799282380888ULL, 9064547182192831ULL}, {669776786229ULL, 6212760147216476ULL},
        {137502285691ULL, 1323211665147700ULL}, {137502285691ULL, 9106860118099698ULL}};
    char rows[16000] = {};
    for (int n = 0; n < 6; n++) {
        int pad = prepareUiJumpDetailNative(springs[n][0], springs[n][1]);
        bool ok = false;
        if (pad >= 0) {
            stepUiPhysics(page, dt);
            stepUiPhysics(page, dt);
            const Props &p = *page.bean.physics;
            float expected = root(-2 * p.num("gravity", -200) *
                                  maxf(p.num("jump-height", 37), page.playfield[pad].jumpPower)) +
                             p.num("gravity", -200) * dt;
            bool animated = false;
            for (int i = 0; i < page.playfieldCount; i++)
                if (page.playfield[i].source == page.playfield[pad].source &&
                    page.playfield[i].state.active == 2)
                    animated = true;
            ok = absf(page.bean.state.vy - expected) < .02f && animated &&
                 pinballDetailVoiceCount("megajump") >= 1 && pinballDetailVoiceCount("jump") == 0;
            char row[250], scene[24];
            idstr(springs[n][0], scene);
            wsprintfA(
                row,
                "scene=%s spring=%d gravity100=%d power100=%d vy100=%d expected100=%d megaVoices=%d jumpVoices=%d\r\n",
                scene, pad, (int)(p.num("gravity", -200) * 100),
                (int)(page.playfield[pad].jumpPower * 100), (int)(page.bean.state.vy * 100),
                (int)(expected * 100), pinballDetailVoiceCount("megajump"),
                pinballDetailVoiceCount("jump"));
            lstrcatA(rows, row);
        }
        char label[180];
        wsprintfA(
            label,
            "Original UI spring placement%d uses its own Tom gravity, authored height and only its dedicated bounce sound",
            n);
        check(ok, label);
    }
    const u64 jellies[] = {6285546912625162820ULL, 12499371799514561398ULL};
    for (int n = 0; n < 2; n++) {
        int pad = prepareUiJumpDetailNative(12038123762473955738ULL, jellies[n]);
        tickInterface(dt);
        tickInterface(dt);
        check(
            pad >= 0 && page.specialJump.launchedKind == 2 &&
                page.specialJump.launchedSource == pad && page.bean.state.vy > 200 &&
                pinballDetailVoiceCount("jump") == 1 && pinballDetailVoiceCount("megajump") == 0,
            n == 0
                ? "The original left selector jelly plays one ordinary jump sound without a spurious strong bounce"
                : "The original right selector jelly plays one ordinary jump sound without a spurious strong bounce");
        check(uiJumpDustCountNative() == 0,
              n == 0 ? "A left selector jelly launch does not add ordinary-ground dust"
                     : "A right selector jelly launch does not add ordinary-ground dust");
    }
    for (int kind = 0; kind < 2; kind++) {
        u64 scene = kind ? 669776786229ULL : 12038123762473955738ULL,
            source = kind ? 6212760147216476ULL : jellies[0];
        int pad = prepareUiJumpDetailNative(scene, source);
        bool noContact = false, ordinary = false;
        if (pad >= 0) {
            b2Fixture *own = page.bodies[pad]->GetFixtureList();
            b2Filter filter = own->GetFilterData();
            filter.maskBits &= ~2;
            own->SetFilterData(filter);
            Rect r = hitRect(page.playfield[pad]);
            page.bean.state.x = page.playfield[pad].state.x;
            page.bean.state.y = r.t - page.bean.hit.b + .2f;
            page.bean.state.vy = -30;
            resetUiPhysics(page);
            b2BodyDef bd;
            b2Body *floor = page.physics->CreateBody(&bd);
            b2PolygonShape shape;
            shape.SetAsBox(16 * metres, 8 * metres,
                           b2Vec2(page.bean.state.x * metres, (r.t - 8) * metres), 0);
            floor->CreateFixture(&shape, 0);
            clearEffects(1);
            stopSounds();
            tickInterface(dt);
            tickInterface(dt);
            ordinary =
                page.ordinaryJumpNative &&
                absf(page.bean.state.vy -
                     (page.bean.jumpPower + page.bean.physics->num("gravity", -200) * dt)) < .02f &&
                pinballDetailVoiceCount("jump") == 1;
            noContact = page.contacts->trampoline < 0;
            for (int i = 0; i < page.contacts->incomingCount; i++)
                if (page.contacts->incoming[i].target == page.playfieldCount &&
                    page.contacts->incoming[i].solved)
                    noContact = false;
            char row[220];
            wsprintfA(row,
                      "ghostKind=%d pad=%d active=%d ordinary=%d actualPadContact=%d dust=%d\r\n",
                      kind, pad, page.playfield[pad].state.active, ordinary, !noContact,
                      uiJumpDustCountNative());
            lstrcatA(rows, row);
        }
        check(
            pad >= 0 && noContact && ordinary && page.playfield[pad].state.active != 2,
            kind
                ? "Ordinary ground beneath a noncolliding spring cannot animate that nearby spring through a sprite overlap"
                : "Ordinary ground beneath a noncolliding jelly cannot animate that nearby jelly through a sprite overlap");
        check(
            ordinary && uiJumpDustCountNative() > 0,
            "Ordinary selector terrain still emits its original jump dust after mechanism-only dust is removed");
    }
    int spring = prepareUiJumpDetailNative(669776786229ULL, 6212760147216476ULL);
    tickInterface(dt);
    float takeoffY = page.bean.state.y;
    tickInterface(dt);
    check(
        spring >= 0 && page.bean.state.vy > 200 && uiJumpDustCountNative() == 0,
        "Actual selector spring contact retains its animation and bounce without ordinary-ground dust");
    float launchY = takeoffY, peak = page.bean.state.y;
    for (int frame = 0; frame < 360 && page.bean.state.vy > 0; frame++) {
        tickInterface(dt);
        peak = maxf(peak, page.bean.state.y);
    }
    check(
        peak - launchY > 190 && peak - launchY < 193,
        "The miniature spring's actual unsteered flight rises about its authored192pixels rather than the former doubled384pixel height");
    spring = prepareUiJumpDetailNative(669776786229ULL, 6212760147216476ULL);
    page.bean.state.vy = 0;
    resetUiPhysics(page);
    stepUiPhysics(page, dt);
    stepUiPhysics(page, dt);
    check(
        spring >= 0 && page.ordinaryJumpNative &&
            absf(page.bean.state.vy -
                 (page.bean.jumpPower + page.bean.physics->num("gravity", -200) * dt)) < .02f &&
            pinballDetailVoiceCount("jump") == 1 && pinballDetailVoiceCount("megajump") == 0,
        "A zero-incoming spring contact retains its ordinary request and audible minimized-selector jump");
    page.ordinaryJumpNative = true;
    resetUiPhysics(page);
    bool cleared = !page.ordinaryJumpNative;
    page.ordinaryJumpNative = true;
    stepUiPhysics(page, 0);
    check(cleared && !page.ordinaryJumpNative,
          "Reset and skipped zero-time UI physics clear the transient ordinary-jump feedback flag");
    check(
        stage == hashBytes(entities, sizeof(Entity) * entityCount) &&
            save == hashBytes(&checkpoint, sizeof(checkpoint)) &&
            mega == hashBytes(&pinballPendingNative, sizeof(pinballPendingNative)) &&
            tramp == hashBytes(&trampolineStateNative, sizeof(trampolineStateNative)) &&
            oldJumps == jumps,
        "UI jump feedback and miniature power changes cannot mutate campaign state, pending mechanisms, checkpoint or jump count");
    atomicWrite("test-output/ui-jump-detail-observations.txt", rows, lstrlenA(rows));
    stopSounds();
    clearEffects(1);
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
