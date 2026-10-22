#pragma once

// Course maker integration. The maker chapter appears before Grasslands in World Select once the
// Wizard is defeated (or SKELETONKEY is entered). Custom courses run through the campaign stage
// runtime; leaving them returns to the editor (test play) or to the course list.
#include "maker_ui.h"
#include "maker_data.h"
#include "maker_catalog.h"
#include "maker_bake.h"
#include "maker_editor.h"
#include "maker_browser.h"

const u64 MAKER_WORLD_NODE = 0x4d414b4552ULL; // "MAKER"
MakerLevel *makerPlayLevel;
bool makerPlayFromCursor, makerJustVerified;
Vec makerPlayStart;
int makerCourseCount;
int makerTestClearJumps; // jumps of the last test clear, -1 while the test has not been cleared
Texture makerCardSceneTexture, makerCardTitleTexture;

bool makerScreen() {
    return screen == MAKER_EDIT || screen == MAKER_BROWSE;
}

void makerLoadCurrentLevel() {
    if (!makerPlayLevel) {
        makerRun.active = false;
        loadLevelData(levels[stageList[currentStage]].file);
        return;
    }
    makerBakeCourse(*makerPlayLevel, makerRun.slot, current, levelArena,
                    makerPlayFromCursor ? &makerPlayStart : 0);
}

// currentStage keeps the campaign position; startStage consults makerRun to bake the course.
void makerStartRun(MakerLevel &level, int slot, bool testing, bool fromCursor, Vec start) {
    makerPlayLevel = &level;
    makerRun.active = true;
    makerRun.testing = testing;
    makerRun.verifying = testing && !fromCursor;
    makerRun.slot = slot;
    makerRun.recordJumps = !testing && level.head.cleared ? level.head.bestJumps : 0;
    makerPlayFromCursor = fromCursor;
    makerPlayStart = start;
    makerJustVerified = false;
    makerTestClearJumps = -1;
    worldSelectPointerNative = WorldSelectPointerNative{};
    pointerHeld = false;
    pointerNode = -1;
    stopSounds();
    startStage(currentStage, false);
    if (testing) {
        // Test runs start at once; course runs keep the READY banner with the course name.
        readyOpening = readyNativeInitial(true);
        readyTimer = 0;
    }
    transitionTime = transitionDuration = 0.3f;
}

void makerStartTest(bool fromCursor) {
    if (fromCursor && !edInCanvas(makerEd.mouseX, makerEd.mouseY))
        fromCursor = false;
    float wx = edWorldX(makerEd.mouseX), wy = edWorldY(makerEd.mouseY);
    Vec start = {edSnapCoord(wx, 32) + 16, edSnapCoord(wy, 32) + 16};
    makerEd.drag = DRAG_NONE;
    makerEd.picker = makerEd.settings = makerEd.help = makerEd.nameEditing = false;
    makerStartRun(makerEd.level, makerEd.slot, true, fromCursor, start);
}

void makerStartCourse(int slot) {
    makerAllocate(makerPlayCourse);
    if (!makerLoadLevel(slot, makerPlayCourse)) {
        mkToast("This course could not be loaded", 0xff8080ff);
        return;
    }
    makerProfile.lastSlot = slot;
    makerWriteProfile();
    makerStartRun(makerPlayCourse, slot, false, false, Vec{0, 0});
}

void makerEditCourse(int slot, bool fresh, int theme) {
    if (!edOpen(slot, fresh, theme)) {
        mkToast("This course could not be loaded", 0xff8080ff);
        return;
    }
    makerProfile.lastSlot = slot;
    makerWriteProfile();
    if (fresh) {
        makerSaveLevel(slot, makerEd.level);
        makerEd.dirty = false;
    }
    screen = MAKER_EDIT;
    transitionTime = transitionDuration = 0.25f;
    playMusic(makerCourseMusic(makerEd.level.head));
}

