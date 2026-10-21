#pragma once

// Helper perch, carrying, release and collision behavior, shared with the original authored bird settings.

// Native states: 0 roosting, 1 following/flying, 2 returning. Roost position
// lives in MotionState's checkpointed origin; no transient pointers are saved.
int flyingHelper() {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER && !entities[i].state.gone && entities[i].state.active == 1)
            return i;
    return -1;
}

// Native StopFollowing 0x21b7e8 changes to returning/manic without audio.
// OnCollected and solid-contact PostSolve own their separate bird sounds.
void stopHelper(Entity &bird) {
    if (bird.state.active != 1)
        return;
    bird.state.active = 2;
    bird.state.timer = 0;
}

// OnCollected0x2159a0..215bd0 selects flight and emits the pickup burst.
// Keep Bean-facing initialization and effects in the owning world.
void collectHelperNative(Entity &bird, const State &bean, int domain) {
    if (bird.state.active != 0)
        return;
    bird.state.active = 1;
    bird.state.timer = 0;
    bird.state.aux = bean.aux;
    sound("bird");
    entityEffect(bird, "HelperBirdBehaviour", "effect", domain);
}

void collectHelper(Entity &bird) {
    if (flyingHelper() < 0)
        collectHelperNative(bird, entities[playerIndex].state, 0);
}

void helperPreSolve(b2Contact *contact) {
    b2Fixture *a = contact->GetFixtureA();
    b2Fixture *b = contact->GetFixtureB();
    b2Fixture *fs[] = {a, b};
    for (int side = 0; side < 2; side++) {
        int index = (int)fs[side]->GetBody()->GetUserData() - 1,
            other = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
        if (index < 0 || index >= entityCount || entities[index].kind != HELPER)
            continue;
        // PreSolve 0x21cc88 collects only the tracked Bean and suppresses
        // that contact; off/return states pass through the environment too.
        Entity &bird = entities[index];
        if (other == playerIndex) {
            collectHelper(bird);
            contact->SetEnabled(false);
        } else if (bird.state.active != 1)
            contact->SetEnabled(false);
    }
}

bool helperArrivalComponentNative(float a, float b) {
    u32 x, y;
    memcpy(&x, &a, 4);
    memcpy(&y, &b, 4);
    int orderedX = (int)((x & 0x80000000u) ? 0x80000000u - x : x),
        orderedY = (int)((y & 0x80000000u) ? 0x80000000u - y : y);
    __int64 distance = (__int64)orderedX - orderedY;
    return distance >= -100 && distance <= 100;
}

#pragma float_control(precise, on, push)

// Preserve the native float steps before the ULP-sensitive arrival decision.
__declspec(noinline) bool advanceHelperReturnNative(float &x, float &y, float targetX,
                                                    float targetY, float dt, float &facing) {
    float dx = targetX - x, dy = targetY - y, length = b2Sqrt(dx * dx + dy * dy);
    if (length >= 1.1920928955078125e-7f) {
        float inv = 1.0f / length;
        dx *= inv;
        dy *= inv;
    }
    float step = 150.0f * dt;
    dx *= step;
    dy *= step;
    if (x < targetX)
        x = minf(x + absf(dx), targetX);
    else if (x > targetX)
        x = maxf(x - absf(dx), targetX);
    if (y < targetY)
        y = minf(y + absf(dy), targetY);
    else if (y > targetY)
        y = maxf(y - absf(dy), targetY);
    // HelperBird::OnUpdate 0x2170d0..0x217130 compares each IEEE float
    // coordinate within 100 representations before snapping to the roost.
    if (helperArrivalComponentNative(x, targetX) && helperArrivalComponentNative(y, targetY)) {
        x = targetX;
        y = targetY;
        return true;
    }
    facing = dx < 0 ? -1.0f : 1.0f;
    return false;
}

#pragma float_control(pop)

void updateHelper(Entity &bird, float dt) {
    State &s = bird.state;
    s.vx = s.vy = 0;
    if (s.active == 2) {
        if (advanceHelperReturnNative(s.x, s.y, bird.originX, bird.originY, dt, s.aux)) {
            s.active = 0;
            s.timer = 0;
        }
    } else if (s.active == 0) {
        s.x = bird.originX;
        s.y = bird.originY;
    }
    // Actor::PreUpdate advances the selected instance after behaviour OnUpdate.
    advanceHelperAnimationNative(bird, dt);
}

