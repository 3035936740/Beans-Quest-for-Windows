#pragma once

// Campaign physics orchestration. Contact callbacks record events; gameplay consumes them after Box2D unlocks.
#include <Box2D/Box2D.h>

// Pimento stores geometry and motion in pixels; Box2D operates in metres.
// CollisionWorld::WorldToPhysicsScale in libBeansQuest.so is exactly 1/64.
const float metres = 1.0f / 64.0f;
// CollisionWorld global constructor 0x17091c: the world is -500 px/s^2.
// Tom supplies its independent behaviour gravity (normally -400).
const float nativeWorldGravity = -500.0f;
b2World *physicsWorld;
b2Body *bodies[MAX_ENTITIES];
b2Body *oscillatorProxies[MAX_ENTITIES];
b2Body *shotBodies[MAX_PROJECTILES];
int shotBodyClasses[MAX_PROJECTILES];
b2RevoluteJoint *hinges[MAX_ENTITIES];
bool playerTouches[MAX_ENTITIES], playerShotTouches[MAX_PROJECTILES],
    helperSolvedContactsNative[MAX_ENTITIES];
int supportEntity = -1;
bool onGround;
float preStepVy;
#include "water.h"
#include "jumper_water_pose.h"
#include "force_area_native.h"
#include "tom_force_native.h"
#include "secret_reference_native.h"
#include "physics_shapes.h"
#include "motion.h"
#include "walk_native.h"
#include "jumper_ground_native.h"
#include "contact_behaviors.h"
#include "pinball_dynamic_native.h"
#include "touch_alternator_native.h"
#include "level_end_native.h"
#include "single_collectable_lifecycle_native.h"
#include "alternator_body_native.h"
#include "pinball_native.h"
#include "wizard_contact_native.h"
#include "spring_player_native.h"
#include "trampoline_campaign_native.h"
#include "projectiles.h"
#include "bullet_source.h"
#include "tom_contact.h"
#include "checkpoint_native.h"
#include "killable_contact_native.h"
#include "breakable_native.h"
#include "mechanism_event_checkpoint_native.h"
#include "spike_group_contact_native.h"
#include "tom_jump_diagnostics.h"

void burstStageCloud(Entity &e, int index) {
    if (e.state.active == 2)
        return;
    sound("james-cloud");
    entityEffect(e, "CrumblingPlatformBehaviour", "effect");
    e.state.active = 2;
    e.state.timer = 0;
    if (bodies[index])
        bodies[index]->SetActive(false);
}

// Native Tom PreSolve stores incoming velocity; each eligible PostSolve
// replaces the ordinary jump normal. Preserve the last callback this step.
float tomOrdinaryIncomingVyNative, tomOrdinarySolvedVxNative;
Vec tomOrdinaryNormalNative;
bool tomOrdinaryNormalFoundNative;

void tomOrdinaryPostSolveNative(b2Contact *contact, b2Body *player, float incomingVy,
                                Vec &normalOut, bool &found, float &solvedVx,
                                OrdinaryJumpStateNative *request = 0, Entity *bean = 0) {
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fs[0]->IsSensor() || fs[1]->IsSensor() || !contact->IsEnabled() ||
        !contact->GetManifold()->pointCount)
        return;
    for (int side = 0; side < 2; side++) {
        if (fs[side]->GetBody() != player)
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        float sign = side == 0 ? -1.0f : 1.0f;
        Vec normal = {m.normal.x * sign, m.normal.y * sign};
        if (nativeTomCanAutoJump(incomingVy, normal.y)) {
            normalOut = normal;
            found = true;
            solvedVx = fs[side]->GetBody()->GetLinearVelocity().x / metres;
            if (request && bean) {
                Vec point = {0, 0};
                int points = contact->GetManifold()->pointCount;
                for (int k = 0; k < points; k++) {
                    point.x += m.points[k].x / metres;
                    point.y += m.points[k].y / metres;
                }
                point.x /= points;
                point.y /= points;
                request->pending = 1;
                request->direction =
                    nativeTomJumpFactors(normal.x, bean->physics->num("x-jump-factor", .2f));
                request->point = point;
                request->speed = nativeStompLaunch(bean->physics->num("jump-height", 74), 0, 0,
                                                   bean->physics->num("gravity", -400));
            }
        }
    }
}

void tomOrdinaryPostSolveNative(b2Contact *contact) {
    tomOrdinaryPostSolveNative(contact, bodies[playerIndex], tomOrdinaryIncomingVyNative,
                               tomOrdinaryNormalNative, tomOrdinaryNormalFoundNative,
                               tomOrdinarySolvedVxNative, &ordinaryJumpStateNative,
                               &entities[playerIndex]);
}

// KilledEnemyFromJump requests the same Jump and keeps +c8 sticky until
// the next controller update; a later ordinary callback can replace payload.
void recordCampaignOrdinaryJumpNative(Vec normal, Vec point, float boost, bool stomp) {
    if (playerIndex < 0)
        return;
    Entity &bean = entities[playerIndex];
    const Props &p = *bean.physics;
    ordinaryJumpStateNative.pending = 1;
    ordinaryJumpStateNative.stomp |= stomp ? 1 : 0;
    ordinaryJumpStateNative.direction = nativeTomJumpFactors(normal.x, p.num("x-jump-factor", .2f));
    ordinaryJumpStateNative.point = point;
    ordinaryJumpStateNative.speed =
        nativeStompLaunch(p.num("jump-height", 74), 0, boost, p.num("gravity", -400));
}

void consumeCampaignOrdinaryJumpNative(float oldAge, bool specialPending) {
    OrdinaryJumpStateNative request = ordinaryJumpStateNative;
    ordinaryJumpStateNative = OrdinaryJumpStateNative{};
    ordinaryJumpLaunchedNative = false;
    if (!request.pending || specialPending || request.magnet || oldAge < .05f || deathTimer > 0 ||
        playerWalkMode || playerIndex < 0 || entities[playerIndex].state.gone)
        return;
    Entity &bean = entities[playerIndex];
    bean.state.vx += request.speed * request.direction.x;
    bean.state.vy = request.speed * request.direction.y;
    bean.lastLaunchSpeed = request.speed;
    bean.state.timer = 0;
    bean.state.active = 1;
    ordinaryJumpLaunchedNative = true;
    if (!request.stomp && bean.state.hp == 1 && jumps < 999)
        jumps++;
    nativeTrampolineOrdinaryJump(bean.state.y, bean.physics->num("jump-height", 74), 0,
                                 trampolineStateNative.apex);
    emitEffect(idnum(bean.physics->get("dust-actor")), request.point.x, request.point.y,
               bean.layer);
    sound("jump");
}

