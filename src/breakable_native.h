#pragma once

// Contact-driven breakable-block state and destruction behavior.
int breakableDamageNative[MAX_ENTITIES];

void resetBreakableDamageNative() {
    memset(breakableDamageNative, 0, sizeof(breakableDamageNative));
}

// Breakable::PostSolve0x139158 casts the other PhysicsObject to
// IKillableEnemyAffector. Of the original body owners, only Tom implements
// both PhysicsObject and this interface. HelperBird owns a separate ordinary
// BoxPhysics body; its behaviour's interface is not the body's dynamic type.
void breakablePostSolveNative(b2Contact *contact) {
    if (!contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fs[0]->IsSensor() || fs[1]->IsSensor())
        return;
    for (int side = 0; side < 2; side++) {
        b2Body *own = fs[side]->GetBody();
        int index = (int)own->GetUserData() - 1,
            other = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= entityCount || other < 0 || other >= entityCount ||
            bodies[index] != own || bodies[other] != fs[1 - side]->GetBody())
            continue;
        Entity &block = entities[index];
        Entity &affector = entities[other];
        if (!block.breakable || block.state.gone || block.turnCooldown > 0 ||
            affector.kind != PLAYER)
            continue;
        b2Fixture *main = own->GetFixtureList();
        while (main && main->IsSensor())
            main = main->GetNext();
        if (fs[side] != main)
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        float normalY = side ? -m.normal.y : m.normal.y;
        const Props &props = *block.source->behavior("BreakableBehaviour");
        breakableDamageNative[index] += (int)props.num(
            normalY < -.9f ? "hp-on-bottom-hit" : "hp-on-other-hit", normalY < -.9f ? 3.0f : 0.0f);
    }
}

void updateBreakableContacts() {
    // Native PostPhysicsStep consumes the accumulated solved damage once,
    // regardless of which contacts survive the end of the Box2D step.
    for (int i = 0; i < entityCount; i++) {
        int damage = breakableDamageNative[i];
        breakableDamageNative[i] = 0;
        Entity &e = entities[i];
        if (!e.breakable || e.state.gone || !bodies[i] || damage <= 0)
            continue;
        e.state.hp -= damage;
        e.turnCooldown = .25f;
        if (e.state.hp <= 0) {
            e.state.gone = 1;
            bodies[i]->SetActive(false);
            entityEffect(e, "BreakableBehaviour", "explosion-actor");
            sound("enemyexplosion");
        } else
            sound("smallexplosion");
    }
}
