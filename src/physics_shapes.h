#pragma once

// Builds authored fixtures and applies one-way contact gates, support continuity and per-world contact bookkeeping.
#include "rotation_native.h"
#include "circle_fixture_native.h"

// Original Circle/Box fixture masks use native categories loose1, terrain2,
// oscillator4, player8. Port fixtures split loose/enemy and oscillator/proxy.
// collide-other=false is native mask &= 0xc, not merely ~loose1.
unsigned short nativePhysicsMask(const Props *physics) {
    unsigned short mask = 0xffff;
    if (!physics)
        return mask;
    if (!physics->flag("collide-player", true))
        mask &= ~2;
    if (!physics->flag("collide-oscillating", true))
        mask &= ~(8 | 32);
    if (!physics->flag("collide-other", true))
        mask &= 2 | 8 | 32;
    return mask;
}

bool boxGroupMatrix(const Entity &e) {
    // AlternatorBlock ctor0xebe70 derives from BoxGroupPhysics. The touch
    // variant ctor0xff660 derives from BoxPhysics and remains per actlet.
    return e.source->base == 7 &&
           (e.source->has("BoxGroupPhysicsBehaviour") ||
            e.source->has("AlternatorBlockBehaviour") || e.source->has("SpikeGroupBehaviour"));
}

// BoxGroupPhysics::OnActivate0x1344c0 initializes body angle to zero.
// ActletMatrix rotates each sprite independently; that visual angle is not
// a rotation of the merged physical rectangle around its first actlet.
float nativePhysicsAngle(const Entity &e) {
    return boxGroupMatrix(e) ? 0 : e.state.angle * 0.017453293f;
}

bool boxGroupMatrixOwner(const Entity &e) {
    if (!boxGroupMatrix(e))
        return true;
    const Object &o = *e.source;
    float w = minf(o.props.num("actlet-width", 32), o.rect.r - o.rect.l),
          h = minf(o.props.num("actlet-height", 32), o.rect.t - o.rect.b);
    return absf(e.originX - (o.rect.l + w * 0.5f)) < 0.01f &&
           absf(e.originY - (o.rect.b + h * 0.5f)) < 0.01f;
}

Rect boxGroupMatrixFixture(const Entity &e) {
    Rect r = shifted(e.source->rect, -e.originX, -e.originY);
    if (e.physics) {
        r.l += e.physics->num("adjust-left");
        r.r += e.physics->num("adjust-right");
        r.b += e.physics->num("adjust-bottom");
        r.t += e.physics->num("adjust-top");
    }
    return r;
}

bool magnetMatrix(const Entity &e) {
    return e.kind == MAGNET && boxGroupMatrix(e);
}

bool magnetMatrixOwner(const Entity &e) {
    return boxGroupMatrixOwner(e);
}

Rect magnetMatrixFixture(const Entity &e) {
    return boxGroupMatrixFixture(e);
}

// Shared original map fixtures for the stage and independent selector worlds.
void terrainOneWayPolygon(b2PolygonShape &polygon, Vec a, Vec b) {
    if (a.x > b.x) {
        Vec swap = a;
        a = b;
        b = swap;
    }
    // LevelGeometry::Load 0x254e70..0x254f20 subtracts 8*WorldToPhysicsScale
    // from both endpoint Y coordinates, after converting points to metres.
    // Keep the top endpoints at indices0/3; slope is not a max-Y lookup.
    b2Vec2 vertices[4] = {
        b2Vec2(a.x * metres, a.y * metres), b2Vec2(a.x * metres, (a.y - 8) * metres),
        b2Vec2(b.x * metres, (b.y - 8) * metres), b2Vec2(b.x * metres, b.y * metres)};
    polygon.Set(vertices, 4);
}

bool terrainOneWaySegment(b2Fixture *fixture, b2Vec2 *a, b2Vec2 *b) {
    if (fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        const b2PolygonShape *shape = (const b2PolygonShape *)fixture->GetShape();
        if (shape->m_vertexCount != 4)
            return false;
        *a = fixture->GetBody()->GetWorldPoint(shape->m_vertices[0]);
        *b = fixture->GetBody()->GetWorldPoint(shape->m_vertices[3]);
        return true;
    }
    // Preserve compatibility with explicit edge fixtures in isolated checks.
    if (fixture->GetShape()->GetType() == b2Shape::e_edge) {
        const b2EdgeShape *shape = (const b2EdgeShape *)fixture->GetShape();
        *a = fixture->GetBody()->GetWorldPoint(shape->m_vertex1);
        *b = fixture->GetBody()->GetWorldPoint(shape->m_vertex2);
        return true;
    }
    return false;
}

