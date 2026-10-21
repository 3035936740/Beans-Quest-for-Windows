#pragma once

// Regression coverage for jump awards native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void wizardHudNativeChecks() {
    bool oldInterface = interfaceEnabled;
    interfaceEnabled = true;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int hidden = 0;
    bool pauseVisible = false;
    for (int i = 0; i < hudScene.level.objectCount; i++) {
        UiNode &n = hudScene.nodes[i];
        Object &o = *n.object;
        if (o.props.flag("anchor-left") && o.props.flag("anchor-top") && !visibleNode(hudScene, n))
            hidden++;
        if (o.id == 15781655334108297295ULL)
            pauseVisible = visibleNode(hudScene, n);
    }
    check(
        hidden == 9 && pauseVisible,
        "Wizard entry hides all nine native top-left HUD actors while retaining the pause button");
    bool rescue = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS) {
            entities[i].state.active = 5;
            entities[i].state.gone = 1;
        }
    captureCheckpoint(false);
    restoreCheckpoint();
    openPause(false);
    for (int i = 0; i < hudScene.level.objectCount; i++) {
        UiNode &n = hudScene.nodes[i];
        Object &o = *n.object;
        if (o.props.flag("anchor-left") && o.props.flag("anchor-top"))
            rescue &= !visibleNode(hudScene, n);
    }
    check(rescue && screen == PAUSE,
          "Defeated Wizard checkpoint and pause retain boss-mode HUD suppression during rescue");
    restartStage();
    bool retry = true;
    for (int i = 0; i < hudScene.level.objectCount; i++) {
        UiNode &n = hudScene.nodes[i];
        Object &o = *n.object;
        if (o.props.flag("anchor-left") && o.props.flag("anchor-top"))
            retry &= !visibleNode(hudScene, n);
    }
    startStage(0);
    readyTimer = 0;
    int regular = 0;
    for (int i = 0; i < hudScene.level.objectCount; i++) {
        UiNode &n = hudScene.nodes[i];
        Object &o = *n.object;
        if (o.props.flag("anchor-left") && o.props.flag("anchor-top") && visibleNode(hudScene, n))
            regular++;
    }
    check(
        retry && regular == 6,
        "Wizard retry keeps counters hidden and entering an ordinary level restores both complete six-actor counter groups");
    interfaceEnabled = oldInterface;
}

bool jumpAwardCaptionPixels(u64 id, const char *expected) {
    for (int i = 0; i < hudScene.level.objectCount; i++)
        if (hudScene.nodes[i].object->id == id) {
            Rect r = {80, 180, 280, 212};
            cloudRenderBegin();
            originalCaption(hudScene, hudScene.nodes[i], r);
            u32 actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
            cloudRenderBegin();
            drawTextObject(*hudScene.nodes[i].object, r, expected);
            return actual == cloudRenderPixels(Rect{0, 0, 512, 384}, false);
        }
    return false;
}

