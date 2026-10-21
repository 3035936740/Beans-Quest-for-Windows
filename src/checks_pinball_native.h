#pragma once

// Regression coverage for pinball native. Runs only in the isolated BQ_ENABLE_TESTS executable.

struct PinballDetailImpactNative {
    bool contact;
    float nx, ny, vx, vy, solverVx, speed;
    int jumps, jumpVoices, pinVoices;
};

int pinballDetailVoiceCount(const char *name) {
    int count = 0;
    for (int clip = 0; clip < soundClipCount; clip++)
        if (eq(soundClips[clip].name, name))
            for (int voice = 0; voice < 32; voice++)
                if (audioVoices[voice].clip == &soundClips[clip].clip)
                    count++;
    return count;
}

int preparePinballDetailNative(int mode, bool debug = false) {
    startStage(stageById(191561700160ULL));
    readyTimer = deathTimer = 0;
    readyOpening.showing = false;
    current.geometryCount = 0;
    clearEffects();
    int pin = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOUNCER) {
            pin = i;
            break;
        }
    for (int i = 0; i < entityCount; i++)
        if (i != playerIndex && i != pin)
            entities[i].state.gone = 1;
    if (pin < 0)
        return -1;
    Entity &bean = entities[playerIndex];
    Entity &pad = entities[pin];
    Rect r = shifted(Rect{-32, -32, 32, 32}, pad.state.x, pad.state.y);
    bean.state.hp = 1;
    bean.state.timer = .1f;
    bean.state.vx = mode == 0 ? 60.0f : 0;
    bean.state.vy = mode == 0 ? 0 : -20.0f;
    bean.state.x = mode == 0 ? r.l - bean.hit.r + .2f : pad.state.x + (mode == 2 ? 12.0f : 0);
    bean.state.y = mode == 0 ? pad.state.y : r.t - bean.hit.b - .2f;
    debugMode = debug;
    jumps = 0;
    rebuildPhysics();
    stopSounds();
    return pin;
}

PinballDetailImpactNative runPinballDetailNative(int mode, bool debug = false) {
    PinballDetailImpactNative result = {};
    int pin = preparePinballDetailNative(mode, debug);
    if (pin < 0)
        return result;
    for (int frame = 0; frame < 12; frame++) {
        float previousVx = entities[playerIndex].state.vx;
        tick(1.0f / 120, false, false);
        if (!pinballLaunchedThisStepNative || pinballConsumedNative.source != pin)
            continue;
        Entity &bean = entities[playerIndex];
        result.contact = true;
        result.nx = pinballConsumedNative.normal.x;
        result.ny = pinballConsumedNative.normal.y;
        result.speed = pinballConsumedNative.speed;
        result.vx = bean.state.vx;
        result.vy = bean.state.vy;
        result.solverVx =
            approach(previousVx, 0, bean.physics->num("stop-acceleration", 200) / 120);
        result.jumps = jumps;
        result.jumpVoices = pinballDetailVoiceCount("jump");
        result.pinVoices = pinballDetailVoiceCount("pinball");
        break;
    }
    char row[420], file[160];
    wsprintfA(
        row,
        "scene=191561700160 mode=%d debug=%d actualContact=%d normal10000=%d,%d vx100=%d vy100=%d solverVx100=%d speed100=%d jumps=%d ordinaryJumpVoices=%d pinballVoices=%d\r\n",
        mode, debug, result.contact, (int)(result.nx * 10000), (int)(result.ny * 10000),
        (int)(result.vx * 100), (int)(result.vy * 100), (int)(result.solverVx * 100),
        (int)(result.speed * 100), result.jumps, result.jumpVoices, result.pinVoices);
    wsprintfA(file, "test-output/pinball-detail-impact-%d-%d.txt", mode, debug);
    atomicWrite(file, row, lstrlenA(row));
    return result;
}

