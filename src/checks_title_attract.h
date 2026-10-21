#pragma once

// Regression coverage for title attract. Runs only in the isolated BQ_ENABLE_TESTS executable.
void attractTestPump() {
    MSG m;
    while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
}

void attractAwaitVideo() {
    DWORD start = timeGetTime();
    while (introActive() && !introMovie.ready && introMovie.status == INTRO_PLAYING &&
           timeGetTime() - start < 5000) {
        attractTestPump();
        Sleep(5);
    }
}

void attractTestAdvance(float seconds) {
    for (int i = 0; i < (int)(seconds * 120 + .5f); i++) {
        attractTestPump();
        if (introActive() && !introMovie.ready)
            attractAwaitVideo();
        titleAttractAdvance(1.0f / 120, true, false);
    }
}

void titleAttractChecks() {
    bool oldSound = soundEnabled, oldMusic = musicEnabled, oldInterface = interfaceEnabled,
         oldRunning = running;
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/attract-saves");
    CreateDirectoryA(saveRoot, 0);
    musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    stopSounds();
    interfaceEnabled = true;
    navigate(titleAttractTitle);
    memset(keys, 0, sizeof(keys));
    u32 stageHash = hashBytes(entities, sizeof(Entity) * entityCount),
        saveHash = hashBytes(&checkpoint, sizeof(checkpoint));
    int oldJumps = jumps, focus = page.focus;
    int beanNode = -1;
    for (int i = 0; i < page.level.objectCount; i++)
        if (titleBeanNative(page.level, page.nodes[i].object->id))
            beanNode = i;
    check(
        beanNode >= 0 && page.nodes[beanNode].object->id == 8198813640126290ULL,
        "The original title controller binds its large Bean artwork by the authored bean reference");
    if (beanNode >= 0) {
        UiNode &n = page.nodes[beanNode];
        Rect authored = n.rect;
        float oldAge = page.age;
        u32 originalRect = hashBytes(&n.rect, sizeof(n.rect));
        page.age = 2.5f;
        Rect centerPose = visualRect(page, n);
        page.age = 3.125f;
        Rect topPose = visualRect(page, n);
        page.age = 4.375f;
        Rect bottomPose = visualRect(page, n);
        check(
            absf(centerPose.b - authored.b) < .001f && absf(topPose.b - authored.b - 4) < .001f &&
                absf(bottomPose.b - authored.b + 4) < .001f && topPose.l == authored.l &&
                topPose.r == authored.r && topPose.t - topPose.b == authored.t - authored.b,
            "The actual title Bean floats four authored pixels above and below its position with the native2.5-second period");
        page.age = 5.625f;
        Rect repeatedPose = visualRect(page, n);
        check(
            absf(repeatedPose.b - topPose.b) < .001f &&
                hashBytes(&n.rect, sizeof(n.rect)) == originalRect,
            "Title floating repeats after entry without accumulating movement into the original resource rectangle");
        int button = -1;
        for (int i = 0; i < page.level.objectCount; i++)
            if (buttonProps(*page.nodes[i].object)) {
                button = i;
                break;
            }
        Rect before = visualRect(page, page.nodes[button]);
        page.age += .625f;
        Rect after = visualRect(page, page.nodes[button]);
        check(
            hashBytes(&before, sizeof(before)) == hashBytes(&after, sizeof(after)),
            "Bean floating leaves the original title buttons and their pointer hit rectangles fixed");
        page.age = oldAge;
    }
    {
        int nodes[11];
        for (int i = 0; i < 11; i++)
            nodes[i] = -1;
        int count = 0;
        for (int i = 0; i < page.level.objectCount; i++) {
            int track = titleNativeTrackFor(page.level, page.nodes[i].object->id);
            if (track >= 0) {
                nodes[track] = i;
                count++;
            }
        }
        bool completeBindings = count == 11;
        for (int i = 0; i < 11; i++)
            completeBindings &= nodes[i] >= 0;
        check(
            completeBindings,
            "All eleven original title artwork references bind independently, including both hat layers and the apostrophe");
        if (completeBindings) {
            float oldAge = page.age, oldW = viewW, oldH = viewH;
            viewW = 512;
            viewH = 384;
            page.age = 0;
            Rect scroll = visualRect(page, page.nodes[nodes[1]]),
                 bean = visualRect(page, page.nodes[nodes[0]]);
            check(
                absf(scroll.b - page.nodes[nodes[1]].rect.b - 192) < .001f &&
                    absf(bean.l - page.nodes[nodes[0]].rect.l + 256) < .001f &&
                    absf(bean.b - page.nodes[nodes[0]].rect.b + 768) < .001f,
                "Title entry uses the native half-height scroll offset and Bean's half-width/two-height offscreen origin");
            page.age = .35f;
            Rect b = visualRect(page, page.nodes[nodes[2]]),
                 e = visualRect(page, page.nodes[nodes[3]]),
                 s = visualRect(page, page.nodes[nodes[7]]);
            check(
                absf(b.b - page.nodes[nodes[2]].rect.b + 28.406739f) < .002f &&
                    absf((e.b - page.nodes[nodes[3]].rect.b) -
                         (b.b - page.nodes[nodes[2]].rect.b)) > 5 &&
                    absf(s.b - page.nodes[nodes[7]].rect.b - 192) < .001f,
                "At350ms B follows the independently reconstructed elastic pose while later letters retain their own entry delays");
            page.age = 1.275f;
            bean = visualRect(page, page.nodes[nodes[0]]);
            check(
                absf(bean.l - page.nodes[nodes[0]].rect.l + 74.98066f) < .002f &&
                    absf(bean.b - page.nodes[nodes[0]].rect.b - 113.37579f) < .003f,
                "Bean's native entry midpoint combines horizontal sine easing, vertical overshoot and the continuing four-pixel bob");
            page.age = .95f;
            Rect back = visualRect(page, page.nodes[nodes[9]]),
                 front = visualRect(page, page.nodes[nodes[10]]);
            check(
                absf((back.b - page.nodes[nodes[9]].rect.b) -
                     (front.b - page.nodes[nodes[10]].rect.b)) < .001f &&
                    absf(back.b - page.nodes[nodes[9]].rect.b) > 1,
                "Both authored hat layers share their800ms entry and remain aligned throughout the title motion");
            page.age = 1.7f;
            bool settled = true;
            for (int i = 1; i < 11; i++) {
                Rect r = visualRect(page, page.nodes[nodes[i]]);
                Rect source = page.nodes[nodes[i]].rect;
                settled &= absf(r.b - source.b) < .001f && absf(r.l - source.l) < .001f;
            }
            check(
                settled,
                "All ten logo tracks settle exactly to their original artwork bounds after their individual intervals");
            page.age = 0;
            viewW = 864;
            viewH = 512;
            bean = visualRect(page, page.nodes[nodes[0]]);
            scroll = visualRect(page, page.nodes[nodes[1]]);
            check(
                absf(bean.l - page.nodes[nodes[0]].rect.l + 432) < .001f &&
                    absf(bean.b - page.nodes[nodes[0]].rect.b + 1024) < .001f &&
                    absf(scroll.b - page.nodes[nodes[1]].rect.b - 256) < .001f,
                "Native title origins scale with the current camera width and height instead of a fixed desktop pixel displacement");
            viewW = oldW;
            viewH = oldH;
            page.age = oldAge;
            u32 rects = 0;
            for (int i = 0; i < 11; i++)
                rects ^= hashBytes(&page.nodes[nodes[i]].rect, sizeof(Rect));
            page.age = 9;
            for (int i = 0; i < 11; i++)
                visualRect(page, page.nodes[nodes[i]]);
            u32 after = 0;
            for (int i = 0; i < 11; i++)
                after ^= hashBytes(&page.nodes[nodes[i]].rect, sizeof(Rect));
            check(
                after == rects && titleAttract.idle == 0 && !titleAttractBusy(),
                "Title presentation never mutates authored bounds or advances the separate fifteen-second idle timer");
            page.age = oldAge;
        }
    }
    check(titleAttractMediaAvailable(),
          "The title attract loop reads the original MP4 at data/Media/Movies/intro.mp4");
    {
        float oldAge = page.age, oldTransition = transitionTime;
        page.age = 2.5f;
        transitionTime = 0;
        u8 source[72] = {};
        int sourceLight = 0;
        render();
        glReadBuffer(GL_FRONT);
        for (int i = 0; i < 24; i++) {
            int x = viewportX + viewportW * (1 + i % 6) / 7,
                y = viewportY + viewportH * (1 + i / 6) / 5;
            u8 pixel[4];
            glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            for (int c = 0; c < 3; c++) {
                source[i * 3 + c] = pixel[c];
                sourceLight += pixel[c];
            }
        }
        bool gradual = sourceLight > 3000, black = true;
        for (int step = 1; step <= 4; step++) {
            titleAttract.black = step * .25f;
            render();
            glReadBuffer(GL_FRONT);
            for (int i = 0; i < 24; i++) {
                int x = viewportX + viewportW * (1 + i % 6) / 7,
                    y = viewportY + viewportH * (1 + i / 6) / 5;
                u8 pixel[4];
                glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                for (int c = 0; c < 3; c++) {
                    gradual &= absf(pixel[c] - source[i * 3 + c] * (1 - step * .25f)) <= 2;
                    if (step == 4)
                        black &= pixel[c] == 0;
                }
            }
        }
        check(
            gradual && black,
            "The production title renderer visibly darkens through quarter, half and three-quarter opacity to complete black without relying on a foreground popup");
        titleAttract.black = 0;
        render();
        glReadBuffer(GL_FRONT);
        bool restored = true;
        for (int i = 0; i < 24; i++) {
            int x = viewportX + viewportW * (1 + i % 6) / 7,
                y = viewportY + viewportH * (1 + i / 6) / 5;
            u8 pixel[4];
            glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            for (int c = 0; c < 3; c++)
                restored &= pixel[c] == source[i * 3 + c];
        }
        check(
            restored,
            "Returning the menu fade to zero restores the same original title pixels with no residual black mask");
        page.age = oldAge;
        transitionTime = oldTransition;
        titleAttractRuntimeFrame(15, false);
        bool first = titleAttract.phase == ATTRACT_TITLE_OUT && titleAttract.black > 0 &&
                     titleAttract.black < .1f && !introActive();
        titleAttractRuntimeFrame(10, false);
        check(
            first && titleAttract.phase == ATTRACT_TITLE_OUT && titleAttract.black < .2f &&
                !introActive(),
            "Long background frames count toward fifteen-second inactivity but cannot skip either the first or following visible black-fade frames");
        navigate(titleAttractTitle);
    }
    titleAttractAdvance(14.9f, true, false);
    check(titleAttract.phase == ATTRACT_IDLE && titleAttract.idle > 14 && !introActive(),
          "Title remains interactive before the15-second attract idle threshold");
    keyPressed('Q');
    check(titleAttract.idle == 0 && titleAttract.phase == ATTRACT_IDLE && page.focus == focus,
          "Even an unbound title key resets idle without navigating or starting a movie");
    titleAttract.idle = 14.9f;
    titleAttractAdvance(1, true, true);
    check(titleAttract.idle == 0 && !titleAttractBusy(),
          "Held title input prevents attract playback instead of timing out beneath the player");
    titleAttract.idle = 14.9f;
    titleAttractAdvance(.2f, false, false);
    check(
        titleAttract.phase == ATTRACT_TITLE_OUT && titleAttract.idle > 15,
        "An unfocused title reaches its fifteen-second idle threshold and begins the movie transition");
    titleAttractReset();
    screen = SELECT;
    titleAttractAdvance(30, true, false);
    check(!titleAttractBusy() && titleAttract.idle == 0,
          "The idle movie is restricted to Title and never starts in level selection or gameplay");
    screen = TITLE;
    titleAttract.idle = 14.9f;
    titleAttractAdvance(.2f, true, false);
    check(titleAttract.phase == ATTRACT_TITLE_OUT && titleAttract.black > 0 &&
              titleAttract.black < 1 && musicGain > 0 && musicGain < 1 && !introActive(),
          "Attract entry fades the visible title and its music before opening the movie");
    keyPressed(VK_ESCAPE);
    attractTestAdvance(1);
    check(screen == TITLE && !titleAttractBusy() && running == oldRunning && musicGain == 1 &&
              !introActive(),
          "Input during title fade cancels the demo and cannot also quit the game");
    titleAttract.idle = 14.9f;
    titleAttractAdvance(.2f, true, false);
    attractTestAdvance(.65f);
    bool opened = titleAttract.phase == ATTRACT_MOVIE_IN && screen == INTRO && introActive() &&
                  introMovie.status == INTRO_PLAYING;
    check(
        opened && introMovie.volumeError == 0 && introMovie.next == titleAttractTitle,
        "The original MP4 opens through native Media Foundation with controllable audio and a Title return destination");
    if (opened) {
        check(introMovie.width == 1920 && introMovie.height == 1080 &&
                  absf((float)introMovie.destination.right / introMovie.destination.bottom -
                       (float)introMovie.width / introMovie.height) < .01f,
              "Movie resize preserves the supplied16:9 aspect ratio with black letterboxing");
        titleAttractAdvance(.2f, true, false);
        float gain = introMovie.volume;
        check(gain == 0 && titleAttract.black > 0 && titleAttract.black < 1,
              "Music-off also mutes intro audio while the video fades in");
        titleAttractAdvance(.1f, false, false);
        bool backgroundPlaying = !introMovie.paused;
        titleAttractAdvance(.1f, true, false);
        check(backgroundPlaying && !introMovie.paused,
              "The actual title movie keeps playing across focus loss and focus return");
        MSG child = {};
        child.message = WM_KEYDOWN;
        child.wParam = VK_RETURN;
        bool consumed = titleAttractMessage(child);
        titleAttractAdvance(.2f, true, false);
        check(
            consumed && titleAttract.phase == ATTRACT_MOVIE_OUT && screen == INTRO &&
                introActive() && musicGain > 0 && musicGain < 1,
            "Any child-video key starts a gradual return and is consumed before the Start button can activate");
        keyPressed(VK_ESCAPE);
        pointerDown(0, 0);
        attractTestAdvance(1.2f);
        check(
            screen == TITLE && !titleAttractBusy() && !introActive() && !pointerHeld &&
                page.focus == focus && running == oldRunning && musicGain == 1,
            "Repeated keys and clicks finish one black transition back to the same title without quitting or starting a game");
    }
    titleAttract.idle = 14.9f;
    titleAttractAdvance(.2f, true, false);
    attractTestAdvance(1.2f);
    bool live = introActive() && screen == INTRO;
    MFP_EVENT_HEADER event = {};
    event.eEventType = MFP_EVENT_TYPE_PLAYBACK_ENDED;
    event.pMediaPlayer = (IMFPMediaPlayer *)1;
    introMfEvent(&event);
    bool stale = introMovie.status == INTRO_PLAYING;
    event.pMediaPlayer = introMf.player;
    introMfEvent(&event);
    titleAttractAdvance(0, true, false);
    check(
        live && stale && titleAttract.phase == ATTRACT_MOVIE_OUT,
        "Only the current movie completion notification initiates attract return; stale device callbacks are ignored");
    attractTestAdvance(1.3f);
    check(screen == TITLE && !titleAttractBusy() && titleAttract.idle < 1 && musicGain == 1,
          "A completed movie returns to Title and begins a fresh full idle interval");
    titleAttract.idle = 14.9f;
    titleAttractAdvance(.2f, true, false);
    attractTestAdvance(.65f);
    navigate(18006219643615701355ULL);
    check(
        !titleAttractBusy() && !introActive() && musicGain == 1 && !titleAttract.curtain,
        "Navigation cancels every attract transition and releases video, curtain and music attenuation");
    stopMusic();
    stopSounds();
    short music[] = {4000, -4000};
    short effect[] = {1000, -1000};
    AudioClip song = {0, music, 1}, sfx = {0, effect, 1};
    musicClip = song;
    musicPosition = 0;
    musicEnabled = true;
    setMusicGain(.5f);
    audioVoices[0] = AudioVoice{};
    audioVoices[0].clip = &sfx;
    audioVoices[0].gain = 1;
    audioJumpVoiceTrace[0] = 0;
    short output[2];
    mixAudio(output, 1);
    bool half = output[0] == 1250 && output[1] == -1250;
    setMusicGain(1);
    mixAudio(output, 1);
    bool normal = output[0] == 1000 && output[1] == -1000;
    memset(&musicClip, 0, sizeof(musicClip));
    setMusicGain(1);
    check(
        half && normal,
        "Menu music fade scales only music and leaves concurrent effects and default music PCM unchanged");
    check(
        stageHash == hashBytes(entities, sizeof(Entity) * entityCount) &&
            saveHash == hashBytes(&checkpoint, sizeof(checkpoint)) && oldJumps == jumps,
        "Title attract playback and input never mutate campaign entities, checkpoint or jump count");
    titleAttractReset();
    navigate(titleAttractTitle);
    lstrcpyA(saveRoot, oldRoot);
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
    interfaceEnabled = oldInterface;
    running = oldRunning;
}
