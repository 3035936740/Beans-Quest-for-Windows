#pragma once

// Runs ending-credit actors in their own muted physics scene, separate from campaign state.

// The ending is a live LevelTask, not an image of its actors. The controller
// mutes every Object (0x1854c8..1854dc), ignores Tom input (0x185a5c), and puts
// Malmo into cutscene mode (0x185764); ordinary actor controllers still tick.
// Everything below belongs to UiScene's world; no stage entity, projectile,
// checkpoint, RNG or animation array is borrowed.
enum {
    CREDITS_PROJECTILE_ID_BASE_NATIVE = 544,
    CREDITS_PROXY_ID_BASE_NATIVE = 576,
    CREDITS_PHYSICS_ID_LIMIT_NATIVE = 1089
};

struct CreditsShotNative {
    Entity entity;
    b2Body *body;
    int owner, animationPool;
    bool live, first, hit;
    Vec hitPoint;
    u32 ignored[(CREDITS_PHYSICS_ID_LIMIT_NATIVE + 31) / 32];
};

struct CreditsShotAnimationNative {
    Object *source;
    Actor *art;
    int owner;
    float age;
};

struct CreditsRuntimeNative {
    BulletSourceState sources[513];
    BulletSourceAnimationNativeState visuals[513];
    CreditsShotNative shots[32];
    CreditsShotAnimationNative shotAnimations[32];
    int shotAnimationCount;
    int emitted[513], warnings[513];
    int frogBall, frogLeft, frogRight, frogPending;
    float frogPeriod;
    Vec frogLeftTarget, frogRightTarget;
    WizardArmMotion arms[2];
    bool grin;
    float grinAge, bossAmplitudeY;
};

CreditsRuntimeNative creditsRuntime;

// BulletSource::OnActivate0x147dc4 creates its pool before firing. Actor's
// PreUpdate0xe6150 advances the selected clip even while SetVisible(false);
// EnemyBullet::Shoot0x1beb44 never restarts that clip.
void advanceCreditsShotAnimationsNative(float dt) {
    if (dt <= 0)
        return;
    for (int i = 0; i < creditsRuntime.shotAnimationCount; i++) {
        CreditsShotAnimationNative &p = creditsRuntime.shotAnimations[i];
        float length = animationLength(animation(p.art, p.source->props.get("default-animation")));
        if (length > 0) {
            p.age += dt;
            if (p.age >= length)
                p.age -= length;
        }
    }
}

int freeCreditsShotAnimationNative(int owner) {
    for (int i = 0; i < creditsRuntime.shotAnimationCount; i++)
        if (creditsRuntime.shotAnimations[i].owner == owner) {
            bool used = false;
            for (int n = 0; n < 32; n++)
                used |= creditsRuntime.shots[n].live && creditsRuntime.shots[n].animationPool == i;
            if (!used)
                return i;
        }
    return -1;
}

float creditsShotAnimationAgeNative(const CreditsShotNative &shot) {
    return creditsRuntime.shotAnimations[shot.animationPool].age;
}

int creditsProjectilePhysicsIdentityNative(UiScene &scene, b2Body *body) {
    int index = (int)body->GetUserData() - 1;
    if (index >= 0 && index <= scene.playfieldCount) {
        if (body == scene.bodies[index])
            return index;
        if (body == scene.oscillatorProxies[index])
            return CREDITS_PROXY_ID_BASE_NATIVE + index;
    }
    int slot = index - 1024;
    if (slot >= 0 && slot < 32) {
        const CreditsShotNative &shot = creditsRuntime.shots[slot];
        if (shot.live && shot.body == body && shot.animationPool >= 0 &&
            shot.animationPool < creditsRuntime.shotAnimationCount)
            return CREDITS_PROJECTILE_ID_BASE_NATIVE + shot.animationPool;
    }
    return -1;
}

bool creditsRuntimeScene(const UiScene &s) {
    return s.level.id == 137502285691ULL && s.hasBean && s.playfield;
}

int creditsEntityById(const UiScene &s, u64 id) {
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.playfield[i].source->id == id)
            return i;
    return -1;
}

