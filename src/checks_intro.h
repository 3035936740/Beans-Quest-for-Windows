#pragma once

// Regression coverage for intro. Runs only in the isolated BQ_ENABLE_TESTS executable.

void makeIntroFixture() {
    // Six 8x8 uncompressed DIB frames, generated here so the regression has
    // no optional FFmpeg or external-media dependency. This is test data only.
    const u32 header[] = {
        0x46464952, 0x588,      0x20495641, 0x5453494c, 0xc0,       0x6c726468, 0x68697661,
        0x38,       0x1046a,    0xb40,      0x0,        0x0,        0x6,        0x0,
        0x1,        0xc0,       0x8,        0x8,        0x0,        0x0,        0x0,
        0x0,        0x5453494c, 0x74,       0x6c727473, 0x68727473, 0x38,       0x73646976,
        0x20424944, 0x0,        0x0,        0x0,        0x1,        0xf,        0x0,
        0x6,        0xc0,       0xffffffff, 0x0,        0x0,        0x80008,    0x66727473,
        0x28,       0x28,       0x8,        0x8,        0x180001,   0x0,        0xc0,
        0x0,        0x0,        0x0,        0x0,        0x5453494c, 0x4b4,      0x69766f6d};
    u8 bytes[1600];
    memcpy(bytes, header, sizeof(header));
    int cursor = sizeof(header);
    for (int frame = 0; frame < 6; frame++) {
        u32 tag = 0x62643030, size = 192;
        memcpy(bytes + cursor, &tag, 4);
        memcpy(bytes + cursor + 4, &size, 4);
        cursor += 8;
        for (int pixel = 0; pixel < 64; pixel++) {
            bytes[cursor++] = frame % 2 ? 255 : 0;
            bytes[cursor++] = 0;
            bytes[cursor++] = frame % 2 ? 0 : 255;
        }
    }
    atomicWrite("test-output/intro-playback-fixture.avi", bytes, cursor);
}

void introChecks() {
    makeIntroFixture();
    interfaceEnabled = true;
    musicEnabled = true;
    navigate(610822198339107ULL);
    int introButton = -1;
    for (int i = 0; i < page.level.objectCount; i++) {
        const Props *p = page.level.objects[i].behavior("LevelButtonBehaviour");
        if (p && levelById(idnum(p->get("level"))) &&
            eq(levelById(idnum(p->get("level")))->props.get("name"), "Cutscene - Movie")) {
            introButton = i;
            break;
        }
    }
    check(introButton >= 0, "Settings retains its authored Watch Intro button");
    if (introButton >= 0) {
        activate(page, introButton);
        check(
            screen == INTRO && introMovie.status == INTRO_PLAYING && introActive() &&
                eq(introMovie.path, "data/Media/Movies/intro.mp4"),
            "Actual settings Watch Intro opens the supplied MP4 when authored movie1/movie2 files are absent");
        DWORD started = timeGetTime();
        while (introMovie.status == INTRO_PLAYING && !introMovie.ready &&
               timeGetTime() - started < 5000) {
            MSG m;
            while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&m);
                DispatchMessageA(&m);
            }
            Sleep(5);
        }
        check(
            introMovie.ready && introMovie.width == 1920 && introMovie.height == 1080 &&
                introMovie.error == 0,
            "The actual settings movie decodes the original1920x1080 video instead of reporting a missing resource");
        tickInterface(2);
        check(
            screen == INTRO && introMovie.status == INTRO_PLAYING,
            "The supplied settings movie waits for its real playback callback rather than a fabricated timeout");
        DWORD endWait = timeGetTime();
        while (introMovie.status == INTRO_PLAYING && timeGetTime() - endWait < 40000) {
            MSG m;
            while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&m);
                DispatchMessageA(&m);
            }
            Sleep(5);
        }
        check(introMovie.status == INTRO_FINISHED,
              "Settings Watch Intro reaches the original MP4's real natural playback completion");
        tickInterface(1 / 120.0f);
        bool settings = screen == OPTIONS && page.level.id == 610822198339107ULL;
        check(
            settings && !introActive() && transitionTime == .35f,
            "Naturally completed Watch Intro returns to its Settings page rather than chapter selection");
        check(settings && page.focus >= 0 &&
                  page.nodes[page.focus].object->id == page.level.objects[introButton].id &&
                  musicClip.frames > 0,
              "Natural Watch Intro return restores the Watch Intro focus and menu music");
        navigate(610822198339107ULL);
        activate(page, introButton);
        confirm();
        check(
            screen == OPTIONS && !introActive() && page.focus == introButton,
            "Accepting during Settings Watch Intro returns to the same Settings button instead of chapter selection");
        navigate(610822198339107ULL);
        activate(page, introButton);
        cancel();
        check(
            screen == OPTIONS && !introActive() && page.focus == introButton,
            "Cancelling the actual settings MP4 releases its player and returns to the same Settings button");
        musicEnabled = false;
        navigate(610822198339107ULL);
        activate(page, introButton);
        confirm();
        check(screen == OPTIONS && !introActive() && musicClip.frames == 0,
              "Settings movie return respects disabled music rather than restarting menu music");
    }
    bool missing =
        introPlayPath("test-output/intentionally-absent-intro.mp4", 18006219643615701355ULL);
    screen = INTRO;
    tickInterface(2);
    check(
        !missing && introMovie.status == INTRO_MISSING && !introActive() && screen == INTRO,
        "An explicitly absent movie still reports missing and cannot fabricate an automatic next-scene transition");
    cancel();
    check(screen == OPTIONS, "Cancelling a genuinely missing movie still returns to settings");
    bool opened = introPlayPath("test-output/intro-playback-fixture.avi", 18006219643615701355ULL);
    check(opened && introMovie.status == INTRO_PLAYING,
          "XP-compatible MCI opens the isolated AVI playback fixture");
    if (opened) {
        screen = INTRO;
        IntroStatus before = introMovie.status;
        introNotify(MCI_NOTIFY_ABORTED, introMovie.device);
        check(before == INTRO_PLAYING && introMovie.status == INTRO_PLAYING,
              "An aborted MCI notification is not mistaken for successful completion");
        DWORD started = timeGetTime();
        while (introMovie.status == INTRO_PLAYING && timeGetTime() - started < 3000) {
            MSG m;
            while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&m);
                DispatchMessageA(&m);
            }
            Sleep(5);
        }
        check(introMovie.status == INTRO_FINISHED,
              "Real AVI playback reaches its Windows completion callback");
        if (introMovie.status == INTRO_FINISHED) {
            tickInterface(1 / 120.0f);
            check(
                screen == WORLD && page.level.id == 18006219643615701355ULL &&
                    transitionTime == 0.35f,
                "Successful intro callback enters the original next scene using its native transition duration");
        }
    }
    introClose();
    navigate(12192614360109026946ULL);
    interfaceEnabled = false;
}
