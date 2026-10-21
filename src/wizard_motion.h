#pragma once

// Wizard encounter motion and its authored state transitions.

// FinalBossBehaviour::PrePhysicsStep 0x1da934 and OnActivate 0x1df760.
// This header is included after bossAlternate / wizardArmShots are defined.
// Camera positions in the ELF are CENTRES, whereas camX/camY are view edges.
unsigned int wizardRandomWord() {
    WizardMotionState &w = wizardMotion;
    if (w.randomIndex >= 624) {
        for (int i = 0; i < 624; i++) {
            unsigned int y = (w.random[i] & 0x80000000u) | (w.random[(i + 1) % 624] & 0x7fffffffu);
            w.random[i] = w.random[(i + 397) % 624] ^ (y >> 1) ^ ((y & 1) ? 0x9908b0dfu : 0);
        }
        w.randomIndex = 0;
    }
    unsigned int x = w.random[w.randomIndex++];
    x ^= x >> 11;
    x ^= (x << 7) & 0x9d2c5680u;
    x ^= (x << 15) & 0xefc60000u;
    x ^= x >> 18;
    return x;
}

int wizardRandomInt(int low, int high) {
    if (low == high)
        return low;
    unsigned int range = (unsigned int)(high - low + 1), bucket = 0xffffffffu / range;
    if (0xffffffffu % range == range - 1)
        bucket++;
    // boost::uniform_int's full-range engine division/rejection algorithm.
    for (;;) {
        unsigned int value = wizardRandomWord() / bucket;
        if (value < range)
            return low + (int)value;
    }
}

void resetWizardMotion(Entity &boss, unsigned int seed) {
    WizardMotionState &w = wizardMotion;
    memset(&w, 0, sizeof(w));
    w.initialized = 1;
    setWizardHitClockNative(boss, boss.state.active == 3 ? 2.0f : 0.0f);
    memset(&wizardCameraBlend, 0, sizeof(wizardCameraBlend));
    w.baseX = boss.state.x;
    w.baseY = boss.state.y;
    w.periodX = 7;
    w.periodY = 12;
    w.amplitudeX = 10;
    w.amplitudeY = -80;
    // OnActivate0x1e177c gets Camera::GetOrthoSize then halves X. This
    // is runtime view width, unlike the authored combat spread224.
    w.offsetX = viewW * 0.5f - (boss.local.r - boss.local.l) * 0.5f;
    w.initialBeanX = entities[playerIndex].state.x;
    w.shooting = 1;
    for (int i = 0; i < 3; i++)
        w.pending[i] = -1;
    w.random[0] = seed;
    for (unsigned int i = 1; i < 624; i++)
        w.random[i] = 1812433253u * (w.random[i - 1] ^ (w.random[i - 1] >> 30)) + i;
    w.randomIndex = 624;
}

void wizardStartCameraBlend() {
    // Native 0x1dc1b8 / 0x1db984: retain the rendered camera position
    // before restoring FollowObject, not the Bean position or next arena.
    wizardCameraBlend.remaining = 1;
    wizardCameraBlend.fromX = camX;
    wizardCameraBlend.fromY = camY;
}

void wizardCameraPreStep(float dt) {
    // 0x1da9dc..0x1da9f4 runs before state processing. A transition can
    // reset this to1 later in that same step; do not decrement it twice.
    wizardCameraBlend.remaining -= dt;
}

void wizardPostCameraStep(float dt) {
    (void)dt;
    // OnPostUpdate 0x1d3b74 first runs the original camera follower. The
    // caller has already done that; blend its result, then apply the clamp.
    Entity *boss = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS) {
            boss = entities + i;
            break;
        }
    if (!boss)
        return;
    int mode = boss->state.active;
    if (mode != 1 && mode != 2 && mode != 5)
        return;
    float t = wizardCameraBlend.remaining;
    if (t > 0) {
        float weight = t * t * (3 - 2 * t);
        weight = clamp(weight, 0, 1);
        camX = camX * (1 - weight) + wizardCameraBlend.fromX * weight;
        camY = camY * (1 - weight) + wizardCameraBlend.fromY * weight;
    }
    if (mode == 1 || mode == 2) {
        float arena = listNumber(boss->source->behavior("FinalBossBehaviour")->get("area-offsets"),
                                 5 - boss->state.hp);
        camX = minf(camX, arena - viewW * 0.5f);
    }
    cameraState.x = camX;
    cameraState.y = camY;
}

float wizardArena(const Entity &boss) {
    return listNumber(boss.source->behavior("FinalBossBehaviour")->get("area-offsets"),
                      (int)clamp((float)(5 - boss.state.hp), 0, 4));
}