// Actor::GetSize returns half-extents. Frog OnActivate0x1f6f5c/6f88
// adds one ball half-height and one frog half-width, without doubling them.
Vec creditsFrogTarget(const Entity &frog, const Entity &ball, float side) {
    return Vec{frog.originX + side * (frog.local.r - frog.local.l) * .5f,
               frog.originY +
                   frog.source->behavior("EnemyJumperBehaviour")->num("jump-height", 60) +
                   (ball.local.t - ball.local.b) * .5f};
}

bool creditsFrogBallistic(Entity &ball, Vec from, Vec to, float period) {
    float time = period * .5f - ball.state.timer;
    if (time <= .0001f)
        return false;
    ball.state.x = from.x;
    ball.state.y = from.y;
    ball.state.vx = (to.x - from.x) / time;
    ball.state.vy = (to.y - from.y) / time - .5f * nativeWorldGravity * time;
    ball.state.aux = nativeFrogSpinNative(1);
    return true;
}

void creditsPositionAttachment(UiScene &s, Entity &e) {
    if (e.parent < 0 || e.parent >= s.playfieldCount)
        return;
    const Entity &parent = s.playfield[e.parent];
    e.state.active = parent.state.active;
    e.state.gone = parent.state.gone;
    if (e.motion == 1) {
        float a = bqCos((parent.state.timer + e.phase) * 3.1415927410125732f / e.period) * e.speed *
                  0.01745329238474369f;
        e.state.x = parent.state.x + bqSin(a) * e.radius;
        e.state.y = parent.state.y - bqCos(a) * e.radius;
    } else {
        float a = nativeOrbitAngle(e.state.timer, e.period, e.phase);
        e.state.x = parent.state.x + bqCos(a) * e.radius;
        e.state.y = parent.state.y + bqSin(a) * e.radius;
    }
}

void creditsAttach(UiScene &s, int parent, u64 klass, float radius, int motion, float period,
                   float amplitude, float phase) {
    Object *base = classById(klass);
    if (!base || s.playfieldCount >= 512)
        return;
    Object *copy = s.memory.array<Object>(1);
    *copy = *base;
    copy->layer =
        s.playfield[parent]
            .layer; // SpawnObject retains the spawned class Z; only its scene layer is inherited.
    Vec c = center(base->rect);
    Entity &child = s.playfield[s.playfieldCount++];
    initializeEntity(child, *copy,
                     shifted(base->rect, s.playfield[parent].state.x - c.x,
                             s.playfield[parent].state.y - c.y - radius));
    child.parent = parent;
    child.motion = motion;
    child.radius = radius;
    child.period = period;
    child.speed = amplitude;
    child.phase = phase;
    child.dynamic = 0;
    creditsPositionAttachment(s, child);
}

// Scene-owned native solved-fixture membership survives disabled contacts
// until EndContact; it is reset on scene/body reconstruction, never serialized.
bool creditsGrounded(UiScene &s, int index) {
    return index >= 0 && index < s.playfieldCount && s.contacts->jumperGround[index] != 0;
}

bool creditsDetector(UiScene &s, int index, int detector, b2Fixture *against = 0) {
    return walkDetectorContainsNative(s.bodies[index], detector, against);
}

void creditsWalkContacts(UiScene &s, int index) {
    Entity &e = s.playfield[index];
    if (!e.walk || !s.bodies[index] || e.state.gone || !e.state.active)
        return;
    if (e.reverseEdge && e.walkCanWalk) {
        bool left = creditsDetector(s, index, 5), right = creditsDetector(s, index, 6);
        if (!walkCloudSpringStepNative(e, s.bodies[index], s.playfield, s.playfieldCount, left,
                                       right)) {
            if (left && !right)
                nativeWalkDirection(e, true);
            else if (right && !left)
                nativeWalkDirection(e, false);
        }
    }
}

class CreditsContactsNative : public UiContacts {
  public:
    explicit CreditsContactsNative(UiScene &s) : UiContacts(s) {}

