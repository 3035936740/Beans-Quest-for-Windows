#pragma once

// Detects user-requested crush deaths from solved opposing contacts and queues them outside the physics callback.

// User-requested crush death for killable creatures. This is a port rule,
// not an extra branch attributed to the original EnemyJumper callbacks.
// Only real solved, non-sensor contacts can contribute. Keep the queue in
// its owning world; no transient contact or pressure state enters a save.
struct CreaturePressureContact {
    b2Body *other;
    Vec normal;
    float closing;
    bool fixed;
};

template <int N> struct CreaturePressureState {
    CreaturePressureContact contacts[N][8];
    u8 count[N];
};

template <int N> void resetCreaturePressure(CreaturePressureState<N> &state) {
    memset(state.count, 0, sizeof(state.count));
}

template <int N>
void creaturePressurePostSolve(b2Contact *contact, const b2ContactImpulse *impulse, Entity *actors,
                               int count, b2Body **mainBodies, CreaturePressureState<N> &pressure,
                               bool *pending) {
    b2Fixture *f[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (!impulse || !contact->IsEnabled() || !contact->GetManifold()->pointCount ||
        f[0]->IsSensor() || f[1]->IsSensor())
        return;
    bool force = false;
    for (int i = 0; i < impulse->count; i++)
        force |= impulse->normalImpulses[i] > 0;
    if (!force)
        return;
    b2WorldManifold manifold;
    contact->GetWorldManifold(&manifold);
    for (int side = 0; side < 2; side++) {
        b2Body *own = f[side]->GetBody();
        int index = (int)own->GetUserData() - 1;
        if (index < 0 || index >= count || index >= N || mainBodies[index] != own ||
            own->GetType() != b2_dynamicBody || actors[index].kind != ENEMY ||
            actors[index].state.gone || !actors[index].source->has("KillableEnemyBehaviour"))
            continue;
        b2Body *other = f[1 - side]->GetBody();
        // Kinematic velocity is not changed by solver impulses. Orbit and
        // attached controllers may leave actor velocity fields at zero.
        b2Vec2 velocity = other->GetLinearVelocity();
        bool fixed = other->GetType() == b2_staticBody ||
                     (other->GetType() == b2_kinematicBody &&
                      velocity.LengthSquared() <= b2_epsilon * b2_epsilon);
        Vec normal = {side ? manifold.normal.x : -manifold.normal.x,
                      side ? manifold.normal.y : -manifold.normal.y};
        float closing = velocity.x * normal.x + velocity.y * normal.y;
        // Finite-mass creatures landing on each other must keep ordinary
        // stacking and restitution; a driven kinematic carrier cannot yield.
        if (!fixed && (other->GetType() != b2_kinematicBody || closing <= b2_epsilon))
            continue;
        int slot = 0;
        for (; slot < pressure.count[index]; slot++) {
            CreaturePressureContact &previous = pressure.contacts[index][slot];
            if (previous.other == other &&
                previous.normal.x * normal.x + previous.normal.y * normal.y > .99f)
                break;
        }
        if (slot >= 8)
            continue;
        if (slot == pressure.count[index])
            pressure.count[index]++;
        pressure.contacts[index][slot] = CreaturePressureContact{other, normal, closing, fixed};
        for (int i = 0; i < pressure.count[index]; i++)
            for (int j = i + 1; j < pressure.count[index]; j++) {
                const CreaturePressureContact &a = pressure.contacts[index][i];
                const CreaturePressureContact &b = pressure.contacts[index][j];
                if (a.other != b.other &&
                    a.normal.x * b.normal.x + a.normal.y * b.normal.y < -.95f &&
                    ((a.fixed && !b.fixed && b.closing > b2_epsilon) ||
                     (b.fixed && !a.fixed && a.closing > b2_epsilon)))
                    pending[index] = true;
            }
    }
}