// HelperBirdPerch::BeginContact0x22170c..0x2217a0 compares the cached
// roost within100 float representations, then restarts ringing immediately.
void helperPerchBeginContactNative(b2Contact *contact, Entity *things, int count, int player) {
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    for (int side = 0; side < 2; side++) {
        int source = (int)fs[side]->GetBody()->GetUserData() - 1,
            other = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= count || other != player || things[source].kind != PERCH ||
            things[source].state.gone)
            continue;
        Entity &perch = things[source];
        const Props &p = *perch.source->behavior("HelperBirdPerchBehaviour");
        float x = perch.state.x + p.num("offset-x"), y = perch.state.y + p.num("offset-y", 23);
        for (int j = 0; j < count; j++) {
            Entity &bird = things[j];
            if (bird.kind != HELPER || bird.state.gone)
                continue;
            if (helperArrivalComponentNative(bird.originX, x) &&
                helperArrivalComponentNative(bird.originY, y))
                continue;
            bird.originX = x;
            bird.originY = y;
            if (bird.state.active == 0) {
                bird.state.active = 2;
                bird.state.timer = 0;
            }
            perch.state.active = 2;
            perch.state.timer = 0;
            sound("ringing");
        }
    }
}

void updateHelperPerchNative(Entity &perch) {
    // Native OnUpdate0x21e0c4 checks IsAtEnd before Actor advances the
    // selected clip. Keep its final STOP frame until the following update.
    if (perch.state.active == 2 &&
        perch.state.timer >= animationLength(animation(perch.art, "ringing"))) {
        perch.state.active = 0;
        perch.state.timer = 0;
    }
}

// HelperBird::PostSolve0x2174e8 stops Tom flying and plays the impact
// voice from the solved event, before later actor removal can erase it.
void recordHelperSolvedContactNative(b2Contact *contact, Entity *things, int count, bool *solved) {
    if (!contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fs[0]->IsSensor() || fs[1]->IsSensor())
        return;
    for (int side = 0; side < 2; side++) {
        int index = (int)fs[side]->GetBody()->GetUserData() - 1;
        if (index >= 0 && index < count && things[index].kind == HELPER &&
            !things[index].state.gone && things[index].state.active == 1)
            solved[index] = true;
    }
}

void helperPostStep(float dt) {
    State &bean = entities[playerIndex].state;
    for (int i = 0; i < entityCount; i++) {
        Entity &bird = entities[i];
        if (bird.kind != HELPER || bird.state.gone)
            continue;
        State &s = bird.state;
        if (s.active == 1) {
            bool blocked = helperSolvedContactsNative[i];
            helperSolvedContactsNative[i] = false;
            // Native PostSolve 0x2176b8..d4 plays bird on a solved collision.
            // Coalesce the solved contacts before leaving following state.
            if (blocked) {
                sound("bird");
                stopHelper(bird);
            } else if (killableHelperStopPendingNative[i]) {
                sound("bird");
                stopHelper(bird);
            } else if (deathTimer > 0 || complete)
                stopHelper(bird);
            else {
                const Props &p = *bird.source->behavior("HelperBirdBehaviour");
                float blend = clamp(s.timer / 0.1f, 0, 1);
                s.x = bird.originX * (1 - blend) + (bean.x + p.num("follow-offset-x")) * blend;
                s.y = bird.originY * (1 - blend) + (bean.y + p.num("follow-offset-y", 34)) * blend;
                s.aux = bean.aux;
            }
        }
        killableHelperStopPendingNative[i] = false;
        if (bodies[i]) {
            bodies[i]->SetTransform(b2Vec2(s.x * metres, s.y * metres), 0);
            bodies[i]->SetLinearVelocity(b2Vec2(0, 0));
            if (s.active == 1)
                bodies[i]->SetAwake(true);
        }
        s.vx = s.vy = 0;
    }
    (void)dt;
}
