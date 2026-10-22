#pragma once

// Course list of the maker chapter, laid out like a world's level select: the MAKER logo over the
// menu sky and pages of course cards framed in the colour of each course's theme, each with a
// photo of its opening screen and the level-select award badges. A new course first picks its
// theme from the world logos.

enum MakerBrowserCommand {
    BCMD_NONE,
    BCMD_BACKDROP,
    BCMD_CARD,
    BCMD_PLAY,
    BCMD_EDIT,
    BCMD_NEW,
    BCMD_COPY,
    BCMD_DELETE,
    BCMD_BACK,
    BCMD_PAGE,
    BCMD_THEME,
    BCMD_THEME_CANCEL,
    BCMD_CONFIRM_DELETE,
    BCMD_CONFIRM_CANCEL,
    BCMD_MUSIC
};

struct MakerBrowser {
    MakerHeader heads[MAKER_SLOTS];
    bool exists[MAKER_SLOTS];
    int selected, page;
    int confirm, confirmFocus; // slot awaiting delete confirmation (-1 none); 1 focuses Delete
    float age, mouseX, mouseY, lastClick, lastClickX, lastClickY;
    bool themePicker, loading, rendered;
    MakerButtons buttons;
};

MakerBrowser makerBrowser;
MakerLevel makerPlayCourse, makerCopyCourse;

void makerStartCourse(int slot);
void makerEditCourse(int slot, bool fresh, int theme);
void makerLeaveBrowser();
void makerLoadChapterArtwork();
extern Texture makerCardTitleTexture;

// Course photos: the opening screen of each course on the page, drawn with its own bricks and
// items. Only the pieces around the start are kept, and one course is read per frame.
enum { BR_PHOTO_TILES = 640, BR_PHOTO_OBJECTS = 96 };

const float BR_PHOTO_W = 432, BR_PHOTO_H = 144;

struct MakerPhoto {
    bool ready;
    int slot, tileCount, objectCount;
    Rect view;
    MakerTile tiles[BR_PHOTO_TILES];
    MakerObject objects[BR_PHOTO_OBJECTS];
};

MakerPhoto makerPhotos[MAKER_PAGE];
MakerLevel makerPhotoCourse;

// Sky behind each theme's photos, top then horizon.
const u32 brPhotoSky[6][2] = {{0x57c4ffff, 0xc8ecffff}, {0x57c4ffff, 0xc8ecffff},
                              {0x6a5acdff, 0xd8a8f0ff}, {0xff9a2eff, 0xffe7a0ff},
                              {0xe0407aff, 0xffa040ff}, {0x1a1030ff, 0x4a2a60ff}};

void brRefresh() {
    for (int i = 0; i < MAKER_SLOTS; i++)
        makerBrowser.exists[i] = makerLoadHeader(i, makerBrowser.heads[i]);
    for (int i = 0; i < MAKER_PAGE; i++)
        makerPhotos[i].ready = false;
}

// Keep the bricks and items of a course's opening screen for its card.
void brBuildPhoto(int slot) {
    MakerPhoto &p = makerPhotos[slot % MAKER_PAGE];
    p.ready = true;
    p.slot = slot;
    p.tileCount = p.objectCount = 0;
    if (!makerLoadLevel(slot, makerPhotoCourse))
        return;
    const MakerLevel &l = makerPhotoCourse;
    const MakerHeader &h = l.head;
    float x = clamp(h.startX - 96, 0, maxf(0, h.width - BR_PHOTO_W));
    float y = clamp(h.startY - 88, 0, maxf(0, h.height - BR_PHOTO_H));
    p.view = mkBox(x, y, BR_PHOTO_W, BR_PHOTO_H);
    for (int i = 0; i < l.tileCount && p.tileCount < BR_PHOTO_TILES; i++) {
        const MakerTile &t = l.tiles[i];
        if (t.r > p.view.l && t.l < p.view.r && t.t > p.view.b && t.b < p.view.t)
            p.tiles[p.tileCount++] = t;
    }
    for (int i = 0; i < l.objectCount && p.objectCount < BR_PHOTO_OBJECTS; i++) {
        const MakerObject &o = l.objects[i];
        const MakerObjectDef *d = makerDefFor(o.klass);
        Rect r = edObjectRect(o);
        if (d && !(d->flags & (OBJ_FREE | OBJ_INVISIBLE | OBJ_START)) && r.r > p.view.l - 64 &&
            r.l < p.view.r + 64 && r.t > p.view.b - 64 && r.b < p.view.t + 64)
            p.objects[p.objectCount++] = o;
    }
}

