#pragma once

// Records solved support fixtures for jumping creatures and clears them when contacts end.

// EnemyJumper::PostSolve 0x1c51a4 inserts an upward-solved other fixture;
// EndContact 0x1c000c removes it. A disabled/changed-normal contact alone does
// not erase this set. These transient pointers are rebuilt, never saved.
struct JumperGroundFixture {
    b2Fixture *fixture;
    JumperGroundFixture *next;
};

JumperGroundFixture *jumperGroundFixtures[MAX_ENTITIES];

bool jumperNativeActor(int index) {
    return index >= 0 && index < entityCount && entities[index].source->has("EnemyJumperBehaviour");
}

bool jumperNativeGrounded(int index) {
    return index >= 0 && index < MAX_ENTITIES && jumperGroundFixtures[index] != 0;
}

void clearJumperGroundNative(JumperGroundFixture **heads, int count) {
    for (int i = 0; i < count; i++)
        while (heads[i]) {
            JumperGroundFixture *entry = heads[i];
            heads[i] = entry->next;
            delete entry;
        }
}

void jumperNativeClearGround() {
    clearJumperGroundNative(jumperGroundFixtures, MAX_ENTITIES);
}

void endJumperGroundNative(b2Contact *contact, b2Body **ownedBodies, JumperGroundFixture **heads,
                           int count) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int index = (int)fixtures[side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= count || ownedBodies[index] != fixtures[side]->GetBody())
            continue;
        JumperGroundFixture **cursor = heads + index;
        while (*cursor) {
            JumperGroundFixture *entry = *cursor;
            if (entry->fixture == fixtures[1 - side]) {
                *cursor = entry->next;
                delete entry;
                break;
            }
            cursor = &entry->next;
        }
    }
}

void jumperNativeEndContact(b2Contact *contact) {
    endJumperGroundNative(contact, bodies, jumperGroundFixtures, MAX_ENTITIES);
}

void recordJumperGroundNative(b2Contact *contact, Entity *actors, int count, b2Body **ownedBodies,
                              JumperGroundFixture **heads) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    b2WorldManifold manifold;
    contact->GetWorldManifold(&manifold);
    for (int side = 0; side < 2; side++) {
        int index = (int)fixtures[side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= count || !actors[index].source->has("EnemyJumperBehaviour") ||
            ownedBodies[index] != fixtures[side]->GetBody() ||
            (side == 0 ? -manifold.normal.y : manifold.normal.y) <= .95f)
            continue;
        bool present = false;
        for (JumperGroundFixture *entry = heads[index]; entry; entry = entry->next)
            if (entry->fixture == fixtures[1 - side])
                present = true;
        if (!present) {
            JumperGroundFixture *entry = new JumperGroundFixture;
            entry->fixture = fixtures[1 - side];
            entry->next = heads[index];
            heads[index] = entry;
        }
    }
}

void jumperNativePostSolve(b2Contact *contact) {
    recordJumperGroundNative(contact, entities, entityCount, bodies, jumperGroundFixtures);
}

// PrePhysics 0x1c4574 compares the old clock before advancing it. A launch
// keeps that clock until EndContact/no-ground resets ordinary mode. Vertical
// speed is not a grounded predicate, including during a ball's upward shove.
void jumperNativeStepState(Entity &e, float dt, float delay, bool special, bool grounded,
                           float targetX) {
    State &s = e.state;
    if (grounded) {
        if (s.aux > delay) {
            const Props &jump = *e.source->behavior("EnemyJumperBehaviour");
            s.vy = root(-2 * nativeWorldGravity * jump.num("jump-height", 60));
            s.vx = (targetX < s.x ? -1.0f : 1.0f) * jump.num("jump-x-velocity", 100);
            s.hp = 1;
            selectJumperAnimationNative(e, true);
        } else {
            s.hp = 0;
            selectJumperAnimationNative(e, false);
        }
        s.aux += dt;
    } else if (special) {
        s.aux += dt;
        if (s.aux > delay)
            s.aux -= delay;
    } else
        s.aux = 0;
}

void jumperNativeStep(int index, float dt, float delay, bool special) {
    jumperNativeStepState(entities[index], dt, delay, special, jumperNativeGrounded(index),
                          entities[playerIndex].state.x);
}
