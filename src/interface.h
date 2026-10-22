#pragma once

// Original menu scenes, focus, pointer input and screen transitions, including playable selector backgrounds.
#include "render.h"
#include "windows_port_credits.h"
#include "window_resolution.h"
#include "intro.h"
#include "stage_clear_native.h"
#include "ending_credits_native.h"
#include "ending_credits_presentation_native.h"
#include "selector_preview_repair.h"
#include "selector_preview_geometry_repair.h"
#include "selector_water_surface_repair.h"
#include "pause_transition_native.h"
#include "title_presentation_native.h"
#include "world_select_native.h"

// Original Pimento scenes remain separate from the running level and HUD.
// This allows menus to preserve the current checkpoint and game simulation.
struct UiNode {
    Object *object;
    Actor *art;
    Rect rect;
    bool hidden, disabled;
    int stage, world;
};
class UiContacts;
#include "selector_special_jump_native.h"

struct UiScene {
    Arena memory;
    Level level;
    UiNode *nodes;
    bool *menuLayers;
    int focus, selectorLayer;
    float age, cameraX, cameraY, targetX, homeX, homeY, minimizePhase;
    Entity bean;
    State beanSpawn;
    Entity *playfield;
    int playfieldCount, miniCoins;
    float miniDeath, trampolineApex;
    bool hasBean, minimized;
    b2World *physics;
    b2Body **bodies;
    b2Body **oscillatorProxies;
    DiagonalRuntimeState *diagonal;
    OscillatorRuntimeState *oscillator;
    UiContacts *contacts;
    bool waterStartup, inWater, ordinaryJumpNative;
    SelectorSpecialJumpNative specialJump;
};

UiScene page, hudScene;

// Course maker hooks; maker.h at the end of this file implements them.
bool makerWorldNode(const Object &o);
bool makerDrawWorldNode(UiScene &s, UiNode &n);
void makerDecorateWorldSelect();
void makerOpenBrowser();
bool makerHudCaption(u64 id, char *value);
bool makerKey(int key);
bool makerPointerMove(float x, float y);
bool makerPointerDown(float x, float y, int button);
bool makerPointerUp(float x, float y, int button);
void makerTick(float dt);
void makerRender();
void makerRenderPlayOverlay();

PauseTransitionNativeState pauseInterfaceNative;
u64 pauseBackgroundNative, pauseInitialFocusNative = 7545429638806213302ULL;
int pauseLayerNative = 1;

void bindPauseInterfaceNative() {
    pauseBackgroundNative = 0;
    pauseInitialFocusNative = 7545429638806213302ULL;
    pauseLayerNative = 1;
    for (int i = 0; i < hudScene.level.objectCount; i++) {
        Object &o = hudScene.level.objects[i];
        const Props *p = o.behavior("PauseMenuController");
        if (p) {
            pauseBackgroundNative = idnum(p->get("background"));
            pauseInitialFocusNative = idnum(p->get("initial-focus"));
            pauseLayerNative = o.layer;
            break;
        }
    }
}

bool pauseMenuInputReady() {
    return screen != PAUSE || !pauseInterfaceNative.active ||
           pauseTransitionNativeInputReady(pauseInterfaceNative);
}

void buildCreditsRuntime(UiScene &s);
void tickCreditsRuntime(UiScene &s, float dt);
bool drawCreditsRuntimeNode(UiScene &s, Object &o);

struct EndingCreditsSceneBindings {
    EndingCreditsPresentationInput input;
    u64 emilio, maria, emilioText, mariaText, endText, tint, fadeIn;
};

EndingCreditsSceneBindings endingCreditsScene;

void bindEndingCreditsScene(UiScene &s) {
    endingCreditsScene = EndingCreditsSceneBindings{};
    endingCreditsScene.input.startY = s.cameraY + viewH * 0.5f;
    for (int i = 0; i < s.level.objectCount; i++) {
        const Props *p = s.level.objects[i].behavior("CutsceneEndingController");
        if (!p)
            continue;
        endingCreditsScene.emilio = idnum(p->get("emilio"));
        endingCreditsScene.maria = idnum(p->get("maria"));
        endingCreditsScene.emilioText = idnum(p->get("emilio-text"));
        endingCreditsScene.mariaText = idnum(p->get("maria-text"));
        endingCreditsScene.endText = idnum(p->get("end-text"));
        endingCreditsScene.tint = idnum(p->get("tint-object"));
        endingCreditsScene.fadeIn = idnum(p->get("fade-in-object"));
    }
    for (int i = 0; i < s.level.objectCount; i++) {
        Object &o = s.level.objects[i];
        Vec c = center(o.rect);
        if (o.id == endingCreditsScene.emilio) {
            endingCreditsScene.input.emilioX = c.x;
            endingCreditsScene.input.emilioY = c.y;
        }
        if (o.id == endingCreditsScene.maria) {
            endingCreditsScene.input.mariaX = c.x;
            endingCreditsScene.input.mariaY = c.y;
        }
    }
}

// Resize preserves camera centres. LayoutActors always starts from authored
// rectangles, so repeated resize never accumulates anchor displacement.
void viewportSceneResize(float oldWidth, float oldHeight) {
    float dx = (oldWidth - viewW) * 0.5f, dy = (oldHeight - viewH) * 0.5f;
    camX += dx;
    camY += dy;
    cameraState.x += dx;
    cameraState.y += dy;
    wizardCameraBlend.fromX += dx;
    wizardCameraBlend.fromY += dy;
    UiScene *scenes[] = {&page, &hudScene};
    for (int i = 0; i < 2; i++) {
        UiScene &s = *scenes[i];
        s.cameraX += dx;
        s.cameraY += dy;
        s.homeX += dx;
        s.homeY += dy;
        s.targetX += dx;
    }
}

void burstUiCloud(Entity &e);
#include "selector_physics.h"
bool pointerHeld, pointerDragged;
int pointerNode = -1;
float pointerStartX, pointerStartY, pointerCameraX;
u64 pointerScene;

void resetPauseInterfaceNative() {
    pauseInterfaceNative = PauseTransitionNativeState{};
    hudScene.focus = -1;
    pointerHeld = false;
    pointerNode = -1;
}

bool running = true, fullscreen, keys[256], turbo;
float transitionTime, transitionDuration = 0.22f;
int selectedWorld = 1;
Screen optionReturn = TITLE;
int resolutionChoice = DEFAULT_WINDOW_RESOLUTION;
bool windowMode = true;
int rebindAction = -1, rebindColumn, debugSequence;

struct UiPreferences {
    u32 magic, version;
    int resolution, full, turbo;
    u32 checksum;
};

void writePreferences() {
    UiPreferences p = {0x55535142, 1, resolutionChoice, fullscreen ? 1 : 0, turbo ? 1 : 0, 0};
    p.checksum = hashBytes(&p, sizeof(p) - 4);
    char path[MAX_PATH];
    savePath(path, "settings.dat");
    atomicWrite(path, &p, sizeof(p));
}

struct Bindings {
    u32 magic;
    int primary[8], secondary[8];
    u32 checksum;
};

Bindings bindings;
int *primaryKey = bindings.primary;
int *secondaryKey = bindings.secondary;
void toggleFullscreen();
void resizeWindow(int w, int h);
void navigate(u64 id);
void confirm();
void cancel();
bool worldOpen(int world);
bool selectorLevelUnlocked(u64 level);

void defaultBindings() {
    const int a[8] = {VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_SPACE, VK_ESCAPE, 'S', 'R'};
    const int b[8] = {'A', 'D', 0, 'X', VK_RETURN, VK_BACK, 0, 0};
    memset(&bindings, 0, sizeof(bindings));
    bindings.magic = 0x4B535142;
    for (int i = 0; i < 8; i++) {
        bindings.primary[i] = a[i];
        bindings.secondary[i] = b[i];
    }
}

void readBindings() {
    defaultBindings();
    char p[MAX_PATH];
    savePath(p, "controls.dat");
    Bindings tmp;
    if (exactRead(p, &tmp, sizeof(tmp)) && tmp.magic == 0x4B535142 &&
        tmp.checksum == hashBytes(&tmp, sizeof(tmp) - 4)) {
        bool ok = true;
        for (int i = 0; i < 8; i++)
            if (tmp.primary[i] < 0 || tmp.primary[i] > 255 || tmp.secondary[i] < 0 ||
                tmp.secondary[i] > 255)
                ok = false;
        if (ok)
            bindings = tmp;
    }
}

void writeBindings() {
    bindings.checksum = hashBytes(&bindings, sizeof(bindings) - 4);
    char p[MAX_PATH];
    savePath(p, "controls.dat");
    atomicWrite(p, &bindings, sizeof(bindings));
}

bool bound(int key, int action) {
    return key && (key == primaryKey[action] || key == secondaryKey[action]);
}

bool held(int action) {
    return keys[primaryKey[action]] || (secondaryKey[action] && keys[secondaryKey[action]]);
}

#include "title_attract.h"

struct GlyphFont {
    GLuint texture;
    int size, mono, bold;
    float width[128], lineHeight;
};

GlyphFont glyphFonts[32];
int glyphFontCount;

