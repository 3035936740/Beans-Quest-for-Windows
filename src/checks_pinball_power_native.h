#pragma once

// Regression coverage for pinball power native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class PinballPowerBonusObserverNative : public OriginalContacts {
  public:
    float bonus;

    PinballPowerBonusObserverNative() : bonus(0) {}

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        nativePinballPreSolve(c, entities, entityCount, playerIndex, pinballPlayerStepNative,
                              bonus);
    }
};

char pinballPowerProbeRowsNative[6000];

struct PinballPowerObservationNative {
    bool contact, launched;
    float speed;
    int jumps;
};

PinballPowerObservationNative pinballPowerProbeNative(const char *power, bool ui, float bonus = 0,
                                                      bool disk = false) {
    PinballPowerObservationNative result = {};
    int pin = preparePinballFixtureNative(31.75f);
    if (pin < 0)
        return result;
    Object source = *entities[pin].source;
    Pair pairs[] = {{(char *)"physics-power", (char *)"75"}, {(char *)"jump-power", (char *)power}};
    Behavior behavior = {0, (char *)"PinballBouncerBehaviour", Props{power ? 2 : 1, pairs}};
    source.behaviorCount = 1;
    source.behaviors = &behavior;
    entities[pin].source = &source;
    entities[pin].physics = &behavior.props;
    rebuildPhysics();
    float dt = 1 / 240.0f;
    if (!ui) {
        PinballPowerBonusObserverNative observer;
        observer.bonus = bonus;
        if (bonus)
            physicsWorld->SetContactListener(&observer);
        tick(dt, false, false);
        physicsWorld->SetContactListener(originalContacts);
        result.contact = pinballPendingNative.pending && pinballPendingNative.source == pin;
        result.speed = pinballPendingNative.speed;
        if (disk) {
            captureCheckpoint(false);
            char path[MAX_PATH];
            savePath(path, "pinball-power-checkpoint.dat");
            bool saved = atomicWrite(path, &checkpoint, sizeof(checkpoint));
            rebuildPhysics();
            saved &= readCheckpoint(path);
            if (saved)
                restoreCheckpoint();
            result.contact &= saved && pinballPendingNative.pending &&
                              pinballPendingNative.speed == result.speed &&
                              checkpoint.version == 28 && sizeof(Snapshot) == 543392;
        }
        tick(dt, false, false);
        result.launched = pinballLaunchedThisStepNative;
        result.jumps = jumps;
    } else {
        navigate(137502285691ULL);
        UiScene local = {};
        local.memory.init(1024 * 1024);
        local.playfield = local.memory.array<Entity>(2);
        local.playfieldCount = 1;
        local.hasBean = true;
        local.playfield[0] = entities[pin];
        local.bean = page.bean;
        Entity &pad = local.playfield[0];
        local.bean.state.x = pad.state.x - 31.75f - local.bean.hit.r;
        local.bean.state.y = pad.state.y - (local.bean.hit.b + local.bean.hit.t) * .5f;
        local.bean.state.vx = local.bean.state.vy = 0;
        local.bean.state.timer = .2f;
        createUiPhysics(local);
        stepUiPhysics(local, dt);
        result.contact = local.specialJump.mega.pending && local.specialJump.mega.source == 0;
        result.speed = local.specialJump.mega.speed;
        stepUiPhysics(local, dt);
        result.launched =
            local.specialJump.launchedKind == 1 && local.specialJump.launchedSource == 0;
        result.jumps = local.specialJump.mega.pending;
        destroyUiPhysics(local);
        VirtualFree(local.memory.data, 0, MEM_RELEASE);
    }
    char row[240];
    wsprintfA(row, "%s,%d,%d,%d,%d,%d,%d\r\n", power ? power : "absent", ui, (int)bonus, disk,
              result.contact, result.launched, (int)(result.speed * 1000));
    lstrcatA(pinballPowerProbeRowsNative, row);
    startStage(0);
    readyTimer = 0;
    return result;
}

