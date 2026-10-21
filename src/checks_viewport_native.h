#pragma once

// Regression coverage for viewport native. Runs only in the isolated BQ_ENABLE_TESTS executable.

u32 viewportRegionHash(Rect region) {
    int x = (int)(region.l * screenScale), y = (int)(region.b * screenScale),
        w = (int)((region.r - region.l) * screenScale),
        h = (int)((region.t - region.b) * screenScale);
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > viewportW || y + h > viewportH)
        return 0;
    u32 bytes = w * h * 4;
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, bytes);
    if (!pixels)
        return 0;
    glReadBuffer(GL_BACK);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    u32 result = hashBytes(pixels, bytes);
    HeapFree(GetProcessHeap(), 0, pixels);
    return result;
}

void viewportNativeChecks() {
    struct Case {
        int w, h;
        float scale;
    };

    const Case cases[] = {{512, 384, 1},           {1024, 768, 2},   {892, 402, 1},
                          {1280, 720, 2},          {1280, 960, 3},   {960, 640, 2},
                          {800, 600, 2},           {800, 480, 1.5f}, {1280, 319, 319 / 320.0f},
                          {1280, 320, 1},          {1280, 479, 1},   {1280, 599, 1.5f},
                          {1280, 641, 2},          {1280, 959, 2},   {1280, 960, 3},
                          {200, 240, 200 / 480.0f}};
    bool exact = true;
    for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        const Case &c = cases[i];
        NativeViewportSize size = nativeViewportSize(c.w, c.h);
        exact &= absf(size.scale - c.scale) < 0.00001f &&
                 absf(size.width * c.scale - c.w) < 0.001f &&
                 absf(size.height * c.scale - c.h) < 0.001f;
    }
    check(
        exact,
        "Native viewport preserves all original320,480,600,640and960height scale thresholds and the short-screen480wide fallback");
    NativeViewportSize wide = nativeViewportSize(892, 402),
                       standard = nativeViewportSize(1024, 768),
                       small = nativeViewportSize(512, 384), minimized = nativeViewportSize(0, 0);
    check(
        wide.width == 892 && wide.height == 402 && standard.width == 512 &&
            standard.height == 384 && small.width == 512 && small.height == 384,
        "Original892by402wide window exposes a wider playfield while both legacy4to3resolutions retain their matching512by384view");
    check(minimized.scale > 0 && minimized.width > 0 && minimized.height > 0,
          "Minimized Windows client dimensions retain a finite viewport until restoration");
    RECT saved;
    GetWindowRect(window, &saved);
    const int actual[][2] = {{892, 402}, {1024, 768}, {1280, 720}};
    bool clientExact = true, projectionExact = true, pointerExact = true, edges = true;
    endingScene.active = false;
    pointerHeld = false;
    for (int i = 0; i < 3; i++) {
        int w = actual[i][0], h = actual[i][1];
        resizeWindow(w, h);
        RECT client;
        GetClientRect(window, &client);
        NativeViewportSize size = nativeViewportSize(w, h);
        clientExact &= client.right == w && client.bottom == h && clientW == w && clientH == h &&
                       viewportX == 0 && viewportY == 0 && viewportW == w && viewportH == h &&
                       absf(viewW - size.width) < 0.001f && absf(viewH - size.height) < 0.001f;
        nativeViewportProjection();
        GLint viewport[4] = {};
        GLfloat matrix[16] = {};
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetFloatv(GL_PROJECTION_MATRIX, matrix);
        projectionExact &= viewport[0] == 0 && viewport[1] == 0 && viewport[2] == w &&
                           viewport[3] == h && absf(matrix[0] - 2 / viewW) < 0.000001f &&
                           absf(matrix[5] - 2 / viewH) < 0.000001f && matrix[12] == -1 &&
                           matrix[13] == -1;
        Vec topLeft = nativePointerPosition(0, 0),
            bottomRight = nativePointerPosition((float)w, (float)h);
        pointerExact &= topLeft.x == 0 && absf(topLeft.y - viewH) < 0.001f &&
                        absf(bottomRight.x - viewW) < 0.001f && bottomRight.y == 0;
        int px = w * 3 / 4, py = h / 4;
        SendMessageA(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(px, py));
        pointerExact &= absf(pointerStartX - px / screenScale) < 0.001f &&
                        absf(pointerStartY - (h - py) / screenScale) < 0.001f;
        ReleaseCapture();
        pointerHeld = false;
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        box(0, 0, 16, 16, 1, 0, 0);
        box(viewW - 16, viewH - 16, 16, 16, 0, 1, 0);
        u8 a[4] = {}, b[4] = {};
        glReadBuffer(GL_BACK);
        glReadPixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, a);
        glReadPixels(w - 2, h - 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, b);
        edges &= a[0] > 240 && a[1] < 10 && b[0] < 10 && b[1] > 240;
    }
    check(
        clientExact,
        "Actual Windows resize messages use the entire892by402,1024by768and1280by720client without letterbox margins");
    check(
        projectionExact && edges,
        "Actual OpenGL viewport and projection draw to both physical client corners at every native aspect ratio");
    check(
        pointerExact,
        "Actual Windows mouse-down dispatch converts client pixels through native scale and agrees with full-screen virtual corners");
    interfaceEnabled = true;
    startStage(0);
    readyTimer = 0;
    transitionTime = 0;
    endingCreditsFade = 0;
    respawnTransition = RespawnTransition{};
    int pause = -1, counter = -1;
    for (int i = 0; i < hudScene.level.objectCount; i++) {
        Object &o = *hudScene.nodes[i].object;
        if (o.id == 15781655334108297295ULL)
            pause = i;
        if (o.id == 8799797905732598206ULL)
            counter = i;
    }
    bool anchored = pause >= 0 && counter >= 0, hit = true;
    Rect first = {};
    for (int pass = 0; pass < 4 && anchored; pass++) {
        int w = pass == 1 ? 1024 : 892, h = pass == 1 ? 768 : 402;
        resizeWindow(w, h);
        screen = PLAY;
        hudScene.focus = -1;
        int indices[] = {pause, counter};
        Rect actualRects[2];
        for (int j = 0; j < 2; j++) {
            UiNode &n = hudScene.nodes[indices[j]];
            float x, y;
            layerOffset(hudScene, n.object->layer, &x, &y);
            Rect r = shifted(visualRect(hudScene, n), x, y);
            actualRects[j] = r;
            Layer &layer = hudScene.level.layers[n.object->layer];
            Rect authored = n.object->rect;
            anchored &= absf((viewH - r.t) - (layer.size.y - authored.t)) < 0.001f;
            if (j == 0)
                anchored &= absf((viewW - r.r) - (layer.size.x - authored.r)) < 0.001f;
            else
                anchored &= absf(r.l - authored.l) < 0.001f;
        }
        Rect r = actualRects[0];
        if (pass == 0)
            first = r;
        if (pass >= 2)
            anchored &= absf(r.l - first.l) < 0.001f && absf(r.t - first.t) < 0.001f;
        Vec c = center(r);
        int mx = (int)(c.x * screenScale), my = clientH - (int)(c.y * screenScale);
        Vec logical = nativePointerPosition((float)mx, (float)my);
        hit &= pointerHit(logical.x, logical.y) == pause;
        SendMessageA(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(mx, my));
        hit &= pointerNode == pause;
        ReleaseCapture();
        pointerHeld = false;
    }
    check(
        anchored,
        "Actual HUD counter and pause button preserve authored top and left or right edge margins across repeated wide and standard resizes without drift");
    check(
        hit && pause >= 0,
        "Actual resized top-right HUD button remains hittable through both virtual hit testing and Windows mouse-down dispatch");
    resizeWindow(892, 402);
    int coin = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == COIN && !entities[i].state.gone) {
            coin = i;
            break;
        }
    bool distant = false;
    if (coin >= 0) {
        Entity &e = entities[coin];
        camX = e.state.x - 350 - viewW * 0.5f;
        camY = e.state.y - viewH * 0.5f;
        Vec shift = layerTranslation(current, e.layer, camX, camY);
        Rect region = shifted(entityRect(e), shift.x, shift.y);
        nativeViewportProjection();
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        drawWorld();
        u32 present = viewportRegionHash(region);
        e.state.gone = 1;
        glClear(GL_COLOR_BUFFER_BIT);
        drawWorld();
        u32 absent = viewportRegionHash(region);
        e.state.gone = 0;
        distant = region.l > 512 && region.r < viewW && present && present != absent;
    }
    check(
        distant,
        "Actual original stage diamond350world units right of the camera centre renders in the newly exposed wide playfield beyond the former512pixel canvas");
    screen = COMPLETE;
    complete = true;
    goalLaunch.active = 1;
    goalLaunch.age = 0;
    StageClearNativePose wideEntry = completionUiPose();
    goalLaunch.age = 2;
    StageClearNativePose wideDone = completionUiPose();
    check(
        absf(wideEntry.remainingUiOffsetY + 2 * viewH) < 0.001f && wideDone.focusReady &&
            wideDone.stageClearRect.l == 96,
        "Completion panel derives its wide-window entrance from the actual camera height and retains authored resting placement");
    complete = false;
    screen = PLAY;
    goalLaunch.active = 0;
    interfaceEnabled = false;
    SetWindowPos(window, 0, saved.left, saved.top, saved.right - saved.left,
                 saved.bottom - saved.top, SWP_NOZORDER | SWP_NOACTIVATE);
    nativeViewportProjection();
}
