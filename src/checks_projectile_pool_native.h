#pragma once

// Regression coverage for projectile pool native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void projectilePoolNativeChecks() {
    const float dt = 1.0f / 120;
    int stage = stageById(7138745870511843ULL);
    check(stage >= 0,
          "Original Inner Chambers stage is available for mixed-class projectile pool regression");
    if (stage < 0)
        return;
    startStage(stage);
    readyTimer = 0;
    int fire = -1, plasma = -1;
    for (int i = 0; i < entityCount; i++) {
        const Props *p = entities[i].source->behavior("BulletSource");
        if (p) {
            u64 klass = idnum(p->get("bulletClass"));
            if (klass == 6430600862785566ULL)
                fire = i;
            if (klass == 12978618438667112518ULL)
                plasma = i;
        }
        if (i != playerIndex)
            entities[i].state.gone = 1;
    }
    check(fire >= 0 && plasma >= 0,
          "Original Inner Chambers contains both authored fireball and plasma emitters");
    if (fire < 0 || plasma < 0)
        return;
    current.geometryCount = 0;
    State &bean = entities[playerIndex].state;
    bean.x = 48;
    bean.y = 800;
    bean.vx = bean.vy = 0;
    rebuildPhysics();
    int emitters[] = {fire, plasma, fire, plasma};
    bool classesCorrect = true, materialsCorrect = true, stableSameClass = true;
    for (int n = 0; n < 4; n++) {
        Entity &emitter = entities[emitters[n]];
        shoot(emitter, *emitter.source->behavior("BulletSource"));
        simulateBodies(dt);
        Bullet &bullet = bullets[0];
        b2Body *body = shotBodies[0];
        bool isFire = emitters[n] == fire;
        float expectedRadius = isFire ? 6.0f : -2.0f; // Native signed adjusted-width radius.
        classesCorrect &=
            body && bullet.life > 0 &&
            classes[bullet.klass].id == (isFire ? 6430600862785566ULL : 12978618438667112518ULL) &&
            shotBodyClasses[0] == bullet.klass;
        if (body) {
            b2Fixture *f = body->GetFixtureList();
            materialsCorrect &=
                f && f->GetShape()->GetType() == b2Shape::e_circle &&
                absf(((b2CircleShape *)f->GetShape())->m_radius / metres - expectedRadius) <
                    0.0001f &&
                body->IsFixedRotation() == !isFire &&
                f->GetFilterData().maskBits ==
                    nativePhysicsMask(classes[bullet.klass].behavior("CirclePhysicsBehaviour"));
            b2Body *before = body;
            ensureShotBodyClass(0);
            stableSameClass &= shotBodies[0] == before;
        }
        destroyShot(0);
    }
    check(
        classesCorrect && materialsCorrect,
        "A slot reused fireball-plasma-fireball-plasma rebuilds each original signed class radius, fixed rotation and collision filter");
    check(
        stableSameClass,
        "Ensuring an unchanged projectile class preserves its body and existing contact lifecycle");
    // Check all original projectile fixture differences, not just the two
    // classes naturally sharing this stage. This is an explicit pool stress
    // case; it does not claim the Wizard appears in Inner Chambers.
    Bullet &bullet = bullets[0];
    bullet.life = 1;
    bullet.klass = (int)(classById(537747635566ULL) - classes);
    bullet.state.x = 400;
    bullet.state.y = 800;
    bullet.state.vx = bullet.state.vy = 0;
    ensureShotBodyClass(0);
    b2Fixture *wizard = shotBodies[0]->GetFixtureList();
    check(
        wizard->GetFilterData().maskBits == 2 && absf(wizard->GetFriction() - 0.2f) < 0.0001f &&
            absf(wizard->GetRestitution() - 0.5f) < 0.0001f &&
            absf(((b2CircleShape *)wizard->GetShape())->m_radius / metres - 16) < 0.0001f,
        "Explicit cross-class pool stress rebuilds Wizard ball filters and material instead of retaining cannon fixture data");
    ShotSavedContacts *saved = (ShotSavedContacts *)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ShotSavedContacts) * MAX_PROJECTILES);
    bullet.state.timer = 1.25f;
    shotContacts[0].first = false;
    shotContacts[0].ignored[playerIndex / 32] |= 1u << (playerIndex % 32);
    captureShotContacts(saved);
    physicsWorld->DestroyBody(shotBodies[0]);
    shotBodies[0] = 0;
    memset(shotContacts, 0, sizeof(shotContacts));
    shotContacts[0].hit = shotContacts[0].hitBean = true;
    restoreShotContacts(saved);
    prepareShot(0);
    check(
        shotBodies[0] && shotBodies[0]->IsActive() && shotBodyClasses[0] == bullet.klass &&
            !shotContacts[0].first && shotIgnored(0, playerIndex) &&
            shotContacts[0].lastTime == bullet.state.timer && !shotContacts[0].hit &&
            !shotContacts[0].hitBean,
        "Restoring saved projectile contacts builds missing class-correct bodies and preserves ignored identities past prepareShot while clearing transient hits");
    saved[0].first = 1;
    restoreShotContacts(saved);
    prepareShot(0);
    check(
        shotContacts[0].first && shotIgnored(0, playerIndex),
        "A projectile genuinely saved before its first physics step retains the native launch-ignore collection flag");
    HeapFree(GetProcessHeap(), 0, saved);
    startStage(0);
    readyTimer = 0;
}
