#pragma once

// Regression coverage for selector locked awards. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorLockedAwardsChecks() {
    Progress retained = progress;
    bool oldDebug = debugMode, oldInterface = interfaceEnabled;
    int oldStage = currentStage;
    debugMode = false;
    interfaceEnabled = true;
    pointerHeld = false;
    for (int i = 0; i < stageCount; i++) {
        progress.cleared[i] = 1;
        progress.bestCoins[i] = (int)levels[stageList[i]].props.num("bean-max-coins");
        progress.bestJumps[i] = (int)levels[stageList[i]].props.num("bean-jump-best", 999);
        progress.axolotls[i] = 1;
    }
    int stageBadges = 0, worldBadges = 0, locked = 0, unlocked = 0;
    bool stageVisibility = true, restored = true, buttonsAgree = true, debugVisible = true,
         decorPreserved = true;
    for (int w = 0; w < 5; w++) {
        progress.unlocked = firstStageInWorld(w + 1);
        loadUi(page, worldScenes[w]);
        screen = SELECT;
        for (int i = 0; i < page.level.objectCount; i++) {
            UiNode &node = page.nodes[i];
            Object &o = *node.object;
            if (selectorAwardObject(o)) {
                int stage = stageById(idnum(o.props.get("level")));
                bool expected = stage == progress.unlocked;
                stageBadges++;
                if (expected)
                    unlocked++;
                else
                    locked++;
                stageVisibility &= visibleNode(page, node) == expected;
            } else if (eq(o.props.get("name"), "Diamond") && !node.hidden)
                decorPreserved &= visibleNode(page, node);
            if (node.stage >= 0)
                buttonsAgree &= node.disabled == (node.stage > progress.unlocked);
        }
        progress.unlocked = stageCount - 1;
        for (int i = 0; i < page.level.objectCount; i++)
            if (selectorAwardObject(*page.nodes[i].object))
                restored &= visibleNode(page, page.nodes[i]);
        debugMode = true;
        progress.unlocked = 0;
        loadUi(page, worldScenes[w]);
        screen = SELECT;
        for (int i = 0; i < page.level.objectCount; i++) {
            UiNode &node = page.nodes[i];
            if (selectorAwardObject(*node.object))
                debugVisible &= visibleNode(page, node);
            if (node.stage >= 0)
                debugVisible &= !node.disabled;
        }
        debugMode = false;
    }
    check(
        stageBadges == 196 && unlocked == 20 && locked == 176 && stageVisibility && buttonsAgree,
        "All five selector maps hide all196 awarded icons and stars consistently with locked buttons, while each world's first unlocked node retains its four badges");
    check(
        restored && debugVisible && decorPreserved,
        "Unlocking or DEBUG restores ordinary/golden shoes and all other awards without hiding actual collectible diamonds in selector playfields");
    progress.unlocked = 0;
    loadUi(page, 18006219643615701355ULL);
    screen = WORLD;
    bool worldVisibility = true;
    for (int i = 0; i < page.level.objectCount; i++) {
        UiNode &node = page.nodes[i];
        if (selectorAwardObject(*node.object)) {
            u64 level = idnum(node.object->props.get("level"));
            worldBadges++;
            worldVisibility &= visibleNode(page, node) == (level == worldScenes[0]);
        }
    }
    check(
        worldBadges == 15 && worldVisibility,
        "World selection hides the15 summary badge actors on locked worlds while preserving the first unlocked world's three icons");
    progress.unlocked = firstStageInWorld(2);
    loadUi(page, worldScenes[1]);
    screen = SELECT;
    int lockedButton = -1;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].stage >= 0 && page.nodes[i].disabled && page.nodes[i].rect.l >= 24 &&
            page.nodes[i].rect.r <= 488 && page.nodes[i].rect.b >= 24 &&
            page.nodes[i].rect.t <= 344) {
            lockedButton = i;
            break;
        }
    bool pixels = false, iconWouldRender = false;
    if (lockedButton >= 0) {
        UiNode &button = page.nodes[lockedButton];
        u64 target = idnum(button.object->behavior("LevelButtonBehaviour")->get("level"));
        Rect crop = button.rect;
        crop = Rect{crop.l - 24, crop.b - 24, crop.r + 24, crop.t + 40};
        cloudRenderBegin();
        drawUiNode(page, button, lockedButton);
        u32 pureLock = cloudRenderPixels(crop, false);
        for (int i = 0; i < page.level.objectCount; i++)
            if (selectorAwardObject(*page.nodes[i].object) &&
                idnum(page.nodes[i].object->props.get("level")) == target)
                drawUiNode(page, page.nodes[i], i);
        pixels = cloudRenderPixels(crop, false) == pureLock;
        for (int i = 0; i < page.level.objectCount; i++)
            if (contains(page.nodes[i].object->props.get("name"), "Shoe") &&
                idnum(page.nodes[i].object->props.get("level")) == target) {
                UiNode &shoe = page.nodes[i];
                drawActor(shoe.art, shoe.rect, "best", 0);
                iconWouldRender = cloudRenderPixels(crop, false) != pureLock;
                break;
            }
        page.focus = lockedButton;
        transitionTime = 0;
        render();
        capture("test-output/selector-locked-awards.bmp");
    }
    check(
        pixels && iconWouldRender,
        "Actual OpenGL locked-node pixels equal the lock button alone even when completed/golden records exist; forcibly drawn shoe would visibly alter those pixels");
    debugMode = oldDebug;
    interfaceEnabled = oldInterface;
    progress = retained;
    currentStage = oldStage;
    screen = PLAY;
}