void createPhysicsTerrain(b2World &world, Level &scene) {
    b2BodyDef terrainDef;
    b2Body *terrain = world.CreateBody(&terrainDef);
    for (int i = 0; i < scene.geometryCount; i++) {
        Geometry &g = scene.geometry[i];
        if (g.count < 2)
            continue;
        b2FixtureDef fd;
        fd.friction = 0.2f;
        if (g.kind == 2) {
            for (int j = 1; j < g.count; j++) {
                b2PolygonShape polygon;
                terrainOneWayPolygon(polygon,
                                     Vec{(float)g.points[j - 1].x, (float)g.points[j - 1].y},
                                     Vec{(float)g.points[j].x, (float)g.points[j].y});
                fd.shape = &polygon;
                fd.userData = (void *)1;
                terrain->CreateFixture(&fd);
            }
        } else {
            int n = g.count;
            if (g.points[0].x == g.points[n - 1].x && g.points[0].y == g.points[n - 1].y)
                n--;
            if (n < 3)
                continue;
            b2Vec2 *points = (b2Vec2 *)HeapAlloc(GetProcessHeap(), 0, sizeof(b2Vec2) * n);
            for (int j = 0; j < n; j++)
                points[j].Set(g.points[j].x * metres, g.points[j].y * metres);
            b2ChainShape chain;
            chain.CreateLoop(points, n);
            fd.shape = &chain;
            terrain->CreateFixture(&fd);
            HeapFree(GetProcessHeap(), 0, points);
        }
    }
}

// Native loose BoxPhysics bodies own their solved velocity between steps.
bool nativeLooseBoxGravity(const Entity &e) {
    return e.kind == BLOCK && e.dynamic && !e.move && !e.orbit && !e.motion && !e.crumble &&
           !e.rotate && !e.source->has("DiagonalBehaviour") && e.physics &&
           e.physics->flag("is-gravitated", true);
}

// Deadly is an independent contact behaviour; it does not change the
// BoxPhysics gravity owner of an otherwise free dynamic obstacle.
bool nativeFreeEnemyGravity(const Entity &e) {
    return e.kind == ENEMY && e.dynamic && !e.walk && !e.source->has("EnemyJumperBehaviour") &&
           !e.move && !e.orbit && !e.motion && !e.crumble && !e.rotate &&
           !e.source->has("DiagonalBehaviour") && e.physics &&
           e.physics->flag("is-gravitated", true);
}

bool nativeFreeBodyGravity(const Entity &e) {
    return nativeLooseBoxGravity(e) || nativeFreeEnemyGravity(e);
}

