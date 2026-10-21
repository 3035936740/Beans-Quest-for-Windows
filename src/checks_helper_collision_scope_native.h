#pragma once

// Regression coverage for helper collision scope native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int helperAnimationCampaignPrepareNative(u64 stage);

class HelperCollisionScopeObserverNative : public OriginalContacts {
  public:
    int bird, target, begin, pre, disabled, solved;
    b2Body *targetBody;

    HelperCollisionScopeObserverNative(int h, int t)
        : bird(h), target(t), begin(0), pre(0), disabled(0), solved(0), targetBody(0) {}

    bool pair(b2Contact *c) {
        b2Body *against = targetBody ? targetBody : bodies[target];
        return c->GetFixtureA()->GetBody() == bodies[bird] &&
                   c->GetFixtureB()->GetBody() == against ||
               c->GetFixtureB()->GetBody() == bodies[bird] &&
                   c->GetFixtureA()->GetBody() == against;
    }

    void BeginContact(b2Contact *c) {
        if (pair(c))
            begin++;
        OriginalContacts::BeginContact(c);
    }

    void PreSolve(b2Contact *c, const b2Manifold *old) {
        OriginalContacts::PreSolve(c, old);
        if (pair(c)) {
            pre++;
            disabled += !c->IsEnabled();
        }
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        if (pair(c))
            solved++;
        OriginalContacts::PostSolve(c, impulse);
    }
};

int helperCollisionScopeBirdVoicesNative() {
    AudioClip *clip = 0;
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, "bird"))
            clip = &soundClips[i].clip;
    int n = 0;
    for (int i = 0; i < 32; i++)
        n += clip && audioVoices[i].clip == clip;
    return n;
}