void wizardBeginCombat(Entity &boss) {
    WizardMotionState &w = wizardMotion;
    bossAlternate(boss, false);
    boss.state.active = 3;
    boss.state.timer = 2;
    setWizardHitClockNative(boss, 2);
    // Entering an arena disables both arms (0x1dafc0/0x1dafc8), even
    // though the cycle's shooting flag starts true. They are enabled only
    // by a later four-second transition. Negative ages encode disabled.
    w.cycleAge = 2.7f;
    w.cycleCount = 0;
    w.shooting = 1;
    w.leftAge = w.rightAge = -1;
    w.pendingCount = 0;
    w.prespawnAge = 0;
    camX = wizardArena(boss) - viewW * 0.5f;
    cameraState.x = camX;
}

// 0x1db534 / 0x1db66c / 0x1db788: smooth the three mode vectors,
// derive the camera-relative target, then chase it at no more than 220 px/s.
void wizardMoveBody(Entity &boss, float dt) {
    WizardMotionState &w = wizardMotion;
    State &s = boss.state;
    float width = (boss.local.r - boss.local.l) * 0.5f;
    bool waiting = s.active <= 1;
    float blend = dt;
    float ampY = waiting ? -80.0f : -10.0f;
    float offset = waiting ? viewW * 0.5f - width : 224 - width + 32;
    w.periodX += (7 - w.periodX) * blend;
    w.periodY += (12 - w.periodY) * blend;
    w.amplitudeX += (10 - w.amplitudeX) * blend;
    w.amplitudeY += (ampY - w.amplitudeY) * blend;
    w.offsetX += (offset - w.offsetX) * blend;
    w.offsetY += (0 - w.offsetY) * blend;
    float limit = wizardArena(boss) + 224 - width + 32,
          targetX = minf(camX + viewW * 0.5f + w.offsetX, limit),
          targetY = camY + viewH * 0.5f + w.offsetY;
    if (s.active == 2)
        targetX = limit;
    float delta = targetX - w.baseX, step = minf(absf(delta) / 0.1f, 220) * dt;
    w.baseX = approach(w.baseX, targetX, step);
    w.baseY = w.baseY * (1 - dt / 0.1f) + targetY * (dt / 0.1f);
    w.age += dt;
    s.x = w.baseX + sine(6.28318530718f * w.age / w.periodX) * w.amplitudeX;
    s.y = w.baseY + sine(6.28318530718f * w.age / w.periodY) * w.amplitudeY;
    s.vx = s.vy = 0;
}

bool wizardCategoryActive(int category) {
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == category && !entities[i].state.gone)
            return true;
    return false;
}

int wizardChooseCategory(Entity &boss) {
    const Props &p = *boss.source->behavior("FinalBossBehaviour");
    WizardMotionState &w = wizardMotion;
    int area = 5 - boss.state.hp;
    // 0x1da694: fish-only skips the draw. Other categories are weighted by
    // switch entries, and later summon cycles unlock flying enemies.
    if (listNumber(p.get("area-fish-only"), area) > 0)
        return 4;
    int roll;
    if (area == 4) {
        int low = 0, high = w.cycleCount <= 0 ? 3 : w.cycleCount <= 2 ? 4 : 7;
        if (w.cycleCount == 3)
            low = 6;
        roll = wizardRandomInt(low, high);
    } else
        roll = wizardRandomInt(0, w.cycleCount <= 0 ? 3 : w.cycleCount == 1 ? 6 : 12);
    if (roll == 0)
        return 2;
    if (roll == 1)
        return 6;
    if (roll == 2)
        return 5;
    if (roll == 3)
        return 8;
    if (area == 4)
        return roll <= 5 ? 9 : 3;
    if (roll == 4) {
        if (wizardCategoryActive(1) || wizardCategoryActive(3) || wizardCategoryActive(7))
            return 0;
        return 4;
    }
    if (roll == 5) {
        if (wizardCategoryActive(4) || wizardCategoryActive(3) || wizardCategoryActive(7))
            return 0;
        return 1;
    }
    if (roll == 6 || roll == 7)
        return 9;
    if (roll <= 10) {
        if (wizardCategoryActive(1) || wizardCategoryActive(4) || wizardCategoryActive(7))
            return 0;
        return 3;
    }
    if (wizardCategoryActive(1) || wizardCategoryActive(3) || wizardCategoryActive(4))
        return 0;
    return 7;
}

