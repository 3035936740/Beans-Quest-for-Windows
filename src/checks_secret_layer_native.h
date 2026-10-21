#pragma once

// Regression coverage for secret layer native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Source: SecretRevealerController::OnUpdate 0x2f160c, speed4 at0x2f1648.
void secretLayerNativeChecks() {
    char oldSaveRoot[MAX_PATH];
    lstrcpyA(oldSaveRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/secret-layer-saves");
    CreateDirectoryA(saveRoot, 0);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    startStage(stageById(1897451970646878057ULL));
    readyTimer = 0;
    int secret = -1, coin = -1, count = 0;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == SECRET) {
            count++;
            entities[i].state.aux = 0;
            if (secret < 0)
                secret = i;
        }
        if (coin < 0 && entities[i].kind == COIN)
            coin = i;
    }
    check(
        secret >= 0 && coin >= 0 && count == 4,
        "The original four secret sensors and a textured actor are present in the real Secret Passage stage");
    if (secret >= 0 && coin >= 0) {
        Entity &reveal = entities[secret];
        State &bean = entities[playerIndex].state;
        Rect area = entityRect(reveal);
        bean.x = (area.l + area.r) * .5f;
        bean.y = (area.b + area.t) * .5f;
        bean.vx = bean.vy = 0;
        restorePhysics();
        tick(.1f, false, false);
        check(
            absf(reveal.state.aux - .4f) < .00001f &&
                absf(secretLayerAlpha(reveal.layer) - .6f) < .00001f,
            "An actual secret entrance fades its foreground by0.4 within0.1seconds at the original4-per-second rate");
        bean.x = area.l - 100;
        bean.y = area.b - 100;
        bean.vx = bean.vy = 0;
        restorePhysics();
        tick(.05f, false, false);
        check(absf(reveal.state.aux - .2f) < .00001f,
              "Leaving midway reverses the actual secret fade continuously at the native rate");
        bean.x = (area.l + area.r) * .5f;
        bean.y = (area.b + area.t) * .5f;
        bean.vx = bean.vy = 0;
        restorePhysics();
        tick(.25f, false, false);
        bool hidden = reveal.state.aux == 1 && secretLayerAlpha(reveal.layer) == 0;
        bean.x = area.l - 100;
        bean.y = area.b - 100;
        bean.vx = bean.vy = 0;
        restorePhysics();
        tick(.25f, false, false);
        check(
            hidden && reveal.state.aux == 0 && secretLayerAlpha(reveal.layer) == 1,
            "The native quarter-second secret fade reaches fully hidden and fully restored endpoints");
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == SECRET)
                entities[i].state.aux = 0;
        reveal.state.aux = .4f;
        Tile *tile = 0;
        for (int i = 0; i < current.tileCount; i++)
            if (current.tiles[i].layer == reveal.layer) {
                tile = current.tiles + i;
                break;
            }
        bool tileAlpha = false;
        if (tile) {
            cloudRenderBegin();
            drawTile(*tile);
            GLfloat color[4];
            glGetFloatv(GL_CURRENT_COLOR, color);
            tileAlpha = absf(color[3] - .6f) < .00001f;
        }
        check(
            tile && tileAlpha && secretLayerAlpha(entities[playerIndex].layer) == 1 &&
                secretLayerAlpha(reveal.layer, &current) == 1,
            "Real secret tiles receive the same layer alpha while unrelated layers and separately supplied UI scenes remain opaque");
        Entity sprite = entities[coin];
        sprite.kind = DECOR;
        sprite.layer = reveal.layer;
        sprite.state.x = 240;
        sprite.state.y = 192;
        sprite.state.timer = 0;
        Rect crop = entityRect(sprite);
        cloudRenderBegin();
        drawEntity(sprite);
        u32 actual = cloudRenderPixels(crop, false), pixels = cloudRenderPixels(crop, true);
        cloudRenderBegin();
        drawActor(sprite.art, crop, sprite.source->props.get("default-animation"), 0,
                  sprite.state.angle, false, -1, .6f);
        u32 expected = cloudRenderPixels(crop, false);
        check(
            pixels > 0 && actual == expected,
            "Actual drawEntity OpenGL pixels on a secret layer match the original actor rendered at the layer's0.6 opacity");
        reveal.state.aux = 1;
        cloudRenderBegin();
        drawEntity(sprite);
        check(cloudRenderPixels(crop, true) == 0,
              "A fully revealed foreground hides its sprites as well as its tiles");
        char trace[256];
        wsprintfA(
            trace,
            "scene=1897451970646878057 sensors=%d layer=%d tile-alpha=%d sprite-oracle-match=%d visible-pixels=%u hidden-alpha=%d\r\n",
            count, reveal.layer, tileAlpha, actual == expected, pixels,
            (int)secretLayerAlpha(reveal.layer));
        atomicWrite("test-output/secret-layer-native.txt", trace, lstrlenA(trace));
    }
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldSaveRoot);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
}