b2Body *createPhysicsEntity(b2World &world, Entity &e, int i, b2RevoluteJoint **hinge = 0,
                            b2Body **oscillatorProxy = 0) {
    if (e.kind == FORCE)
        return createNativeForceAreaBody(world, e, i);
    if (e.kind == SECRET)
        return createNativeSecretBody(world, e, i);
    bool physical = e.kind == SPIKE || e.kind == PLAYER || e.kind == ENEMY || solid(e) ||
                    e.kind == ALTERNATOR || e.kind == BOSS || e.kind == HELPER || e.kind == PERCH ||
                    e.kind == WATER || e.kind == BOUNCER || e.kind == SWITCH || e.kind == COIN ||
                    e.kind == SALAMANDER || e.kind == CHECKPOINT || e.kind == POTION;
    if (!physical)
        return 0;
    // BoxGroupPhysicsBehaviour::OnActivate 0x1342c0/0x1345cc/0x134740
    // creates ONE full-matrix box. Visual actlets must not introduce vertical
    // collision seams under the grounded Bean's feet.
    if (boxGroupMatrix(e) && !boxGroupMatrixOwner(e))
        return 0;
    Rect r = boxGroupMatrix(e) ? boxGroupMatrixFixture(e) : e.hit;
    bool singleCollectable = e.kind == SALAMANDER || e.kind == POTION;
    bool nativeCircle = e.circle && (e.source->has("CirclePhysicsBehaviour") || singleCollectable);
    if (!nativeCircle && e.kind != BOUNCER &&
        (r.r < r.l || r.t < r.b || (!boxGroupMatrix(e) && (r.r == r.l || r.t == r.b))))
        return 0;
    b2BodyDef bd;
    bd.position.Set(e.state.x * metres, e.state.y * metres);
    bd.angle = nativePhysicsAngle(e);
    bd.userData = (void *)(i + 1);
    bd.bullet = e.kind == PLAYER;
    bd.allowSleep = e.kind != PLAYER && !e.source->has("EnemyJumperBehaviour");
    bd.fixedRotation = e.kind == PLAYER || !e.physics || e.physics->flag("fixed-rotation", false);
    bd.gravityScale = 0;
    bool moving = e.move || e.orbit || e.motion || e.crumble ||
                  e.source->has("DiagonalBehaviour") || e.kind == BOSS;
    bool dynamic =
        e.kind == PLAYER || (!moving && e.dynamic) ||
        (e.kind == ENEMY && !moving && (e.walk || e.source->has("EnemyJumperBehaviour"))) ||
        e.rotate;
    // Native BoxPhysics::CreateBodyDef0x135f30..54 supplies world
    // gravity through the body. Keep loose crates eligible for sleep;
    // injecting a downward velocity every tick wakes a resting pile.
    if (nativeFreeBodyGravity(e) ||
        ((e.walk || e.rotate || e.source->has("EnemyJumperBehaviour")) && e.physics &&
         e.physics->flag("is-gravitated", true)))
        bd.gravityScale = 1;
    // The original diagonal controller has a dynamic collision proxy.
    // Kinematic bodies cannot collide with static map geometry.
    bd.type = e.kind == BOUNCER ? b2_staticBody
              : (singleCollectable || e.touch || e.kind == EXIT || boxGroupMatrix(e) || e.move ||
                 e.source->has("DiagonalBehaviour"))
                  ? b2_kinematicBody
              : dynamic || e.kind == HELPER ? b2_dynamicBody
              : moving                      ? b2_kinematicBody
                                            : b2_staticBody;
    // CollectableObject::OnActivate0x167498 forces kinematic type1;
    // its inherited Circle fixture becomes a sensor at0x1674b4.
    if (e.rotate) {
        bd.fixedRotation = false;
        bd.angularDamping = 0;
    }
    b2Body *body = world.CreateBody(&bd);
    b2FixtureDef fd;
    fd.density = e.physics ? e.physics->num("density", 1) : 3;
    fd.friction = e.physics ? e.physics->num("friction", 0.2f) : 0.3f;
    fd.restitution = e.physics ? e.physics->num("restitution", 0) : 0;
    fd.isSensor = e.kind == PERCH || e.kind == WATER || e.kind == COIN || e.kind == SALAMANDER ||
                  e.kind == CHECKPOINT || e.kind == POTION ||
                  (e.physics && e.physics->flag("sensor"));
    fd.filter.categoryBits = e.kind == PLAYER ? (playerWalkMode ? 4 : 2) : e.kind == ENEMY ? 16 : 4;
    // A moving carrier still owns the regular Box/Circle main fixture.
    // Oscillating OnActivate creates category4 only on its separate
    // detector, not on the main fixture. Motion does not change category.
    // Touch alternators and LevelEnd also inherit BoxPhysics's fixture
    // factory (vtable0x540430/0x546208 slot64). Their own serialized
    // behaviour supplies the same defaults as Crumbling and Seesaw.
    if (e.source->has("BoxPhysicsBehaviour") || nativeCircle || e.touch || e.kind == EXIT ||
        e.crumble || e.rotate == 2)
        nativeBoxCircleFixtureMaterial(fd, *e.physics);
    // Crumbling inherits loose BoxPhysics categories; kinematic body type
    // does not make it an OscillatingBehaviour fixture.
    if (e.crumble)
        fd.filter.categoryBits =
            4 | ((e.physics && e.physics->flag("static-position", false)) ? 1 : 0);
    // Box/Circle CreateMainFixture starts at native loose1 and adds
    // terrain2 for static-position, independently of the body type.
    // Port terrain2 maps to bit1. Crumbling overrides its body type to
    // kinematic but retains native loose1 when static-position is absent.
    if (e.physics && e.physics->flag("static-position", false))
        fd.filter.categoryBits |= 1;
    // Tom::CreateMainFixture 0x3101e0/0x3101ec overrides restitution=-1
    // and friction=0. Layout is independently confirmed by native
    // b2Fixture::Create 0x3c6c14..0x3c6c2c, not inferred from density.
    if (e.kind == PLAYER) {
        fd.friction = 0;
        fd.restitution = -1;
    }
    fd.filter.maskBits = nativePhysicsMask(e.physics);
    // Pinball::CreateMainFixture0x2c9d64 hardcodes its own material
    // and filter; it does not query the inherited Circle fixture options.
    if (e.kind == BOUNCER) {
        fd.density = 0;
        fd.friction = 0;
        fd.restitution = 1;
        fd.isSensor = false;
        fd.filter.categoryBits = 4;
        fd.filter.maskBits = 0xffff;
    }
    // BoxGroup::OnActivate 0x134488 and0x1345e4..604 supplies fixed
    // kinematic group material/category, including inherited alternators.
    if (boxGroupMatrix(e)) {
        fd.density = 0;
        fd.friction = 0.2f;
        fd.restitution = 0;
        fd.isSensor = e.kind == SPIKE && e.physics && e.physics->flag("sensor");
        fd.filter.categoryBits = 1 | 4;
    }
    b2CircleShape circle;
    b2PolygonShape box;
    if (e.circle || e.kind == BOUNCER || e.kind == COIN) {
        if (e.kind == BOUNCER) {
            circle.m_radius = 32 * metres;
            circle.m_p.Set(0, 0);
        } else if (nativeCircle)
            nativeCircleFixtureGeometry(circle, r);
        else {
            circle.m_radius =
                ((e.kind == COIN || e.kind == POTION) ? r.r - r.l : minf(r.r - r.l, r.t - r.b)) *
                0.5f * metres;
            circle.m_p.Set((r.l + r.r) * 0.5f * metres, (r.b + r.t) * 0.5f * metres);
        }
        fd.shape = &circle;
    } else {
        box.SetAsBox((r.r - r.l) * 0.5f * metres, (r.t - r.b) * 0.5f * metres,
                     b2Vec2((r.l + r.r) * 0.5f * metres, (r.b + r.t) * 0.5f * metres), 0);
        fd.shape = &box;
    }
    if (e.source->has("PlatformBehaviour") || e.crumble)
        fd.userData = (void *)2;
    if (e.kind == WATER)
        fd.userData = (void *)7;
    body->CreateFixture(&fd);
    // OscillatingBehaviour::OnActivate 0x29209c/0x29239c/0x2924c0:
    // the authored kinematic body carries actors, while a separate
    // zero-density dynamic proxy senses static walls through a joint.
    // Native category 4 / mask 6 excludes player category 8 and loose
    // dynamic category 1. Port categories are mapped explicitly here.
    if ((e.move || e.source->has("DiagonalBehaviour")) && oscillatorProxy) {
        b2BodyDef proxyDef = bd;
        proxyDef.type = b2_dynamicBody;
        proxyDef.fixedRotation = true;
        proxyDef.allowSleep = false;
        b2Body *proxy = world.CreateBody(&proxyDef);
        b2FixtureDef detector = fd;
        detector.shape = &box;
        box.SetAsBox((r.r - r.l) * 0.5f * metres, (r.t - r.b) * 0.5f * metres,
                     b2Vec2((r.l + r.r) * 0.5f * metres, (r.b + r.t) * 0.5f * metres), 0);
        detector.density = 0;
        detector.friction = 0.2f;
        detector.restitution = 0;
        detector.isSensor = false;
        detector.userData = (void *)15;
        detector.filter.categoryBits = 32;
        detector.filter.maskBits = 1 | 8 | 32;
        proxy->CreateFixture(&detector);
        b2RevoluteJointDef joint;
        joint.Initialize(body, proxy, bd.position);
        world.CreateJoint(&joint);
        *oscillatorProxy = proxy;
    }
    if (e.kind == SWITCH) {
        body->DestroyFixture(body->GetFixtureList());
        b2FixtureDef detector;
        detector.isSensor = true;
        detector.density = 0;
        detector.friction = 0.2f;
        for (int face = 0; face < 2; face++) {
            b2PolygonShape shape;
            shape.SetAsBox(12 * metres, 4 * metres, b2Vec2(0, (face ? 20 : -20) * metres), 0);
            detector.shape = &shape;
            detector.userData = (void *)(11 + face);
            body->CreateFixture(&detector);
        }
    }
    // WalkBehaviour::OnActivate 0x328de8 creates side and floor
    // tracking sensors around the adjusted main fixture, in pixels.
    if (e.walk) {
        b2FixtureDef sensor = fd;
        sensor.isSensor = true;
        sensor.density = 0;
        sensor.restitution = 0;
        sensor.friction = 0;
        b2PolygonShape detector;
        sensor.shape = &detector;
        for (int side = 0; side < 2; side++) {
            float x = side ? r.r : r.l;
            detector.SetAsBox(4 * metres, maxf(0.1f, (r.t - r.b) * 0.5f - 4) * metres,
                              b2Vec2(x * metres, (r.b + r.t) * 0.5f * metres), 0);
            sensor.userData = (void *)(3 + side);
            body->CreateFixture(&sensor);
            detector.SetAsBox(4 * metres, 4 * metres,
                              b2Vec2((x + (side ? 4 : -4)) * metres, r.b * metres), 0);
            sensor.userData = (void *)(5 + side);
            body->CreateFixture(&sensor);
        }
    }
    body->SetLinearVelocity(b2Vec2(e.state.vx * metres, e.state.vy * metres));
    if ((e.dynamic && !e.walk && !e.move && !e.source->has("DiagonalBehaviour") &&
         !e.source->has("EnemyJumperBehaviour")) ||
        e.rotate)
        body->SetAngularVelocity(e.state.aux);
    if (e.rotate) {
        b2BodyDef pivotDef;
        pivotDef.position = bd.position;
        b2Body *pivot = world.CreateBody(&pivotDef);
        b2RevoluteJointDef joint;
        joint.Initialize(pivot, body, bd.position);
        joint.enableMotor = true;
        joint.maxMotorTorque =
            e.rotate == 2 ? e.source->behavior("SeesawBehaviour")->num("rotational-friction", 1)
                          : nativeRotatingMotorPower(*e.source->behavior("RotatingBehaviour"));
        joint.motorSpeed =
            e.rotate == 1 ? nativeRotatingMotorSpeed(*e.source->behavior("RotatingBehaviour")) : 0;
        b2RevoluteJoint *created = (b2RevoluteJoint *)world.CreateJoint(&joint);
        if (hinge)
            *hinge = created;
    }
    body->SetActive(!e.state.gone && !(e.crumble && e.state.active == 2) &&
                    (!e.source->has("ProximityActivateBehaviour") || e.state.active) &&
                    (e.kind != BOSS || e.state.active <= 3) &&
                    (e.kind != ALTERNATOR || activeBlock(e)));
    if (oscillatorProxy && *oscillatorProxy)
        (*oscillatorProxy)->SetActive(body->IsActive());
    return body;
}

