#pragma once

// Dispatches contact-driven creature, spring and mechanism behavior using authored fixture roles.

// Crumbling::PostSolve0x191300 records each solved Tom contact, independent
// of velocity or whichever other fixture becomes the final ground support.
void crumblingCloudPostSolve(b2Contact *contact, Entity *things, int count, int player) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            other = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        if (source >= 0 && source < count && other == player && things[source].crumble &&
            !things[source].state.gone && things[source].state.active != 2)
            things[source].state.active = 1;
    }
}

bool switchPending[MAX_ENTITIES];

// Spring::PreSolve 0x2fe1dc saves the other fixture's velocity before
// impulses. PostSolve tests this saved Y, never the already bounced body Y.
struct SpringIncomingVelocity {
    b2Contact *contact;
    b2Fixture *other;
    float y, solvedY;
    int source, target, points;
    bool solved;
    b2WorldManifold manifold;
};

SpringIncomingVelocity springIncoming[4096];
int springIncomingCount;
bool playerMagnetContact;
int playerSpringContact = -1;
void markMagnetActlets(Entity *things, int thingCount, Entity &owner,
                       const b2WorldManifold &manifold, int points);

void mechanismsPreStep() {
    springIncomingCount = 0;
    playerMagnetContact = false;
    playerSpringContact = -1;
}

void recordSpringIncoming(b2Contact *contact, Entity *things, int thingCount,
                          SpringIncomingVelocity *incoming, int &incomingCount, int capacity) {
    if (!contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= thingCount || things[source].kind != SPRING ||
            fixtures[side]->IsSensor() || fixtures[1 - side]->IsSensor())
            continue;
        b2Fixture *other = fixtures[1 - side];
        int slot = 0;
        for (; slot < incomingCount; slot++)
            if (incoming[slot].contact == contact && incoming[slot].other == other)
                break;
        if (slot == incomingCount) {
            if (slot == capacity)
                fatal("Too many spring contacts.");
            incomingCount++;
            memset(&incoming[slot], 0, sizeof(incoming[slot]));
            incoming[slot].contact = contact;
            incoming[slot].other = other;
            incoming[slot].source = source;
            incoming[slot].target = (int)other->GetBody()->GetUserData() - 1;
        }
        incoming[slot].y = other->GetBody()->GetLinearVelocity().y;
    }
}

void mechanismPreSolve(b2Contact *contact) {
    recordSpringIncoming(contact, entities, entityCount, springIncoming, springIncomingCount, 4096);
}

void recordSpringPostSolve(b2Contact *contact, SpringIncomingVelocity *incoming, int count) {
    for (int i = 0; i < count; i++) {
        SpringIncomingVelocity &impact = incoming[i];
        if (impact.contact != contact || impact.y >= 0)
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        b2Vec2 normal = contact->GetFixtureB() == impact.other ? m.normal : -m.normal;
        if (normal.y <= 0.1f)
            continue;
        impact.solved = true;
        impact.solvedY = impact.y;
        impact.manifold = m;
        impact.manifold.normal = normal;
        impact.points = contact->GetManifold()->pointCount;
    }
}

bool magnetPostSolve(b2Contact *contact, Entity *things, int thingCount, int player) {
    if (player < 0 || player >= thingCount)
        return false;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fixtures[0]->IsSensor() || fixtures[1]->IsSensor())
        return false;
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            target = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= thingCount || target != player || things[source].kind != MAGNET)
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        float normalY = side ? -m.normal.y : m.normal.y;
        if (normalY > 0.1f) {
            markMagnetActlets(things, thingCount, things[source], m,
                              contact->GetManifold()->pointCount);
            things[player].state.active = 2;
            return true;
        }
    }
    return false;
}

void mechanismPostSolve(b2Contact *contact) {
    recordSpringPostSolve(contact, springIncoming, springIncomingCount);
    playerMagnetContact |= magnetPostSolve(contact, entities, entityCount, playerIndex);
}

bool springIncomingDescending(const SpringIncomingVelocity *incoming, int count, b2Contact *contact,
                              b2Body *other) {
    for (int i = 0; i < count; i++)
        if (incoming[i].contact == contact && incoming[i].other->GetBody() == other)
            return incoming[i].solved && incoming[i].solvedY < 0;
    return false;
}

