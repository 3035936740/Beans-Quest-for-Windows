#pragma once

// Native walker movement and edge decisions, using contact-derived support rather than sprite overlap.

// Walk::PrePhysics0x328ab8 reads the retained permission from PostSolve.
// It clears that flag only after assigning a velocity within +/-walk-speed;
// an outside impulse leaves the permission intact until speed is eligible.
void walkPrePhysicsNative(Entity &e) {
    if (e.walkCanWalk && e.state.vx >= -e.speed && e.state.vx <= e.speed) {
        e.state.vx = e.walkRight ? e.speed : -e.speed;
        e.walkCanWalk = 0;
    }
}

// TrackingDetector retains a non-sensor fixture between Begin and EndContact.
// Its membership is independent of a contact's temporary enabled flag.
bool walkDetectorContainsNative(b2Body *body, int tag, b2Fixture *against = 0) {
    if (!body)
        return false;
    for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (!c->IsTouching())
            continue;
        b2Fixture *own = c->GetFixtureA()->GetBody() == body ? c->GetFixtureA() : c->GetFixtureB();
        b2Fixture *other = own == c->GetFixtureA() ? c->GetFixtureB() : c->GetFixtureA();
        if (own->GetUserData() == (void *)tag && !other->IsSensor() &&
            (!against || against == other))
            return true;
    }
    return false;
}

// Observed-original compatibility: a cloud lip above a nearby spring is a
// step down, not a cliff. Native static Walk branches alone did not reproduce
// the supplied original footage. Use actual support and collision geometry;
// never change authored reverse-at-edge, walking speed or airborne velocity.
class WalkLowerLandingNative : public b2RayCastCallback {
  public:
    b2Fixture *own;
    Entity *actors;
    int count;
    bool spring;
    float fraction;

    WalkLowerLandingNative(b2Fixture *main, Entity *list, int size)
        : own(main), actors(list), count(size), spring(false), fraction(1) {}

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &, const b2Vec2 &normal, float32 hit) {
        if (f->IsSensor() || f->GetBody() == own->GetBody())
            return -1;
        b2ContactFilter filter;
        if (!filter.ShouldCollide(own, f))
            return -1;
        if (hit <= fraction) {
            fraction = hit;
            int i = (int)f->GetBody()->GetUserData() - 1;
            spring = normal.y > .95f && i >= 0 && i < count && actors[i].kind == SPRING &&
                     !actors[i].state.gone;
        }
        return hit;
    }
};

bool walkCloudSpringStepNative(Entity &e, b2Body *body, Entity *actors, int count, bool leftSupport,
                               bool rightSupport) {
    if (!body || !e.walk || !e.reverseEdge || !e.walkCanWalk || !body->IsActive())
        return false;
    int front = e.walkRight ? 6 : 5;
    if (e.walkRight ? (!leftSupport || rightSupport) : (!rightSupport || leftSupport))
        return false;
    b2Fixture *main = 0;
    b2Fixture *foot = 0;
    for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext()) {
        if (!f->IsSensor())
            main = f;
        else if (f->GetUserData() == (void *)front)
            foot = f;
    }
    if (!main || !foot || foot->GetShape()->GetType() != b2Shape::e_polygon)
        return false;
    // CanWalk is the retained solved-support latch. At low frame rates the
    // last manifold may already be disabled by the one-way platform, so use
    // the still-touching rear foot detector for the supporting cloud identity.
    int rear = front == 6 ? 5 : 6;
    float depth = 0;
    for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (!c->IsTouching())
            continue;
        b2Fixture *own = c->GetFixtureA()->GetBody() == body ? c->GetFixtureA() : c->GetFixtureB();
        b2Fixture *other = own == c->GetFixtureA() ? c->GetFixtureB() : c->GetFixtureA();
        if (own->GetUserData() != (void *)rear || other->IsSensor())
            continue;
        int i = (int)other->GetBody()->GetUserData() - 1;
        if (i >= 0 && i < count && actors[i].crumble && actors[i].state.active != 2)
            depth = maxf(depth, (actors[i].hit.t - actors[i].hit.b) * metres);
    }

    if (depth <= 0)
        return false;
    const b2PolygonShape *shape = (const b2PolygonShape *)foot->GetShape();
    b2Vec2 from = body->GetWorldPoint(shape->m_centroid),
           to = from - b2Vec2(0, depth + 2 * b2_polygonRadius + b2_linearSlop);
    WalkLowerLandingNative landing(main, actors, count);
    body->GetWorld()->RayCast(&landing, from, to);
    return landing.spring;
}

// Walk::PostSolve0x32ab14 sets CanWalk on an upward support normal >.95.
// Querying a newly rebuilt Box2D contact list cannot reproduce this state.
void walkPostSolveNative(b2Contact *c, Entity *actors, int count) {
    if (!c->IsEnabled() || !c->GetManifold()->pointCount)
        return;
    b2Fixture *f[] = {c->GetFixtureA(), c->GetFixtureB()};
    if (f[0]->IsSensor() || f[1]->IsSensor())
        return;
    b2WorldManifold m;
    c->GetWorldManifold(&m);
    for (int side = 0; side < 2; side++) {
        b2Body *body = f[side]->GetBody();
        int i = (int)body->GetUserData() - 1;
        if (i < 0 || i >= count || !actors[i].walk)
            continue;
        Entity &e = actors[i];
        if ((side ? m.normal.y : -m.normal.y) > .95f)
            e.walkCanWalk = 1;
        // Native0x32ae40..84 tests both side memberships only for the
        // fixture that actually produced this PostSolve, in this order.
        if (walkDetectorContainsNative(body, 3, f[1 - side]))
            nativeWalkDirection(e, false);
        if (walkDetectorContainsNative(body, 4, f[1 - side]))
            nativeWalkDirection(e, true);
    }
}