void tomPreSolve(b2Contact *contact, Entity *things, int count,
                 float *incomingVy = &tomOrdinaryIncomingVyNative) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int index = (int)fixtures[side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= count || things[index].kind != PLAYER ||
            fixtures[0]->IsSensor() || fixtures[1]->IsSensor())
            continue;
        *incomingVy = fixtures[side]->GetBody()->GetLinearVelocity().y;
        const b2Manifold *local = contact->GetManifold();
        if (!local->pointCount)
            continue;
        b2WorldManifold m;
        contact->GetWorldManifold(&m);
        if (local->pointCount == 2 &&
            nativeTomCollapsedManifold(Vec{m.points[0].x, m.points[0].y},
                                       Vec{m.points[1].x, m.points[1].y})) {
            contact->SetEnabled(false);
            continue;
        }
        if (local->pointCount == 1 &&
            fixtures[1 - side]->GetShape()->GetType() == b2Shape::e_chain) {
            b2Vec2 position = fixtures[side]->GetBody()->GetPosition();
            Rect r = things[index].hit;
            r = Rect{position.x + r.l * metres, position.y + r.b * metres,
                     position.x + r.r * metres, position.y + r.t * metres};
            if (nativeTomLoopCorner(r, Vec{m.points[0].x, m.points[0].y},
                                    Vec{m.normal.x, m.normal.y}))
                contact->SetEnabled(false);
        }
    }
}

void helperPreSolve(b2Contact *contact);
void recordHelperSolvedContactNative(b2Contact *contact, Entity *things, int count, bool *solved);
void helperPerchBeginContactNative(b2Contact *contact, Entity *things, int count, int player);
void frogPreSolve(b2Contact *contact);
void frogPostSolveNative(b2Contact *contact);

class OriginalContacts : public b2ContactListener {
  public:
    void BeginContact(b2Contact *contact) {
        singleCollectableBeginContactNative(contact);
        helperPerchBeginContactNative(contact, entities, entityCount, playerIndex);
        beginOneWayContact(contact);
        spikeGroupBeginContact(contact);
        forceAreaBeginContact(contact);
        waterContact(contact);
        nativeWaterContact(contact, true);
        mechanismBeginContact(contact);
    }

    void EndContact(b2Contact *contact) {
        jumperNativeEndContact(contact);
        forceAreaEndContact(contact);
        waterContact(contact);
        nativeWaterContact(contact, false);
        forgetOneWayContact(contact);
    }

    void PreSolve(b2Contact *contact, const b2Manifold *) {
        tomPreSolve(contact, entities, entityCount);
        frogPreSolve(contact);
        oneWayContacts(contact, entities, entityCount);
        helperPreSolve(contact);
        projectilePreSolve(contact);
        oscillatorProxyContacts(contact);
        killableContactPreSolve(contact);
        nativeMotionProxyPreSolve(contact, entities, entityCount, diagonalRuntime,
                                  oscillatorRuntime);
        mechanismPreSolve(contact);
        recordPinballDynamicNative(contact, entities, entityCount, playerIndex,
                                   campaignPinballDynamicNative);
        nativePinballPreSolve(contact, entities, entityCount, playerIndex, pinballPlayerStepNative);
    }

    void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {
        creaturePressurePostSolve(contact, impulse, entities, entityCount, bodies,
                                  campaignCreaturePressure, killableContactPending);
        oneWaySupportPostSolveNative(contact);
        wizardPostSolveNative(contact);
        frogPostSolveNative(contact);
        walkPostSolveNative(contact, entities, entityCount);
        recordHelperSolvedContactNative(contact, entities, entityCount, helperSolvedContactsNative);
        breakablePostSolveNative(contact);
        tomOrdinaryPostSolveNative(contact);
        tomJumpDiagnosticPostSolve(contact, tomOrdinaryIncomingVyNative);
        jumperNativePostSolve(contact);
        projectilePostSolve(contact);
        mechanismPostSolve(contact);
        springPlayerPostSolveNative(contact);
        touchAlternatorPostSolveNative(contact);
        levelEndPostSolveNative(contact);
        trampolineCampaignPostSolveNative(contact);
        crumblingCloudPostSolve(contact, entities, entityCount, playerIndex);
        killableDeadlyPostSolve(contact);
    }
};

OriginalContacts *originalContacts;

// Until Snapshot24, walker gravityScale=0 encoded the port's manual
// integration strategy. Upgrade that old implementation detail from authored
// BoxPhysics configuration; Snapshot25 preserves explicit runtime flags.
void restoreLegacyWalkerGravityNative() {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].walk && bodies[i])
            bodies[i]->SetGravityScale(
                entities[i].physics && entities[i].physics->flag("is-gravitated", true) ? 1.0f
                                                                                        : 0.0f);
}

// Snapshot25 and earlier encode manual integration as gravityScale0 for
// jumpers. Preserve current26 explicit body flags, upgrade only old files.
void restoreLegacyJumperGravityNative() {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->has("EnemyJumperBehaviour") && bodies[i])
            bodies[i]->SetGravityScale(
                entities[i].physics && entities[i].physics->flag("is-gravitated", true) ? 1.0f
                                                                                        : 0.0f);
}

// Versions24..26 stored manual rotor gravity and missing seesaw gravity as zero.
void restoreLegacyRotorGravityNative() {
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.rotate && bodies[i])
            bodies[i]->SetGravityScale(e.physics && e.physics->flag("is-gravitated", true) ? 1.0f
                                                                                           : 0.0f);
    }
}

void restoreLegacyFreeEnemyGravityNative() {
    for (int i = 0; i < entityCount; i++)
        if (nativeFreeEnemyGravity(entities[i]) && bodies[i])
            bodies[i]->SetGravityScale(1.0f);
}