bool springWasDescending(b2Contact *contact, b2Body *other) {
    return springIncomingDescending(springIncoming, springIncomingCount, contact, other);
}

bool switchOn(const Entity &e) {
    return ((alternationBits & e.mask) != 0) != (e.alternation != 0);
}

void mechanismBeginContact(b2Contact *contact) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int index = (int)fixtures[side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= entityCount || entities[index].kind != SWITCH ||
            fixtures[1 - side]->IsSensor())
            continue;
        // AlternatorSwitch::BeginContact 0xef6dc accepts only its active
        // face: OFF has the upper detector, ON has the lower detector.
        if (fixtures[side]->GetUserData() == (void *)(switchOn(entities[index]) ? 11 : 12))
            switchPending[index] = true;
    }
}

void updateSeesaw(Entity &e, b2Body &body, b2RevoluteJoint &joint) {
    const Props &p = *e.source->behavior("SeesawBehaviour");
    if (!p.flag("auto-correct"))
        return;
    float angle = body.GetAngle();
    while (angle > b2_pi)
        angle -= 2 * b2_pi;
    while (angle < -b2_pi)
        angle += 2 * b2_pi;
    float absolute = absf(angle), error = minf(absolute, b2_pi - absolute), speed = 0;
    if (error > 0.1f * 0.017453293f) {
        float low = p.num("min-adjust-speed", 0.1f), high = p.num("max-adjust-speed", 5);
        speed = low + (high - low) * clamp(error / (b2_pi * 0.5f), 0, 1);
        if ((angle > 0) == (absolute <= b2_pi * 0.5f))
            speed = -speed;
    }
    // Native joint has the rotating body first and pivot second. Our joint
    // reverses that order, so the correction motor sign is reversed too.
    joint.SetMotorSpeed(speed);
}

bool launchDynamic(Entity &source, Entity &target, b2Body &body, b2Vec2 direction, float height) {
    if (body.GetType() != b2_dynamicBody || height <= 0)
        return false;
    float speed = root(1000 * height);
    b2Vec2 velocity = body.GetLinearVelocity();
    if (source.kind == SPRING) {
        velocity.y = speed * metres;
    } else {
        if (direction.Normalize() < 0.00001f)
            direction.Set(0, 1);
        velocity.x += speed * metres * direction.x;
        velocity.y = speed * metres * direction.y;
    }
    body.SetLinearVelocity(velocity);
    target.state.vx = velocity.x / metres;
    target.state.vy = velocity.y / metres;
    if (target.kind == PLAYER)
        target.lastLaunchSpeed = speed;
    if (!boxGroupMatrix(source)) {
        source.state.active = 2;
        source.state.timer = 0;
    }
    sound(source.kind == SPRING ? "megajump" : "pinball");
    return true;
}

void markMatrixContactActlets(Entity *things, int thingCount, Entity &owner,
                              const b2WorldManifold &manifold, int points, bool restart) {
    if (!boxGroupMatrix(owner)) {
        owner.state.active = 2;
        if (restart)
            owner.state.timer = 0;
        return;
    }
    // Magnet::PostSolve 0x2736d0..0x2737bc and Spring::PostSolve
    // 0x2fd48c..0x2fd578 project one/two contact points onto the whole matrix
    // AABB; only those actlets change animation (spring also restarts them).
    int count = 0;
    for (int i = 0; i < thingCount; i++)
        if (things[i].source == owner.source)
            count++;
    float left = owner.source->rect.l + owner.state.x - owner.originX,
          width = owner.source->rect.r - owner.source->rect.l;
    if (count <= 0 || width <= 0)
        return;
    for (int point = 0; point < points && point < 2; point++) {
        int cell = (int)clamp((manifold.points[point].x / metres - left) / width * count, 0,
                              (float)(count - 1)),
            ordinal = 0;
        for (int i = 0; i < thingCount; i++)
            if (things[i].source == owner.source) {
                if (ordinal == cell) {
                    things[i].state.active = 2;
                    if (restart)
                        things[i].state.timer = 0;
                    break;
                }
                ordinal++;
            }
    }
}

