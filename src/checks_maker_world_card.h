#pragma once

// Render over black and white for an exact transparent export, including antialiased text/shadows.
// The conversion step derives alpha from the two backgrounds rather than guessing a chroma key.
void makerCardArtworkExports(UiScene &s, UiNode &node, bool grasslands = false) {
    resizeWindow(832, 1024);
    Vec c = center(node.rect);
    const char *parts[] = {"cover", "scene", "title"};
    for (int part = 0; part < 3; part++)
        for (int background = 0; background < 2; background++) {
            glViewport(0, 0, viewportW, viewportH);
            glClearColor((float)background, (float)background, (float)background, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(c.x - 104, c.x + 104, c.y - 112, c.y + 144, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            if (grasslands) {
                // Match original Z values: scenery 4, button overlay 50, illustrated title 100.
                if (part != 2) {
                    Tile *art = makerChapterArtwork(s, 1);
                    if (art)
                        drawTile(*art, &s.level);
                    drawUiNode(s, node, (int)(&node - s.nodes));
                }
                if (part != 1)
                    for (int i = 0; i < s.level.objectCount; i++)
                        if (s.nodes[i].object->klass == 2648011722103052292ULL)
                            drawUiNode(s, s.nodes[i], i);
            } else if (part == 2)
                makerCardTitle(c);
            else
                makerDrawChapterIllustration(s, node, part == 0);
            SwapBuffers(gameDC);
            char path[96];
            wsprintfA(path, "test-output/%s-art-%s-%s.bmp", grasslands ? "grasslands" : "maker",
                      parts[part],
                      background ? "white" : "black");
            capture(path);
        }
}

// Exercise the real chapter controller with Maker both locked and unlocked. Test saveRoot is isolated.
void makerWorldCardChecks() {
    MakerProfile oldProfile = makerProfile;
    bool oldLoaded = makerProfileLoaded, oldSound = soundEnabled, oldMusic = musicEnabled;
    int oldStage = currentStage;
    makerProfileLoaded = true;
    makerProfile = MakerProfile{};
    soundEnabled = musicEnabled = false;
    currentStage = 0;
    worldSelectNativeChecks();
    check(worldSelectSlots == 6 && worldSelectFirstSlot == 1,
          "Locked Maker preserves the six original chapter slots and native drag boundaries");

    makerProfile.unlocked = makerProfile.seen = 1;
    navigate(18006219643615701355ULL);
    transitionTime = 0;
    int first = -1, maker = -1, boss = -1;
    bool authored = true;
    for (int i = 0; i < page.level.objectCount; i++) {
        UiNode &node = page.nodes[i];
        if (node.world == 1)
            first = i;
        if (node.world == 7)
            maker = i;
        if (node.world == 6)
            boss = i;
        if (node.world > 0 && node.world < 7)
            authored &= absf(center(node.rect).x -
                             (page.level.size.x * .5f + (displayWorld(node.world) - 1) * 224)) <
                        .001f;
    }
    bool present = first >= 0 && maker >= 0 && boss >= 0;
    check(present && authored && worldSelectSlots == 7 && worldSelectFirstSlot == 0 &&
              absf(center(page.nodes[first].rect).x - center(page.nodes[maker].rect).x - 224) <
                  .001f,
          "Unlocked Maker sits one slot before Grasslands; every campaign card keeps its original position");
    if (present) {
        int count = page.level.objectCount;
        makerDecorateWorldSelect();
        check(page.level.objectCount == count && worldSelectSlots == 7 &&
                  worldSelectFirstSlot == 0,
              "Decorating an existing chapter scene keeps Maker unique and preserves the slot range");
        page.focus = first;
        moveFocus("focus-left");
        tickInterface(.2f);
        bool left = page.focus == maker && selectedWorld == 7 &&
                    absf(page.cameraX - (center(page.nodes[maker].rect).x - viewW * .5f)) < .001f;
        moveFocus("focus-right");
        tickInterface(.2f);
        const Props *last = buttonProps(*page.nodes[boss].object);
        check(left && page.focus == first && selectedWorld == 1 && last &&
                  !*last->get("focus-right"),
              "Keyboard navigation crosses Maker and chapter one in both directions; Wizard remains the last card");

        bool drags = true, focus = true;
        const int sizes[][2] = {{1280, 720}, {1024, 768}, {640, 360}, {1600, 480}};
        for (int size = 0; size < 4; size++) {
            resizeWindow(sizes[size][0], sizes[size][1]);
            page.focus = first;
            selectedWorld = 1;
            page.cameraX = page.targetX = center(page.nodes[first].rect).x - viewW * .5f;
            // Rightward flick selects the card on the left; a reverse flick returns to Grasslands.
            for (int direction = 1; direction >= -1; direction -= 2) {
                float x = viewW * .5f, y = viewH * .5f;
                pointerDown(x, y);
                tickInterface(0);
                mouseInterface(x + direction * 30, y, false);
                tickInterface(.1f);
                pointerUp(x + direction * 30, y);
                tickInterface(.2f);
                int target = direction > 0 ? maker : first;
                drags &= page.focus == target &&
                         absf(page.cameraX - (center(page.nodes[target].rect).x - viewW * .5f)) <
                             .001f;
            }
            makerFocusWorldCard();
            tickInterface(.1f);
            focus &= page.focus == maker && selectedWorld == 7 &&
                     absf(page.cameraX - (center(page.nodes[maker].rect).x - viewW * .5f)) <
                         .001f;
            page.age = 2;
            transitionTime = 0;
            render();
            char path[96];
            wsprintfA(path, "test-output/maker-chapter-%dx%d.bmp", sizes[size][0], sizes[size][1]);
            capture(path);
        }
        check(drags, "Production pointer flicks cross Maker and Grasslands at four viewport sizes");
        check(focus, "Returning to Maker keeps its camera centered at four viewport sizes");
        check(makerCardSceneTexture.gl && makerCardTitleTexture.gl &&
                  makerCardSceneTexture.width == 1024 && makerCardSceneTexture.height == 1024 &&
                  makerCardTitleTexture.width == 1024 && makerCardTitleTexture.height == 1024 &&
                  glGetError() == GL_NO_ERROR,
              "Maker cover loads its edited scene and illustrated title as two transparent texture layers");
        bool pixelsMatch = true;
        const char *layers[] = {"scene", "title"};
        Texture *textures[] = {&makerCardSceneTexture, &makerCardTitleTexture};
        u8 *gpu = (u8 *)mkAlloc(1024 * 1024 * 4);
        for (int layer = 0; layer < 2; layer++) {
            char path[128];
            wsprintfA(path, "data/Media/Windows/Maker/%s.ptex", layers[layer]);
            u32 bytes;
            u8 *source = makerReadWhole(path, &bytes);
            glBindTexture(GL_TEXTURE_2D, textures[layer]->gl);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, gpu);
            pixelsMatch &= source && bytes == 56 + 1024 * 1024 * 4 &&
                           memcmp(source + 56, gpu, 1024 * 1024 * 4) == 0;
            mkFree(source);
        }
        mkFree(gpu);
        check(pixelsMatch, "GPU textures preserve every prepared RGBA pixel, including the edited transparent frame");
        makerCardArtworkExports(page, page.nodes[maker]);
        makerCardArtworkExports(page, page.nodes[first], true);
    }

    WorldSelectPointerNative pointer = {};
    worldSelectBeginNative(pointer, 200);
    pointer.justDown = false;
    pointer.dragged = true;
    int leftLimit = worldSelectReleaseNative(pointer, 1, 10000, 1);
    worldSelectBeginNative(pointer, 200);
    pointer.justDown = false;
    pointer.dragged = true;
    int rightLimit = worldSelectReleaseNative(pointer, 1, -10000, 1);
    check(leftLimit == 0 && rightLimit == 6 && worldSelectTargetNative(0, 0) == -224 &&
              worldSelectTargetNative(6, 0) == 1120 &&
              worldSelectTargetNative(0, -100) < -224 &&
              worldSelectTargetNative(0, -100) > -324,
          "Drag clamps to Maker and Wizard, with elastic resistance beyond the new left boundary");

    resizeWindow(1024, 768);
    makerProfile = oldProfile;
    makerProfileLoaded = oldLoaded;
    currentStage = oldStage;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    navigate(18006219643615701355ULL);
}
