#pragma once

// Applies authored force areas to overlapping eligible physics bodies.

// ForceAreaBehaviour::ResolveForce 0x1effa8 and PostPhysicsStep0x1f1e4c.
// Pixel-space helpers mirror the native scale cancellation (both box and
// forces are multiplied by1/64 on activation).
Vec nativeForceAreaResolve(const Props &p, Rect bounds, Vec point) {
    float x = clamp((point.x - bounds.l) / (bounds.r - bounds.l), 0, 1),
          y = clamp((point.y - bounds.b) / (bounds.t - bounds.b), 0, 1);
    float sw = (1 - x) * (1 - y), se = x * (1 - y), nw = (1 - x) * y, ne = x * y;
    return Vec{sw * p.num("sw-force-x") + se * p.num("se-force-x") + nw * p.num("nw-force-x") +
                   ne * p.num("ne-force-x"),
               sw * p.num("sw-force-y") + se * p.num("se-force-y") + nw * p.num("nw-force-y") +
                   ne * p.num("ne-force-y")};
}

float nativeForceAreaCappedAxis(float velocity, float force, float cap, float dt) {
    // Positive cap accelerates only velocities <=cap; negative cap only
    // velocities >=cap. Opposite-direction inertia is never hard-clamped.
    // Native cap0 approaches zero from either side. Preserve signed force.
    if ((cap > 0 && velocity > cap) || (cap < 0 && velocity < cap))
        return velocity;
    if (velocity < cap)
        return minf(cap, velocity + force * dt);
    if (velocity > cap)
        return maxf(cap, velocity - force * dt);
    return velocity;
}

b2Body *createNativeForceAreaBody(b2World &world, Entity &e, int index) {
    // Box::Initialize0x1310e4..1100 stores half-extents; Box::Activate
    // 0x130ee4 /0x130fc8..100c supplies kinematic/sensor/category1/maskFFFF.
    // Native loose category1 maps to this port loose category4.
    b2BodyDef bd;
    bd.type = b2_kinematicBody;
    bd.position.Set(e.state.x * metres, e.state.y * metres);
    bd.angle = e.source->angle * 0.017453293f;
    bd.fixedRotation = true;
    bd.userData = (void *)(index + 1);
    bd.gravityScale = 0;
    b2Body *body = world.CreateBody(&bd);
    b2PolygonShape shape;
    shape.SetAsBox((e.local.r - e.local.l) * 0.5f * metres,
                   (e.local.t - e.local.b) * 0.5f * metres);
    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = 0;
    fd.friction = 0.2f;
    fd.restitution = 0;
    fd.isSensor = true;
    fd.filter.categoryBits = 4;
    fd.filter.maskBits = 0xffff;
    body->CreateFixture(&fd);
    body->SetActive(!e.state.gone);
    return body;
}

void nativeForceAreaApply(Entity &area, b2Body &body, float dt) {
    if (body.GetType() != b2_dynamicBody || !body.IsActive())
        return;
    const Props &p = *area.source->behavior("ForceAreaBehaviour");
    b2Vec2 position = body.GetPosition(), velocity = body.GetLinearVelocity();
    Vec force =
        nativeForceAreaResolve(p, entityRect(area), Vec{position.x / metres, position.y / metres});
    b2Vec2 queued(0, 0);
    bool capped = false;
    if (p.flag("use-velocity-cap-x")) {
        velocity.x = nativeForceAreaCappedAxis(velocity.x, force.x * metres,
                                               p.num("velocity-cap-x") * metres, dt);
        capped = true;
    } else
        queued.x = force.x * metres * body.GetMass();
    if (p.flag("use-velocity-cap-y")) {
        velocity.y = nativeForceAreaCappedAxis(velocity.y, force.y * metres,
                                               p.num("velocity-cap-y") * metres, dt);
        capped = true;
    } else
        queued.y = force.y * metres * body.GetMass();
    if (capped)
        body.SetLinearVelocity(velocity);
    if (queued.x != 0 || queued.y != 0)
        body.ApplyForce(queued, position);
    // The game copies actor velocity to Box2D at the next pre-step. Keep
    // that snapshot coherent with native post-step capped velocity changes.
    int index = (int)body.GetUserData() - 1;
    if (index >= 0 && index < entityCount && bodies[index] == &body) {
        entities[index].state.vx = velocity.x / metres;
        entities[index].state.vy = velocity.y / metres;
    } else if (index >= MAX_ENTITIES && index < MAX_ENTITIES + MAX_PROJECTILES &&
               shotBodies[index - MAX_ENTITIES] == &body) {
        bullets[index - MAX_ENTITIES].state.vx = velocity.x / metres;
        bullets[index - MAX_ENTITIES].state.vy = velocity.y / metres;
    }
}

