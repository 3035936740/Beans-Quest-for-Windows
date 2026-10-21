#pragma once

// Regression coverage for selector preview audio. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorPreviewAudioChecks() {
    bool oldSound = soundEnabled, oldMusic = musicEnabled, oldInterface = interfaceEnabled;
    interfaceEnabled = true;
    musicEnabled = false;
    stopMusic();
    char observations[6000] = {};
    for (int scene = 0; scene < 5; scene++)
        for (int mode = 0; mode < 4; mode++) {
            navigate(worldScenes[scene]);
            page.level.geometryCount = 0;
            for (int i = 0; i < page.playfieldCount; i++)
                page.playfield[i].state.gone = 1;
            destroyUiPhysics(page);
            page.bean.state.x = page.bean.state.y = 128;
            page.bean.state.vx = 0;
            page.bean.state.vy = -30;
            page.bean.state.timer = .1f;
            createUiPhysics(page);
            int pad = page.hasBean ? 0 : -1;
            bool launched = false;
            int voices = 0;
            bool audible = false;
            if (pad >= 0) {
                b2BodyDef bd;
                b2Body *floor = page.physics->CreateBody(&bd);
                b2PolygonShape shape;
                shape.SetAsBox(32 * metres, 8 * metres,
                               b2Vec2(128 * metres, (128 + page.bean.hit.b - .2f - 8) * metres), 0);
                floor->CreateFixture(&shape, 0);
                page.minimized = mode != 0;
                page.minimizePhase = mode == 3 ? .1f : (page.minimized ? 1 : 0);
                soundEnabled = mode != 2;
                stopSounds();
                tickInterface(1.0f / 120);
                tickInterface(1.0f / 120);
                launched = page.ordinaryJumpNative && page.bean.state.vy > 0;
                voices = pinballDetailVoiceCount("jump");
                static short samples[8192];
                AudioSilenceKeepAlive keep = audioSilenceKeepAlive;
                audioSilenceKeepAlive = AudioSilenceKeepAlive{};
                mixAudio(samples, 4096);
                for (int i = 0; i < 8192; i++)
                    audible |= samples[i] != 0;
                audioSilenceKeepAlive = keep;
            }
            bool expected = mode == 1 || mode == 3;
            char label[200];
            wsprintfA(label,
                      "scene=%d mode=%d pad=%d launched=%d voices=%d audible=%d expected=%d\r\n",
                      scene, mode, pad, launched, voices, audible, expected);
            lstrcatA(observations, label);
            wsprintfA(
                label,
                "Selector%d actual terrain bounce mode%d launches and %s its jump voice and audible samples",
                scene, mode, expected ? "plays" : "suppresses");
            check(pad >= 0 && launched && voices == (expected ? 1 : 0) && audible == expected,
                  label);
        }
    atomicWrite("test-output/selector-preview-audio-observations.txt", observations,
                lstrlenA(observations));
    stopSounds();
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    interfaceEnabled = oldInterface;
}
