#pragma once

// Title idle timer and automatic intro playback lifecycle.

enum TitleAttractPhase {
    ATTRACT_IDLE,
    ATTRACT_TITLE_OUT,
    ATTRACT_MOVIE_IN,
    ATTRACT_MOVIE,
    ATTRACT_MOVIE_OUT,
    ATTRACT_TITLE_IN
};

struct TitleAttractState {
    TitleAttractPhase phase;
    float idle, age, black, startBlack, startVolume, startGain;
    bool unavailable;
    HWND curtain;
    bool pointerKnown;
    float pointerX, pointerY;
};

TitleAttractState titleAttract;
const float titleAttractIdleSeconds = 15, titleAttractFadeSeconds = .6f;
const u64 titleAttractMenuMusic = 3080012186760326ULL, titleAttractTitle = 12192614360109026946ULL;

bool titleAttractBusy() {
    return titleAttract.phase != ATTRACT_IDLE;
}

void titleAttractCurtain() {
    HWND h = titleAttract.curtain;
    // Menus draw their fade in OpenGL. The native movie child needs a separate
    // curtain, kept immediately above its owner even when another app is active.
    if (!introActive() || titleAttract.black <= 0 || !IsWindowVisible(window) || IsIconic(window)) {
        if (h)
            ShowWindow(h, SW_HIDE);
        return;
    }
    if (!h) {
        WNDCLASSA c = {};
        c.lpfnWndProc = DefWindowProcA;
        c.hInstance = GetModuleHandleA(0);
        c.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        c.lpszClassName = "BeansQuestMovieCurtain";
        RegisterClassA(&c);
        h = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
                            c.lpszClassName, "", WS_POPUP | WS_DISABLED, 0, 0, 1, 1, window, 0,
                            c.hInstance, 0);
        titleAttract.curtain = h;
    }
    if (!h)
        return;
    RECT r;
    GetClientRect(window, &r);
    POINT origin = {0, 0};
    ClientToScreen(window, &origin);
    SetLayeredWindowAttributes(h, 0, (BYTE)(int)(clamp(titleAttract.black, 0, 1) * 255 + .5f),
                               LWA_ALPHA);
    HWND above = GetWindow(window, GW_HWNDPREV);
    if (above == h)
        above = GetWindow(h, GW_HWNDPREV);
    SetWindowPos(h, above ? above : HWND_TOP, origin.x, origin.y, r.right, r.bottom,
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

void titleAttractReset() {
    if (titleAttractBusy())
        introClose();
    if (titleAttract.curtain)
        DestroyWindow(titleAttract.curtain);
    titleAttract = TitleAttractState{};
    setMusicGain(1);
}

bool titleAttractMediaAvailable() {
    return introFileExists("data/Media/Movies/intro.mp4");
}

bool titleAttractOpen() {
    return introPlayPath("data/Media/Movies/intro.mp4", titleAttractTitle, 0);
}

void titleAttractReturn() {
    if (titleAttract.phase == ATTRACT_MOVIE_OUT || titleAttract.phase == ATTRACT_TITLE_IN)
        return;
    titleAttract.age = 0;
    titleAttract.startBlack = titleAttract.black;
    if (titleAttract.phase == ATTRACT_TITLE_OUT) {
        titleAttract.phase = ATTRACT_TITLE_IN;
        titleAttract.startGain = musicGain;
        return;
    }
    titleAttract.startVolume = introMovie.volume;
    titleAttract.phase = ATTRACT_MOVIE_OUT;
    setMusicGain(0);
    playMusic(titleAttractMenuMusic);
}

// Returns true while input belongs to the demo transition. The same key or
// click must never also activate Start, Quit, SFX or a control rebind beneath it.
bool titleAttractInput() {
    titleAttract.idle = 0;
    if (!titleAttractBusy())
        return false;
    titleAttractReturn();
    return true;
}

void titleAttractPointerActivity(float x, float y, bool click) {
    bool moved =
        titleAttract.pointerKnown && (x != titleAttract.pointerX || y != titleAttract.pointerY);
    titleAttract.pointerKnown = true;
    titleAttract.pointerX = x;
    titleAttract.pointerY = y;
    // Windows may send WM_MOUSEMOVE without physical motion after repaint,
    // focus or window changes. Repeated coordinates are not user activity.
    if (click || moved)
        titleAttract.idle = 0;
}

bool titleAttractMessage(const MSG &m) {
    if (!titleAttractBusy())
        return false;
    if ((m.message == WM_SYSKEYDOWN || m.message == WM_SYSKEYUP) && m.wParam == VK_F4)
        return false;
    if (m.message == WM_KEYDOWN || m.message == WM_SYSKEYDOWN) {
        titleAttractInput();
        return true;
    }
    if (m.message == WM_KEYUP || m.message == WM_SYSKEYUP) {
        if (m.wParam < 256)
            keys[m.wParam] = false;
        return true;
    }
    if (m.message == WM_LBUTTONDOWN || m.message == WM_RBUTTONDOWN || m.message == WM_MBUTTONDOWN) {
        titleAttractInput();
        return true;
    }
    return m.message == WM_LBUTTONUP || m.message == WM_RBUTTONUP || m.message == WM_MBUTTONUP ||
           m.message == WM_CHAR || m.message == WM_SYSCHAR;
}

void titleAttractAdvance(float dt, bool focused, bool heldInput) {
    dt = maxf(0, dt);
    // Title inactivity and its movie cycle continue in the background. Only
    // input held in the focused game counts as activity; stale capture/key
    // state must not block the demo after focus moves to another application.
    if (titleAttractBusy())
        introPause(false);
    if (titleAttract.phase == ATTRACT_MOVIE_IN && !introMovie.ready &&
        introMovie.status == INTRO_PLAYING) {
        titleAttract.age = 0;
        titleAttract.black = 1;
        titleAttractCurtain();
        return;
    }
    if (titleAttract.phase == ATTRACT_IDLE) {
        if (screen != TITLE || (focused && heldInput) || titleAttract.unavailable) {
            titleAttract.idle = 0;
            return;
        }
        titleAttract.idle += dt;
        if (titleAttract.idle < titleAttractIdleSeconds || !titleAttractMediaAvailable())
            return;
        titleAttract.phase = ATTRACT_TITLE_OUT;
        titleAttract.age = 0;
        titleAttract.black = 0;
        stopSounds();
    }
    // Inactivity uses wall time, but a delayed/background frame must not spend
    // an entire fade at once and jump straight from the menu into the movie.
    titleAttract.age += minf(dt, .05f);
    float t = clamp(titleAttract.age / titleAttractFadeSeconds, 0, 1);
    switch (titleAttract.phase) {
    case ATTRACT_TITLE_OUT:
        titleAttract.black = t;
        setMusicGain(1 - t);
        if (t >= 1) {
            titleAttractCurtain();
            stopMusic();
            if (titleAttractOpen()) {
                screen = INTRO;
                titleAttract.phase = ATTRACT_MOVIE_IN;
                titleAttract.age = 0;
            } else {
                introClose();
                titleAttract.unavailable = true;
                titleAttract.phase = ATTRACT_TITLE_IN;
                titleAttract.age = 0;
                titleAttract.startBlack = 1;
                titleAttract.startGain = 0;
                playMusic(titleAttractMenuMusic);
            }
        }
        break;
    case ATTRACT_MOVIE_IN:
        titleAttract.black = 1 - t;
        introVolume(musicEnabled ? t : 0);
        if (introMovie.status == INTRO_FINISHED || introMovie.status == INTRO_ERROR)
            titleAttractReturn();
        else if (t >= 1) {
            titleAttract.phase = ATTRACT_MOVIE;
            titleAttract.age = 0;
        }
        break;
    case ATTRACT_MOVIE:
        titleAttract.black = 0;
        if (introMovie.status == INTRO_FINISHED || introMovie.status == INTRO_ERROR)
            titleAttractReturn();
        break;
    case ATTRACT_MOVIE_OUT:
        titleAttract.black = titleAttract.startBlack + (1 - titleAttract.startBlack) * t;
        introVolume(musicEnabled ? titleAttract.startVolume * (1 - t) : 0);
        setMusicGain(t * .5f);
        if (t >= 1) {
            titleAttractCurtain();
            introClose();
            screen = TITLE;
            titleAttract.phase = ATTRACT_TITLE_IN;
            titleAttract.age = 0;
            titleAttract.startBlack = 1;
            titleAttract.startGain = .5f;
        }
        break;
    case ATTRACT_TITLE_IN:
        titleAttract.black = titleAttract.startBlack * (1 - t);
        setMusicGain(titleAttract.startGain + (1 - titleAttract.startGain) * t);
        if (t >= 1) {
            HWND curtain = titleAttract.curtain;
            bool unavailable = titleAttract.unavailable;
            titleAttract = TitleAttractState{};
            titleAttract.unavailable = unavailable;
            if (curtain)
                DestroyWindow(curtain);
        }
        break;
    default:
        break;
    }
    titleAttractCurtain();
}

void titleAttractRuntimeFrame(double elapsed, bool focused) {
    bool heldInput = pointerHeld;
    for (int i = 0; i < 256; i++)
        heldInput |= keys[i];
    // UI demo inactivity follows real elapsed time, once per outer frame.
    // Gameplay's catch-up clamp and fixed physics frequency must not slow it.
    titleAttractAdvance(elapsed > 0 ? (float)elapsed : 0, focused, heldInput);
}