// Native Begin/EndContact stores a set, not per-fixture reference counts.
// Box/IPhysicsObject membership is transient and rebuilt by new contacts;
// it is not a serialized ForceArea behaviour state.
enum { MAX_NATIVE_FORCE_FIELDS = 32, NATIVE_FORCE_BODY_KEYS = MAX_ENTITIES * 2 + MAX_PROJECTILES };

struct NativeForceAreaMembers {
    int owner;
    unsigned char members[NATIVE_FORCE_BODY_KEYS];
};

NativeForceAreaMembers nativeForceAreaMembers[MAX_NATIVE_FORCE_FIELDS];

void forceAreasResetContacts() {
    memset(nativeForceAreaMembers, 0, sizeof(nativeForceAreaMembers));
}

int nativeForceAreaBodyKey(b2Body *body) {
    int index = (int)body->GetUserData() - 1;
    if (index < 0)
        return -1;
    if (index < entityCount) {
        if (bodies[index] == body)
            return index;
        if (oscillatorProxies[index] == body)
            return MAX_ENTITIES + MAX_PROJECTILES + index;
    }
    if (index >= MAX_ENTITIES && index < MAX_ENTITIES + MAX_PROJECTILES &&
        shotBodies[index - MAX_ENTITIES] == body)
        return index;
    return -1;
}

NativeForceAreaMembers *nativeForceAreaSet(int index, bool create) {
    NativeForceAreaMembers *empty = 0;
    for (int i = 0; i < MAX_NATIVE_FORCE_FIELDS; i++) {
        NativeForceAreaMembers &set = nativeForceAreaMembers[i];
        if (set.owner == index + 1)
            return &set;
        if (!set.owner && !empty)
            empty = &set;
    }
    if (create && empty) {
        empty->owner = index + 1;
        return empty;
    }
    return 0;
}

void forceAreaContact(b2Contact *contact, bool begin) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int index = (int)fixtures[side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= entityCount || entities[index].kind != FORCE ||
            bodies[index] != fixtures[side]->GetBody())
            continue;
        int key = nativeForceAreaBodyKey(fixtures[1 - side]->GetBody());
        if (key < 0)
            continue;
        NativeForceAreaMembers *set = nativeForceAreaSet(index, begin);
        if (set)
            set->members[key] = begin ? 1 : 0;
    }
}

void forceAreaBeginContact(b2Contact *contact) {
    forceAreaContact(contact, true);
}

void forceAreaEndContact(b2Contact *contact) {
    forceAreaContact(contact, false);
}

void forceAreasPostStep(float dt) {
    for (int field = 0; field < MAX_NATIVE_FORCE_FIELDS; field++) {
        NativeForceAreaMembers &set = nativeForceAreaMembers[field];
        int i = set.owner - 1;
        if (i < 0 || i >= entityCount || entities[i].kind != FORCE || !bodies[i] ||
            !bodies[i]->IsActive())
            continue;
        for (int key = 0; key < NATIVE_FORCE_BODY_KEYS; key++)
            if (set.members[key]) {
                b2Body *body = key < MAX_ENTITIES ? bodies[key]
                               : key < MAX_ENTITIES + MAX_PROJECTILES
                                   ? shotBodies[key - MAX_ENTITIES]
                                   : oscillatorProxies[key - MAX_ENTITIES - MAX_PROJECTILES];
                if (body)
                    nativeForceAreaApply(entities[i], *body, dt);
            }
    }
}