Vec wizardSpawnOffset(int category, int ordinal) {
    // ActivateEnemies 0x1d75f4: f=2*(i+1)/(N+1)-1; N=3.
    float f = (ordinal - 1) * 0.5f, x = 0, y = 0, axisX = 1, axisY = 0;
    if (category == 2 || category == 8)
        y = -64;
    else if (category == 5 || category == 6)
        y = -32;
    if (category == 8)
        axisX = 1.75f;
    if (category == 4)
        axisY = -1;
    if (category == 1) {
        x = y = -30;
        axisX = 0.5f;
        axisY = 1;
    }
    if (category == 3) {
        x = -16;
        axisX = 0.5f;
        axisY = 0.75f;
    }
    if (category == 7) {
        x = -20;
        axisX = 0.75f;
        axisY = -0.75f;
    }
    if (category == 9)
        y = 104;
    return Vec{x + f * 224 * axisX, y + f * 160 * axisY};
}

void wizardPrepareSummons(Entity &boss) {
    WizardMotionState &w = wizardMotion;
    w.pendingCount = 0;
    // 0x1dba98 retries an empty category up to four times. PrepareSpawnableArray
    // selects every inactive member of ONE pool, never three arbitrary types.
    for (int attempt = 0; attempt < 4 && !w.pendingCount; attempt++) {
        int category = wizardChooseCategory(boss);
        if (!category)
            continue;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == category && entities[i].state.gone && w.pendingCount < 3)
                w.pending[w.pendingCount++] = i;
    }
    w.cycleCount++;
    w.prespawnAge = 0;
}

void wizardSummonPoint(Entity &boss, int index, float *x, float *y) {
    int ordinal = 0;
    for (int i = 0; i < index; i++)
        if (entities[i].bossSpawn == entities[index].bossSpawn)
            ordinal++;
    Vec offset = wizardSpawnOffset(entities[index].bossSpawn, ordinal);
    *x = wizardArena(boss) + offset.x;
    *y = camY + viewH * 0.5f + offset.y;
}

void wizardStopPrespawn(Entity &boss) {
    Object *fx =
        effectClass(idnum(boss.source->behavior("FinalBossBehaviour")->get("prespawn-effect")));
    if (!fx)
        return;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition && effectParticles[i].domain == 0 &&
            effectParticles[i].art == actor(fx->id))
            effectParticles[i].definition = 0;
}

void wizardMaterializeSummons(Entity &boss) {
    WizardMotionState &w = wizardMotion;
    const Props &p = *boss.source->behavior("FinalBossBehaviour");
    wizardStopPrespawn(boss);
    for (int n = 0; n < w.pendingCount; n++) {
        int i = w.pending[n];
        if (i < 0 || i >= entityCount)
            continue;
        Entity &e = entities[i];
        if (!e.state.gone)
            continue;
        float x, y;
        wizardSummonPoint(boss, i, &x, &y);
        e.state = initialState[i];
        e.state.gone = 0;
        e.state.active = 1;
        e.state.x = e.originX = x;
        e.state.y = e.originY = y;
        e.state.timer = 0;
        e.state.vx = e.state.vy = 0;
        resetNativeMotion(i);
        // Enemy::Spawn 0x1d3ef8 moves the orbit centre up by the radius;
        // Orbit::Reset0x284e7c applies the boss-specific270degree phase,
        // so the bottom of that circle coincides with the smoke/spawn point.
        if (e.orbit) {
            e.originY = y + e.radius;
            positionNativeOrbit(e);
        }
        // Spawn0x1d3f8c calls Killable::Respawn, which zeros body velocity
        // at0x239f24..30. Walk has no later reset override: its first solved
        // supporting contact must enable walking before horizontal movement.
        if (e.walk)
            resetWalkNative(e);
        // Enemy::Spawn0x1d3f50 calls Actor::SetPositionAndRotation. The
        // BasePhysics notification0x11f45c immediately SetTransforms even an
        // inactive pooled body; activation must never sweep from its old arena.
        if (bodies[i]) {
            bodies[i]->SetTransform(b2Vec2(e.state.x * metres, e.state.y * metres),
                                    nativePhysicsAngle(e));
            bodies[i]->SetLinearVelocity(b2Vec2(e.state.vx * metres, e.state.vy * metres));
            bodies[i]->SetAngularVelocity(0);
            bodies[i]->SetAwake(true);
        }
        if (oscillatorProxies[i]) {
            oscillatorProxies[i]->SetTransform(bodies[i]->GetPosition(), bodies[i]->GetAngle());
            oscillatorProxies[i]->SetLinearVelocity(bodies[i]->GetLinearVelocity());
            oscillatorProxies[i]->SetAngularVelocity(0);
            oscillatorProxies[i]->SetAwake(true);
        }
        emitEffect(idnum(p.get("spawn-effect")), x, y, e.layer);
    }
    w.pendingCount = 0;
}

