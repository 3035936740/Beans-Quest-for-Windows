#pragma once

// Regression coverage for projectile ccd native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Original CirclePhysics bodyDef+0x27 is zero. Keep Tom's separately enabled
// bullet flag: a non-bullet shot still sweeps against static/kinematic or
// bullet peers. These probes distinguish that branch from blanket CCD removal.
struct ProjectileCcdProbeNative : public b2ContactListener {
    int solved;

    ProjectileCcdProbeNative() : solved(0) {}

    void PostSolve(b2Contact *, const b2ContactImpulse *) {
        solved++;
    }
};

int projectileCcdImpactFrameNative(bool credits, int targetMode, int forcedBullet, float dt) {
    Object &source = *classById(6430600862785566ULL);
    Object &box = *classById(2913132245228912724ULL);
    Entity crate;
    initializeEntity(crate, box, shifted(box.rect, 400, 200));
    float x = 400 + crate.hit.l - 6 - .68f;
    float speed = source.behavior("EnemyBulletBehaviour")->num("speed", 125);
    b2Body *shot = 0, *target = 0;
    int result = 0;
    if (credits) {
        UiScene scene = {};
        scene.physics = new b2World(b2Vec2(0, 0), true);
        ProjectileCcdProbeNative listener;
        scene.physics->SetContactListener(&listener);
        Entity projectile;
        initializeEntity(projectile, source, source.rect);
        projectile.state.x = x;
        projectile.state.y = 200;
        shot = creditsCreateShotBody(scene, projectile, 0);
        target = createPhysicsEntity(*scene.physics, crate, 0);
        target->SetGravityScale(0);
        target->SetType(targetMode == 1   ? b2_staticBody
                        : targetMode == 2 ? b2_kinematicBody
                                          : b2_dynamicBody);
        if (targetMode == 3)
            target->SetBullet(true);
        if (forcedBullet >= 0)
            shot->SetBullet(forcedBullet != 0);
        shot->SetLinearVelocity(b2Vec2(speed * metres, 0));
        for (int frame = 1; frame <= 3; frame++) {
            scene.physics->Step(dt, 10, 10);
            if (listener.solved > 0) {
                result = frame;
                break;
            }
        }
        delete scene.physics;
    } else {
        killableProjectileFixture(source.id, x, 200, speed, 0);
        int index = entityCount++;
        entities[index] = crate;
        restorePhysics();
        if (!bodies[index])
            bodies[index] = createPhysicsEntity(*physicsWorld, entities[index], index);
        shot = shotBodies[0];
        target = bodies[index];
        target->SetGravityScale(0);
        target->SetType(targetMode == 1   ? b2_staticBody
                        : targetMode == 2 ? b2_kinematicBody
                                          : b2_dynamicBody);
        if (targetMode == 3)
            target->SetBullet(true);
        if (forcedBullet >= 0)
            shot->SetBullet(forcedBullet != 0);
        shotContacts[0].first = false;
        shotContacts[0].hit = shotContacts[0].hitBean = false;
        for (int frame = 1; frame <= 3; frame++) {
            physicsWorld->Step(dt, 10, 10);
            if (shotContacts[0].hit) {
                result = frame;
                break;
            }
        }
    }
    return result;
}

void projectileCcdNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    const u64 ids[] = {6430600862785566ULL, 12978618438667112518ULL, 537747635566ULL};
    bool flags = true, player = true, disk = true;
    for (int i = 0; i < 3; i++) {
        killableProjectileFixture(ids[i], 280, 200, 0, 0);
        restorePhysics();
        flags &= shotBodies[0] && !shotBodies[0]->IsBullet();
        player &= bodies[playerIndex] && bodies[playerIndex]->IsBullet();
        Entity projectile;
        Object &source = *classById(ids[i]);
        initializeEntity(projectile, source, source.rect);
        UiScene scene = {};
        scene.physics = new b2World(b2Vec2(0, 0), true);
        b2Body *body = creditsCreateShotBody(scene, projectile, 0);
        flags &= body && !body->IsBullet();
        delete scene.physics;
        shotContacts[0].first = false;
        captureCheckpoint(false);
        const char *path = "test-output/projectile-ccd-checkpoint.dat";
        bool written = atomicWrite(path, &checkpoint, sizeof(checkpoint));
        physicsWorld->DestroyBody(shotBodies[0]);
        shotBodies[0] = 0;
        bool loaded = written && readCheckpoint(path);
        if (loaded)
            restoreCheckpoint();
        disk &= loaded && shotBodies[0] && !shotBodies[0]->IsBullet() && !shotContacts[0].first;
    }
    check(
        flags && player,
        "All3original projectile classes retain native non-bullet flags in both factories while Tom keeps his separately enabled bullet flag");
    bool timing[2] = {true, true};
    int rows = 0;
    char trace[4096] = "factory,fps,target,nativeDiscreteFrame,ccdControlFrame,factoryFrame\r\n";
    for (int factory = 0; factory < 2; factory++)
        for (int fps = 30; fps <= 120; fps *= 2)
            for (int target = 0; target < 4; target++) {
                float dt = 1.0f / fps;
                int reference = projectileCcdImpactFrameNative(factory != 0, target, 0, dt),
                    control = projectileCcdImpactFrameNative(factory != 0, target, 1, dt),
                    actual = projectileCcdImpactFrameNative(factory != 0, target, -1, dt);
                timing[factory] &= reference == ((target != 0 && fps < 120) ? 1 : 2) &&
                                   control == (fps < 120 ? 1 : 2) && actual == reference;
                char row[100];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d\r\n", factory, fps, target, reference, control,
                          actual);
                lstrcatA(trace, row);
                rows++;
            }
    check(
        timing[0],
        "Campaign fireball at original125px/s matches native dynamic-crate contact-frame timing at30/60/120Hz, retaining continuous contacts against static/kinematic/bullet peers");
    check(
        timing[1],
        "Ending projectile factory matches the same real fireball/crate contact timing and all admitted continuous-collision peer controls");
    check(
        disk,
        "Disk reload reconstructs all3projectile classes with native non-bullet flags while preserving saved first-tick contact state");
    char summary[100];
    wsprintfA(summary, "rows=%d flags=%d player=%d campaign=%d credits=%d disk=%d\r\n", rows, flags,
              player, timing[0], timing[1], disk);
    atomicWrite("test-output/projectile-ccd-summary.txt", summary, lstrlenA(summary));
    atomicWrite("test-output/projectile-ccd-timing.csv", trace, lstrlenA(trace));
    atomicWrite("test-output/projectile-ccd-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