GlyphFont *originalFont(const char *face, int size) {
    size = (int)clamp((float)size, 6, 40);
    int mono = contains(face, "Mo"), bold = contains(face, "Bd") || contains(face, "BI");
    for (int i = 0; i < glyphFontCount; i++)
        if (glyphFonts[i].size == size && glyphFonts[i].mono == mono && glyphFonts[i].bold == bold)
            return glyphFonts + i;
    if (glyphFontCount == 32)
        return glyphFonts;
    GlyphFont &f = glyphFonts[glyphFontCount++];
    f.size = size;
    f.mono = mono;
    f.bold = bold;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = 1024;
    bi.bmiHeader.biHeight = -512;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void *pixels = 0;
    HDC dc = CreateCompatibleDC(0);
    HBITMAP bitmap = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &pixels, 0, 0);
    HGDIOBJ old = SelectObject(dc, bitmap);
    memset(pixels, 0, 1024 * 512 * 4);
    HFONT font =
        CreateFontA(-size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH,
                    mono ? "Bitstream Vera Sans Mono" : "Bitstream Vera Sans");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    TEXTMETRICA metrics;
    GetTextMetricsA(dc, &metrics);
    f.lineHeight = (float)metrics.tmHeight;
    for (int c = 32; c < 127; c++) {
        char ch = (char)c;
        SIZE extent;
        GetTextExtentPoint32A(dc, &ch, 1, &extent);
        f.width[c] = (float)extent.cx;
        TextOutA(dc, (c % 16) * 64 + 1, (c / 16) * 64 + 1, &ch, 1);
    }
    GdiFlush();
    u8 *bytes = (u8 *)pixels;
    for (int i = 0; i < 1024 * 512; i++) {
        bytes[i * 4 + 3] = bytes[i * 4];
        bytes[i * 4] = bytes[i * 4 + 1] = bytes[i * 4 + 2] = 255;
    }
    glGenTextures(1, &f.texture);
    glBindTexture(GL_TEXTURE_2D, f.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    SelectObject(dc, oldFont);
    SelectObject(dc, old);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return &f;
}

u32 hexColor(const char *s) {
    u32 n = 0;
    for (int i = 0; i < 6 && s[i]; i++) {
        char c = s[i];
        n = (n << 4) + (c >= '0' && c <= '9'   ? c - '0'
                        : c >= 'a' && c <= 'f' ? c - 'a' + 10
                        : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                               : 0);
    }
    return n;
}

struct Colour {
    float r, g, b, a;
};

Colour colour(const char *s) {
    Colour c = {1, 1, 1, 1};
    if (!contains(s, "rgba("))
        return c;
    float *parts = &c.r;
    s += 5;
    for (int i = 0; i < 4; i++) {
        while (*s == ' ')
            s++;
        parts[i] = number(s, i == 3 ? 1.0f : 255.0f) / (i == 3 ? 1.0f : 255.0f);
        while (*s && *s != ',' && *s != ')')
            s++;
        if (*s == ',')
            s++;
    }
    return c;
}

float lineWidth(GlyphFont &f, const char *s, int n) {
    float width = 0;
    for (int i = 0; i < n; i++) {
        if (s[i] == '|' && i + 7 < n && s[i + 7] == '|') {
            i += 7;
            continue;
        }
        u8 c = (u8)s[i];
        if (c >= 32 && c < 127)
            width += f.width[c];
    }
    return width;
}

void drawTextObject(Object &o, Rect rect, const char *overrideText = 0, float alphaScale = 1) {
    const char *s = overrideText ? overrideText : o.props.get("text");
    if (!*s)
        return;
    GlyphFont &f =
        *originalFont(o.props.get("font-face", "Vera"), (int)o.props.num("font-size", 12));
    Colour col = colour(o.props.get("colour"));
    col.a *= alphaScale;
    float ax = o.props.num("anchor-x", 0.5f), ay = o.props.num("anchor-y", 0.5f);
    int starts[64], lengths[64], lineCount = 0;
    int start = 0, lastSpace = -1;
    float width = 0;
    int n = lstrlenA(s);
    float maxWidth = maxf(1, rect.r - rect.l);
    for (int i = 0; i <= n && lineCount < 63; i++) {
        if (s[i] == '|' && i + 7 < n && s[i + 7] == '|') {
            i += 7;
            continue;
        }
        if (s[i] == ' ')
            lastSpace = i;
        u8 c = (u8)s[i];
        if (c >= 32 && c < 127)
            width += f.width[c];
        bool wrap = o.props.flag("multiline") && width > maxWidth && lastSpace >= start;
        if (!s[i] || s[i] == '\n' || wrap) {
            int end = wrap ? lastSpace : i;
            starts[lineCount] = start;
            lengths[lineCount++] = end - start;
            start = wrap ? end + 1 : i + 1;
            i = start - 1;
            lastSpace = -1;
            width = 0;
            if (!s[start] && end == n)
                break;
        }
    }
    if (!lineCount)
        return;
    float total = f.lineHeight * (float)lineCount;
    float y = rect.b + (rect.t - rect.b) * ay + (1 - ay) * total - f.lineHeight;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, f.texture);
    for (int l = 0; l < lineCount; l++) {
        const char *line = s + starts[l];
        int len = lengths[l];
        float w = lineWidth(f, line, len);
        float x = rect.l + ax * (rect.r - rect.l - w);
        glColor4f(col.r, col.g, col.b, col.a);
        for (int i = 0; i < len; i++) {
            if (line[i] == '|' && i + 7 < len && line[i + 7] == '|') {
                u32 rgb = hexColor(line + i + 1);
                glColor4f(((rgb >> 16) & 255) / 255.0f, ((rgb >> 8) & 255) / 255.0f,
                          (rgb & 255) / 255.0f, col.a);
                i += 7;
                continue;
            }
            if (line[i] == '|' && i + 1 < len && line[i + 1] == '|') {
                i++;
                continue;
            }
            u8 ch = (u8)line[i];
            if (ch < 32 || ch > 126)
                continue;
            float u = (ch % 16) * 64 / 1024.0f, vv = (ch / 16) * 64 / 512.0f;
            glBegin(GL_QUADS);
            glTexCoord2f(u, vv + 64 / 512.0f);
            glVertex2f(x - 1, y + f.lineHeight - 63);
            glTexCoord2f(u + 64 / 1024.0f, vv + 64 / 512.0f);
            glVertex2f(x + 63, y + f.lineHeight - 63);
            glTexCoord2f(u + 64 / 1024.0f, vv);
            glVertex2f(x + 63, y + f.lineHeight + 1);
            glTexCoord2f(u, vv);
            glVertex2f(x - 1, y + f.lineHeight + 1);
            glEnd();
            x += f.width[ch];
        }
        y -= f.lineHeight;
    }
}

const Props *buttonProps(Object &o) {
    const char *names[] = {"LevelButtonBehaviour",      "WorldButtonBehaviour",
                           "WorldLevelButtonBehaviour", "MusicToggleBehaviour",
                           "SFXToggleBehaviour",        "PauseResumeBehaviour",
                           "PauseRestartBehaviour",     "LevelCompleteRestartButton",
                           "LevelSelectButton",         "LevelCompleteContinueButton",
                           "QuitButtonBehaviour",       "LevelSelectMinimizeButton",
                           "WebLinkButtonBehaviour",    "TurboToggleBehaviour",
                           "ResetButtonBehaviour",      "GfxWindowModeButton",
                           "GfxResolutionButton",       "GfxApplyButton",
                           "ControlsKeyButton",         "HUDMenuBehaviour"};
    for (int i = 0; i < 20; i++) {
        const Props *p = o.behavior(names[i]);
        if (p)
            return p;
    }
    return 0;
}

// Load the authored menu scene into its own arena before resolving nodes and the playable background.
void loadUi(UiScene &s, u64 id) {
    destroyUiPhysics(s);
    if (&s == &page) {
        clearEffects(1);
        resetBurstRandomNative(1, timeGetTime());
    }
    if (!s.memory.data)
        s.memory.init(32 * 1024 * 1024);
    LevelInfo *info = levelById(id);
    if (!info)
        fatal("Missing original interface scene.");
    readLevelData(info->file, s.level, s.memory);
    repairFifthSelectorTint(s.level);
    repairFifthSelectorGeometry(s.level);
    s.nodes = s.memory.array<UiNode>(s.level.objectCount);
    s.age = 0;
    s.focus = -1;
    s.selectorLayer = -1;
    s.minimizePhase = 0;
    s.minimized = false;
    s.hasBean = false;
    s.playfieldCount = s.miniCoins = 0;
    s.miniDeath = 0;
    s.trampolineApex = 0;
    s.playfield = 0;
    s.cameraX = s.cameraY = 0;
    for (int i = 0; i < s.level.objectCount; i++) {
        Object &o = s.level.objects[i];
        if (o.base == 4) {
            Vec c = center(o.rect);
            s.cameraX = c.x - viewW * 0.5f;
            s.cameraY = c.y - viewH * 0.5f;
            break;
        }
    }
    nativeContinuousResetScene(s.level);
    nativeContinuousAdvanceScene(s.level, 1, 0, 0);
    s.targetX = s.homeX = s.cameraX;
    s.homeY = s.cameraY;
    s.menuLayers = s.memory.array<bool>(s.level.layerCount);
    for (int i = 0; i < s.level.objectCount; i++) {
        Object &o = s.level.objects[i];
        UiNode &node = s.nodes[i];
        node.object = &o;
        node.rect = o.rect;
        node.stage = -1;
        node.world = 0;
        node.hidden = !o.visible;
        node.disabled = false;
        if (o.base == 1 || o.base == 7 || o.base == 5)
            node.art = actor(o.klass);
        const Props *bp = buttonProps(o);
        if (bp) {
            u64 target = idnum(bp->get("level"));
            node.stage = stageById(target);
            if (node.stage >= 0)
                node.disabled = !selectorLevelUnlocked(target);
            node.world = (int)bp->num("world");
            if (node.world > 0)
                node.disabled = !worldOpen(node.world);
            if (bp->flag("focus-initial"))
                s.focus = i;
        }
        if (o.layer >= 0 && o.layer < s.level.layerCount &&
            (bp || o.has("LevelSelectController") || contains(o.props.get("name"), "Title")))
            s.menuLayers[o.layer] = true;
        if (o.has("LevelSelectController"))
            s.selectorLayer = o.layer;
        if (o.has("TomPlayerBehaviour")) {
            initializeEntity(s.bean, o, o.rect);
            s.beanSpawn = s.bean.state;
            s.trampolineApex = s.beanSpawn.y;
            s.hasBean = true;
        }
    }
}

