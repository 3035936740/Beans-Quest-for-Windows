#pragma once

// Projectile emitter activation, firing cadence and source-owned shot behavior.

// BulletSource::OnActivate 0x147114 starts at timeOffset. OnUpdate
// 0x146188 advances its own clock, fires only after interval, preserves the
// fmod remainder, then samples the next timeMin..timeMax interval.
void updateBulletSource(Entity &e, float dt) {
    const Props *p = e.source->behavior("BulletSource");
    if (!p || e.state.gone)
        return;
    int index = (int)(&e - entities);
    BulletSourceState &s = bulletSourceStates[index];
    if (!s.initialized)
        resetBulletSource(index, *p);
    bulletSourceAnimationBeforeUpdate(e, dt);
    float desired = trackBulletSourceAimNative(e, *p, s, sourceTargetNative(*p), dt);
    advanceBulletSourceClockNative(s, dt);
    if (s.clock > s.interval &&
        (!*p->get("targetValidAngles") || sourceAimCloseNative(s.angle, desired))) {
        shoot(e, *p);
        bulletSourceAnimationAfterFire(e);
        s.clock -= (int)(s.clock / s.interval) * s.interval;
        s.interval = sourceInterval(*p);
    }
}

// OrbitingObject::PrePhysicsStep 0x28939c copies paused, visible and physics
// activation from its parent. An orbiting hazard is not an independent shot.
void syncAttachedActors() {
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (!e.motion || e.parent < 0 || e.parent >= entityCount)
            continue;
        Entity &parent = entities[e.parent];
        e.state.gone = parent.state.gone;
        e.state.active = parent.state.active;
        if (bodies[i])
            bodies[i]->SetActive(!e.state.gone && e.state.active != 0);
    }
}