void restorePhysics() {
    killableContactReset();
    ordinaryJumpStateNative = OrdinaryJumpStateNative{};
    ordinaryJumpLaunchedNative = false;
    resetBreakableDamageNative();
    memset(helperSolvedContactsNative, 0, sizeof(helperSolvedContactsNative));
    resetSpringPlayerStepNative();
    resetTouchAlternatorNative();
    resetLevelEndContactsNative();
    resetSingleCollectableNative();
    nativePinballReset(pinballPlayerStepNative);
    resetPinballDynamicNative(campaignPinballDynamicNative);
    clearPinballPendingNative();
    resetTrampolineStateNative(playerIndex >= 0 ? entities[playerIndex].state.y : 0);
    resetJumperWaterPose();
    jumperNativeClearGround();
    checkpointResetContacts();
    suppressWaterSplash = true;
    if (physicsWorld) {
        physicsWorld->ClearForces();
        clearOneWayWorld(physicsWorld);
    }
    memset(switchPending, 0, sizeof(switchPending));
    memset(shotContacts, 0, sizeof(shotContacts));
    configureFrogs(false);
    if (!physicsWorld) {
        rebuildPhysics();
        return;
    }
    for (int i = 0; i < entityCount; i++)
        if (bodies[i]) {
            Entity &e = entities[i];
            b2Body &b = *bodies[i];
            if (boxGroupMatrix(e))
                e.state.angle = e.source->angle;
            b.SetTransform(b2Vec2(e.state.x * metres, e.state.y * metres), nativePhysicsAngle(e));
            b.SetLinearVelocity(b2Vec2(e.state.vx * metres, e.state.vy * metres));
            b.SetAngularVelocity((e.dynamic && !e.walk && !e.move &&
                                  !e.source->has("DiagonalBehaviour") &&
                                  !e.source->has("EnemyJumperBehaviour")) ||
                                         e.rotate
                                     ? e.state.aux
                                     : 0);
            b.SetAwake(true);
            b.SetActive((e.kind == COIN ? coinGroupAvailableNative(e.source, entities, entityCount)
                                        : !e.state.gone) &&
                        !(e.crumble && e.state.active == 2) &&
                        ((!proximityControlled(e) && !e.motion) || e.state.active) &&
                        (e.kind != BOSS || e.state.active <= 3) &&
                        (e.kind != ALTERNATOR || activeBlock(e)));
        }
    for (int i = 0; i < entityCount; i++)
        if (oscillatorProxies[i]) {
            oscillatorProxies[i]->SetTransform(bodies[i]->GetPosition(), bodies[i]->GetAngle());
            oscillatorProxies[i]->SetLinearVelocity(bodies[i]->GetLinearVelocity());
            oscillatorProxies[i]->SetActive(bodies[i]->IsActive());
        }
    if (playerIndex >= 0 && bodies[playerIndex])
        for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext())
            if (!f->IsSensor()) {
                b2Filter filter = f->GetFilterData();
                filter.categoryBits = playerWalkMode ? 4 : 2;
                f->SetFilterData(filter);
            }
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (bullets[i].life <= 0) {
            if (shotBodies[i]) {
                physicsWorld->DestroyBody(shotBodies[i]);
                shotBodies[i] = 0;
            }
        } else {
            ensureShotBodyClass(i);
            shotBodies[i]->SetActive(shotFlyingNative(bullets[i]));
            shotBodies[i]->SetTransform(
                b2Vec2(bullets[i].state.x * metres, bullets[i].state.y * metres), 0);
            shotBodies[i]->SetLinearVelocity(
                b2Vec2(bullets[i].state.vx * metres, bullets[i].state.vy * metres));
            shotContacts[i].first = true;
            shotContacts[i].lastTime = bullets[i].state.timer;
            int owner = bullets[i].state.active - 1;
            if (owner >= 0 && owner < entityCount)
                shotContacts[i].ignored[owner / 32] |= 1u << (owner % 32);
        }
    }
    memset(playerTouches, 0, sizeof(playerTouches));
    memset(playerShotTouches, 0, sizeof(playerShotTouches));
    onGround = false;
    supportEntity = -1;
}

// Retain Box2D units exactly, matching native body offsets0x54/0x58/0x5c.
b2Body *checkpointBodyNative(int key) {
    if (key < MAX_ENTITIES)
        return key < entityCount ? bodies[key] : 0;
    if (key < MAX_ENTITIES + MAX_PROJECTILES)
        return shotBodies[key - MAX_ENTITIES];
    int index = key - MAX_ENTITIES - MAX_PROJECTILES;
    return index < entityCount ? oscillatorProxies[index] : 0;
}

void captureBodyPhysicsNative(BodyPhysicsCheckpointNative *records) {
    for (int key = 0; key < NATIVE_PHYSICS_CHECKPOINT_BODIES; key++) {
        BodyPhysicsCheckpointNative &r = records[key];
        r = BodyPhysicsCheckpointNative{};
        b2Body *b = checkpointBodyNative(key);
        if (!b)
            continue;
        const b2Vec2 &force = b->GetCheckpointForce();
        r.force = Vec{force.x, force.y};
        r.torque = b->GetCheckpointTorque();
        r.gravityScale = b->GetGravityScale();
        r.valid = 1;
    }
}

void restoreBodyPhysicsNative(const BodyPhysicsCheckpointNative *records) {
    for (int key = 0; key < NATIVE_PHYSICS_CHECKPOINT_BODIES; key++) {
        const BodyPhysicsCheckpointNative &r = records[key];
        b2Body *b = checkpointBodyNative(key);
        if (!r.valid || !b)
            continue;
        b->SetGravityScale(r.gravityScale);
        if (r.force.x != 0 || r.force.y != 0)
            b->ApplyForce(b2Vec2(r.force.x, r.force.y), b->GetWorldCenter());
        if (r.torque != 0)
            b->ApplyTorque(r.torque);
    }
}

void deactivateGoalBodies() {
    for (int i = 0; i < entityCount; i++)
        if (bodies[i] && (entities[i].kind == PLAYER || entities[i].kind == EXIT))
            bodies[i]->SetActive(false);
}

#include "oneway_checkpoint_native.h"

// Create a fresh campaign world from restored entities, then rebuild controller and contact bookkeeping.
void rebuildPhysics() {
    killableContactReset();
    ordinaryJumpStateNative = OrdinaryJumpStateNative{};
    ordinaryJumpLaunchedNative = false;
    resetBreakableDamageNative();
    memset(helperSolvedContactsNative, 0, sizeof(helperSolvedContactsNative));
    resetSpringPlayerStepNative();
    resetTouchAlternatorNative();
    resetLevelEndContactsNative();
    resetSingleCollectableNative();
    nativePinballReset(pinballPlayerStepNative);
    resetPinballDynamicNative(campaignPinballDynamicNative);
    clearPinballPendingNative();
    resetTrampolineStateNative(playerIndex >= 0 ? entities[playerIndex].state.y : 0);
    resetJumperWaterPose();
    jumperNativeClearGround();
    checkpointResetContacts();
    forceAreasResetContacts();
    suppressWaterSplash = true;
    if (physicsWorld) {
        clearNativeWaterWorld(physicsWorld);
        clearOneWayWorld(physicsWorld);
        delete physicsWorld;
    }
    physicsWorld = new b2World(b2Vec2(0, nativeWorldGravity * metres), true);
    if (!originalContacts)
        originalContacts = new OriginalContacts;
    physicsWorld->SetContactListener(originalContacts);
    memset(bodies, 0, sizeof(bodies));
    memset(shotBodies, 0, sizeof(shotBodies));
    memset(hinges, 0, sizeof(hinges));
    createPhysicsTerrain(*physicsWorld, current);
    memset(oscillatorProxies, 0, sizeof(oscillatorProxies));
    for (int i = 0; i < entityCount; i++)
        bodies[i] =
            createPhysicsEntity(*physicsWorld, entities[i], i, &hinges[i], &oscillatorProxies[i]);
    restoreCoinGroupActivityNative(entities, entityCount, bodies);
}

bool sensorBodyContact(b2Body *sensor, b2Body *player) {
    if (!sensor || !player || !sensor->IsActive() || !player->IsActive())
        return false;
    for (b2ContactEdge *edge = sensor->GetContactList(); edge; edge = edge->next)
        if (edge->other == player && edge->contact->IsTouching())
            return true;
    return false;
}