void makerOpenBrowser() {
    makerRun.active = false;
    makerEd.open = false;
    resetPauseInterfaceNative();
    respawnCancel(respawnTransition);
    deathTimer = deathAge = 0;
    resetEndingScene();
    pointerHeld = false;
    pointerNode = -1;
    makerReadProfile();
    if (!makerProfile.seen) {
        makerProfile.seen = 1;
        makerWriteProfile();
    }
    brOpen();
    screen = MAKER_BROWSE;
    transitionTime = transitionDuration = 0.25f;
    playMusic(3080012186760326ULL);
}

void makerFocusWorldCard() {
    for (int i = 0; i < page.level.objectCount; i++)
        if (page.nodes[i].object->id == MAKER_WORLD_NODE) {
            page.focus = i;
            selectedWorld = 7;
            page.cameraX = page.targetX = center(page.nodes[i].rect).x - viewW * 0.5f;
            return;
        }
}

void makerLeaveBrowser() {
    makerWriteProfile();
    navigate(18006219643615701355ULL);
    makerFocusWorldCard();
    transitionTime = transitionDuration = 0.25f;
}

void makerExitPlay() {
    bool testing = makerRun.testing, verifying = makerRun.verifying;
    makerRun.active = false;
    resetPauseInterfaceNative();
    respawnCancel(respawnTransition);
    deathTimer = deathAge = 0;
    resetEndingScene();
    stopSounds();
    clearEffects();
    complete = false;
    readyTimer = 0;
    pointerHeld = false;
    pointerNode = -1;
    if (testing && makerEd.open) {
        screen = MAKER_EDIT;
        transitionTime = transitionDuration = 0.25f;
        playMusic(makerCourseMusic(makerEd.level.head));
        char text[96];
        if (makerTestClearJumps >= 0) {
            if (!verifying)
                lstrcpyA(text, "Cleared from the cursor - test from the start to verify");
            else
                wsprintfA(text, "%sCleared in %d jumps (gold shoe: %d)",
                          makerJustVerified ? "Course verified! " : "", makerTestClearJumps,
                          makerEd.level.head.jumpBest);
            mkToast(text, 0x80ff80ff, 4);
        }
        makerJustVerified = false;
    } else
        makerOpenBrowser();
}

// Clears from the course start verify a course under test; real runs keep their records.
void makerRecordClear() {
    if (!makerPlayLevel)
        return;
    MakerHeader &h = makerPlayLevel->head;
    if (makerRun.testing) {
        makerTestClearJumps = jumps;
        if (makerRun.verifying && !(h.flags & MAKER_VERIFIED)) {
            h.flags |= MAKER_VERIFIED;
            makerEd.dirty = true;
            makerJustVerified = true;
        }
        return;
    }
    h.cleared = 1;
    if (!h.bestJumps || jumps < h.bestJumps)
        h.bestJumps = jumps;
    if (coins > h.bestCoins)
        h.bestCoins = coins;
    if (salamander)
        h.axolotl = 1;
    makerRun.recordJumps = h.bestJumps;
    makerSaveLevel(makerRun.slot, *makerPlayLevel);
}

bool makerHudCaption(u64 id, char *value) {
    if (!makerRun.active)
        return false;
    if (id == 11477711412535822434ULL) {
        if (makerRun.testing)
            lstrcpyA(value, "TEST PLAY");
        else
            wsprintfA(value, "COURSE C-%02d", makerRun.slot + 1);
        return true;
    }
    if (id == 8364913218960181251ULL) {
        int par = (int)current.props.num("bean-jump-par");
        lstrcpyA(value, makerRun.recordJumps && makerRun.recordJumps <= par ? "BEST" : "PAR");
        return true;
    }
    return false;
}

int makerCountCourses() {
    int n = 0;
    for (int i = 0; i < MAKER_SLOTS; i++) {
        char path[MAX_PATH];
        makerCoursePath(i, path);
        n += GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
    }
    return n;
}

