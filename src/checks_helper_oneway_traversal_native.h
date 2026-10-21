#pragma once

// Regression coverage for helper oneway traversal native. Runs only in the isolated BQ_ENABLE_TESTS executable.
void helperOneWayTraversalNativeChecks() {
    const u64 classesToCheck[] = {14815823507569313204ULL, 6181314796472045350ULL,
                                  14843448166857854273ULL};
    Object sourceCopy = {};
    bool all = true;
    char trace[10000] =
        "type,fps,reload,flying,crossed,begin,pre,disabled,solved,voices,reloaded,start1000,end1000,high1000\r\n";
    for (int type = 0; type < 6; type++)
        for (int rate = 0; rate < 3; rate++)
            for (int reload = 0; reload < 2; reload++) {
                int bird = helperAnimationCampaignPrepareNative(641851699177ULL);
                if (bird < 0) {
                    all = false;
                    continue;
                }
                Entity &helper = entities[bird];
                State &bean = entities[playerIndex].state;
                helper.state.active = 1;
                helper.state.timer = 1;
                helper.state.vx = helper.state.vy = 0;
                int target = -1;
                b2Body *platform = 0;
                float low = 400, high = 408;
                float moving = type == 1 ? 12.0f : type == 4 ? -12.0f : 0;
                if (type < 3 || type == 4) {
                    sourceCopy = *classById(classesToCheck[type == 4 ? 1 : type]);
                    Vec origin = center(sourceCopy.rect);
                    sourceCopy.rect = shifted(sourceCopy.rect, 500 - origin.x, 400 - origin.y);
                    target = entityCount;
                    addEntity(sourceCopy, sourceCopy.rect);
                    entities[target].state.active = entities[target].crumble ? 0 : 1;
                    entities[target].state.vx = 0;
                    entities[target].state.vy = moving;
                }
                rebuildPhysics();
                if (target >= 0) {
                    platform = bodies[target];
                    b2Fixture *f = platform->GetFixtureList();
                    b2AABB a;
                    f->GetShape()->ComputeAABB(&a, platform->GetTransform(), 0);
                    low = a.lowerBound.y / metres;
                    high = a.upperBound.y / metres;
                } else {
                    b2BodyDef body;
                    platform = physicsWorld->CreateBody(&body);
                    b2PolygonShape shape;
                    if (type == 5) {
                        terrainOneWayPolygon(shape, Vec{452, 384}, Vec{548, 416});
                        low = 376;
                        high = 416;
                    } else
                        terrainOneWayPolygon(shape, Vec{452, 408}, Vec{548, 408});
                    b2FixtureDef fixture;
                    fixture.shape = &shape;
                    fixture.userData = (void *)1;
                    platform->CreateFixture(&fixture);
                }
                float start = low - helper.hit.t - 5;
                helper.state.x = 500;
                helper.state.y = start;
                bean.x = 500;
                bean.y = start -
                         helper.source->behavior("HelperBirdBehaviour")->num("follow-offset-y", 34);
                bean.vx = bean.vy = 0;
                bodies[bird]->SetTransform(b2Vec2(500 * metres, start * metres), 0);
                bodies[bird]->SetLinearVelocity(b2Vec2(0, 0));
                bodies[playerIndex]->SetType(b2_staticBody);
                for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext()) {
                    b2Filter filter = f->GetFilterData();
                    filter.maskBits = 0;
                    f->SetFilterData(filter);
                }
                HelperCollisionScopeObserverNative observer(bird, target);
                observer.targetBody = platform;
                physicsWorld->SetContactListener(&observer);
                int fps = rate == 0   ? 30
                          : rate == 1 ? 120
                                      : 240,
                    frames = (int)(fps * (high - low + helper.hit.t - helper.hit.b + 40) /
                                   (60 - absf(moving))) +
                             5;
                float dt = 1.0f / fps;
                bool flying = true, didReload = false;
                stopSounds();
                soundEnabled = true;
                for (int step = 0; step < frames; step++) {
                    bean.y += 60 * dt;
                    if (target >= 0 && moving) {
                        entities[target].state.y += moving * dt;
                        entities[target].state.vy = moving;
                    }
                    simulateBodies(dt);
                    helperPostStep(dt);
                    flying &= helper.state.active == 1;
                    if (reload && !didReload && observer.disabled > 0) {
                        captureCheckpoint(false);
                        restoreCheckpoint();
                        didReload = true;
                    }
                }
                if (target >= 0) {
                    b2AABB finalAabb;
                    platform->GetFixtureList()->GetShape()->ComputeAABB(
                        &finalAabb, platform->GetTransform(), 0);
                    high = finalAabb.upperBound.y / metres;
                }
                int voices = helperCollisionScopeBirdVoicesNative();
                bool crossed = helper.state.y + helper.hit.b > high + 2;
                char label[260];
                wsprintfA(
                    label,
                    "Helper traverses original one-way kind%d from below at%dHz%s with real refused contacts, no impact voice and no return",
                    type, fps, reload ? " across checkpoint restore" : "");
                bool ok = flying && crossed && observer.begin > 0 && observer.disabled > 0 &&
                          observer.solved == 0 && voices == 0 && (!reload || didReload);
                check(ok, label);
                all &= ok;
                char row[250];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", type, fps, reload,
                          flying, crossed, observer.begin, observer.pre, observer.disabled,
                          observer.solved, voices, didReload, (int)(start * 1000),
                          (int)(helper.state.y * 1000), (int)(high * 1000));
                lstrcatA(trace, row);
                physicsWorld->SetContactListener(originalContacts);
                soundEnabled = false;
                stopSounds();
            }
    atomicWrite("test-output/helper-oneway-traversal-trace.csv", trace, lstrlenA(trace));
    check(
        all,
        "All36one-way traversal and restore combinations cover original cloud fixtures, both moving directions, horizontal and sloped terrain without helper impact");
    startStage(0);
    soundEnabled = musicEnabled = false;
}
