#pragma once

// Handles strong-bounce contacts and authored launch power.

// Pinball PreSolve 0x002cbe68 requests Tom::MegaJump 0x003129e0 with
// the manifold normal, never with the line between the sprite centres.
// Per-contact effects belong to the current solver step. The final player
// request survives to the following PrePhysics and is saved separately.
struct PinballPlayerStepNative {
    bool pending;
    int source;
    Vec normal;
    float speed;
    int hits[MAX_ENTITIES];
};

PinballPlayerStepNative pinballPlayerStepNative;

void nativePinballReset(PinballPlayerStepNative &step) {
    memset(&step, 0, sizeof(step));
    step.source = -1;
}

float nativePinballJumpSpeed(float gravity, float defaultHeight, float bonusHeight,
                             float authoredPower) {
    return root(maxf(0, -2 * gravity * maxf(defaultHeight + bonusHeight, authoredPower)));
}

Vec nativePinballNormal(Vec normal) {
    float length = root(normal.x * normal.x + normal.y * normal.y);
    if (length >= 1.1920928955078125e-7f) {
        normal.x /= length;
        normal.y /= length;
    }
    return normal;
}

void nativeMegaJumpRequest(int source, Vec normal, float gravity, float height, float bonus,
                           float power) {
    pinballPendingNative =
        PinballPendingNative{1, source, nativePinballNormal(normal),
                             nativePinballJumpSpeed(gravity, height, bonus, power)};
}

void nativePinballPreSolve(b2Contact *contact, Entity *things, int count, int player,
                           PinballPlayerStepNative &step, float bonusHeight = 0) {
    if (!contact->IsEnabled() || !contact->GetManifold()->pointCount)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    if (fixtures[0]->IsSensor() || fixtures[1]->IsSensor())
        return;
    for (int side = 0; side < 2; side++) {
        int source = (int)fixtures[side]->GetBody()->GetUserData() - 1,
            target = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
        if (source < 0 || source >= count || source >= MAX_ENTITIES || target != player ||
            player < 0 || player >= count || things[source].kind != BOUNCER ||
            things[source].state.gone || things[player].state.gone)
            continue;
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        Vec direction = {side ? -manifold.normal.x : manifold.normal.x,
                         side ? -manifold.normal.y : manifold.normal.y};
        const Props &props = *things[player].physics;
        const Props &pinball = *things[source].source->behavior("PinballBouncerBehaviour");
        // OnActivate0x2cb020..30 supplies200 when jump-power is absent.
        step.pending = true;
        step.source = source;
        step.normal = nativePinballNormal(direction);
        step.speed =
            nativePinballJumpSpeed(props.num("gravity", -400), props.num("jump-height", 74),
                                   bonusHeight, pinball.num("jump-power", 200));
        step.hits[source]++;
        nativeMegaJumpRequest(source, direction, props.num("gravity", -400),
                              props.num("jump-height", 74), bonusHeight,
                              pinball.num("jump-power", 200));
    }
}

// Pinball effects happen at contact, while +c9 remains queued for the next
// PrePhysics. This separation avoids applying horizontal movement too early.
void nativePinballFinishContacts(Entity *things, int count, PinballPlayerStepNative &step) {
    for (int i = 0; i < count && i < MAX_ENTITIES; i++)
        if (step.hits[i]) {
            if (!things[i].state.gone) {
                things[i].state.active = 2;
                things[i].state.timer = 0;
                for (int hit = 0; hit < step.hits[i]; hit++)
                    sound("pinball");
            }
            step.hits[i] = 0;
        }
    // Requests were written in callback order. Never overwrite a subsequent
    // Spring PostSolve request while dispatching the Pinball's contact sound.
}

// PrePhysics0x311468 checks the OLD age>=0.05 before consuming +c9;
// all request flags are cleared at0x311a4c..58 even when rejected.
bool nativePinballConsumePending(Entity *things, int count, int player, bool canLaunch,
                                 bool countEnabled, int &jumpCount, float jumpAge) {
    PinballPendingNative request = pinballPendingNative;
    pinballPendingNative = PinballPendingNative{};
    pinballPendingNative.source = -1;
    pinballConsumedNative = request;
    pinballLaunchedThisStepNative = false;
    if (!request.pending || !canLaunch || !validPinballPendingNative(request, count) ||
        player < 0 || player >= count || things[player].state.gone || jumpAge < .05f)
        return false;
    Entity &bean = things[player];
    bean.state.vx += request.speed * request.normal.x;
    bean.state.vy = request.speed * request.normal.y;
    bean.lastLaunchSpeed = request.speed;
    bean.state.timer = 0;
    bean.state.active = 1;
    if (countEnabled && jumpCount < 999)
        jumpCount++;
    return pinballLaunchedThisStepNative = true;
}
