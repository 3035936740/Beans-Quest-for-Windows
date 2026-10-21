#pragma once

// Regression coverage for bullet source animation native. Runs only in the isolated BQ_ENABLE_TESTS executable.

u32 bulletSourceSpriteHash(Entity &source) {
    int size = (int)(40 * screenScale);
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, size * size * 4);
    if (!pixels)
        return 0;
    camX = source.state.x - viewW * 0.5f;
    camY = source.state.y - viewH * 0.5f;
    nativeViewportProjection();
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(-camX, -camY, 0);
    drawEntity(source);
    glPopMatrix();
    glReadBuffer(GL_BACK);
    glReadPixels(viewportW / 2 - size / 2, viewportH / 2 - size / 2, size, size, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
    u32 h = hashBytes(pixels, size * size * 4);
    HeapFree(GetProcessHeap(), 0, pixels);
    return h;
}

void bulletSourceAnimationNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    const float dt = 1.0f / 120.0f;
    const u64 classesToCheck[] = {12795324931407506847ULL, 303150949257ULL, 117211337098ULL};
    bool resources = true, thresholds = true, rawTime = true, retainedIdle = true;
    for (int i = 0; i < 3; i++) {
        Actor *art = actor(classesToCheck[i]);
        Animation *idle = bulletSourceExactAnimation(art, "idle-shoot"),
                  *pre = bulletSourceExactAnimation(art, "pre-shoot"),
                  *post = bulletSourceExactAnimation(art, "post-shoot"),
                  *initial = bulletSourceExactAnimation(art, "");
        resources &=
            idle && pre && !post && idle->count == 1 && pre->count == (i ? 4 : 8) &&
            absf(animationLength(pre) - 0.4f) < 0.00001f && initial &&
            initial->frames[0].texture == idle->frames[0].texture &&
            memcmp(initial->frames[0].uv, idle->frames[0].uv, sizeof(idle->frames[0].uv)) == 0;
        if (!pre)
            continue;
        BulletSourceAnimationNativeState visual = {};
        float interval = 1.5f, boundary = interval - animationLength(pre);
        nativeBulletSourceAnimationBefore(visual, art, boundary - 0.001f, interval, dt);
        thresholds &= visual.phase == SOURCE_VISUAL_DEFAULT;
        nativeBulletSourceAnimationBefore(visual, art, boundary, interval, dt);
        thresholds &= visual.phase == SOURCE_VISUAL_PRE && absf(visual.preAge) < 0.00001f;
        nativeBulletSourceAnimationBefore(visual, art, boundary + 0.21f, interval, dt);
        thresholds &= absf(visual.preAge - 0.21f) < 0.00001f &&
                      animationFrame(pre, visual.preAge, ANIMATION_STOP) == (i ? 2 : 4);
        nativeBulletSourceAnimationBefore(visual, art, interval + 0.3f, interval, dt);
        rawTime &= absf(visual.preAge - 0.7f) < 0.00001f &&
                   animationFrame(pre, visual.preAge, ANIMATION_STOP) == pre->count - 1;
        visual.idleAge = 0.007f;
        nativeBulletSourceAnimationFired(visual, art);
        retainedIdle &= visual.phase == SOURCE_VISUAL_IDLE && visual.idleAge == 0.007f;
    }
    check(
        resources,
        "All three original turret actors have identical default/idle artwork and a real0.4second pre-shoot sequence: green8frames and left/right4frames, with no post-shoot resource");
    check(
        thresholds && rawTime,
        "Native turret warning selects exact named pre-shoot at the old-clock boundary, manually samples its correct frames and holds the final frame if firing is delayed");
    check(
        retainedIdle,
        "Each actual turret returns to its retained idle-shoot instance after Fire without inventing a missing post-shoot animation or restarting idle time");
    // Exercise the native post-shoot branch with an explicit synthetic clip,
    // since none of the three shipped actors supplies it.
    Frame frames[2] = {};
    frames[0].duration = frames[1].duration = 1;
    Animation clips[] = {{(char *)"idle-shoot", 10, 2, frames},
                         {(char *)"post-shoot", 10, 2, frames}};
    Actor synthetic = {};
    synthetic.count = 2;
    synthetic.animations = clips;
    BulletSourceAnimationNativeState v = {};
    v.initialized = 1;
    v.phase = SOURCE_VISUAL_IDLE;
    v.idleAge = 0.13f;
    nativeBulletSourceAnimationFired(v, &synthetic);
    bool post = v.phase == SOURCE_VISUAL_POST && v.postAge == 0;
    nativeBulletSourceAnimationBefore(v, &synthetic, 0, 1.5f, 0.1f);
    post &= v.phase == SOURCE_VISUAL_POST;
    nativeBulletSourceAnimationBefore(v, &synthetic, 0.1f, 1.5f, 0.1f);
    post &= v.phase == SOURCE_VISUAL_IDLE && v.idleAge == 0.13f;
    check(
        post,
        "Native optional post-shoot restarts at zero, stops at clip end and returns to the unchanged idle instance instead of looping or overwriting its age");
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int count = 0, green = 0, right = 0, left = 0, ending = 0;
    bool authored = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        for (int i = 0; i < scene.objectCount; i++) {
            Object &o = scene.objects[i];
            const Props *source = o.behavior("BulletSource");
            if (!source)
                continue;
            count++;
            green += o.klass == classesToCheck[0];
            right += o.klass == classesToCheck[1];
            left += o.klass == classesToCheck[2];
            ending += eq(scene.props.get("name"), "Cutscene - Ending");
            Actor *art = actor(o.klass);
            Animation *pre = bulletSourceExactAnimation(art, "pre-shoot");
            float interval = source->num("timeMin", 3);
            BulletSourceAnimationNativeState state = {};
            nativeBulletSourceAnimationBefore(state, art, interval - animationLength(pre) + 0.2f,
                                              interval, dt);
            bool correct = absf(source->num("timeMin") - source->num("timeMax")) < 0.00001f &&
                           state.phase == SOURCE_VISUAL_PRE && absf(state.preAge - 0.2f) < 0.00001f;
            if (!correct) {
                char why[160];
                wsprintfA(why, "class=%u phase=%d interval1000=%d preAge1000000=%d count=%d\r\n",
                          (u32)o.klass, state.phase, (int)(interval * 1000),
                          (int)(state.preAge * 1000000), pre ? pre->count : -1);
                atomicWrite("test-output/bullet-source-warning-inventory-failure.txt", why,
                            lstrlenA(why));
            }
            authored &= correct;
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    char inventoryTrace[180];
    wsprintfA(inventoryTrace, "total=%d green=%d right=%d left=%d ending=%d authored=%d\r\n", count,
              green, right, left, ending, authored);
    atomicWrite("test-output/bullet-source-warning-inventory.txt", inventoryTrace,
                lstrlenA(inventoryTrace));
    check(
        count == 35 && green == 9 && right == 15 && left == 11 && ending == 2 && authored,
        "All74source scenes preserve the warning window for all35 turret instances, including the two separate ending-scene actors");
    // Actual gameplay clock, source firing and visual phase share the same
    // active tick; the observer stays above the horizontal projectile lanes.
    startStage(stageById(739118707375ULL));
    readyTimer = 0;
    int turret = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == 3089867997079985ULL)
            turret = i;
    bool live = turret >= 0;
    int warningFrames = 0, idleFrames = 0, fires = 0;
    unsigned int seen = 0;
    char csv[3000] = "step,clock_before_milli,clock_after_milli,phase,pre_age_milli,frame\r\n";
    if (turret >= 0) {
        Entity &t = entities[turret];
        Entity &bean = entities[playerIndex];
        bean.state.x = t.state.x + 180;
        bean.state.y = t.state.y + 180;
        bean.state.vx = bean.state.vy = 0;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        for (int step = 0; step < 480; step++) {
            float before = bulletSourceStates[turret].clock;
            tick(dt, false, false);
            BulletSourceAnimationNativeState &state = bulletSourceAnimations[turret];
            if (state.phase == SOURCE_VISUAL_PRE) {
                warningFrames++;
                int frame = animationFrame(bulletSourceExactAnimation(t.art, "pre-shoot"),
                                           state.preAge, ANIMATION_STOP);
                seen |= 1u << frame;
                if (step % 12 == 0) {
                    char row[140];
                    wsprintfA(row, "%d,%d,%d,%d,%d,%d\r\n", step, (int)(before * 1000),
                              (int)(bulletSourceStates[turret].clock * 1000), state.phase,
                              (int)(state.preAge * 1000), frame);
                    lstrcatA(csv, row);
                }
            }
            if (state.phase == SOURCE_VISUAL_IDLE)
                idleFrames++;
            if (bulletSourceStates[turret].clock < before) {
                fires++;
                live &= state.phase == SOURCE_VISUAL_IDLE;
            }
        }
    }
    atomicWrite("test-output/bullet-source-warning-native.csv", csv, lstrlenA(csv));
    check(
        live && warningFrames > 100 && idleFrames > 100 && fires >= 3 && seen == 15,
        "Actual turret-intro physics repeatedly advances all four warning frames before at least three live Fire events and visibly returns to idle");
    if (turret >= 0) {
        BulletSourceAnimationNativeState saved = bulletSourceAnimations[turret];
        captureCheckpoint(false);
        bulletSourceAnimations[turret].phase = SOURCE_VISUAL_DEFAULT;
        bulletSourceAnimations[turret].idleAge = 99;
        restoreCheckpoint();
        check(
            saved.phase == bulletSourceAnimations[turret].phase &&
                saved.idleAge == bulletSourceAnimations[turret].idleAge &&
                saved.preAge == bulletSourceAnimations[turret].preAge &&
                saved.postAge == bulletSourceAnimations[turret].postAge,
            "In-process checkpoint restoration keeps exact cached turret warning phase and retained animation ages from the captured current-format state");
    }
    bool pixels = true;
    const u64 scenes[] = {806701038719ULL, 707197359596ULL, 789619714186ULL};
    for (int i = 0; i < 3; i++) {
        startStage(stageById(scenes[i]));
        readyTimer = 0;
        int target = -1;
        for (int j = 0; j < entityCount; j++)
            if (entities[j].source->klass == classesToCheck[i]) {
                target = j;
                break;
            }
        if (target < 0) {
            pixels = false;
            continue;
        }
        Entity &t = entities[target];
        resetBulletSourceAnimation(target);
        u32 idle = bulletSourceSpriteHash(t);
        Animation *pre = bulletSourceExactAnimation(t.art, "pre-shoot");
        bool changed = false;
        u32 warning = 0;
        for (int f = 0; pre && f < pre->count; f++) {
            nativeBulletSourceAnimationBefore(bulletSourceAnimations[target], t.art,
                                              (float)f / pre->duration + 0.001f,
                                              animationLength(pre), dt);
            warning = bulletSourceSpriteHash(t);
            changed |= idle != warning;
        }
        char trace[192], path[96];
        wsprintfA(
            trace,
            "case=%d target=%d idle=%u warning=%u changed=%d x100=%d y100=%d visible=%d gone=%d\r\n",
            i, target, idle, warning, changed, (int)(t.state.x * 100), (int)(t.state.y * 100),
            t.source->visible, t.state.gone);
        wsprintfA(path, "test-output/bullet-source-warning-pixels-%d.txt", i);
        atomicWrite(path, trace, lstrlenA(trace));
        pixels &= idle && warning && changed;
    }
    check(
        pixels,
        "Actual OpenGL drawEntity displays different original pre-shoot artwork for green, left and right turret instances instead of rendering permanent idle");
    startStage(0);
    readyTimer = 0;
}
