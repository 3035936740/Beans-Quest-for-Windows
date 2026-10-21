#pragma once

// Regression coverage for ending transformation. Runs only in the isolated BQ_ENABLE_TESTS executable.
void endingWalkIntegrationChecks();
bool endingPotionFixtureCorrect, endingPotionCornerClear;

float endingLowestRenderedPixel(Entity &e) {
    nativeViewportProjection();
    glDisable(GL_SCISSOR_TEST);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(viewW * 0.5f - e.state.x, 0, 0);
    drawEntity(e);
    glPopMatrix();
    int x = (int)((viewW * 0.5f - 24) * screenScale), y = (int)(64 * screenScale),
        w = (int)(48 * screenScale), h = (int)(104 * screenScale);
    u32 n = w * h * 4;
    u8 *bytes = (u8 *)HeapAlloc(GetProcessHeap(), 0, n);
    if (!bytes)
        return -10000;
    glReadBuffer(GL_BACK);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    int first = h;
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++) {
            u8 *p = bytes + (row * w + col) * 4;
            if (p[0] != 255 || p[1] != 0 || p[2] != 255) {
                if (row < first)
                    first = row;
                break;
            }
        }
    HeapFree(GetProcessHeap(), 0, bytes);
    return first < h ? (y + first + 0.5f) / screenScale : -10000;
}

void endingTransformationChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(stageById(976871008727ULL));
    readyTimer = deathTimer = 0;
    Object *maria = 0;
    Object *emilio = 0;
    Object *controller = 0;
    for (int i = 0; i < current.objectCount; i++) {
        Object &o = current.objects[i];
        if (o.id == 8846813228102110ULL)
            maria = &o;
        if (o.id == 1928996116911671ULL)
            emilio = &o;
        if (o.id == 8687262745152374ULL)
            controller = &o;
    }
    check(
        maria && emilio && controller && maria->layer == emilio->layer && maria->layer == 6,
        "Final Wizard stage contains its original Emilio, Maria and transformation controller on the same layer");
    if (!maria || !emilio || !controller) {
        startStage(0);
        return;
    }
    const Props *config = controller->behavior("EndOfGameCutsceneController");
    check(
        config && idnum(config->get("emilio")) == emilio->id &&
            idnum(config->get("maria")) == maria->id &&
            idnum(config->get("next-level")) == 137502285691ULL,
        "Original ending controller binds both real character instances and sends the later transition to the scrolling credits scene");
    EndingTransformationInput in = {(maria->rect.l + maria->rect.r) * 0.5f,
                                    (maria->rect.b + maria->rect.t) * 0.5f,
                                    (emilio->rect.b + emilio->rect.t) * 0.5f, 6900, 6910};
    EndingTransformationPose p = endingTransformationPose(in, 0.2f);
    check(
        absf(p.emilioX - 6922) < 0.001f && p.emilioY == 120 && p.mariaX == 6688 &&
            p.mariaY == 112 && eq(p.emilioAnimation, "walking") && eq(p.mariaAnimation, ""),
        "Heart-switch rescue moves Emilio horizontally in0.4seconds while preserving his original120Y and Maria112Y so differently sized sprites share a floor");
    p = endingTransformationPose(in, 1);
    check(
        p.emilioX == 6944 && p.mariaX == 6688 && p.cameraX == 6816 && eq(p.emilioAnimation, ""),
        "Ending camera reaches Maria plus128 after1second while Emilio waits at the first endpoint");
    p = endingTransformationPose(in, 5);
    check(p.mariaX == 6747 && p.emilioX == 6885 && p.mariaScaleX == 1 &&
              eq(p.mariaAnimation, "walking") && eq(p.emilioAnimation, "walking"),
          "Original rescue walks both characters toward each other only during seconds4through6");
    p = endingTransformationPose(in, 6.01f);
    check(
        p.mariaX == 6806 && p.emilioX == 6826 && eq(p.mariaAnimation, "") &&
            eq(p.emilioAnimation, "") && absf(p.emilioWalkAdvance - 2.4f) < 0.0001f,
        "Rescue finishes with a20pixel separation and Emilio resumes the retained walking instance rather than restarting it");
    check(
        endingTransformationPose(in, 2).mariaScaleX == -1 &&
            endingTransformationPose(in, 2.001f).mariaScaleX == 1 &&
            eq(endingTransformationPose(in, 4).mariaAnimation, "") &&
            eq(endingTransformationPose(in, 6).mariaAnimation, "walking"),
        "Native strict animation and facing boundaries preserve the exact2,4and6second endpoint states");
    check(
        !(endingTransformationEvents(0.3f, 0.4f) & ENDING_EMILIO_IDLE) &&
            (endingTransformationEvents(0.4f, 0.401f) & ENDING_EMILIO_IDLE) &&
            !(endingTransformationEvents(7, 7.1f) & ENDING_BOSS_ACHIEVEMENT) &&
            (endingTransformationEvents(6.9f, 7) & ENDING_BOSS_ACHIEVEMENT) &&
            (endingTransformationEvents(7.9f, 8) & ENDING_CREDITS) &&
            !(endingTransformationEvents(8, 8.1f) & ENDING_CREDITS),
        "Ending animation switches cross strictly after their thresholds while achievements and the8second credits request fire once on reaching theirs");
    Actor *ma = actor(maria->klass);
    Actor *em = actor(emilio->klass);
    Animation *mw = animation(ma, "walking");
    Animation *ew = animation(em, "walking");
    check(mw && ew && eq(mw->name, "walking") && eq(ew->name, "walking") && mw->count == 4 &&
              ew->count == 8 && absf(animationLength(mw) - 0.8f) < 0.0001f &&
              absf(animationLength(ew) - 1.6f) < 0.0001f,
          "Rescue uses original Maria4frame0.8second and Emilio8frame1.6second walking resources");
    Rect mr = {220, 160, 252, 192}, er = {276, 160, 308, 208};
    cloudRenderBegin();
    drawActor(ma, mr, "", 0);
    drawActor(em, er, "", 0, 0, true);
    u32 idle = cloudRenderPixels(Rect{220, 160, 308, 208}, false);
    cloudRenderBegin();
    drawActor(ma, mr, "walking", 0.3f);
    drawActor(em, er, "walking", 0.3f, 0, true);
    u32 walking = cloudRenderPixels(Rect{220, 160, 308, 208}, false);
    check(
        idle != walking && cloudRenderPixels(Rect{220, 160, 308, 208}, true) > 100,
        "Actual OpenGL renders the supplied prince and princess sprites and their distinct walking frames");
    SwapBuffers(gameDC);
    capture("test-output/ending-original-characters-walking.bmp");
    endingWalkIntegrationChecks();
    interfaceEnabled = false;
    startStage(0);
}

