#pragma once

// Collects contact-driven enemy death requests without destroying bodies inside Box2D callbacks.
#include "creature_compression.h"
CreaturePressureState<MAX_ENTITIES> campaignCreaturePressure;

// KillableEnemyBehaviour::PreSolve 0x238bf8..0x238c44: only a dynamic
// non-player body can kill by impact. This is its ABSOLUTE linear momentum
// in pixels/second times Box2D mass, not relative or normal impulse.
bool nativeKillableMomentum(float mass, Vec velocityPixels, float threshold, bool dynamic) {
    float x = mass * velocityPixels.x, y = mass * velocityPixels.y;
    return dynamic && x * x + y * y >= threshold * threshold;
}

// Full player branch, 0x238aac..0x238b34. Every world-manifold point
// must lie in the authored vulnerable strip. Downward absolute OR relative
// player velocity qualifies; the real collision normal is not a jump vector.
bool nativeKillablePlayerStomp(const Vec *points, int count, float bodyY, float adjustedTop,
                               float vulnerableSize, float playerVy, float enemyVy) {
    bool strip = true;
    float lower = bodyY + adjustedTop - vulnerableSize;
    for (int i = 0; i < count; i++)
        if (points[i].y < lower)
            strip = false;
    return strip && (playerVy < 0 || playerVy - enemyVy < 0);
}

// Native0x238a40 casts to IKillableEnemyAffector, implemented by both Tom
// and HelperBird. Helper KilledEnemyFromJump is empty; Die stops flight.
bool recordHelperEnemyContactNative(b2Contact *contact, Entity *actors, int count,
                                    b2Body **mainBodies, bool *enemyPending, bool *helperStop) {
    if (!contact->IsTouching() || !contact->GetManifold()->pointCount)
        return false;
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fs[0]->IsSensor() || fs[1]->IsSensor())
        return false;
    for (int side = 0; side < 2; side++) {
        b2Body *own = fs[side]->GetBody();
        b2Body *other = fs[1 - side]->GetBody();
        int enemy = (int)own->GetUserData() - 1, bird = (int)other->GetUserData() - 1;
        if (enemy < 0 || enemy >= count || bird < 0 || bird >= count || mainBodies[enemy] != own ||
            mainBodies[bird] != other || actors[bird].kind != HELPER || actors[bird].state.gone ||
            actors[enemy].state.gone || !actors[enemy].state.active)
            continue;
        const Props *config = actors[enemy].source->behavior("KillableEnemyBehaviour");
        b2Fixture *main = own->GetFixtureList();
        while (main && main->IsSensor())
            main = main->GetNext();
        if (!config || fs[side] != main)
            continue;
        if (!enemyPending[enemy]) {
            b2WorldManifold m;
            contact->GetWorldManifold(&m);
            Vec points[2];
            int n = contact->GetManifold()->pointCount;
            for (int i = 0; i < n && i < 2; i++)
                points[i] = Vec{m.points[i].x / metres, m.points[i].y / metres};
            if (nativeKillablePlayerStomp(points, n, own->GetPosition().y / metres,
                                          actors[enemy].hit.t, config->num("vulnerable-size", 8),
                                          other->GetLinearVelocity().y / metres,
                                          own->GetLinearVelocity().y / metres))
                enemyPending[enemy] = true;
            else
                helperStop[bird] = true;
        }
        contact->SetEnabled(false);
        return true;
    }
    return false;
}

struct KillablePlayerStep {
    bool stomp, die;
    float boost;
    Vec point;
};

KillablePlayerStep killablePlayerStep;

bool killablePlayerStompedThisStep() {
    return killablePlayerStep.stomp;
}

bool killableContactPending[MAX_ENTITIES];
bool killableShotPending[MAX_PROJECTILES];
bool killableHelperStopPendingNative[MAX_ENTITIES];

void killableContactReset() {
    resetCreaturePressure(campaignCreaturePressure);
    memset(killableContactPending, 0, sizeof(killableContactPending));
    memset(killableShotPending, 0, sizeof(killableShotPending));
    memset(killableHelperStopPendingNative, 0, sizeof(killableHelperStopPendingNative));
    memset(&killablePlayerStep, 0, sizeof(killablePlayerStep));
}

