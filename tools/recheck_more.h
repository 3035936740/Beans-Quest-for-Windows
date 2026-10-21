// Controlled reproductions using the real gameplay implementation. Each probe
// starts from an authored stage; only the diagnostic saveRoot is writable here.
void moreRechecks() {
    const float dt = 1 / 120.0f;
    char detail[768];
    lstrcatA(report, "\r\nAdditional physics, checkpoint and input probes\r\n");

    // Expiry happens after simulateBodies. A different turret can reuse that
    // logical slot on the next frame before the old Box2D body is destroyed.
    startStage(42);
    int first = -1, second = -1;
    float distance = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == TURRET) {
            if (first < 0)
                first = i;
            else {
                float d = absf(entities[i].state.x - entities[first].state.x) +
                          absf(entities[i].state.y - entities[first].state.y);
                if (d > distance) {
                    distance = d;
                    second = i;
                }
            }
        }
    if (first >= 0 && second >= 0) {
        shoot(entities[first], *entities[first].source->behavior("BulletSource"));
        simulateBodies(dt);
        b2Body *oldBody = shotBodies[0];
        bullets[0].life = dt * 0.5f;
        tick(dt, false, false);
        bool expired = bullets[0].life <= 0;
        float oldX = bullets[0].state.x, oldY = bullets[0].state.y;
        shoot(entities[second], *entities[second].source->behavior("BulletSource"));
        float expectedX = bullets[0].state.x, expectedY = bullets[0].state.y;
        simulateBodies(dt);
        float actualX = bullets[0].state.x, actualY = bullets[0].state.y;
        wsprintfA(
            detail,
            "5-06; expired=%d; old=(%d,%d); new spawn=(%d,%d); actual next=(%d,%d); same body=%d",
            expired, (int)oldX, (int)oldY, (int)expectedX, (int)expectedY, (int)actualX,
            (int)actualY, shotBodies[0] == oldBody);
        observation(expired && shotBodies[0] == oldBody &&
                        absf(actualX - expectedX) + absf(actualY - expectedY) > 100,
                    "Reused projectile slot fires from its previous body's location", detail);
    }

    // Use the authored circular guard from 5-01. Remove unrelated contacts and
    // freeze its orbit to compare its exact fixture with the manual AABB check.
    startStage(37);
    int hazard = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].deadly && entities[i].circle && entities[i].motion == 2) {
            hazard = i;
            break;
        }
    if (hazard >= 0) {
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex && i != hazard)
                entities[i].state.gone = 1;
        current.geometryCount = 0;
        Entity &e = entities[hazard];
        Entity &p = entities[playerIndex];
        e.motion = e.orbit = e.dynamic = 0;
        e.move = 0;
        e.state.x = 500;
        e.state.y = 300;
        e.state.vx = e.state.vy = 0;
        e.state.active = 1;
        p.state.x = e.state.x + e.hit.r - p.hit.l - 1;
        p.state.y = e.state.y + e.hit.t - p.hit.b - 1;
        p.state.vx = p.state.vy = 0;
        rebuildPhysics();
        b2Fixture *pf = bodies[playerIndex]->GetFixtureList();
        b2Fixture *ef = bodies[hazard]->GetFixtureList();
        bool exact =
            b2TestOverlap(pf->GetShape(), 0, ef->GetShape(), 0, bodies[playerIndex]->GetTransform(),
                          bodies[hazard]->GetTransform());
        bool aabb = overlaps(hitRect(p), hitRect(e));
        tick(dt, false, false);
        wsprintfA(
            detail,
            "5-01 circular guard; circle/box overlap=%d; AABB overlap=%d; player contact=%d; death timer(ms)=%d",
            exact, aabb, playerContact(hazard), (int)(deathTimer * 1000));
        observation(!exact && aabb && !playerContact(hazard) && deathTimer > 0,
                    "Circular hazard kills outside its physical shape", detail);
    }

    // A valid tilted pose has an unchanged hinge reference before saving.
    // Restoring the pose must not recenter the hinge's angular limits on it.
    startStage(0);
    int seesaw = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].rotate == 2 && hinges[i]) {
            seesaw = i;
            break;
        }
    if (seesaw >= 0) {
        b2Body *body = bodies[seesaw];
        body->SetTransform(body->GetPosition(), 0.5f);
        simulateBodies(dt);
        b2RevoluteJoint *joint = hinges[seesaw];
        float before = joint->GetBodyB()->GetAngle() - joint->GetBodyA()->GetAngle() -
                       joint->GetJointAngle(),
              upperBefore = before + joint->GetUpperLimit();
        float saved = body->GetAngle();
        captureCheckpoint(true);
        restoreCheckpoint();
        joint = hinges[seesaw];
        float after = joint->GetBodyB()->GetAngle() - joint->GetBodyA()->GetAngle() -
                      joint->GetJointAngle(),
              upperAfter = after + joint->GetUpperLimit();
        wsprintfA(
            detail,
            "1-01; saved tilt(milliradians)=%d; hinge reference before/after=%d/%d; absolute upper limit before/after=%d/%d",
            (int)(saved * 1000), (int)(before * 1000), (int)(after * 1000),
            (int)(upperBefore * 1000), (int)(upperAfter * 1000));
        observation(absf(after - before) > 0.1f && absf(upperAfter - upperBefore) > 0.1f,
                    "Checkpoint restore shifts seesaw angular limits", detail);
    }

    // Bind via the actual Controls button and route the new key through the
    // real key handler. Up now selects menus; Space remains confirmation.
    defaultBindings();
    navigate(9521867969610021ULL);
    focusId(page, 1266841032746010ULL);
    confirm();
    keyPressed('W');
    bool stored = primaryKey[2] == 'W';
    navigate(12192614360109026946ULL);
    int before = page.focus;
    keyPressed('W');
    int remapped = page.focus;
    navigate(12192614360109026946ULL);
    keyPressed(VK_UP);
    int fixed = page.focus;
    wsprintfA(
        detail,
        "Controls up row stores W=%d; initial focus=%d; focus after W=%d; focus after fixed Up=%d",
        stored, before, remapped, fixed);
    observation(stored && remapped == before && fixed != before,
                "Controls up binding is saved but ignored by input handling", detail);
    defaultBindings();
}