// Prepend Maker visually without renumbering campaign chapters or shifting their artwork.
// Rebuild the node/object arrays together so every retained pointer belongs to the scene arena.
void makerDecorateWorldSelect() {
    worldSelectSlots = 6;
    worldSelectFirstSlot = 1;
    if (!makerUnlocked())
        return;
    UiScene &s = page;
    int first = -1;
    for (int i = 0; i < s.level.objectCount; i++) {
        if (s.level.objects[i].id == MAKER_WORLD_NODE) {
            worldSelectSlots = 7;
            worldSelectFirstSlot = 0;
            return;
        }
        if (s.nodes[i].world == 1)
            first = i;
    }
    if (first < 0)
        return;
    int n = s.level.objectCount;
    Object *objects = s.memory.array<Object>(n + 1);
    memcpy(objects, s.level.objects, sizeof(Object) * n);
    UiNode *nodes = s.memory.array<UiNode>(n + 1);
    memcpy(nodes, s.nodes, sizeof(UiNode) * n);
    for (int i = 0; i < n; i++)
        nodes[i].object = objects + i;
    Object &firstObject = objects[first];
    char firstId[24], makerId[24];
    idstr(firstObject.id, firstId);
    idstr(MAKER_WORLD_NODE, makerId);
    Props *firstProps = makerBehaviorProps(firstObject, "WorldButtonBehaviour");
    if (firstProps)
        makerSetProp(*firstProps, "focus-left", makerId, s.memory);
    Object &o = objects[n];
    o = firstObject;
    o.id = MAKER_WORLD_NODE;
    Vec c = center(firstObject.rect);
    o.rect = makerCentered(c.x - 224, c.y, 160, 160);
    o.behaviors = s.memory.array<Behavior>(1);
    o.behaviorCount = 1;
    o.behaviors[0].type = makerString(s.memory, "WorldButtonBehaviour");
    Props p = {};
    makerSetProp(p, "world", "7", s.memory);
    makerSetProp(p, "level", "0", s.memory);
    makerSetProp(p, "focus-right", firstId, s.memory);
    makerSetProp(p, "focus-up", "17649694756044529744", s.memory); // the authored Back button
    o.behaviors[0].props = p;
    UiNode &node = nodes[n];
    memset(&node, 0, sizeof(node));
    node.object = &o;
    node.rect = o.rect;
    node.stage = -1;
    node.world = 7;
    s.level.objects = objects;
    s.nodes = nodes;
    s.level.objectCount = n + 1;
    // Particle pools are keyed by object address; rebind them as loadUi does.
    nativeContinuousResetScene(s.level);
    nativeContinuousAdvanceScene(s.level, 1, 0, 0);
    worldSelectSlots = 7;
    worldSelectFirstSlot = 0;
    makerCourseCount = makerCountCourses();
}

bool makerWorldNode(const Object &o) {
    return o.id == MAKER_WORLD_NODE;
}

// Reuse the chapter vignettes already owned by World Select instead of loading a second atlas.
Tile *makerChapterArtwork(UiScene &s, int world) {
    for (int i = 0; i < s.level.objectCount; i++) {
        if (s.nodes[i].world != world)
            continue;
        Vec c = center(s.nodes[i].rect);
        for (int j = 0; j < s.level.tileCount; j++) {
            Tile &t = s.level.tiles[j];
            Vec tc = center(t.rect);
            if (t.bytes == sizeof(Vertex) * 4 && t.rect.r - t.rect.l == 192 &&
                t.rect.t - t.rect.b == 192 && absf(tc.x - c.x) < 1 && absf(tc.y - c.y) < 1)
                return &t;
        }
    }
    return 0;
}

