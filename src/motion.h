#pragma once

// Advances moving, attached and oscillating actors while preserving the native update order.

bool nativeMotionHasAnimation(const Entity &e, const char *name) {
    if (!e.art)
        return false;
    for (int i = 0; i < e.art->count; i++)
        if (eq(e.art->animations[i].name, name))
            return true;
    return false;
}

// WalkLeft0x326490 / WalkRight0x3266b4 restart the selected instance on a
// changed direction, or when explicitly forced by the original edge probe.
void nativeWalkDirection(Entity &e, bool left, bool force = false) {
    if (!force && (e.walkRight == !left))
        return;
    e.walkRight = !left;
    e.state.aux = left ? -e.speed : e.speed;
    if (nativeMotionHasAnimation(e, left ? "walk-left" : "walk-right"))
        e.state.timer = 0;
}

bool nativeOscillatorDirectionAnimation(const Entity &e, float direction) {
    float x = e.moveX * direction, y = e.moveY * direction;
    const char *name = x > .9f    ? "right"
                       : x < -.9f ? "left"
                       : y > .9f  ? "up"
                       : y < -.9f ? "down"
                                  : 0;
    return name && nativeMotionHasAnimation(e, name);
}

// ProximityActivateBehaviour::OnPostUpdate 0x2db088 compares actor centres
// against the two authored distances. Activation sticks unless configured
// otherwise. This also applies to moving blocks, not only killable enemies.
bool proximityControlled(const Entity &e) {
    return e.source->has("ProximityActivateBehaviour");
}

bool updateProximity(Entity &e, const State &bean) {
    const Props *p = e.source->behavior("ProximityActivateBehaviour");
    if (!p)
        return true;
    if (e.state.active && !p->flag("auto-deactivate"))
        return true;
    bool active = absf(e.state.x - bean.x) <= p->num("distance-x", 100) &&
                  absf(e.state.y - bean.y) <= p->num("distance-y", 100);
    e.state.active = active ? 1 : 0;
    return active;
}

bool updateProximity(Entity &e) {
    return updateProximity(e, entities[playerIndex].state);
}

// LevelTask::OnTick0x270778 finishes CollisionWorld::Update before its
// EnsurePostUpdate calls0x270644. Proximity therefore observes solved actor
// positions and changes the pause state for the NEXT physics pass.
void advanceProximityAfterPhysicsNative(Entity *things, int count, const State &bean,
                                        b2Body **primary, b2Body **proxies) {
    for (int i = 0; i < count; i++) {
        Entity &e = things[i];
        if (e.state.gone || !proximityControlled(e))
            continue;
        updateProximity(e, bean);
        if (!primary[i])
            continue;
        bool active = e.state.active && !(e.crumble && e.state.active == 2) &&
                      (e.kind != BOSS || e.state.active <= 3) &&
                      (e.kind != ALTERNATOR || activeBlock(e));
        primary[i]->SetActive(active);
        if (proxies && proxies[i])
            proxies[i]->SetActive(active);
    }
}

bool motionActive(const Entity &e) {
    return !e.state.gone && (!proximityControlled(e) || e.state.active != 0);
}

// These helpers mirror the controller's saved fields instead of inferring
// direction from solver velocity, which can be changed by contact impulses.
void resetNativeMotionState(Entity &e, DiagonalRuntimeState &d, OscillatorRuntimeState &o,
                            bool legacy = false) {
    memset(&d, 0, sizeof(d));
    memset(&o, 0, sizeof(o));
    if (e.source->has("DiagonalBehaviour")) {
        d.directionX = e.moveX;
        d.directionY = e.moveY;
        d.effectiveSpeed = e.speed;
        if (legacy) {
            int bits = (int)e.state.aux;
            d.directionX = absf(d.directionX) * (bits & 1 ? -1 : 1);
            d.directionY = absf(d.directionY) * (bits & 2 ? -1 : 1);
        }
    }
    if (e.move) {
        o.effectiveSpeed = e.speed;
        o.initialized = 1;
        if (e.hi > e.lo && e.speed > 0) {
            // SetConfig 0x28f980..9a8,0x2912a4..c4,0x2913c4..cc.
            o.clock = e.hi < 0 ? (e.hi - e.lo) / e.speed : e.lo <= 0 ? -e.lo / e.speed : 0;
            if (legacy) {
                float distance =
                    (e.state.x - e.originX) * e.moveX + (e.state.y - e.originY) * e.moveY;
                float phase = distance - e.lo;
                if (e.state.aux < 0)
                    phase = 2 * (e.hi - e.lo) - phase;
                o.clock = phase / e.speed;
            }
        }
    }
}