void oscillatorProxyContacts(b2Contact *contact) {
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();
    if (a->GetUserData() == (void *)15 || b->GetUserData() == (void *)15) {
        b2Fixture *other = a->GetUserData() == (void *)15 ? b : a;
        if (other->IsSensor() ||
            (other->GetBody()->GetType() == b2_dynamicBody && other->GetUserData() != (void *)15))
            contact->SetEnabled(false);
    }
}

// OneWayPlatform::BeginContact/EndContact keep one allowed bit per platform
// fixture + other body until ALL fixture contacts (including sensors) end.
struct OneWayEntry {
    b2Contact *contact;
    b2World *world;
    b2Fixture *platform;
    b2Body *other;
    bool above;
    bool supported;
};

OneWayEntry oneWayEntries[4096];
int oneWayEntryLimit;

struct OneWayRestoredRefusalNative {
    b2Fixture *platform;
    b2Body *other;
    b2World *world;
};

OneWayRestoredRefusalNative oneWayRestoredRefusalsNative[4096];
int oneWayRestoredRefusalCountNative;

bool oneWayFixtureBodyOverlapNative(b2Fixture *platform, b2Body *other) {
    if (platform->GetBody() == other || !platform->GetBody()->IsActive() || !other->IsActive())
        return false;
    for (b2Fixture *f = other->GetFixtureList(); f; f = f->GetNext())
        if (b2TestOverlap(platform->GetShape(), 0, f->GetShape(), 0,
                          platform->GetBody()->GetTransform(), other->GetTransform()))
            return true;
    return false;
}

