#pragma once

// Regression coverage for wizard music. Runs only in the isolated BQ_ENABLE_TESTS executable.
void wizardMusicChecks() {
    bool oldMusic = musicEnabled, oldSound = soundEnabled, oldInterface = interfaceEnabled,
         oldDebug = debugMode;
    Progress oldProgress = progress;
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/wizard-music-saves");
    CreateDirectoryA(saveRoot, 0);
    interfaceEnabled = debugMode = soundEnabled = false;
    musicEnabled = true;
    stopSounds();
    stopMusic();
    int stage = stageById(976871008727ULL);
    startStage(stage);
    readyTimer = 0;
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    check(boss >= 0 && playingMusic == idnum(current.props.get("music")) && musicClip.frames > 0,
          "The living Wizard stage still starts its original boss music");
    if (boss >= 0) {
        Entity &wizard = entities[boss];
        wizard.state.hp = 1;
        wizard.state.active = 3;
        wizard.state.timer = 2.1f;
        wizardHurt(wizard);
        check(
            wizard.state.active == 4 && playingMusic == 0 && musicClip.frames == 0,
            "The actual final hit stops the boss music immediately before the death animation finishes");
        keyPressed('M');
        keyPressed('M');
        check(
            musicEnabled && playingMusic == 0 && !musicClip.frames,
            "Toggling music with the real M handler during Wizard death preserves the authored silent state");
        updateWizard(wizard, 3.01f);
        keyPressed('M');
        keyPressed('M');
        check(wizard.state.active == 5 && wizard.state.gone && musicEnabled && playingMusic == 0,
              "After Wizard death the same music toggle cannot revive the removed boss track");
        stageInterface(false);
        readyTimer = 0;
        openPause(false);
        int toggle = -1;
        for (int i = 0; i < hudScene.level.objectCount; i++)
            if (hudScene.nodes[i].object->has("MusicToggleBehaviour"))
                toggle = i;
        if (toggle >= 0) {
            activate(hudScene, toggle);
            activate(hudScene, toggle);
        }
        check(
            toggle >= 0 && musicEnabled && playingMusic == 0 && !musicClip.frames,
            "The actual pause-menu music button also keeps a defeated Wizard stage silent after off and on");
        resetPauseInterfaceNative();
        screen = PLAY;
        captureCheckpoint(true);
        restartStage();
        bool restarted =
            playingMusic == idnum(current.props.get("music")) && entities[boss].state.active < 4;
        check(restarted,
              "A genuine stage restart restores the living Wizard and its original battle music");
        wizard.state.hp = 1;
        wizard.state.active = 3;
        wizard.state.timer = 2.1f;
        wizardHurt(wizard);
        updateWizard(wizard, 3.01f);
        captureCheckpoint(true);
        stopMusic();
        restoreCheckpoint();
        check(
            entities[boss].state.active == 5 && playingMusic == 0 && !musicClip.frames,
            "Restoring a post-Wizard checkpoint preserves silence instead of restarting the scene's default battle music");
        startStage(stage, true);
        check(
            entities[boss].state.active == 5 && playingMusic == 0 && !musicClip.frames,
            "Starting a stage from its defeated-Wizard checkpoint does not replay boss music after the restore");
    }
    navigate(titleAttractTitle);
    check(playingMusic == titleAttractMenuMusic && musicClip.frames > 0,
          "Leaving the defeated Wizard for the title restores normal menu music");
    startStage(0);
    keyPressed('M');
    bool off = !musicEnabled && playingMusic == 0;
    keyPressed('M');
    check(off && musicEnabled && playingMusic == idnum(current.props.get("music")),
          "Ordinary stages still stop and resume their authored music through M");
    stopMusic();
    stopSounds();
    musicEnabled = false;
    startStage(0);
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    progress = oldProgress;
    lstrcpyA(saveRoot, oldRoot);
}