// CollectableObjectGroup::BeginContact0x16dd3c..0x16e258 accepts a
// BasePhysics actor containing HelperBirdBehaviour as well as Tom. The
// single CollectableObject callback0x1660a4 accepts Tom only.
bool helperGemContactNative(int index) {
    if (index < 0 || index >= entityCount || entities[index].kind != COIN || !bodies[index] ||
        !bodies[index]->IsActive())
        return false;
    for (b2ContactEdge *edge = bodies[index]->GetContactList(); edge; edge = edge->next) {
        int other = (int)edge->other->GetUserData() - 1;
        if (other >= 0 && other < entityCount && entities[other].kind == HELPER &&
            !entities[other].state.gone && edge->other->IsActive() && edge->contact->IsTouching() &&
            edge->contact->IsEnabled())
            return true;
    }
    return false;
}

bool playerContact(int index) {
    return index >= 0 && index < entityCount && playerTouches[index];
}

bool enemyGrounded(int index) {
    if (index < 0 || index >= entityCount || !bodies[index])
        return false;
    b2Body *body = bodies[index];
    for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (!c->IsTouching() || !c->IsEnabled() || !c->GetManifold()->pointCount ||
            c->GetFixtureA()->IsSensor() || c->GetFixtureB()->IsSensor())
            continue;
        b2WorldManifold m;
        c->GetWorldManifold(&m);
        float ny = c->GetFixtureA()->GetBody() == body ? -m.normal.y : m.normal.y;
        if (ny > 0.95f)
            return true;
    }
    return false;
}

bool walkDetector(int index, int detector, b2Fixture *against = 0) {
    return walkDetectorContainsNative(bodies[index], detector, against);
}

void updateWalkContacts(int index) {
    Entity &e = entities[index];
    if (!e.walk || !bodies[index] || !e.state.active || e.state.gone)
        return;
    // Native PostPhysics0x328618..70 handles only edge avoidance here.
    if (e.reverseEdge && e.walkCanWalk) {
        bool left = walkDetector(index, 5), right = walkDetector(index, 6);
        if (!walkCloudSpringStepNative(e, bodies[index], entities, entityCount, left, right)) {
            if (left && !right)
                nativeWalkDirection(e, true);
            else if (right && !left)
                nativeWalkDirection(e, false);
        }
    }
}

// Synchronize actor targets, step Box2D, then consume solved contacts while the world is unlocked.
void simulateBodies(float dt) {
    pruneOneWayRefusalsNative(physicsWorld);
    for (int i = 0; i < entityCount; i++) {
        b2Body *body = bodies[i];
        if (!body)
            continue;
        Entity &e = entities[i];
        // FinalBoss SetPosition feeds BasePhysics::OnActorChanged0x11f3f8
        // even while its fixture is inactive. Keep the body at that pose:
        // an old arena-to-new-arena velocity would hit Box2D's128px step cap.
        if (e.kind == BOSS) {
            body->SetTransform(b2Vec2(e.state.x * metres, e.state.y * metres),
                               nativePhysicsAngle(e));
            body->SetLinearVelocity(b2Vec2(0, 0));
        }
        body->SetActive((e.kind == COIN ? coinGroupAvailableNative(e.source, entities, entityCount)
                                        : !e.state.gone) &&
                        !(e.crumble && e.state.active == 2) &&
                        ((!proximityControlled(e) && !e.motion) || e.state.active) &&
                        (e.kind != BOSS || e.state.active <= 3) &&
                        (e.kind != ALTERNATOR || activeBlock(e)));
        if (oscillatorProxies[i])
            oscillatorProxies[i]->SetActive(body->IsActive());
        if (!body->IsActive())
            continue;
        b2Vec2 target(e.state.x * metres, e.state.y * metres);
        float angle = nativePhysicsAngle(e);
        if (body->GetType() == b2_kinematicBody) {
            if ((e.move || e.source->has("DiagonalBehaviour")) &&
                (target - body->GetPosition()).LengthSquared() >
                    (0.5f * metres) * (0.5f * metres)) {
                body->SetTransform(target, angle);
                if (oscillatorProxies[i])
                    oscillatorProxies[i]->SetTransform(target, angle);
            }
            body->SetLinearVelocity((e.move || e.source->has("DiagonalBehaviour"))
                                        ? b2Vec2(e.state.vx * metres, e.state.vy * metres)
                                        : (1.0f / dt) * (target - body->GetPosition()));
            body->SetAngularVelocity((angle - body->GetAngle()) / dt);
        } else if (body->GetType() == b2_dynamicBody) {
            if ((target - body->GetPosition()).LengthSquared() > (0.5f * metres) * (0.5f * metres))
                body->SetTransform(target, angle);
            b2Vec2 velocity(e.state.vx * metres, e.state.vy * metres);
            if (!nativeFreeBodyGravity(e) && (velocity.x != body->GetLinearVelocity().x ||
                                              velocity.y != body->GetLinearVelocity().y))
                body->SetLinearVelocity(velocity);
            if (hinges[i] && e.rotate == 2)
                updateSeesaw(e, *body, *hinges[i]);
        } else if (target.x != body->GetPosition().x || target.y != body->GetPosition().y ||
                   angle != body->GetAngle())
            body->SetTransform(target, angle);
    }
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Bullet &b = bullets[i];
        if (b.life <= 0) {
            if (shotBodies[i]) {
                physicsWorld->DestroyBody(shotBodies[i]);
                shotBodies[i] = 0;
            }
            continue;
        }
        if (b.state.gone) {
            if (shotBodies[i])
                shotBodies[i]->SetActive(false);
            continue;
        }
        prepareShot(i);
        ensureShotBodyClass(i);
        shotBodies[i]->SetActive(true);
        b2Vec2 target(b.state.x * metres, b.state.y * metres);
        if ((target - shotBodies[i]->GetPosition()).LengthSquared() > 0.001f)
            shotBodies[i]->SetTransform(target, 0);
        if (projectileGravitated(classes[b.klass]))
            b.state.vy -= 500 * dt;
        shotBodies[i]->SetLinearVelocity(b2Vec2(b.state.vx * metres, b.state.vy * metres));
    }
    memset(helperSolvedContactsNative, 0, sizeof(helperSolvedContactsNative));
    killableContactReset();
    spikeGroupContactReset();
    mechanismsPreStep();
    resetSpringPlayerStepNative();
    resetLevelEndContactsNative();
    resetSingleCollectableNative();
    nativePinballReset(pinballPlayerStepNative);
    resetPinballDynamicNative(campaignPinballDynamicNative);
    trampolineContactThisStepNative = false;
    tomOrdinaryNormalFoundNative = false;
    physicsWorld->Step(dt, 10, 10);
    killableContactPostStep();
    spikeGroupContactPostStep();
    physicsWorld->ClearForces();
    memset(playerTouches, 0, sizeof(playerTouches));
    memset(playerShotTouches, 0, sizeof(playerShotTouches));
    supportEntity = -1;
    onGround = false;
    for (int i = 0; i < entityCount; i++)
        if (bodies[i] && bodies[i]->IsActive()) {
            Entity &e = entities[i];
            b2Body &body = *bodies[i];
            b2Vec2 pos = body.GetPosition(), vel = body.GetLinearVelocity();
            e.state.x = pos.x / metres;
            e.state.y = pos.y / metres;
            e.state.vx = vel.x / metres;
            e.state.vy = vel.y / metres;
            if (!boxGroupMatrix(e))
                e.state.angle = body.GetAngle() * 57.29578f;
            if ((e.dynamic && !e.walk && !e.move && !e.source->has("DiagonalBehaviour") &&
                 !e.source->has("EnemyJumperBehaviour")) ||
                e.rotate)
                e.state.aux = body.GetAngularVelocity();
        }
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (shotBodies[i] && shotBodies[i]->IsActive() && shotFlyingNative(bullets[i])) {
            b2Vec2 p = shotBodies[i]->GetPosition(), vel = shotBodies[i]->GetLinearVelocity();
            bullets[i].state.x = p.x / metres;
            bullets[i].state.y = p.y / metres;
            bullets[i].state.vx = vel.x / metres;
            bullets[i].state.vy = vel.y / metres;
        }
    b2Body *player = bodies[playerIndex];
    if (!player)
        return;
    for (b2ContactEdge *edge = player->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (!c->IsTouching() || !c->IsEnabled() || c->GetManifold()->pointCount == 0)
            continue;
        int index = (int)edge->other->GetUserData() - 1;
        if (index >= 0 && index < entityCount)
            playerTouches[index] = true;
        if (index >= MAX_ENTITIES && index < MAX_ENTITIES + MAX_PROJECTILES)
            playerShotTouches[index - MAX_ENTITIES] = true;
        b2WorldManifold manifold;
        c->GetWorldManifold(&manifold);
        float ny = c->GetFixtureA()->GetBody() == player ? -manifold.normal.y : manifold.normal.y;
        if (ny >= 0.5f && index < MAX_ENTITIES) {
            onGround = true;
            supportEntity = index;
        }
    }
    // Consume after copying Tom contacts, outside the locked solver.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].crumble && entities[i].state.active == 1)
            burstStageCloud(entities[i], i);
}

