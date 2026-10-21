#pragma once

// Regression coverage for walk callback native. Runs only in the isolated BQ_ENABLE_TESTS executable.

// Interpret the native TrackingDetector Begin/End membership and Walk
// PostSolve branches against actual Windows solver events. This is a local
// contact-order comparison, not execution of the Android library.
class WalkCallbackProbeNative : public OriginalContacts {
  public:
    int target, used, eligible, changes;
    bool right;
    b2Contact *tracked[128];
    int tags[128];
    b2Fixture *others[128];

    WalkCallbackProbeNative(int i, bool facing)
        : target(i), used(0), eligible(0), changes(0), right(facing) {}

    int ownSide(b2Contact *c) {
        if ((int)c->GetFixtureA()->GetBody()->GetUserData() - 1 == target)
            return 0;
        if ((int)c->GetFixtureB()->GetBody()->GetUserData() - 1 == target)
            return 1;
        return -1;
    }

    bool contains(int tag, b2Fixture *other = 0) {
        for (int i = 0; i < used; i++)
            if (tracked[i] && tags[i] == tag && (!other || others[i] == other))
                return true;
        return false;
    }

    void direction(bool facing) {
        if (right != facing)
            changes++;
        right = facing;
    }

    void BeginContact(b2Contact *c) {
        int side = ownSide(c);
        if (side >= 0) {
            b2Fixture *own = side ? c->GetFixtureB() : c->GetFixtureA();
            b2Fixture *other = side ? c->GetFixtureA() : c->GetFixtureB();
            int tag = (int)own->GetUserData();
            if (tag >= 3 && tag <= 6 && !other->IsSensor() && used < 128) {
                tracked[used] = c;
                tags[used] = tag;
                others[used++] = other;
            }
        }
        OriginalContacts::BeginContact(c);
    }

    void EndContact(b2Contact *c) {
        for (int i = 0; i < used; i++)
            if (tracked[i] == c)
                tracked[i] = 0;
        OriginalContacts::EndContact(c);
    }

    void PostSolve(b2Contact *c, const b2ContactImpulse *impulse) {
        OriginalContacts::PostSolve(c, impulse);
        int side = ownSide(c);
        if (side < 0 || !c->IsEnabled() || !c->GetManifold()->pointCount)
            return;
        b2Fixture *own = side ? c->GetFixtureB() : c->GetFixtureA();
        b2Fixture *other = side ? c->GetFixtureA() : c->GetFixtureB();
        if (own->IsSensor() || other->IsSensor())
            return;
        if (contains(3, other)) {
            direction(true);
            eligible++;
        }
        if (contains(4, other)) {
            direction(false);
            eligible++;
        }
    }

    void postPhysics() {
        Entity &e = entities[target];
        if (e.reverseEdge && e.walkCanWalk) {
            bool left = contains(5), r = contains(6);
            if (left && !r)
                direction(false);
            else if (r && !left)
                direction(true);
        }
    }
};

void walkCallbackNativeChecks() {
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/walk-callback-saves");
    CreateDirectoryA(saveRoot, 0);
    bool order = true, controls = true;
    int mismatches = 0, eligible = 0;
    char rows[16000] =
        "layout,order,step,callbacks,referenceRight,actualRight,referenceChanges,left,right\r\n";
    for (int layout = 0; layout < 4; layout++)
        for (int ordering = 0; ordering < 2; ordering++) {
            int target = prepareWalkPermissionNative(2, 0);
            for (int i = 0; i < entityCount; i++)
                if (i != target && i != playerIndex)
                    entities[i].state.gone = 1;
            current.geometryCount = 0;
            Entity &e = entities[target];
            e.state.x = 500;
            e.state.y = 100;
            e.state.vx = e.state.vy = 0;
            e.walkCanWalk = 0;
            e.reverseEdge = 0;
            nativeWalkDirection(e, false, true);
            rebuildPhysics();
            bodies[playerIndex]->SetType(b2_staticBody);
            b2BodyDef bd;
            bd.position.Set(500 * metres, 60 * metres);
            b2Body *floor = physicsWorld->CreateBody(&bd);
            b2PolygonShape shape;
            shape.SetAsBox(200 * metres, 10 * metres);
            floor->CreateFixture(&shape, 0);
            for (int n = 0; n < 2; n++) {
                int side = ordering ? 1 - n : n;
                if (layout == 3 || layout == 1 && side == 1 || layout == 2 && side == 0)
                    continue;
                bd.position.Set((500 + (side ? 22 : -22)) * metres, 100 * metres);
                b2Body *wall = physicsWorld->CreateBody(&bd);
                shape.SetAsBox(10 * metres, 60 * metres);
                wall->CreateFixture(&shape, 0);
            }
            WalkCallbackProbeNative *probe = new WalkCallbackProbeNative(target, e.walkRight != 0);
            physicsWorld->SetContactListener(probe);
            for (int step = 0; step < 12; step++) {
                tick(1 / 120.0f, false, false);
                probe->postPhysics();
                bool match = probe->right == (e.walkRight != 0);
                if (layout == 0) {
                    order &= match;
                    mismatches += !match;
                } else
                    controls &= match;
                char row[180];
                wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n", layout, ordering, step,
                          probe->eligible, probe->right, e.walkRight, probe->changes,
                          probe->contains(3), probe->contains(4));
                lstrcatA(rows, row);
            }
            eligible += probe->eligible;
            physicsWorld->SetContactListener(originalContacts);
            delete probe;
        }
    check(
        controls,
        "Native callback reference agrees with actual single-wall and floor-only walking direction controls");
    check(
        order && eligible > 0,
        "Both wall-creation orders preserve native last-PostSolve steering when an actual walker touches two walls in one step");
    atomicWrite("test-output/walk-callback-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/walk-callback-results.txt", testLog, lstrlenA(testLog));
    lstrcpyA(saveRoot, oldRoot);
    startStage(0);
    readyTimer = 0;
}