void pinballPowerNativeChecks() {
    lstrcpyA(pinballPowerProbeRowsNative, "power,ui,bonus,disk,contact,launched,speed1000\r\n");
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int count = 0, jumpException = 0, physicsException = 0;
    bool explicitValues = true;
    for (int i = 0; i < levelCount; i++) {
        readLevelData(levels[i].file, scene, memory);
        for (int j = 0; j < scene.objectCount; j++) {
            const Props *p = scene.objects[j].behavior("PinballBouncerBehaviour");
            if (!p)
                continue;
            count++;
            jumpException += p->num("jump-power") == 170;
            physicsException += p->num("physics-power") == 50;
            explicitValues &= *p->get("jump-power") &&
                              (p->num("jump-power") == 130 || p->num("jump-power") == 170) &&
                              *p->get("physics-power") &&
                              (p->num("physics-power") == 75 || p->num("physics-power") == 50);
        }
    }
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    check(
        count == 19 && jumpException == 1 && physicsException == 1 && explicitValues,
        "All19authored Pinballs retain their explicit powers, including the single jump-power170 and physics-power50 exceptions, so absent-property corrections do not retune authored routes");
    PinballPowerObservationNative campaign = pinballPowerProbeNative(0, false),
                                  ui = pinballPowerProbeNative(0, true);
    check(
        campaign.contact && campaign.launched && campaign.jumps == 1 &&
            absf(campaign.speed - 400) < .001f,
        "A controlled original campaign Pinball missing jump-power uses native200height and queues/launches400px/s with one counted MegaJump");
    check(
        ui.contact && ui.launched && absf(ui.speed - 400) < .001f,
        "A controlled original ending-world Pinball missing jump-power uses the same native200height with its actual minus400gravity and independent MegaJump request");
    const char *overrides[] = {"130", "170", "250", "0"};
    bool authored = true;
    for (int mode = 0; mode < 4; mode++)
        for (int miniature = 0; miniature < 2; miniature++) {
            PinballPowerObservationNative r =
                pinballPowerProbeNative(overrides[mode], miniature != 0);
            float height = mode == 0 ? 130 : mode == 1 ? 170 : mode == 2 ? 250 : 74;
            float expected = root(800 * height);
            authored &= r.contact && r.launched && absf(r.speed - expected) < .002f;
        }
    check(
        authored,
        "Real campaign and independent UI contacts preserve explicit130/170/250/zero powers and Tom's basic-height lower bound rather than replacing authored values with the default");
    PinballPowerObservationNative lower = pinballPowerProbeNative(0, false, 120),
                                  higher = pinballPowerProbeNative(0, false, 210);
    check(
        lower.contact && lower.launched && absf(lower.speed - 400) < .001f && higher.contact &&
            higher.launched && absf(higher.speed - root(800 * 284)) < .002f,
        "Actual controlled contact callbacks compare the native200fallback with basic-plus-bonus height:194keeps200 while284wins");
    PinballPowerObservationNative restored = pinballPowerProbeNative(0, false, 0, true);
    check(
        restored.contact && restored.launched && restored.jumps == 1 &&
            absf(restored.speed - 400) < .001f,
        "A private disk checkpoint captured after a real missing-power contact retains the correct400px/s pending launch through world reconstruction without changing version28 bytes");
    int target = -1, pin = preparePinballDynamicNative(target);
    bool ordinary = false;
    if (pin >= 0) {
        Object source = *entities[pin].source;
        Props empty = {};
        Behavior behavior = {0, (char *)"PinballBouncerBehaviour", empty};
        source.behaviorCount = 1;
        source.behaviors = &behavior;
        entities[pin].source = &source;
        entities[pin].physics = &behavior.props;
        rebuildPhysics();
        bodies[target]->SetGravityScale(0);
        PinballDynamicObserverNative observer;
        observer.pin = pin;
        observer.target = target;
        physicsWorld->SetContactListener(&observer);
        simulateBodies(1 / 120.0f);
        float x = 0, y = 0;
        bool expected = pinballDynamicExpectedNative(observer, x, y);
        mechanismsPostStep(1 / 120.0f);
        ordinary = expected && absf(entities[target].state.vx - x) < .03f &&
                   absf(entities[target].state.vy - y) < .03f;
        physicsWorld->SetContactListener(originalContacts);
        startStage(0);
        readyTimer = 0;
    }
    check(
        ordinary,
        "The existing ordinary-body callback independently retains native missing physics-power200 instead of confusing it with authored75 or Tom's separate jump-power");
    atomicWrite("test-output/pinball-power-probes.csv", pinballPowerProbeRowsNative,
                lstrlenA(pinballPowerProbeRowsNative));
    atomicWrite("test-output/pinball-power-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
