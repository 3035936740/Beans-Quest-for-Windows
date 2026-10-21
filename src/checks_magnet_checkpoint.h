#pragma once

// Regression coverage for magnet checkpoint. Runs only in the isolated BQ_ENABLE_TESTS executable.

void magnetCheckpointContactChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    char *log = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16000);
    if (!log) {
        check(false, "Magnetic checkpoint contact log allocation succeeds");
        return;
    }
    const int stages[] = {11, 9, 7};
    const u64 checkpoints[] = {109602108092177ULL, 1735000787919831ULL, 878679571967ULL};
    for (int scene = 0; scene < 3; scene++) {
        int stage = motionStage(5, stages[scene]);
        check(stage >= 0, "Reported fifth-world magnetic checkpoint stage exists");
        if (stage < 0)
            continue;
        startStage(stage);
        readyTimer = 0;
        int point = -1, magnet = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == CHECKPOINT && entities[i].source->id == checkpoints[scene])
                point = i;
        check(point >= 0, "Reported authored checkpoint sensor is retained");
        if (point < 0)
            continue;
        Entity &cp = entities[point];
        float x = cp.state.x;
        for (int i = 0; i < entityCount; i++) {
            Entity &e = entities[i];
            if (e.kind == MAGNET && boxGroupMatrixOwner(e) && x > e.source->rect.l &&
                x < e.source->rect.r && absf(e.source->rect.t - cp.source->rect.b) < 1)
                magnet = i;
        }
        check(magnet >= 0, "Checkpoint stands directly on its original full-width magnetic matrix");
        if (magnet < 0)
            continue;
        // Preserve every authored terrain polygon, checkpoint and magnetic
        // fixture. Only unrelated actors capable of killing this probe stop.
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == ENEMY || entities[i].kind == TURRET ||
                entities[i].source->has("BulletSource"))
                entities[i].state.gone = 1;
        rebuildPhysics();
        Entity &beanEntity = entities[playerIndex];
        State &bean = beanEntity.state;
        float top = entities[magnet].source->rect.t;
        bean.x = x;
        bean.y = top - beanEntity.hit.b + 0.2f;
        bean.vx = 0;
        bean.vy = -10;
        restorePhysics();
        int before = jumps;
        bool touched = false, launched = false, terrainAlso = false;
        float maxVy = -10000;
        for (int tickIndex = 0; tickIndex < 120; tickIndex++) {
            tick(dt, false, false);
            touched |= playerMagnetContact;
            launched |= playerMagnetContact && bean.vy > 1;
            maxVy = maxf(maxVy, bean.vy);
            for (b2ContactEdge *edge = bodies[playerIndex]->GetContactList(); edge;
                 edge = edge->next)
                if (edge->other->GetUserData() == 0 && edge->contact->IsTouching() &&
                    edge->contact->IsEnabled() && edge->contact->GetManifold()->pointCount)
                    terrainAlso = true;
            if (tickIndex < 8) {
                char row[240];
                wsprintfA(
                    row, "5-%d tick%d x%d feet%d vy%d support%d magnet%d checkpoint%d jumps%d\r\n",
                    stages[scene], tickIndex, (int)bean.x, (int)hitRect(beanEntity).b, (int)bean.vy,
                    supportEntity, playerMagnetContact ? 1 : 0, cp.state.hp, jumps);
                lstrcatA(log, row);
            }
        }
        check(
            touched && !launched && maxVy < 1 && jumps == before && bean.active == 2,
            "Real checkpoint and coincident terrain cannot override a genuine magnetic contact and make Bean jump");
        check(
            cp.state.hp == 2 && haveCheckpoint,
            "Standing on the magnetic floor still activates and saves its real checkpoint sensor");
        if (scene == 0)
            check(
                terrainAlso,
                "Reported Blaster Climb checkpoint regression includes simultaneous original terrain and magnet contacts");
        int savedJumps = checkpoint.jumps;
        restoreCheckpoint();
        readyTimer = 0;
        for (int i = 0; i < 30; i++)
            tick(dt, false, false);
        check(
            playerMagnetContact && entities[playerIndex].state.active == 2 &&
                entities[playerIndex].state.vy < 1 && jumps == savedJumps,
            "Restoring the actual magnetic checkpoint resumes attachment without an accidental automatic jump");
        bool crossed = true;
        for (int direction = 0; direction < 2; direction++) {
            bean.x = x + (direction ? -8.0f : 8.0f);
            bean.y = top - beanEntity.hit.b + 0.2f;
            bean.vx = 0;
            bean.vy = -10;
            restorePhysics();
            int count = jumps;
            for (int i = 0; i < 30; i++)
                tick(dt, !direction, direction != 0);
            crossed &= jumps == count && playerMagnetContact && bean.active == 2 &&
                       absf(hitRect(beanEntity).b - top) < 2.5f &&
                       (direction ? bean.x > x : bean.x < x);
        }
        check(
            crossed,
            "Bean crosses the checkpoint pole on its original magnetic floor in both directions without losing attachment");
    }
    atomicWrite("test-output/magnet-checkpoint-contacts.txt", log, lstrlenA(log));
    HeapFree(GetProcessHeap(), 0, log);
}

void springIncomingContactChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    startStage(2);
    readyTimer = 0;
    int spring = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SPRING && boxGroupMatrixOwner(entities[i])) {
            spring = i;
            break;
        }
    check(spring >= 0,
          "Original spring matrix is available for incoming-velocity and box-edge checks");
    if (spring < 0)
        return;
    Object *springSource = entities[spring].source;
    current.geometryCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && entities[i].source != springSource)
            entities[i].state.gone = 1;
    const float angles[] = {0, 0, 35};
    const float incoming[] = {-1, -180, -40};
    for (int scenario = 0; scenario < 3; scenario++) {
        int boxIndex = entityCount;
        Object *box = classById(2913132245228912724ULL);
        addEntity(*box, shifted(box->rect, 0, 0));
        Entity &target = entities[boxIndex];
        Entity &pad = entities[spring];
        float left = springSource->rect.l, right = springSource->rect.r;
        target.state.x = scenario == 2 ? right + 6 : (left + right) * 0.5f;
        target.state.angle = angles[scenario];
        float a = angles[scenario] * 0.017453293f, halfWidth = (target.hit.r - target.hit.l) * 0.5f,
              halfHeight = (target.hit.t - target.hit.b) * 0.5f;
        float vertical = absf(bqSin(a)) * halfWidth + absf(bqCos(a)) * halfHeight;
        target.state.y = springSource->rect.t + vertical + 0.5f;
        target.state.vx = 0;
        target.state.vy = incoming[scenario];
        entities[playerIndex].state.x = left - 80;
        entities[playerIndex].state.y = pad.state.y + 100;
        rebuildPhysics();
        if (scenario == 1)
            bodies[boxIndex]->GetFixtureList()->SetRestitution(1);
        bool launched = false, descending = false;
        float speed = 0;
        for (int frame = 0; frame < 60 && !launched; frame++) {
            tick(dt, false, false);
            if (target.state.vy > 100) {
                launched = true;
                speed = target.state.vy;
                for (int c = 0; c < springIncomingCount; c++)
                    if (springIncoming[c].other->GetBody() == bodies[boxIndex] &&
                        springIncoming[c].solved && springIncoming[c].solvedY < 0)
                        descending = true;
            }
        }
        check(
            launched && descending &&
                absf(speed - root(1000 * pad.source->behavior("SpringBehaviour")
                                             ->num("physics-power", 200))) < 0.1f,
            scenario == 0
                ? "A barely descending original yellow box launches from spring using saved incoming velocity"
            : scenario == 1
                ? "High restitution cannot hide a descending spring impact after the solver has already reversed box velocity"
                : "A rotated yellow box overlapping the authored spring edge launches through its real contact without a widened trigger");
        target.state.gone = 1;
    }
}

void selectorMechanismContactChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    navigate(worldScenes[4]);
    page.minimized = true;
    page.minimizePhase = 1;
    int magnet = -1, spring = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        Entity &e = page.playfield[i];
        if (e.source->id == 8759011704404663ULL && boxGroupMatrixOwner(e))
            magnet = i;
        if (e.source->id == 6212760147216476ULL && boxGroupMatrixOwner(e))
            spring = i;
    }
    check(
        magnet >= 0 && spring >= 0,
        "Real fifth-world selector retains both authored magnet and spring matrices for physical contact ordering");
    if (magnet < 0 || spring < 0)
        return;
    u32 stageHash = hashBytes(entities, sizeof(Entity) * entityCount);
    int stageIncomingCount = springIncomingCount, stageSpring = playerSpringContact,
        stageJumps = jumps;
    bool stageMagnet = playerMagnetContact;
    u32 incomingHash =
        hashBytes(springIncoming, sizeof(SpringIncomingVelocity) * springIncomingCount);
    Entity &magnetEntity = page.playfield[magnet];
    Rect magnetFixture = boxGroupMatrixFixture(magnetEntity);
    float top = magnetEntity.state.y + magnetFixture.t;
    page.bean.state.x = 424;
    page.bean.state.y = top - page.bean.hit.b + 0.2f;
    page.bean.state.vx = 0;
    page.bean.state.vy = -10;
    resetUiPhysics(page);
    bool attached = false, bounced = false;
    for (int frame = 0; frame < 30; frame++) {
        stepUiPhysics(page, dt);
        attached |= page.bean.state.active == 2;
        bounced |= page.bean.state.vy > 1;
    }
    check(
        attached && !bounced && page.bean.state.active == 2,
        "Selector Bean remains attached on real magnetic and terrain contacts without depending on last support ownership");
    Entity &springEntity = page.playfield[spring];
    Rect springFixture = boxGroupMatrixFixture(springEntity);
    top = springEntity.state.y + springFixture.t;
    page.bean.state.x = (springEntity.source->rect.l + springEntity.source->rect.r) * 0.5f;
    page.bean.state.y = top - page.bean.hit.b + 0.2f;
    page.bean.state.vx = 0;
    page.bean.state.vy = -10;
    resetUiPhysics(page);
    bool launched = false, savedIncoming = false;
    char uiProbe[3000] = {};
    for (int frame = 0; frame < 20 && !launched; frame++) {
        stepUiPhysics(page, dt);
        char uiRow[180];
        wsprintfA(uiRow, "tick%d x%d feet%d vy%d incoming%d active%d\r\n", frame,
                  (int)page.bean.state.x, (int)hitRect(page.bean).b, (int)page.bean.state.vy,
                  page.contacts->incomingCount, page.bean.state.active);
        lstrcatA(uiProbe, uiRow);
        for (int i = 0; i < page.contacts->incomingCount; i++)
            if (page.contacts->incoming[i].other->GetBody() == page.bodies[page.playfieldCount] &&
                page.contacts->incoming[i].solved && page.contacts->incoming[i].solvedY < 0)
                savedIncoming = true;
        if (page.bean.state.vy > 100)
            launched = true;
    }
    atomicWrite("test-output/selector-spring-incoming-probe.txt", uiProbe, lstrlenA(uiProbe));
    char details[240];
    wsprintfA(details, "launched%d saved%d active%d vy100%d expected100%d\r\n", launched,
              savedIncoming, page.bean.state.active, (int)(page.bean.state.vy * 100),
              (int)((root(-2 * page.bean.physics->num("gravity", -200) *
                          maxf(page.bean.physics->num("jump-height", 37), springEntity.jumpPower)) +
                     page.bean.physics->num("gravity", -200) * dt) *
                    100));
    atomicWrite("test-output/selector-spring-incoming-details.txt", details, lstrlenA(details));
    check(
        launched && savedIncoming && page.bean.state.active == 1 &&
            absf(page.bean.state.vy -
                 (root(-2 * page.bean.physics->num("gravity", -200) *
                       maxf(page.bean.physics->num("jump-height", 37), springEntity.jumpPower)) +
                  page.bean.physics->num("gravity", -200) * dt)) < 0.1f,
        "Selector spring uses its own real incoming-velocity cache and preserves Tom MegaJump power");
    check(
        stageHash == hashBytes(entities, sizeof(Entity) * entityCount) &&
            stageIncomingCount == springIncomingCount &&
            incomingHash ==
                hashBytes(springIncoming, sizeof(SpringIncomingVelocity) * springIncomingCount) &&
            stageMagnet == playerMagnetContact && stageSpring == playerSpringContact &&
            stageJumps == jumps,
        "Selector magnetic and spring callbacks cannot mutate suspended stage bodies, counters or contact caches");
}