void wizardCombatCycle(Entity &boss, float dt) {
    WizardMotionState &w = wizardMotion;
    const Props &p = *boss.source->behavior("FinalBossBehaviour");
    float previous = w.cycleAge;
    w.cycleAge += dt;
    if (w.shooting && w.leftAge >= 0) {
        float left = w.leftAge;
        w.leftAge += dt;
        w.rightAge += dt;
        wizardArmShots(boss, left, w.leftAge, true);
    } else if (!w.shooting) {
        if (previous < 0.2f && w.cycleAge >= 0.2f)
            wizardPrepareSummons(boss);
        if (w.pendingCount && w.cycleAge < 2.7f) {
            // Native keeps three prespawn emitters running during this window.
            // Match emission rate from the effect resource, not one explosion.
            Object *fx = effectClass(idnum(p.get("prespawn-effect")));
            float rate = fx ? fx->props.num("particles-per-second", 0) : 0;
            if (rate > 0) {
                float old = w.prespawnAge;
                w.prespawnAge += dt;
                int count = (int)(w.prespawnAge * rate) - (int)(old * rate);
                if (count)
                    for (int n = 0; n < w.pendingCount; n++) {
                        float x, y;
                        wizardSummonPoint(boss, w.pending[n], &x, &y);
                        emitEffect(fx->id, x, y, boss.layer, 1, 0, count, n + 1);
                    }
            }
        }
        if (previous < 2.7f && w.cycleAge >= 2.7f)
            wizardMaterializeSummons(boss);
    }
    if (w.cycleAge >= 4) {
        w.cycleAge -= (int)(w.cycleAge / 4) * 4;
        bool shootOnly = listNumber(p.get("area-shoot-only"), 5 - boss.state.hp) > 0;
        w.shooting = shootOnly || (!w.shooting && wizardRandomInt(0, 1) == 0);
        w.leftAge = w.rightAge = w.shooting ? 0.0f : -1.0f;
        wizardTroll();
    }
}

// Does not own the final death/cutscene sequence or HP changes. Call once per
// pre-physics step for living native states 0..3; increment state.timer outside.
// Desktop expansion must not close an arena gate around Tom at the
// checkpoint. Native camera-centre entry is retained, with its authored
// 512px framing as an additional progress guard when the view is wider.
// FollowObject's offset otherwise makes a wide rendered centre cross the
// trigger while Tom is still physically inside the previous gate column.
bool wizardArenaEntryReachedNative(const Entity &boss) {
    float centre = camX + viewW * .5f;
    if (viewW > 512 && cameraFollow && cameraFollow->flag("follow-x", true)) {
        int target = cameraTarget >= 0 && cameraTarget < entityCount ? cameraTarget : playerIndex;
        centre = minf(centre, entities[target].state.x + cameraFollow->num("offset-x") * 512);
    }
    return centre >= wizardArena(boss);
}

void wizardNativeAliveStep(Entity &boss, float dt) {
    WizardMotionState &w = wizardMotion;
    State &s = boss.state;
    if (!w.initialized)
        resetWizardMotion(boss, GetTickCount());
    // 0x1daa50..0x1dae74: only the obstacle pool has a 2.5-second timeout.
    // Ordinary summoned enemies remain until killed or explicitly despawned.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == 9 && !entities[i].state.gone &&
            entities[i].state.timer > 2.5f) {
            Entity &e = entities[i];
            e.state.gone = 1;
            emitEffect(idnum(boss.source->behavior("FinalBossBehaviour")->get("spawn-effect")),
                       e.state.x, e.state.y, e.layer);
        }
    if (s.active == 0 && absf(entities[playerIndex].state.x - w.initialBeanX) > 10) {
        s.active = 1;
        w.leftAge = w.rightAge = 0;
    }
    if (s.active == 1 && wizardArenaEntryReachedNative(boss))
        s.active = 2;
    if (s.active == 2 && wizardArenaEntryReachedNative(boss))
        wizardBeginCombat(boss);
    if (s.active == 1) {
        float previous = w.leftAge;
        w.leftAge += dt;
        w.rightAge += dt;
        wizardArmShots(boss, previous, w.leftAge, false);
    }
    if (s.active == 3) {
        float oldHit = wizardHitClockNative(boss);
        if (oldHit < 2) {
            setWizardHitClockNative(boss, oldHit + dt);
            if (oldHit == 0) {
                wizardHurt(boss);
                return;
            }
        } else
            wizardCombatCycle(boss, dt);
    }
    if (s.active <= 3)
        wizardMoveBody(boss, dt);
}
