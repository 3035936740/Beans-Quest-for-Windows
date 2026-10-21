#pragma once

// Builds original circular fixture geometry and its collision filters.

// CirclePhysics::CreateMainFixture0x1608e8..998 uses signed adjusted X
// extent, not the smaller dimension, abs() or a synthetic minimum radius.
// Actor::Initialize centres its local half-size before physics adjustments.
Rect nativeCircleAdjustedLocal(Rect local, const Props *physics) {
    if (physics) {
        local.l += physics->num("adjust-left");
        local.r += physics->num("adjust-right");
        local.b += physics->num("adjust-bottom");
        local.t += physics->num("adjust-top");
    }
    return local;
}

Rect nativeCirclePrototypeLocal(const Object &source, const Props *physics) {
    Vec c = center(source.rect);
    return nativeCircleAdjustedLocal(shifted(source.rect, -c.x, -c.y), physics);
}

void nativeCircleFixtureGeometry(b2CircleShape &shape, Rect adjusted) {
    shape.m_radius = (adjusted.r - adjusted.l) * .5f * metres;
    shape.m_p.Set((adjusted.l + adjusted.r) * .5f * metres,
                  (adjusted.b + adjusted.t) * .5f * metres);
}

// Both explicit Box/Circle components query2.5/.5/.2 when absent. Actor
// behaviours with their own material overrides retain those later overrides.
void nativeBoxCircleFixtureMaterial(b2FixtureDef &fixture, const Props &physics) {
    fixture.density = physics.num("density", 2.5f);
    fixture.restitution = physics.num("restitution", .5f);
    fixture.friction = physics.num("friction", .2f);
}