    void PreSolve(b2Contact *contact, const b2Manifold *old) {
        UiContacts::PreSolve(contact, old);
        b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
        for (int side = 0; side < 2; side++) {
            int index = (int)fs[side]->GetBody()->GetUserData() - 1,
                other = (int)fs[1 - side]->GetBody()->GetUserData() - 1;
            if (index == creditsRuntime.frogBall &&
                (other == creditsRuntime.frogLeft || other == creditsRuntime.frogRight) &&
                scene->playfield[index].state.hp) {
                contact->SetEnabled(false);
                if (scene->playfield[index].state.active != other + 1) {
                    scene->playfield[index].state.active = other + 1;
                    creditsRuntime.frogPending = other == creditsRuntime.frogLeft ? 1 : 2;
                }
            }
            if (index >= 0 && index < scene->playfieldCount &&
                scene->playfield[index].kind == HELPER) {
                Entity &bird = scene->playfield[index];
                if (other == scene->playfieldCount) {
                    collectHelperNative(bird, scene->bean.state, 1);
                    contact->SetEnabled(false);
                } else if (bird.state.active != 1)
                    contact->SetEnabled(false);
            }
            int slot = index - 1024;
            if (slot < 0 || slot >= 32 || !creditsRuntime.shots[slot].live)
                continue;
            CreditsShotNative &shot = creditsRuntime.shots[slot];
            b2Fixture *target = fs[1 - side];
            if (target->IsSensor() || target->GetUserData() == (void *)1 ||
                target->GetUserData() == (void *)2 || target->GetUserData() == (void *)15) {
                contact->SetEnabled(false);
                continue;
            }
            int identity = creditsProjectilePhysicsIdentityNative(*scene, target->GetBody());
            if (identity >= 0 && identity < CREDITS_PHYSICS_ID_LIMIT_NATIVE) {
                if (shot.first)
                    shot.ignored[identity / 32] |= 1u << (identity % 32);
                if (shot.ignored[identity / 32] & (1u << (identity % 32)))
                    contact->SetEnabled(false);
            }
        }
    }

    void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {
        UiContacts::PostSolve(contact, impulse);
        b2Fixture *fs[] = {contact->GetFixtureA(), contact->GetFixtureB()};
        for (int side = 0; side < 2; side++) {
            if (creditsRuntime.frogBall >= 0 &&
                fs[side]->GetBody() == scene->bodies[creditsRuntime.frogBall])
                scene->playfield[creditsRuntime.frogBall].state.hp = 0;
            int slot = (int)fs[side]->GetBody()->GetUserData() - 1 - 1024;
            if (slot >= 0 && slot < 32 && creditsRuntime.shots[slot].live &&
                !fs[1 - side]->IsSensor() && contact->IsEnabled() &&
                contact->GetManifold()->pointCount) {
                creditsRuntime.shots[slot].hit = true;
                creditsRuntime.shots[slot].hitPoint = projectileContactPointNative(contact);
            }
        }
    }
};