// Textures are prepared from the two editable PNGs. Read through the existing native texture loader,
// then free the temporary CPU copy; these textures live until the game releases its OpenGL context.
Texture makerLoadChapterTexture(const char *name) {
    char path[128];
    wsprintfA(path, "data/Media/Windows/Maker/%s.ptex", name);
    u32 bytes;
    u8 *data = makerReadWhole(path, &bytes);
    if (!data)
        fatal("Missing Maker chapter artwork. Rebuild with tools/prepare_maker_art.py.");
    Reader reader(data, bytes, permanent);
    Texture result = textureRead(reader);
    if (reader.pos != reader.size)
        fatal("Unsupported Maker chapter artwork.");
    mkFree(data);
    return result;
}

void makerLoadChapterArtwork() {
    if (!makerCardSceneTexture.gl)
        makerCardSceneTexture = makerLoadChapterTexture("scene");
    if (!makerCardTitleTexture.gl)
        makerCardTitleTexture = makerLoadChapterTexture("title");
}

// The 832x1024 editing canvas maps to 208x256 scene units, with the original chapter's center kept
// at (104,144) in image coordinates. Transparent padding is retained rather than cropping the frame.
void makerDrawChapterTexture(const Texture &texture, Vec c, float alpha = 1) {
    float u = 832.0f / texture.width, v = 1024.0f / texture.height;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture.gl);
    glColor4f(1, 1, 1, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0, v);
    glVertex2f(c.x - 104, c.y - 112);
    glTexCoord2f(u, v);
    glVertex2f(c.x + 104, c.y - 112);
    glTexCoord2f(u, 0);
    glVertex2f(c.x + 104, c.y + 144);
    glTexCoord2f(0, 0);
    glVertex2f(c.x - 104, c.y + 144);
    glEnd();
}

// Lettering uses the bundled Vera Bold face, a gentle arch and a shaded gold fill.
// Outlines are drawn in scene units along with the cover; no Tahoma/editor widget text is used.
void makerCardGlyph(GlyphFont &font, char ch, float x, float y, float angle, u32 bottom, u32 top) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    float u = (ch % 16) * 64 / 1024.0f, v = (ch / 16) * 64 / 512.0f;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font.texture);
    glBegin(GL_QUADS);
    mkColor(bottom);
    glTexCoord2f(u, v + 64 / 512.0f);
    glVertex2f(-1, font.lineHeight - 63);
    glTexCoord2f(u + 64 / 1024.0f, v + 64 / 512.0f);
    glVertex2f(63, font.lineHeight - 63);
    mkColor(top);
    glTexCoord2f(u + 64 / 1024.0f, v);
    glVertex2f(63, font.lineHeight + 1);
    glTexCoord2f(u, v);
    glVertex2f(-1, font.lineHeight + 1);
    glEnd();
    glPopMatrix();
}

void makerCardTitle(Vec c) {
    makerLoadChapterArtwork();
    makerDrawChapterTexture(makerCardTitleTexture, c);
}

// Keep the illustration separate from captions so it can also be exported for manual artwork edits.
void makerDrawChapterIllustration(UiScene &s, UiNode &n, bool title = true) {
    (void)s;
    makerLoadChapterArtwork();
    Vec c = center(n.rect);
    makerDrawChapterTexture(makerCardSceneTexture, c, n.disabled ? .45f : 1);
    if (title)
        makerCardTitle(c);
}

// The edited scene and illustrated title share the same canvas, so frame and logo stay aligned.
bool makerDrawWorldNode(UiScene &s, UiNode &n) {
    if (&s != &page || !n.object || n.object->id != MAKER_WORLD_NODE)
        return false;
    makerDrawChapterIllustration(s, n);
    Vec c = center(n.rect);
    float l = c.x - 96, b = c.y - 96;
    char caption[32];
    wsprintfA(caption, "%d course%s", makerCourseCount, makerCourseCount == 1 ? "" : "s");
    // Use the chapter font at a readable native size; tiny bold glyphs looked flattened.
    GlyphFont &font = *originalFont("VeraBd", 12);
    float x = c.x - lineWidth(font, caption, lstrlenA(caption)) * .5f;
    for (int i = 0; caption[i]; i++) {
        makerCardGlyph(font, caption[i], x, b - 14, 0, 0xffffffff, 0xffffffff);
        x += font.width[(u8)caption[i]];
    }
    makerReadProfile();
    if (!makerProfile.seen && ((int)(s.age * 3) & 1))
        mkTextShadow(l + 120, b + 160, "NEW!", 12, 0xff5050ff, true);
    return true;
}