int endingTestCollectPotion() {
    startStage(stageById(976871008727ULL));
    readyTimer = deathTimer = transitionTime = 0;
    int potion = -1;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind == POTION)
            potion = i;
        if (e.kind == BOSS || e.bossSpawn)
            e.state.gone = 1;
    }
    wizardMotion.initialized = 0;
    memset(bullets, 0, sizeof(bullets));
    endingPotionFixtureCorrect = endingPotionCornerClear = false;
    if (potion >= 0) {
        Entity &bean = entities[playerIndex];
        Entity &bottle = entities[potion];
        b2Body *body = bodies[potion];
        if (body) {
            b2Fixture *f = body->GetFixtureList();
            endingPotionFixtureCorrect = body->GetType() == b2_kinematicBody && f &&
                                         f->IsSensor() && f->GetFilterData().categoryBits == 4 &&
                                         f->GetShape()->GetType() == b2Shape::e_circle;
            if (endingPotionFixtureCorrect)
                endingPotionFixtureCorrect =
                    absf(((const b2CircleShape *)f->GetShape())->m_radius / metres - 16) < 0.001f;
        }
        bean.state.x = bottle.state.x + 23;
        bean.state.y = bottle.state.y + 29;
        bean.state.vx = bean.state.vy = 0;
        restorePhysics();
        tick(1 / 120.0f, false, false);
        endingPotionCornerClear = !bottle.state.gone && !playerWalkMode && !endingScene.active;
        bean.state.x = bottle.state.x;
        bean.state.y = bottle.state.y;
        bean.state.vx = 0;
        bean.state.vy = 100;
        restorePhysics();
        tick(1 / 120.0f, false, false);
    }
    return potion;
}