// The photo covers the box from its bottom-left corner, so the ground and Bean stay in view.
void brDrawPhoto(int slot, Rect box) {
    const MakerHeader &h = makerBrowser.heads[slot];
    const u32 *sky = brPhotoSky[(int)clamp((float)h.theme, 1, 5)];
    mkGradient(box.l, box.b, box.r - box.l, box.t - box.b, sky[1], sky[0]);
    const MakerPhoto &p = makerPhotos[slot % MAKER_PAGE];
    if (!p.ready || p.slot != slot)
        return;
    float scale = maxf((box.r - box.l) / BR_PHOTO_W, (box.t - box.b) / BR_PHOTO_H);
    glEnable(GL_SCISSOR_TEST);
    glScissor(viewportX + makerRound(box.l * screenScale),
              viewportY + makerRound(box.b * screenScale),
              makerRound((box.r - box.l) * screenScale), makerRound((box.t - box.b) * screenScale));
    glPushMatrix();
    glTranslatef(box.l, box.b, 0);
    glScalef(scale, scale, 1);
    glTranslatef(-p.view.l, -p.view.b, 0);
    for (int layer = 0; layer < MAKER_LAYERS; layer++) {
        if (layer == MAKER_FRONT) {
            for (int i = 0; i < p.objectCount; i++)
                edDrawObject(p.objects[i], 0, 0, 1, -1, false);
            edDrawStartBean(makerCentered(h.startX, h.startY, 32, 32));
        }
        // Main-layer blocks come before overlays such as grass, as in the editor.
        for (int pass = 0; pass < 2; pass++)
            for (int i = 0; i < p.tileCount; i++) {
                const MakerTile &t = p.tiles[i];
                bool opaque = (t.flags & TILE_OPAQUE) != 0;
                if (t.layer == layer && (layer == MAKER_MAIN ? (pass == 0) == opaque : pass == 0))
                    edDrawTile(t, 0, 0, 1);
            }
    }
    glPopMatrix();
    glDisable(GL_SCISSOR_TEST);
}

int brCoinGoal(const MakerHeader &h) {
    return h.coinGoal > 0 && h.coinGoal < h.coins ? h.coinGoal : h.coins;
}

// Every award of the course: its gems, the gold shoe and its axolotl.
bool brPerfect(const MakerHeader &h) {
    return h.cleared && (h.coins <= 0 || h.bestCoins >= brCoinGoal(h)) &&
           h.bestJumps <= h.jumpBest && (!h.hasAxolotl || h.axolotl);
}

void brBadges(const MakerHeader &h, float x, float y, float size) {
    Actor *gem = actor(16535973452759161611ULL), *shoe = actor(7897082502132702692ULL),
          *axolotl = actor(12192429290865414990ULL);
    bool gems = h.cleared && h.coins > 0 && h.bestCoins >= brCoinGoal(h);
    const char *shoeAnim = !h.cleared                  ? "off"
                           : h.bestJumps <= h.jumpBest ? "best"
                           : h.bestJumps <= h.jumpPar  ? "on"
                                                       : "off";
    if (h.coins > 0) {
        drawActor(gem, mkBox(x, y, size, size), gems ? "on" : "off", 0);
        x += size + 3;
    }
    drawActor(shoe, mkBox(x, y, size, size), shoeAnim, 0);
    x += size + 3;
    if (h.hasAxolotl)
        drawActor(axolotl, mkBox(x, y, size, size), h.axolotl ? "on" : "off", 0);
}

// The chapter's MAKER logo, cut from the illustrated title layer of the world select card.
void brDrawLogo(float cx, float cy, float height) {
    makerLoadChapterArtwork();
    const Texture &t = makerCardTitleTexture;
    if (!t.gl)
        return;
    // Bounds of the lettering on the 832x1024 canvas.
    const float l = 54, top = 60, r = 757, bottom = 278;
    float w = height * (r - l) / (bottom - top);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, t.gl);
    // The painted logo is shown well below its source size, so filter it smoothly.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(l / t.width, bottom / t.height);
    glVertex2f(cx - w * 0.5f, cy - height * 0.5f);
    glTexCoord2f(r / t.width, bottom / t.height);
    glVertex2f(cx + w * 0.5f, cy - height * 0.5f);
    glTexCoord2f(r / t.width, top / t.height);
    glVertex2f(cx + w * 0.5f, cy + height * 0.5f);
    glTexCoord2f(l / t.width, top / t.height);
    glVertex2f(cx - w * 0.5f, cy + height * 0.5f);
    glEnd();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

