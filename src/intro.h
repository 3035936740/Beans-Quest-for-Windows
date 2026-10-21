#pragma once

// Intro playback lifecycle, return destination and fallback handling.
#include <digitalv.h>

// CutsceneMovieController::OnMovieDone @0x187424 is the only native
// completion source. Missing media must never become a fabricated timer.
extern HWND window;

enum IntroStatus { INTRO_NONE, INTRO_MISSING, INTRO_PLAYING, INTRO_FINISHED, INTRO_ERROR };

struct IntroMovie {
    MCIDEVICEID device;
    IntroStatus status;
    u64 next, settingsOrigin, settingsFocus;
    MCIERROR error, volumeError;
    char path[MAX_PATH];
    LONG originalClipChildren;
    float volume;
    bool paused, ready;
    int width, height;
    RECT destination;
};

IntroMovie introMovie;
#include "intro_mf.h"

void introVolume(float volume) {
    introMovie.volume = clamp(volume, 0, 1);
    if (introMf.player) {
        introMovie.volumeError = (MCIERROR)introMf.player->SetVolume(introMovie.volume);
        return;
    }
    if (!introMovie.device)
        return;
    MCI_DGV_SETAUDIO_PARMSA p = {};
    p.dwItem = MCI_DGV_SETAUDIO_VOLUME;
    p.dwValue = (DWORD)(int)(introMovie.volume * 1000 + .5f);
    introMovie.volumeError =
        mciSendCommandA(introMovie.device, MCI_SETAUDIO,
                        MCI_DGV_SETAUDIO_ITEM | MCI_DGV_SETAUDIO_VALUE, (DWORD_PTR)&p);
}

void introPause(bool paused) {
    if (!introActive() || introMovie.paused == paused || introMovie.status != INTRO_PLAYING)
        return;
    introMovie.paused = paused;
    if (introMf.player) {
        if (paused)
            introMf.player->Pause();
        else
            introMf.player->Play();
        return;
    }
    if (paused)
        mciSendCommandA(introMovie.device, MCI_PAUSE, MCI_WAIT, 0);
    else {
        MCI_PLAY_PARMS p = {};
        p.dwCallback = (DWORD_PTR)window;
        mciSendCommandA(introMovie.device, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&p);
    }
}

