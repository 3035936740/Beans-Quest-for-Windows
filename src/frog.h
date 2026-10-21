#pragma once

// Enemy jumper behavior and takeoff timing; ground support is supplied by solved contacts.
#include "frog_spin_native.h"

// FrogSpecialBallBehaviour (0x1f6848): the pair's clocks are half a period
// apart. Actor::GetSize returns half-extents: native1f6f5c/1f6f88 offsets targets
// inward by half the frog width and up by jump height plus half ball height. Mutable protocol state lives
// in the existing checkpointed State; links contain only resource-derived data.
struct FrogLink {
    int ball, left, right;
    float period;
    Vec leftTarget, rightTarget;
};

FrogLink frogLinks[16];
int frogLinkCount;
int frogPending[MAX_ENTITIES];

int frogEntity(u64 id) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->id == id)
            return i;
    return -1;
}

Vec frogTarget(const Entity &frog, const Entity &ball, float side) {
    const Props &jump = *frog.source->behavior("EnemyJumperBehaviour");
    return Vec{frog.originX + side * (frog.local.r - frog.local.l) * 0.5f,
               frog.originY + jump.num("jump-height", 60) + (ball.local.t - ball.local.b) * 0.5f};
}

// Recalculate 0x1f5a84 solves x(t)=x0+vx*t and
// y(t)=y0+vy*t+gravity*t*t/2 for the remaining half-period.
bool frogBallistic(Entity &ball, Vec from, Vec to, float period) {
    float time = period * 0.5f - ball.state.timer;
    if (time <= 0.0001f)
        return false;
    ball.state.x = from.x;
    ball.state.y = from.y;
    ball.state.vx = (to.x - from.x) / time;
    ball.state.vy = (to.y - from.y) / time - 0.5f * nativeWorldGravity * time;
    ball.state.aux = nativeFrogSpinNative(0);
    return true;
}

void configureFrogs(bool reset) {
    frogLinkCount = 0;
    memset(frogPending, 0, sizeof(frogPending));
    for (int i = 0; i < entityCount && frogLinkCount < 16; i++) {
        Entity &ball = entities[i];
        const Props *p = ball.source->behavior("FrogSpecialBallBehaviour");
        if (!p)
            continue;
        int left = frogEntity(idnum(p->get("frog-left"))),
            right = frogEntity(idnum(p->get("frog-right")));
        if (left < 0 || right < 0 || !entities[left].source->has("EnemyJumperBehaviour") ||
            !entities[right].source->has("EnemyJumperBehaviour"))
            continue;
        FrogLink &link = frogLinks[frogLinkCount++];
        link.ball = i;
        link.left = left;
        link.right = right;
        link.period = maxf(0.01f, p->num("period", 2));
        link.leftTarget = frogTarget(entities[left], ball, 1);
        link.rightTarget = frogTarget(entities[right], ball, -1);
        if (reset) {
            entities[left].state.aux = link.period * 0.5f;
            entities[right].state.aux = 0;
            ball.state.hp = 1;
            ball.state.active = 0;
            ball.state.timer = -0.5f *
                               root(-2 * nativeWorldGravity *
                                    entities[left]
                                        .source->behavior("EnemyJumperBehaviour")
                                        ->num("jump-height", 60)) /
                               (-nativeWorldGravity);
            frogBallistic(ball, link.rightTarget, link.leftTarget, link.period);
        }
    }
}

// SetSpecial 0x1bf8b0 stores the period as the jumper's effective delay;
// UnsetSpecial 0x1bf8c4 clears only its flag, retaining that delay afterward.
float frogJumperDelay(int index, float authoredDelay) {
    for (int i = 0; i < frogLinkCount; i++)
        if (index == frogLinks[i].left || index == frogLinks[i].right)
            return frogLinks[i].period;
    return authoredDelay;
}

bool frogJumperStep(int index, float dt) {
    for (int i = 0; i < frogLinkCount; i++) {
        FrogLink &link = frogLinks[i];
        Entity &ball = entities[link.ball];
        if (!ball.state.hp || ball.state.gone || (index != link.left && index != link.right))
            continue;
        jumperNativeStep(index, dt, link.period, true);
        return true;
    }
    return false;
}

// 0x1f2920 disables contact with either linked frog, and schedules one
// recalculation per newly encountered frog after Box2D releases its world lock.
void frogPreSolve(b2Contact *contact) {
    b2Body *a = contact->GetFixtureA()->GetBody();
    b2Body *b = contact->GetFixtureB()->GetBody();
    for (int i = 0; i < frogLinkCount; i++) {
        FrogLink &link = frogLinks[i];
        Entity &ball = entities[link.ball];
        if (!ball.state.hp || ball.state.gone)
            continue;
        int other = -1;
        if (a == bodies[link.ball])
            other = (int)b->GetUserData() - 1;
        else if (b == bodies[link.ball])
            other = (int)a->GetUserData() - 1;
        if (other != link.left && other != link.right)
            continue;
        contact->SetEnabled(false);
        if (ball.state.active != other + 1) {
            ball.state.active = other + 1;
            frogPending[link.ball] = other == link.left ? 1 : 2;
        }
    }
}

// Native PostSolve0x1f3654 clears cooperation during the actual callback.
// A later actor deactivation or a sleeping/stale contact list must not change
// whether that solved event happened. No body mutation is needed while locked.
void frogPostSolveNative(b2Contact *contact) {
    b2Body *a = contact->GetFixtureA()->GetBody();
    b2Body *b = contact->GetFixtureB()->GetBody();
    for (int i = 0; i < frogLinkCount; i++) {
        int index = frogLinks[i].ball;
        if (a == bodies[index] || b == bodies[index])
            entities[index].state.hp = 0;
    }
}

void frogPostStep() {
    for (int i = 0; i < frogLinkCount; i++) {
        FrogLink &link = frogLinks[i];
        Entity &ball = entities[link.ball];
        int pending = frogPending[link.ball];
        frogPending[link.ball] = 0;
        b2Body *body = bodies[link.ball];
        if (!pending || ball.state.gone || !body)
            continue;
        // DoRecalculate0x1f6550 subtracts half a period and recalculates even
        // when a simultaneous PostSolve has already cleared the mode flag.
        ball.state.timer -= link.period * 0.5f;
        Vec from = {ball.state.x, ball.state.y},
            to = pending == 1 ? link.rightTarget : link.leftTarget;
        if (frogBallistic(ball, from, to, link.period)) {
            body->SetLinearVelocity(b2Vec2(ball.state.vx * metres, ball.state.vy * metres));
            body->SetAngularVelocity(ball.state.aux);
            body->SetAwake(true);
        }
    }
}