void seedOneWayRefusalNative(b2Fixture *platform, b2Body *other) {
    if (!oneWayFixtureBodyOverlapNative(platform, other))
        return;
    int empty = -1;
    for (int i = 0; i < oneWayRestoredRefusalCountNative; i++) {
        OneWayRestoredRefusalNative &p = oneWayRestoredRefusalsNative[i];
        if (p.platform == platform && p.other == other)
            return;
        if (!p.platform && empty < 0)
            empty = i;
    }
    if (empty < 0 && oneWayRestoredRefusalCountNative < 4096)
        empty = oneWayRestoredRefusalCountNative++;
    if (empty >= 0)
        oneWayRestoredRefusalsNative[empty] =
            OneWayRestoredRefusalNative{platform, other, other->GetWorld()};
}

void pruneOneWayRefusalsNative(b2World *world) {
    for (int i = 0; i < oneWayRestoredRefusalCountNative; i++) {
        OneWayRestoredRefusalNative &p = oneWayRestoredRefusalsNative[i];
        if (p.platform && p.world == world && !oneWayFixtureBodyOverlapNative(p.platform, p.other))
            p.platform = 0;
    }
    while (oneWayRestoredRefusalCountNative &&
           !oneWayRestoredRefusalsNative[oneWayRestoredRefusalCountNative - 1].platform)
        oneWayRestoredRefusalCountNative--;
}