void buildUiPlayfield(UiScene &s) {
    if (!s.hasBean)
        return;
    s.playfield = s.memory.array<Entity>(513);
    for (int i = 0; i < s.level.objectCount; i++) {
        Object &o = s.level.objects[i];
        Kind kind = kindOf(o);
        if ((kind == DECOR &&
             !(s.level.id == 137502285691ULL && o.has("WreckingBallControllerBehaviour"))) ||
            kind == PLAYER || buttonProps(o))
            continue;
        float w = o.base == 7 ? maxf(1, o.props.num("actlet-width", 32)) : o.rect.r - o.rect.l,
              h = o.base == 7 ? maxf(1, o.props.num("actlet-height", 32)) : o.rect.t - o.rect.b;
        if (w <= 0 || h <= 0)
            continue;
        for (float y = o.rect.b; y < o.rect.t - 0.5f; y += h)
            for (float x = o.rect.l; x < o.rect.r - 0.5f; x += w) {
                if (s.playfieldCount >= 512)
                    fatal("Too many selector objects.");
                initializeEntity(s.playfield[s.playfieldCount++], o,
                                 Rect{x, y, minf(o.rect.r, x + w), minf(o.rect.t, y + h)});
            }
    }
    if (s.level.id == 137502285691ULL)
        buildCreditsRuntime(s);
    else
        createUiPhysics(s);
}

void focusId(UiScene &s, u64 id) {
    for (int i = 0; i < s.level.objectCount; i++)
        if (s.nodes[i].object->id == id) {
            s.focus = i;
            return;
        }
}

int firstStageInWorld(int world) {
    for (int i = 0; i < stageCount; i++)
        if ((int)levels[stageList[i]].props.num("bean-world") == world)
            return i;
    return 0;
}

int displayWorld(int world) {
    return world == 4 ? 2 : world == 2 ? 3 : world == 3 ? 4 : world;
}

bool worldOpen(int world) {
    return debugMode || firstStageInWorld(world) <= progress.unlocked;
}

const u64 worldScenes[] = {8808768056724772154ULL, 12038123762473955738ULL, 71376317651ULL,
                           799282380888ULL, 669776786229ULL};

// Native badge callbacks use IsUnlocked -> Actor::SetVisible independently
// of their on/off/best artwork. Match the port's existing button unlock policy.
bool selectorLevelUnlocked(u64 level) {
    int stage = stageById(level);
    if (stage >= 0)
        return debugMode || stage <= progress.unlocked;
    for (int world = 0; world < 5; world++)
        if (worldScenes[world] == level)
            return worldOpen(world + 1);
    return false;
}

bool selectorAwardObject(const Object &object) {
    const char *name = object.props.get("name");
    return contains(name, "LevelSelect") &&
           (contains(name, "Diamond") || contains(name, "Shoe") || contains(name, "Salamander") ||
            eq(name, "LevelSelectStar"));
}

// Switch menu scene and focus together; screen-specific transitions keep their own return destination.
void navigate(u64 id) {
    worldSelectPointerNative = WorldSelectPointerNative{};
    titleAttractReset();
    introClose();
    int stage = stageById(id);
    if (stage >= 0) {
        if (debugMode || stage <= progress.unlocked) {
            stopSounds();
            startStage(stage, false);
        }
        return;
    }
    // The old stage no longer owns an overlay or a pending restore once a
    // standalone scene is opened. Its checkpoint stays available on disk.
    resetPauseInterfaceNative();
    respawnCancel(respawnTransition);
    deathTimer = deathAge = 0;
    resetEndingScene();
    loadUi(page, id);
    buildUiPlayfield(page);
    pointerHeld = false;
    pointerNode = -1;
    transitionTime = transitionDuration = 0.22f;
    const char *name = page.level.props.get("name");
    if (eq(name, "Title"))
        screen = TITLE;
    else if (eq(name, "World Select")) {
        screen = WORLD;
        makerDecorateWorldSelect();
        int w = (int)levels[stageList[currentStage]].props.num("bean-world", 1);
        selectedWorld = w;
        for (int i = 0; i < page.level.objectCount; i++)
            if (page.nodes[i].world == w) {
                page.focus = i;
                page.cameraX = page.targetX = center(page.nodes[i].rect).x - viewW * 0.5f;
                break;
            }
    } else if (contains(name, "Level Select - World")) {
        screen = SELECT;
        for (int i = 0; i < page.level.objectCount; i++) {
            UiNode &n = page.nodes[i];
            if (n.stage == currentStage) {
                page.focus = i;
                break;
            }
        }
        if (page.focus < 0)
            for (int i = 0; i < page.level.objectCount; i++)
                if (page.nodes[i].stage >= 0) {
                    page.focus = i;
                    break;
                }
        selectedWorld =
            (int)levels[stageList[page.nodes[page.focus].stage]].props.num("bean-world", 1);
    } else if (eq(name, "Options - Desktop"))
        screen = OPTIONS;
    else if (eq(name, "Options - Graphics"))
        screen = GRAPHICS;
    else if (eq(name, "Options - Keys"))
        screen = CONTROLS;
    else if (eq(name, "Credits"))
        screen = CREDITS;
    else if (contains(name, "Splash"))
        screen = SPLASH;
    else if (eq(name, "Cutscene - Ending"))
        screen = ENDING;
    else if (eq(name, "Cutscene - Movie"))
        screen = INTRO;
    if (screen == TITLE || screen == WORLD || screen == SELECT)
        playMusic(3080012186760326ULL);
    if (screen == ENDING) {
        u64 next = 12192614360109026946ULL;
        for (int i = 0; i < page.level.objectCount; i++)
            if (page.level.objects[i].has("CutsceneEndingController"))
                next = idnum(
                    page.level.objects[i].behavior("CutsceneEndingController")->get("next-level"));
        resetEndingCredits(page.cameraY + viewH * 0.5f, next);
        bindEndingCreditsScene(page);
        playMusic(idnum(page.level.props.get("music")));
    }
    if (screen == INTRO)
        introOpen(page.level);
}

// Settings replays return to their caller; a movie entered by other scene
// controllers keeps its authored destination. Title attract owns its return.
// Return to the recorded caller after the movie, including settings when Watch Intro launched playback.
void finishIntro(bool cancelled = false, bool natural = false) {
    u64 origin = introMovie.settingsOrigin, focus = introMovie.settingsFocus;
    u64 next = origin ? origin : cancelled ? 610822198339107ULL : introMovie.next;
    introMovie.settingsOrigin = introMovie.settingsFocus = 0;
    introClose();
    if (natural && !next)
        return;
    navigate(next ? next : 18006219643615701355ULL);
    if (origin) {
        focusId(page, focus);
        playMusic(3080012186760326ULL);
    } else if (cancelled)
        playMusic(3080012186760326ULL);
    if (natural)
        transitionTime = transitionDuration = .35f;
}

void initInterface() {
    loadUi(hudScene, 14897698531695279339ULL);
    bindPauseInterfaceNative();
    interfaceEnabled = true;
    const char *fonts[] = {"Vera.ttf", "VeraBd.ttf", "VeraMono.ttf", "VeraMoBd.ttf"};
    for (int i = 0; i < 4; i++) {
        char path[128];
        lstrcpyA(path, "data/Media/Font/");
        lstrcatA(path, fonts[i]);
        AddFontResourceExA(path, FR_PRIVATE, 0);
    }
    readBindings();
    UiPreferences pref;
    char path[MAX_PATH];
    savePath(path, "settings.dat");
    if (exactRead(path, &pref, sizeof(pref)) && pref.magic == 0x55535142 && pref.version == 1 &&
        pref.checksum == hashBytes(&pref, sizeof(pref) - 4) && pref.resolution >= 0 &&
        pref.resolution < WINDOW_RESOLUTION_COUNT) {
        resolutionChoice = pref.resolution;
        turbo = pref.turbo != 0;
        windowMode = !pref.full;
        if (fullscreen == windowMode)
            toggleFullscreen();
        if (windowMode)
            resizeWindow(windowResolutions[resolutionChoice].width,
                         windowResolutions[resolutionChoice].height);
    }
    navigate(9740832380187429308ULL);
}

void stageInterface(bool resume) {
    resetPauseInterfaceNative();
    readyOpening = readyNativeInitial(resume);
    readyPointerDown = false;
    readyTimer = resume ? 0 : 2.5f;
    hudScene.age = 0;
    hudScene.focus = -1;
    screen = PLAY;
    transitionTime = respawnBusy(respawnTransition) ? 0 : 0.2f;
}

void endGameInterface() {
    u64 next = endingScene.next ? endingScene.next : 137502285691ULL;
    navigate(next);
    endingCreditsFade = 2;
    endingCreditsFadeDuration = 2;
    transitionTime = 0;
}

void completeInterface() {
    hudScene.age = 0;
    hudScene.focus = -1;
    if (goalLaunch.age >= 2)
        focusId(hudScene, 8954898098276390340ULL);
}

float stageClearUiAge() {
    return complete && goalLaunch.active ? goalLaunch.age : 2.0f;
}

bool completionUiReady() {
    return screen != COMPLETE || stageClearUiAge() >= 2;
}

StageClearNativePose completionUiPose() {
    return stageClearNativePose(stageClearUiAge(),
                                StageClearNativeRect{256 - viewW * 0.5f, 192 - viewH * 0.5f,
                                                     256 + viewW * 0.5f, 192 + viewH * 0.5f},
                                StageClearNativeRect{115, 293, 435, 337},
                                StageClearNativeRect{30, 196, 182, 324});
}

void openPause(bool animate) {
    if (endingScene.active)
        return;
    if (screen != PAUSE)
        resetPauseInterfaceNative();
    if (!pauseTransitionNativeBegin(pauseInterfaceNative, animate))
        return;
    screen = PAUSE;
    hudScene.age = 0;
    hudScene.focus = -1;
    pointerHeld = false;
    pointerNode = -1;
    if (!animate)
        focusId(hudScene, pauseInitialFocusNative);
}

void resumePauseInterfaceNative() {
    if (screen != PAUSE)
        return;
    if (!pauseInterfaceNative.active)
        pauseTransitionNativeBegin(pauseInterfaceNative, false);
    pauseTransitionNativeResume(pauseInterfaceNative, true);
    hudScene.focus = -1;
    pointerHeld = false;
    pointerNode = -1;
}