#include "helper_bird.h"
#include "frog.h"

// The wizard's five arena offsets and extra alternation masks come from the
// authored FinalBossBehaviour. State stays in the checkpointed entity fields.
void bossAlternate(Entity &boss, bool open) {
    const Props &bp = *boss.source->behavior("FinalBossBehaviour");
    int area = 5 - boss.state.hp;
    int extra =
        (int)listNumber(bp.get(open ? "area-end-extra-alternate" : "area-start-extra-alternate"),
                        (int)clamp((float)(area - (open ? 1 : 0)), 0, 4));
    // FinalBoss::DoAlternate 0x1d4d4c/0x1d5038 sends4|extra to each
    // listener; AlternatorBlock::DoAlternate 0xeced0 TOGGLES on overlap.
    // Every authored Wizard listener has one mask bit, so event parity is
    // represented by XOR. Do not replace the cumulative8/16 history.
    // Native DoAlternate(false)0x1d505c..78 plays once before listeners.
    if (!open)
        sound("switch");
    alternateScene(4 | extra);
}

void wizardShot(Entity &boss, float side) {
    const Props &bp = *boss.source->behavior("FinalBossBehaviour");
    Object *cls = classById(idnum(bp.get("bullet-class")));
    if (!cls)
        return;
    State &p = entities[playerIndex].state;
    int used = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (bullets[i].life > 0 && bullets[i].klass == (int)(cls - classes))
            used++;
    if (used >= bp.num("bullet-pool-size", 10))
        return;
    int hand = side < 0 ? 0 : 1;
    WizardArmPose pose =
        wizardArmPose(Vec{boss.state.x, boss.state.y}, hand == 1, 1, wizardArms[hand]);
    Vec aim = wizardHandAim(pose.hand, Vec{p.x, p.y});
    float direction = bqAtan2(aim.y, aim.x) * 57.29578f;
    if (direction < 140 && direction > -90)
        return;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (bullets[i].life <= 0) {
            int member =
                freeProjectileAnimationNative((int)(&boss - entities), (int)(cls - classes));
            if (member < 0)
                return;
            Bullet &b = bullets[i];
            memset(&b, 0, sizeof(b));
            projectileAnimationsNative.binding[i] = member;
            b.life = 1;
            b.klass = (int)(cls - classes);
            b.state.active = (int)(&boss - entities) + 1;
            resetProjectileLaunchContactsNative(i, b.state.active - 1);
            b.state.hp = hand + 1;
            b.state.x = pose.hand.x;
            b.state.y = pose.hand.y;
            float speed = cls->behavior("FinalBossBulletBehaviour")->num("speed", 180);
            b.state.vx = aim.x * speed;
            b.state.vy = aim.y * speed;
            wizardArmShot(wizardArms[hand], aim);
            actor(cls->id);
            break;
        }
}

#include "wizard_presentation.h"
void wizardStopPrespawn(Entity &boss);
void wizardStartCameraBlend();

void wizardHurt(Entity &boss) {
    if (boss.state.active != 3)
        return;
    boss.state.hp--;
    wizardHeadHurt();
    boss.state.active = boss.state.hp ? 2 : 4;
    boss.state.timer = 0;
    wizardMotion.pendingCount = 0;
    wizardMotion.leftAge = wizardMotion.rightAge = 0;
    wizardStopPrespawn(boss);
    if (boss.state.hp) {
        wizardStartCameraBlend();
        bossAlternate(boss, true);
    }
    sound(boss.state.hp ? "malmo-hurt" : "malmo-death");
    if (!boss.state.hp) {
        stopMusic();
        entityEffect(boss, "FinalBossBehaviour", "death-effect");
    } // Enemy::Despawn0x1d6d30 / Obstacle::Despawn0x1d7890 explode the
    // shared spawn effect at Actor::GetPosition before instant deactivation.
    // Native IsDead0x237c40 excludes only the hidden terminal state; a
    // still-visible dying actor remains eligible for this dismissal puff.
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (!e.bossSpawn)
            continue;
        if (!e.state.gone || killableDeathVisibleNative(e))
            emitEffect(idnum(boss.source->behavior("FinalBossBehaviour")->get("spawn-effect")),
                       e.state.x, e.state.y, e.layer);
        e.state.gone = 1;
        if (bodies[i])
            bodies[i]->SetActive(false);
        if (oscillatorProxies[i])
            oscillatorProxies[i]->SetActive(false);
    }
    for (int i = 0; i < MAX_PROJECTILES; i++)
        bullets[i].life = 0;
    // Tom::MegaJump normalizes the authored direction and derives launch
    // speed from its power (0x3129e0); the final hit has a different vector.
    Entity &bean = entities[playerIndex];
    const Props &props = *bean.physics;
    float dx = boss.state.hp ? -0.75f : -0.45f;
    nativeMegaJumpRequest((int)(&boss - entities), Vec{dx, .35f}, props.num("gravity", -400),
                          props.num("jump-height", 74), 0, 350);
}