void trimOneWayEntries() {
    while (oneWayEntryLimit > 0 && !oneWayEntries[oneWayEntryLimit - 1].contact)
        oneWayEntryLimit--;
}

void forgetOneWayContact(b2Contact *contact) {
    for (int i = 0; i < oneWayEntryLimit; i++)
        if (oneWayEntries[i].contact == contact)
            oneWayEntries[i].contact = 0;
    trimOneWayEntries();
}

void clearOneWayWorld(b2World *world) {
    for (int i = 0; i < oneWayEntryLimit; i++)
        if (oneWayEntries[i].world == world)
            oneWayEntries[i].contact = 0;
    trimOneWayEntries();
    for (int i = 0; i < oneWayRestoredRefusalCountNative; i++)
        if (oneWayRestoredRefusalsNative[i].world == world)
            oneWayRestoredRefusalsNative[i].platform = 0;
    while (oneWayRestoredRefusalCountNative &&
           !oneWayRestoredRefusalsNative[oneWayRestoredRefusalCountNative - 1].platform)
        oneWayRestoredRefusalCountNative--;
}

void beginOneWayContact(b2Contact *contact) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        void *tag = fixtures[side]->GetUserData();
        if (tag != (void *)1 && tag != (void *)2)
            continue;
        b2Fixture *platform = fixtures[side];
        b2Body *other = fixtures[1 - side]->GetBody();
        int empty = -1;
        bool found = false, above = true, supported = false;
        for (int i = 0; i < oneWayRestoredRefusalCountNative; i++) {
            OneWayRestoredRefusalNative &p = oneWayRestoredRefusalsNative[i];
            if (p.platform == platform && p.other == other) {
                above = false;
                p.platform = 0;
            }
        }
        for (int i = 0; i < oneWayEntryLimit; i++) {
            OneWayEntry &entry = oneWayEntries[i];
            if (!entry.contact) {
                if (empty < 0)
                    empty = i;
                continue;
            }
            if (entry.contact == contact && entry.platform == platform)
                found = true;
            if (entry.platform == platform && entry.other == other) {
                above &= entry.above;
                supported |= entry.supported;
            }
        }
        if (!above)
            for (int i = 0; i < oneWayEntryLimit; i++)
                if (oneWayEntries[i].contact && oneWayEntries[i].platform == platform &&
                    oneWayEntries[i].other == other)
                    oneWayEntries[i].above = false;
        if (!found && empty < 0 && oneWayEntryLimit < 4096)
            empty = oneWayEntryLimit++;
        if (!found && empty >= 0)
            oneWayEntries[empty] = OneWayEntry{
                contact, platform->GetBody()->GetWorld(), platform, other, above, supported};
    }
}