// Slot number in yellow on a black capsule, like the pause captions.
Rect brNumber(int slot, float x, float y, float h, float alpha = 1) {
    char text[8];
    wsprintfA(text, "%02d", slot + 1);
    Rect r = mkBox(x, y, mkTextWidth(text, 7.5f, true) + 9, h);
    mkPill(r, alpha);
    mkTextCentered((r.l + r.r) * 0.5f, (r.b + r.t) * 0.5f, text, 7.5f,
                   0xffee3300 | (u32)(int)(255 * alpha), true);
    return r;
}

void brCard(int slot, Rect r) {
    MakerBrowser &b = makerBrowser;
    bool selected = slot == b.selected;
    bool hover = b.confirm < 0 && !b.themePicker && mkInside(r, b.mouseX, b.mouseY);
    bool held = hover && mkKeyDown(VK_LBUTTON);
    float w = r.r - r.l, h = r.t - r.b, pad = 5, rowH = 11;
    Rect focus = {r.l - 2, r.b - 2, r.r + 2, r.t + 2};
    if (!b.exists[slot]) {
        // Free slots are the pressed level button, faded into the sky.
        mkWorldFrame(r, 1, "down", hover ? 0.75f : 0.45f);
        brNumber(slot, r.l + pad, r.t - pad - rowH, rowH, 0.8f);
        float cx = (r.l + r.r) * 0.5f, cy = (r.b + r.t) * 0.5f;
        mkLabelCentered(cx, cy + h * 0.08f, "+", minf(18, h * 0.25f), 0xffffffff);
        mkLabelCentered(cx, cy - h * 0.16f, "New course", 8, hover ? 0xffffffff : 0xe8f0e0ff);
        if (selected)
            mkBrackets(focus, 6);
        return;
    }
    const MakerHeader &head = b.heads[slot];
    int theme = (int)clamp((float)head.theme, 1, 5);
    mkWorldFrame(r, theme, held ? "down" : "up", 1, hover ? 0.16f : 0);
    Rect number = brNumber(slot, r.l + pad, r.t - pad - rowH, rowH);
    char name[64];
    mkFitText(name, sizeof(name), head.name, r.r - pad - number.r - 4, 8.5f, true);
    mkLabel(number.r + 4, (number.b + number.t - mkLineHeight(8.5f, true)) * 0.5f, name, 8.5f,
            0xffffffff);
    // The photo sits in the stage-clear score box; badges and the clear check run underneath.
    float badge = clamp(h * 0.14f, 8, 11);
    Rect frame = {r.l + pad - 1, r.b + pad + badge + 1, r.r - pad + 1, number.b - 2};
    mkInset(frame);
    mkBeginPhotoMask(frame);
    brDrawPhoto(slot, frame);
    glDisable(GL_STENCIL_TEST);
    brBadges(head, r.l + pad, r.b + pad - 1, badge);
    bool verified = (head.flags & MAKER_VERIFIED) != 0;
    // Move the whole verification group clear of the frame; its label follows this rectangle.
    Rect tick = mkBox(r.r - pad - badge - 3, r.b + pad - 1, badge, badge);
    mkSprite(MKART_CHECK, verified ? "up-on" : "up-off", tick);
    const char *state = verified ? "Verified" : "Not verified";
    float tw = mkTextWidth(state, 7, true);
    if (tw + badge * 5 + 16 < w)
        mkLabel(tick.l - tw - (verified ? 3 : 6), (tick.b + tick.t - mkLineHeight(7, true)) * 0.5f, state, 7,
                verified ? 0xffffffff : 0xd8d8d8ff);
    if (selected)
        mkBrackets(focus, 6);
}

// Awards form one foreground pass after every card, including neighbouring frames and focus marks.
void brCardAward(int slot, Rect r) {
    if (makerBrowser.exists[slot] && brPerfect(makerBrowser.heads[slot]))
        mkSprite(MKART_STAR, "", makerCentered(r.r - 3, r.t - 3, 30, 30),
                 1, 0, -135 * makerBrowser.age);
}

