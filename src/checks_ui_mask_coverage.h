#pragma once

// Compare real translucent sprite pixels across the viewport, including the
// former exposed side margins. A white backdrop makes both gaps and repeated
// alpha blending visible without depending on the level's background art.
void uiMaskCheckDraw(UiScene &scene, int index) {
    glDisable(GL_SCISSOR_TEST);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    nativeViewportProjection();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    UiNode &node = scene.nodes[index];
    float x, y;
    layerOffset(scene, node.object->layer, &x, &y);
    glPushMatrix();
    glTranslatef(x, y, 0);
    drawUiNode(scene, node, index);
    glPopMatrix();
    glReadBuffer(GL_BACK);
}

bool uiMaskCheckRow(float y) {
    int row = (int)(y * viewportH / viewH);
    if (row < 1 || row >= viewportH - 1)
        return true; // The panel has already moved outside the screen.
    static u8 pixels[512 * 4];
    glReadPixels(0, row, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int shade = pixels[256 * 4];
    if (shade >= 245)
        return false;
    for (int x = 1; x < 511; x++)
        for (int c = 0; c < 3; c++)
            if (pixels[x * 4 + c] != shade)
                return false;
    return true;
}

void uiMaskCoverageChecks() {
    int oldWidth = clientW, oldHeight = clientH;
    interfaceEnabled = true;
    musicEnabled = soundEnabled = debugMode = false;
    const int sizes[][2] = {{1024, 768}, {1280, 720}, {1920, 560},
                           {3440, 1440}, {3200, 160}, {560, 1100}};
    const float ages[] = {.6f, 2.25f, 2.375f};
    bool readyPixels = true, boundaries = true, oldMarginsDetected = false;
    startStage(0);
    int upper = stageClearNode(17109890148369247723ULL),
        lower = stageClearNode(11001010908721541493ULL);
    check(upper >= 0 && lower >= 0, "Ready coverage resolves both authored translucent panels");
    if (upper < 0 || lower < 0)
        return;
    Rect authoredUpper = hudScene.nodes[upper].rect, authoredLower = hudScene.nodes[lower].rect;
    for (int size = 0; size < 6; size++) {
        resizeNativeViewport(sizes[size][0], sizes[size][1]);
        // Render the full logical viewport into a bounded, real GL framebuffer.
        viewportW = 512;
        viewportH = 384;
        for (int age = 0; age < 3; age++) {
            screen = PLAY;
            readyTimer = 2.5f - ages[age];
            readyOpening = ReadyNativeState{ages[age], true};
            const int nodes[] = {upper, lower};
            for (int part = 0; part < 2; part++) {
                UiNode &n = hudScene.nodes[nodes[part]];
                float x, y;
                layerOffset(hudScene, n.object->layer, &x, &y);
                Rect r = shifted(visualRect(hudScene, n), x, y);
                Vec originalCentre = center(n.rect);
                Rect view = uiLayerViewportRect(hudScene, n.object->layer);
                Vec pose = readyNativePosition(ages[age], originalCentre, view, false);
                float shiftY = pose.y - originalCentre.y + y;
                boundaries &= absf((part ? r.t - authoredLower.t : r.b - authoredUpper.b) -
                                   shiftY) < .001f;
                float inner = part ? minf(viewH, r.t) : maxf(0, r.b);
                float sample = part ? inner * .5f : (inner + viewH) * .5f;
                if ((part && r.t > 2) || (!part && r.b < viewH - 2)) {
                    uiMaskCheckDraw(hudScene, nodes[part]);
                    readyPixels &= uiMaskCheckRow(sample);
                }
                oldMarginsDetected |= n.rect.r - n.rect.l < viewW - 2;
            }
        }
    }
    check(readyPixels && oldMarginsDetected,
          "Ready shades cover both viewport edges without alpha seams at six window sizes and three animation phases; cases expose the former fixed-width gap");
    check(boundaries && hudScene.nodes[upper].rect.l == authoredUpper.l &&
              hudScene.nodes[lower].rect.b == authoredLower.b,
          "Ready coverage preserves authored source rectangles, inner opening edges and native outward animation displacement");

    navigate(137502285691ULL);
    int shade = endingCreditsNode(endingCreditsScene.fadeIn),
        illustration = endingCreditsNode(endingCreditsScene.tint);
    check(shade >= 0 && illustration >= 0, "Ending coverage resolves the background shade separately from its illustration");
    if (shade >= 0 && illustration >= 0) {
        bool pixels = true, art = true;
        Rect authoredArt = page.nodes[illustration].rect;
        const float times[] = {9, 12, 40};
        for (int size = 0; size < 6; size++) {
            resizeNativeViewport(sizes[size][0], sizes[size][1]);
            viewportW = 512;
            viewportH = 384;
            for (int age = 0; age < 3; age++) {
                page.age = times[age];
                page.cameraY = endingCreditsCameraY(page.age) - viewH * .5f;
                uiMaskCheckDraw(page, shade);
                pixels &= uiMaskCheckRow(viewH * .05f) && uiMaskCheckRow(viewH * .5f) &&
                          uiMaskCheckRow(viewH * .95f);
                Rect r = visualRect(page, page.nodes[illustration]);
                art &= r.l == authoredArt.l && r.b == authoredArt.b &&
                       r.r == authoredArt.r && r.t == authoredArt.t;
            }
        }
        check(pixels, "Ending background shade covers every edge during partial/full darkening and scrolling at all six window sizes");
        check(art, "Ending coverage retains the illustration's original bounds and its separate RGB darkening animation");
    }
    resizeNativeViewport(oldWidth, oldHeight);
    startStage(0);
    readyTimer = 0;
}

// Capture the composed game frames, not just the isolated alpha coverage.
void uiMaskCoveragePreviews() {
    int oldWidth = clientW, oldHeight = clientH;
    resizeWindow(1920, 560);
    int stage = 0;
    for (int i = 0; i < stageCount; i++) {
        const Props &p = levels[stageList[i]].props;
        if (p.num("bean-world") == 4 && p.num("bean-level") == 2)
            stage = i;
    }
    startStage(stage);
    readyOpening = ReadyNativeState{.6f, true};
    readyTimer = 1.9f;
    transitionTime = 0;
    nativeViewportProjection();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawWorld();
    drawOriginalScene(hudScene, false);
    captureResponsiveBack("test-output/ui-mask-ready-wide.bmp");

    navigate(137502285691ULL);
    page.age = 9;
    page.cameraY = endingCreditsCameraY(page.age) - viewH * .5f;
    nativeViewportProjection();
    drawOriginalScene(page);
    captureResponsiveBack("test-output/ui-mask-ending-wide.bmp");
    resizeWindow(oldWidth, oldHeight);
    startStage(0);
    readyTimer = 0;
}