void introResize() {
    if (!introActive())
        return;
    int w = viewportW, h = viewportH;
    if (introMovie.width > 0 && introMovie.height > 0) {
        float scale = minf((float)w / introMovie.width, (float)h / introMovie.height);
        w = (int)(introMovie.width * scale + .5f);
        h = (int)(introMovie.height * scale + .5f);
    }
    MCI_DGV_PUT_PARMS p = {};
    p.rc.left = viewportX + (viewportW - w) / 2;
    p.rc.top = clientH - viewportY - viewportH + (viewportH - h) / 2;
    p.rc.right = w;
    p.rc.bottom = h;
    introMovie.destination = p.rc;
    if (introMf.player) {
        SetWindowPos(introMf.video, 0, p.rc.left, p.rc.top, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
        introMf.player->UpdateVideo();
        return;
    }
    mciSendCommandA(introMovie.device, MCI_PUT, MCI_DGV_PUT_DESTINATION, (DWORD_PTR)&p);
}

void introClose() {
    bool active = introActive();
    introMfClose();
    MCIDEVICEID device = introMovie.device;
    introMovie.device = 0;
    if (device) {
        mciSendCommandA(device, MCI_STOP, MCI_WAIT, 0);
        mciSendCommandA(device, MCI_CLOSE, MCI_WAIT, 0);
    }
    if (active && !introMovie.originalClipChildren)
        SetWindowLongA(window, GWL_STYLE, GetWindowLongA(window, GWL_STYLE) & ~WS_CLIPCHILDREN);
}

void introNotify(WPARAM event, LPARAM device) {
    if (introMf.player || introMovie.status != INTRO_PLAYING || !introMovie.device ||
        (MCIDEVICEID)device != introMovie.device)
        return;
    if (event == MCI_NOTIFY_SUCCESSFUL)
        introMovie.status = INTRO_FINISHED;
    else if (event == MCI_NOTIFY_FAILURE) {
        introMovie.status = INTRO_ERROR;
        introMovie.error = MCIERR_DEVICE_NOT_READY;
        introClose();
    }
}

bool introFileExists(const char *path) {
    DWORD flags = GetFileAttributesA(path);
    return flags != INVALID_FILE_ATTRIBUTES && !(flags & FILE_ATTRIBUTE_DIRECTORY);
}

bool introPlayPath(const char *path, u64 next, float initialVolume = 1) {
    introClose();
    memset(&introMovie, 0, sizeof(introMovie));
    introMovie.next = next;
    lstrcpynA(introMovie.path, path, MAX_PATH);
    if (!introFileExists(path)) {
        introMovie.status = INTRO_MISSING;
        return false;
    }
    if (contains(path, ".mp4") && introMfOpen(path, initialVolume))
        return true;
    MCI_DGV_OPEN_PARMSA p = {};
    p.lpstrElementName = introMovie.path;
    p.lpstrDeviceType = contains(path, ".mp4") ? "MPEGVideo" : "avivideo";
    p.hWndParent = window;
    p.dwStyle = WS_CHILD | WS_VISIBLE;
    DWORD flags =
        MCI_OPEN_ELEMENT | MCI_OPEN_TYPE | MCI_DGV_OPEN_PARENT | MCI_DGV_OPEN_WS | MCI_WAIT;
    introMovie.error = mciSendCommandA(0, MCI_OPEN, flags, (DWORD_PTR)&p);
    if (introMovie.error) {
        introMovie.status = INTRO_ERROR;
        return false;
    }
    introMovie.device = p.wDeviceID;
    introMovie.originalClipChildren = GetWindowLongA(window, GWL_STYLE) & WS_CLIPCHILDREN;
    SetWindowLongA(window, GWL_STYLE, GetWindowLongA(window, GWL_STYLE) | WS_CLIPCHILDREN);
    MCI_DGV_WHERE_PARMS source = {};
    if (!mciSendCommandA(introMovie.device, MCI_WHERE, MCI_DGV_WHERE_SOURCE, (DWORD_PTR)&source)) {
        introMovie.width = source.rc.right;
        introMovie.height = source.rc.bottom;
    }
    introResize();
    introVolume(initialVolume);
    MCI_PLAY_PARMS play = {};
    play.dwCallback = (DWORD_PTR)window;
    introMovie.status = INTRO_PLAYING;
    introMovie.error = mciSendCommandA(introMovie.device, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&play);
    if (introMovie.error) {
        introMovie.status = INTRO_ERROR;
        introClose();
        return false;
    }
    introMovie.ready = true;
    return true;
}

void introOpen(Level &level) {
    const Props *config = 0;
    for (int i = 0; i < level.objectCount; i++)
        if (level.objects[i].has("CutsceneMovieController")) {
            config = level.objects[i].behavior("CutsceneMovieController");
            break;
        }
    u64 next = config ? idnum(config->get("next-level")) : 0;
    if (!next)
        next = idnum(level.props.get("next-level"));
    const char *names[] = {config ? config->get("movie-name-hq", "movie2") : "movie2",
                           config ? config->get("movie-name", "movie1") : "movie1"};
    char selected[MAX_PATH] = {};
    for (int i = 0; i < 2 && !*selected; i++)
        for (int extension = 0; extension < 2 && !*selected; extension++) {
            char path[MAX_PATH];
            lstrcpyA(path, "data/Media/Movies/");
            lstrcatA(path, names[i]);
            lstrcatA(path, extension ? ".mp4" : ".avi");
            if (introFileExists(path))
                lstrcpyA(selected, path);
        }
    // The supplied movie also serves the authored Watch Intro scene, whose
    // APK configuration names the absent movie1/movie2 files. Keep that
    // scene's next destination for direct scene entry. A Settings button
    // replay records its caller separately after this opens successfully.
    if (!*selected && introFileExists("data/Media/Movies/intro.mp4"))
        lstrcpyA(selected, "data/Media/Movies/intro.mp4");
    if (*selected)
        introPlayPath(selected, next);
    else {
        introClose();
        memset(&introMovie, 0, sizeof(introMovie));
        introMovie.next = next;
        introMovie.status = INTRO_MISSING;
    }
    stopMusic();
}

void drawIntroStatus() {
    box(0, 0, viewW, viewH, 0, 0, 0);
    if (introMovie.status == INTRO_PLAYING || introMovie.status == INTRO_FINISHED)
        return;
    centered(246,
             introMovie.status == INTRO_ERROR ? "ORIGINAL INTRO COULD NOT PLAY"
                                              : "ORIGINAL INTRO MOVIE IS MISSING",
             13);
    centered(212,
             introMovie.status == INTRO_ERROR
                 ? "The movie could not be decoded on this system."
                 : "This APK does not include the original intro video.",
             10);
    centered(190, "Movie folder: data/Media/Movies/", 10);
    centered(172, "Original MP4 files also work with a supported codec.", 9);
    if (introMovie.error) {
        char message[128];
        if (mciGetErrorStringA(introMovie.error, message, sizeof(message)))
            text(24, 140, message, 9, 1, 0.75f, 0.4f);
    }
    centered(100, "Space: continue without the movie", 11);
    centered(78, "Esc: return to settings", 11);
}
