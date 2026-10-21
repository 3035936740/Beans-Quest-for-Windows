#pragma once

// Regression coverage for helper projectile scope native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int prepareCreditsProjectileNative(u64 klass);

void helperProjectileUiScopeNativeChecks() {
    const u64 ids[] = {12978618438667112518ULL, 6430600862785566ULL, 537747635566ULL};
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    startStage(0);
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    int authored = 0, cases = 0;
    bool contacts = true, audio = true, launches = true;
    char rows[14000] =
        "class_id,fps,direction,launch,expected_stop,actual_stop,bird_voices,begin,pre,disabled,solved,bullet_live\r\n";
    for (int c = 0; c < 3; c++) {
        int owner = prepareCreditsProjectileNative(ids[c]);
        if (owner < 0)
            continue;
        authored++;
        for (int rate = 0; rate < 3; rate++)
            for (int direction = 0; direction < 4; direction++)
                for (int launch = 0; launch < 2; launch++) {
                    owner = prepareCreditsProjectileNative(ids[c]);
                    int bird = -1;
                    for (int i = 0; i < page.playfieldCount; i++)
                        if (page.playfield[i].kind == HELPER)
                            bird = i;
                    if (owner < 0 || bird < 0) {
                        contacts = false;
                        continue;
                    }
                    page.playfield[owner].state.gone = 1;
                    if (page.bodies[owner])
                        page.bodies[owner]->SetActive(false);
                    Entity &helper = page.playfield[bird];
                    helper.state.gone = 0;
                    helper.state.active = 1;
                    helper.state.timer = .2f;
                    helper.state.vx = helper.state.vy = 0;
                    page.bodies[bird]->SetActive(true);
                    CreditsShotNative &shot = creditsRuntime.shots[0];
                    shot.first = launch != 0;
                    shot.body->SetLinearVelocity(b2Vec2(0, 0));
                    b2Fixture *main = shot.body->GetFixtureList();
                    b2AABB a;
                    main->GetShape()->ComputeAABB(&a, shot.body->GetTransform(), 0);
                    float x = (a.lowerBound.x + a.upperBound.x) * .5f / metres,
                          y = (a.lowerBound.y + a.upperBound.y) * .5f / metres;
                    if (direction == 0)
                        x = a.upperBound.x / metres - helper.hit.l - .25f;
                    if (direction == 1)
                        x = a.lowerBound.x / metres - helper.hit.r + .25f;
                    if (direction == 2)
                        y = a.lowerBound.y / metres - helper.hit.t + .25f;
                    if (direction == 3)
                        y = a.upperBound.y / metres - helper.hit.b - .25f;
                    helper.state.x = x;
                    helper.state.y = y;
                    page.bodies[bird]->SetTransform(b2Vec2(x * metres, y * metres), 0);
                    page.bodies[bird]->SetLinearVelocity(b2Vec2(0, 0));
                    delete page.contacts;
                    HelperCollisionScopeUiObserverNative *probe =
                        new HelperCollisionScopeUiObserverNative(page, bird, -1);
                    probe->targetBody = shot.body;
                    page.contacts = probe;
                    page.physics->SetContactListener(probe);
                    int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240;
                    bool expected = !launch;
                    stopSounds();
                    soundEnabled = true;
                    page.physics->Step(1.0f / fps, 10, 10);
                    creditsProjectilePostStepNative(page, 1.0f / fps);
                    creditsHelperPostStep(page, 1.0f / fps);
                    bool stopped = helper.state.active == 2;
                    int voices = helperCollisionScopeBirdVoicesNative();
                    contacts &= stopped == expected;
                    audio &= voices == (expected ? 1 : 0);
                    if (launch)
                        launches &= probe->begin > 0 && probe->disabled > 0 && probe->solved == 0 &&
                                    shot.live;
                    creditsHelperPostStep(page, 0);
                    audio &= voices == helperCollisionScopeBirdVoicesNative();
                    soundEnabled = false;
                    char row[260];
                    wsprintfA(row, "%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", ids[c], fps,
                              direction, launch, expected, stopped, voices, probe->begin,
                              probe->pre, probe->disabled, probe->solved, shot.live);
                    lstrcatA(rows, row);
                    cases++;
                    stopSounds();
                }
    }
    atomicWrite("test-output/helper-projectile-ui-scope.csv", rows, lstrlenA(rows));
    check(
        authored > 0 && cases == authored * 24 && contacts && launches,
        "Every projectile class actually authored in the ending uses its real shot pool for helper launch-overlap and later-impact controls at30/120/240Hz");
    check(
        audio,
        "Actual ending projectile retirement keeps one accepted helper impact and cannot invent sounds on ignored launch contacts");
    check(
        campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "Independent ending projectile/helper collision controls cannot change campaign actors or checkpoint bytes");
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(0);
}