void openSelect() {
    if (makerRun.active) {
        makerExitPlay();
        return;
    }
    if (currentStage < 0 || currentStage >= stageCount)
        currentStage = 0;
    u64 target = idnum(levels[stageList[currentStage]].props.get("level-select"));
    if (target) {
        navigate(target);
        return;
    }
    int w = (int)levels[stageList[currentStage]].props.num("bean-world", 1);
    navigate(worldScenes[(int)clamp((float)w - 1, 0, 4)]);
}

// Original scene drawing, dynamic captions and scene controllers follow below.
int actionIndex(const char *name) {
    const char *names[] = {"left",   "right",  "up",         "down",
                           "accept", "cancel", "checkpoint", "retry"};
    for (int i = 0; i < 8; i++)
        if (eq(name, names[i]))
            return i;
    return 0;
}

void keyName(int key, char *out) {
    if (!key) {
        lstrcpyA(out, "None");
        return;
    }
    const char *names[] = {"Left", "Right", "Up", "Down", "Space", "Escape", "Enter", "Backspace"};
    const int codes[] = {VK_LEFT,  VK_RIGHT,  VK_UP,     VK_DOWN,
                         VK_SPACE, VK_ESCAPE, VK_RETURN, VK_BACK};
    for (int i = 0; i < 8; i++)
        if (key == codes[i]) {
            lstrcpyA(out, names[i]);
            return;
        }
    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) {
        out[0] = (char)key;
        out[1] = 0;
        return;
    }
    wsprintfA(out, "Key %d", key);
}

bool stageBadge(int stage, int badge) {
    if (stage < 0 || stage >= stageCount || !progress.cleared[stage])
        return false;
    Props &p = levels[stageList[stage]].props;
    if (badge == 0)
        return progress.bestCoins[stage] >= (int)p.num("bean-max-coins");
    if (badge == 1 || badge == 4) {
        int threshold = (int)p.num(badge == 4 ? "bean-jump-best" : "bean-jump-par", 999);
        return threshold > 0 && threshold < 999 && progress.bestJumps[stage] <= threshold;
    }
    if (badge == 2)
        return progress.axolotls[stage] != 0;
    return stageBadge(stage, 0) && stageBadge(stage, 1) && stageBadge(stage, 2);
}

bool badgeForLevel(u64 id, int badge) {
    int stage = stageById(id);
    if (stage >= 0)
        return stageBadge(stage, badge);
    int world = 0;
    for (int i = 0; i < 5; i++)
        if (worldScenes[i] == id)
            world = i + 1;
    if (!world)
        return false;
    for (int i = 0; i < stageCount; i++)
        if ((int)levels[stageList[i]].props.num("bean-world") == world && !stageBadge(i, badge))
            return false;
    return true;
}

// Native OnShoe prioritizes the authored BEST award over the ordinary PAR award.
const char *shoeAnimationForLevel(u64 level) {
    return badgeForLevel(level, 4) ? "best" : badgeForLevel(level, 1) ? "on" : "off";
}

bool sceneLayerVisible(UiScene &s, int layer) {
    if (&s == &hudScene)
        return layer == 0 || (layer == 1 && screen == PAUSE) ||
               (layer == 2 && screen == COMPLETE) ||
               (layer == 3 && screen == PLAY && readyTimer > 0);
    if (screen == CONTROLS && layer == 2)
        return rebindAction >= 0;
    if (screen == SELECT && s.minimized && s.menuLayers[layer]) {
        if (layer == s.selectorLayer)
            return s.minimizePhase < 1;
        bool controls = false;
        for (int i = 0; i < s.level.objectCount; i++)
            if (s.level.objects[i].layer == layer &&
                s.level.objects[i].has("LevelSelectMinimizeButton"))
                controls = true;
        if (!controls)
            return false;
    }
    return s.level.layers[layer].flags[1] != 0;
}

bool visibleNode(UiScene &scene, UiNode &n) {
    Object &o = *n.object;
    if (n.hidden)
        return false;
    const char *name = o.props.get("name");
    if (!sceneLayerVisible(scene, o.layer))
        return false;
    if (selectorAwardObject(o) && !selectorLevelUnlocked(idnum(o.props.get("level"))))
        return false;
    if (&scene == &hudScene) {
        // FinalBossBehaviour::OnActivate calls HUDController::SetBossMode
        // (0x1e07e4 -> 0x224ef8). It hides every top-left anchored actor,
        // including label backings, before combat and throughout the rescue.
        if (o.props.flag("anchor-left") && o.props.flag("anchor-top")) {
            for (int i = 0; i < entityCount; i++)
                if (entities[i].kind == BOSS)
                    return false;
        }
        if (o.id == 4267930709906329387ULL || o.id == 9184915223959083603ULL ||
            o.id == 13422676027307937412ULL)
            return false;
        if (o.id == 9309191080070077ULL)
            return false;
        if (o.id == 10379613525228204374ULL || o.id == 17592391182038304666ULL ||
            o.id == 2983488999281832068ULL) {
            bool active = false;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].kind == SWITCH && entities[i].state.aux > 0)
                    active = true;
            if (!active)
                return false;
        }
        // Both overlapping halves form each authored label backing. Hiding
        // the right halves shortened the black capsules beneath the text.
    }
    if (eq(name, "LevelSelectStar"))
        return badgeForLevel(idnum(o.props.get("level")), 3);
    return true;
}

// TitleScreenController::OnActivate @0x306fd8..0x30703c binds "bean"
// with period2.5, amplitude4 and phase0. OnUpdate @0x305fb8..0x305fec
// adds amplitude*sin((age+phase)*twoPi/period) to its authored position.
bool titleBeanNative(const Level &level, u64 object) {
    return titleNativeTrackFor(level, object) == 0;
}

float titleBeanBobNative(float age) {
    return 4 * bqSin(age * 6.2831854820251465f / 2.5f);
}

Rect uiLayerViewportRect(UiScene &scene, int layer) {
    Vec camera = layerCameraPosition(scene.level, layer, scene.cameraX, scene.cameraY);
    return Rect{camera.x - viewW * 0.5f, camera.y - viewH * 0.5f,
                camera.x + viewW * 0.5f, camera.y + viewH * 0.5f};
}

Rect visualRect(UiScene &scene, UiNode &n) {
    Rect r = n.rect;
    Object &o = *n.object;
    const char *name = o.props.get("name");
    r = selectorWaterSurfaceVisualRect(scene.level, o, r);
    if (&scene == &hudScene) {
        Layer &layer = scene.level.layers[o.layer];
        float dx = (layer.size.x - viewW) * 0.5f, dy = (layer.size.y - viewH) * 0.5f;
        r = shifted(r,
                    o.props.flag("anchor-left")    ? dx
                    : o.props.flag("anchor-right") ? -dx
                                                   : 0,
                    o.props.flag("anchor-bottom") ? dy
                    : o.props.flag("anchor-top")  ? -dy
                                                  : 0);
    }
    if (&scene == &hudScene && screen == PAUSE && o.layer == pauseLayerNative &&
        o.id != pauseBackgroundNative) {
        Vec offset = pauseTransitionNativeOffset(pauseInterfaceNative, viewW, viewH);
        r = shifted(r, offset.x, offset.y);
    }
    if (&scene == &hudScene && screen == PLAY && o.layer == 3 && readyTimer > 0) {
        Vec c = center(r);
        Rect view = uiLayerViewportRect(scene, o.layer);
        Vec pose = readyNativePosition(readyOpening.age, c, view, o.id == 17323956314806549368ULL);
        if (o.id == 17109890148369247723ULL || o.id == 11001010908721541493ULL) {
            // These solid, translucent panels are viewport shades. Extend each
            // once, without overlapping tiles that would darken their seams.
            // Keep the inner edges and apply the original exit displacement
            // after extension so tall windows retain the same opening gap.
            r.l = minf(r.l, view.l);
            r.r = maxf(r.r, view.r);
            if (o.id == 17109890148369247723ULL)
                r.t = maxf(r.t, view.t);
            else
                r.b = minf(r.b, view.b);
        }
        r = shifted(r, pose.x - c.x, pose.y - c.y);
    }
    if (&scene == &hudScene && screen == COMPLETE && o.layer == 2) {
        StageClearNativePose pose = completionUiPose();
        if (o.id == 167178695708687765ULL) {
            StageClearNativeRect q = pose.stageClearRect;
            r = Rect{q.l, q.b - pose.remainingUiOffsetY, q.r, q.t - pose.remainingUiOffsetY};
        }
        if (o.id == 14545820365738952861ULL) {
            StageClearNativeRect q = pose.beanRect;
            r = Rect{q.l, q.b - pose.remainingUiOffsetY, q.r, q.t - pose.remainingUiOffsetY};
        }
    }
    if (&scene == &page && screen == TITLE && eq(name, "Title Screen Quit"))
        r = shifted(r, scene.cameraX, 0);
    if (o.has("BackgroundBehaviour")) {
        Vec c = layerCameraPosition(scene.level, o.layer, scene.cameraX, scene.cameraY);
        r = backgroundRect(o, r, scene.age, c.x - viewW * 0.5f, c.y - viewH * 0.5f);
    }
    if (screen == TITLE && &scene == &page) {
        Vec offset =
            titleNativeOffset(titleNativeTrackFor(scene.level, o.id), scene.age, viewW, viewH);
        r = shifted(r, offset.x, offset.y);
    }
    if (screen == ENDING && &scene == &page &&
        o.id == endingCreditsScene.fadeIn) {
        // The ending controller fades this uniform background shade, separately
        // from its illustration. Cover the current layer viewport at every
        // scroll position and resize without stretching the illustration.
        Rect view = uiLayerViewportRect(scene, o.layer);
        r = Rect{minf(r.l, view.l), minf(r.b, view.b),
                 maxf(r.r, view.r), maxf(r.t, view.t)};
    }
    if (screen == ENDING && &scene == &page &&
        (o.id == endingCreditsScene.emilio || o.id == endingCreditsScene.maria)) {
        EndingCreditsPresentation pose =
            endingCreditsPresentation(endingCreditsScene.input, scene.age);
        Vec c = center(n.rect);
        r = shifted(r, (o.id == endingCreditsScene.emilio ? pose.emilioX : pose.mariaX) - c.x,
                    (o.id == endingCreditsScene.emilio ? pose.emilioY : pose.mariaY) - c.y);
    }
    return r;
}