void buildCreditsRuntime(UiScene &s) {
    memset(&creditsRuntime, 0, sizeof(creditsRuntime));
    creditsRuntime.frogBall = creditsRuntime.frogLeft = creditsRuntime.frogRight = -1;
    creditsRuntime.arms[0] = creditsRuntime.arms[1] = wizardArmInitial();
    creditsRuntime.bossAmplitudeY = -80;
    int roots = s.playfieldCount;
    for (int i = 0; i < roots; i++) {
        Object &o = *s.playfield[i].source;
        const Props *b = o.behavior("WreckingBallControllerBehaviour");
        if (b) {
            Object *link = classById(idnum(b->get("LinkClass")));
            float half = link ? (link->rect.t - link->rect.b) * 0.5f : 8,
                  distance = (o.rect.t - o.rect.b) * 0.5f;
            int n = (int)b->num("NumLinks", 4);
            for (int j = 0; j < n; j++) {
                distance += half + b->num("LinkSpacing", 1);
                creditsAttach(s, i, idnum(b->get("LinkClass")), distance, 1, b->num("Period", 2),
                              b->num("MaxAngle", 60), b->num("TimeOffset"));
                distance += half;
            }
            Object *end = classById(idnum(b->get("EndClass")));
            creditsAttach(s, i, idnum(b->get("EndClass")),
                          distance + (end ? (end->rect.t - end->rect.b) * 0.5f : 16) +
                              b->num("LastLinkSpacing", -6),
                          1, b->num("Period", 2), b->num("MaxAngle", 60), b->num("TimeOffset"));
        }
        b = o.behavior("OrbitingObjectBehaviour");
        if (b)
            creditsAttach(s, i, idnum(b->get("class")), b->num("radius", 40), 2,
                          b->num("period", 2), 0, b->num("start-angle") * 0.017453293f);
        b = o.behavior("BulletSource");
        if (b) {
            creditsRuntime.sources[i] =
                BulletSourceState{b->num("timeOffset"), maxf(.001f, b->num("timeMin", 3)), 0, 1};
            Object *bullet = classById(idnum(b->get("bulletClass")));
            int count = (int)b->num("bulletPoolSize", 10);
            if (bullet)
                for (int n = 0; n < count; n++) {
                    if (creditsRuntime.shotAnimationCount >= 32)
                        fatal("Ending projectile animation pool is full.");
                    CreditsShotAnimationNative &pool =
                        creditsRuntime.shotAnimations[creditsRuntime.shotAnimationCount++];
                    pool.source = bullet;
                    pool.art = actor(bullet->id);
                    pool.owner = i;
                }
        }
        b = o.behavior("FrogSpecialBallBehaviour");
        if (b) {
            int left = creditsEntityById(s, idnum(b->get("frog-left"))),
                right = creditsEntityById(s, idnum(b->get("frog-right")));
            if (left >= 0 && right >= 0) {
                creditsRuntime.frogBall = i;
                creditsRuntime.frogLeft = left;
                creditsRuntime.frogRight = right;
                creditsRuntime.frogPeriod = maxf(.01f, b->num("period", 2));
                Entity &ball = s.playfield[i];
                creditsRuntime.frogLeftTarget = creditsFrogTarget(s.playfield[left], ball, 1);
                creditsRuntime.frogRightTarget = creditsFrogTarget(s.playfield[right], ball, -1);
                s.playfield[left].state.aux = creditsRuntime.frogPeriod * .5f;
                s.playfield[right].state.aux = 0;
                ball.state.hp = 1;
                ball.state.active = 0;
                ball.state.timer = -.5f *
                                   root(-2 * nativeWorldGravity *
                                        s.playfield[left]
                                            .source->behavior("EnemyJumperBehaviour")
                                            ->num("jump-height", 60)) /
                                   (-nativeWorldGravity);
                creditsFrogBallistic(ball, creditsRuntime.frogRightTarget,
                                     creditsRuntime.frogLeftTarget, creditsRuntime.frogPeriod);
            }
        }
        if (s.playfield[i].kind == BOSS)
            s.playfield[i].state.active = 6;
    }
    createUiPhysics(s);
    delete s.contacts;
    s.contacts = new CreditsContactsNative(s);
    s.physics->SetContactListener(s.contacts);
}

b2Body *creditsCreateShotBody(UiScene &s, Entity &e, int slot) {
    const Props *p = e.source->behavior("CirclePhysicsBehaviour");
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.gravityScale = projectileGravitated(*e.source) ? 1 : 0;
    def.bullet = p == 0;
    def.fixedRotation = p ? p->flag("fixed-rotation", false) : true;
    def.position.Set(e.state.x * metres, e.state.y * metres);
    def.userData = (void *)(1025 + slot);
    b2Body *body = s.physics->CreateBody(&def);
    float width = e.local.r - e.local.l, height = e.local.t - e.local.b;
    b2CircleShape circle;
    if (p)
        nativeCircleFixtureGeometry(circle, nativeCircleAdjustedLocal(e.local, p));
    else
        circle.m_radius = maxf(1, minf(absf(width), absf(height)) * .5f) * metres;
    b2FixtureDef fixture;
    fixture.shape = &circle;
    fixture.density = 1;
    fixture.restitution = 0;
    fixture.friction = .2f;
    if (p)
        nativeBoxCircleFixtureMaterial(fixture, *p);
    fixture.filter.categoryBits = 4;
    fixture.filter.maskBits = nativePhysicsMask(p);
    body->CreateFixture(&fixture);
    return body;
}