// The original static fixture remains touching for two solver steps before
// the deferred velocity separates it: each real PreSolve supplies pinball.
void pinballNativeChecks() {
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = false;
    musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    check(
        absf(nativePinballJumpSpeed(-400, 74, 0, 130) - root(104000)) < .001f &&
            absf(nativePinballJumpSpeed(-200, 37, 120, 130) - root(62800)) < .001f,
        "Pinball MegaJump derives speed from original gravity and the maximum of authored power and Tom's basic plus bonus height");
    Vec normalized = nativePinballNormal(Vec{3, 4});
    Vec tiny = nativePinballNormal(Vec{0, 0});
    check(absf(normalized.x - .6f) < .00001f && absf(normalized.y - .8f) < .00001f && tiny.x == 0 &&
              tiny.y == 0,
          "Pinball MegaJump normalizes directions with the native Box2D epsilon guard");
    PinballDetailImpactNative side = runPinballDetailNative(0);
    check(
        side.contact && side.nx < -.99f && absf(side.ny) < .01f && side.jumps == 1 &&
            side.jumpVoices == 0 && side.pinVoices == 2 &&
            absf(side.vx - (side.solverVx + side.speed * side.nx)) < .02f,
        "Original Pinball side contact counts one MegaJump, adds horizontal normal velocity, and plays only pinball");
    PinballDetailImpactNative top = runPinballDetailNative(1);
    check(
        top.contact && top.ny > .99f && top.jumps == 1 && top.jumpVoices == 0 &&
            top.pinVoices == 2 && absf(top.vy - (top.speed * top.ny - 400.0f / 120)) < .02f,
        "Original Pinball top contact takes MegaJump precedence over the ordinary jump and avoids its extra sound");
    PinballDetailImpactNative offset = runPinballDetailNative(2);
    check(
        offset.contact && offset.nx > .01f && offset.nx < .2f && offset.ny > .98f &&
            offset.jumps == 1 && offset.jumpVoices == 0 && offset.pinVoices == 2 &&
            absf(offset.vx - (offset.solverVx + offset.speed * offset.nx)) < .02f &&
            absf(offset.vy - (offset.speed * offset.ny - 400.0f / 120)) < .02f,
        "Offset landing on the original Pinball fixture launches along its real contact normal rather than the sprite-centre radial approximation");
    PinballDetailImpactNative debug = runPinballDetailNative(0, true);
    check(debug.contact && debug.jumps == 1 && debug.jumpVoices == 0 && debug.pinVoices == 2 &&
              debug.vx < -300,
          "Session unlock mode retains Pinball jump counting, launch and dedicated sound");
    int pin = preparePinballDetailNative(0);
    bool overlapOnly = false;
    if (pin >= 0) {
        Entity &bean = entities[playerIndex];
        Rect r = hitRect(entities[pin]);
        bean.state.x = r.l - bean.hit.r + 1;
        bean.state.y = r.t - bean.hit.b - 1;
        bean.state.vx = bean.state.vy = 0;
        restorePhysics();
        bool visualOverlap = overlaps(hitRect(bean), r);
        tick(1.0f / 120, false, false);
        overlapOnly = visualOverlap && pinballPlayerStepNative.source < 0 &&
                      pinballDetailVoiceCount("pinball") == 0 && jumps == 0;
    }
    check(
        overlapOnly,
        "Pinball sprite AABB overlap at a rounded corner cannot launch Bean without an actual fixture contact");
    PinballPlayerStepNative *temporary = new PinballPlayerStepNative;
    nativePinballReset(*temporary);
    bool filtered = false;
    pin = preparePinballDetailNative(1);
    if (pin >= 0) {
        simulateBodies(1.0f / 120);
        for (b2ContactEdge *edge = bodies[playerIndex]->GetContactList(); edge; edge = edge->next)
            if (edge->other == bodies[pin] && edge->contact->GetManifold()->pointCount) {
                b2Contact *c = edge->contact;
                c->SetEnabled(false);
                nativePinballPreSolve(c, entities, entityCount, playerIndex, *temporary);
                filtered = !temporary->pending;
                c->SetEnabled(true);
                b2Fixture *f = c->GetFixtureA();
                f->SetSensor(true);
                nativePinballPreSolve(c, entities, entityCount, playerIndex, *temporary);
                filtered &= !temporary->pending;
                f->SetSensor(false);
                break;
            }
    }
    check(filtered, "Disabled and sensor Pinball contacts cannot schedule a MegaJump");
    temporary->pending = true;
    temporary->source = pin;
    temporary->normal = Vec{-1, 0};
    temporary->speed = 300;
    int oldJumps = jumps;
    nativePinballReset(*temporary);
    clearPinballPendingNative();
    bool stale =
        nativePinballConsumePending(entities, entityCount, playerIndex, true, true, jumps, .1f);
    check(
        !stale && jumps == oldJumps && temporary->source == -1,
        "Clearing the per-step and queued Pinball requests removes stale launches across world rebuilds and fresh stage initialization");
    delete temporary;
    stopSounds();
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
