#pragma once

// Fluid forces and splash handling, with membership tracked in the owning physics world.
#include "water_contacts_native.h"

// WaterAreaBehaviour::PostPhysicsStep 0x32b700-0x32c0b0. The original clips
// every fixture against a horizontal half-plane, uses geometric area for
// drag, and density-weighted area only for the buoyancy application point.
struct WaterSample {
    float area, massArea;
    b2Vec2 areaCentre, massCentre, buoyancy, drag;
    float angularDrag;
};

float submergedFixture(const b2Fixture &fixture, float surface, b2Vec2 &centre) {
    const b2Body &body = *fixture.GetBody();
    const b2Shape *shape = fixture.GetShape();
    if (shape->GetType() == b2Shape::e_circle) {
        const b2CircleShape &circle = *(const b2CircleShape *)shape;
        b2Vec2 p = body.GetWorldPoint(circle.m_p);
        float r = circle.m_radius, l = surface - p.y;
        if (l <= -r)
            return 0;
        if (l >= r) {
            centre = p;
            return b2_pi * r * r;
        }
        float chord = b2Sqrt(maxf(0, r * r - l * l));
        float area = r * r * (bqAtan2(l, chord) + b2_pi * 0.5f) + l * chord;
        if (area <= b2_epsilon)
            return 0;
        centre = p + b2Vec2(0, -(2.0f / 3.0f) * chord * chord * chord / area);
        return area;
    }
    if (shape->GetType() != b2Shape::e_polygon)
        return 0;
    const b2PolygonShape &polygon = *(const b2PolygonShape *)shape;
    b2Vec2 vertices[b2_maxPolygonVertices + 2];
    int count = 0;
    // Work relative to the body origin to avoid cancelling large world
    // coordinates when computing small submerged areas far along a stage.
    b2Vec2 origin = body.GetPosition();
    float height = surface - origin.y;
    b2Vec2 previous = b2Mul(body.GetTransform().q, polygon.m_vertices[polygon.m_vertexCount - 1]);
    bool wasBelow = previous.y < height;
    for (int i = 0; i < polygon.m_vertexCount; i++) {
        b2Vec2 point = b2Mul(body.GetTransform().q, polygon.m_vertices[i]);
        bool below = point.y < height;
        if (below != wasBelow) {
            float fraction = (height - previous.y) / (point.y - previous.y);
            vertices[count++] = previous + fraction * (point - previous);
        }
        if (below)
            vertices[count++] = point;
        previous = point;
        wasBelow = below;
    }
    if (count < 3)
        return 0;
    float twiceArea = 0;
    b2Vec2 sum(0, 0);
    for (int i = 0; i < count; i++) {
        const b2Vec2 &a = vertices[i];
        const b2Vec2 &b = vertices[(i + 1) % count];
        float cross = b2Cross(a, b);
        twiceArea += cross;
        sum += cross * (a + b);
    }
    if (twiceArea <= 2 * b2_epsilon)
        return 0;
    centre = origin + (1 / (3 * twiceArea)) * sum;
    return twiceArea * 0.5f;
}

WaterSample sampleWater(b2Body &body, bool bean, const Props &props, float surface) {
    WaterSample result = {};
    result.areaCentre.SetZero();
    result.massCentre.SetZero();
    result.buoyancy.SetZero();
    result.drag.SetZero();
    bool overrideDensity =
        props.flag(bean ? "bean-object-density-override" : "object-density-override");
    for (b2Fixture *fixture = body.GetFixtureList(); fixture; fixture = fixture->GetNext()) {
        b2Vec2 centre(0, 0);
        float area = submergedFixture(*fixture, surface, centre),
              density = overrideDensity ? 1 : fixture->GetDensity();
        result.area += area;
        result.areaCentre += area * centre;
        result.massArea += area * density;
        result.massCentre += (area * density) * centre;
    }
    if (result.area < b2_epsilon || result.massArea < b2_epsilon || body.GetMass() <= 0) {
        result.area = 0;
        return result;
    }
    result.areaCentre *= 1 / result.area;
    result.massCentre *= 1 / result.massArea;
    float density = props.num(bean ? "bean-density" : "density", bean ? 15.0f : 1.75f),
          linear = props.num(bean ? "bean-linear-drag" : "linear-drag", bean ? 20.0f : 5.0f),
          angular = props.num(bean ? "bean-angular-drag" : "angular-drag", bean ? 0.0f : 10.0f);
    result.buoyancy.Set(0, density * result.area * (-nativeWorldGravity) * metres);
    result.drag = (-linear * result.area) * body.GetLinearVelocityFromWorldPoint(result.areaCentre);
    result.angularDrag =
        -body.GetInertia() / body.GetMass() * result.area * body.GetAngularVelocity() * angular;
    return result;
}

