#pragma once

// Regression coverage for cloud57 jump native. Runs only in the isolated BQ_ENABLE_TESTS executable.
class Cloud57JumpProbeNative : public OriginalContacts {
  public:
    int cloud, solved, disabledAfterSolve, residueRetained;
    bool eligible;
    float ny, incoming;

    Cloud57JumpProbeNative(int index)
        : cloud(index), solved(0), disabledAfterSolve(0), residueRetained(0), eligible(false),
          ny(0), incoming(0) {}

    bool matches(b2Contact *c) {
        b2Body *a = c->GetFixtureA()->GetBody();
        b2Body *b = c->GetFixtureB()->GetBody();
        return (a == bodies[playerIndex] && b == bodies[cloud]) ||
               (b == bodies[playerIndex] && a == bodies[cloud]);
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        if (solved && matches(c)) {
            if (!c->IsEnabled())
                disabledAfterSolve++;
            float vy = bodies[playerIndex]->GetLinearVelocity().y;
            if (c->IsEnabled() && vy > 0 && vy <= b2_epsilon)
                residueRetained++;
        }
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        if (matches(c)) {
            b2WorldManifold m;
            c->GetWorldManifold(&m);
            ny = c->GetFixtureA()->GetBody() == bodies[playerIndex] ? -m.normal.y : m.normal.y;
            incoming = tomOrdinaryIncomingVyNative;
            eligible |= nativeTomCanAutoJump(incoming, ny);
            solved++;
        }
        OriginalContacts::PostSolve(c, impulse);
    }
};

void cloud57JumpNativeChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = debugMode = false;
    stopMusic();
    stopSounds();
    const u64 clouds[] = {6603767170669253ULL, 8696626167641160ULL};
    const float speeds[] = {-120, -1, -120, -600};
    const float heights[] = {1, .1f, 81, 81};
    bool cloudJump[] = {true, true}, burst = true, continues = true;
    int solved = 0, eligible = 0, failed = 0, lostGround = 0, lateDisabled = 0, residueRetained = 0;
    char csv[10000] = {};
    lstrcpyA(
        csv,
        "cloud,offset,height1000,speed1000,solved,eligible,normalY1000,incoming1000,grounded,recorded,preVy1000,afterVy1000,active,lateDisabled\r\n");
    // Actual authored late5-7 pair, centre and both top edges, including
    // subpixel/TOI landings that lose their final ground-list contact.
    for (int ci = 0; ci < 2; ci++)
        for (int offset = -24; offset <= 24; offset += 24)
            for (int speed = 0; speed < 4; speed++) {
                float height = heights[speed];
                startStage(stageById(881271676947ULL));
                readyTimer = 0;
                int cloud = crumblingCloudNativeIndex(clouds[ci]);
                if (cloud < 0) {
                    cloudJump[ci] = false;
                    continue;
                }
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].kind == ENEMY)
                        entities[i].state.gone = 1;
                Entity &p = entities[playerIndex];
                Rect r = hitRect(entities[cloud]);
                p.state.x = (r.l + r.r) * .5f + offset;
                p.state.y = r.t - p.hit.b + height;
                p.state.vx = 0;
                p.state.vy = speeds[speed];
                p.state.timer = .2f;
                restorePhysics();
                Cloud57JumpProbeNative *probe = new Cloud57JumpProbeNative(cloud);
                physicsWorld->SetContactListener(probe);
                for (int frame = 0; frame < 180 && !probe->solved && deathTimer <= 0; frame++)
                    tick(1 / 120.0f, false, false);
                if (probe->solved)
                    solved++;
                if (probe->eligible) {
                    tick(1.0f / 120, false, false);
                    eligible++;
                    bool good = p.state.vy >= p.jumpPower * .95f;
                    cloudJump[ci] &= good;
                    if (!good)
                        failed++;
                    if (!onGround)
                        lostGround++;
                }
                lateDisabled += probe->disabledAfterSolve;
                residueRetained += probe->residueRetained;
                burst &= probe->solved && entities[cloud].state.active == 2 &&
                         !bodies[cloud]->IsActive() && !entities[cloud].state.gone;
                char row[240];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", ci, offset,
                          (int)(height * 1000), (int)(speeds[speed] * 1000), probe->solved,
                          (int)probe->eligible, (int)(probe->ny * 1000),
                          (int)(probe->incoming * 1000), (int)onGround,
                          (int)tomOrdinaryNormalFoundNative, (int)(preStepVy * 1000),
                          (int)(p.state.vy * 1000), entities[cloud].state.active,
                          probe->disabledAfterSolve);
                lstrcatA(csv, row);
                float y = p.state.y;
                for (int frame = 0; frame < 3; frame++)
                    tick(1.0f / 120, false, false);
                continues &= p.state.y > y + 2 && p.state.vy > 0 && p.state.timer >= .024f &&
                             entities[cloud].state.active == 2;
                physicsWorld->SetContactListener(originalContacts);
                delete probe;
            }
    check(
        solved == 24 && eligible == 24,
        "Both actual late5-7 clouds receive all24 real descending centre, edge, weak and fast solved impacts");
    check(
        cloudJump[0],
        "Lower5-7 cloud launches Bean at full native power on every qualifying solved contact, even when its final support contact is gone");
    check(
        cloudJump[1],
        "Upper5-7 cloud launches Bean at full native power on every qualifying solved contact, even when its final support contact is gone");
    // Before the Windows numerical guard these TOI rechecks were disabled
    // solely by positive resting roundoff. Keep the lost-support jump test,
    // and now require those actual residual rechecks to remain enabled.
    check(
        lostGround == 24 && lateDisabled == 0 && residueRetained > 0,
        "All24 burst clouds lose final support after recording Tom's jump while actual sub-epsilon TOI rechecks retain their support contact");
    check(
        burst,
        "Both5-7 clouds still burst and disable collision on the solved impact without permanent deletion");
    check(
        continues,
        "Bean keeps rising after the cloud bursts instead of dropping or receiving repeated jumps from its removed fixture");
    atomicWrite("test-output/cloud57-contact-matrix.csv", csv, lstrlenA(csv));
    char summary[220];
    wsprintfA(
        summary,
        "solved=%d eligible=%d missed=%d lostFinalGround=%d disabledAfterSolve=%d residueRechecksRetained=%d\r\n",
        solved, eligible, failed, lostGround, lateDisabled, residueRetained);
    atomicWrite("test-output/cloud57-summary.txt", summary, lstrlenA(summary));
    startStage(0);
    readyTimer = 0;
}