void creditsFire(UiScene &s, int owner) {
    Entity &source = s.playfield[owner];
    const Props &p = *source.source->behavior("BulletSource");
    Object *object = classById(idnum(p.get("bulletClass")));
    if (!object)
        return;
    int pool = freeCreditsShotAnimationNative(owner);
    if (pool < 0)
        return;
    for (int i = 0; i < 32; i++)
        if (!creditsRuntime.shots[i].live) {
            CreditsShotNative &shot = creditsRuntime.shots[i];
            if (shot.body)
                s.physics->DestroyBody(shot.body);
            memset(&shot, 0, sizeof(shot));
            shot.owner = owner;
            shot.animationPool = pool;
            shot.live = shot.first = true;
            Vec c = center(object->rect);
            initializeEntity(shot.entity, *object,
                             shifted(object->rect, source.state.x - c.x, source.state.y - c.y));
            shot.entity.kind = ENEMY;
            shot.entity.dynamic = 1;
            shot.entity.layer = source.layer;
            shot.entity.z = source.z - 1;
            BulletSourceState &aim = creditsRuntime.sources[owner];
            if (aim.initialized < 2)
                alignBulletSourceAimNative(source, p, aim, &s.bean.state);
            float a = aim.angle;
            const Props *bullet = object->behavior("EnemyBulletBehaviour");
            float speed = bullet ? bullet->num("speed", 125) : 125;
            shot.entity.state.vx = bqCos(a) * speed;
            shot.entity.state.vy = bqSin(a) * speed;
            shot.body = creditsCreateShotBody(s, shot.entity, i);
            if (shot.body)
                shot.body->SetLinearVelocity(
                    b2Vec2(shot.entity.state.vx * metres, shot.entity.state.vy * metres));
            creditsRuntime.emitted[owner]++;
            return;
        }
}

void creditsHelperPostStep(UiScene &s, float dt) {
    for (int i = 0; i < s.playfieldCount; i++) {
        Entity &bird = s.playfield[i];
        if (bird.kind != HELPER)
            continue;
        State &state = bird.state;
        if (state.active == 1) {
            bool blocked = s.contacts->helperSolved[i];
            s.contacts->helperSolved[i] = false;
            if (blocked) {
                sound("bird");
                stopHelper(bird);
            } else if (s.contacts->helperStopPending[i]) {
                sound("bird");
                stopHelper(bird);
            } else {
                const Props &p = *bird.source->behavior("HelperBirdBehaviour");
                float blend = clamp(state.timer / .1f, 0, 1);
                state.x = bird.originX * (1 - blend) +
                          (s.bean.state.x + p.num("follow-offset-x")) * blend;
                state.y = bird.originY * (1 - blend) +
                          (s.bean.state.y + p.num("follow-offset-y", 34)) * blend;
                state.aux = s.bean.state.aux;
            }
        }
        s.contacts->helperStopPending[i] = false;
        if (s.bodies[i]) {
            s.bodies[i]->SetTransform(b2Vec2(state.x * metres, state.y * metres), 0);
            s.bodies[i]->SetLinearVelocity(b2Vec2(0, 0));
        }
        state.vx = state.vy = 0;
    }
    (void)dt;
}

// EnemyBullet activation reads speed/will-rotate/effect, not the unused
// follow-effect resource field. PostSolve0x1ba4a8 passes its world contact
// point to Deactivate0x1bbea8, which positions and explodes the effect once.
// Ordinary EnemyBullet has no camera/world-edge expiry; only a real hit
// or explicit owner/world despawn releases its pool entry.
void creditsProjectilePostStepNative(UiScene &s, float dt) {
    if (dt <= 0)
        return;
    for (int i = 0; i < 32; i++) {
        CreditsShotNative &shot = creditsRuntime.shots[i];
        if (!shot.live || !shot.body)
            continue;
        shot.first = false;
        shot.entity.state.timer += dt;
        b2Vec2 p = shot.body->GetPosition(), v = shot.body->GetLinearVelocity();
        shot.entity.state.x = p.x / metres;
        shot.entity.state.y = p.y / metres;
        shot.entity.state.vx = v.x / metres;
        shot.entity.state.vy = v.y / metres;
        const Props *behaviour = shot.entity.source->behavior("EnemyBulletBehaviour");
        if (behaviour && behaviour->flag("will-rotate"))
            shot.entity.state.angle = bqAtan2(v.y, v.x) * 57.29578f;
        if (shot.hit) {
            shot.live = false;
            shot.body->SetActive(false);
            if (behaviour)
                emitEffect(idnum(behaviour->get("effect")), shot.hitPoint.x, shot.hitPoint.y,
                           shot.entity.layer, 1, 1);
        }
    }
}