void markMagnetActlets(Entity *things, int thingCount, Entity &owner,
                       const b2WorldManifold &manifold, int points) {
    markMatrixContactActlets(things, thingCount, owner, manifold, points, false);
}

void markBounceActlets(int index) {
    if (index < 0 || index >= entityCount)
        return;
    Entity &owner = entities[index];
    if (!boxGroupMatrix(owner)) {
        owner.state.active = 2;
        owner.state.timer = 0;
        return;
    }
    if (!bodies[index])
        return;
    for (b2ContactEdge *edge = bodies[index]->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (edge->other != bodies[playerIndex] || !c->IsTouching() || !c->IsEnabled() ||
            !c->GetManifold()->pointCount || c->GetFixtureA()->IsSensor() ||
            c->GetFixtureB()->IsSensor())
            continue;
        b2WorldManifold m;
        c->GetWorldManifold(&m);
        float ny = c->GetFixtureA()->GetBody() == bodies[index] ? m.normal.y : -m.normal.y;
        if (ny > 0.1f)
            markMatrixContactActlets(entities, entityCount, owner, m, c->GetManifold()->pointCount,
                                     true);
    }
}

void dispatchCampaignPinballDynamicNative();

void mechanismsPostStep(float dt) {
    (void)dt;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.state.gone)
            continue;
        if (e.kind == SWITCH && switchPending[i]) {
            alternateScene(e.mask);
            e.state.timer = 0;
            e.state.aux = 0;
            sound("switch");
        }
        switchPending[i] = false;
        if (!bodies[i] || !bodies[i]->IsActive())
            continue;
        // Magnet PostSolve already records the per-step Tom flag and exact
        // touched actlets. A retained contact list is not another solved event.
        if (e.kind == SPRING) {
            for (int event = 0; event < springIncomingCount; event++) {
                SpringIncomingVelocity &impact = springIncoming[event];
                int target = impact.target;
                if (!impact.solved || impact.source != i || target < 0 || target >= entityCount ||
                    !bodies[target] || entities[target].state.gone ||
                    bodies[target]->GetType() != b2_dynamicBody)
                    continue;
                if (target == playerIndex)
                    continue; // Tom request and actlets were recorded in actual PostSolve callback order.
                if (launchDynamic(e, entities[target], *bodies[target], impact.manifold.normal,
                                  e.source->behavior("SpringBehaviour")->num("physics-power", 200)))
                    markMatrixContactActlets(entities, entityCount, e, impact.manifold,
                                             impact.points, true);
            }
            continue;
        }
    }
    dispatchCampaignPinballDynamicNative();
}

void checkPlayerCompression() {
    b2Body *bean = bodies[playerIndex];
    if (!bean || deathTimer > 0)
        return;
    bool left = false, right = false, above = false, below = false, closingX = false,
         closingY = false;
    for (b2ContactEdge *edge = bean->GetContactList(); edge; edge = edge->next) {
        b2Contact *contact = edge->contact;
        if (!contact->IsTouching() || !contact->IsEnabled() ||
            !contact->GetManifold()->pointCount || contact->GetFixtureA()->IsSensor() ||
            contact->GetFixtureB()->IsSensor())
            continue;
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        b2Vec2 normal =
            contact->GetFixtureA()->GetBody() == bean ? manifold.normal : -manifold.normal;
        left |= normal.x < -0.8f;
        right |= normal.x > 0.8f;
        above |= normal.y > 0.8f;
        below |= normal.y < -0.8f;
        int index = (int)edge->other->GetUserData() - 1;
        if (index < 0 || index >= entityCount)
            continue;
        Entity &e = entities[index];
        if (!e.move || !motionActive(e))
            continue;
        float direction = e.state.aux < 0 ? -1.0f : 1.0f;
        closingX |= absf(normal.x) > 0.8f && normal.x * e.moveX * direction * e.speed < -1;
        closingY |= absf(normal.y) > 0.8f && normal.y * e.moveY * direction * e.speed < -1;
    }
    // Opposed physical contacts plus an inward-driven moving block are the
    // actual compression condition reported by the player. Static narrow
    // passages and harmless contacts on a single side do not kill Bean.
    if ((left && right && closingX) || (above && below && closingY))
        die();
}
