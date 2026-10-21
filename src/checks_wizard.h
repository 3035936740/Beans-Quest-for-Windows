#pragma once

// Regression coverage for wizard. Runs only in the isolated BQ_ENABLE_TESTS executable.

void wizardFollowupChecks() {
    interfaceEnabled = false;
    musicEnabled = soundEnabled = false;
    startStage(49);
    readyTimer = 0;
    int index = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS) {
            index = i;
            break;
        }
    check(index >= 0, "Original Wizard exists for native combat follow-up");
    if (index < 0)
        return;
    Entity &boss = entities[index];
    State &bean = entities[playerIndex].state;
    float x = initialState[playerIndex].x;
    bean.x = x + 10;
    updateWizard(boss, 0.01f);
    check(boss.state.active == 0,
          "Wizard remains idle at exactly ten pixels of initial Bean displacement");
    bean.x = x - 10.1f;
    updateWizard(boss, 0.01f);
    check(
        boss.state.active == 1,
        "Wizard wakes on native absolute initial displacement without an invented global coordinate or delay");
    memset(bullets, 0, sizeof(bullets));
    boss.state.x = 4000;
    boss.state.y = 250;
    bean.x = 3800;
    bean.y = 100;
    wizardArmShots(boss, 0, 0.21f, true);
    int early = 0;
    for (int i = 0; i < 64; i++)
        early += bullets[i].life > 0;
    wizardArmShots(boss, 0.21f, 0.71f, true);
    int later = 0;
    for (int i = 0; i < 64; i++)
        later += bullets[i].life > 0;
    check(
        early == 1 && later == 2,
        "Native independent arm phases produce staggered shots instead of a shared 1.7-second timer");
    for (int i = 0; i < 20; i++)
        wizardShot(boss, 1);
    int active = 0;
    for (int i = 0; i < 64; i++)
        active += bullets[i].life > 0;
    check(active == 10,
          "Wizard respects its original ten-projectile pool without replacing active shots");
    boss.state.active = 3;
    boss.state.hp = 1;
    boss.state.timer = 2;
    wizardHurt(boss);
    check(
        boss.state.hp == 0 && boss.state.active == 4 && wizardInputLocked() &&
            pinballPendingNative.pending &&
            absf(pinballPendingNative.normal.x / pinballPendingNative.normal.y + 0.45f / 0.35f) <
                0.001f,
        "Final Wizard hit requests its original normalized bounce vector and starts the controlled death sequence before Tom consumes it");
    cloudRenderBegin();
    drawEntity(boss);
    u32 hidden = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    u32 empty = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    check(hidden == empty, "Wizard body, head and hands disappear immediately on the final hit");
    updateWizard(boss, 2.99f);
    check(!boss.state.gone && boss.state.active == 4,
          "Final Wizard sequence remains active before its original three-second boundary");
    updateWizard(boss, 0.02f);
    check(boss.state.gone && boss.state.active == 5 && !wizardInputLocked(),
          "Final Wizard sequence ends after three seconds and releases Bean input");
    startStage(0);
}