// A theme choice: the normal green button recoloured in code, with its original world logo.
void brThemeChoice(MakerButton &btn) {
    MakerBrowser &b = makerBrowser;
    bool hover = mkInside(btn.r, b.mouseX, b.mouseY), held = hover && mkKeyDown(VK_LBUTTON);
    float h = btn.r.t - btn.r.b;
    mkButtonArt(btn.r, held, hover, btn.disabled, btn.arg);
    char key[4];
    wsprintfA(key, "%d", btn.arg);
    Rect pill = mkBox(btn.r.l + 6, (btn.r.b + btn.r.t) * 0.5f - 6, 14, 12);
    mkPill(pill);
    mkTextCentered((pill.l + pill.r) * 0.5f, (pill.b + pill.t) * 0.5f, key, 7.5f, 0xffee33ff, true);
    mkSpriteFit(MKART_WORLD_TITLE[btn.arg], "",
                Rect{pill.r + 6, btn.r.b + 3, btn.r.r - pill.r + btn.r.l - 6, btn.r.t - 3});
    if (hover)
        mkBrackets(Rect{btn.r.l - 2, btn.r.b - 2, btn.r.r + 2, btn.r.t + 2}, minf(6, h * 0.3f));
}

void brRender() {
    MakerBrowser &b = makerBrowser;
    b.buttons.clear();
    b.rendered = true;
    mkSky(b.age);
    if (b.loading || !makerCatalogReady) {
        mkLabelCentered(viewW * 0.5f, viewH * 0.5f, "Loading the course maker...", 12, 0xffffffff);
        return;
    }
    // Header: back button, logo and page selector, as on a world's level select.
    float margin = 10, headerH = clamp(viewH * 0.17f, 44, 66), cy = viewH - headerH * 0.5f;
    MakerButton *back = b.buttons.add(mkBox(margin - 4, cy - 18, 36, 36), BCMD_BACK, 0);
    mkDrawIcon(*back, MKART_BACK, b.mouseX, b.mouseY);
    brDrawLogo(viewW * 0.5f, cy, headerH - 8);
    char text[64];
    int pages = MAKER_SLOTS / MAKER_PAGE;
    wsprintfA(text, "%d / %d", b.page + 1, pages);
    float arrow = 24, pw = mkTextWidth(text, 9, true) + 16;
    float x = viewW - margin - arrow * 2 - pw - 4;
    MakerButton *prev = b.buttons.add(mkBox(x, cy - arrow * 0.5f, arrow, arrow), BCMD_PAGE, 0, -1);
    mkDrawArrow(*prev, true, b.mouseX, b.mouseY);
    Rect pill = mkBox(prev->r.r + 2, cy - 7, pw, 14);
    mkPill(pill);
    mkTextCentered((pill.l + pill.r) * 0.5f, cy, text, 9, 0xffffffff, true);
    MakerButton *next =
        b.buttons.add(mkBox(pill.r + 2, cy - arrow * 0.5f, arrow, arrow), BCMD_PAGE, 0, 1);
    mkDrawArrow(*next, false, b.mouseX, b.mouseY);
    // Course cards.
    float barH = 26, infoH = 13, gap = 7;
    float gridTop = viewH - headerH, gridBottom = barH + infoH + 9;
    int columns = 4, rows = MAKER_PAGE / columns;
    float cw = (viewW - margin * 2 - gap * (columns - 1)) / columns;
    float ch = (gridTop - gridBottom - gap * (rows - 1)) / rows;
    Rect cardRects[MAKER_PAGE];
    for (int i = 0; i < MAKER_PAGE; i++) {
        int slot = b.page * MAKER_PAGE + i;
        int c = i % columns, r = i / columns;
        Rect card = mkBox(margin + c * (cw + gap), gridTop - (r + 1) * ch - r * gap, cw, ch);
        cardRects[i] = card;
        brCard(slot, card);
        b.buttons.add(card, BCMD_CARD, 0, slot);
    }
    for (int i = 0; i < MAKER_PAGE; i++)
        brCardAward(b.page * MAKER_PAGE + i, cardRects[i]);
    bool exists = b.exists[b.selected];

    struct Spec {
        const char *label, *keyed;
        int id;
        bool disabled;
    };

    // Labels name their keys when the window is wide enough.
    const Spec specs[] = {{"Play", "Play (Enter)", BCMD_PLAY, !exists},
                          {"Edit", "Edit (E)", BCMD_EDIT, !exists},
                          {"New", "New (N)", BCMD_NEW, exists},
                          {"Copy", "Copy (C)", BCMD_COPY, !exists},
                          {"Delete", "Delete (Del)", BCMD_DELETE, !exists}};
    const int specCount = (int)(sizeof(specs) / sizeof(specs[0]));
    float size = 9, music = 24, keyedW = margin * 2 + music + 8;
    for (int i = 0; i < specCount; i++)
        keyedW += mkButtonWidth(specs[i].keyed, size) + 4;
    bool keyed = keyedW < viewW;
    x = margin;
    for (int i = 0; i < specCount; i++) {
        const char *label = keyed ? specs[i].keyed : specs[i].label;
        float w = mkButtonWidth(label, size);
        MakerButton *btn = b.buttons.add(mkBox(x, 6, w, barH - 6), specs[i].id, label, 0, false,
                                         specs[i].disabled);
        mkDrawButton(*btn, b.mouseX, b.mouseY, size,
                     specs[i].id == BCMD_DELETE ? 0xc80f0fff : 0x000000ff);
        x += w + 4;
    }
    // The title screen's music toggle; M does the same.
    MakerButton *tune =
        b.buttons.add(mkBox(viewW - margin - music, 4, music, music), BCMD_MUSIC, 0);
    bool tuneDown = mkInside(tune->r, b.mouseX, b.mouseY) && mkKeyDown(VK_LBUTTON);
    mkSprite(MKART_MUSIC,
             musicEnabled ? (tuneDown ? "down-on" : "up-on") : (tuneDown ? "down-off" : "up-off"),
             tune->r);
    char info[128], fitted[128];
    if (exists) {
        const MakerHeader &h = b.heads[b.selected];
        if (h.cleared)
            wsprintfA(info, "%s   %d tiles long   Best %d jumps   Gems %d/%d",
                      makerThemeNames[(int)clamp((float)h.theme, 1, 5)], h.width / 32, h.bestJumps,
                      h.bestCoins, brCoinGoal(h));
        else
            wsprintfA(info, "%s   %d tiles long   Par %d   Gold shoe %d",
                      makerThemeNames[(int)clamp((float)h.theme, 1, 5)], h.width / 32, h.jumpPar,
                      h.jumpBest);
    } else
        wsprintfA(info, "C-%02d is free   New (N) or a double-click starts a course here",
                  b.selected + 1);
    mkFitText(fitted, sizeof(fitted), info, viewW - margin * 2 - 12, 8, false);
    mkHudPlate(mkBox(margin, barH + 4, mkTextWidth(fitted, 8) + 12, infoH));
    mkText(margin + 6, barH + 4 + (infoH - mkLineHeight(8)) * 0.5f, fitted, 8, 0xffffffff);
    if (b.themePicker) {
        b.buttons.add(mkBox(0, 0, viewW, viewH), BCMD_THEME_CANCEL, 0);
        mkShade(0.85f);
        float rowH = clamp((viewH - 100) / 5, 24, 34);
        float w = minf(300, viewW - 20), h = rowH * 5 + 36, px = (viewW - w) * 0.5f,
              py = (viewH - h) * 0.5f - 6;
        mkGreenPanel(mkBox(px, py, w, h));
        b.buttons.add(mkBox(px, py, w, h), BCMD_BACKDROP, 0);
        mkPanelTitle(viewW * 0.5f, py + h, "Choose a theme");
        for (int t = 1; t <= 5; t++) {
            MakerButton *btn =
                b.buttons.add(mkBox(px + 14, py + h - 18 - t * rowH, w - 28, rowH - 4), BCMD_THEME,
                              makerThemeNames[t], t);
            brThemeChoice(*btn);
        }
    }
    if (b.confirm >= 0) {
        // In-game confirmation; a system dialog could hide behind the fullscreen window.
        b.buttons.add(mkBox(0, 0, viewW, viewH), BCMD_CONFIRM_CANCEL, 0);
        mkShade(0.85f);
        float w = minf(320, viewW - 20), h = 100, px = (viewW - w) * 0.5f, py = (viewH - h) * 0.5f;
        mkGreenPanel(mkBox(px, py, w, h));
        b.buttons.add(mkBox(px, py, w, h), BCMD_BACKDROP, 0);
        mkPanelTitle(viewW * 0.5f, py + h, "Delete course?");
        char name[64], question[96];
        mkFitText(name, sizeof(name), b.heads[b.confirm].name, w - 90, 10, true);
        wsprintfA(question, "C-%02d \"%s\"", b.confirm + 1, name);
        mkTextCentered(viewW * 0.5f, py + h - 30, question, 10, 0x000000ff, true);
        mkTextCentered(viewW * 0.5f, py + h - 47, "The course and its records cannot be restored.",
                       8, 0x1e3200ff);
        MakerButton *yes =
            b.buttons.add(mkBox(viewW * 0.5f - 96, py + 14, 88, 22), BCMD_CONFIRM_DELETE, "Delete");
        mkDrawButton(*yes, b.mouseX, b.mouseY, 9.5f, 0xc80f0fff);
        MakerButton *no =
            b.buttons.add(mkBox(viewW * 0.5f + 8, py + 14, 88, 22), BCMD_CONFIRM_CANCEL, "Cancel");
        mkDrawButton(*no, b.mouseX, b.mouseY, 9.5f);
        Rect f = b.confirmFocus == 1 ? yes->r : no->r;
        mkBrackets(Rect{f.l - 2, f.b - 2, f.r + 2, f.t + 2}, 5);
    }
    mkDrawToast();
}

