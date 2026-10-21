#pragma once

// Queues strong-bounce effects on dynamic actors until after physics stepping.

// Native Pinball::PreSolve0x2cc31c..468 captures every non-Tom dynamic
// contact's normal and schedules Launch after physics, even if its contact
// is later disabled or separated. Sound/animation belong to PreSolve.
struct PinballDynamicEventNative {
    int source, target;
    b2Body *body;
    Vec normal;
};

struct PinballDynamicQueueNative {
    int count;
    PinballDynamicEventNative events[1024];
};

PinballDynamicQueueNative campaignPinballDynamicNative;

void resetPinballDynamicNative(PinballDynamicQueueNative &queue) {
    queue.count = 0;
}

void recordPinballDynamicNative(b2Contact *contact, Entity *things, int count, int player,
                                PinballDynamicQueueNative &queue) {
    if (!contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fixtures[0]->IsSensor() || fixtures[1]->IsSensor())
        return;
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            target = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        b2Body *body = fixtures[1 - side]->GetBody();
        if (source < 0 || source >= count || target < 0 || target >= count || target == player ||
            things[source].kind != BOUNCER || things[source].state.gone ||
            body->GetType() != b2_dynamicBody)
            continue;
        if (queue.count >= 1024)
            fatal("Too many Pinball contacts.");
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        Vec normal = {side ? -m.normal.x : m.normal.x, side ? -m.normal.y : m.normal.y};
        queue.events[queue.count++] = PinballDynamicEventNative{source, target, body, normal};
        things[source].state.active = 2;
        things[source].state.timer = 0;
        sound("pinball");
    }
}

void dispatchPinballDynamicNative(PinballDynamicQueueNative &queue, Entity *things, int count,
                                  b2Body **bodies) {
    int events = queue.count;
    queue.count = 0;
    for (int i = 0; i < events; i++) {
        const PinballDynamicEventNative &event = queue.events[i];
        if (event.source < 0 || event.source >= count || event.target < 0 ||
            event.target >= count || bodies[event.target] != event.body)
            continue;
        float height = things[event.source]
                           .source->behavior("PinballBouncerBehaviour")
                           ->num("physics-power", 200);
        if (height <= 0)
            continue;
        float speed = root(-2 * nativeWorldGravity * height);
        b2Vec2 velocity = event.body->GetLinearVelocity();
        velocity.x += speed * metres * event.normal.x;
        velocity.y = speed * metres * event.normal.y;
        event.body->SetLinearVelocity(velocity);
        things[event.target].state.vx = velocity.x / metres;
        things[event.target].state.vy = velocity.y / metres;
    }
}

void dispatchCampaignPinballDynamicNative() {
    dispatchPinballDynamicNative(campaignPinballDynamicNative, entities, entityCount, bodies);
}
