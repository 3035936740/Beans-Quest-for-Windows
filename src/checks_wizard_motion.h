#pragma once

// Regression coverage for wizard motion. Runs only in the isolated BQ_ENABLE_TESTS executable.

void wizardMotionChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    int stage = -1;
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].props.num("bean-world") == 6 &&
            levels[stageList[i]].props.num("bean-level") == 1)
            stage = i;
    check(stage >= 0,
          "The authored Wizard encounter exists for native motion and summoning checks");
    if (stage < 0)
        return;
    startStage(stage);
    readyTimer = 0;
    int index = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            index = i;
    check(index >= 0, "The original FinalBossBehaviour instance resolves");
    if (index < 0)
        return;
    Entity &boss = entities[index];
    State &bean = entities[playerIndex].state;
    const float dt = 1.0f / 120.0f;
    resetWizardMotion(boss, 5489);
    unsigned int first = wizardRandomWord(), second = wizardRandomWord();
    check(
        first == 3499211612u && second == 581869302u,
        "Wizard summon RNG reproduces the MT19937 reference vector used by the original boost engine");
    resetWizardMotion(boss, 5489);
    float x = boss.state.x;
    camX = 0;
    camY = 0;
    bean.x = wizardMotion.initialBeanX + 10;
    wizardNativeAliveStep(boss, dt);
    check(boss.state.active == 0,
          "Wizard waits at exactly ten pixels of initial Bean displacement");
    bean.x += 0.1f;
    wizardNativeAliveStep(boss, dt);
    check(
        boss.state.active == 1 && absf(boss.state.x - x) < 5,
        "Waking the wizard preserves its authored position and chases the camera without teleporting to the first arena");
    float previous = wizardMotion.baseX;
    camX = 2000;
    wizardMoveBody(boss, dt);
    check(absf(wizardMotion.baseX - previous) <= 220 * dt + 0.001f,
          "Wizard base motion obeys the original 220-pixel horizontal chase cap");
    // Centre-versus-left regression: the camera left is still 256 px before
    // the arena here. Native advances through both gate states this step.
    camX = 3856 - 256;
    cameraState.x = camX;
    wizardNativeAliveStep(boss, dt);
    check(
        boss.state.active == 3 && (alternationBits & 4),
        "The first arena closes as soon as the camera centre reaches its authored offset, without a 1.5-second entry delay");
    // Actual resource pools and spawn offsets, with deterministic seed only
    // for reproducibility of this check. The original live seed is not known.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn)
            entities[i].state.gone = 1;
    resetWizardMotion(boss, 5489);
    boss.state.active = 3;
    boss.state.hp = 5;
    wizardBeginCombat(boss);
    wizardCombatCycle(boss, 1.31f);
    check(
        !wizardMotion.shooting && wizardMotion.pendingCount == 0,
        "The initial combat shooting window completes at the original four-second cycle boundary");
    wizardCombatCycle(boss, 0.21f);
    int count = wizardMotion.pendingCount;
    bool same = count == 3;
    int category = count ? entities[wizardMotion.pending[0]].bossSpawn : 0;
    for (int n = 0; n < count; n++)
        same &= entities[wizardMotion.pending[n]].bossSpawn == category &&
                entities[wizardMotion.pending[n]].state.gone;
    check(
        same && (category == 2 || category == 5 || category == 6 || category == 8),
        "At0.2s the Wizard prepares three inactive members of one of the four native first-cycle pools without hardcoding the seed-dependent category");
    wizardCombatCycle(boss, 2.45f);
    bool hidden = true;
    for (int n = 0; n < wizardMotion.pendingCount; n++)
        hidden &= entities[wizardMotion.pending[n]].state.gone != 0;
    check(hidden && wizardMotion.pendingCount == 3,
          "Prepared enemies remain hidden until the original 2.7-second materialization boundary");
    wizardCombatCycle(boss, 0.04f);
    int active = 0;
    bool offsets = true;
    int ordinal = 0;
    float spacing = category == 8 ? 196.0f : 112.0f,
          spawnY = (category == 5 || category == 6) ? 160.0f : 128.0f;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == category) {
            Entity &e = entities[i];
            if (!e.state.gone) {
                active++;
                offsets &= absf(e.state.x - (3856 + (ordinal - 1) * spacing)) < 0.01f &&
                           absf(e.state.y - spawnY) < 0.01f;
            }
            ordinal++;
        }
    check(
        active == 3 && offsets && wizardMotion.pendingCount == 0,
        "All three selected first-cycle enemies materialize together at the native category-specific224/160 formation offsets");
    int bat = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].bossSpawn == 1) {
            bat = i;
            break;
        }
    if (bat >= 0) {
        Entity &e = entities[bat];
        e.state.gone = 1;
        wizardMotion.pendingCount = 1;
        wizardMotion.pending[0] = bat;
        float sx, sy;
        wizardSummonPoint(boss, bat, &sx, &sy);
        wizardMaterializeSummons(boss);
        check(
            e.orbit && absf(e.originX - sx) < 0.001f &&
                absf(e.originY - (sy + e.radius)) < 0.001f && absf(e.state.x - sx) < 0.001f &&
                absf(e.state.y - sy) < 0.001f,
            "Summoned bat uses the native raised centre and270degree Orbit Reset to appear at its smoke point");
    } else
        check(false, "Authored bat summon pool exists for Orbit Reset regression");
    // Body phase must be independent of cycle resets and arena handoff.
    boss.state.active = 2;
    boss.state.hp = 4;
    float phase = wizardMotion.age;
    camX = 4000;
    wizardNativeAliveStep(boss, dt);
    check(
        boss.state.active == 2 && wizardMotion.age > phase && wizardMotion.baseX < 4368 + 128,
        "Between arenas the wizard continuously follows the capped native target without dropping offscreen or resetting its wobble phase");
    startStage(0);
    readyTimer = 0;
}