// LevelSelectController::OnUpdate @0x25cd00 advances its own layer's
// minimize progress at four units/sec, then uses p*p*(3-2*p) to slide it
// down by twice the camera height. Other playfield layers keep their origin.
float selectorLayerSlide(const UiScene &s) {
    float p = s.minimizePhase;
    return -2 * viewH * p * p * (3 - 2 * p);
}

void layerOffset(UiScene &s, int layer, float *x, float *y) {
    bool menu = screen == SELECT && s.hasBean && s.menuLayers[layer];
    Vec t =
        layerTranslation(s.level, layer, menu ? s.homeX : s.cameraX, menu ? s.homeY : s.cameraY);
    if (screen == SELECT && layer == s.selectorLayer)
        t.y += selectorLayerSlide(s);
    if (&s == &hudScene && screen == COMPLETE && layer == 2)
        t.y += completionUiPose().remainingUiOffsetY;
    *x = t.x;
    *y = t.y;
}

void originalCaption(UiScene &s, UiNode &n, Rect rect) {
    Object &o = *n.object;
    const char *source = windowsPortCreditCaption(s.level.id, o.id, o.props.get("text"));
    char value[160];
    bool replace = false;
    if (o.id == 6373888218169984ULL) {
        lstrcpyA(value, "Space / Up to Continue > > >");
        replace = true;
    }
    if (&s == &hudScene) {
        if (o.id == 8799797905732598206ULL) {
            wsprintfA(value, "%d / %d", coins, (int)current.props.num("bean-max-coins"));
            replace = true;
        }
        if (o.id == 8775559588655153142ULL) {
            wsprintfA(value, "%d / |FFFF00|%d", jumps, stageJumpTarget);
            replace = true;
        }
        if (o.id == 17592391182038304666ULL) {
            float time = 0;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].kind == SWITCH)
                    time = maxf(time, entities[i].state.aux);
            wsprintfA(value, "%d.%02d", (int)time, (int)(time * 100) % 100);
            replace = true;
        }
        if (o.id == 17529542748944344776ULL) {
            lstrcpynA(value, current.props.get("bean-level-name"), sizeof(value));
            replace = true;
        }
        if (o.id == 11477711412535822434ULL) {
            wsprintfA(value, "STAGE %d-%d", displayWorld((int)current.props.num("bean-world")),
                      (int)current.props.num("bean-level"));
            replace = true;
        }
        if (o.id == 6850205356280605228ULL) {
            wsprintfA(value, "%d", coins);
            replace = true;
        }
        if (o.id == 13476334470372182008ULL) {
            wsprintfA(value, "%d", (int)current.props.num("bean-max-coins"));
            replace = true;
        }
        if (o.id == 6288522777881954502ULL) {
            wsprintfA(value, "%d", jumps);
            replace = true;
        }
        if (o.id == 10498441872698914575ULL) {
            wsprintfA(value, "%d", jumpTargetForStage(currentStage));
            replace = true;
        }
        if (o.id == 8364913218960181251ULL) {
            lstrcpyA(value,
                     progress.cleared[currentStage] && progress.bestJumps[currentStage] <=
                                                           (int)current.props.num("bean-jump-par")
                         ? "BEST"
                         : "PAR");
            replace = true;
        }
        if (makerHudCaption(o.id, value))
            replace = true;
    }
    if (screen == CONTROLS) {
        for (int i = 0; i < s.level.objectCount; i++) {
            Object &button = s.level.objects[i];
            const Props *b = button.behavior("ControlsKeyButton");
            if (b && idnum(b->get("caption")) == o.id) {
                int action = actionIndex(b->get("key")), column = (int)b->num("index");
                keyName(column ? secondaryKey[action] : primaryKey[action], value);
                replace = true;
                break;
            }
        }
    }
    if (screen == GRAPHICS) {
        if (o.id == 7067643741759160ULL) {
            lstrcpyA(value, windowResolutions[resolutionChoice].caption);
            replace = true;
        }
        if (o.id == 3099780739837707ULL) {
            lstrcpyA(value, windowMode ? "Windowed" : "Fullscreen");
            replace = true;
        }
    }
    float alpha = 1;
    if (screen == ENDING && &s == &page) {
        EndingCreditsPresentation pose = endingCreditsPresentation(endingCreditsScene.input, s.age);
        if (o.id == 6373888218169984ULL)
            alpha = endingCreditsPromptAlpha();
        else if (o.id == endingCreditsScene.emilioText || o.id == endingCreditsScene.mariaText)
            alpha = pose.nameAlpha;
        else if (o.id == endingCreditsScene.endText)
            alpha = pose.endAlpha;
    }
    drawTextObject(o, rect, replace ? value : source, alpha);
}

void drawUiNode(UiScene &s, UiNode &n, int index) {
    if (!visibleNode(s, n))
        return;
    if (makerDrawWorldNode(s, n))
        return;
    Object &o = *n.object;
    Rect r = visualRect(s, n);
    if (o.base == 3) {
        originalCaption(s, n, r);
        return;
    }
    if (o.base != 1 && o.base != 7 && o.base != 5)
        return;
    if (drawCreditsRuntimeNode(s, o))
        return;
    if (o.has("TomPlayerBehaviour") && s.hasBean) {
        if (s.miniDeath <= 0)
            drawEntity(s.bean, &s.level, s.cameraX, s.cameraY, s.age);
        return;
    }
    if (s.hasBean) {
        bool handled = false;
        for (int i = 0; i < s.playfieldCount; i++)
            if (s.playfield[i].source == &o) {
                drawEntity(s.playfield[i], &s.level, s.cameraX, s.cameraY, s.age);
                handled = true;
            }
        if (handled)
            return;
    }
    if (o.base == 5) {
        drawParticles(o, n.art, r, s.age, s.selectorLayer >= 0 ? 0.5f : 1.0f);
        return;
    }
    const char *anim = o.props.get("default-animation");
    float alpha = 1, angle = o.angle, tintR = 1, tintG = 1, tintB = 1;
    bool flip = false;
    float animationTime = s.age;
    if (buttonProps(o))
        anim = n.disabled ? "disabled" : "up";
    if (o.has("MusicToggleBehaviour"))
        anim = musicEnabled ? "up-on" : "up-off";
    if (o.has("SFXToggleBehaviour"))
        anim = soundEnabled ? "up-on" : "up-off";
    if (o.has("TurboToggleBehaviour"))
        anim = turbo ? "up-on" : "up-off";
    const char *name = o.props.get("name");
    u64 level = idnum(o.props.get("level"));
    // The controller keeps its awarded stars rotating at -135 degrees/sec
    // (OnUpdate @0x25c30c-0x25c41c), independently of their sprite animation.
    if (screen == SELECT && eq(name, "LevelSelectStar"))
        angle -= 135 * s.age;
    if (level && contains(name, "LevelSelect")) {
        if (contains(name, "Diamond"))
            anim = badgeForLevel(level, 0) ? "on" : "off";
        if (contains(name, "Shoe"))
            anim = shoeAnimationForLevel(level);
        if (contains(name, "Salamander"))
            anim = badgeForLevel(level, 2) ? "on" : "off";
    }
    if (o.has("WorldButtonBehaviour") || o.has("WorldLevelButtonBehaviour"))
        anim = worldOpen(n.world) ? "up" : "disabled";
    if (&s == &hudScene && o.id == 12094503816206857335ULL) {
        anim = salamander ? "on" : "off";
    }
    // Requested improvement to the original result panel: award this run's
    // gold shoe at the same authored threshold used by the selector badge.
    if (&s == &hudScene && screen == COMPLETE && o.id == 4449368542626346951ULL) {
        int best = (int)current.props.num("bean-jump-best", 999);
        if (best > 0 && best < 999 && jumps <= best) {
            float side = minf(r.r - r.l, r.t - r.b);
            Vec c = center(r);
            drawActor(actor(7897082502132702692ULL),
                      Rect{c.x - side * .5f, c.y - side * .5f, c.x + side * .5f, c.y + side * .5f},
                      "best", animationTime, angle);
            return;
        }
    }
    if (pointerHeld && !pointerDragged && pointerScene == s.level.id && pointerNode == index &&
        buttonProps(o) && !n.disabled) {
        if (o.has("MusicToggleBehaviour"))
            anim = musicEnabled ? "down-on" : "down-off";
        else if (o.has("SFXToggleBehaviour"))
            anim = soundEnabled ? "down-on" : "down-off";
        else
            anim = "down";
    }
    if (screen == ENDING && &s == &page) {
        EndingCreditsPresentation pose = endingCreditsPresentation(endingCreditsScene.input, s.age);
        if (o.id == endingCreditsScene.tint) {
            tintR = pose.tintR;
            tintG = pose.tintG;
            tintB = pose.tintB;
            alpha = pose.tintA;
        }
        if (o.id == endingCreditsScene.fadeIn)
            alpha = pose.backdropA;
        if (o.id == endingCreditsScene.emilio || o.id == endingCreditsScene.maria) {
            anim = pose.characterAnimation;
            animationTime = eq(anim, "walking") ? pose.walkingAge : pose.defaultAge;
            flip = o.id == endingCreditsScene.maria && pose.mariaScaleX < 0;
        }
    }
    if (o.base == 7) {
        float w = maxf(1, o.props.num("actlet-width", 32)),
              h = maxf(1, o.props.num("actlet-height", 32));
        for (float y = r.b; y < r.t - 0.5f; y += h)
            for (float x = r.l; x < r.r - 0.5f; x += w)
                drawActor(n.art, Rect{x, y, minf(x + w, r.r), minf(y + h, r.t)}, anim,
                          animationTime, angle, flip, -1, alpha, ANIMATION_LOOP, 1, false, tintR,
                          tintG, tintB);
    } else
        drawActor(n.art, r, anim, animationTime, angle, flip, -1, alpha, ANIMATION_LOOP, 1, false,
                  tintR, tintG, tintB);
}