bool wizardInputLocked() {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS && !entities[i].state.gone && entities[i].state.active == 4 &&
            entities[i].state.hp == 0)
            return true;
    return false;
}

void wizardArmShots(Entity &boss, float previous, float now, bool combat) {
    // Arm clocks start independently at 4.3/4.8 (period 5) or 1.3/1.8
    // (period 2). Deriving them from the checkpointed state clock preserves
    // their phase over retry without changing the save layout.
    float period = combat ? 2.0f : 5.0f;
    for (int hand = 0; hand < 2; hand++) {
        float phase = combat ? (hand ? 1.8f : 1.3f) : (hand ? 4.8f : 4.3f);
        if ((int)((now + phase + 0.5f) / period) > (int)((previous + phase + 0.5f) / period))
            wizardTroll();
        if ((int)((now + phase) / period) > (int)((previous + phase) / period))
            wizardShot(boss, hand ? 1.0f : -1.0f);
    }
}

#include "wizard_motion.h"

// FinalBoss::PrePhysicsStep0x1db194..228 uses fmod wrap and a2.4s
// per-event envelope, independently of the3s death animation.
float wizardDeathSoundGainNative(float oldAge, float age) {
    float oldPhase = nativeMotionRemainder(oldAge, .4f);
    if (oldAge < 0)
        oldPhase += .4f;
    if (nativeMotionRemainder(age, .4f) >= oldPhase)
        return 0;
    volatile float ratio = age / 2.4f;
    if (ratio <= 0)
        return 1;
    if (ratio >= 1)
        return 0;
    volatile float gain = 1 - ratio;
    return gain;
}

void updateWizard(Entity &boss, float dt) {
    State &s = boss.state;
    float previous = s.timer;
    s.timer += dt;
    wizardPresentationTick(dt);
    if (s.active <= 3)
        wizardNativeAliveStep(boss, dt);
    else if (s.active == 4 && !s.hp) {
        float gain = wizardDeathSoundGainNative(previous, s.timer);
        if (gain > 0)
            sound("malmo-death", 1, gain);
        if (s.timer >= 3) {
            s.gone = 1;
            s.active = 5;
            wizardStartCameraBlend();
            bossAlternate(boss, true);
        }
    }
    int mode = s.active == 4 ? 2 : 1;
    for (int hand = 0; hand < 2; hand++)
        wizardArmTick(wizardArms[hand], dt, mode);
}

void applyEndingSequencePose() {
    EndingTransformationPose pose = endingTransformationPose(endingScene.input, endingScene.age);
    Entity &emilio = entities[endingScene.emilio];
    Entity &maria = entities[endingScene.maria];
    emilio.state.x = pose.emilioX;
    emilio.state.y = pose.emilioY;
    maria.state.x = pose.mariaX;
    maria.state.y = pose.mariaY;
    camX = pose.cameraX - viewW * 0.5f;
    cameraState.x = camX;
}

void beginPlayerWalkMode() {
    if (playerIndex < 0)
        return;
    playerWalkMode = true;
    playerWalkMoving = false;
    playerWalkAge = playerWalkIdleAge = 0;
    State &s = entities[playerIndex].state;
    s.vy = minf(s.vy, 0);
    if (bodies[playerIndex]) {
        b2Vec2 velocity = bodies[playerIndex]->GetLinearVelocity();
        velocity.y = minf(velocity.y, 0);
        bodies[playerIndex]->SetLinearVelocity(velocity);
        for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext())
            if (!f->IsSensor()) {
                b2Filter filter = f->GetFilterData();
                filter.categoryBits = 4;
                f->SetFilterData(filter);
            }
    }
    sound("axolotl");
}

// PostPhysicsCollected0x166f88..fa8 disables physics and visibility before
// calling the derived OnCollected; its effect is emitted afterwards.
void singleCollectablePostStepNative() {
    int count = singleCollectableEventCountNative;
    singleCollectableEventCountNative = 0;
    for (int n = 0; n < count; n++) {
        const SingleCollectableEventNative &event = singleCollectableEventsNative[n];
        int i = event.source;
        if (i < 0 || i >= entityCount || bodies[i] != event.body)
            continue;
        Entity &e = entities[i];
        if (e.kind == COIN) {
            e.state.gone = 1;
            coins++;
            sound("coin");
            entityEffect(e, "CoinGroupBehaviour", "effect");
            coinGroupActivityNative(e.source, entities, entityCount, bodies,
                                    coinCollectablePendingNative);
            continue;
        }
        if (e.kind != SALAMANDER && e.kind != POTION)
            continue;
        bodies[i]->SetActive(false);
        e.state.gone = 1;
        if (e.kind == SALAMANDER) {
            salamander = 1;
            sound("axolotl");
            entityEffect(e, "CollectableSalamanderBehaviour", "effect");
        } else {
            beginPlayerWalkMode();
            entityEffect(e, "UnbeanPotionBehaviour", "effect");
        }
    }
    memset(coinCollectablePendingNative, 0, sizeof(coinCollectablePendingNative));
}

bool beginEndingSequence() {
    if (endingScene.active)
        return true;
    const Props *config = 0;
    for (int i = 0; i < current.objectCount; i++)
        if (current.objects[i].has("EndOfGameCutsceneController")) {
            config = current.objects[i].behavior("EndOfGameCutsceneController");
            break;
        }
    if (!config)
        return false;
    int emilio = -1, maria = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].source->id == idnum(config->get("emilio")))
            emilio = i;
        if (entities[i].source->id == idnum(config->get("maria")))
            maria = i;
    }
    if (emilio < 0 || maria < 0)
        return false;
    endingScene.emilio = emilio;
    endingScene.maria = maria;
    endingScene.next = idnum(config->get("next-level"));
    endingScene.input = EndingTransformationInput{
        entities[maria].state.x, entities[maria].state.y, entities[emilio].state.y,
        entities[playerIndex].state.x, camX + viewW * 0.5f};
    endingScene.age = 0;
    endingScene.active = 1;
    if (bodies[playerIndex])
        bodies[playerIndex]->SetActive(false);
    applyEndingSequencePose();
    return true;
}