bool applyNativeWaterMembers(b2Body *water, b2Body *bean, const Props &props, float surface) {
    bool inWater = false;
    for (NativeWaterMember *member = nativeWaterMembers; member; member = member->next)
        if (member->water == water) {
            b2Body &body = *member->other;
            if (body.GetType() != b2_dynamicBody || !body.IsActive())
                continue;
            bool isBean = &body == bean;
            if (isBean)
                inWater = true;
            if (!body.IsAwake())
                continue;
            WaterSample sample = sampleWater(body, isBean, props, surface);
            if (sample.area <= 0)
                continue;
            body.ApplyForce(sample.buoyancy, sample.massCentre);
            body.ApplyForce(sample.drag, sample.areaCentre);
            body.ApplyTorque(sample.angularDrag);
        }
    return inWater;
}

bool suppressWaterSplash = true;

// WaterArea::SaveState0x32afe4 arms suppression until its next PostPhysicsStep.
void saveWaterStateNative() {
    suppressWaterSplash = true;
}

void waterSplash(Entity &water, const b2Body &body, int domain = 0) {
    float speed = absf(body.GetLinearVelocity().y / metres);
    if (speed < 100)
        return;
    // Splash 0x32cfa8..0x32d274 samples the LevelTask MT before Explode.
    // Its unsigned integer distribution rejects the fourth bucket's lone
    // UINT32_MAX word. Sound-off still consumes this draw in the original.
    NativeParticleRng &random = burstRandomForNative(domain).engine;
    u32 choice;
    do {
        choice = nativeParticleBits(random) / 1431655765u;
    } while (choice >= 3);
    const char *sounds[] = {"water1", "water2", "water3"};
    sound(sounds[choice]);
    const Props &props = *water.source->behavior("WaterAreaBehaviour");
    int count = (int)(10 + 30 * clamp((speed - 100) / 500, 0, 1));
    emitEffect(idnum(props.get("splash-class")), body.GetPosition().x / metres, entityRect(water).t,
               water.layer, 1, domain, count);
}

void waterContact(b2Contact *contact) {
    if (suppressWaterSplash)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++)
        if (fixtures[side]->GetUserData() == (void *)7) {
            int index = (int)fixtures[side]->GetBody()->GetUserData() - 1;
            if (index < 0 || index >= entityCount)
                return;
            b2Body &other = *fixtures[1 - side]->GetBody();
            // Splash 0x32cd68: both entry and exit require |vy| >= 100 px/s;
            // particle count grows from 10 at 100 to 40 at 600 px/s.
            waterSplash(entities[index], other);
        }
}

void waterPostStep() {
    playerInWater = false;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == WATER && bodies[i] && bodies[i]->IsActive()) {
            Entity &water = entities[i];
            const Props &props = *water.source->behavior("WaterAreaBehaviour");
            float surface = entityRect(water).t * metres;
            playerInWater |= applyNativeWaterMembers(
                bodies[i], playerIndex >= 0 ? bodies[playerIndex] : 0, props, surface);
        }
    // Forces are deliberately retained for the next Box2D step. The original
    // applies them after Step/ClearForces, rather than editing velocity/caps.
    suppressWaterSplash = false;
}