unsigned short endingTestBeanCategory() {
    if (playerIndex < 0 || !bodies[playerIndex])
        return 0;
    for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext())
        if (!f->IsSensor())
            return f->GetFilterData().categoryBits;
    return 0;
}

void endingWalkIntegrationChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = true;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    memset(keys, 0, sizeof(keys));
    int potion = endingTestCollectPotion();
    check(potion >= 0,
          "Original final-stage transformation potion is present for the playable prince route");
    if (potion < 0) {
        interfaceEnabled = false;
        return;
    }
    Entity &bean = entities[playerIndex];
    check(
        entities[potion].state.gone && playerWalkMode && !endingScene.active && screen == PLAY &&
            !complete && bodies[playerIndex]->IsActive() &&
            absf(bean.state.x - entities[potion].state.x) < 1 &&
            absf(bean.state.y - entities[potion].state.y) < 2 && bean.state.vy <= 0,
        "Real potion contact transforms Bean in place and clips upward velocity while preserving the current playable stage and body");
    check(
        endingPotionFixtureCorrect && endingPotionCornerClear,
        "Original potion is an actual kinematic radius16loose circle sensor and a visual corner overlap outside that circle does not collect it");
    check(
        endingTestBeanCategory() == 4,
        "Playable prince uses the original loose fixture category so player-only cage walls no longer stop him");
    captureCheckpoint(false);
    keyPressed('S');
    bool queued = respawnBusy(respawnTransition);
    for (int i = 0; i < 110; i++)
        tick(dt, false, false);
    check(
        queued && !respawnBusy(respawnTransition) && playerWalkMode &&
            entities[potion].state.gone && endingTestBeanCategory() == 4 && !endingScene.active &&
            bodies[playerIndex]->IsActive() && bean.layer == bean.source->layer,
        "S remains usable after drinking the potion and restores the human form, active actor layer and loose collision category without starting the rescue");
    keyPressed('R');
    queued = respawnBusy(respawnTransition);
    for (int i = 0; i < 110; i++)
        tick(dt, false, false);
    check(
        queued && !respawnBusy(respawnTransition) && !playerWalkMode &&
            !entities[potion].state.gone && endingTestBeanCategory() == 2 && !endingScene.active &&
            bodies[playerIndex]->IsActive() && bean.layer == bean.source->layer,
        "R remains usable before the rescue and cleanly restores normal Bean, the potion and player collision category");
    potion = endingTestCollectPotion();
    float beforeX = bean.state.x;
    bool moved = false, insideCage = false, noAutoJump = true, speed = true, walkFootSeen = false,
         walkFootAligned = false;
    char route[8000] = "step,x_milli,y_milli,vx_milli,vy_milli,ground,ending,death\r\n";
    for (int i = 0; i < 1100 && !endingScene.active && deathTimer <= 0; i++) {
        tick(dt, true, false);
        moved |= bean.state.x < beforeX - 80;
        speed &= absf(bean.state.vx) <= 100.1f;
        if (onGround && !endingScene.active)
            noAutoJump &= bean.state.vy <= 0.1f;
        insideCage |=
            bean.state.x >= 6976 && bean.state.x <= 7008 && bean.state.y > 96 && bean.state.y < 160;
        if (!walkFootSeen && onGround && bean.state.x > 7080 && bean.state.x < 7160) {
            float foot = endingLowestRenderedPixel(bean);
            walkFootSeen = true;
            walkFootAligned = foot >= 95.5f && foot <= 98.5f;
            captureCase("test-output/ending-playable-prince-grounded.bmp");
        }
        if (i % 60 == 0 || endingScene.active) {
            char row[150];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d\r\n", i, (int)(bean.state.x * 1000),
                      (int)(bean.state.y * 1000), (int)(bean.state.vx * 1000),
                      (int)(bean.state.vy * 1000), onGround ? 1 : 0, endingScene.active ? 1 : 0,
                      deathTimer > 0 ? 1 : 0);
            lstrcatA(route, row);
        }
    }
    atomicWrite("test-output/ending-playable-prince-route.csv", route, lstrlenA(route));
    check(
        moved && speed && noAutoJump,
        "Actual left input moves the prince at half speed and ordinary floor contact no longer launches automatic Bean jumps");
    check(
        walkFootSeen && walkFootAligned,
        "Actual OpenGL playable-prince feet remain on the original96Yfloor using the rotated atlas transparent padding and native threefold sprite height");
    check(
        insideCage && endingScene.active && (alternationBits & 1) &&
            absf(endingScene.input.beanX - 6944) < 40 && deathTimer <= 0,
        "The prince physically walks through the original cage-stop barrier and starts rescue only by contacting the separate mask1heart switch");
    if (endingScene.active) {
        RespawnTransition before = respawnTransition;
        float age = endingScene.age;
        keyPressed('S');
        keyPressed('R');
        keyPressed(VK_ESCAPE);
        keyPressed(VK_SPACE);
        SendMessageA(window, WM_KILLFOCUS, 0, 0);
        bool ignored = hashBytes(&before, sizeof(before)) ==
                           hashBytes(&respawnTransition, sizeof(respawnTransition)) &&
                       screen == PLAY && endingScene.active;
        tick(0.2f, true, false);
        check(
            ignored && endingScene.age > age &&
                absf(entities[endingScene.emilio].state.x - endingScene.input.beanX) < 40,
            "S,Randmenu input plus actual Windows focus loss cannot queue a restore or pause and deadlock the formal rescue");
        for (int i = 0; i < 200 && endingScene.active && endingScene.age < 1.1f; i++)
            tick(dt, false, false);
        Entity &earlyPrince = entities[endingScene.emilio];
        Entity &earlyPrincess = entities[endingScene.maria];
        float earlyPrinceFoot = endingLowestRenderedPixel(earlyPrince),
              earlyPrincessFoot = endingLowestRenderedPixel(earlyPrincess);
        check(
            earlyPrince.state.y == 120 && earlyPrincess.state.y == 112 && earlyPrinceFoot >= 96 &&
                earlyPrincessFoot >= 96 &&
                absf(earlyPrinceFoot - earlyPrincessFoot) <= 1 / screenScale,
            "Actual OpenGL rescue prince keeps the96Yfloor after the first horizontal tween rather than dropping8pixels to Maria centre height");
        captureCase("test-output/ending-rescue-1s.bmp");
        for (int i = 0; i < 800 && endingScene.active && endingScene.age < 6.1f; i++)
            tick(dt, false, false);
        check(
            screen == PLAY && endingScene.active &&
                absf(entities[endingScene.emilio].state.x - entities[endingScene.maria].state.x -
                     20) < 0.001f,
            "The actual switch-triggered rescue joins prince and princess before the white credits transition");
        Entity &prince = entities[endingScene.emilio];
        Entity &princess = entities[endingScene.maria];
        float princeFoot = endingLowestRenderedPixel(prince),
              princessFoot = endingLowestRenderedPixel(princess);
        check(
            prince.state.y == 120 && princess.state.y == 112 && entityRect(prince).b == 96 &&
                entityRect(princess).b == 96 && princeFoot >= 96 && princessFoot >= 96 &&
                absf(princeFoot - princessFoot) <= 1 / screenScale,
            "Actual OpenGL rescue prince and princess share the authored96Yfloor despite their48and32pixel heights without either sprite sinking below the platform");
        captureCase("test-output/ending-rescue-6s.bmp");
        for (int i = 0; i < 500 && endingScene.active && endingScene.age < 8.5f; i++)
            tick(dt, false, false);
        check(screen == PLAY && endingScene.active && endingScene.age > 8 && endingScene.age < 10,
              "Native8second change request plays its white fade before changing scenes");
        for (int i = 0; i < 250 && screen == PLAY; i++)
            tick(dt, false, false);
        check(
            screen == ENDING && !endingScene.active && page.level.id == 137502285691ULL,
            "Rescue continues through the white fade into credits after ignored S/R without leaking the formal cutscene input lock");
    }
    interfaceEnabled = false;
    memset(keys, 0, sizeof(keys));
}
