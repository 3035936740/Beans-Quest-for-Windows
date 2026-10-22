#pragma once

// Render real custom-course emitters into a bounded GL viewport while retaining each window's
// logical aspect ratio. Visibility checks catch offscreen side emitters that still have live slots.
void makerWeatherCheckViewport(int width, int height) {
    resizeNativeViewport(width, height);
    screenScale = minf(512 / viewW, 384 / viewH);
    viewportW = (int)(viewW * screenScale);
    viewportH = (int)(viewH * screenScale);
    nativeViewportProjection();
}

void makerWeatherCheckClear() {
    glReadBuffer(GL_BACK);
    glDisable(GL_SCISSOR_TEST);
    nativeViewportProjection();
    glClearColor(.04f, .06f, .09f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int makerWeatherCheckPixels() {
    static u8 pixels[512 * 384 * 4];
    glReadPixels(0, 0, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int changed = 0;
    for (int i = 0; i < viewportW * viewportH; i++)
        changed += pixels[i * 4] != 10 || pixels[i * 4 + 1] != 15 || pixels[i * 4 + 2] != 23;
    return changed;
}

Object *makerWeatherCheckSource(Level &scene) {
    for (int i = 0; i < scene.objectCount; i++)
        if (scene.objects[i].props.flag("maker-weather"))
            return scene.objects + i;
    return 0;
}

void makerWeatherChecks() {
    int oldW = clientW, oldH = clientH;
    interfaceEnabled = musicEnabled = soundEnabled = false;
    makerBuildCatalog();
    edOpen(MAKER_SLOTS - 1, true, 2);
    const int sizes[][2] = {{1024, 768}, {1920, 560}, {560, 960}};
    bool presets = true, preview = true, resized = true;
    for (int size = 0; size < 3; size++) {
        makerWeatherCheckViewport(sizes[size][0], sizes[size][1]);
        for (int effect = 0; effect < makerEffectCount; effect++) {
            makerEd.level.head.effect = effect;
            makerEd.level.head.weatherAmount = 0;
            makerStartRun(makerEd.level, MAKER_SLOTS - 1, true, false, Vec{0, 0});
            readyTimer = transitionTime = 0;
            Object *source = makerWeatherCheckSource(current);
            int entity = -1;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].source == source)
                    entity = i;
            presets &= effect ? source && source->klass == makerEffects[effect].klass && entity >= 0
                              : !source;
            int visible = 0;
            for (int frame = 1; frame <= 240; frame++) {
                float time = frame / 60.0f;
                nativeContinuousAdvanceScene(current, 1, time, 1 / 60.0f);
                if (frame % 30)
                    continue;
                makerWeatherCheckClear();
                if (entity >= 0) {
                    entities[entity].state.timer = time;
                    glPushMatrix();
                    glTranslatef(-camX, -camY, 0);
                    drawEntity(entities[entity]);
                    glPopMatrix();
                }
                visible = (int)maxf((float)visible, (float)makerWeatherCheckPixels());
            }
            char label[160];
            wsprintfA(label, "%s produces %s at %dx%d through actual course particle rendering",
                      makerEffects[effect].name, effect ? "visible weather pixels" : "no added weather",
                      sizes[size][0], sizes[size][1]);
            check(effect ? visible > 0 : visible == 0, label);
            if (source) {
                float priorW = viewW, priorH = viewH;
                viewW += 137;
                viewH += 43;
                nativeContinuousAdvanceScene(current, 1, 4.01f, .01f);
                Rect expected = makerWeatherEmitterRect(*source, viewW, viewH);
                resized &= source->rect.r == expected.r && source->rect.t == expected.t;
                viewW = priorW;
                viewH = priorH;
            }

            makerRun = MakerRunState{};
            makerPlayLevel = 0;
            makerEd.toolbarH = 22;
            makerEd.paletteH = 60;
            makerEd.settings = true;
            makerEd.backgroundDirty = true;
            edTick(1 / 60.0f);
            Object *editorSource = makerWeatherCheckSource(makerEd.background);
            bool samePreset = effect ? editorSource && source &&
                                    eq(editorSource->props.get("particles-per-second"),
                                       source->props.get("particles-per-second"))
                              : !editorSource;
            preview &= samePreset;
            wsprintfA(label, "%s editor/course preset and rate agree at %dx%d (runtime %d, preview %d)",
                      makerEffects[effect].name, sizes[size][0], sizes[size][1],
                      source ? (int)(source->props.num("particles-per-second") * 100) : -1,
                      editorSource ? (int)(editorSource->props.num("particles-per-second") * 100) : -1);
            check(samePreset, label);
            int previewPixels = 0;
            for (int frame = 1; frame <= 240; frame++) {
                makerEd.weatherAge = frame / 60.0f;
                if (editorSource) {
                    editorSource->rect = makerWeatherEmitterRect(
                        *editorSource, viewW, edCanvasTop() - edCanvasBottom());
                    Vec c = center(editorSource->rect);
                    Rect bounds = shifted(nativeParticleEmitterBounds(editorSource->rect, 1), -c.x, -c.y);
                    nativeContinuousFor(*editorSource, bounds, 1, makerEd.weatherAge);
                }
                if (frame % 30)
                    continue;
                makerWeatherCheckClear();
                edDrawWeather();
                previewPixels = (int)maxf((float)previewPixels, (float)makerWeatherCheckPixels());
            }
            wsprintfA(label, "%s has %s in the editor preview at %dx%d",
                      makerEffects[effect].name, effect ? "visible weather pixels" : "no added weather",
                      sizes[size][0], sizes[size][1]);
            check(effect ? previewPixels > 0 : previewPixels == 0, label);
            if (size == 0 && effect) {
                makerWeatherCheckClear();
                edDrawWeather();
                SwapBuffers(gameDC);
                char path[96];
                wsprintfA(path, "test-output/maker-weather-%d.bmp", effect);
                capture(path);
            }
        }
    }
    check(presets && preview, "All six weather choices share one preset/density bake path in editor and course play");
    check(resized, "Every custom weather emitter follows viewport changes during normal scene updates");

    bool density = true, dirty = true, isolated = true;
    for (int effect = 1; effect < makerEffectCount; effect++) {
        makerEd.level.head.effect = effect;
        float previous = 0;
        const int amounts[] = {25, 100, 400};
        for (int amount = 0; amount < 3; amount++) {
            makerEd.level.head.weatherAmount = amounts[amount];
            edRefreshBackground();
            Object *source = makerWeatherCheckSource(makerEd.background);
            float rate = source ? source->props.num("particles-per-second") : 0;
            density &= rate > previous;
            previous = rate;
        }
        makerEd.backgroundDirty = false;
        edCommand(CMD_SET_EFFECT, 0);
        dirty &= makerEd.backgroundDirty;
        makerEd.backgroundDirty = false;
        edCommand(CMD_SET_WEATHER_AMOUNT, 0);
        dirty &= makerEd.backgroundDirty;
        isolated &= !makerClass(makerEffects[effect].klass)->props.flag("maker-weather");
    }
    check(density, "Weather amounts of 25/100/400 percent increase emission for every preset");
    check(dirty, "Weather selection and strength commands invalidate and refresh editor weather");
    check(isolated, "Custom weather adaptation never changes the original campaign emitter classes");
    check(glGetError() == GL_NO_ERROR, "Weather preview and course rendering complete without GL errors");
    makerEd.settings = false;
    makerRun = MakerRunState{};
    makerPlayLevel = 0;
    resizeNativeViewport(oldW, oldH);
}
