#pragma once

// Media Foundation playback backend for the unchanged intro movie.
// Optional native MP4 decoder, dynamically loaded on Windows7+.
#pragma push_macro("WINVER")
#pragma push_macro("_WIN32_WINNT")
#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <mfplay.h>
#pragma pop_macro("_WIN32_WINNT")
#pragma pop_macro("WINVER")
#ifdef small
#undef small
#endif

void introResize();

struct IntroMfPlatform {
    HMODULE ole, mf;
    bool tried, available;
};

IntroMfPlatform introMfPlatform;

struct IntroMfState {
    IMFPMediaPlayer *player;
    HWND video;
};

IntroMfState introMf;

bool introActive() {
    return introMovie.device || introMf.player;
}

void introMfEvent(MFP_EVENT_HEADER *event) {
    if (!introMf.player || event->pMediaPlayer != introMf.player)
        return;
    if (FAILED(event->hrEvent)) {
        introMovie.status = INTRO_ERROR;
        introMovie.error = (MCIERROR)event->hrEvent;
        return;
    }
    if (event->eEventType == MFP_EVENT_TYPE_MEDIAITEM_SET) {
        SIZE size = {}, aspect = {};
        if (SUCCEEDED(introMf.player->GetNativeVideoSize(&size, &aspect))) {
            introMovie.width = size.cx;
            introMovie.height = size.cy;
        }
        introResize();
        introMovie.volumeError = (MCIERROR)introMf.player->SetVolume(introMovie.volume);
        if (!introMovie.paused)
            introMf.player->Play();
        else
            introMovie.ready = true;
    }
    if (event->eEventType == MFP_EVENT_TYPE_PLAY)
        introMovie.ready = true;
    if (event->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED)
        introMovie.status = INTRO_FINISHED;
}

struct IntroMfCallback : IMFPMediaPlayerCallback {
    LONG refs;

    IntroMfCallback() : refs(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **out) {
        if (!out)
            return E_POINTER;
        *out = 0;
        const GUID &cb = __uuidof(IMFPMediaPlayerCallback);
        const GUID &unknown = __uuidof(IUnknown);
        bool a = true, b = true;
        for (int i = 0; i < 16; i++) {
            a &= ((const u8 *)&iid)[i] == ((const u8 *)&cb)[i];
            b &= ((const u8 *)&iid)[i] == ((const u8 *)&unknown)[i];
        }
        if (!a && !b)
            return E_NOINTERFACE;
        *out = static_cast<IMFPMediaPlayerCallback *>(this);
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() {
        return (ULONG)InterlockedIncrement(&refs);
    }

    ULONG STDMETHODCALLTYPE Release() {
        return (ULONG)InterlockedDecrement(&refs);
    }

    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER *event) {
        introMfEvent(event);
    }
};

IntroMfCallback introMfCallback;

LRESULT CALLBACK introMfVideoProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_ERASEBKGND)
        return 1;
    if (msg == WM_PAINT) {
        PAINTSTRUCT p;
        BeginPaint(hwnd, &p);
        if (introMf.player)
            introMf.player->UpdateVideo();
        EndPaint(hwnd, &p);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

void introMfClose() {
    IMFPMediaPlayer *player = introMf.player;
    introMf.player = 0;
    if (player) {
        player->Shutdown();
        player->Release();
    }
    if (introMf.video)
        DestroyWindow(introMf.video);
    introMf.video = 0;
}

bool introMfOpen(const char *path, float volume) {
    if (!introMfPlatform.tried) {
        introMfPlatform.tried = true;
        introMfPlatform.ole = LoadLibraryA("ole32.dll");
        introMfPlatform.mf = LoadLibraryA("mfplay.dll");
        if (introMfPlatform.ole && introMfPlatform.mf) {
            typedef HRESULT(WINAPI * Init)(LPVOID, DWORD);
            Init init = (Init)GetProcAddress(introMfPlatform.ole, "CoInitializeEx");
            introMfPlatform.available = init && SUCCEEDED(init(0, COINIT_APARTMENTTHREADED));
        }
    }
    if (!introMfPlatform.available)
        return false;
    typedef HRESULT(WINAPI * Create)(LPCWSTR, BOOL, MFP_CREATION_OPTIONS, IMFPMediaPlayerCallback *,
                                     HWND, IMFPMediaPlayer **);
    Create create = (Create)GetProcAddress(introMfPlatform.mf, "MFPCreateMediaPlayer");
    if (!create)
        return false;
    WNDCLASSA c = {};
    c.lpfnWndProc = introMfVideoProcedure;
    c.hInstance = GetModuleHandleA(0);
    c.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    c.lpszClassName = "BeansQuestNativeMp4";
    RegisterClassA(&c);
    introMf.video = CreateWindowExA(0, c.lpszClassName, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                    0, 0, 1, 1, window, 0, c.hInstance, 0);
    if (!introMf.video)
        return false;
    HRESULT hr = create(0, FALSE, 0, &introMfCallback, introMf.video, &introMf.player);
    if (FAILED(hr)) {
        introMovie.error = (MCIERROR)hr;
        introMfClose();
        return false;
    }
    wchar_t relative[MAX_PATH], absolute[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, relative, MAX_PATH);
    GetFullPathNameW(relative, MAX_PATH, absolute, 0);
    IMFPMediaItem *item = 0;
    hr = introMf.player->CreateMediaItemFromURL(absolute, TRUE, 0, &item);
    if (SUCCEEDED(hr)) {
        introMovie.volume = volume;
        introMf.player->SetVolume(volume);
        hr = introMf.player->SetMediaItem(item);
        item->Release();
    }
    if (FAILED(hr)) {
        introMovie.error = (MCIERROR)hr;
        introMfClose();
        return false;
    }
    introMovie.originalClipChildren = GetWindowLongA(window, GWL_STYLE) & WS_CLIPCHILDREN;
    SetWindowLongA(window, GWL_STYLE, GetWindowLongA(window, GWL_STYLE) | WS_CLIPCHILDREN);
    introMovie.status = INTRO_PLAYING;
    introMovie.error = 0;
    introResize();
    return true;
}