void stormySpringContactChecks() {
    const float dt = 1 / 120.0f;
    interfaceEnabled = false;
    debugMode = true;
    musicEnabled = soundEnabled = false;
    startStage(stageById(641851699177ULL));
    readyTimer = 0;
    int spring = -1, box = -1, point = -1;
    for (int i = 0; i < entityCount; i++) {
        u64 id = entities[i].source->id;
        if (id == 627870581536ULL && boxGroupMatrixOwner(entities[i]))
            spring = i;
        if (id == 798147122958ULL)
            box = i;
        if (id == 789478853586ULL)
            point = i;
    }
    check(
        spring >= 0 && box >= 0 && point >= 0,
        "Reported displayed4-6 resolves its real middle spring,40-pixel spiked physics block and checkpoint");
    if (spring < 0 || box < 0 || point < 0)
        return;
    for (int i = 0; i < entityCount; i++)
        if (i != box && entities[i].kind == ENEMY)
            entities[i].state.gone = 1;
    rebuildPhysics();
    Entity &pad = entities[spring];
    Entity &block = entities[box];
    State &bean = entities[playerIndex].state;
    bean.x = entities[point].state.x;
    bean.y = entities[point].source->rect.b - entities[playerIndex].hit.b + 0.2f;
    bean.vx = bean.vy = 0;
    block.state.x = 370;
    block.state.y = 512 - block.hit.b + 1;
    block.state.vx = 0;
    block.state.vy = -20;
    block.state.angle = block.state.aux = 0;
    restorePhysics();
    bool springTouched = false, remoteLaunch = false;
    float peak = -10000;
    for (int frame = 0; frame < 100; frame++) {
        tick(dt, false, false);
        peak = maxf(peak, block.state.vy);
        for (b2ContactEdge *edge = bodies[box]->GetContactList(); edge; edge = edge->next) {
            int other = (int)edge->other->GetUserData() - 1;
            if (other >= 0 && other < entityCount && entities[other].kind == SPRING &&
                edge->contact->IsTouching() && edge->contact->IsEnabled() &&
                edge->contact->GetManifold()->pointCount)
                springTouched = true;
        }
        remoteLaunch |=
            absf(block.state.vy -
                 root(1000 * pad.source->behavior("SpringBehaviour")->num("physics-power", 200))) <
            0.1f;
    }
    check(
        !springTouched && !remoteLaunch && peak < 100,
        "Actual4-6 spiked block beside its checkpoint receives no spring energy when its real fixture never touches a spring");
    block.state.x = (pad.source->rect.l + pad.source->rect.r) * 0.5f;
    Rect fixture = boxGroupMatrixFixture(pad);
    block.state.y = pad.state.y + fixture.t - block.hit.b + 1;
    block.state.vx = 35;
    block.state.vy = -20;
    block.state.angle = block.state.aux = 0;
    restorePhysics();
    bool launched = false, realContact = false, descending = false;
    float vx = 10000, vy = 0;
    for (int frame = 0; frame < 40 && !launched; frame++) {
        tick(dt, false, false);
        if (absf(block.state.vy -
                 root(1000 * pad.source->behavior("SpringBehaviour")->num("physics-power", 200))) <
            0.1f) {
            launched = true;
            vx = block.state.vx;
            vy = block.state.vy;
            for (b2ContactEdge *edge = bodies[spring]->GetContactList(); edge; edge = edge->next)
                if (edge->other == bodies[box] && edge->contact->IsTouching() &&
                    edge->contact->IsEnabled() && edge->contact->GetManifold()->pointCount) {
                    b2WorldManifold m;
                    edge->contact->GetWorldManifold(&m);
                    float normal = edge->contact->GetFixtureA()->GetBody() == bodies[spring]
                                       ? m.normal.y
                                       : -m.normal.y;
                    realContact |= normal > 0.1f;
                    descending |= springWasDescending(edge->contact, bodies[box]);
                }
        }
    }
    check(
        launched && realContact && descending && absf(vx) <= 35.1f,
        "Actual4-6 spring launches a descending spiked block only on its physical upper contact and adds no sideways spring kick");
    char row[300];
    wsprintfA(
        row,
        "scene641851699177 spring627870581536 box798147122958 no-contact peak-vy%d spring-touch%d remote-launch%d; real-contact%d incoming-down%d launch-vx%d launch-vy%d\r\n",
        (int)peak, springTouched ? 1 : 0, remoteLaunch ? 1 : 0, realContact ? 1 : 0,
        descending ? 1 : 0, (int)vx, (int)vy);
    atomicWrite("test-output/stormy-spring-contact.txt", row, lstrlenA(row));
}
