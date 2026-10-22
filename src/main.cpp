// Win32 entry point, event routing and fixed-step game loop. Test dispatch is compiled out of release builds.
#include "interface.h"
extern "C" void runConstructors();

#if BQ_ENABLE_TESTS
// Capture gameplay context on the caller thread; the mixer only updates
// already-created per-voice counters and never reads world state.
void runtimeJumpAudioContext(AudioJumpTrace &trace) {
    trace.stageId =
        currentStage >= 0 && currentStage < stageCount ? levels[stageList[currentStage]].id : 0;
    trace.scene = (int)screen;
    trace.jumps = jumps;
    trace.contextFlags = (debugMode ? 1 : 0) | (endingScene.active ? 2 : 0) |
                         (playerWalkMode ? 4 : 0) | (readyTimer > 0 ? 8 : 0);
    if (screen != PLAY && screen != PAUSE && screen != COMPLETE && page.hasBean) {
        trace.stageId = page.level.id;
        trace.x100 = (int)(page.bean.state.x * 100);
        trace.y100 = (int)(page.bean.state.y * 100);
        trace.contextFlags |= 16;
    } else if (playerIndex >= 0 && playerIndex < entityCount) {
        trace.x100 = (int)(entities[playerIndex].state.x * 100);
        trace.y100 = (int)(entities[playerIndex].state.y * 100);
    }
}

#endif

HWND window;
HGLRC context;
RECT savedWindow;
char *commandLine;

void resize(int w, int h) {
    float oldW = viewW, oldH = viewH;
    resizeNativeViewport(w, h);
    viewportSceneResize(oldW, oldH);
}

