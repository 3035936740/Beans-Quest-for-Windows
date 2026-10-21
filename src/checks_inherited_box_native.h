#pragma once

// Regression coverage for inherited box native. Runs only in the isolated BQ_ENABLE_TESTS executable.

class InheritedBoxImpactNative : public b2ContactListener {
  public:
    int solved;
    float restitution;

    InheritedBoxImpactNative() : solved(0), restitution(-10) {}

    void PostSolve(b2Contact *contact, const b2ContactImpulse *) {
        solved++;
        restitution = contact->GetRestitution();
    }
};

float inheritedBoxImpactNative(Entity block, Entity bean, float dt, bool reference,
                               bool ordinaryPeer, int &solved, float &restitution) {
    b2World *world = new b2World(b2Vec2(0, 0), true);
    InheritedBoxImpactNative observer;
    world->SetContactListener(&observer);
    block.state.x = 256;
    block.state.y = 128;
    block.state.gone = 0;
    block.state.vx = block.state.vy = 0;
    bean.state.x = 256;
    bean.state.y = block.state.y + block.hit.t - bean.hit.b + .25f;
    bean.state.vx = 0;
    bean.state.vy = -200;
    b2Body *target = createPhysicsEntity(*world, block, 0);
    target->SetActive(true);
    if (reference)
        target->GetFixtureList()->SetRestitution(.5f);
    b2Body *player = createPhysicsEntity(*world, bean, 1);
    player->SetGravityScale(0);
    if (ordinaryPeer)
        player->GetFixtureList()->SetRestitution(0);
    world->Step(dt, 10, 10);
    float result = player->GetLinearVelocity().y / metres;
    solved = observer.solved;
    restitution = observer.restitution;
    delete world;
    return result;
}

bool inheritedBoxAuthoredNative(int &count) {
    count = 0;
    bool result = true;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].touch) {
            Entity &e = entities[i];
            b2Body *body = bodies[i];
            b2Fixture *f = body ? body->GetFixtureList() : 0;
            count++;
            result &= circleFixtureDefaultMaterialNative(body) &&
                      body->GetType() == b2_kinematicBody && body->IsActive() == activeBlock(e) &&
                      f->GetFilterData().categoryBits == 4 &&
                      f->GetFilterData().maskBits == 0xffff && !f->IsSensor();
        }
    return result;
}

