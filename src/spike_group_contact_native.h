#pragma once

// Evaluates spike-group contacts using native fixture roles and actor eligibility.

// SpikeGroup::BeginContact 0x2fb4d8; this is a sensor BeginContact path,
// distinct from standalone SpikeBehaviour and Deadly::PostSolve.
bool spikeGroupEntityOn(const Entity &e) {
    return (e.alternation != 0) != ((alternationBits & e.mask) != 0);
}

bool nativeSpikeGroupAffects(bool on, bool otherSensor, bool affector, bool killable) {
    return on && !otherSensor && (affector || killable);
}

bool spikeGroupPending[MAX_ENTITIES], spikeGroupShotPending[MAX_PROJECTILES];
void stopHelper(Entity &bird);

void spikeGroupContactReset() {
    memset(spikeGroupPending, 0, sizeof(spikeGroupPending));
    memset(spikeGroupShotPending, 0, sizeof(spikeGroupShotPending));
}

void spikeGroupBeginContact(b2Contact *contact) {
    b2Fixture *f[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)f[side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= entityCount || entities[source].kind != SPIKE ||
            !entities[source].source->has("SpikeGroupBehaviour"))
            continue;
        Entity &spike = entities[source];
        if (spike.state.gone || !spikeGroupEntityOn(spike) || f[1 - side]->IsSensor())
            continue;
        int target = (int)f[1 - side]->GetBody()->GetUserData() - 1;
        // Native0x2fb530 dynamic_casts the other PhysicsObject to
        // BasePhysicsBehaviour. Oscillator/diagonal detectors are separate
        // PhysicsObjects, despite sharing their actor index in this port.
        if (target >= 0 && target < entityCount && f[1 - side]->GetBody() == bodies[target]) {
            Entity &e = entities[target];
            bool affector = e.kind == PLAYER || e.kind == HELPER;
            if (!e.state.gone && nativeSpikeGroupAffects(true, false, affector,
                                                         e.source->has("KillableEnemyBehaviour")))
                spikeGroupPending[target] = true;
        } else {
            int slot = target - MAX_ENTITIES;
            if (slot >= 0 && slot < MAX_PROJECTILES && f[1 - side]->GetBody() == shotBodies[slot] &&
                bullets[slot].life > 0 &&
                classes[bullets[slot].klass].has("KillableEnemyBehaviour"))
                spikeGroupShotPending[slot] = true;
        }
    }
}

void spikeGroupContactPostStep() {
    // Native only changes behaviour flags in BeginContact. Defer any port
    // Box2D body activation changes until the world is no longer locked.
    bool playerDeath = false;
    for (int i = 0; i < entityCount; i++)
        if (spikeGroupPending[i]) {
            spikeGroupPending[i] = false;
            Entity &e = entities[i];
            if (e.state.gone)
                continue;
            if (e.kind == PLAYER)
                playerDeath = true;
            else if (e.kind == HELPER) {
                if (e.state.active == 1)
                    sound("bird");
                stopHelper(e);
            } else if (e.source->has("KillableEnemyBehaviour")) {
                killEnemy(e);
                if (bodies[i])
                    bodies[i]->SetActive(false);
                if (oscillatorProxies[i])
                    oscillatorProxies[i]->SetActive(false);
            }
        }
    for (int slot = 0; slot < MAX_PROJECTILES; slot++)
        if (spikeGroupShotPending[slot]) {
            spikeGroupShotPending[slot] = false;
            if (bullets[slot].life <= 0)
                continue;
            if (shotBodies[slot]) {
                b2Vec2 p = shotBodies[slot]->GetPosition();
                shotContacts[slot].point = Vec{p.x / metres, p.y / metres};
            }
            destroyShot(slot);
            sound("enemyexplosion");
        }
    if (playerDeath)
        die();
}
