#pragma once

// Regression coverage for wizard solved native. Runs only in the isolated BQ_ENABLE_TESTS executable.

bool wizardSolvedPendingClockNative(const Entity &boss) {
    u32 bits;
    memcpy(&bits, &boss.state.aux, 4);
    return bits == 0x80000000u;
}

class WizardSolvedObserverNative : public OriginalContacts {
  public:
    int boss, solved;
    Vec normal;
    bool blocked;
    int hpAtSolve;
    float vxAtSolve, vyAtSolve;

    explicit WizardSolvedObserverNative(int index)
        : boss(index), solved(0), blocked(false), hpAtSolve(-1), vxAtSolve(0), vyAtSolve(0) {
        normal = Vec{0, 0};
    }

    bool pair(b2Contact *c) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        return a == bodies[boss] && b == bodies[playerIndex] ||
               b == bodies[boss] && a == bodies[playerIndex];
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        if (blocked && pair(c))
            c->SetEnabled(false);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        if (pair(c)) {
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            float sign = c->GetFixtureA()->GetBody() == bodies[boss] ? 1.0f : -1.0f;
            normal = Vec{sign * m.normal.x, sign * m.normal.y};
            solved++;
            hpAtSolve = entities[boss].state.hp;
            vxAtSolve = bodies[playerIndex]->GetLinearVelocity().x / metres;
            vyAtSolve = bodies[playerIndex]->GetLinearVelocity().y / metres;
        }
        OriginalContacts::PostSolve(c, impulse);
    }
};

int wizardSolvedPrepareNative(int side, bool finalHit, int &summon) {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    int boss = -1;
    summon = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == BOSS)
            boss = i;
        if (summon < 0 && entities[i].bossSpawn == 2)
            summon = i;
    }
    if (boss < 0)
        return -1;
    for (int i = 0; i < entityCount; i++)
        if (i != boss && i != playerIndex)
            entities[i].state.gone = 1;
    current.geometryCount = 0;
    Entity &b = entities[boss];
    b.state.hp = finalHit ? 1 : 5;
    b.state.active = 3;
    b.state.timer = 3;
    resetWizardMotion(b, 5489);
    float arena = wizardArena(b);
    camX = arena - viewW * .5f;
    camY = 0;
    cameraState.x = camX;
    cameraState.y = 0;
    cameraState.valid = 1;
    wizardMotion.baseX = arena + 192;
    wizardMotion.baseY = viewH * .5f;
    wizardMotion.offsetX = 192;
    wizardMotion.amplitudeY = -10;
    wizardMotion.leftAge = wizardMotion.rightAge = -1;
    wizardMotion.cycleAge = 0;
    wizardMotion.shooting = 1;
    b.state.x = wizardMotion.baseX;
    b.state.y = wizardMotion.baseY;
    Entity &bean = entities[playerIndex];
    bean.state.x = b.state.x;
    bean.state.y = b.state.y;
    bean.state.vx = 0;
    bean.state.vy = 0;
    bean.state.timer = .3f;
    bean.state.hp = 1;
    if (side == 0) {
        bean.state.y = b.state.y + b.hit.t - bean.hit.b + .15f;
        bean.state.vy = -100;
    }
    if (side == 1) {
        bean.state.x = b.state.x + b.hit.r - bean.hit.l + .15f;
        bean.state.vx = -70;
    }
    if (side == 2) {
        bean.state.y = b.state.y + b.hit.b - bean.hit.t - .15f;
        bean.state.vy = 100;
    }
    if (summon >= 0) {
        entities[summon].state.gone = 0;
        entities[summon].state.active = 1;
        entities[summon].state.x = arena - 1000;
        entities[summon].state.y = 1000;
    }
    alternationBits = legacyWizardArenaParity(b.state.hp, 3, false);
    jumps = 7;
    restorePhysics();
    clearEffects();
    return boss;
}

void wizardSolvedMoveBeanAwayNative() {
    Entity &bean = entities[playerIndex];
    bean.state.x = 6000;
    bean.state.y = 1000;
    bodies[playerIndex]->SetTransform(b2Vec2(bean.state.x * metres, bean.state.y * metres), 0);
}

bool wizardSolvedDiskReloadNative() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(stageById(976871008727ULL));
    bool read = readCheckpoint("test-output/wizard-hit-pending.dat");
    if (read)
        restoreCheckpoint();
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    bool pending = read && boss >= 0 && entities[boss].state.hp == 5 &&
                   entities[boss].state.active == 3 &&
                   wizardSolvedPendingClockNative(entities[boss]);
    wizardSolvedMoveBeanAwayNative();
    if (pending)
        tick(1 / 120.0f, false, false);
    check(
        pending && entities[boss].state.hp == 4 && pinballLaunchedThisStepNative &&
            pinballConsumedNative.source == boss,
        "Fresh-process reload resumes the saved Wizard hit clock once and delivers its strong bounce in the next physical step");
    return failures == 0;
}