void jumpAwardsNativeChecks() {
    Progress retained = progress;
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/jump-awards-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    memset(&progress, 0, sizeof(progress));
    progress.unlocked = stageCount - 1;
    startStage(0);
    readyTimer = 0;
    u64 first = levels[stageList[0]].id;
    check((int)current.props.num("bean-jump-par") == 15 &&
              (int)current.props.num("bean-jump-best") == 11 && stageJumpTarget == 15 &&
              eq(shoeAnimationForLevel(first), "off"),
          "Original1-1 begins at0/15 with no shoe and distinct authored golden11-jump threshold");
    jumps = 11;
    captureCheckpoint(false);
    restoreCheckpoint();
    check(
        stageJumpTarget == 15 && progress.cleared[0] == 0 && jumpTargetForStage(0) == 15,
        "A checkpoint or unfinished eleven-jump attempt does not award a shoe or change the persistent target");
    finish();
    check(
        progress.bestJumps[0] == 11 && stageJumpTarget == 15 && jumpTargetForStage(0) == 11 &&
            eq(shoeAnimationForLevel(first), "best"),
        "Eleven-jump completion awards the native golden best clip and updates result target while the current HUD retains its entry cache");
    check(
        jumpAwardCaptionPixels(10498441872698914575ULL, "11") &&
            jumpAwardCaptionPixels(8364913218960181251ULL, "BEST"),
        "Actual result value and BEST label use the newly updated record rather than the HUD entry target");
    restartStage();
    readyTimer = 0;
    check(stageJumpTarget == 11 && jumps == 0,
          "Reentering after an eleven-jump record displays0/11 instead of the authored0/15");
    progress.bestJumps[0] = 20;
    startStage(0);
    readyTimer = 0;
    check(stageJumpTarget == 15 && eq(shoeAnimationForLevel(first), "off"),
          "A worse-than-par completed record keeps the standard15-jump target without an award");
    check(
        jumpAwardCaptionPixels(10498441872698914575ULL, "15") &&
            jumpAwardCaptionPixels(8364913218960181251ULL, "PAR"),
        "Actual result retains PAR and15 when the stored record is worse than the authored target");
    progress.bestJumps[0] = 15;
    check(eq(shoeAnimationForLevel(first), "on"),
          "Exactly meeting15 awards the ordinary shoe rather than the golden11 award");
    progress.bestJumps[0] = 12;
    check(
        eq(shoeAnimationForLevel(first), "on"),
        "A personal improvement below par but above authored best still awards the ordinary shoe");
    progress.bestJumps[0] = 11;
    progress.bestCoins[0] = 35;
    progress.axolotls[0] = 1;
    check(
        stageBadge(0, 3),
        "Complete-star requirements retain the native ordinary par award plus all diamonds and salamander");
    progress.bestJumps[0] = 0;
    startStage(0);
    readyTimer = 0;
    check(stageJumpTarget == 0 && eq(shoeAnimationForLevel(first), "best"),
          "A genuinely stored zero record remains a valid native displayed target and award");
    for (int i = 0; i < stageCount; i++)
        if ((int)levels[stageList[i]].props.num("bean-world") == 1) {
            progress.cleared[i] = 1;
            progress.bestJumps[i] = (int)levels[stageList[i]].props.num("bean-jump-best", 999);
        }
    bool allBest = eq(shoeAnimationForLevel(worldScenes[0]), "best");
    progress.bestJumps[0] = 15;
    bool allPar = eq(shoeAnimationForLevel(worldScenes[0]), "on");
    progress.bestJumps[0] = 16;
    check(
        allBest && allPar && eq(shoeAnimationForLevel(worldScenes[0]), "off"),
        "World shoe aggregates all authored level awards with best-before-par priority and falls off when one stage misses par");
    progress.bestJumps[0] = 11;
    startStage(0);
    readyTimer = 0;
    interfaceEnabled = true;
    stageInterface(true);
    jumps = 0;
    int caption = -1;
    for (int i = 0; i < hudScene.level.objectCount; i++)
        if (hudScene.nodes[i].object->id == 8775559588655153142ULL)
            caption = i;
    bool hudPixels = false;
    if (caption >= 0) {
        Object &o = *hudScene.nodes[caption].object;
        Rect r = {80, 180, 280, 212};
        cloudRenderBegin();
        originalCaption(hudScene, hudScene.nodes[caption], r);
        u32 actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
        cloudRenderBegin();
        drawTextObject(o, r, "0 / |FFFF00|11");
        hudPixels = actual == cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    }
    check(
        hudPixels,
        "Actual HUD caption OpenGL output matches0/11 after loading the persistent eleven-jump best");
    jumps = 16;
    check(
        jumpAwardCaptionPixels(8775559588655153142ULL, "16 / |FFFF00|11"),
        "HUD keeps the current count white and the cached target yellow even after exceeding the target");
    jumps = 0;
    openSelect();
    int shoe = -1;
    for (int i = 0; i < page.level.objectCount; i++) {
        Object &o = *page.nodes[i].object;
        if (contains(o.props.get("name"), "Shoe") && idnum(o.props.get("level")) == first)
            shoe = i;
    }
    bool goldPixels = false, animated = false;
    if (shoe >= 0) {
        UiNode &n = page.nodes[shoe];
        page.age = .15f;
        Rect r = visualRect(page, n);
        cloudRenderBegin();
        drawUiNode(page, n, shoe);
        u32 actual = cloudRenderPixels(r, false);
        cloudRenderBegin();
        drawActor(n.art, r, "best", page.age, n.object->angle);
        goldPixels = actual == cloudRenderPixels(r, false);
        cloudRenderBegin();
        drawActor(n.art, r, "on", page.age, n.object->angle);
        goldPixels &= actual != cloudRenderPixels(r, false);
        page.age = .35f;
        cloudRenderBegin();
        drawUiNode(page, n, shoe);
        animated = cloudRenderPixels(r, false) != actual;
    }
    check(
        goldPixels && animated,
        "Actual level-selector shoe uses the animated original golden best artwork instead of permanent ordinary on artwork");
    transitionTime = 0;
    render();
    capture("test-output/gold-shoe-native.bmp");
    interfaceEnabled = false;
    progress = retained;
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}

void completionGoldShoeChecks() {
    Progress retained = progress;
    bool old = interfaceEnabled;
    interfaceEnabled = true;
    debugMode = false;
    startStage(0);
    readyTimer = 0;
    jumps = 11;
    finish();
    goalLaunch.age = 2;
    hudScene.age = .15f;
    int index = stageClearNode(4449368542626346951ULL);
    bool gold = false, blue = false, animated = false;
    if (index >= 0) {
        UiNode &n = hudScene.nodes[index];
        Rect r = visualRect(hudScene, n);
        float side = minf(r.r - r.l, r.t - r.b);
        Vec c = center(r);
        Rect gr = {c.x - side * .5f, c.y - side * .5f, c.x + side * .5f, c.y + side * .5f};
        cloudRenderBegin();
        drawUiNode(hudScene, n, index);
        u32 actual = cloudRenderPixels(r, false);
        cloudRenderBegin();
        drawActor(actor(7897082502132702692ULL), gr, "best", hudScene.age, n.object->angle);
        gold = actual == cloudRenderPixels(r, false);
        cloudRenderBegin();
        drawActor(n.art, r, "", hudScene.age, n.object->angle);
        gold &= actual != cloudRenderPixels(r, false);
        hudScene.age = .35f;
        cloudRenderBegin();
        drawUiNode(hudScene, n, index);
        animated = actual != cloudRenderPixels(r, false);
        jumps = 12;
        cloudRenderBegin();
        drawUiNode(hudScene, n, index);
        u32 regular = cloudRenderPixels(r, false);
        cloudRenderBegin();
        drawActor(n.art, r, "", hudScene.age, n.object->angle);
        blue = regular == cloudRenderPixels(r, false);
        jumps = 11;
        transitionTime = 0;
        render();
        capture("test-output/completion-gold-shoe.bmp");
    }
    check(
        gold,
        "Gold-threshold completion renders the original gold-shoe artwork in the actual result-panel node");
    check(
        animated,
        "The completion gold shoe retains its original best-award animation instead of a recoloured static blue sprite");
    check(
        blue && progress.bestJumps[0] <= 11,
        "A later non-gold run retains its blue completion shoe even when a previous stored record earned gold");
    interfaceEnabled = old;
    progress = retained;
    startStage(0);
    readyTimer = 0;
}