void brSelect(int slot) {
    makerBrowser.selected = (int)clamp((float)slot, 0, MAKER_SLOTS - 1);
    makerBrowser.page = makerBrowser.selected / MAKER_PAGE;
    makerProfile.lastSlot = makerBrowser.selected;
    makerProfile.page = makerBrowser.page;
}

int brNextEmpty(int from) {
    for (int i = 0; i < MAKER_SLOTS; i++) {
        int slot = (from + i) % MAKER_SLOTS;
        if (!makerBrowser.exists[slot])
            return slot;
    }
    return -1;
}

void brCommand(int id, int arg) {
    MakerBrowser &b = makerBrowser;
    int slot = b.selected;
    switch (id) {
    case BCMD_CARD: {
        bool doubleClick = b.age - b.lastClick < 0.35f && slot == arg;
        b.lastClick = b.age;
        brSelect(arg);
        sound("switch", 1, 0.4f);
        if (doubleClick) {
            if (b.exists[arg])
                makerStartCourse(arg);
            else
                b.themePicker = true;
        }
        break;
    }
    case BCMD_PLAY:
        if (b.exists[slot])
            makerStartCourse(slot);
        break;
    case BCMD_EDIT:
        if (b.exists[slot])
            makerEditCourse(slot, false, 1);
        break;
    case BCMD_NEW:
        if (!b.exists[slot])
            b.themePicker = true;
        break;
    case BCMD_THEME:
        b.themePicker = false;
        makerEditCourse(slot, true, arg);
        break;
    case BCMD_THEME_CANCEL:
        b.themePicker = false;
        break;
    case BCMD_COPY: {
        if (!b.exists[slot])
            break;
        int target = brNextEmpty(slot + 1);
        if (target < 0) {
            mkToast("Every slot is in use", 0xff8080ff);
            break;
        }
        makerAllocate(makerCopyCourse);
        if (makerLoadLevel(slot, makerCopyCourse)) {
            MakerHeader &h = makerCopyCourse.head;
            h.uid = makerNewUid();
            makerResetRecords(h);
            if (lstrlenA(h.name) < MAKER_NAME - 8)
                lstrcatA(h.name, " copy");
            if (makerSaveLevel(target, makerCopyCourse)) {
                brRefresh();
                brSelect(target);
                mkToast("Course copied");
                sound("coin", 1, 0.5f);
            }
        }
        break;
    }
    case BCMD_DELETE:
        if (b.exists[slot]) {
            b.confirm = slot;
            b.confirmFocus = 0;
        }
        break;
    case BCMD_CONFIRM_DELETE:
        if (b.confirm >= 0 && b.exists[b.confirm]) {
            makerDeleteSlot(b.confirm);
            brRefresh();
            sound("smallexplosion", 1, 0.5f);
            mkToast("Course deleted");
        }
        b.confirm = -1;
        break;
    case BCMD_CONFIRM_CANCEL:
        b.confirm = -1;
        break;
    case BCMD_BACK:
        makerLeaveBrowser();
        break;
    case BCMD_PAGE: {
        int pages = MAKER_SLOTS / MAKER_PAGE;
        b.page = (b.page + arg + pages) % pages;
        brSelect(b.page * MAKER_PAGE + b.selected % MAKER_PAGE);
        break;
    }
    case BCMD_MUSIC:
        musicEnabled = !musicEnabled;
        if (musicEnabled)
            playMusic(3080012186760326ULL);
        else
            stopMusic();
        writeProgress();
        break;
    }
}

