#pragma once

// Regression coverage for selector slope native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorSlopeNativeChecks() {
    bool oldInterface = interfaceEnabled, oldDebug = debugMode;
    interfaceEnabled = true;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    memset(keys, 0, sizeof(keys));
    startStage(0);
    readyTimer = 0;
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    float oldIncoming = tomOrdinaryIncomingVyNative, oldSolved = tomOrdinarySolvedVxNative;
    Vec oldNormal = tomOrdinaryNormalNative;
    bool oldFound = tomOrdinaryNormalFoundNative;
    tomOrdinaryIncomingVyNative = 123;
    tomOrdinarySolvedVxNative = 456;
    tomOrdinaryNormalNative = Vec{.7f, .8f};
    tomOrdinaryNormalFoundNative = true;
    bool isolated = true;
    const u64 scenes[] = {8808768056724772154ULL, 12038123762473955738ULL, 71376317651ULL,
                          799282380888ULL, 669776786229ULL};
    char rows[12000] = {};
    lstrcpyA(rows, "scene,angle1000,vx1000,expectedVx1000,vy1000,expectedVy1000,factor1000\r\n");
    for (int w = 0; w < 5; w++) {
        bool correct = true;
        for (int direction = -1; direction <= 1; direction++) {
            navigate(scenes[w]);
            transitionTime = 0;
            page.minimized = true;
            page.minimizePhase = 1;
            page.miniDeath = 0;
            for (b2Body *b = page.physics->GetBodyList(); b;) {
                b2Body *next = b->GetNext();
                if (!b->GetUserData())
                    page.physics->DestroyBody(b);
                b = next;
            }
            for (int i = 0; i < page.playfieldCount; i++) {
                page.playfield[i].state.gone = 1;
                if (page.bodies[i])
                    page.bodies[i]->SetActive(false);
                if (page.oscillatorProxies[i])
                    page.oscillatorProxies[i]->SetActive(false);
            }
            float angle = direction * 25 * .017453293f;
            b2BodyDef definition;
            definition.position.Set(300 * metres, 100 * metres);
            definition.angle = angle;
            b2Body *floor = page.physics->CreateBody(&definition);
            b2PolygonShape shape;
            shape.SetAsBox(40 * metres, 4 * metres);
            floor->CreateFixture(&shape, 0);
            page.bean.state.x = 300;
            page.bean.state.y = 100 + 4 / bqCos(angle) - page.bean.hit.b + 1;
            page.bean.state.vx = 0;
            page.bean.state.vy = -40;
            resetUiPhysics(page);
            bool jumped = false;
            for (int f = 0; f < 40; f++) {
                float beforeVx = page.bean.state.vx;
                stepUiPhysics(page, 1.0f / 120);
                if (!page.ordinaryJumpNative)
                    continue;
                float nx = -bqSin(angle), factor = page.bean.physics->num("x-jump-factor", .2f),
                      speed = page.bean.jumpPower;
                float expectedX = beforeVx + speed * nx * factor,
                      expectedY = speed * root(1 - nx * nx * factor * factor) +
                                  page.bean.physics->num("gravity", -200) / 120;
                jumped = true;
                correct &= absf(page.bean.state.vx - expectedX) < .002f &&
                           absf(page.bean.state.vy - expectedY) < .002f;
                char row[220], scene[24];
                idstr(scenes[w], scene);
                wsprintfA(row, "%s,%d,%d,%d,%d,%d,%d\r\n", scene, direction * 25000,
                          (int)(page.bean.state.vx * 1000), (int)(expectedX * 1000),
                          (int)(page.bean.state.vy * 1000), (int)(expectedY * 1000),
                          (int)(factor * 1000));
                lstrcatA(rows, row);
                break;
            }
            correct &= jumped;
            isolated &= tomOrdinaryIncomingVyNative == 123 && tomOrdinarySolvedVxNative == 456 &&
                        tomOrdinaryNormalNative.x == .7f && tomOrdinaryNormalNative.y == .8f &&
                        tomOrdinaryNormalFoundNative;
        }
        char label[190];
        wsprintfA(
            label,
            "Selector scene%d real miniature Bean applies its authored slope-jump factor on both tilted faces and keeps flat-floor launch unchanged",
            w + 1);
        check(correct, label);
    }
    check(
        isolated && campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "Selector PreSolve and PostSolve retain their own jump-contact state without touching campaign physics or checkpoint data");
    atomicWrite("test-output/selector-slope-native.csv", rows, lstrlenA(rows));
    tomOrdinaryIncomingVyNative = oldIncoming;
    tomOrdinarySolvedVxNative = oldSolved;
    tomOrdinaryNormalNative = oldNormal;
    tomOrdinaryNormalFoundNative = oldFound;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    startStage(0);
    readyTimer = 0;
}