bool nativeOneWayAbove(b2Vec2 a, b2Vec2 b, b2Vec2 point) {
    return (a.x - point.x) * (b.y - point.y) - (a.y - point.y) * (b.x - point.x) >= 0;
}

bool nativeOneWayFace(float normalY, float verticalVelocity) {
    return absf(normalY) >= 0.01f && verticalVelocity <= 0;
}

// Windows numerical compatibility, not an extra native gameplay branch.
// IA32's solved resting velocity can retain a positive8.54e-18m/s residue.
// Treat values within Box2D's float epsilon as zero for this gate only;
// otherwise a resting actor loses every support before another impact.
// Do not alter body velocity, manifold geometry or the per-body below latch.
float oneWaySolverVelocity(float velocity) {
    return absf(velocity) <= b2_epsilon ? 0 : velocity;
}

// Resolve the same authored top segment for normal entry and a neighboring
// support handoff. The animation never moves a crumbling cloud's fixture.
bool oneWayTopSegment(b2Fixture *fixture, Entity *things, int count, b2Vec2 &a, b2Vec2 &b) {
    if (fixture->GetUserData() == (void *)1)
        return terrainOneWaySegment(fixture, &a, &b);
    if (fixture->GetUserData() != (void *)2)
        return false;
    b2Body *body = fixture->GetBody();
    int own = (int)body->GetUserData() - 1;
    if (own < 0 || own >= count)
        return false;
    Entity &entity = things[own];
    Rect r = boxGroupMatrix(entity) ? boxGroupMatrixFixture(entity) : entity.hit;
    b2Vec2 p = body->GetPosition();
    a = p + b2Vec2(r.l * metres, r.t * metres);
    b = p + b2Vec2(r.r * metres, r.t * metres);
    return true;
}

// A top contact solved on one stationary tile remains valid as the actor
// reaches a touching, level tile. A subpixel penetration at the new manifold
// must not latch the next tile as an underside entry for the entire traversal.
// Require actual existing support, connected surfaces and feet within solver
// slop. This cannot bridge a gap, raise an actor from below, or catch a flight
// against a moving platform. No actor class or level coordinates are involved.
bool oneWayAdjacentSupport(b2Fixture *platform, b2Body *other, b2Vec2 a, b2Vec2 b, Entity *things,
                           int count) {
    if (absf(a.y - b.y) > b2_epsilon ||
        platform->GetBody()->GetLinearVelocity().LengthSquared() > b2_epsilon * b2_epsilon ||
        absf(platform->GetBody()->GetAngularVelocity()) > b2_epsilon)
        return false;
    float bottom = 1e30f;
    for (b2Fixture *f = other->GetFixtureList(); f; f = f->GetNext()) {
        if (f->IsSensor())
            continue;
        b2AABB bounds;
        const b2Shape *shape = f->GetShape();
        shape->ComputeAABB(&bounds, other->GetTransform(), 0);
        // Polygon skin is a solver margin; a circle's radius is its geometry.
        float skin = shape->GetType() == b2Shape::e_polygon ? shape->m_radius : 0;
        bottom = minf(bottom, bounds.lowerBound.y + skin);
    }
    if (bottom < a.y - 2 * b2_linearSlop || bottom == 1e30f)
        return false;
    float left = minf(a.x, b.x), right = maxf(a.x, b.x);
    for (int i = 0; i < oneWayEntryLimit; i++) {
        OneWayEntry &entry = oneWayEntries[i];
        if (!entry.contact || entry.platform == platform || entry.other != other ||
            entry.world != other->GetWorld() || !entry.above || !entry.supported ||
            !entry.contact->IsTouching() || !entry.platform->GetBody()->IsActive())
            continue;
        b2Body *support = entry.platform->GetBody();
        if (support->GetLinearVelocity().LengthSquared() > b2_epsilon * b2_epsilon ||
            absf(support->GetAngularVelocity()) > b2_epsilon)
            continue;
        b2Vec2 sa, sb;
        if (oneWayTopSegment(entry.platform, things, count, sa, sb) &&
            absf(sa.y - sb.y) <= b2_epsilon && absf(a.y - sa.y) <= b2_linearSlop &&
            minf(sa.x, sb.x) <= right + b2_linearSlop && maxf(sa.x, sb.x) >= left - b2_linearSlop)
            return true;
    }
    return false;
}