void resetNativeMotion(int index, bool legacy) {
    resetNativeMotionState(entities[index], diagonalRuntime[index], oscillatorRuntime[index],
                           legacy);
}

// Controllers receive one CollisionWorld physics pass: native
// GetInvDtPerFrame is this pass's inverse dt, not the renderer's FPS.
float nativeMotionPhysicsFPS(float dt) {
    return dt > 0 ? 1.0f / dt : 0;
}

void selectNativeMotionSpeed(const Props &config, float fps, float &speed, int &selected) {
    if (selected)
        return;
    // Diagonal 0x19ee80/0x19f514, Oscillating 0x292f18/0x29369c.
    if (fps <= 45.0f)
        speed = config.num("3g-speed", speed);
    selected = 1;
}

void nativeDiagonalOnHit(DiagonalRuntimeState &state, float normalX, float normalY) {
    // Native 0x19d840: independent, strictly opposing component signs.
    if ((state.directionX < 0 && normalX > 0) || (state.directionX > 0 && normalX < 0))
        state.directionX = -state.directionX;
    if ((state.directionY < 0 && normalY > 0) || (state.directionY > 0 && normalY < 0))
        state.directionY = -state.directionY;
}

void advanceNativeDiagonalState(Entity &e, DiagonalRuntimeState &d, float fps) {
    if (!e.source->has("DiagonalBehaviour"))
        return;
    selectNativeMotionSpeed(*e.source->behavior("DiagonalBehaviour"), fps, d.effectiveSpeed,
                            d.selected);
    if (!motionActive(e))
        return;
    e.state.vx = d.directionX * d.effectiveSpeed;
    e.state.vy = d.directionY * d.effectiveSpeed;
    selectFishAnimationNative(e, d.directionX);
}

void advanceNativeDiagonal(int index, float dt) {
    advanceNativeDiagonalState(entities[index], diagonalRuntime[index], nativeMotionPhysicsFPS(dt));
}

float nativeMotionRemainder(float value, float divisor) {
    float result;
    __asm {
        fld divisor
        fld value
nativeMotionRemainderLoop:
        fprem
        fnstsw ax
        test ah,4
        jnz nativeMotionRemainderLoop
        fstp result
        fstp st(0)
    }
    return result;
}

float nativeOscillatorPhase(const Entity &e, const OscillatorRuntimeState &state) {
    float cycle = 2 * (e.hi - e.lo);
    if (cycle <= 0)
        return 0;
    float distance = state.clock * state.effectiveSpeed;
    // GetNextPosition/GetCurrentDirection explicitly handle negative fmod.
    float phase = nativeMotionRemainder(distance, cycle);
    if (distance < 0)
        phase += cycle;
    return phase;
}

float nativeOscillatorDirection(const Entity &e, const OscillatorRuntimeState &state) {
    return nativeOscillatorPhase(e, state) <= e.hi - e.lo ? 1.0f : -1.0f;
}

void nativeOscillatorOnHit(Entity &e, OscillatorRuntimeState &state, float normalX, float normalY) {
    float direction = nativeOscillatorDirection(e, state);
    if ((normalX * e.moveX + normalY * e.moveY) * direction < 0 && state.effectiveSpeed > 0) {
        // ReverseDirection 0x28c3c8 mirrors TIME, not the solved position.
        state.clock = 2 * (e.hi - e.lo) / state.effectiveSpeed - state.clock;
    }
}

void advanceNativeOscillatorState(Entity &e, OscillatorRuntimeState &o, float dt, float fps) {
    if (!e.move || dt <= 0)
        return;
    if (!o.initialized) {
        DiagonalRuntimeState unused;
        resetNativeMotionState(e, unused, o);
    }
    selectNativeMotionSpeed(*e.move, fps, o.effectiveSpeed, o.selected);
    if (!motionActive(e))
        return;
    if (e.hi <= e.lo || o.effectiveSpeed <= 0) {
        e.state.vx = e.state.vy = 0;
        return;
    }
    float phase = nativeOscillatorPhase(e, o), span = e.hi - e.lo;
    float offset = e.lo + (phase > span ? 2 * span - phase : phase),
          direction = phase <= span ? 1.0f : -1.0f;
    e.state.vx = (e.originX + e.moveX * offset - e.state.x) / dt;
    e.state.vy = (e.originY + e.moveY * offset - e.state.y) / dt;
    // 0x293458..293490 only restarts an existing directional instance.
    // Actors retaining their authored idle (spider/eye/block) keep its clock.
    if ((e.state.aux < 0) != (direction < 0) && nativeOscillatorDirectionAnimation(e, direction))
        e.state.timer = 0;
    e.state.aux = direction;
    // PrePhysics 0x293194 gets the target BEFORE 0x293218 increments time.
    o.clock += dt;
}