// Preserve the native task/update/physics order: checkpoint requests and launches depend on this boundary.
void tick(float dt, bool left, bool right) {
    if (noticeTimer > 0)
        noticeTimer -= dt;
    if (endingScene.active && screen == PLAY) {
        endingScene.age += dt;
        elapsed += dt;
        tickEffects(dt);
        nativeContinuousAdvanceScene(current, 1, elapsed, dt);
        applyEndingSequencePose();
        if (endingScene.age >= 10 && interfaceEnabled)
            endGameInterface();
        return;
    }
    if (respawnBusy(respawnTransition) && (screen == PLAY || screen == PAUSE)) {
        bool paused = screen == PAUSE;
        tickEffects(dt);
        if (respawnTransition.request == RESPAWN_DEATH) {
            float previous = deathAge;
            deathAge += dt;
            if (previous <= respawnNativeDeathSoundSeconds &&
                deathAge > respawnNativeDeathSoundSeconds)
                sound("bean-die");
        }
        RespawnRequest action = respawnAdvance(respawnTransition, dt);
        if (action != RESPAWN_NONE) {
            if (action == RESPAWN_RESTART)
                restartStage();
            else
                restoreCheckpoint();
            readyTimer = 0;
            respawnRestored(respawnTransition);
            if (paused)
                openPause();
        }
        return;
    }
    if (complete && (screen == PLAY || screen == COMPLETE)) {
        tickEffects(dt);
        float before = goalLaunch.age;
        if (advanceGoalLaunch(entities[playerIndex], dt))
            playMusic(idnum(entities[playerIndex].physics->get("win-music")));
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == EXIT)
                entities[i].state.timer += dt;
        if (interfaceEnabled && before < 2 && goalLaunch.age >= 2)
            completeInterface();
        return;
    }
    if (screen == PAUSE) {
        advanceNativePausedStageAnimations(dt);
        advanceProjectileAnimationsNative(dt, true);
        return;
    }
    if (screen != PLAY)
        return;
    checkpointFlushSaveNative();
    if (readyTimer <= 0)
        readyOpening.showing = false;
    readyNativeAdvance(readyOpening, dt, readyPointerDown, left, right);
    readyTimer = readyOpening.showing ? maxf(0.000001f, 2.5f - readyOpening.age) : 0;
    tickEffects(dt);
    if (!physicsWorld || playerIndex < 0)
        return;
    advanceProjectileAnimationsNative(dt);
    if (wizardMotion.initialized)
        wizardCameraPreStep(dt);
    for (int i = 0; i < MAX_PROJECTILES; i++)
        advanceShotDeathNative(i, dt);
    elapsed += dt;
    nativeContinuousAdvanceScene(current, 1, elapsed, dt);
    // Original CollisionWorld visits the newest body first. The authored
    // Wizard body is created after Tom, so its pending hit must be processed
    // before Tom consumes the same-step MegaJump request.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS && !entities[i].state.gone)
            updateWizard(entities[i], dt);
    Entity &p = entities[playerIndex];
    State &ps = p.state;
    tomJumpDiagnosticBegin(dt, left, right);
    readyNativeCountMovement(ps.hp, ps.x, p.originX);
    nativeTrampolineObserve(ps.y, trampolineStateNative.apex);
    float pinballJumpAge = ps.timer;
    ps.timer += dt;
    const Props &playerProps = *p.physics;
    float maxSpeed = playerProps.num("max-velocity", 200) * (playerWalkMode ? 0.5f : 1),
          accel = playerProps.num("move-acceleration", 500);
    if (wizardInputLocked())
        left = right = false;
    if (!wizardInputLocked())
        ps.vx = nativeTomHorizontalVelocity(ps.vx, left != right ? (left ? -1 : 1) : 0, maxSpeed,
                                            accel, playerProps.num("stop-acceleration", 200), dt);
    int helper = flyingHelper();
    if (helper >= 0 &&
        ps.vy <=
            entities[helper].source->behavior("HelperBirdBehaviour")->num("max-fly-speed", 60)) {
        const Props &fly = *entities[helper].source->behavior("HelperBirdBehaviour");
        ps.vy =
            approach(ps.vy, fly.num("max-fly-speed", 60), fly.num("fly-acceleration", 1000) * dt);
    }
    bool megaPending = pinballPendingNative.pending != 0,
         specialPending = megaPending || trampolineStateNative.pending != 0;
    nativePinballConsumePending(entities, entityCount, playerIndex, deathTimer <= 0,
                                ps.hp == 1 && !ordinaryJumpStateNative.stomp, jumps,
                                pinballJumpAge);
    trampolineCampaignConsumeNative(pinballJumpAge, megaPending);
    consumeCampaignOrdinaryJumpNative(pinballJumpAge, specialPending);
    if (bodies[playerIndex])
        nativeTomGravityForce(
            *bodies[playerIndex], playerProps.num("gravity", -400), helper >= 0, ps.vy,
            helper >= 0
                ? entities[helper].source->behavior("HelperBirdBehaviour")->num("max-fly-speed", 60)
                : 0);
    preStepVy = ps.vy;
    playerWalkMoving = left != right || absf(ps.vx) > 5;
    if (playerWalkMode) {
        if (playerWalkMoving)
            playerWalkAge += dt;
        else
            playerWalkIdleAge += dt;
    }
    if (left != right)
        ps.aux = left ? -1.0f : 1.0f;
    ps.active = 1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == MAGNET)
            entities[i].state.active = 1;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        State &s = e.state;
        if (i == playerIndex)
            continue;
        if (s.gone) {
            advanceKillableDeathNative(e, dt);
            continue;
        }
        if (!proximityControlled(e) || s.active || nativeActorAnimateWhilePaused(e)) {
            advanceJumperAnimationNative(e, dt);
            advanceFishAnimationNative(e, dt);
        }
        if (e.kind == BOSS)
            continue;
        if (e.kind == PERCH)
            updateHelperPerchNative(e);
        if (e.kind == SPIKE)
            advanceSpikeAnimation(e, dt);
        else if (!e.touch)
            s.timer += dt;
        if (e.breakable)
            e.turnCooldown = maxf(0, e.turnCooldown - dt);
        if (s.y < -500) {
            s.gone = 1;
            continue;
        }
        if (e.kind == HELPER) {
            updateHelper(e, dt);
            continue;
        }
        if (e.kind == SECRET)
            continue; // Shared layer fade advances from solved sensor contacts below.
        if (e.move)
            selectNativeMotionSpeed(*e.move, nativeMotionPhysicsFPS(dt),
                                    oscillatorRuntime[i].effectiveSpeed,
                                    oscillatorRuntime[i].selected);
        if (e.source->has("DiagonalBehaviour"))
            selectNativeMotionSpeed(*e.source->behavior("DiagonalBehaviour"),
                                    nativeMotionPhysicsFPS(dt), diagonalRuntime[i].effectiveSpeed,
                                    diagonalRuntime[i].selected);
        if (proximityControlled(e) && !s.active) {
            if (!(nativeActorAnimateWhilePaused(e) && nativeActorPauseAnimationClockSafe(e)))
                s.timer -= dt;
            if (e.source->has("BulletSource"))
                bulletSourceStates[i].clock = e.source->behavior("BulletSource")->num("timeOffset");
            continue;
        }
        if (e.kind == ENEMY)
            e.turnCooldown = maxf(0, e.turnCooldown - dt);
        if (e.motion && e.parent >= 0) {
            State &parent = entities[e.parent].state;
            if (parent.gone || !parent.active) {
                s.timer -= dt;
                continue;
            }
            if (e.motion == 1) {
                positionWreckingChild(e);
            } else {
                float a = nativeOrbitAngle(s.timer, e.period, e.phase);
                s.x = parent.x + bqCos(a) * e.radius;
                s.y = parent.y + bqSin(a) * e.radius;
            }
            continue;
        }
        if (e.move)
            advanceNativeOscillator(i, dt);
        if (e.source->has("DiagonalBehaviour"))
            advanceNativeDiagonal(i, dt);
        if (e.orbit && s.active)
            positionNativeOrbit(e);
        if (!e.walk && !e.rotate && !e.source->has("EnemyJumperBehaviour") &&
            !nativeFreeBodyGravity(e) && e.dynamic && !e.move && !e.orbit && !e.motion &&
            !e.source->has("DiagonalBehaviour") && e.physics &&
            e.physics->flag("is-gravitated", true) &&
            (!bodies[i] || bodies[i]->GetGravityScale() == 0))
            s.vy += nativeWorldGravity * dt;
        if (e.crumble && s.active == 1)
            burstStageCloud(e, i);
        else if (e.crumble && s.active == 2 &&
                 s.timer - dt > e.source->behavior("CrumblingPlatformBehaviour")->num("Time", 3)) {
            s.active = 0;
            s.timer = 0;
        }
        if (e.kind == ENEMY && s.active && !e.move && !e.orbit) {
            if (e.walk) {
                walkPrePhysicsNative(e);
            } else if (e.source->has("EnemyJumperBehaviour") && !frogJumperStep(i, dt)) {
                const Props &jb = *e.source->behavior("EnemyJumperBehaviour");
                jumperNativeStep(i, dt, frogJumperDelay(i, jb.num("jump-delay", 1)), false);
            }
        }
        if (e.source->has("BulletSource"))
            updateBulletSource(e, dt);
    }
    checkpointPrePhysics();
    syncAttachedActors();
    simulateBodies(dt);
    advanceNativeSecretLayers(entities, entityCount, bodies, playerIndex, dt);
    checkpointObserveContacts();
    projectilePostStep();
    mechanismsPostStep(dt);
    touchAlternatorPostStepNative(dt);
    checkPlayerCompression();
    waterPostStep();
    advanceJumperWaterPose(dt);
    for (int i = 0; i < entityCount; i++)
        if (!entities[i].state.gone)
            selectJumperAnimationNative(entities[i], !jumperWaterStanding(entities[i]) &&
                                                         entities[i].state.hp != 0);
    forceAreasPostStep(dt);
    frogPostStep();
    helperPostStep(dt);
    updateBreakableContacts();
    advanceProximityAfterPhysicsNative(entities, entityCount, ps, bodies, oscillatorProxies);
    if (endingScene.active) {
        singleCollectablePostStepNative();
        tomJumpDiagnosticDecision();
        tomJumpDiagnosticEnd();
        return;
    }

    tomJumpDiagnosticDecision();
    bool levelEndReachedNative = levelEndPostStepNative();
    singleCollectablePostStepNative();
    if (levelEndReachedNative) {
        tomJumpDiagnosticBranch(p.jumpPower, true);
        tomJumpDiagnosticEnd();
        return;
    }
    nativePinballFinishContacts(entities, entityCount, pinballPlayerStepNative);
    springPlayerFinishContactsNative();
    if (pinballLaunchedThisStepNative) {
        tomJumpDiagnosticBranch(pinballConsumedNative.speed);
        tomJumpDiagnosticCommit();
    }
    if (trampolineLaunchedThisStepNative) {
        tomJumpDiagnosticBranch(trampolineStateNative.speed);
        tomJumpDiagnosticCommit();
    }
    // Tom::PostSolve0x312bf4..0x312ce4 latches Jump from the incoming
    // velocity and solved normal. A later TOI pass can disable that contact;
    // its final ground-list state must not cancel the already valid jump.
    if (!trampolineLaunchedThisStepNative && !trampolineContactThisStepNative &&
        !springPlayerContactThisStepNative && !pinballLaunchedThisStepNative &&
        !pinballPlayerStepNative.pending && deathTimer <= 0 && !playerWalkMode &&
        tomOrdinaryNormalFoundNative && !killablePlayerStompedThisStep()) {
        // Tom::Magnet sets an independent per-step flag, not a chosen ground
        // owner. It suppresses normal Jump even when coincident terrain was
        // enumerated last; native MegaJump/Trampoline keep their precedence.
        if (playerMagnetContact)
            ps.active = 2;
    }
    ordinaryJumpStateNative.magnet |= playerMagnetContact ? 1 : 0;
    if (ordinaryJumpLaunchedNative) {
        tomJumpDiagnosticBranch(p.lastLaunchSpeed);
        tomJumpDiagnosticCommit();
    }
    Rect pr = hitRect(p);
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        State &s = e.state;
        if (i == playerIndex || s.gone ||
            (e.motion && e.parent >= 0 && entities[e.parent].state.gone))
            continue;
        bool sensor = e.kind == CHECKPOINT;
        if (sensor) {
            if (!sensorBodyContact(bodies[i], bodies[playerIndex]) && !helperGemContactNative(i))
                continue;
        } else if (!overlaps(pr, hitRect(e)) && !playerContact(i))
            continue;
        if (e.kind == ENEMY && bodies[i] && !playerContact(i))
            continue;
        switch (e.kind) {
        case CHECKPOINT:
            checkpointBeginNative(i);
            break;
        case EXIT:
            break; // Completion is consumed only from LevelEnd PostSolve.
        case SPIKE:
            break;
        case TURRET:
            if (!e.source->has("KillableEnemyBehaviour"))
                break;
        case ENEMY:
            if (!e.source->has("KillableEnemyBehaviour"))
                die();
            break;
        case SWITCH:
            break;
        case BOUNCER:
            break; // Real PreSolve manifold handled before the ordinary jump branch.
        case BLOCK:
            break;
        case BOSS:
            break; // Actual PostSolve queued the native hit or side jump.
        default:
            break;
        }
    }
    syncAttachedActors();
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (bullets[i].life > 0)
            bullets[i].state.timer += dt;
    for (int i = 0; i < entityCount; i++)
        updateWalkContacts(i);
    if (ps.y < -80 || ps.x < -96 || ps.x > current.size.x + 96)
        die();
    updateCamera();
    wizardPostCameraStep(dt);
    tomJumpDiagnosticEnd();
}
