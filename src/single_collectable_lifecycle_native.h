#pragma once

// Controls single-collectable activation, collection and restored lifecycle state.

// CollectableObject::BeginContact0x1660e8 casts the other physics object to
// Tom, then schedules PostPhysicsCollected at0x16615c. EndContact does not
// cancel it. Preserve callback order and repeated real fixture contacts.
struct SingleCollectableEventNative {
    int source;
    b2Body *body;
};

SingleCollectableEventNative singleCollectableEventsNative[4096];
int singleCollectableEventCountNative;
bool coinCollectablePendingNative[MAX_ENTITIES];

void resetSingleCollectableNative() {
    singleCollectableEventCountNative = 0;
    memset(coinCollectablePendingNative, 0, sizeof(coinCollectablePendingNative));
}

// Group BeginContact0x16dca8..dd38 removes the accepted actlet from the
// available set before scheduling0x16e258/3cc. Helpers are accepted too.
// A single item keeps its distinct native repeated-fixture behavior.
void recordCollectableContactNative(b2Contact *contact, Entity *things, int count, int player,
                                    SingleCollectableEventNative *events, int &eventCount,
                                    int capacity, bool *pending, bool singles) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            other = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= count || other < 0 || other >= count)
            continue;
        bool tom = other == player && things[other].kind == PLAYER;
        Entity &e = things[source];
        if (e.kind == COIN) {
            if ((!tom && things[other].kind != HELPER) || e.state.gone || pending[source])
                continue;
            pending[source] = true;
        } else if (!singles || !tom || (e.kind != SALAMANDER && e.kind != POTION))
            continue;
        if (eventCount >= capacity)
            fatal("Too many collectable contacts.");
        events[eventCount++] = SingleCollectableEventNative{source, fixtures[side]->GetBody()};
    }
}

void singleCollectableBeginContactNative(b2Contact *contact) {
    recordCollectableContactNative(contact, entities, entityCount, playerIndex,
                                   singleCollectableEventsNative, singleCollectableEventCountNative,
                                   4096, coinCollectablePendingNative, true);
}

bool coinGroupAvailableNative(const Object *source, Entity *things, int count,
                              const bool *pending = 0) {
    for (int i = 0; i < count; i++)
        if (things[i].kind == COIN && things[i].source == source && !things[i].state.gone &&
            (!pending || !pending[i]))
            return true;
    return false;
}

void coinGroupActivityNative(const Object *source, Entity *things, int count, b2Body **bodies,
                             const bool *pending = 0) {
    bool active = coinGroupAvailableNative(source, things, count, pending);
    for (int i = 0; i < count; i++)
        if (things[i].kind == COIN && things[i].source == source && bodies[i])
            bodies[i]->SetActive(active);
}

void restoreCoinGroupActivityNative(Entity *things, int count, b2Body **bodies) {
    for (int i = 0; i < count; i++)
        if (things[i].kind == COIN) {
            bool first = true;
            for (int j = 0; j < i; j++)
                if (things[j].kind == COIN && things[j].source == things[i].source) {
                    first = false;
                    break;
                }
            if (first)
                coinGroupActivityNative(things[i].source, things, count, bodies);
        }
}

void singleCollectablePostStepNative();