void advanceNativeOscillator(int index, float dt) {
    advanceNativeOscillatorState(entities[index], oscillatorRuntime[index], dt,
                                 nativeMotionPhysicsFPS(dt));
}

// Both native detector PreSolve implementations disable their own contact
// before sending the inward normal: diagonal0x19d994, oscillator0x28cfdc.
// This prevents solver impulses between the dummy body and map geometry.
void nativeMotionProxyPreSolve(b2Contact *contact, Entity *things, int count,
                               DiagonalRuntimeState *diagonals = 0,
                               OscillatorRuntimeState *oscillators = 0) {
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (!contact->GetManifold()->pointCount)
        return;
    b2WorldManifold world;
    contact->GetWorldManifold(&world);
    for (int side = 0; side < 2; side++) {
        if (fixtures[side]->GetUserData() != (void *)15)
            continue;
        int index = (int)fixtures[side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= count)
            continue;
        Entity &e = things[index];
        b2Vec2 normal = side ? world.normal : -world.normal;
        contact->SetEnabled(false);
        // Original category4/mask6 excludes loose dynamic category1. The
        // port's enemy category16 also contains kinematic carriers, so its
        // broad filter needs this explicit equivalent category distinction.
        if (fixtures[1 - side]->GetBody()->GetType() == b2_dynamicBody &&
            fixtures[1 - side]->GetUserData() != (void *)15)
            continue;
        if (e.source->has("DiagonalBehaviour") && diagonals)
            nativeDiagonalOnHit(diagonals[index], normal.x, normal.y);
        else if (e.move) {
            if (oscillators)
                nativeOscillatorOnHit(e, oscillators[index], normal.x, normal.y);
            else if ((normal.x * e.moveX + normal.y * e.moveY) * (e.state.aux < 0 ? -1 : 1) < 0)
                e.state.aux = -e.state.aux;
        }
    }
}

// Native SetConfig normalizes the authored direction. GetNextPosition folds
// distance into a triangle wave; mutable direction and position are already
// in State, so checkpoint restoration does not reset its phase.
void advanceOscillator(Entity &e, float dt, bool dynamicBody = false) {
    if (!e.move || dt <= 0)
        return;
    State &s = e.state;
    float span = e.hi - e.lo;
    float norm = e.moveX * e.moveX + e.moveY * e.moveY;
    if (span <= 0 || norm < 0.0000001f || e.speed <= 0) {
        s.vx = s.vy = 0;
        return;
    }
    float offset = ((s.x - e.originX) * e.moveX + (s.y - e.originY) * e.moveY) / norm;
    float cycle = 2 * span, phase = s.aux < 0 ? cycle - (offset - e.lo) : offset - e.lo;
    phase += e.speed * dt;
    phase -= (int)(phase / cycle) * cycle;
    if (phase < 0)
        phase += cycle;
    if (phase > span) {
        offset = e.hi - (phase - span);
        s.aux = -1;
    } else {
        offset = e.lo + phase;
        s.aux = 1;
    }
    float x = e.originX + e.moveX * offset, y = e.originY + e.moveY * offset;
    s.vx = (x - s.x) / dt;
    s.vy = (y - s.y) / dt;
    // Dynamic collision bodies integrate these velocities. Kinematic callers
    // (the independent selector world) still consume the desired position.
    if (!dynamicBody) {
        s.x = x;
        s.y = y;
    }
}

// OscillatingBehaviour::OnHit 0x28cf38 reverses only when its current
// direction points into the contacted surface. Tangent floor contacts do not
// reverse horizontal rails and do not make vertical spiders drift sideways.
void finishOscillatorBody(Entity &e, b2Body *body) {
    if (!e.move || !motionActive(e) || !body)
        return;
    float direction = e.state.aux < 0 ? -1.0f : 1.0f;
    for (b2ContactEdge *edge = body->GetContactList(); edge; edge = edge->next) {
        b2Contact *c = edge->contact;
        if (!c->IsTouching() || !c->IsEnabled() || !c->GetManifold()->pointCount ||
            c->GetFixtureA()->IsSensor() || c->GetFixtureB()->IsSensor())
            continue;
        b2WorldManifold manifold;
        c->GetWorldManifold(&manifold);
        b2Vec2 n = c->GetFixtureA()->GetBody() == body ? manifold.normal : -manifold.normal;
        if ((n.x * e.moveX + n.y * e.moveY) * direction > 0.0001f) {
            e.state.aux = -direction;
            break;
        }
    }
}

void finishOscillatorContacts(int index) {
    finishOscillatorBody(entities[index],
                         oscillatorProxies[index] ? oscillatorProxies[index] : bodies[index]);
}