void tickCreditsRuntime(UiScene &s, float dt) {
    if (!creditsRuntimeScene(s) || !s.physics || dt <= 0)
        return;
    ScopedObjectSoundMuteNative objectMute;
    advanceCreditsShotAnimationsNative(dt);
    tickEffects(dt, 1);
    tickUiPlayfield(s, dt, false);
    for (int i = 0; i < s.playfieldCount; i++) {
        Entity &e = s.playfield[i];
        State &state = e.state;
        if (state.gone || !state.active && proximityControlled(e))
            continue;
        if (e.motion) {
            creditsPositionAttachment(s, e);
            continue;
        }
        if (e.kind == HELPER) {
            updateHelper(e, dt);
            continue;
        }
        if (e.kind == BOSS) {
            creditsRuntime.bossAmplitudeY += (-10 - creditsRuntime.bossAmplitudeY) * dt;
            state.x = e.originX + bqSin(state.timer * 6.2831854820251465f / 7) * 10;
            state.y = e.originY +
                      bqSin(state.timer * 6.2831854820251465f / 12) * creditsRuntime.bossAmplitudeY;
            wizardArmTick(creditsRuntime.arms[0], dt, 0);
            wizardArmTick(creditsRuntime.arms[1], dt, 0);
            if (!creditsRuntime.grin && s.cameraY + viewH * .5f < state.y)
                creditsRuntime.grin = true;
            if (creditsRuntime.grin)
                creditsRuntime.grinAge += dt;
            continue;
        }
        if (e.walk) {
            creditsWalkContacts(s, i);
            walkPrePhysicsNative(e);
        }
        const Props *jump = e.source->behavior("EnemyJumperBehaviour");
        if (jump) {
            bool linked = (i == creditsRuntime.frogLeft || i == creditsRuntime.frogRight) &&
                          creditsRuntime.frogBall >= 0;
            bool special = linked && s.playfield[creditsRuntime.frogBall].state.hp;
            float delay = linked ? creditsRuntime.frogPeriod : jump->num("jump-delay", 1);
            jumperNativeStepState(e, dt, delay, special, creditsGrounded(s, i), s.bean.state.x);
        }

        const Props *source = e.source->behavior("BulletSource");
        if (source) {
            BulletSourceState &clock = creditsRuntime.sources[i];
            BulletSourceAnimationNativeState &visual = creditsRuntime.visuals[i];
            nativeBulletSourceAnimationBefore(visual, e.art, clock.clock, clock.interval, dt);
            if (visual.phase == SOURCE_VISUAL_PRE)
                creditsRuntime.warnings[i]++;
            float desired = trackBulletSourceAimNative(e, *source, clock, &s.bean.state, dt);
            advanceBulletSourceClockNative(clock, dt);
            if (clock.clock > clock.interval && (!*source->get("targetValidAngles") ||
                                                 sourceAimCloseNative(clock.angle, desired))) {
                creditsFire(s, i);
                nativeBulletSourceAnimationFired(visual, e.art);
                clock.clock -= (int)(clock.clock / clock.interval) * clock.interval;
            }
        }
    }
    Entity &bean = s.bean;
    bean.state.vx = approach(bean.state.vx, 0, bean.physics->num("stop-acceleration", 200) * dt);
    int helper = -1;
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.playfield[i].kind == HELPER && s.playfield[i].state.active == 1)
            helper = i;
    if (helper >= 0 &&
        bean.state.vy <=
            s.playfield[helper].source->behavior("HelperBirdBehaviour")->num("max-fly-speed", 60)) {
        const Props &fly = *s.playfield[helper].source->behavior("HelperBirdBehaviour");
        bean.state.vy = approach(bean.state.vy, fly.num("max-fly-speed", 60),
                                 fly.num("fly-acceleration", 1000) * dt);
    }
    stepUiPhysics(s, dt);
    creditsHelperPostStep(s, dt);
    // Match campaign ordering: an actual solved callback ends the mode, but
    // an already queued native DoRecalculate still supplies its final throw.
    if (creditsRuntime.frogBall >= 0) {
        Entity &ball = s.playfield[creditsRuntime.frogBall];
        b2Body *body = s.bodies[creditsRuntime.frogBall];
        if (body && !ball.state.gone && creditsRuntime.frogPending) {
            ball.state.timer -= creditsRuntime.frogPeriod * .5f;
            Vec to = creditsRuntime.frogPending == 1 ? creditsRuntime.frogRightTarget
                                                     : creditsRuntime.frogLeftTarget;
            if (creditsFrogBallistic(ball, Vec{ball.state.x, ball.state.y}, to,
                                     creditsRuntime.frogPeriod)) {
                body->SetLinearVelocity(b2Vec2(ball.state.vx * metres, ball.state.vy * metres));
                body->SetAngularVelocity(ball.state.aux);
            }
        }
        creditsRuntime.frogPending = 0;
    }
    // Ordinary-body Pinball callbacks are already dispatched by stepUiPhysics.
    creditsProjectilePostStepNative(s, dt);
}

