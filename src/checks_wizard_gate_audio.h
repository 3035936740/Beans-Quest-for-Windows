#pragma once

// Regression coverage for wizard gate audio. Runs only in the isolated BQ_ENABLE_TESTS executable.
u32 wizardGateSwitchSerial() {
    AudioClip *clip = 0;
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, "switch"))
            clip = &soundClips[i].clip;
    u32 serial = 0;
    for (int i = 0; i < 32; i++)
        if (clip && audioVoices[i].clip == clip && audioVoices[i].serial > serial)
            serial = audioVoices[i].serial;
    return serial;
}

void wizardGateAudioChecks() {
    interfaceEnabled = false;
    musicEnabled = false;
    soundEnabled = true;
    startStage(stageById(976871008727ULL));
    readyTimer = 0;
    stopSounds();
    int bi = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            bi = i;
    bool close = true, open = true;
    if (bi < 0) {
        check(false, "Original Wizard resolves for gate audio");
        return;
    }
    Entity &boss = entities[bi];
    for (int area = 0; area < 5; area++) {
        boss.state.hp = 5 - area;
        u32 before = wizardGateSwitchSerial(), voices = audioVoicesStarted;
        wizardBeginCombat(boss);
        close &= wizardGateSwitchSerial() > before && audioVoicesStarted == voices + 1;
        before = wizardGateSwitchSerial();
        voices = audioVoicesStarted;
        bossAlternate(boss, true);
        open &= wizardGateSwitchSerial() == before && audioVoicesStarted == voices;
    }
    check(
        close,
        "Each of five real Wizard arena-start events queues exactly one original switch effect for all gate actlets");
    check(open,
          "Original Wizard gate-open events remain silent rather than replaying the close effect");
    soundEnabled = false;
    stopSounds();
    startStage(0);
}