void inheritedBoxNativeChecks() {
    int target = touchAlternatorNativeFixture(false), count = 0;
    bool authored = inheritedBoxAuthoredNative(count);
    check(
        target >= 0 && count == 10 && authored,
        "All ten authored5-2 touch alternators retain inherited Box density2.5/restitution0.5/friction0.2 with unchanged kinematic bodies and filters");
    if (target < 0)
        return;
    alternateScene(2);
    bool toggled = inheritedBoxAuthoredNative(count);
    check(
        count == 10 && toggled,
        "Both initially on and initially off touch blocks retain native material after immediate alternation changes their actual physics membership");
    Entity block = entities[target], bean = entities[playerIndex];
    bool impacts = true, tom = true;
    char rows[2200] =
        "controlledOrdinaryPeer,fps,actualVy1000,referenceVy1000,actualMixedRestitution1000,actualSolved,referenceSolved\r\n";
    for (int peer = 0; peer < 2; peer++)
        for (int rate = 0; rate < 3; rate++) {
            int fps = rate == 0 ? 30 : rate == 1 ? 120 : 240, actualSolved = 0, referenceSolved = 0;
            float actualRestitution, referenceRestitution;
            float actual = inheritedBoxImpactNative(block, bean, 1.0f / fps, false, peer != 0,
                                                    actualSolved, actualRestitution),
                  reference = inheritedBoxImpactNative(block, bean, 1.0f / fps, true, peer != 0,
                                                       referenceSolved, referenceRestitution);
            bool valid = actualSolved > 0 && referenceSolved > 0 &&
                         absf(actual - (peer ? 100 : 0)) < .01f &&
                         absf(reference - (peer ? 100 : 0)) < .01f &&
                         absf(actualRestitution - (peer ? .5f : 0)) < .0001f &&
                         absf(actual - reference) < .01f;
            if (peer)
                impacts &= valid;
            else
                tom &= valid;
            char row[220];
            wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d\r\n", peer, fps, (int)(actual * 1000),
                      (int)(reference * 1000), (int)(actualRestitution * 1000), actualSolved,
                      referenceSolved);
            lstrcatA(rows, row);
        }
    check(
        impacts,
        "A controlled ordinary restitution0 peer against actual touch geometry rebounds from200px/s to100px/s at30/120/240Hz under native material mixing; no authored peer placement is implied");
    check(
        tom,
        "Actual unmodified Tom retains its native minus-one restitution sentinel and zero solver rebound at30/120/240Hz before ordinary-jump callbacks");
    Object variant = *block.source;
    Pair changes[] = {{(char *)"Alternation", (char *)"1"},
                      {(char *)"AlternationMask", (char *)"2"},
                      {(char *)"density", (char *)"4"},
                      {(char *)"restitution", (char *)"0.75"},
                      {(char *)"friction", (char *)"0.1"}};
    Behavior behavior = {0, (char *)"AlternatorTouchBlockBehaviour", Props{5, changes}};
    variant.behaviorCount = 1;
    variant.behaviors = &behavior;
    Entity explicitBlock;
    initializeEntity(explicitBlock, variant, variant.rect);
    b2World *world = new b2World(b2Vec2(0, 0), true);
    b2Body *b = createPhysicsEntity(*world, explicitBlock, 0);
    b2Fixture *f = b ? b->GetFixtureList() : 0;
    check(
        f && absf(f->GetDensity() - 4) < .001f && absf(f->GetRestitution() - .75f) < .001f &&
            absf(f->GetFriction() - .1f) < .001f,
        "Inherited touch material honors explicit serialized overrides rather than forcing native defaults");
    delete world;
    const u64 ids[] = {14815823507569313204ULL, 17963251161137489403ULL, 4105260327524925064ULL,
                       271445704564135871ULL, 670936219271ULL};
    bool controls = true;
    for (int i = 0; i < 5; i++) {
        Object &source = *classById(ids[i]);
        Entity e;
        Rect r = source.rect;
        if (source.base == 7) {
            r.r = minf(r.r, r.l + source.props.num("actlet-width", 32));
            r.t = minf(r.t, r.b + source.props.num("actlet-height", 32));
        }
        initializeEntity(e, source, r);
        world = new b2World(b2Vec2(0, 0), true);
        b = createPhysicsEntity(*world, e, 0);
        f = b ? b->GetFixtureList() : 0;
        controls &= f &&
                    absf(f->GetDensity() - (i == 0   ? 2.5f
                                            : i == 1 ? 3
                                                     : 0)) < .001f &&
                    absf(f->GetRestitution() - (i < 2 ? .5f : 0)) < .001f &&
                    absf(f->GetFriction() - .2f) < .001f;
        delete world;
    }
    check(
        controls,
        "Crumbling and authored density3 Seesaw retain inherited materials while Spring/Trampoline/Magnet BoxGroups retain their distinct zero-density zero-restitution material");
    target = touchAlternatorNativeFixture(false);
    captureCheckpoint(false);
    char path[MAX_PATH];
    savePath(path, "inherited-box-checkpoint.dat");
    bool disk = atomicWrite(path, &checkpoint, sizeof(checkpoint));
    rebuildPhysics();
    disk &= readCheckpoint(path);
    if (disk)
        restoreCheckpoint();
    bool rebuilt = inheritedBoxAuthoredNative(count);
    check(
        disk && rebuilt && count == 10 && checkpoint.version == 28 && sizeof(Snapshot) == 543392,
        "A real disk checkpoint plus world rebuild reconstructs all ten touch fixtures with native material without adding snapshot fields");
    UiScene scene = {};
    scene.memory.init(1024 * 1024);
    scene.playfield = scene.memory.array<Entity>(2);
    scene.playfieldCount = 1;
    scene.hasBean = true;
    scene.playfield[0] = entities[target];
    scene.bean = entities[playerIndex];
    createUiPhysics(scene);
    bool ui = circleFixtureDefaultMaterialNative(scene.bodies[0]);
    destroyUiPhysics(scene);
    VirtualFree(scene.memory.data, 0, MEM_RELEASE);
    check(
        ui,
        "The independent UI-world factory uses the same inherited material for a controlled original touch class; no authored selector placement is implied");
    atomicWrite("test-output/inherited-box-impact.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/inherited-box-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