bool oneWayAllowed(b2Contact *contact, b2Fixture *platform, b2Body *other,
                   const b2WorldManifold &manifold, b2Vec2 a, b2Vec2 b, Entity *things, int count) {
    beginOneWayContact(contact);
    bool above = true, supported = false;
    for (int i = 0; i < oneWayEntryLimit; i++)
        if (oneWayEntries[i].contact && oneWayEntries[i].platform == platform &&
            oneWayEntries[i].other == other) {
            above &= oneWayEntries[i].above;
            supported |= oneWayEntries[i].supported;
        }
    float topNormal = contact->GetFixtureA() == platform ? manifold.normal.y : -manifold.normal.y;
    if (!supported && topNormal >= 0.01f &&
        oneWayAdjacentSupport(platform, other, a, b, things, count)) {
        // Replace only the new tile's refusal. Solved continuity supplies the
        // evidence that the body reached this face from the connected top.
        above = true;
        supported = true;
        for (int i = 0; i < oneWayEntryLimit; i++)
            if (oneWayEntries[i].contact && oneWayEntries[i].platform == platform &&
                oneWayEntries[i].other == other)
                oneWayEntries[i].above = true;
    }
    // Retained top support never grants an exemption on the bottom face.
    // An actor rounding the lip must still rise through the cloud from below.
    if (supported)
        return topNormal >= 0.01f;
    for (int i = 0; i < contact->GetManifold()->pointCount; i++)
        above &= nativeOneWayAbove(a, b, manifold.points[i]);
    if (!above)
        for (int i = 0; i < oneWayEntryLimit; i++)
            if (oneWayEntries[i].contact && oneWayEntries[i].platform == platform &&
                oneWayEntries[i].other == other)
                oneWayEntries[i].above = false;
    return above &&
           nativeOneWayFace(manifold.normal.y, oneWaySolverVelocity(other->GetLinearVelocity().y));
}

void oneWayContacts(b2Contact *contact, Entity *things, int count) {
    if (!contact->GetManifold()->pointCount)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int i = 0; i < 2; i++) {
        void *tag = fixtures[i]->GetUserData();
        if (tag != (void *)1 && tag != (void *)2)
            continue;
        b2Body *other = fixtures[1 - i]->GetBody();
        int index = (int)other->GetUserData() - 1;
        if (index < 0 || index >= count)
            continue;
        b2Vec2 a, b;
        if (!oneWayTopSegment(fixtures[i], things, count, a, b))
            continue;
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        contact->SetEnabled(
            oneWayAllowed(contact, fixtures[i], other, manifold, a, b, things, count));
    }
}

// Windows continuity for an already solved one-way support. An upward
// collision impulse must not disable that support before a later downward
// impulse in the same Step; tiny solver penetration must not relatch it below.
// New underside contacts still use the complete native point/velocity gate.
void oneWaySupportPostSolveNative(b2Contact *contact) {
    b2Fixture *f[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (f[0]->IsSensor() || f[1]->IsSensor() || !contact->IsEnabled() ||
        !contact->GetManifold()->pointCount)
        return;
    b2WorldManifold m;
    contact->GetWorldManifold(&m);
    for (int side = 0; side < 2; side++) {
        if (f[side]->GetUserData() != (void *)1 && f[side]->GetUserData() != (void *)2)
            continue;
        if ((side ? -m.normal.y : m.normal.y) <= .95f)
            continue;
        for (int i = 0; i < oneWayEntryLimit; i++) {
            OneWayEntry &entry = oneWayEntries[i];
            if (entry.contact && entry.platform == f[side] &&
                entry.other == f[1 - side]->GetBody() && entry.above)
                entry.supported = true;
        }
    }
}
