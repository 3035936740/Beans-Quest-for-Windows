// Isolated timing runner: no personal saves or audio device thread.
#define start unusedGameEntry
#include "../src/main.cpp"
#undef start
char waterProfile[16000];

int micros(LARGE_INTEGER a, LARGE_INTEGER b, LARGE_INTEGER f, int count) {
    return (int)((double)(b.QuadPart - a.QuadPart) * 1000000.0 / ((double)f.QuadPart * count));
}

extern "C" void WINAPI start() {
    runConstructors();
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(0, path, MAX_PATH);
    for (int i = lstrlenW(path) - 1; i >= 0; i--)
        if (path[i] == '\\') {
            path[i] = 0;
            break;
        }
    lstrcatW(path, L"/../bin");
    if (!SetCurrentDirectoryW(path))
        ExitProcess(2);
    permanent.init(160 * 1024 * 1024);
    levelArena.init(32 * 1024 * 1024);
    initWindow(true);
    loadProject();
    preloadEffects();
    typedef BOOL(WINAPI * SwapInterval)(int);
    SwapInterval interval = (SwapInterval)wglGetProcAddress("wglSwapIntervalEXT");
    if (interval)
        interval(0);
    CreateDirectoryA("test-output", 0);
    lstrcpyA(saveRoot, "test-output/water-profile-saves");
    CreateDirectoryA(saveRoot, 0);
    musicEnabled = soundEnabled = false;
    interfaceEnabled = false;
    debugMode = true;
    LARGE_INTEGER f, a, b;
    QueryPerformanceFrequency(&f);
    lstrcpyA(waterProfile,
             "world,level,entities,emitters,tick_us,full_render_us,without_emitters_us\r\n");
    for (int stage = 0; stage < stageCount; stage++) {
        startStage(stage);
        readyTimer = 0;
        int water = -1, emitters = 0;
        for (int i = 0; i < entityCount; i++) {
            if (entities[i].kind == WATER && water < 0)
                water = i;
            if (entities[i].source->base == 5)
                emitters++;
        }
        if (water < 0)
            continue;
        Rect pool = entityRect(entities[water]);
        float x = (pool.l + pool.r) * 0.5f, y = (pool.b + pool.t) * 0.5f;
        for (int i = 0; i < 60; i++) {
            State &p = entities[playerIndex].state;
            p.x = x;
            p.y = y;
            p.vx = p.vy = 0;
            tick(1 / 120.0f, false, false);
        }
        deathTimer = 0;
        complete = false;
        screen = PLAY;
        QueryPerformanceCounter(&a);
        for (int i = 0; i < 240; i++) {
            State &p = entities[playerIndex].state;
            p.x = x;
            p.y = y;
            p.vx = p.vy = 0;
            deathTimer = 0;
            tick(1 / 120.0f, false, false);
        }
        QueryPerformanceCounter(&b);
        int tickUs = micros(a, b, f, 240);
        camX = clamp(x - 256, 0, maxf(0, current.size.x - 512));
        camY = clamp(y - 192, 0, maxf(0, current.size.y - 384));
        render();
        glFinish();
        QueryPerformanceCounter(&a);
        for (int i = 0; i < 8; i++) {
            render();
            glFinish();
        }
        QueryPerformanceCounter(&b);
        int renderUs = micros(a, b, f, 8);
        char image[120];
        wsprintfA(image, "test-output/water-profile-%d-%d.bmp",
                  (int)current.props.num("bean-world"), (int)current.props.num("bean-level"));
        capture(image);
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->base == 5)
                entities[i].state.gone = 1;
        QueryPerformanceCounter(&a);
        for (int i = 0; i < 8; i++) {
            render();
            glFinish();
        }
        QueryPerformanceCounter(&b);
        int noEmitters = micros(a, b, f, 8);
        char row[180];
        wsprintfA(row, "%d,%d,%d,%d,%d,%d,%d\r\n", (int)current.props.num("bean-world"),
                  (int)current.props.num("bean-level"), entityCount, emitters, tickUs, renderUs,
                  noEmitters);
        lstrcatA(waterProfile, row);
    }
    atomicWrite("test-output/water-performance.csv", waterProfile, lstrlenA(waterProfile));
    ExitProcess(0);
}