void drawCreditsEntity(UiScene &s, Entity &e, int index) {
    if (e.kind == TURRET) {
        const BulletSourceAnimationNativeState &state = creditsRuntime.visuals[index];
        const char *name = e.source->props.get("default-animation");
        float age = e.state.timer;
        AnimationEndAction end = ANIMATION_LOOP;
        if (state.phase == SOURCE_VISUAL_PRE) {
            name = "pre-shoot";
            age = state.preAge;
            end = ANIMATION_STOP;
        } else if (state.phase == SOURCE_VISUAL_IDLE) {
            name = "idle-shoot";
            age = state.idleAge;
        } else if (state.phase == SOURCE_VISUAL_POST) {
            name = "post-shoot";
            age = state.postAge;
            end = ANIMATION_STOP;
        }
        drawActor(e.art, entityRect(e), name, age, e.state.angle, false, -1, 1, end);
        return;
    }
    if (e.kind == BOSS) {
        for (int part = 0; part < 5; part++) {
            if (part == 2) {
                drawEntity(e, &s.level, s.cameraX, s.cameraY, s.age, true);
                float x = e.state.x, y = e.state.y;
                drawActor(actor(106556998914888ULL), Rect{x - 53, y - 38, x + 43, y + 90},
                          creditsRuntime.grin && creditsRuntime.grinAge < 1 ? "trolling2" : "",
                          creditsRuntime.grinAge, 0, false, -1, 1,
                          creditsRuntime.grin && creditsRuntime.grinAge < 1 ? ANIMATION_STOP
                                                                            : ANIMATION_LOOP);
                continue;
            }
            bool left = part >= 3;
            WizardArmPose pose = wizardArmPose(Vec{e.state.x, e.state.y}, left, 0,
                                               creditsRuntime.arms[left ? 1 : 0]);
            drawActor(actor(part == 0 || part == 3 ? 761998489631ULL
                            : left                 ? 843976466165ULL
                                                   : 866681511178ULL),
                      wizardPartRect(part == 0 || part == 3 ? pose.sleeve : pose.hand), "", s.age);
        }
        return;
    }
    drawEntity(e, &s.level, s.cameraX, s.cameraY, s.age);
}

void drawCreditsShotNative(const CreditsShotNative &shot) {
    const Entity &e = shot.entity;
    drawActor(e.art, entityRect(e), e.source->props.get("default-animation"),
              creditsShotAnimationAgeNative(shot), e.state.angle);
}

bool drawCreditsRuntimeNode(UiScene &s, Object &o) {
    if (!creditsRuntimeScene(s))
        return false;
    if (o.has("TomPlayerBehaviour")) {
        drawEntity(s.bean, &s.level, s.cameraX, s.cameraY, s.age);
        return true;
    }
    bool handled = false;
    for (int i = 0; i < s.playfieldCount; i++)
        if (s.playfield[i].source == &o) {
            for (int j = 0; j < 32; j++)
                if (creditsRuntime.shots[j].live && creditsRuntime.shots[j].owner == i)
                    drawCreditsShotNative(creditsRuntime.shots[j]);
            drawCreditsEntity(s, s.playfield[i], i);
            for (int j = 0; j < s.playfieldCount; j++)
                if (s.playfield[j].parent == i)
                    drawCreditsEntity(s, s.playfield[j], j);
            handled = true;
        }
    return handled;
}