Rect focusRect(UiScene &s, UiNode &n) {
    Rect r = visualRect(s, n);
    float w = n.object->props.num("focus-width", r.r - r.l),
          h = n.object->props.num("focus-height", r.t - r.b);
    Vec c = center(r);
    float dx, dy;
    layerOffset(s, n.object->layer, &dx, &dy);
    return Rect{c.x - w * 0.5f - 3 + dx, c.y - h * 0.5f - 3 + dy, c.x + w * 0.5f + 3 + dx,
                c.y + h * 0.5f + 3 + dy};
}

void drawFocus(UiScene &s) {
    if (s.focus < 0 || s.minimized || s.focus >= s.level.objectCount)
        return;
    UiNode &n = s.nodes[s.focus];
    if (!buttonProps(*n.object) || !visibleNode(s, n))
        return;
    Rect r = focusRect(s, n);
    for (int pass = 0; pass < 2; pass++) {
        float width = pass ? 0.75f : 2.0f, c = pass ? 1.0f : 0.0f;
        const Vec corners[] = {{r.l, r.b}, {r.r, r.b}, {r.r, r.t}, {r.l, r.t}};
        for (int i = 0; i < 4; i++) {
            float x = corners[i].x, y = corners[i].y, sx = i == 0 || i == 3 ? 1.0f : -1.0f,
                  sy = i < 2 ? 1.0f : -1.0f;
            box(minf(x, x + sx * 5) - width * 0.5f, y - width * 0.5f, 5 + width, width, c, c, c);
            box(x - width * 0.5f, minf(y, y + sy * 5) - width * 0.5f, width, 5 + width, c, c, c);
        }
    }
}

void drawOriginalScene(UiScene &s, bool clear = true) {
    // Application::OnRender 0x356de8 clears transparent black; the level's
    // background-colour is an editor property, not a runtime white backdrop.
    if (!s.level.layerCount)
        return;
    if (clear)
        box(0, 0, viewW, viewH, 0, 0, 0);
    for (int layer = 0; layer < s.level.layerCount; layer++) {
        if (!sceneLayerVisible(s, layer))
            continue;
        float x, y;
        layerOffset(s, layer, &x, &y);
        Rect visibleBounds = {-x, -y, viewW - x, viewH - y};
        glPushMatrix();
        glTranslatef(x, y, 0);
        int last = -2147483647;
        for (;;) {
            int next = 2147483647;
            for (int i = 0; i < s.level.tileCount; i++)
                if (s.level.tiles[i].layer == layer && s.level.tiles[i].z > last &&
                    s.level.tiles[i].z < next)
                    next = s.level.tiles[i].z;
            for (int i = 0; i < s.level.objectCount; i++) {
                Object &o = s.level.objects[i];
                if (o.layer == layer && o.z > last && o.z < next)
                    next = o.z;
            }
            if (next == 2147483647)
                break;
            for (int i = 0; i < s.level.tileCount; i++)
                if (s.level.tiles[i].layer == layer && s.level.tiles[i].z == next)
                    drawTile(s.level.tiles[i], &s.level, &visibleBounds);
            for (int i = 0; i < s.level.objectCount; i++)
                if (s.level.objects[i].layer == layer && s.level.objects[i].z == next)
                    drawUiNode(s, s.nodes[i], i);
            last = next;
        }
        if (&s == &page)
            drawEffects(1, layer);
        glPopMatrix();
    }
    drawFocus(s);
}