void helperProjectileScopeNativeChecks() {
    const u64 ids[] = {12978618438667112518ULL, 6430600862785566ULL, 537747635566ULL};
    bool clips = true;
    char rows[16000] =
        "class_id,fps,direction,launch,authored_filter,expected_stop,actual_stop,bird_voices,begin,pre,disabled,solved,bullet_alive\r\n";
    for (int c = 0; c < 3; c++) {
        bool contact = true, audio = true, launchControl = true;
        int cases = 0;
        for (int rate = 0; rate < 3; rate++)
            for (int direction = 0; direction < 4; direction++)
                for (int launch = 0; launch < 2; launch++) {
                    int bird = helperAnimationCampaignPrepareNative(641851699177ULL);
                    if (bird < 0) {
                        contact = false;
                        continue;
                    }
                    int klass = -1;
                    for (int i = 0; i < classCount; i++)
                        if (classes[i].id == ids[c])
                            klass = i;
                    if (klass < 0) {
                        clips = false;
                        continue;
                    }
                    Entity &helper = entities[bird];
                    helper.state.active = 1;
                    helper.state.timer = .2f;
                    helper.state.vx = helper.state.vy = 0;
                    entities[playerIndex].state.x = 1500;
                    entities[playerIndex].state.y = 1500;
                    rebuildPhysics();
                    bodies[playerIndex]->SetType(b2_staticBody);
                    for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f;
                         f = f->GetNext()) {
                        b2Filter filter = f->GetFilterData();
                        filter.maskBits = 0;
                        f->SetFilterData(filter);
                    }
                    Bullet &bullet = bullets[0];
                    memset(&bullet, 0, sizeof(bullet));
                    bullet.klass = klass;
                    bullet.life = 1;
                    bullet.state.active = playerIndex + 1;
                    bullet.state.timer = 1;
                    bullet.state.x = 500;
                    bullet.state.y = 400;
                    ensureShotBodyClass(0);
                    resetProjectileLaunchContactsNative(0, playerIndex);
                    shotContacts[0].first = launch != 0;
                    shotContacts[0].lastTime = 1;
                    b2Fixture *main = shotBodies[0]->GetFixtureList();
                    b2AABB a;
                    main->GetShape()->ComputeAABB(&a, shotBodies[0]->GetTransform(), 0);
                    float x = (a.lowerBound.x + a.upperBound.x) * .5f / metres,
                          y = (a.lowerBound.y + a.upperBound.y) * .5f / metres;
                    if (direction == 0)
                        x = a.upperBound.x / metres - helper.hit.l - .25f;
                    if (direction == 1)
                        x = a.lowerBound.x / metres - helper.hit.r + .25f;
                    if (direction == 2)
                        y = a.lowerBound.y / metres - helper.hit.t + .25f;
                    if (direction == 3)
                        y = a.upperBound.y / metres - helper.hit.b - .25f;
                    helper.state.x = x;
                    helper.state.y = y;
                    bodies[bird]->SetTransform(b2Vec2(x * metres, y * metres), 0);
                    bodies[bird]->SetLinearVelocity(b2Vec2(0, 0));
                    const Props *p = classes[klass].behavior("CirclePhysicsBehaviour");
                    bool authoredFilter = !p || p->flag("collide-other", true),
                         expected = authoredFilter && !launch;
                    int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240;
                    HelperCollisionScopeObserverNative observer(bird, -1);
                    observer.targetBody = shotBodies[0];
                    physicsWorld->SetContactListener(&observer);
                    stopSounds();
                    soundEnabled = true;
                    simulateBodies(1.0f / fps);
                    projectilePostStep();
                    helperPostStep(1.0f / fps);
                    int voices = helperCollisionScopeBirdVoicesNative();
                    bool stopped = helper.state.active == 2;
                    contact &= stopped == expected;
                    audio &= voices == (expected ? 1 : 0);
                    if (launch && authoredFilter)
                        launchControl &= observer.begin > 0 && observer.disabled > 0 &&
                                         observer.solved == 0 && shotIgnored(0, bird) &&
                                         bullet.life > 0;
                    helperPostStep(0);
                    audio &= helperCollisionScopeBirdVoicesNative() == voices;
                    soundEnabled = false;
                    char row[260];
                    wsprintfA(row, "%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", ids[c], fps,
                              direction, launch, authoredFilter, expected, stopped, voices,
                              observer.begin, observer.pre, observer.disabled, observer.solved,
                              bullet.life > 0);
                    lstrcatA(rows, row);
                    cases++;
                    physicsWorld->SetContactListener(originalContacts);
                    stopSounds();
                }
        char label[250];
        wsprintfA(
            label,
            "Actual runtime projectile%I64u preserves original helper filter and launch-overlap rules across%dreal shot-body contacts",
            ids[c], cases);
        check(contact && launchControl && cases == 24, label);
        wsprintfA(
            label,
            "Actual runtime projectile%I64u retains exactly one accepted helper impact even after the projectile retires and never sounds on pass-through",
            ids[c]);
        check(audio && cases == 24, label);
    }
    atomicWrite("test-output/helper-projectile-scope.csv", rows, lstrlenA(rows));
    check(
        clips,
        "All three original projectile classes run through real campaign shot bodies and independent launch/late-contact controls");
    soundEnabled = musicEnabled = false;
    startStage(0);
}