void killableContactPreSolve(b2Contact *contact) {
    if (recordHelperEnemyContactNative(contact, entities, entityCount, bodies,
                                       killableContactPending, killableHelperStopPendingNative))
        return;
    if (!contact->IsTouching() || contact->GetManifold()->pointCount <= 0)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fixtures[0]->IsSensor() || fixtures[1]->IsSensor())
        return;
    for (int side = 0; side < 2; side++) {
        b2Body *own = fixtures[side]->GetBody();
        int index = (int)own->GetUserData() - 1, slot = index - MAX_ENTITIES;
        bool shot = slot >= 0 && slot < MAX_PROJECTILES;
        Object *source = 0;
        float adjustedTop = 0;
        bool *pending = 0;
        if (index >= 0 && index < entityCount && own == bodies[index]) {
            Entity &enemy = entities[index];
            if (enemy.state.gone || !enemy.state.active)
                continue;
            source = enemy.source;
            adjustedTop = enemy.hit.t;
            pending = killableContactPending + index;
        } else if (shot && own == shotBodies[slot] && shotFlyingNative(bullets[slot])) {
            source = classes + bullets[slot].klass;
            const Props *shape = source->behavior("CirclePhysicsBehaviour");
            adjustedTop =
                (source->rect.t - source->rect.b) * 0.5f + (shape ? shape->num("adjust-top") : 0);
            pending = killableShotPending + slot;
        } else
            continue;
        const Props *props = source->behavior("KillableEnemyBehaviour");
        // Box2D prepends the later walking probes. Resolve the first solid
        // fixture rather than mistaking the list head (a sensor) for main.
        b2Fixture *main = own->GetFixtureList();
        while (main && main->IsSensor())
            main = main->GetNext();
        if (!props || fixtures[side] != main)
            continue;
        b2Body *other = fixtures[1 - side]->GetBody();
        if (other == bodies[playerIndex]) {
            b2WorldManifold manifold;
            contact->GetWorldManifold(&manifold);
            int count = contact->GetManifold()->pointCount;
            Vec points[2];
            for (int i = 0; i < count && i < 2; i++)
                points[i] = Vec{manifold.points[i].x / metres, manifold.points[i].y / metres};
            if (nativeKillablePlayerStomp(points, count, own->GetPosition().y / metres, adjustedTop,
                                          props->num("vulnerable-size", 8),
                                          other->GetLinearVelocity().y / metres,
                                          own->GetLinearVelocity().y / metres)) {
                *pending = true;
                killablePlayerStep.stomp = true;
                killablePlayerStep.boost = props->num("jump-boost");
                killablePlayerStep.point = points[0];
                recordCampaignOrdinaryJumpNative(Vec{0, 1}, points[0], killablePlayerStep.boost,
                                                 true);
                if (count == 2)
                    killablePlayerStep.point =
                        Vec{(points[0].x + points[1].x) * 0.5f, (points[0].y + points[1].y) * 0.5f};
            } else
                killablePlayerStep.die = true;
            contact->SetEnabled(false);
            continue;
        }
        if (*pending) {
            contact->SetEnabled(false);
            continue;
        }
        b2Vec2 v = other->GetLinearVelocity();
        Vec pixels = {v.x / metres, v.y / metres};
        if (nativeKillableMomentum(other->GetMass(), pixels,
                                   props->num("vulnerable-momentum", 1000),
                                   other->GetType() == b2_dynamicBody)) {
            *pending = true;
            contact->SetEnabled(false);
        }
    }
}

// Deadly::PostSolve0x197e24 excludes the other fixture's sensor before
// inspecting its actual PhysicsObject behaviours. kill-enemies+44 gates
// the fallback KillableEnemy lookup0x198498..0x19860c. It neither tests
// sprite rectangles nor requires a momentum threshold for this path.
Object *nativeKillableContactSource(b2Body *body, int &entity, int &shot) {
    entity = shot = -1;
    if (!body)
        return 0;
    int index = (int)body->GetUserData() - 1;
    if (index >= 0 && index < entityCount && body == bodies[index] && !entities[index].state.gone) {
        entity = index;
        return entities[index].source;
    }
    int slot = index - MAX_ENTITIES;
    if (slot >= 0 && slot < MAX_PROJECTILES && body == shotBodies[slot] &&
        shotFlyingNative(bullets[slot])) {
        shot = slot;
        return classes + bullets[slot].klass;
    }
    return 0;
}

bool nativeDeadlyKillsEnemy(const Object &deadly, const Object &victim) {
    const Props *p = deadly.behavior("DeadlyBehaviour");
    return p && p->flag("kill-enemies", false) && victim.has("KillableEnemyBehaviour");
}

void killableDeadlyPostSolve(b2Contact *contact) {
    if (!contact->IsTouching() || !contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fixtures[0]->IsSensor() || fixtures[1]->IsSensor())
        return;
    for (int side = 0; side < 2; side++) {
        int sourceEntity, sourceShot, targetEntity, targetShot;
        Object *source =
            nativeKillableContactSource(fixtures[side]->GetBody(), sourceEntity, sourceShot);
        Object *target =
            nativeKillableContactSource(fixtures[1 - side]->GetBody(), targetEntity, targetShot);
        if (!source || !target || !nativeDeadlyKillsEnemy(*source, *target))
            continue;
        // Killable::Die0x237dbc schedules DidDie after the step. Reuse the
        // same pending queue as true momentum/stomp contacts, idempotently.
        if (targetEntity >= 0)
            killableContactPending[targetEntity] = true;
        else if (targetShot >= 0)
            killableShotPending[targetShot] = true;
    }
}

void killableContactPostStep() {
    // Native schedules DidDie through CollisionWorld::SchedulePostPhysicsStep
    // at 0x238c48..0x238c8c, then disables the contact at 0x238b48.
    for (int i = 0; i < entityCount; i++)
        if (killableContactPending[i]) {
            killableContactPending[i] = false;
            if (entities[i].state.gone)
                continue;
            killEnemy(entities[i]);
            if (bodies[i])
                bodies[i]->SetActive(false);
            if (oscillatorProxies[i])
                oscillatorProxies[i]->SetActive(false);
        }
    // killable_contact_native.h is included after projectiles.h. A stomped
    // wizard projectile uses the SAME KillableEnemy callback and its sound.
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (killableShotPending[i]) {
            killableShotPending[i] = false;
            if (shotFlyingNative(bullets[i]))
                beginShotDeathNative(i);
        }
    if (killablePlayerStep.die) {
        if (playerIndex >= 0 && bodies[playerIndex]) {
            b2Vec2 p = bodies[playerIndex]->GetPosition();
            entities[playerIndex].state.x = p.x / metres;
            entities[playerIndex].state.y = p.y / metres;
        }
        die();
        return;
    }
}