void render() {
    glViewport(0, 0, clientW, clientH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    nativeViewportProjection();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (screen == PLAY || screen == PAUSE || screen == COMPLETE) {
        drawWorld();
        if (screen == PAUSE)
            box(0, 0, viewW, viewH, 0, 0, 0, 0.45f);
        if (!endingScene.active)
            drawOriginalScene(hudScene, false);
        makerRenderPlayOverlay();
        if (saveFailed && noticeTimer > 0)
            centered(12, notice, 10);
    } else if (screen == INTRO)
        drawIntroStatus();
    else if (screen == MAKER_EDIT || screen == MAKER_BROWSE)
        makerRender();
    else
        drawOriginalScene(page);
    if (endingScene.active && endingScene.age > 8)
        box(0, 0, viewW, viewH, 1, 1, 1, clamp((endingScene.age - 8) * 0.5f, 0, 1));
    if (screen == ENDING && endingCredits.exiting)
        box(0, 0, viewW, viewH, 1, 1, 1, clamp(endingCredits.exitAge * 0.5f, 0, 1));
    if (endingCreditsFade > 0)
        box(0, 0, viewW, viewH, 1, 1, 1,
            clamp(endingCreditsFade / maxf(0.001f, endingCreditsFadeDuration), 0, 1));
    if (transitionTime > 0)
        box(0, 0, viewW, viewH, 0, 0, 0, transitionTime / maxf(0.001f, transitionDuration));
    if (respawnAlpha(respawnTransition) > 0)
        box(0, 0, viewW, viewH, 0, 0, 0, respawnAlpha(respawnTransition));
    if (!introActive() && titleAttract.black > 0)
        box(0, 0, viewW, viewH, 0, 0, 0, clamp(titleAttract.black, 0, 1));
    SwapBuffers(gameDC);
}

// Selector scenes contain ordinary playfield objects too. Their authored
// behaviours keep running while the level buttons cover the miniature level.
// OscillatingBehaviour::GetNextPosition (0x28df40) folds distance into a
// triangular wave; preserve the overshoot when a step crosses an endpoint.
void burstUiCloud(Entity &e) {
    const Props *p = e.source->behavior("CrumblingPlatformBehaviour");
    e.state.active = 2;
    e.state.timer = 0;
    emitEffect(idnum(p->get("effect")), e.state.x, e.state.y, e.layer, (e.local.r - e.local.l) / 32,
               1);
    sound("james-cloud");
}

void tickUiPlayfield(UiScene &s, float dt, bool carryBean = true) {
    if (dt <= 0)
        return;
    for (int i = 0; i < s.playfieldCount; i++) {
        Entity &e = s.playfield[i];
        State &state = e.state;
        if (state.gone) {
            advanceKillableDeathNative(e, dt, 1);
            continue;
        }
        if (!proximityControlled(e) || state.active || nativeActorAnimateWhilePaused(e)) {
            advanceJumperAnimationNative(e, dt);
            advanceFishAnimationNative(e, dt);
        }
        if (e.kind == PERCH)
            updateHelperPerchNative(e);
        if (e.kind == SPIKE)
            advanceSpikeAnimation(e, dt);
        else
            state.timer += dt;
        if (e.move)
            selectNativeMotionSpeed(*e.move, nativeMotionPhysicsFPS(dt),
                                    s.oscillator[i].effectiveSpeed, s.oscillator[i].selected);
        if (e.source->has("DiagonalBehaviour"))
            selectNativeMotionSpeed(*e.source->behavior("DiagonalBehaviour"),
                                    nativeMotionPhysicsFPS(dt), s.diagonal[i].effectiveSpeed,
                                    s.diagonal[i].selected);
        if (proximityControlled(e)) {
            if (!state.active) {
                if (!(nativeActorAnimateWhilePaused(e) && nativeActorPauseAnimationClockSafe(e)))
                    state.timer -= dt;
                continue;
            }
        }
        if (!e.walk && !e.rotate && !e.source->has("EnemyJumperBehaviour") &&
            !nativeFreeBodyGravity(e) && e.dynamic && e.physics &&
            e.physics->flag("is-gravitated", true))
            state.vy += nativeWorldGravity * dt;
        if (e.source->has("DiagonalBehaviour"))
            advanceNativeDiagonalState(e, s.diagonal[i], nativeMotionPhysicsFPS(dt));
        if (e.move && !carryBean)
            advanceNativeOscillatorState(e, s.oscillator[i], dt, nativeMotionPhysicsFPS(dt));
        else if (e.move) {
            float norm = e.moveX * e.moveX + e.moveY * e.moveY;
            if (norm > 0.01f && e.hi > e.lo && e.speed > 0) {
                float oldX = state.x, oldY = state.y;
                Rect old = hitRect(e), bean = hitRect(s.bean);
                bool carry = carryBean && s.miniDeath <= 0 && s.bean.state.vy <= 0 &&
                             absf(bean.b - old.t) < 1.1f && bean.r > old.l && bean.l < old.r;
                float distance =
                    ((state.x - e.originX) * e.moveX + (state.y - e.originY) * e.moveY) / norm;
                float span = e.hi - e.lo, cycle = span * 2;
                float phase = state.aux < 0 ? cycle - (distance - e.lo) : distance - e.lo;
                phase += e.speed * dt;
                phase -= (int)(phase / cycle) * cycle;
                if (phase < 0)
                    phase += cycle;
                if (phase > span) {
                    distance = e.hi - (phase - span);
                    state.aux = -1;
                } else {
                    distance = e.lo + phase;
                    state.aux = 1;
                }
                state.x = e.originX + e.moveX * distance;
                state.y = e.originY + e.moveY * distance;
                state.vx = (state.x - oldX) / dt;
                state.vy = (state.y - oldY) / dt;
                if (carry) {
                    s.bean.state.x += state.x - oldX;
                    s.bean.state.y += state.y - oldY;
                }
            }
        }
        if (e.orbit && state.active)
            positionNativeOrbit(e);
        if (e.crumble && state.active == 1)
            burstUiCloud(e);
        else if (e.crumble && state.active == 2 &&
                 state.timer - dt >
                     e.source->behavior("CrumblingPlatformBehaviour")->num("Time", 3)) {
            state.active = 0;
            state.timer = 0;
        }
    }
}

#include "credits_runtime_native.h"

// Advance UI presentation and selector gameplay; campaign physics remains owned by the campaign tick.
void tickInterface(float dt) {
    if (titleAttractBusy())
        return;
    if (screen == MAKER_EDIT || screen == MAKER_BROWSE) {
        if (transitionTime > 0)
            transitionTime = maxf(0, transitionTime - dt);
        makerTick(dt);
        return;
    }
    if (screen == PAUSE && pauseInterfaceNative.active) {
        int event = pauseTransitionNativeAdvance(pauseInterfaceNative, dt);
        if (event == PAUSE_NATIVE_FOCUS)
            focusId(hudScene, pauseInitialFocusNative);
        if (event == PAUSE_NATIVE_RESUMED) {
            screen = PLAY;
            hudScene.focus = -1;
            pointerHeld = false;
            pointerNode = -1;
        }
    }
    if (endingCreditsFade > 0)
        endingCreditsFade = maxf(0, endingCreditsFade - dt);
    if (transitionTime > 0)
        transitionTime = maxf(0, transitionTime - dt);
    hudScene.age += dt;
    if (screen == PLAY || screen == PAUSE || screen == COMPLETE)
        return;
    page.age += dt;
    nativeContinuousAdvanceScene(page.level, page.selectorLayer >= 0 ? 0.5f : 1.0f, page.age, dt);
    if (screen == SELECT)
        page.minimizePhase = approach(page.minimizePhase, page.minimized ? 1.0f : 0.0f, 4 * dt);
    if (screen == WORLD) {
        if (page.focus >= 0 && page.nodes[page.focus].world > 0)
            selectedWorld = page.nodes[page.focus].world;
        if (!pointerHeld) {
            worldSelectPointerNative.down = worldSelectPointerNative.justDown =
                worldSelectPointerNative.dragged = false;
        }
        float drag = worldSelectDragNative(worldSelectPointerNative, dt, screenScale);
        pointerDragged = worldSelectPointerNative.dragged;
        page.targetX = page.level.size.x * .5f +
                       worldSelectTargetNative(worldSelectSlot(selectedWorld), drag) - viewW * .5f;
        page.cameraX = approach(page.cameraX, page.targetX, 2048 * dt);
    }
    if (page.hasBean && screen == SELECT) {
        tickEffects(dt, 1);
        tickUiPlayfield(page, dt, false);
        Entity &e = page.bean;
        const Props &p = *e.physics;
        if (page.miniDeath > 0) {
            page.miniDeath -= dt;
            if (page.miniDeath <= 0) {
                e.state = page.beanSpawn;
                page.trampolineApex = page.beanSpawn.y;
                resetUiPhysics(page);
            }
        } else {
            bool left = page.minimized && held(0), right = page.minimized && held(1);
            if (left != right)
                e.state.aux = left ? -1.0f : 1.0f;
            e.state.vx = nativeTomHorizontalVelocity(
                e.state.vx, left != right ? (left ? -1 : 1) : 0, p.num("max-velocity", 100),
                p.num("move-acceleration", 250), p.num("stop-acceleration", 250), dt);
            stepUiPhysics(page, dt);
            for (int i = 0; i < page.playfieldCount; i++)
                if (page.playfield[i].crumble && page.playfield[i].state.active == 1)
                    burstUiCloud(page.playfield[i]);
            bool dead = e.state.y < -40 || page.contacts->spikeBeanPending;
            for (int i = 0; i < page.playfieldCount; i++) {
                Entity &item = page.playfield[i];
                if (item.state.gone)
                    continue;
                if (!overlaps(hitRect(e), hitRect(item)))
                    continue;
                if (item.kind == ENEMY)
                    dead = true;
            }
            if (dead) {
                page.miniDeath = deathDuration;
                resetUiPhysics(page);
                emitEffect(idnum(p.get("explosion-actor")), e.state.x, e.state.y, e.layer, 0.5f, 1);
                sound("bean-die");
            }
        }
        // Selector scenes contain an authored fixed camera. Minimizing the
        // buttons enables input; it does not turn this into a follow camera.
        page.cameraX = page.homeX;
        page.cameraY = page.homeY;
    }
    if (screen == SPLASH) {
        for (int i = 0; i < page.level.objectCount; i++) {
            const Props *p = page.level.objects[i].behavior("LevelChangeTimerController");
            if (p && page.age >= p->num("time", 1)) {
                navigate(idnum(p->get("next-level")));
                return;
            }
        }
    }
    // Only actual movie completion triggers the authored scene transition.
    if (screen == INTRO && introMovie.status == INTRO_FINISHED) {
        finishIntro(false, true);
        return;
    }
    if (screen == ENDING) {
        tickCreditsRuntime(page, dt);
        page.cameraY = endingCreditsCameraY(page.age) - viewH * 0.5f;
        if (advanceEndingCredits(page.age, dt)) {
            u64 next = endingCredits.next;
            navigate(next);
            endingCreditsFade = endingCreditsFadeDuration = 0.349999994f;
            transitionTime = 0;
        }
    }
}

// Dispatch authored button behavior centrally so keyboard and pointer activation follow the same path.
void activate(UiScene &scene, int index) {
    if ((&scene == &hudScene && (!completionUiReady() || !pauseMenuInputReady())) || index < 0 ||
        index >= scene.level.objectCount)
        return;
    UiNode &node = scene.nodes[index];
    Object &o = *node.object;
    if (node.disabled)
        return;
    if (makerWorldNode(o)) {
        makerOpenBrowser();
        return;
    }
    if (o.has("WorldButtonBehaviour") || o.has("WorldLevelButtonBehaviour")) {
        if (!worldOpen(node.world))
            return;
        const Props *p = o.has("WorldButtonBehaviour") ? o.behavior("WorldButtonBehaviour")
                                                       : o.behavior("WorldLevelButtonBehaviour");
        navigate(idnum(p->get("level")));
        transitionTime = transitionDuration = .35f;
        return;
    }
    if (o.has("LevelButtonBehaviour")) {
        const Props *p = o.behavior("LevelButtonBehaviour");
        const char *id = p->get("level-desktop");
        if (!*id)
            id = p->get("level");
        u64 origin = screen == OPTIONS ? scene.level.id : 0, focus = o.id;
        navigate(idnum(id));
        if (origin && screen == INTRO) {
            introMovie.settingsOrigin = origin;
            introMovie.settingsFocus = focus;
        }
        return;
    }
    if (o.has("MusicToggleBehaviour")) {
        musicEnabled = !musicEnabled;
        if (musicEnabled)
            playMusic(screen == PLAY || screen == PAUSE ? stageMusic() : 3080012186760326ULL);
        else
            stopMusic();
        writeProgress();
        return;
    }
    if (o.has("SFXToggleBehaviour")) {
        soundEnabled = !soundEnabled;
        if (soundEnabled)
            sound("coin");
        else
            stopSounds();
        writeProgress();
        return;
    }
    if (o.has("PauseResumeBehaviour")) {
        resumePauseInterfaceNative();
        return;
    }
    if (o.has("HUDMenuBehaviour")) {
        openPause();
        return;
    }
    if (o.has("PauseRestartBehaviour")) {
        resetPauseInterfaceNative();
        screen = PLAY;
        respawnBegin(respawnTransition, RESPAWN_CHECKPOINT);
        return;
    }
    if (o.has("LevelCompleteRestartButton")) {
        screen = PLAY;
        respawnBegin(respawnTransition, RESPAWN_RESTART);
        return;
    }
    if (o.has("LevelSelectButton")) {
        openSelect();
        return;
    }
    if (o.has("LevelCompleteContinueButton")) {
        if (makerRun.active)
            makerExitPlay();
        else if (currentStage + 1 < stageCount)
            startStage(currentStage + 1);
        else
            navigate(137502285691ULL);
        return;
    }
    if (o.has("QuitButtonBehaviour")) {
        running = false;
        return;
    }
    if (o.has("LevelSelectMinimizeButton")) {
        scene.minimized = !scene.minimized;
        return;
    }
    if (o.has("TurboToggleBehaviour")) {
        turbo = !turbo;
        writePreferences();
        return;
    }
    if (o.has("GfxWindowModeButton")) {
        windowMode = !windowMode;
        return;
    }
    if (o.has("GfxResolutionButton")) {
        resolutionChoice = (resolutionChoice + 1) % WINDOW_RESOLUTION_COUNT;
        return;
    }
    if (o.has("GfxApplyButton")) {
        if (fullscreen == windowMode)
            toggleFullscreen();
        if (windowMode)
            resizeWindow(windowResolutions[resolutionChoice].width,
                         windowResolutions[resolutionChoice].height);
        writePreferences();
        return;
    }
    if (o.has("ControlsKeyButton")) {
        const Props *b = o.behavior("ControlsKeyButton");
        rebindAction = actionIndex(b->get("key"));
        rebindColumn = (int)b->num("index");
        return;
    }
    if (o.has("ResetButtonBehaviour")) {
        if (MessageBoxA(0, "Reset all saved progress?", "Bean's Quest",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
            memset(&progress, 0, sizeof(progress));
            currentStage = 0;
            haveCheckpoint = false;
            char p[MAX_PATH];
            savePath(p, "checkpoint.dat");
            eraseCheckpointFilesNative(p);
            writeProgress();
            navigate(12192614360109026946ULL);
        }
        return;
    }
    if (o.has("WebLinkButtonBehaviour")) {
        const Props *p = o.behavior("WebLinkButtonBehaviour");
        const char *url = p->get("url");
        if (*url) {
            HMODULE shell = LoadLibraryA("shell32.dll");
            if (shell) {
                typedef HINSTANCE(WINAPI * Open)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, int);
                Open open = (Open)GetProcAddress(shell, "ShellExecuteA");
                if (open)
                    open(0, "open", url, 0, 0, SW_SHOWNORMAL);
                FreeLibrary(shell);
            }
        }
    }
}

UiScene &activeScene() {
    return screen == PLAY || screen == PAUSE || screen == COMPLETE ? hudScene : page;
}

void confirm() {
    if (!completionUiReady() || !pauseMenuInputReady())
        return;
    if (screen == PLAY)
        return;
    if (screen == SPLASH) {
        navigate(12192614360109026946ULL);
        return;
    }
    if (screen == INTRO) {
        finishIntro();
        return;
    }
    if (screen == ENDING) {
        acceptEndingCredits(page.age);
        return;
    }
    UiScene &s = activeScene();
    if (s.minimized) {
        s.minimized = false;
        return;
    }
    activate(s, s.focus);
}

void cancel() {
    if (!completionUiReady())
        return;
    if (screen == INTRO) {
        finishIntro(true);
        return;
    }
    if (rebindAction >= 0) {
        rebindAction = -1;
        return;
    }
    if (screen == PLAY) {
        openPause();
        return;
    }
    if (screen == PAUSE) {
        resumePauseInterfaceNative();
        return;
    }
    if (screen == COMPLETE) {
        openSelect();
        return;
    }
    UiScene &s = activeScene();
    if (s.minimized) {
        s.minimized = false;
        return;
    }
    for (int i = 0; i < s.level.objectCount; i++) {
        const Props *p = s.level.objects[i].behavior("LevelButtonBehaviour");
        if (p && p->flag("cancel-button")) {
            activate(s, i);
            return;
        }
    }
    if (screen == TITLE) {
        running = false;
        return;
    }
    navigate(12192614360109026946ULL);
}

void moveFocus(const char *direction) {
    if (!completionUiReady() || !pauseMenuInputReady())
        return;
    UiScene &s = activeScene();
    if (s.focus < 0)
        return;
    const Props *props = buttonProps(*s.nodes[s.focus].object);
    if (props) {
        u64 target = idnum(props->get(direction));
        if (target) {
            int previous = s.focus;
            focusId(s, target);
            if (s.focus != previous)
                return;
        }
    }
    // The original data defines most focus links; spatial fallback covers the
    // vertical Up key reassigned by the user to the same action as Space.
    Vec c = center(s.nodes[s.focus].rect);
    float best = 1e20f;
    int index = s.focus;
    for (int i = 0; i < s.level.objectCount; i++) {
        UiNode &n = s.nodes[i];
        if (i == s.focus || !buttonProps(*n.object) || !visibleNode(s, n))
            continue;
        Vec p = center(n.rect);
        float dx = p.x - c.x, dy = p.y - c.y;
        bool candidate = eq(direction, "focus-left")    ? dx < -1
                         : eq(direction, "focus-right") ? dx > 1
                         : eq(direction, "focus-up")    ? dy > 1
                                                        : dy < -1;
        if (!candidate)
            continue;
        float score =
            dx * dx +
            dy * dy * ((eq(direction, "focus-down") || eq(direction, "focus-up")) ? 1 : 4);
        if (score < best) {
            best = score;
            index = i;
        }
    }
    s.focus = index;
}

void keyPressed(int key) {
    if (titleAttractInput())
        return;
    if (endingScene.active && key != VK_F11)
        return;
    if (makerKey(key))
        return;
    if (screen == TITLE && key >= 'A' && key <= 'Z') {
        const char *code = "SKELETONKEY";
        if (key == code[debugSequence])
            debugSequence++;
        else
            debugSequence = key == 'S' ? 1 : 0;
        if (debugSequence == 11) {
            debugMode = false;
            progress.unlocked = stageCount - 1;
            writeProgress();
            makerUnlock();
            debugSequence = 0;
            sound("ringing");
            return;
        }
    } else if (screen != TITLE)
        debugSequence = 0;
    if (rebindAction >= 0) {
        if (key == VK_ESCAPE) {
            rebindAction = -1;
            return;
        }
        if (rebindColumn)
            secondaryKey[rebindAction] = key;
        else
            primaryKey[rebindAction] = key;
        rebindAction = -1;
        writeBindings();
        return;
    }
    if (key == VK_F11) {
        toggleFullscreen();
        windowMode = !fullscreen;
        writePreferences();
        return;
    }
    if (key == VK_SPACE || key == VK_RETURN) {
        confirm();
        return;
    }
    if (key == VK_UP || bound(key, 2)) {
        if (screen == PLAY)
            confirm();
        else
            moveFocus("focus-up");
        return;
    }
    if (bound(key, 4)) {
        confirm();
        return;
    }
    if (key == VK_ESCAPE || bound(key, 5)) {
        cancel();
        return;
    }
    if ((key == 'R' || key == 'S' || bound(key, 6) || bound(key, 7)) &&
        (screen == PLAY || screen == PAUSE || screen == COMPLETE)) {
        resetPauseInterfaceNative();
        screen = PLAY;
        if (key == 'R' || bound(key, 7))
            respawnBegin(respawnTransition, RESPAWN_RESTART);
        else
            respawnBegin(respawnTransition, RESPAWN_CHECKPOINT);
        return;
    }
    if (key == 'L' && (screen == PLAY || screen == PAUSE || screen == COMPLETE)) {
        openSelect();
        return;
    }
    if (key == 'M') {
        musicEnabled = !musicEnabled;
        if (musicEnabled)
            playMusic(screen == PLAY || screen == PAUSE ? stageMusic() : 3080012186760326ULL);
        else
            stopMusic();
        writeProgress();
        return;
    }
    if (screen == PLAY)
        return;
    if (!pauseMenuInputReady())
        return;
    if (key == VK_TAB) {
        UiScene &s = activeScene();
        for (int n = 1; n <= s.level.objectCount; n++) {
            int i = (s.focus + n) % s.level.objectCount;
            if (buttonProps(*s.nodes[i].object) && visibleNode(s, s.nodes[i])) {
                s.focus = i;
                break;
            }
        }
        return;
    }
    if (bound(key, 0))
        moveFocus("focus-left");
    if (bound(key, 1))
        moveFocus("focus-right");
    if (bound(key, 3))
        moveFocus("focus-down");
}

void mouseInterface(float x, float y, bool click) {
    if (!click && makerPointerMove(x, y))
        return;
    titleAttractPointerActivity(x, y, click);
    if (titleAttractBusy()) {
        if (click)
            titleAttractInput();
        return;
    }
    if (endingScene.active || !pauseMenuInputReady())
        return;
    if (pointerHeld) {
        if (screen == WORLD) {
            worldSelectPointerNative.x = x * screenScale;
            return;
        }
        if (absf(x - pointerStartX) > 6 || absf(y - pointerStartY) > 6)
            pointerDragged = true;
        return;
    }
    UiScene &s = activeScene();
    for (int i = s.level.objectCount - 1; i >= 0; i--) {
        UiNode &n = s.nodes[i];
        if (!buttonProps(*n.object) || !visibleNode(s, n))
            continue;
        float dx, dy;
        layerOffset(s, n.object->layer, &dx, &dy);
        Rect r = shifted(visualRect(s, n), dx, dy);
        if (x >= r.l && x <= r.r && y >= r.b && y <= r.t) {
            if (screen == WORLD) {
                if (click) {
                    if (s.focus == i)
                        activate(s, i);
                    else
                        s.focus = i;
                }
            } else {
                s.focus = i;
                if (click)
                    activate(s, i);
            }
            return;
        }
    }
}

int pointerHit(float x, float y) {
    if (endingScene.active || !completionUiReady() || !pauseMenuInputReady())
        return -1;
    UiScene &s = activeScene();
    for (int i = s.level.objectCount - 1; i >= 0; i--) {
        UiNode &n = s.nodes[i];
        if (!buttonProps(*n.object) || !visibleNode(s, n))
            continue;
        float dx, dy;
        layerOffset(s, n.object->layer, &dx, &dy);
        Rect r = shifted(visualRect(s, n), dx, dy);
        if (x >= r.l && x <= r.r && y >= r.b && y <= r.t)
            return i;
    }
    return -1;
}

void pointerDown(float x, float y) {
    if (makerPointerDown(x, y, 0))
        return;
    if (titleAttractInput()) {
        pointerHeld = false;
        pointerNode = -1;
        return;
    }
    if (screen == PLAY && readyTimer > 0)
        readyPointerDown = true;
    if (screen == ENDING) {
        acceptEndingCredits(page.age);
        pointerHeld = false;
        pointerNode = -1;
        return;
    }
    pointerNode = pointerHit(x, y);
    pointerHeld = true;
    pointerDragged = false;
    pointerStartX = x;
    pointerStartY = y;
    pointerCameraX = page.cameraX;
    pointerScene = activeScene().level.id;
    if (screen == WORLD) {
        if (page.focus >= 0 && page.nodes[page.focus].world > 0)
            selectedWorld = page.nodes[page.focus].world;
        worldSelectBeginNative(worldSelectPointerNative, x * screenScale);
    }
}

// Activate only the matching press target; dragging or changing scenes must cancel the pending button press.
void pointerUp(float x, float y) {
    if (makerPointerUp(x, y, 0))
        return;
    readyPointerDown = false;
    bool wasHeld = pointerHeld;
    pointerHeld = false;
    if (!wasHeld)
        return;
    if (screen == WORLD) {
        bool dragged = worldSelectPointerNative.dragged;
        int slot = worldSelectReleaseNative(worldSelectPointerNative, worldSelectSlot(selectedWorld),
                                            x * screenScale, screenScale);
        if (dragged) {
            for (int i = 0; i < page.level.objectCount; i++)
                if (page.nodes[i].world > 0 && worldSelectSlot(page.nodes[i].world) == slot) {
                    page.focus = i;
                    selectedWorld = page.nodes[i].world;
                    break;
                }
            worldSelectPointerNative.dragged = false;
            pointerDragged = false;
            return;
        }
    }
    if (pointerNode == pointerHit(x, y) && pointerScene == activeScene().level.id)
        mouseInterface(x, y, true);
}

#include "maker.h"