bool makerKey(int key) {
    if (screen == MAKER_EDIT)
        return edKey(key);
    if (screen == MAKER_BROWSE)
        return brKey(key);
    if (makerRun.active && makerRun.testing &&
        (screen == PLAY || screen == PAUSE || screen == COMPLETE) &&
        (key == VK_F5 || key == VK_TAB)) {
        makerExitPlay();
        return true;
    }
    return false;
}

// Held Ctrl+Z / Ctrl+Y keep stepping through history like other editors; held arrows keep moving
// through the course list.
void makerKeyRepeat(int key) {
    if (screen == MAKER_EDIT && mkKeyDown(VK_CONTROL) && (key == 'Z' || key == 'Y'))
        edKey(key);
    if (screen == MAKER_BROWSE && (key == VK_LEFT || key == VK_RIGHT || key == VK_UP ||
                                   key == VK_DOWN || key == VK_PRIOR || key == VK_NEXT))
        brKey(key);
}

void makerChar(int ch) {
    if (screen == MAKER_EDIT)
        edChar(ch);
}

bool makerPointerMove(float x, float y) {
    if (screen == MAKER_EDIT)
        edMouseMove(x, y);
    else if (screen == MAKER_BROWSE)
        brMouseMove(x, y);
    else
        return false;
    return true;
}

bool makerPointerDown(float x, float y, int button) {
    if (screen == MAKER_EDIT)
        edMouseDown(x, y, button);
    else if (screen == MAKER_BROWSE)
        brMouseDown(x, y, button);
    else
        return false;
    return true;
}

bool makerPointerUp(float x, float y, int button) {
    if (screen == MAKER_EDIT)
        edMouseUp(x, y, button);
    else if (!makerScreen())
        return false;
    return true;
}

bool makerWheel(int delta) {
    if (screen == MAKER_EDIT)
        edWheel(delta);
    else if (screen == MAKER_BROWSE)
        brWheel(delta);
    else
        return false;
    return true;
}

void makerCaptureLost() {
    if (screen == MAKER_EDIT)
        makerEd.drag = DRAG_NONE;
}

void makerTick(float dt) {
    if (screen == MAKER_EDIT)
        edTick(dt);
    else if (screen == MAKER_BROWSE)
        brTick(dt);
}

void makerRender() {
    if (screen == MAKER_EDIT)
        edRender();
    else
        brRender();
}

// A small reminder during test play of the keys that return to the editor.
void makerRenderPlayOverlay() {
    if (!makerRun.active || !makerRun.testing || screen != PLAY || endingScene.active)
        return;
    const char *text = makerPlayFromCursor ? "TEST FROM CURSOR   F5 / Tab: back to the editor"
                                           : "TEST PLAY   F5 / Tab: back to the editor";
    float size = 8, w = mkTextWidth(text, size, true) + 14, h = 15;
    float x = (viewW - w) * 0.5f, y = 4;
    mkRect(x, y, w, h, 0x00000080);
    mkText(x + 7, y + (h - mkLineHeight(size, true)) * 0.5f, text, size, 0xffffffd0, true);
}

// Closing the window keeps unsaved editor work.
void makerShutdown() {
    if (makerEd.open && makerEd.dirty) {
        makerPrepareHeader(makerEd.level);
        makerSaveLevel(makerEd.slot, makerEd.level);
    }
    if (makerCardSceneTexture.gl)
        glDeleteTextures(1, &makerCardSceneTexture.gl);
    if (makerCardTitleTexture.gl)
        glDeleteTextures(1, &makerCardTitleTexture.gl);
    makerCardSceneTexture = makerCardTitleTexture = Texture{};
}