void brMouseMove(float x, float y) {
    makerBrowser.mouseX = x;
    makerBrowser.mouseY = y;
}

void brMouseDown(float x, float y, int button) {
    makerBrowser.mouseX = x;
    makerBrowser.mouseY = y;
    if (button != 0 || makerBrowser.loading)
        return;
    MakerButton *b = makerBrowser.buttons.hit(x, y);
    if (b)
        brCommand(b->id, b->arg);
}

void brWheel(int delta) {
    if (!makerBrowser.themePicker && makerBrowser.confirm < 0)
        brCommand(BCMD_PAGE, delta > 0 ? -1 : 1);
}

bool brKey(int key) {
    MakerBrowser &b = makerBrowser;
    if (b.loading)
        return true;
    if (key == VK_F11)
        return false;
    if (b.confirm >= 0) {
        if (key == 'Y' || (key == VK_RETURN && b.confirmFocus == 1))
            brCommand(BCMD_CONFIRM_DELETE, 0);
        else if (key == 'N' || key == VK_ESCAPE || key == VK_BACK || key == VK_RETURN)
            brCommand(BCMD_CONFIRM_CANCEL, 0);
        else if (key == VK_LEFT || key == VK_RIGHT || key == VK_TAB)
            b.confirmFocus ^= 1;
        return true;
    }
    if (b.themePicker) {
        if (key >= '1' && key <= '5')
            brCommand(BCMD_THEME, key - '0');
        else if (key == VK_ESCAPE || key == VK_BACK)
            b.themePicker = false;
        return true;
    }
    int slot = b.selected;
    switch (key) {
    case VK_LEFT:
        brSelect(slot - 1);
        break;
    case VK_RIGHT:
        brSelect(slot + 1);
        break;
    case VK_UP:
        brSelect(slot - 4);
        break;
    case VK_DOWN:
        brSelect(slot + 4);
        break;
    case VK_PRIOR:
        brCommand(BCMD_PAGE, -1);
        break;
    case VK_NEXT:
        brCommand(BCMD_PAGE, 1);
        break;
    case VK_RETURN:
    case VK_SPACE:
        if (b.exists[slot])
            makerStartCourse(slot);
        else
            b.themePicker = true;
        break;
    case 'E':
        brCommand(BCMD_EDIT, 0);
        break;
    case 'N':
        brCommand(BCMD_NEW, 0);
        break;
    case 'C':
        brCommand(BCMD_COPY, 0);
        break;
    case VK_DELETE:
        brCommand(BCMD_DELETE, 0);
        break;
    case VK_ESCAPE:
    case VK_BACK:
        makerLeaveBrowser();
        break;
    case 'M':
        brCommand(BCMD_MUSIC, 0);
        break;
    }
    return true;
}

void brTick(float dt) {
    makerBrowser.age += dt;
    makerToast.age += dt;
    if (makerBrowser.loading && makerBrowser.rendered) {
        makerBuildCatalog();
        makerBrowser.loading = false;
    }
    // Read one missing course photo of the page per frame.
    if (!makerBrowser.loading && makerCatalogReady)
        for (int i = 0; i < MAKER_PAGE; i++) {
            int slot = makerBrowser.page * MAKER_PAGE + i;
            const MakerPhoto &p = makerPhotos[i];
            if (makerBrowser.exists[slot] && (!p.ready || p.slot != slot)) {
                brBuildPhoto(slot);
                break;
            }
        }
}

void brOpen() {
    makerReadProfile();
    brRefresh();
    brSelect(makerProfile.lastSlot);
    makerBrowser.themePicker = false;
    makerBrowser.confirm = -1;
    makerBrowser.loading = !makerCatalogReady;
    makerBrowser.rendered = false;
}