void toggleFullscreen() {
    fullscreen = !fullscreen;
    if (fullscreen) {
        GetWindowRect(window, &savedWindow);
        SetWindowLongA(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        HMONITOR m = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoA(m, &mi);
        SetWindowPos(window, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_FRAMECHANGED);
    } else {
        SetWindowLongA(window, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(window, 0, savedWindow.left, savedWindow.top,
                     savedWindow.right - savedWindow.left, savedWindow.bottom - savedWindow.top,
                     SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

void resizeWindow(int w, int h) {
    RECT rect = {0, 0, w, h};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(window, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER);
}

// Translate native window events into logical viewport input and game lifecycle events.
LRESULT CALLBACK procedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case MM_MCINOTIFY:
        introNotify(wp, lp);
        return 0;
    case WM_CLOSE:
        introClose();
        running = false;
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        resize(LOWORD(lp), HIWORD(lp));
        introResize();
        return 0;
    case WM_KILLFOCUS:
        memset(keys, 0, sizeof(keys));
        if (screen == PLAY)
            openPause(false);
        return 0;
    case WM_KEYDOWN:
        if (wp < 256) {
            keys[wp] = true;
            if (!(lp & (1L << 30)))
                keyPressed((int)wp);
            else
                makerKeyRepeat((int)wp);
        }
        return 0;
    case WM_CHAR:
        makerChar((int)wp);
        return 0;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP: {
        // Only the course editor uses the right and middle buttons.
        if (!makerScreen())
            return 0;
        Vec pointer = nativePointerPosition((float)(short)LOWORD(lp), (float)(short)HIWORD(lp));
        int button = msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ? 1 : 2;
        if (msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN) {
            SetCapture(hwnd);
            makerPointerDown(pointer.x, pointer.y, button);
        } else {
            makerPointerUp(pointer.x, pointer.y, button);
            if (!(wp & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
                ReleaseCapture();
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP: {
        Vec pointer = nativePointerPosition((float)(short)LOWORD(lp), (float)(short)HIWORD(lp));
        float x = pointer.x, y = pointer.y;
        if (msg == WM_LBUTTONDOWN) {
            SetCapture(hwnd);
            pointerDown(x, y);
        } else if (msg == WM_LBUTTONUP) {
            pointerUp(x, y);
            ReleaseCapture();
        } else
            mouseInterface(x, y, false);
        return 0;
    }
    case WM_CAPTURECHANGED:
        pointerHeld = readyPointerDown = false;
        if ((HWND)lp != hwnd)
            makerCaptureLost();
        return 0;
    case WM_MOUSEWHEEL:
        if (makerWheel((short)HIWORD(wp)))
            return 0;
        if (screen == WORLD)
            moveFocus((short)HIWORD(wp) > 0 ? "focus-left" : "focus-right");
        return 0;
    case WM_KEYUP:
        if (wp < 256)
            keys[wp] = false;
        return 0;
    case WM_SYSKEYDOWN:
        if (wp == VK_RETURN) {
            if (!(lp & (1L << 30)))
                toggleFullscreen();
            return 0;
        }
        // Alt is the course editor's eyedropper modifier; keep it from opening the window menu.
        if (makerScreen() && (wp == VK_MENU || wp == VK_F10))
            return 0;
        break;
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
        if (makerScreen() && (msg == WM_SYSCHAR || wp == VK_MENU || wp == VK_F10))
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// Create the game window and compatible OpenGL context before any resource uploads.
void initWindow(bool hidden) {
    HINSTANCE inst = GetModuleHandleA(0);
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = procedure;
    wc.hInstance = inst;
    wc.lpszClassName = "BeansQuestNative";
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = LoadIconA(inst, MAKEINTRESOURCEA(101));
    RegisterClassA(&wc);
    RECT work = {0, 0, 1280, 720};
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0);
    RECT frame = {};
    AdjustWindowRect(&frame, WS_OVERLAPPEDWINDOW, FALSE);
    resolutionChoice = initialWindowResolution(work.right - work.left, work.bottom - work.top,
                                               frame.right - frame.left, frame.bottom - frame.top);
    RECT rect = {0, 0, windowResolutions[resolutionChoice].width,
                 windowResolutions[resolutionChoice].height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowA(wc.lpszClassName, "Bean's Quest - Windows", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
                           rect.bottom - rect.top, 0, 0, inst, 0);
    if (!window)
        fatal("Could not create game window.");
    gameDC = GetDC(window);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(gameDC, &pfd);
    if (!pf || !SetPixelFormat(gameDC, pf, &pfd))
        fatal("Cannot initialize Windows OpenGL.");
    context = wglCreateContext(gameDC);
    if (!context || !wglMakeCurrent(gameDC, context))
        fatal("Cannot create OpenGL 1.1 context.");
    typedef BOOL(WINAPI * SwapInterval)(int);
    SwapInterval interval = (SwapInterval)wglGetProcAddress("wglSwapIntervalEXT");
    if (interval)
        interval(1);
    RECT client;
    GetClientRect(window, &client);
    resize(client.right, client.bottom);
    initFont();
    if (!hidden) {
        ShowWindow(window, SW_SHOW);
        UpdateWindow(window);
    }
}
#if BQ_ENABLE_TESTS
void capture(const char *file) {
    glReadBuffer(GL_FRONT);
    u32 size = viewportW * viewportH * 4;
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    if (!pixels)
        return;
    glReadPixels(viewportX, viewportY, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    for (u32 i = 0; i < size; i += 4) {
        u8 t = pixels[i];
        pixels[i] = pixels[i + 2];
        pixels[i + 2] = t;
    }
    BITMAPFILEHEADER fh = {};
    BITMAPINFOHEADER bi = {};
    fh.bfType = 0x4d42;
    fh.bfSize = sizeof(fh) + sizeof(bi) + size;
    fh.bfOffBits = sizeof(fh) + sizeof(bi);
    bi.biSize = sizeof(bi);
    bi.biWidth = viewportW;
    bi.biHeight = viewportH;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = size;
    HANDLE f = CreateFileA(file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD done;
        WriteFile(f, &fh, sizeof(fh), &done, 0);
        WriteFile(f, &bi, sizeof(bi), &done, 0);
        WriteFile(f, pixels, size, &done, 0);
        CloseHandle(f);
    }
    HeapFree(GetProcessHeap(), 0, pixels);
}

char testLog[1048576];
int failures;

void check(bool ok, const char *label) {
    lstrcatA(testLog, ok ? "PASS " : "FAIL ");
    lstrcatA(testLog, label);
    lstrcatA(testLog, "\r\n");
    if (!ok)
        failures++;
    atomicWrite("test-output/results.txt", testLog, lstrlenA(testLog));
}

#include "checks.h"
#include "checks_av.h"
#include "checks_native.h"
#include "checks_camera_restore_native.h"
#include "checks_helper_camera_restore_native.h"
#include "checks_diagonal.h"
#include "checks_cloud.h"
#include "checks_crumbling_cloud_native.h"
#include "checks_cloud57_jump_native.h"
#include "checks_visual_followup.h"
#include "checks_hidden_physics.h"
#include "checks_jump_audio_followup.h"
#include "checks_finale_route_audio.h"
#include "checks_rescue_jump_audio_native.h"
#include "checks_audio_jump_trace.h"
#include "checks_tom_jump_diagnostics.h"
#include "checks_audio_silence_keepalive.h"
#include "checks_pinball_native.h"
#include "checks_pinball_phase_native.h"
#include "checks_spring_phase_native.h"
#include "checks_spring_phase_placements.h"
#include "checks_campaign_trampoline_native.h"
#include "checks_campaign_trampoline_placements.h"
#include "checks_pause_transition_native.h"
#include "checks_pause_interface_native.h"
#include "checks_touch_alternator_native.h"
#include "checks_touch_cooldown_load_native.h"
#include "checks_secret_reference_native.h"
#include "checks_background_native.h"
#include "checks_background_extension.h"
#include "checks_background_responsive.h"
#include "checks_background_regions.h"
#include "checks_opening.h"
#include "checks_wizard_pose.h"
#include "checks_wizard_motion.h"
#include "checks_bat_smoke_native.h"
#include "checks_wizard_dismiss_native.h"
#include "checks_enemy_edge_native.h"
#include "checks_walk_permission_native.h"
#include "checks_walk_callback_native.h"
#include "checks_wizard_walk_spawn_native.h"
#include "checks_enemy_edge_birth_native.h"
#include "checks_walker_gravity_native.h"
#include "checks_jumper_gravity_native.h"
#include "checks_rotor_gravity_native.h"
#include "checks_deadly_gravity_native.h"
#include "checks_cloud_spring_native.h"
#include "checks_fish_cloud_native.h"
#include "checks_fixture_filter_native.h"
#include "checks_attachment_layer_native.h"
#include "checks_orbit_attachment_birth_native.h"
#include "checks_helper_gem_native.h"
#include "checks_wizard_followup.h"
#include "checks_wizard_transition_native.h"
#include "checks_wizard_final_camera.h"
#include "checks_wizard_gate_audio.h"
#include "checks_pendulum_native.h"
#include "checks_enemy_speed_native.h"
#include "checks_bullet_source_animation_native.h"
#include "checks_wizard_summon_waves.h"
#include "checks_wizard_retry_rng.h"
#include "checks_wizard_presentation.h"
#include "checks_respawn_transition.h"
#include "checks_respawn_controls.h"
#include "checks_spider_animation.h"
#include "checks_motion.h"
#include "checks_motion_native.h"
#include "checks_magnet.h"
#include "checks_magnet_checkpoint.h"
#include "checks_wizard_arena.h"
#include "checks_source_native.h"
#include "checks_source_track_native.h"
#include "checks_projectile_layers.h"
#include "checks_stomp_native.h"
#include "checks_goal_launch.h"
#include "checks_stage_clear_math.h"
#include "checks_stage_clear_ui.h"
#include "checks_ready_native.h"
#include "checks_ending_transformation.h"
#include "checks_ending_credits_native.h"
#include "checks_ending_credits_presentation.h"
#include "checks_ui_mask_coverage.h"
#include "checks_credits_runtime_native.h"
#include "checks_viewport_native.h"
#include "checks_deadly_native.h"
#include "checks_rotation_native.h"
#include "checks_spike_group_native.h"
#include "checks_spike_placement_native.h"
#include "checks_force_area_native.h"
#include "checks_selector_spike_placement.h"
#include "checks_spike_eye_animation.h"
#include "checks_spike_alternation.h"
#include "checks_wall_native.h"
#include "checks_tom_contact.h"
#include "checks_collectable_native.h"
#include "checks_eye_native.h"
#include "checks_oneway_geometry.h"
#include "checks_oneway_native.h"
#include "checks_cloud_seam_native.h"
#include "checks_cloud_transfer.h"
#include "checks_checkpoint_native.h"
#include "checks_checkpoint_save_phase_native.h"
#include "checks_projectile_checkpoint_native.h"
#include "checks_projectile_pool_native.h"
#include "checks_campaign_projectile_capacity_native.h"
#include "checks_campaign_projectile_animation_native.h"
#include "checks_projectile_overlap_native.h"
#include "checks_boss_projectile_instances_native.h"
#include "checks_secret_layer_native.h"
#include "checks_enemy_animation_boundaries.h"
#include "checks_jump_awards_native.h"
#include "checks_selector_locked_awards.h"
#include "checks_motion_timestep_native.h"
#include "checks_source_interval_native.h"
#include "checks_paused_actor_animation_native.h"
#include "checks_walk_start_native.h"
#include "checks_killable_death_native.h"
#include "checks_killable_contact_native.h"
#include "checks_shot_death_native.h"
#include "checks_circle_fixture_native.h"
#include "checks_projectile_ccd_native.h"
#include "checks_water_membership_native.h"
#include "checks_selector_particles.h"
#include "checks_selector_water_surface.h"
#include "checks_selector_trampoline_native.h"
#include "checks_ui_jump_detail_native.h"
#include "checks_selector_deferred_native.h"
#include "checks_ordinary_phase_audit_native.h"
#include "checks_skeletonkey_native.h"
#include "checks_resize_checkpoint_position_native.h"
#include "checks_tom_force_native.h"
#include "checks_continuous_particles.h"
#include "checks_continuous_restore_native.h"
#include "checks_activation_native.h"
#include "checks_contacts.h"
#include "checks_intro.h"
#include "checks_wizard.h"
#include "checks_selector_motion.h"
#include "checks_enemy.h"
#include "checks_animation.h"
#include "checks_frog.h"
#include "checks_frog_interrupt_native.h"
#include "checks_jumper_ground_native.h"
#include "checks_jumper_water_pose.h"
#include "checks_jumper_animation_native.h"
#include "checks_fish_animation_native.h"
#include "checks_credits_projectile_native.h"
#include "checks_credits_shot_animation_native.h"
#include "checks_particle_burst_native.h"
#include "checks_burst_lifecycle_native.h"
#include "checks_particle_distribution_native.h"
#include "checks_frog_spin_native.h"
#include "checks_frog_solved_order_native.h"
#include "checks_credits_jumper_ground_native.h"
#include "checks_magnet_solved_native.h"
#include "checks_wizard_solved_native.h"
#include "checks_wizard_death_audio_native.h"
#include "checks_helper_checkpoint_return_native.h"
#include "checks_helper_checkpoint_disk_native.h"
#include "checks_helper_collision_scope_native.h"
#include "checks_helper_oneway_traversal_native.h"
#include "checks_helper_collision_ui_native.h"
#include "checks_helper_projectile_scope_native.h"
#include "checks_particle_bounds_native.h"
#include "checks_world_card_texture.h"
#include "checks_selector_preview_tint.h"
#include "checks_selector_preview_geometry_repair.h"
#include "checks_helper.h"
#include "checks_helper_animation_native.h"
#include "checks_helper_perch_native.h"
#include "checks_helper_solved_native.h"
#include "checks_helper_affector_native.h"
#include "checks_helper_pickup_presentation_native.h"
#include "checks_ending_object_mute_native.h"
#include "checks_breakable_solved_native.h"
#include "checks_mechanism_event_checkpoint_native.h"
#include "checks_alternator_body_native.h"
#include "checks_water.h"
#include "checks_water_sound_native.h"
#include "checks_water_save_native.h"
#include "checks_selector_physics.h"
#include "checks_jelly_sound_native.h"
#include "checks_title_attract.h"
#include "checks_title_idle_native.h"
#include "checks_wizard_music.h"
#include "checks_world_select_native.h"
#include "checks_maker_world_card.h"
#include "checks_maker_editor.h"
#include "checks_maker_weather.h"
#include "checks_maker_water_controls.h"
#include "checks_switch_visual.h"
#include "checks_box_push_native.h"
#include "checks_selector_slope_native.h"
#include "checks_inherited_box_native.h"
#include "checks_level_end_native.h"
#include "checks_pinball_fixture_native.h"
#include "checks_pinball_dynamic_native.h"
#include "checks_pinball_power_native.h"
#include "checks_single_collectable_native.h"
#include "checks_single_collectable_lifecycle_native.h"
#include "checks_group_collectable_lifecycle_native.h"
#include "checks_coin_sparkle_native.h"
#include "checks_stage53_source_native.h"
#include "checks_stage53_whole_native.h"
#include "checks_widescreen_resolution.h"
#include "checks_creature_support_native.h"
#include "checks_potion_visual.h"
#include "checks_selector_preview_audio.h"

// Run isolated regression coverage with private saves; this entry point is absent from release builds.
int selfTest() {
    lstrcpyA(saveRoot, "test-output/saves");
    CreateDirectoryA("test-output", 0);
    CreateDirectoryA(saveRoot, 0);
    memset(&progress, 0, sizeof(progress));
    musicEnabled = soundEnabled = false;
    progress.unlocked = stageCount - 1;
    initInterface();
    interfaceEnabled = false;
    check(levelCount == 74 && stageCount == 50, "74 original scenes and 50 game stages indexed");
    for (int i = 0; i < stageCount; i++) {
        startStage(i);
        int n = 0;
        for (int j = 0; j < entityCount; j++)
            if (entities[j].kind == COIN)
                n++;
        char label[180];
        wsprintfA(label, "Stage %d-%02d: parse, player, %d diamonds (expected %d)",
                  (int)current.props.num("bean-world"), (int)current.props.num("bean-level"), n,
                  (int)current.props.num("bean-max-coins"));
        check(playerIndex >= 0 && n == (int)current.props.num("bean-max-coins"), label);
        for (int step = 0; step < 240; step++)
            tick(1 / 120.0f, false, false);
        State &s = entities[playerIndex].state;
        check(s.x == s.x && s.y == s.y, "Fixed-step simulation remains finite");
        if (i == 0 || i == 8 || i == 17 || i == 27 || i == 37 || i == 49) {
            render();
            char file[100];
            wsprintfA(file, "test-output/world-%d.bmp", (int)current.props.num("bean-world"));
            capture(file);
        }
    }
    startStage(0);
    for (int i = 0; i < 360; i++)
        tick(1 / 120.0f, false, false);
    check(jumps == 0 && entities[playerIndex].state.vy != 0 && deaths == 0,
          "Automatic stationary bounce keeps original floor collision without counting jumps");
    float startX = entities[playerIndex].state.x;
    for (int i = 0; i < 120; i++)
        tick(1 / 120.0f, false, true);
    check(entities[playerIndex].state.x > startX + 100, "Right movement and acceleration");
    screen = PLAY;
    keyPressed(VK_ESCAPE);
    check(screen == PAUSE, "Escape opens menu");
    float pausedX = entities[playerIndex].state.x;
    tick(1, false, true);
    check(entities[playerIndex].state.x == pausedX, "Pause freezes physics");
    keyPressed(VK_ESCAPE);
    bool resumeDeferred = screen == PAUSE && pauseInterfaceNative.resuming;
    tickInterface(pauseTransitionNativeSeconds);
    bool resumeTerminal = screen == PAUSE;
    tickInterface(0);
    check(resumeDeferred && resumeTerminal && screen == PLAY,
          "Escape resumes only after the native exit and following terminal update");
    coins = 7;
    jumps = 9;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == COIN) {
            entities[i].state.gone = 1;
            break;
        }
    captureCheckpoint(true);
    float savedX = entities[playerIndex].state.x;
    entities[playerIndex].state.x += 50;
    coins = 0;
    keyPressed('S');
    for (int step = 0; step < 44; step++)
        tick(1 / 120.0f, false, false);
    check(coins == 7 && entities[playerIndex].state.x == savedX,
          "S restores checkpoint and collected state at the full-black boundary");
    for (int step = 0; step < 44; step++)
        tick(1 / 120.0f, false, false);
    char path[MAX_PATH];
    savePath(path, "checkpoint.dat");
    Snapshot *disk = (Snapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(Snapshot));
    bool ok = exactRead(path, disk, sizeof(Snapshot)) && validSnapshot(*disk);
    check(ok, "Atomic checkpoint file validates after reload");
    disk->coins++;
    check(!validSnapshot(*disk), "Corrupt checkpoint rejected by checksum");
    HeapFree(GetProcessHeap(), 0, disk);
    keyPressed('R');
    for (int step = 0; step < 90; step++)
        tick(1 / 120.0f, false, false);
    check(coins == 0 && jumps == 0 && entities[playerIndex].state.x == 256,
          "R resets stage, collectibles and checkpoint");
    startStage(0);
    for (int i = 0; i < 120; i++)
        tick(1 / 120.0f, false, false);
    render();
    capture("test-output/stage-1.bmp");
    navigate(12192614360109026946ULL);
    page.age = 2;
    transitionTime = 0;
    render();
    capture("test-output/title.bmp");
    int oldTitleFocus = page.focus;
    keyPressed(VK_UP);
    check(screen == TITLE && page.focus != oldTitleFocus, "Up navigates the original title menu");
    navigate(12192614360109026946ULL);
    keyPressed(VK_SPACE);
    check(screen == WORLD, "Space enters original world selection");
    page.age = 2;
    transitionTime = 0;
    render();
    capture("test-output/world-select.bmp");
    confirm();
    check(screen == SELECT, "World button opens its original stage scene");
    page.age = 2;
    transitionTime = 0;
    render();
    capture("test-output/select.bmp");
    startStage(0);
    openPause();
    tickInterface(pauseTransitionNativeSeconds);
    render();
    capture("test-output/menu.bmp");
    startStage(0);
    int lamp = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == CHECKPOINT) {
            lamp = i;
            break;
        }
    if (lamp >= 0) {
        Entity &p = entities[playerIndex];
        p.state.x = entities[lamp].state.x;
        p.state.y = entities[lamp].state.y;
        tick(1 / 120.0f, false, false);
        tick(1 / 120.0f, false, false);
        float savedX = checkpoint.state[playerIndex].x;
        check(entities[lamp].state.hp == 2 && checkpoint.state[lamp].hp == 2 &&
                  absf(savedX - entities[lamp].state.x) < 1,
              "Touching lamppost persists a checkpoint at the next unpaused task boundary");
        p.state.x += 100;
        die();
        for (int i = 0; i < 330; i++)
            tick(1 / 120.0f, false, false);
        check(absf(p.state.x - savedX) < 1 && absf(savedX - entities[lamp].state.x) < 1,
              "Death restores the actual last lamppost position");
    }
    screen = PLAY;
    int exit = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == EXIT) {
            exit = i;
            break;
        }
    if (exit >= 0) {
        interfaceEnabled = true;
        entities[playerIndex].state.x = entities[exit].state.x;
        entities[playerIndex].state.y = hitRect(entities[exit]).t - entities[playerIndex].hit.b + 2;
        entities[playerIndex].state.vy = -60;
        for (int step = 0; step < 260; step++)
            tick(1 / 120.0f, false, false);
        check(screen == COMPLETE && progress.cleared[0],
              "Goal spring completes stage and saves progress");
        transitionTime = 0;
        render();
        capture("test-output/complete.bmp");
        confirm();
        check(currentStage == 1 && screen == PLAY, "Confirm enters next original stage");
    }
    stageInterface(false);
    readyOpening.age = 0.6f;
    readyTimer = 1.9f;
    transitionTime = 0;
    render();
    capture("test-output/ready.bmp");
    keyPressed(VK_UP);
    check(readyTimer > 0, "Up does not bypass the original Ready input contract");
    readyTimer = 0;
    progress.unlocked = 0;
    navigate(worldScenes[0]);
    int enabled = 0, disabled = 0;
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].stage >= 0) {
            if (page.nodes[i].disabled)
                disabled++;
            else
                enabled++;
        }
    check(enabled == 1 && disabled == 7, "Original stage buttons respect saved unlocks");
    transitionTime = 0;
    render();
    capture("test-output/locked-select.bmp");
    progress.unlocked = stageCount - 1;
    for (int w = 0; w < 5; w++) {
        navigate(worldScenes[w]);
        int count = 0;
        for (int i = 0; i < page.level.objectCount; i++)
            if (page.nodes[i].stage >= 0)
                count++;
        char label[100];
        wsprintfA(label, "Original world %d stage screen contains its own buttons", w + 1);
        const int expected[] = {8, 9, 10, 10, 12};
        check(count == expected[w], label);
        transitionTime = 0;
        page.age = 2;
        render();
        char p[100];
        wsprintfA(p, "test-output/select-world-%d.bmp", w + 1);
        capture(p);
    }
    navigate(610822198339107ULL);
    transitionTime = 0;
    page.age = 2;
    render();
    capture("test-output/options.bmp");
    check(screen == OPTIONS, "Original desktop options scene");
    focusId(page, 636966097302919ULL);
    confirm();
    check(screen == CONTROLS, "Original Controls button opens key bindings");
    transitionTime = 0;
    render();
    capture("test-output/controls.bmp");
    focusId(page, 7212130636121915ULL);
    confirm();
    keyPressed('Q');
    check(primaryKey[0] == 'Q' && rebindAction < 0,
          "Original control buttons bind and persist keys");
    defaultBindings();
    writeBindings();
    navigate(1991411864919998ULL);
    transitionTime = 0;
    render();
    capture("test-output/graphics.bmp");
    check(screen == GRAPHICS, "Original graphics options scene");
    navigate(8466014814437108852ULL);
    transitionTime = 0;
    render();
    capture("test-output/credits.bmp");
    check(screen == CREDITS, "Original credits scene and text");
    fidelityChecks();
    audiovisualChecks();
    nativeChecks();
    cameraRestoreNativeChecks();
    helperCameraRestoreNativeChecks();
    resizeCheckpointPositionNativeChecks();
    diagonalChecks();
    cloudVisualChecks();
    crumblingCloudNativeChecks();
    cloud57JumpNativeChecks();
    selectorMotionChecks();
    enemyChecks();
    animationChecks();
    frogChecks();
    frogSpinNativeChecks();
    frogSolvedOrderNativeChecks();
    creditsJumperGroundNativeChecks();
    magnetSolvedNativeChecks();
    wizardSolvedNativeChecks();
    wizardDeathAudioNativeChecks();
    helperCheckpointReturnNativeChecks();
    helperCheckpointDiskNativeChecks();
    frogInterruptNativeChecks();
    jumperGroundNativeChecks();
    jumperWaterPoseChecks();
    worldCardTextureChecks();
    selectorPreviewTintChecks();
    selectorPreviewGeometryRepairChecks();
    helperChecks();
    helperAnimationNativeChecks();
    helperPerchNativeChecks();
    helperSolvedNativeChecks();
    helperAffectorNativeChecks();
    helperPickupPresentationNativeChecks();
    endingObjectMuteNativeChecks();
    helperCollisionScopeInventoryNativeChecks();
    helperCollisionScopeMatrixNativeChecks();
    helperCollisionScopeMatrixNativeChecks(0);
    helperCollisionScopeMatrixNativeChecks(2);
    helperOneWayTraversalNativeChecks();
    helperCollisionScopeUiNativeChecks();
    helperCollisionScopeUiNativeChecks(0);
    helperCollisionScopeUiNativeChecks(2);
    helperProjectileScopeNativeChecks();
    helperProjectileUiScopeNativeChecks();
    breakableSolvedNativeChecks();
    alternatorBodyNativeChecks();
    waterChecks();
    waterSoundNativeChecks();
    selectorPhysicsChecks();
    visualFollowupChecks();
    motionChecks();
    nativeMotionChecks();
    contactFollowupChecks();
    wizardFollowupChecks();
    introChecks();
    titleAttractChecks();
    titleIdleRuntimeChecks();
    wizardMusicChecks();
    worldSelectNativeChecks();
    selectorSlopeNativeChecks();
    boxPushNativeChecks();
    completionGoldShoeChecks();
    wizardHudNativeChecks();
    sourceNativeChecks();
    sourceTrackNativeChecks();
    projectileLayerChecks();
    magnetCheckpointContactChecks();
    springIncomingContactChecks();
    selectorMechanismContactChecks();
    stormySpringContactChecks();
    wizardArenaChecks();
    stompNativeChecks();
    goalLaunchChecks();
    stageClearMathChecks();
    stageClearUiChecks();
    endingTransformationChecks();
    endingCreditsNativeChecks();
    endingCreditsPresentationChecks();
    creditsRuntimeNativeChecks();
    viewportNativeChecks();
    deadlyNativeChecks();
    rotationNativeChecks();
    spikeGroupNativeChecks();
    spikePlacementNativeChecks();
    forceAreaNativeChecks();
    physicsCheckpointForceNativeChecks();
    selectorSpikePlacementChecks();
    hiddenPhysicsNativeChecks();
    jumpAudioFollowupChecks();
    finaleRouteAudioChecks();
    backgroundNativeChecks();
    backgroundExtensionChecks();
    openingChecks();
    wizardPoseChecks();
    wizardMotionChecks();
    batSmokeNativeChecks();
    wizardDismissNativeChecks();
    attachmentLayerNativeChecks();
    orbitAttachmentBirthNativeChecks();
    helperGemNativeChecks();
    wizardNativeFollowupChecks();
    wizardTransitionNativeChecks();
    wizardFinalCameraChecks();
    wizardGateAudioChecks();
    pendulumNativeChecks();
    enemySpeedNativeChecks();
    bulletSourceAnimationNativeChecks();
    readyNativeChecks();
    uiMaskCoverageChecks();
    wizardSummonWaveNativeChecks();
    wizardRetryRngChecks();
    wizardPresentationChecks();
    respawnTransitionChecks();
    respawnControlChecks();
    magnetSeamChecks();
    selectorMagnetChecks();
    spikeEyeAnimationChecks();
    spikeAlternationChecks();
    wallNativeChecks();
    tomContactChecks();
    ordinaryPhaseAuditNativeChecks();
    skeletonKeyNativeChecks();
    walkPermissionNativeChecks();
    walkCallbackNativeChecks();
    wizardWalkSpawnNativeChecks();
    enemyEdgeBirthNativeChecks(true);
    cloudSeamNativeChecks();
    cloudSupportFaceChecks();
    cloudSupportHandoffChecks();
    cloudSupportMatrixChecks();
    cloudEndpointChecks();
    creatureSupportNativeChecks();
    fishCloudNativeChecks();
    fixtureFilterNativeChecks();
    circleFixtureNativeChecks();
    projectileCcdNativeChecks();
    waterMembershipNativeChecks();
    waterSaveNativeChecks();
    inheritedBoxNativeChecks();
    levelEndNativeChecks();
    pinballFixtureNativeChecks();
    pinballDynamicNativeChecks();
    pinballPowerNativeChecks();
    singleCollectableNativeChecks();
    singleCollectableLifecycleNativeChecks();
    groupCollectableLifecycleNativeChecks();
    coinSparkleNativeChecks();
    stage53SourceNativeChecks();
    stage53WholeNativeChecks();
    widescreenResolutionChecks();
    enemyEdgeNativeChecks();
    cloudSpringNativeChecks();
    walkerGravityNativeChecks();
    jumperGravityNativeChecks();
    rotorGravityNativeChecks();
    deadlyGravityNativeChecks();
    collectableNativeChecks();
    eyeNativeChecks();
    oneWayGeometryChecks();
    oneWayNativeChecks();
    checkpointNativeChecks();
    checkpointSavePhaseNativeChecks();
    projectileCheckpointNativeChecks();
    projectilePoolNativeChecks();
    campaignProjectileCapacityNativeChecks();
    campaignProjectileAnimationNativeChecks();
    projectileOverlapNativeChecks();
    bossProjectileInstancesNativeChecks();
    secretLayerNativeChecks();
    enemyAnimationBoundaryNativeChecks();
    jumpAwardsNativeChecks();
    selectorLockedAwardsChecks();
    motionTimestepNativeChecks();
    sourceIntervalNativeChecks();
    pausedActorAnimationNativeChecks();
    walkStartNativeChecks();
    killableContactNativeChecks();
    killableDeathNativeChecks();
    shotDeathNativeChecks();
    jumperAnimationNativeChecks();
    fishAnimationNativeChecks();
    creditsProjectileNativeChecks();
    creditsShotAnimationNativeChecks();
    particleBurstNativeChecks();
    burstLifecycleNativeChecks();
    particleDistributionNativeChecks();
    particleBoundsNativeChecks();
    selectorParticleBoundsChecks();
    selectorWaterSurfaceChecks();
    selectorTrampolineNativeChecks();
    uiJumpDetailNativeChecks();
    selectorDeferredNativeChecks();
    jellySoundNativeChecks();
    tomForceNativeChecks();
    continuousParticleNativeChecks();
    continuousRestoreNativeChecks();
    activationNativeChecks();
    proximityPhaseNativeChecks();
    spiderAnimationChecks();
    rescueJumpAudioNativeChecks();
    audioJumpTraceChecks();
    tomJumpDiagnosticChecks();
    audioSilenceKeepAliveChecks();
    pinballNativeChecks();
    pinballPhaseNativeChecks();
    campaignTrampolineNativeChecks();
    campaignTrampolinePlacementChecks();
    springPhaseNativeChecks();
    springPhasePlacementChecks();
    pauseTransitionNativeChecks();
    touchAlternatorNativeChecks();
    touchCooldownLoadNativeChecks();
    mechanismEventCheckpointNativeChecks();
    secretReferenceNativeChecks();
    pauseInterfaceNativeChecks();
    check(glGetError() == GL_NO_ERROR, "OpenGL rendering and captures");
    char final[80];
    wsprintfA(final, "Failures: %d\r\n", failures);
    lstrcatA(testLog, final);
    atomicWrite("test-output/results.txt", testLog, lstrlenA(testLog));
    return failures ? 1 : 0;
}
#endif

extern "C" void WINAPI start() {
    runConstructors();
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(0, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        fatal("Game path is too long.");
    for (int i = (int)n - 1; i >= 0; i--)
        if (path[i] == '\\' || path[i] == '/') {
            path[i] = 0;
            break;
        }
    SetCurrentDirectoryW(path);
    commandLine = GetCommandLineA();
#if BQ_ENABLE_TESTS
    bool testing = contains(commandLine, "--self-test"),
         audioTesting = contains(commandLine, "--audio-test");
#else
    const bool testing = false, audioTesting = false;
#endif
    permanent.init(160 * 1024 * 1024);
    levelArena.init(32 * 1024 * 1024);
    initWindow(testing || audioTesting);
#if BQ_ENABLE_TESTS
    if (testing)
        resizeWindow(1024, 768);
#endif
    loadProject();
    preloadEffects();
#if BQ_ENABLE_TESTS
    audioJumpTraceContext = runtimeJumpAudioContext;
    if (audioTesting)
        ExitProcess(audioDeviceSmoke());
    if (testing) {
        lstrcpyA(saveRoot, "test-output/saves");
        CreateDirectoryA("test-output", 0);
        CreateDirectoryA(saveRoot, 0);
        initAudio(soundFiles, false);
        if (contains(commandLine, "--self-test-maker-water-controls")) {
            initInterface();
            makerGridChecks();
            makerControlChecks();
            makerWaterRenderChecks();
            makerPotionFormChecks();
            singleCollectableLifecycleNativeChecks();
            atomicWrite("test-output/maker-water-controls-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-maker-weather")) {
            initInterface();
            makerWeatherChecks();
            potionVisualChecks();
            switchVisualChecks();
            continuousParticleNativeChecks();
            particleBoundsNativeChecks();
            atomicWrite("test-output/maker-weather-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-maker-editor")) {
            initInterface();
            makerEditorChecks();
            coinSparkleNativeChecks();
            atomicWrite("test-output/maker-editor-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-ui-masks")) {
            initInterface();
            readyNativeChecks();
            endingCreditsPresentationChecks();
            uiMaskCoverageChecks();
            uiMaskCoveragePreviews();
            atomicWrite("test-output/ui-mask-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-maker-card")) {
            initInterface();
            makerWorldCardChecks();
            atomicWrite("test-output/maker-card-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-background-responsive")) {
            backgroundExtensionChecks();
            responsiveBackgroundChecks();
            responsiveCloudChecks();
            patternedBackdropChecks();
            storyBackdropChecks();
            backgroundNativeChecks();
            responsiveBackgroundPreviews();
            backdropRegionPreviews();
            atomicWrite("test-output/background-responsive-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-preview-visual-audio")) {
            potionVisualChecks();
            uiJumpDetailNativeChecks();
            selectorPreviewAudioChecks();
            atomicWrite("test-output/preview-visual-audio-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-potion-visual")) {
            potionVisualChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-cloud-transfer")) {
            if (contains(commandLine, "face")) {
                cloudSupportFaceChecks();
                cloudSupportHandoffChecks();
                cloudSupportMatrixChecks();
                shutdownAudio();
                ExitProcess(failures ? 1 : 0);
            }
            cloudSupportFaceChecks();
            cloudSupportHandoffChecks();
            cloudSupportMatrixChecks();
            cloudTransferChecks();
            if (contains(commandLine, "campaign"))
                cloudTransferCampaignChecks();
            cloudEndpointChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-creature-support")) {
            creatureSupportNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-resize-position")) {
            resizeCheckpointPositionNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-skeletonkey")) {
            skeletonKeyNativeChecks();
            previewWindowsPortCredits();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-gem")) {
            helperGemNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-orbit-attachment-birth")) {
            orbitAttachmentBirthNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-attachment-layer")) {
            attachmentLayerNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-projectile-pool")) {
            projectilePoolNativeChecks();
            atomicWrite("test-output/projectile-pool-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-water-regression")) {
            waterChecks();
            waterSoundNativeChecks();
            atomicWrite("test-output/water-regression-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-widescreen-resolution")) {
            widescreenResolutionChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-stage53-whole")) {
            stage53WholeNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-stage53-source")) {
            stage53SourceNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-coin-sparkle")) {
            coinSparkleNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-group-collectable-lifecycle")) {
            groupCollectableLifecycleNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-single-collectable-lifecycle")) {
            singleCollectableLifecycleNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-ending-collectable")) {
            initInterface();
            endingTransformationChecks();
            atomicWrite("test-output/ending-collectable-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-single-collectable")) {
            singleCollectableNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-collectable-regression")) {
            collectableNativeChecks();
            helperGemNativeChecks();
            atomicWrite("test-output/collectable-regression-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-pinball-power")) {
            pinballPowerNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-pinball-dynamic")) {
            pinballDynamicNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-pinball-fixture")) {
            pinballFixtureNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-pinball-regression")) {
            initInterface();
            pinballNativeChecks();
            pinballPhaseNativeChecks();
            selectorDeferredNativeChecks();
            atomicWrite("test-output/pinball-regression-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-level-end")) {
            levelEndNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-goal-regression")) {
            initInterface();
            goalLaunchChecks();
            stageClearMathChecks();
            stageClearUiChecks();
            completionGoldShoeChecks();
            atomicWrite("test-output/goal-regression-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-inherited-box")) {
            inheritedBoxNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-switch-legacy-idle")) {
            switchLegacyIdleNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-mechanism-event-disk-reload")) {
            mechanismEventFreshDiskNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-mechanism-event-checkpoint")) {
            mechanismEventCheckpointNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-touch-cooldown-disk-reload")) {
            bool ok = touchClockFreshDiskNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-touch-cooldown-load")) {
            touchCooldownLoadNativeChecks();
            touchAlternatorNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-touch-material-regression")) {
            touchAlternatorNativeChecks();
            alternatorBodyNativeChecks();
            atomicWrite("test-output/touch-material-regression-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-water-save")) {
            waterSaveNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-water-membership")) {
            waterMembershipNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-projectile-ccd")) {
            projectileCcdNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-circle-fixture")) {
            circleFixtureNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-fixture-filter")) {
            fixtureFilterNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-fish-cloud")) {
            fishCloudNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-cloud-seam")) {
            cloudSeamNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-cloud-spring")) {
            cloudSpringNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-deadly-gravity-disk-reload")) {
            bool ok = deadlyGravityDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-deadly-gravity")) {
            deadlyGravityNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-rotor-gravity-disk-reload")) {
            bool ok = rotorGravityDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-rotation")) {
            rotationNativeChecks();
            atomicWrite("test-output/rotation-gravity-regression-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-rotor-gravity")) {
            rotorGravityNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-jumper-gravity-contact")) {
            frogChecks();
            frogInterruptNativeChecks();
            jumperGroundNativeChecks();
            jumperWaterPoseChecks();
            jumperAnimationNativeChecks();
            atomicWrite("test-output/jumper-gravity-contact-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-jumper-gravity-disk-reload")) {
            bool ok = jumperGravityDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-jumper-gravity")) {
            jumperGravityNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-ui-nonflight")) {
            helperCollisionScopeUiNativeChecks(0);
            helperCollisionScopeUiNativeChecks(2);
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-nonflight")) {
            helperCollisionScopeMatrixNativeChecks(0);
            helperCollisionScopeMatrixNativeChecks(2);
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-collision-matrix")) {
            helperCollisionScopeMatrixNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-oneway-traversal")) {
            helperOneWayTraversalNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-collision-ui")) {
            helperCollisionScopeUiNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-projectile-ui")) {
            helperProjectileUiScopeNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-projectile-scope")) {
            helperProjectileScopeNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-collision-inventory")) {
            helperCollisionScopeInventoryNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-checkpoint-disk-reload")) {
            bool ok = helperCheckpointDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-helper-checkpoint-disk")) {
            helperCheckpointDiskNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-checkpoint-return")) {
            helperCheckpointReturnNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-death-audio")) {
            wizardDeathAudioNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-audio-regression")) {
            initInterface();
            jumpAudioFollowupChecks();
            rescueJumpAudioNativeChecks();
            audioJumpTraceChecks();
            audioSilenceKeepAliveChecks();
            jellySoundNativeChecks();
            wizardMusicChecks();
            wizardGateAudioChecks();
            introChecks();
            titleAttractChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-hit-disk-reload")) {
            bool ok = wizardSolvedDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-wizard-solved") &&
            !contains(commandLine, "--self-test-wizard-solved-regression")) {
            wizardSolvedNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-spring-regression")) {
            springPhaseNativeChecks();
            springPhasePlacementChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-solved-regression")) {
            initInterface();
            wizardFollowupChecks();
            wizardNativeFollowupChecks();
            wizardDismissNativeChecks();
            wizardTransitionNativeChecks();
            wizardMusicChecks();
            wizardGateAudioChecks();
            wizardFinalCameraChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-magnet-solved")) {
            magnetSolvedNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-magnet-contact-regression")) {
            magnetSeamChecks();
            selectorMagnetChecks();
            magnetCheckpointContactChecks();
            springIncomingContactChecks();
            selectorMechanismContactChecks();
            stormySpringContactChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-credits-jumper-ground")) {
            creditsJumperGroundNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-frog-solved-order")) {
            frogSolvedOrderNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-frog-spin")) {
            frogSpinNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-gravity-credits")) {
            creditsRuntimeNativeChecks();
            atomicWrite("test-output/walker-gravity-credits-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-walker-gravity")) {
            walkerGravityNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-walk-spawn")) {
            wizardWalkSpawnNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-walk-callback")) {
            walkCallbackNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-walk-animation")) {
            enemyAnimationBoundaryNativeChecks();
            atomicWrite("test-output/walk-animation-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-walk-permission-disk-reload")) {
            bool ok = walkPermissionDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-walk-permission")) {
            walkPermissionNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-walk-side-callback")) {
            enemyEdgeBirthNativeChecks(false);
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-enemy-edge-birth")) {
            enemyEdgeBirthNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-enemy-edge")) {
            if (contains(commandLine, "--self-test-enemy-edge-context"))
                enemyEdgeContextNativeChecks();
            else
                enemyEdgeNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-dismiss")) {
            wizardDismissNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-bat-smoke-disk-reload")) {
            bool ok = batSmokeDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-bat-smoke")) {
            batSmokeNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-ordinary-phase-regression")) {
            initInterface();
            cloud57JumpNativeChecks();
            selectorPreviewGeometryRepairChecks();
            selectorSlopeNativeChecks();
            boxPushNativeChecks();
            stompNativeChecks();
            jumpAudioFollowupChecks();
            oneWayGeometryChecks();
            killableContactNativeChecks();
            killableDeathNativeChecks();
            shotDeathNativeChecks();
            uiJumpDetailNativeChecks();
            rescueJumpAudioNativeChecks();
            tomJumpDiagnosticChecks();
            touchAlternatorNativeChecks();
            atomicWrite("test-output/ordinary-phase-regression-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-ordinary-phase-disk-reload")) {
            bool ok = ordinaryPhaseDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-ordinary-phase-audit")) {
            ordinaryPhaseAuditNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-selector-deferred-regression")) {
            startStage(0);
            readyTimer = 0;
            initInterface();
            selectorMotionChecks();
            selectorMechanismContactChecks();
            selectorTrampolineNativeChecks();
            uiJumpDetailNativeChecks();
            atomicWrite("test-output/selector-deferred-regression-results.txt", testLog,
                        lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-selector-deferred")) {
            selectorDeferredNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-checkpoint-phase-disk-reload")) {
            bool ok = checkpointSavePhaseDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-checkpoint-phase")) {
            checkpointSavePhaseNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-source-track-disk-reload")) {
            bool ok = sourceTrackDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-source-track")) {
            sourceTrackNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-water-sound")) {
            waterSoundNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-particle-restore")) {
            continuousRestoreNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-alternator-body")) {
            alternatorBodyNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-breakable")) {
            breakableSolvedNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-ending-object-mute")) {
            endingObjectMuteNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-pickup-presentation")) {
            helperPickupPresentationNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-affector")) {
            helperAffectorNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-solved")) {
            helperSolvedNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-perch")) {
            helperPerchNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-disk-reload")) {
            bool ok = helperAnimationDiskReloadNative();
            shutdownAudio();
            ExitProcess(ok ? 0 : 1);
        }
        if (contains(commandLine, "--self-test-helper-clip-baseline")) {
            helperAnimationVisualContinuityChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-animation")) {
            helperAnimationNativeChecks();
            helperChecks();
            cameraRestoreNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-intro-return")) {
            introChecks();
            titleAttractChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-cloud57")) {
            cloud57JumpNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-camera-fresh-disk")) {
            helperCameraFreshDiskNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-camera-restore")) {
            helperCameraRestoreNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-camera-restore")) {
            cameraRestoreNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-helper-audio")) {
            helperChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-proximity-phase")) {
            proximityPhaseNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-optimization")) {
            memset(&progress, 0, sizeof(progress));
            musicEnabled = soundEnabled = false;
            progress.unlocked = stageCount - 1;
            initInterface();
            audiovisualChecks();
            selectorPhysicsChecks();
            endingCreditsPresentationChecks();
            pauseTransitionNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-physics-force")) {
            physicsCheckpointForceNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-wizard-hud")) {
            initInterface();
            wizardHudNativeChecks();
            atomicWrite("test-output/wizard-hud-results.txt", testLog, lstrlenA(testLog));
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-selector-slope")) {
            lstrcpyA(saveRoot, "test-output/selector-slope-saves");
            CreateDirectoryA("test-output", 0);
            CreateDirectoryA(saveRoot, 0);
            initInterface();
            selectorSlopeNativeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        if (contains(commandLine, "--self-test-box-push")) {
            lstrcpyA(saveRoot, "test-output/box-push-saves");
            CreateDirectoryA("test-output", 0);
            CreateDirectoryA(saveRoot, 0);
            initInterface();
            boxPushProbeNative();
            completionGoldShoeChecks();
            shutdownAudio();
            ExitProcess(failures ? 1 : 0);
        }
        int rc = selfTest();
        shutdownAudio();
        ExitProcess(rc);
    }
#endif
    readProgress();
    initAudio(soundFiles);
    initInterface();
    timeBeginPeriod(1);
    LARGE_INTEGER frequency, last, now;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last);
    double accumulator = 0;
    MSG message;
    while (running) {
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT)
                running = false;
            if (titleAttractMessage(message))
                continue;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        QueryPerformanceCounter(&now);
        double delta = (double)(now.QuadPart - last.QuadPart) / (double)frequency.QuadPart;
        last = now;
        titleAttractRuntimeFrame(delta, GetForegroundWindow() == window && !IsIconic(window));
        if (delta > 0)
            nativeMotionRenderFPS = (float)(1.0 / delta);
        if (delta > 0.1)
            delta = 0.1;
        accumulator += delta;
        uiTime += (float)delta;
        int steps = 0;
        while (accumulator >= 1.0 / 120 && steps++ < 12) {
            tickInterface(1 / 120.0f);
            tick((turbo ? 1.5f : 1.0f) / 120.0f, held(0), held(1));
            accumulator -= 1.0 / 120;
        }
        render();
        Sleep(1);
    }
    makerShutdown();
    writeProgress();
    titleAttractReset();
    introClose();
    shutdownAudio();
#if BQ_ENABLE_TESTS
    flushAudioJumpTrace();
    flushTomJumpDiagnostics();
#endif
    timeEndPeriod(1);
    wglMakeCurrent(0, 0);
    wglDeleteContext(context);
    ReleaseDC(window, gameDC);
    DestroyWindow(window);
    ExitProcess(0);
}
