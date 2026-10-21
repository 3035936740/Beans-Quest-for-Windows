#pragma once

// Regression coverage for killable contact native. Runs only in the isolated BQ_ENABLE_TESTS executable.
#include "checks_killable_projectile_native.h"

void killableContactNativeChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    Vec points[] = {{0, 88}, {10, 89}};
    check(
        nativeKillablePlayerStomp(points, 2, 100, 10, 22, -1, 0),
        "Native player stomp accepts every contact point at or above the instance's vulnerable strip boundary");
    points[1].y = 87.99f;
    check(
        !nativeKillablePlayerStomp(points, 2, 100, 10, 22, -100, 0),
        "A single low manifold point rejects a stomp even when the other point lies above the vulnerable strip");
    points[1].y = 89;
    check(
        nativeKillablePlayerStomp(points, 2, 100, 10, 22, 20, 30) &&
            nativeKillablePlayerStomp(points, 2, 100, 10, 22, -20, -30),
        "Native player stomp accepts upward relative approach as well as downward absolute movement");
    check(
        !nativeKillablePlayerStomp(points, 2, 100, 10, 22, 0, 0) &&
            !nativeKillablePlayerStomp(points, 2, 100, 10, 22, 20, 20),
        "Stationary and equal upward velocities do not satisfy the native strict falling condition");
    check(
        nativeKillableMomentum(2, Vec{60, 80}, 200, true) &&
            !nativeKillableMomentum(2, Vec{60, 79}, 200, true),
        "Native non-player enemy kill accepts the exact linear-momentum threshold and rejects a weaker impact");
    check(
        nativeKillableMomentum(1, Vec{-200, 0}, 200, true) &&
            nativeKillableMomentum(1, Vec{0, 200}, 200, true),
        "Native momentum kill uses absolute speed in every direction rather than only a downward stomp");
    check(!nativeKillableMomentum(100, Vec{0, -500}, 200, false),
          "Static and kinematic objects cannot qualify for the native dynamic-body impact branch");
    const float expectedMass = 0.5f * b2_pi * 0.5f * 0.5f;
    bool massCorrect = true, fastKilled = false, slowBounced = false, cornerSurvived = false,
         walkSensorsExercised = false;
    for (int scenario = 0; scenario < 3; scenario++) {
        startStage(0);
        readyTimer = deathTimer = 0;
        current.geometryCount = 0;
        clearEffects();
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex)
                entities[i].state.gone = 1;
        int enemyIndex = entityCount;
        addEntity(*classById(14264481342587988014ULL), Rect{240, 112, 272, 144});
        Entity &enemy = entities[enemyIndex];
        if (scenario)
            enemy.walk = enemy.dynamic = 0;
        enemy.state.active = 1;
        enemy.state.vx = enemy.state.vy = 0;
        float ballX = scenario == 2 ? 298.0f : 256.0f,
              ballY = scenario == 2 ? 170.0f : hitRect(enemy).t + 32 - 0.1f;
        int ballIndex = entityCount;
        addEntity(*classById(14441582509727370349ULL),
                  Rect{ballX - 32, ballY - 32, ballX + 32, ballY + 32});
        entities[ballIndex].state.vy = scenario == 1 ? -80.0f : -600.0f;
        entities[playerIndex].state.x = 48;
        entities[playerIndex].state.y = 280;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        rebuildPhysics();
        massCorrect &=
            bodies[ballIndex] && absf(bodies[ballIndex]->GetMass() - expectedMass) < 0.0001f;
        if (!scenario)
            walkSensorsExercised = enemy.walk && bodies[enemyIndex] &&
                                   bodies[enemyIndex]->GetFixtureList()->IsSensor();
        tick(1 / 120.0f, false, false);
        if (scenario == 0) {
            bool deferred = killableDeathVisibleNative(enemy) && !bodies[enemyIndex]->IsActive() &&
                            liveEffects() == 0;
            for (int n = 0; n < 14; n++)
                tick(1 / 120.0f, false, false);
            fastKilled = deferred && enemy.state.gone == 1 && liveEffects() > 0 &&
                         entities[ballIndex].state.vy < -590;
        }
        if (scenario == 1)
            slowBounced = !enemy.state.gone && entities[ballIndex].state.vy > 0;
        if (scenario == 2)
            cornerSurvived = !enemy.state.gone;
    }
    check(massCorrect,
          "Original 64px BeachBall circle and density produce the native 0.392699 Box2D mass");
    check(
        fastKilled,
        "Actual high-momentum BeachBall contact disables the original beetle immediately and emits its native death effect only after the squash transition");
    check(
        walkSensorsExercised,
        "Impact kill exercises the actual walking beetle with its later sensor fixtures preceding the original main fixture");
    check(
        slowBounced,
        "Actual low-momentum BeachBall bounces from the beetle without triggering an invented overlap kill");
    check(
        cornerSurvived,
        "Fast ball overlapping only the rectangular sprite bounds cannot kill without a real circle contact");
    const u64 birdIds[] = {8253104759903472ULL, 8171782927099924ULL, 1882918009912166ULL};
    bool originalBirdsKilled = true, overrideUsed = true, originalDrop = true;
    for (int target = 0; target < 3; target++) {
        // Keep this level's original terrain, oscillator/proxy setup, bird
        // instances and Boulder, including the per-instance 100 threshold.
        startStage(stageById(6816877730789680ULL));
        readyTimer = deathTimer = 0;
        int bird = -1, ball = -1;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].source->id == birdIds[target])
                bird = i;
            if (entities[i].source->id == 4599895978911819ULL)
                ball = i;
        }
        if (bird < 0 || ball < 0) {
            originalBirdsKilled = overrideUsed = originalDrop = false;
            continue;
        }
        Entity &birdActor = entities[bird];
        Entity &boulder = entities[ball];
        float threshold =
            birdActor.source->behavior("KillableEnemyBehaviour")->num("vulnerable-momentum");
        float expectedBoulderMass = 2 * b2_pi * (31 * metres) * (31 * metres),
              requiredSpeed = threshold / expectedBoulderMass;
        originalDrop &=
            boulder.originY - 31 - 320 > requiredSpeed * requiredSpeed / 1000 && requiredSpeed < 68;
        overrideUsed &= threshold == 100 &&
                        nativeKillableMomentum(expectedBoulderMass, Vec{120, 0}, threshold, true) &&
                        !nativeKillableMomentum(expectedBoulderMass, Vec{120, 0}, 200, true);
        boulder.state.x = hitRect(birdActor).l - 31 + 0.1f;
        boulder.state.y = birdActor.state.y;
        boulder.state.vx = 120;
        boulder.state.vy = 0;
        entities[playerIndex].state.x = 64;
        entities[playerIndex].state.y = 750;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        restorePhysics();
        for (int step = 0; step < 4 && !birdActor.state.gone; step++)
            tick(1 / 120.0f, false, false);
        originalBirdsKilled &= birdActor.state.gone && !bodies[bird]->IsActive() &&
                               (!oscillatorProxies[bird] || !oscillatorProxies[bird]->IsActive());
    }
    check(
        overrideUsed,
        "Real displayed 2-07 birds use their authored 100 momentum override instead of the enemy class's 200 default");
    check(
        originalDrop,
        "Original 2-07 density-two Boulder needs under six pixels of free fall and its authored ledge provides enough height");
    check(
        originalBirdsKilled,
        "Real 2-07 Boulder contacts kill all three authored blue birds while retaining the actual stage terrain and oscillator fixtures");
    bool relativeHit = false, upperSideHit = false, lowerSideKilled = false, stompsFree = true,
         stompSounds = false, nextLandingCounts = false, ordinaryJumpCap = false;
    bool savedSound = soundEnabled;
    for (int scenario = 0; scenario < 3; scenario++) {
        startStage(0);
        readyTimer = deathTimer = 0;
        current.geometryCount = 0;
        clearEffects();
        for (int i = 0; i < entityCount; i++)
            if (i != playerIndex)
                entities[i].state.gone = 1;
        int enemyIndex = entityCount;
        addEntity(*classById(14264481342587988014ULL), Rect{240, 112, 272, 144});
        Entity &enemy = entities[enemyIndex];
        enemy.walk = 0;
        enemy.dynamic = scenario == 0 ? 1 : 0;
        enemy.state.active = 1;
        enemy.state.vx = 0;
        enemy.state.vy = scenario == 0 ? 120.0f : 0;
        Entity &bean = entities[playerIndex];
        bean.state.x = scenario == 0 ? 256.0f : 276.9f;
        bean.state.y = scenario == 0   ? hitRect(enemy).t - bean.hit.b + 0.1f
                       : scenario == 1 ? 128.0f
                                       : 124.0f;
        bean.state.vx = scenario == 0 ? 0 : -30.0f;
        bean.state.vy = scenario == 0 ? 60.0f : -60.0f;
        bean.state.timer = .2f;
        rebuildPhysics();
        int before = jumps;
        if (scenario == 1) {
            stopSounds();
            soundEnabled = true;
        }
        tick(1 / 120.0f, false, false);
        if (scenario < 2)
            tick(1 / 120.0f, false, false);
        if (scenario == 0)
            relativeHit = enemy.state.gone && deathTimer <= 0 && bean.state.vy > 200;
        if (scenario == 1) {
            upperSideHit = enemy.state.gone && deathTimer <= 0 && bean.state.vy > 200;
            stompSounds = nativeStompSoundPair();
            soundEnabled = savedSound;
            stopSounds();
        }
        if (scenario == 2)
            lowerSideKilled = !enemy.state.gone && deathTimer > 0;
        else
            stompsFree &= jumps == before;
        if (scenario == 1) {
            int support = entityCount;
            addEntity(*classById(14843448166857854273ULL), Rect{208, 80, 304, 96});
            bean.state.x = 256;
            bean.state.y = hitRect(entities[support]).t - bean.hit.b + 0.1f;
            bean.state.vx = 0;
            bean.state.vy = -60;
            bean.state.timer = .2f;
            rebuildPhysics();
            for (int step = 0; step < 12 && !ordinaryJumpLaunchedNative; step++)
                tick(1 / 120.0f, false, false);
            nextLandingCounts = jumps == before + 1 && bean.state.vy > 0;
            jumps = 999;
            bean.state.x = 256;
            bean.state.y = hitRect(entities[support]).t - bean.hit.b + 0.1f;
            bean.state.vx = 0;
            bean.state.vy = -60;
            bean.state.timer = .2f;
            restorePhysics();
            for (int step = 0; step < 12 && !ordinaryJumpLaunchedNative; step++)
                tick(1 / 120.0f, false, false);
            ordinaryJumpCap = jumps == 999 && bean.state.vy > 0;
        }
    }
    soundEnabled = savedSound;
    check(
        relativeHit,
        "Actual rising enemy can be stomped by a more slowly rising Bean through native relative velocity");
    check(
        upperSideHit,
        "Actual upper-side contact inside the authored 22px vulnerable strip stomps without the old bottom-minus-ten shortcut");
    check(
        lowerSideKilled,
        "Actual contact with a low manifold point kills Bean instead of accepting a bounding-box stomp");
    check(
        stompsFree && nextLandingCounts,
        "Enemy-induced bounces do not add jumps while the next ordinary platform landing counts normally");
    check(
        ordinaryJumpCap,
        "Actual ordinary landing still launches Bean while the original jump counter remains capped at 999");
    check(stompSounds,
          "Actual enemy stomp plays both original enemyexplosion and Bean jump sounds");
    killableProjectileNativeChecks();
    startStage(0);
}