void wizardSolvedNativeChecks() {
    bool real = true, top = true, next = true, launch = true, side = true, bottom = true,
         pause = true, restore = true, final = true;
    char trace[4096] =
        "rate,contact,hp_contact,hp_next,normal_x_milli,normal_y_milli,queued,launched,jumps\r\n";
    for (int rate = 0; rate < 3; rate++)
        for (int contact = 0; contact < 4; contact++) {
            float dt = 1.0f / (rate == 0 ? 30 : rate == 1 ? 120 : 240);
            int summon = -1,
                boss = wizardSolvedPrepareNative(contact < 3 ? contact : 0, contact == 3, summon);
            if (boss < 0) {
                real = false;
                continue;
            }
            Entity &b = entities[boss];
            Entity &bean = entities[playerIndex];
            WizardSolvedObserverNative observer(boss);
            physicsWorld->SetContactListener(&observer);
            int hp = b.state.hp, bits = alternationBits;
            tick(dt, false, false);
            bool observed = observer.solved > 0;
            real &= observed && observer.hpAtSolve == hp;
            int contactHp = b.state.hp;
            PinballPendingNative request = pinballPendingNative;
            if (contact == 0 || contact == 3) {
                top &= observed && observer.normal.y > .1f && b.state.hp == hp &&
                       b.state.active == 3 && wizardSolvedPendingClockNative(b) && summon >= 0 &&
                       !entities[summon].state.gone && alternationBits == bits && !request.pending;
                if (rate == 1 && contact == 0) {
                    captureCheckpoint(false);
                    pause = true;
                    Screen old = screen;
                    screen = PAUSE;
                    float clock = b.state.aux;
                    tick(dt, false, false);
                    pause &= clock == b.state.aux && b.state.hp == hp;
                    screen = old;
                    bool written = atomicWrite("test-output/wizard-hit-pending.dat", &checkpoint,
                                               sizeof(checkpoint));
                    b.state.aux = 0;
                    restoreCheckpoint();
                    restore &= written && wizardSolvedPendingClockNative(b) && b.state.hp == hp &&
                               b.state.active == 3;
                }
            } else {
                bool queued = observed && b.state.hp == hp && request.pending &&
                              request.source == boss &&
                              absf(request.normal.x - observer.normal.x) < .01f &&
                              absf(request.normal.y - observer.normal.y) < .01f &&
                              absf(request.speed - root(800 * 130)) < .01f && jumps == 7 &&
                              absf(bean.state.vx - observer.vxAtSolve) < .01f &&
                              absf(bean.state.vy - observer.vyAtSolve) < .01f;
                if (contact == 1)
                    side &= queued;
                if (contact == 2)
                    bottom &= queued;
            }
            wizardSolvedMoveBeanAwayNative();
            float incomingX = bean.state.vx;
            tick(dt, false, false);
            if (contact == 0 || contact == 3) {
                bool damaged = b.state.hp == hp - 1 && b.state.active == (contact == 3 ? 4 : 2) &&
                               entities[summon].state.gone && alternationBits != (bits) &&
                               !pinballPendingNative.pending;
                // Final gate opening happens after the death sequence, not on its first step.
                if (contact == 3)
                    damaged = b.state.hp == 0 && b.state.active == 4 &&
                              entities[summon].state.gone && wizardInputLocked();
                next &= damaged;
                float dx = contact == 3 ? -.45f : -.75f, dy = .35f, n = root(dx * dx + dy * dy);
                launch &= pinballLaunchedThisStepNative && pinballConsumedNative.source == boss &&
                          absf(pinballConsumedNative.speed - root(800 * 350)) < .01f &&
                          absf(pinballConsumedNative.normal.x - dx / n) < .00001f &&
                          absf(pinballConsumedNative.normal.y - dy / n) < .00001f && jumps == 8;
                if (contact == 3)
                    final &= pinballLaunchedThisStepNative && bean.state.vx < incomingX &&
                             bean.state.vy > 0;
            } else {
                bool consumed = pinballLaunchedThisStepNative &&
                                pinballConsumedNative.source == boss && b.state.hp == hp &&
                                jumps == 8;
                if (contact == 1)
                    side &= consumed;
                if (contact == 2)
                    bottom &= consumed;
            }
            char row[180];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", rate, contact, contactHp, b.state.hp,
                      (int)(observer.normal.x * 1000), (int)(observer.normal.y * 1000),
                      request.pending, pinballLaunchedThisStepNative, jumps);
            lstrcatA(trace, row);
            physicsWorld->SetContactListener(originalContacts);
        }
    check(
        real,
        "Original Wizard and Bean produce independently observed solved head, side and bottom contacts at30/120/240Hz");
    check(
        top,
        "A solved head contact records a saved zero hit-clock while preserving contact-frame HP, summons and arena state");
    check(
        next,
        "The next Wizard PrePhysics consumes the recorded head hit once and dismisses summons through the native transition");
    check(
        launch,
        "Wizard PrePhysics precedes Tom consumption: its350power request wins over the ordinary ground jump and launches once with native normal/count");
    check(
        side,
        "Actual side impacts queue normalized130power MegaJump without immediate velocity changes and consume it in the following step");
    check(
        bottom,
        "Actual underside impacts use the same deferred130power downward MegaJump without damaging the Wizard");
    check(
        pause && restore,
        "Pause preserves a pending Wizard hit and in-memory checkpoint restoration retains it without applying damage at load");
    check(
        final,
        "The final recorded head hit uses its separate native bounce direction while entering controlled death only in the next update");
    int summon = -1, boss = wizardSolvedPrepareNative(0, false, summon);
    bool reset = false;
    if (boss >= 0) {
        tick(1 / 120.0f, false, false);
        restartStage();
        reset = entities[boss].state.hp == 5 && entities[boss].state.active == 0 &&
                !pinballPendingNative.pending;
    }
    check(
        reset,
        "Restart discards the pending Wizard hit and shared forced-jump request instead of carrying it into the new battle");
    bool filtered = true;
    for (int mode = 0; mode < 2; mode++) {
        boss = wizardSolvedPrepareNative(0, false, summon);
        WizardSolvedObserverNative observer(boss);
        physicsWorld->SetContactListener(&observer);
        if (mode)
            bodies[boss]->GetFixtureList()->SetSensor(true);
        else
            observer.blocked = true;
        tick(1 / 120.0f, false, false);
        filtered &= observer.solved == 0 && entities[boss].state.hp == 5 &&
                    !wizardSolvedPendingClockNative(entities[boss]) &&
                    !pinballPendingNative.pending;
        physicsWorld->SetContactListener(originalContacts);
    }
    check(filtered,
          "Disabled and sensor Wizard contacts cannot record a head hit or request a forced jump");
    boss = wizardSolvedPrepareNative(0, false, summon);
    Entity &idle = entities[boss];
    idle.state.active = 0;
    wizardMotion.initialBeanX = entities[playerIndex].state.x;
    simulateBodies(1 / 120.0f);
    bool recorded = wizardSolvedPendingClockNative(idle);
    wizardSolvedMoveBeanAwayNative();
    wizardMotion.initialBeanX = entities[playerIndex].state.x;
    updateWizard(idle, 1 / 120.0f);
    bool held = idle.state.hp == 5 && idle.state.active == 0;
    wizardBeginCombat(idle);
    updateWizard(idle, 1 / 120.0f);
    check(
        recorded && held && idle.state.hp == 5 && idle.state.active == 3 && idle.state.aux == -2,
        "A physical head contact outside combat records the native clock but cannot apply damage, and arena entry resets that clock to2");
    boss = wizardSolvedPrepareNative(0, false, summon);
    entities[boss].state.aux = 0;
    captureCheckpoint(false);
    bool written =
        atomicWrite("test-output/wizard-hit-legacy.dat", &checkpoint, sizeof(checkpoint));
    entities[boss].state.aux = -1;
    bool read = written && readCheckpoint("test-output/wizard-hit-legacy.dat");
    if (read)
        restoreCheckpoint();
    wizardSolvedMoveBeanAwayNative();
    updateWizard(entities[boss], 1 / 120.0f);
    check(
        read && checkpoint.version == 28 && sizeof(Snapshot) == 543392 &&
            entities[boss].state.hp == 5,
        "A legacy version28 Boss aux0 record remains readable and cannot be mistaken for a newly recorded zero hit-clock");
    boss = wizardSolvedPrepareNative(0, false, summon);
    entities[boss].state.aux = -.00000001f;
    float cycle = wizardMotion.cycleAge;
    wizardSolvedMoveBeanAwayNative();
    updateWizard(entities[boss], 1 / 120.0f);
    check(
        entities[boss].state.hp == 5 && entities[boss].state.aux < -.00000001f &&
            wizardMotion.cycleAge == cycle,
        "Only exact zero hit-clock enters native damage; a positive recovery clock advances without reentering the shooting cycle");
    boss = wizardSolvedPrepareNative(0, false, summon);
    const u32 clockBits[] = {0, 1, 0x322bcc77u, 0x3c088889u, 0x40000000u};
    bool exact = true;
    for (int n = 0; n < 5; n++) {
        float value;
        memcpy(&value, &clockBits[n], 4);
        setWizardHitClockNative(entities[boss], value);
        float decoded = wizardHitClockNative(entities[boss]);
        u32 encoded, decodedBits;
        memcpy(&encoded, &entities[boss].state.aux, 4);
        memcpy(&decodedBits, &decoded, 4);
        exact &= decodedBits == clockBits[n] && encoded == (clockBits[n] | 0x80000000u);
    }
    check(
        exact,
        "The saved Wizard clock preserves all binary32 bits including zero, subnormal and tiny positive ages while distinguishing legacy positive zero");
    atomicWrite("test-output/wizard-solved-observed.csv", trace, lstrlenA(trace));
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
    atomicWrite("test-output/wizard-solved-results.txt", testLog, lstrlenA(testLog));
}
