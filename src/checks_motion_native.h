#pragma once

// Regression coverage for motion native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void nativeMotionChecks() {
    const float dt = 1.0f / 120.0f;
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    nativeMotionRenderFPS = 60;
    DiagonalRuntimeState d = {-0.70710678f, -0.70710678f, 100, 1};
    nativeDiagonalOnHit(d, 0.001f, 0.999f);
    check(
        d.directionX > 0 && d.directionY > 0,
        "Native diagonal OnHit reflects both opposing components even below the former normal threshold");
    nativeDiagonalOnHit(d, 0.6f, 0.8f);
    check(d.directionX > 0 && d.directionY > 0,
          "Diagonal separating contacts do not reflect a second time");
    nativeDiagonalOnHit(d, -1, 0);
    check(d.directionX < 0 && d.directionY > 0,
          "Diagonal tangent contact leaves the zero-normal component unchanged");
    d.directionX = d.directionY = 0;
    nativeDiagonalOnHit(d, 1, -1);
    check(d.directionX == 0 && d.directionY == 0,
          "Native diagonal zero direction remains zero on contact");

    startStage(stageById(666279532945ULL));
    readyTimer = 0;
    int fish = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == 710613169890ULL)
            fish = i;
    check(fish >= 0, "Native motion regression resolves the exact authored Cages fish instance");
    if (fish >= 0) {
        Entity &e = entities[fish];
        const Props &config = *e.source->behavior("DiagonalBehaviour");
        float speed = 100;
        int selected = 0;
        selectNativeMotionSpeed(config, 45, speed, selected);
        check(selected == 1 && speed == 100,
              "Fish without a 3g-speed property retains native default100 at low frame rate");
        check(bodies[fish] && bodies[fish]->GetType() == b2_kinematicBody &&
                  oscillatorProxies[fish] && oscillatorProxies[fish]->GetType() == b2_dynamicBody,
              "Cages fish uses the native kinematic carrier plus separate dynamic detector");
        if (oscillatorProxies[fish]) {
            b2Fixture *detector = oscillatorProxies[fish]->GetFixtureList();
            check(
                detector && detector->GetDensity() == 0 && detector->GetFriction() == 0.2f &&
                    detector->GetRestitution() == 0 && detector->GetUserData() == (void *)15,
                "Fish detector retains zero density, friction0.2, restitution0 and its independent contact identity");
        }
        for (int i = 0; i < entityCount; i++)
            if (i != fish && i != playerIndex && entities[i].kind == ENEMY)
                entities[i].state.gone = 1;
        // Keep ALL original terrain and water. The static observer is within
        // the authored224px proximity range, clear of the fish's actual route.
        entities[playerIndex].state.x = 400;
        entities[playerIndex].state.y = 980;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        restorePhysics();
        bodies[playerIndex]->SetType(b2_staticBody);
        bool reflectedX = false, reflectedY = false, finite = true, disabledProxyContact = false;
        float minX = e.state.x, maxX = e.state.x;
        for (int step = 0; step < 1440; step++) {
            tick(dt, false, false);
            DiagonalRuntimeState &state = diagonalRuntime[fish];
            reflectedX |= state.directionX > 0;
            reflectedY |= state.directionY > 0;
            finite &= e.state.x == e.state.x && e.state.y == e.state.y && !e.state.gone;
            minX = minf(minX, e.state.x);
            maxX = maxf(maxX, e.state.x);
            if (oscillatorProxies[fish])
                for (b2ContactEdge *edge = oscillatorProxies[fish]->GetContactList(); edge;
                     edge = edge->next)
                    disabledProxyContact |=
                        edge->contact->IsTouching() && !edge->contact->IsEnabled();
        }
        check(
            finite && maxX - minX > 250 && reflectedX && reflectedY,
            "Original Cages fish naturally reflects along both axes during12seconds with authored terrain and water intact");
        check(
            disabledProxyContact,
            "Fish wall detection happens through disabled PreSolve proxy contacts without solver impulses");
        captureCheckpoint(false);
        DiagonalRuntimeState saved = diagonalRuntime[fish];
        diagonalRuntime[fish].directionX = 0;
        diagonalRuntime[fish].directionY = 0;
        diagonalRuntime[fish].effectiveSpeed = 17;
        diagonalRuntime[fish].selected = 0;
        restoreCheckpoint();
        DiagonalRuntimeState &restored = diagonalRuntime[fish];
        check(restored.directionX == saved.directionX && restored.directionY == saved.directionY &&
                  restored.effectiveSpeed == saved.effectiveSpeed &&
                  restored.selected == saved.selected,
              "Checkpoint restores every native saved diagonal field after a real wall reflection");
    }

    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 2 &&
            levels[stageList[i]].props.num("bean-level") == 8)
            stage = i;
    check(stage >= 0, "The authored Clifftop Tricks low-frame-rate platform stage exists");
    if (stage >= 0) {
        startStage(stage);
        readyTimer = 0;
        int platform = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->id == 14840038685926996665ULL)
                platform = i;
        check(platform >= 0,
              "Low-frame-rate speed regression resolves the original99versus100 platform");
        if (platform >= 0) {
            Entity &e = entities[platform];
            float speed = 100;
            int selected = 0;
            selectNativeMotionSpeed(*e.move, 45, speed, selected);
            selectNativeMotionSpeed(*e.move, 60, speed, selected);
            check(
                speed == 99 && selected,
                "At exactly45FPS the native authored3g-speed99 is selected once and survives faster frames");
            speed = 100;
            selected = 0;
            selectNativeMotionSpeed(*e.move, 45.001f, speed, selected);
            selectNativeMotionSpeed(*e.move, 30, speed, selected);
            check(
                speed == 100 && selected,
                "Above45FPS the first native decision retains100 and does not adapt on later slow frames");
            resetNativeMotion(platform);
            OscillatorRuntimeState &o = oscillatorRuntime[platform];
            float initialX = e.state.x, initialY = e.state.y;
            advanceNativeOscillator(platform, dt);
            check(
                absf(e.state.vx) < 0.001f && absf(e.state.vy) < 0.001f &&
                    absf(o.clock - (-e.lo / e.speed + dt)) < 0.00001f,
                "Native oscillator samples its initial clock before advancing time, avoiding the port's one-step phase lead");
            o.clock = 0.25f;
            o.effectiveSpeed = 100;
            float phase = nativeOscillatorPhase(e, o);
            e.state.x = initialX + 13;
            e.state.y = initialY - 27;
            nativeOscillatorOnHit(e, o, -e.moveX, -e.moveY);
            check(
                absf(nativeOscillatorPhase(e, o) - (2 * (e.hi - e.lo) - phase)) < 0.0001f,
                "Native oscillator wall reversal mirrors its saved clock independently of a displaced body");
            o.clock = -2 * (e.hi - e.lo) / o.effectiveSpeed;
            check(absf(nativeOscillatorPhase(e, o) - 2 * (e.hi - e.lo)) < 0.001f,
                  "Negative exact-cycle oscillator time preserves native signed-fmod end boundary");
        }
    }
    // Trace the exact selector fixture also checked by checks_contacts.h.
    // Keep this independent of synthetic physics scenes and record actual
    // contacts/phase so a bounds failure cannot be hidden by a loose limit.
    interfaceEnabled = true;
    navigate(worldScenes[4]);
    int rail = -1;
    for (int i = 0; i < page.playfieldCount; i++)
        if (page.playfield[i].move) {
            rail = i;
            break;
        }
    if (rail >= 0) {
        char *trace = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 65536);
        lstrcpyA(trace, "step,x1000,y1000,vx1000,clock1000,dir,contacts,disabled\r\n");
        Entity &e = page.playfield[rail];
        float low = e.state.x, high = low, previous = e.state.aux, hitX = 0, hitDirection = 0;
        int previousContacts = 0, returnStep = -1, returns = 0;
        bool immediateReturn = true;
        for (int step = 0; step < 720; step++) {
            tickInterface(dt);
            low = minf(low, e.state.x);
            high = maxf(high, e.state.x);
            int contacts = 0, disabled = 0;
            for (b2ContactEdge *edge = page.oscillatorProxies[rail]->GetContactList(); edge;
                 edge = edge->next)
                if (edge->contact->IsTouching()) {
                    contacts++;
                    disabled += !edge->contact->IsEnabled();
                }
            if (contacts && !previousContacts) {
                returnStep = step + 2;
                hitX = e.state.x;
                hitDirection = e.state.vx < 0 ? -1.0f : 1.0f;
            }
            if (step == returnStep) {
                returns++;
                immediateReturn &=
                    e.state.vx * hitDirection < 0 && (e.state.x - hitX) * hitDirection < 0.01f;
            }
            if (step % 12 == 0 || contacts || e.state.aux != previous) {
                char row[160];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d\r\n", step, (int)(e.state.x * 1000),
                          (int)(e.state.y * 1000), (int)(e.state.vx * 1000),
                          (int)(page.oscillator[rail].clock * 1000), (int)e.state.aux, contacts,
                          disabled);
                lstrcatA(trace, row);
            }
            previous = e.state.aux;
            previousContacts = contacts;
        }
        char row[120];
        wsprintfA(row, "bounds,%d,%d,y=%d\r\n", (int)(low * 1000), (int)(high * 1000),
                  (int)(e.state.y * 1000));
        lstrcatA(trace, row);
        atomicWrite("test-output/native-selector-motion.csv", trace, lstrlenA(trace));
        HeapFree(GetProcessHeap(), 0, trace);
        float maximumAdvance = 2 * e.speed * dt;
        check(
            low >= 384 - e.hit.l - maximumAdvance - 0.01f &&
                high <= 544 - e.hit.r + maximumAdvance + 0.01f && absf(e.state.y - 232) < 0.01f,
            "Actual selector wall excursion is bounded by the native two-step clock and PreSolve order");
        check(
            returns >= 4 && immediateReturn,
            "Every actual selector wall contact reverses velocity and returns by two physics steps without solver clamping");
    }
    interfaceEnabled = false;
    nativeMotionRenderFPS = 60;
    startStage(0);
    readyTimer = 0;
}
