#pragma once

// Computes enemy jumper pose transitions in and around water.

// User-authorized presentation compatibility: a settled floating jumper may
// show its standing loop. This is not a recovered native jumper branch.
struct JumperWaterPoseState {
    float stableAge;
    int standing;
    const Object *source;
    const b2Body *body;
    const b2Body *water;
};

JumperWaterPoseState jumperWaterPose[MAX_ENTITIES];
// Measured original water fixtures oscillate up to 4.17 px/s while already
// floating. Separate visual entry/hold limits avoid flickering at that noise.
const float jumperWaterPoseEnterSpeed = 6 * b2_linearSleepTolerance;
const float jumperWaterPoseHoldSpeed = 12 * b2_linearSleepTolerance;

void resetJumperWaterPose() {
    memset(jumperWaterPose, 0, sizeof(jumperWaterPose));
}

int jumperWaterPoseIndex(const Entity &e) {
    ULONG_PTR address = (ULONG_PTR)&e, begin = (ULONG_PTR)entities;
    if (address < begin || address - begin >= sizeof(Entity) * (ULONG_PTR)entityCount ||
        (address - begin) % sizeof(Entity))
        return -1;
    return (int)((address - begin) / sizeof(Entity));
}

bool jumperWaterPoseContact(int index, const b2Body **waterResult = 0,
                            float speedLimit = jumperWaterPoseEnterSpeed) {
    if (index < 0 || index >= entityCount)
        return false;
    Entity &e = entities[index];
    b2Body *body = bodies[index];
    if (e.state.gone || e.kind != ENEMY || !e.source->has("EnemyJumperBehaviour") || !body ||
        !body->IsActive() || body->GetType() != b2_dynamicBody)
        return false;
    for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
        b2Contact *contact = edge->contact;
        if (!contact->IsTouching())
            continue;
        int water = (int)edge->other->GetUserData() - 1;
        if (water < 0 || water >= entityCount || entities[water].kind != WATER ||
            bodies[water] != edge->other || !edge->other->IsActive())
            continue;
        b2Fixture *waterFixture = contact->GetFixtureA()->GetBody() == edge->other
                                      ? contact->GetFixtureA()
                                      : contact->GetFixtureB();
        if (!waterFixture->IsSensor())
            continue;
        b2Vec2 relative = body->GetLinearVelocity() - edge->other->GetLinearVelocity();
        if (relative.LengthSquared() >= speedLimit * speedLimit)
            continue;
        float area = 0, total = 0, surface = entityRect(entities[water]).t * metres;
        for (b2Fixture *fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
            if (!fixture->IsSensor()) {
                b2Vec2 centre;
                b2MassData mass;
                fixture->GetShape()->ComputeMass(&mass, 1);
                total += mass.mass;
                area += submergedFixture(*fixture, surface, centre);
            }
        if (area > b2_epsilon && area < total - b2_epsilon) {
            if (waterResult)
                *waterResult = edge->other;
            return true;
        }
    }
    return false;
}

void advanceJumperWaterPose(float dt) {
    if (screen != PLAY || dt <= 0)
        return;
    for (int i = 0; i < entityCount; i++) {
        JumperWaterPoseState &pose = jumperWaterPose[i];
        const b2Body *water = 0;
        bool holding = pose.standing && pose.source == entities[i].source && pose.body == bodies[i];
        if (!jumperWaterPoseContact(
                i, &water, holding ? jumperWaterPoseHoldSpeed : jumperWaterPoseEnterSpeed)) {
            pose = JumperWaterPoseState{};
            continue;
        }
        if (pose.source != entities[i].source || pose.body != bodies[i] || pose.water != water) {
            pose = JumperWaterPoseState{};
            if (!jumperWaterPoseContact(i, &water))
                continue;
        }
        pose.source = entities[i].source;
        pose.body = bodies[i];
        pose.water = water;
        pose.stableAge = minf(b2_timeToSleep, pose.stableAge + dt);
        pose.standing = pose.stableAge >= b2_timeToSleep;
    }
}

bool jumperWaterStanding(const Entity &e) {
    int index = jumperWaterPoseIndex(e);
    if (index < 0)
        return false;
    const JumperWaterPoseState &pose = jumperWaterPose[index];
    const b2Body *water = 0;
    return pose.standing && pose.source == e.source && pose.body == bodies[index] &&
           jumperWaterPoseContact(index, &water, jumperWaterPoseHoldSpeed) && pose.water == water;
}
