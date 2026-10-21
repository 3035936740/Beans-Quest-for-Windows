#pragma once

// Regression coverage for stage clear ui. Runs only in the isolated BQ_ENABLE_TESTS executable.

int stageClearNode(u64 id) {
    for (int i = 0; i < hudScene.level.objectCount; i++)
        if (hudScene.nodes[i].object->id == id)
            return i;
    return -1;
}

Rect stageClearNodeRect(int index) {
    UiNode &n = hudScene.nodes[index];
    Rect r = visualRect(hudScene, n);
    float x, y;
    layerOffset(hudScene, n.object->layer, &x, &y);
    return shifted(r, x, y);
}

void stageClearUiChecks() {
    interfaceEnabled = true;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = transitionTime = 0;
    coins = 15;
    jumps = 15;
    salamander = 0;
    int banner = stageClearNode(167178695708687765ULL),
        portrait = stageClearNode(14545820365738952861ULL),
        frame = stageClearNode(16931184945670423894ULL);
    check(banner >= 0 && portrait >= 0 && frame >= 0,
          "Original completion controller resolves its actual banner, portrait and panel actors");
    if (banner < 0 || portrait < 0 || frame < 0)
        return;
    int stage = currentStage;
    finish();
    check(
        screen == COMPLETE && goalLaunch.age == 0 && hudScene.focus < 0,
        "GOAL immediately starts native completion Show instead of an invented1.2-second static-menu delay");
    confirm();
    check(currentStage == stage && screen == COMPLETE,
          "Completion cannot activate Continue before the native2-second focus boundary");
    tick(0.25f, false, false);
    Rect r = stageClearNodeRect(banner), p = stageClearNodeRect(portrait),
         f = stageClearNodeRect(frame);
    check(
        absf(r.l - 96) < 0.001f && absf(r.b - 170) < 0.001f && p.t < 0 && f.t < 0,
        "At0.25s the actual STAGE CLEAR actor is centred while Bean portrait and whole panel remain below the screen");
    captureCase("test-output/stage-clear-banner-025.bmp");
    cloudRenderBegin();
    float dx, dy;
    layerOffset(hudScene, 2, &dx, &dy);
    glPushMatrix();
    glTranslatef(dx, dy, 0);
    drawUiNode(hudScene, hudScene.nodes[banner], banner);
    glPopMatrix();
    u32 real = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    drawActor(hudScene.nodes[banner].art, Rect{96, 170, 416, 214}, "", 0);
    u32 expected = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    u32 empty = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    check(
        real == expected && real != empty,
        "Original STAGE CLEAR bitmap is visibly rendered at the native intermediate centre, not merely recorded in UI state");
    tick(1, false, false);
    r = stageClearNodeRect(banner);
    f = stageClearNodeRect(frame);
    p = stageClearNodeRect(portrait);
    check(
        absf(r.b - 256.97412f) < 0.002f && absf(f.b + 176.94196f) < 0.002f && p.t < 0,
        "Banner rises and every original panel actor follows the same native bottom-to-top half-sine offset");
    captureCase("test-output/stage-clear-panel-125.bmp");
    tick(0.45f, false, false);
    p = stageClearNodeRect(portrait);
    f = stageClearNodeRect(frame);
    check(
        p.b > 196 && f.b == 48 && hudScene.focus < 0,
        "Bean portrait has its own delayed upward overshoot after the rest of the original panel has settled");
    captureCase("test-output/stage-clear-bean-170.bmp");
    tick(0.3f, false, false);
    p = stageClearNodeRect(portrait);
    check(absf(p.b - 196) < 0.01f && hudScene.focus >= 0 &&
              hudScene.nodes[hudScene.focus].object->id == 8954898098276390340ULL,
          "At2s the original portrait rests and Continue receives its authored initial focus");
    Rect button = focusRect(hudScene, hudScene.nodes[hudScene.focus]);
    Vec c = center(button);
    check(pointerHit(c.x, c.y) == hudScene.focus,
          "Pointer hit testing and focus follow the translated completion controls");
    captureCase("test-output/stage-clear-settled-200.bmp");
    navigate(worldScenes[3]);
    page.minimized = true;
    page.minimizePhase = 1;
    transitionTime = 0;
    for (int i = 0; i < 360; i++)
        tickInterface(1 / 120.0f);
    captureCase("test-output/selector-water-detail-current.bmp");
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
}