void helperCollisionScopeMatrixNativeChecks(int mode = 1) {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 350000);
    lstrcpyA(
        rows,
        "class_id,fps,direction,kind,target_sensor,filter_pair,oneway,expected_stop,actual_stop,bird_voices,begin,pre,disabled,solved\r\n");
    Object targetCopy = {};
    int classRows = 0, runtimeClasses = 0, cases = 0, expectedStops = 0, passes = 0,
        failuresBefore = failures;
    for (int c = 0; c < classCount; c++) {
        Entity prototype;
        initializeEntity(prototype, classes[c], classes[c].rect);
        bool candidate = prototype.physics || prototype.kind == COIN || prototype.kind == SECRET ||
                         prototype.kind == FORCE || prototype.kind == WATER;
        if (!candidate || prototype.kind == PLAYER || prototype.kind == HELPER)
            continue;
        bool contacts = true, audio = true, represented = false;
        int classCases = 0;
        for (int rate = 0; rate < 3; rate++)
            for (int direction = 0; direction < 4; direction++) {
                int bird = helperAnimationCampaignPrepareNative(641851699177ULL);
                if (bird < 0) {
                    contacts = audio = false;
                    continue;
                }
                readyTimer = 0;
                targetCopy = classes[c];
                Vec origin = center(targetCopy.rect);
                targetCopy.rect = shifted(targetCopy.rect, 500 - origin.x, 400 - origin.y);
                Rect firstActlet = targetCopy.rect;
                if (boxGroupMatrix(prototype)) {
                    firstActlet.r = minf(firstActlet.r,
                                         firstActlet.l + targetCopy.props.num("actlet-width", 32));
                    firstActlet.t = minf(firstActlet.t,
                                         firstActlet.b + targetCopy.props.num("actlet-height", 32));
                }
                int first = entityCount;
                addEntity(targetCopy, firstActlet);
                if (first == entityCount)
                    continue;
                Entity &target = entities[first];
                target.state.gone = 0;
                target.state.active = 1;
                target.state.vx = target.state.vy = 0;
                if (target.crumble)
                    target.state.active = 0;
                if (target.kind == SPIKE && !spikeGroupEntityOn(target))
                    alternationBits ^= target.mask;
                if (target.kind == ALTERNATOR && !activeBlock(target))
                    alternationBits ^= target.mask;
                Entity &helper = entities[bird];
                helper.state.active = mode;
                helper.state.timer = .2f;
                helper.state.vx = helper.state.vy = 0;
                State &bean = entities[playerIndex].state;
                bean.x = 1500;
                bean.y = 1500;
                bean.vx = bean.vy = 0;
                rebuildPhysics();
                int targetIndex = -1;
                for (int i = first; i < entityCount; i++)
                    if (bodies[i]) {
                        targetIndex = i;
                        break;
                    }
                if (targetIndex < 0)
                    continue;
                represented = true;
                Entity &actor = entities[targetIndex];
                b2Body *body = bodies[targetIndex];
                b2Fixture *main = body->GetFixtureList();
                for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext())
                    if (!f->IsSensor() && f->GetUserData() != (void *)15) {
                        main = f;
                        break;
                    }
                if (!main)
                    continue;
                b2AABB aabb;
                main->GetShape()->ComputeAABB(&aabb, body->GetTransform(), 0);
                float x = (aabb.lowerBound.x + aabb.upperBound.x) * .5f / metres,
                      y = (aabb.lowerBound.y + aabb.upperBound.y) * .5f / metres;
                if (direction == 0)
                    x = aabb.upperBound.x / metres - helper.hit.l - .25f;
                if (direction == 1)
                    x = aabb.lowerBound.x / metres - helper.hit.r + .25f;
                if (direction == 2)
                    y = aabb.lowerBound.y / metres - helper.hit.t + .25f;
                if (direction == 3)
                    y = aabb.upperBound.y / metres - helper.hit.b - .25f;
                helper.state.x = x;
                helper.state.y = y;
                bodies[bird]->SetTransform(b2Vec2(x * metres, y * metres), 0);
                bodies[bird]->SetLinearVelocity(b2Vec2(0, 0));
                bodies[playerIndex]->SetType(b2_staticBody);
                for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext()) {
                    b2Filter mask = f->GetFilterData();
                    mask.maskBits = 0;
                    f->SetFilterData(mask);
                }
                b2Fixture *birdMain = bodies[bird]->GetFixtureList();
                b2Filter a = main->GetFilterData(), b = birdMain->GetFilterData();
                bool filterPair = (a.maskBits & b.categoryBits) && (b.maskBits & a.categoryBits) &&
                                  body->IsActive();
                bool oneWay = main->GetUserData() == (void *)1 || main->GetUserData() == (void *)2;
                bool deadlySensor = actor.kind == SPIKE &&
                                    actor.source->has("SpikeGroupBehaviour") &&
                                    spikeGroupEntityOn(actor);
                bool authoredFilter =
                    body->IsActive() &&
                    (actor.kind == BOUNCER || !actor.physics ||
                     actor.physics->flag("collide-other", true)) &&
                    (!helper.physics || helper.physics->flag("collide-other", true));
                if (actor.physics)
                    contacts &= filterPair == authoredFilter;
                bool expected = mode == 1 && authoredFilter &&
                                (!main->IsSensor() || deadlySensor) && (!oneWay || direction == 3);
                int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240;
                HelperCollisionScopeObserverNative observer(bird, targetIndex);
                physicsWorld->SetContactListener(&observer);
                stopSounds();
                soundEnabled = true;
                simulateBodies(1.0f / fps);
                helperPostStep(1.0f / fps);
                int voices = helperCollisionScopeBirdVoicesNative();
                bool stopped = mode == 1 && helper.state.active == 2;
                contacts &= (mode == 1 ? stopped == expected
                                       : helper.state.active == mode && observer.solved == 0);
                audio &= voices == (expected ? 1 : 0);
                int previousVoices = voices;
                helperPostStep(0);
                audio &= helperCollisionScopeBirdVoicesNative() == previousVoices;
                soundEnabled = false;
                char row[300];
                wsprintfA(row, "%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", classes[c].id,
                          fps, direction, actor.kind, main->IsSensor(), filterPair, oneWay,
                          expected, stopped, voices, observer.begin, observer.pre,
                          observer.disabled, observer.solved);
                if (lstrlenA(rows) < 349000)
                    lstrcatA(rows, row);
                cases++;
                classCases++;
                expectedStops += expected;
                passes += !expected;
                physicsWorld->SetContactListener(originalContacts);
                stopSounds();
            }
        if (represented) {
            runtimeClasses++;
            char label[250];
            wsprintfA(
                label,
                "Original helper mode%d collider class%I64u preserves accepted hits, sensors, authored masks and one-way face rules across%dphysical cases",
                mode, classes[c].id, classCases);
            check(contacts && classCases == 12, label);
            wsprintfA(
                label,
                "Original helper mode%d collider class%I64u emits exactly one impact voice only for an accepted stop and never repeats on return",
                mode, classes[c].id);
            check(audio && classCases == 12, label);
        }
        classRows++;
    }
    char matrixPath[120];
    if (mode == 1)
        lstrcpyA(matrixPath, "test-output/helper-collision-scope-matrix.csv");
    else
        wsprintfA(matrixPath, "test-output/helper-nonflight-mode%d-matrix.csv", mode);
    atomicWrite(matrixPath, rows, lstrlenA(rows));
    char summary[400];
    wsprintfA(
        summary,
        "all_classes=%d candidates=%d runtime_classes=%d cases=%d expected_stops=%d expected_passes=%d new_failures=%d\r\n",
        classCount, classRows, runtimeClasses, cases, expectedStops, passes,
        failures - failuresBefore);
    char summaryPath[120];
    if (mode == 1)
        lstrcpyA(summaryPath, "test-output/helper-collision-scope-summary.txt");
    else
        wsprintfA(summaryPath, "test-output/helper-nonflight-mode%d-summary.txt", mode);
    atomicWrite(summaryPath, summary, lstrlenA(summary));
    check(
        runtimeClasses > 50 && cases == runtimeClasses * 12,
        "Expanded helper matrix executes real callbacks for every represented physics class; player/helper collection is covered separately");
    HeapFree(GetProcessHeap(), 0, rows);
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(0);
}

void helperCollisionScopeInventoryNativeChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = debugMode = false;
    startStage(0);
    readyTimer = 0;
    char *rows = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 400000);
    lstrcpyA(
        rows,
        "class_id,base,kind,width1000,height1000,physics,sensor,mask,killable,deadly,kill_enemies,spike_group,oneway,enemy_bullet,boss_bullet,orbit,behaviors\r\n");
    int total = 0, physical = 0, hostile = 0, oneway = 0, projectiles = 0;
    Entity *entity = new Entity;
    for (int i = 0; i < classCount; i++) {
        Object &o = classes[i];
        initializeEntity(*entity, o, o.rect);
        const Props *physics = entity->physics;
        char types[1800] = "";
        for (int b = 0; b < o.behaviorCount; b++) {
            if (b)
                lstrcatA(types, "|");
            if (lstrlenA(types) + lstrlenA(o.behaviors[b].type) < 1750)
                lstrcatA(types, o.behaviors[b].type);
        }
        const Props *deadly = o.behavior("DeadlyBehaviour");
        char row[2500];
        wsprintfA(
            row, "%I64u,%I64u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s\r\n", o.id, o.base,
            entity->kind, (int)((o.rect.r - o.rect.l) * 1000), (int)((o.rect.t - o.rect.b) * 1000),
            physics != 0, physics && physics->flag("sensor"), nativePhysicsMask(physics),
            o.has("KillableEnemyBehaviour"), deadly != 0, deadly && deadly->flag("kill-enemies"),
            o.has("SpikeGroupBehaviour"), o.has("PlatformBehaviour") || entity->crumble,
            o.has("EnemyBulletBehaviour"), o.has("FinalBossBulletBehaviour"),
            o.has("OrbitingObjectBehaviour") || entity->orbit != 0, types);
        if (lstrlenA(rows) + lstrlenA(row) < 399000)
            lstrcatA(rows, row);
        else {
            check(false, "Helper collision inventory output capacity covers all original classes");
            break;
        }
        total++;
        physical += physics != 0;
        hostile += deadly != 0 || o.has("KillableEnemyBehaviour") || o.has("SpikeGroupBehaviour");
        oneway += o.has("PlatformBehaviour") || entity->crumble;
        projectiles += o.has("EnemyBulletBehaviour") || o.has("FinalBossBulletBehaviour");
    }
    atomicWrite("test-output/helper-collision-class-inventory.csv", rows, lstrlenA(rows));
    char summary[300];
    wsprintfA(summary, "classes=%d physical=%d hostile=%d oneway=%d projectiles=%d\r\n", total,
              physical, hostile, oneway, projectiles);
    atomicWrite("test-output/helper-collision-inventory-summary.txt", summary, lstrlenA(summary));
    check(
        total == classCount && total > 0 && physical > 0 && hostile > 0 && oneway > 0 &&
            projectiles > 0,
        "All original class definitions are inventoried before the expanded helper collision matrix; counts are not runtime coverage");
    HeapFree(GetProcessHeap(), 0, rows);
    delete entity;
    startStage(0);
}
