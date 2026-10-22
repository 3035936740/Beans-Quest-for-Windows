#pragma once

// Course list of the maker chapter: pages of course cards with schematic thumbnails, award badges
// and verification state. Creating a course asks for a theme first, like choosing a course style.

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
    BCMD_CONFIRM_CANCEL
};

struct MakerBrowser {
    MakerHeader heads[MAKER_SLOTS];
    bool exists[MAKER_SLOTS], thumbValid[MAKER_SLOTS];
    GLuint thumbs[MAKER_SLOTS];
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

void brRefresh() {
    for (int i = 0; i < MAKER_SLOTS; i++) {
        makerBrowser.exists[i] = makerLoadHeader(i, makerBrowser.heads[i]);
        makerBrowser.thumbValid[i] = false;
    }
}

u32 brThumbColor(int code, int theme) {
    const u32 sky[6] = {0x5a8cc8ff, 0x5a8cc8ff, 0x7aa0d8ff, 0xe0a070ff, 0xd89060ff, 0x6a5aa0ff};
    switch (code) {
    case THUMB_SKY:
        return sky[(int)clamp((float)theme, 0, 5)];
    case THUMB_BACK:
        return 0x40506aff;
    case THUMB_SOLID:
        return 0x8a6a46ff;
    case THUMB_DECOR:
        return 0x6aa85aff;
    case THUMB_FRONT:
        return 0x3c6a40ff;
    case THUMB_HAZARD:
        return 0xe04848ff;
    case THUMB_ITEM:
        return 0x60e0ffff;
    case THUMB_GOAL:
        return 0xffd840ff;
    case THUMB_OBJECT:
        return 0xd8d8e0ff;
    case THUMB_START:
        return 0x80ff80ff;
    }
    return 0x141820ff;
}

GLuint brThumb(int slot) {
    if (makerBrowser.thumbValid[slot])
        return makerBrowser.thumbs[slot];
    const MakerHeader &h = makerBrowser.heads[slot];
    u8 pixels[MAKER_THUMB_W * MAKER_THUMB_H * 4];
    for (int i = 0; i < MAKER_THUMB_W * MAKER_THUMB_H; i++) {
        u32 c = brThumbColor(h.thumb[i], h.theme);
        pixels[i * 4] = (u8)(c >> 24);
        pixels[i * 4 + 1] = (u8)(c >> 16);
        pixels[i * 4 + 2] = (u8)(c >> 8);
        pixels[i * 4 + 3] = (u8)c;
    }
    if (!makerBrowser.thumbs[slot])
        glGenTextures(1, &makerBrowser.thumbs[slot]);
    glBindTexture(GL_TEXTURE_2D, makerBrowser.thumbs[slot]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MAKER_THUMB_W, MAKER_THUMB_H, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
    makerBrowser.thumbValid[slot] = true;
    return makerBrowser.thumbs[slot];
}

int brCoinGoal(const MakerHeader &h) {
    return h.coinGoal > 0 && h.coinGoal < h.coins ? h.coinGoal : h.coins;
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

void brCard(int slot, Rect r) {
    bool selected = slot == makerBrowser.selected;
    bool hover = mkInside(r, makerBrowser.mouseX, makerBrowser.mouseY);
    float w = r.r - r.l, h = r.t - r.b;
    u32 top = selected ? 0x3e5f8aff : hover ? 0x30496aff : 0x223349ff;
    mkGradient(r.l, r.b, w, h, 0x141f2cff, top);
    mkFrame(r.l, r.b, w, h, selected ? 0xffe070ff : 0x4a6584ff, selected ? 2 / screenScale : 0);
    char label[64];
    wsprintfA(label, "C-%02d", slot + 1);
    mkText(r.l + 5, r.t - 13, label, 8, 0x9fb4c8ff, true);
    if (!makerBrowser.exists[slot]) {
        mkTextCentered((r.l + r.r) * 0.5f, (r.b + r.t) * 0.5f, "+ New course", 9,
                       hover || selected ? 0xffffffff : 0x8fa3b8ff, true);
        return;
    }
    const MakerHeader &head = makerBrowser.heads[slot];
    mkFitText(label, sizeof(label), head.name, r.r - r.l - 39, 8.5f, true);
    mkText(r.l + 34, r.t - 13, label, 8.5f, 0xffffffff, true);
    float tw = w - 10, th = tw * MAKER_THUMB_H / MAKER_THUMB_W;
    if (th > h - 40) {
        th = h - 40;
        tw = th * MAKER_THUMB_W / MAKER_THUMB_H;
    }
    float tx = r.l + (w - tw) * 0.5f, ty = r.t - 17 - th;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, brThumb(slot));
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(tx, ty);
    glTexCoord2f(1, 0);
    glVertex2f(tx + tw, ty);
    glTexCoord2f(1, 1);
    glVertex2f(tx + tw, ty + th);
    glTexCoord2f(0, 1);
    glVertex2f(tx, ty + th);
    glEnd();
    mkFrame(tx, ty, tw, th, 0x00000080);
    brBadges(head, r.l + 5, r.b + 4, 12);
    if (head.flags & MAKER_VERIFIED)
        mkText(r.r - mkTextWidth("Verified", 7.5f, true) - 5, r.b + 5, "Verified", 7.5f, 0x80ff80ff,
               true);
    else
        mkText(r.r - mkTextWidth("Not verified", 7.5f) - 5, r.b + 5, "Not verified", 7.5f,
               0x8090a0ff);
}

void brRender() {
    MakerBrowser &b = makerBrowser;
    b.buttons.clear();
    b.rendered = true;
    mkGradient(0, 0, viewW, viewH, 0x0b1a2aff, 0x2b5a7aff);
    for (int i = 0; i < 9; i++) {
        float x = makerFloor(b.age * 12 + i * 120) % (int)(viewW + 240) - 120.0f;
        mkRect(x, viewH - 40 - (i % 3) * 30, 90, 10, 0xffffff10);
    }
    if (b.loading || !makerCatalogReady) {
        mkTextCentered(viewW * 0.5f, viewH * 0.5f, "Loading the course maker...", 12, 0xffffffff,
                       true);
        return;
    }
    float titleH = 30;
    mkTextShadow(12, viewH - 24, "COURSE MAKER", 14, 0xfff2b0ff, true);
    char text[64];
    int pages = MAKER_SLOTS / MAKER_PAGE;
    wsprintfA(text, "Page %d / %d", b.page + 1, pages);
    float pw = mkTextWidth(text, 9, true);
    MakerButton *prev =
        b.buttons.add(mkBox(viewW - pw - 70, viewH - 25, 22, 18), BCMD_PAGE, "<", -1);
    mkDrawButton(*prev, b.mouseX, b.mouseY, 9);
    mkText(viewW - pw - 44, viewH - 22, text, 9, 0xffffffff, true);
    MakerButton *next = b.buttons.add(mkBox(viewW - 38, viewH - 25, 22, 18), BCMD_PAGE, ">", 1);
    mkDrawButton(*next, b.mouseX, b.mouseY, 9);
    float barH = 30, margin = 10, gap = 6, infoH = 12;
    float gridTop = viewH - titleH - 4, gridBottom = barH + infoH + 6;
    int columns = 4, rows = MAKER_PAGE / columns;
    float cw = (viewW - margin * 2 - gap * (columns - 1)) / columns;
    float ch = (gridTop - gridBottom - gap * (rows - 1)) / rows;
    for (int i = 0; i < MAKER_PAGE; i++) {
        int slot = b.page * MAKER_PAGE + i;
        int c = i % columns, r = i / columns;
        Rect card = mkBox(margin + c * (cw + gap), gridTop - (r + 1) * ch - r * gap, cw, ch);
        brCard(slot, card);
        b.buttons.add(card, BCMD_CARD, 0, slot);
    }
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
                          {"Delete", "Delete (Del)", BCMD_DELETE, !exists},
                          {"Back", "Back (Esc)", BCMD_BACK, false}};
    float keyedW = margin * 2 + 20;
    for (int i = 0; i < 6; i++)
        keyedW += mkTextWidth(specs[i].keyed, 9, true) + 26;
    bool keyed = keyedW < viewW;
    float x = margin;
    for (int i = 0; i < 6; i++) {
        const char *label = keyed ? specs[i].keyed : specs[i].label;
        float w = mkTextWidth(label, 9, true) + 22;
        if (i == 5)
            x = viewW - margin - w;
        MakerButton *btn = b.buttons.add(mkBox(x, 6, w, barH - 10), specs[i].id, label, 0, false,
                                         specs[i].disabled);
        mkDrawButton(*btn, b.mouseX, b.mouseY, 9);
        x += w + 4;
    }
    if (exists) {
        const MakerHeader &h = b.heads[b.selected];
        char info[128];
        if (h.cleared)
            wsprintfA(info, "%s   %d tiles long   Best %d jumps   Gems %d/%d",
                      makerThemeNames[(int)clamp((float)h.theme, 1, 5)], h.width / 32, h.bestJumps,
                      h.bestCoins, brCoinGoal(h));
        else
            wsprintfA(info, "%s   %d tiles long   Par %d   Gold shoe %d",
                      makerThemeNames[(int)clamp((float)h.theme, 1, 5)], h.width / 32, h.jumpPar,
                      h.jumpBest);
        char fitted[128];
        mkFitText(fitted, sizeof(fitted), info, viewW - margin * 2, 8, false);
        mkText(margin, barH - 2, fitted, 8, 0xc8d8e8ff);
    }
    if (b.themePicker) {
        b.buttons.add(mkBox(0, 0, viewW, viewH), BCMD_THEME_CANCEL, 0);
        mkRect(0, 0, viewW, viewH, 0x00000090);
        float w = minf(300, viewW - 20), h = 168, px = (viewW - w) * 0.5f, py = (viewH - h) * 0.5f;
        mkPanel(px, py, w, h);
        b.buttons.add(mkBox(px, py, w, h), BCMD_BACKDROP, 0);
        mkTextCentered(viewW * 0.5f, py + h - 16, "Choose a theme", 11, 0xfff2b0ff, true);
        for (int t = 1; t <= 5; t++) {
            MakerButton *btn = b.buttons.add(mkBox(px + 20, py + h - 30 - t * 24, w - 40, 20),
                                             BCMD_THEME, makerThemeNames[t], t);
            mkDrawButton(*btn, b.mouseX, b.mouseY, 9.5f);
        }
    }
    if (b.confirm >= 0) {
        // In-game confirmation; a system dialog could hide behind the fullscreen window.
        b.buttons.add(mkBox(0, 0, viewW, viewH), BCMD_CONFIRM_CANCEL, 0);
        mkRect(0, 0, viewW, viewH, 0x00000090);
        float w = minf(320, viewW - 20), h = 92, px = (viewW - w) * 0.5f, py = (viewH - h) * 0.5f;
        mkPanel(px, py, w, h);
        b.buttons.add(mkBox(px, py, w, h), BCMD_BACKDROP, 0);
        char name[64], question[96];
        mkFitText(name, sizeof(name), b.heads[b.confirm].name, w - 120, 10, true);
        wsprintfA(question, "Delete C-%02d \"%s\"?", b.confirm + 1, name);
        mkTextCentered(viewW * 0.5f, py + h - 20, question, 10, 0xfff2b0ff, true);
        mkTextCentered(viewW * 0.5f, py + h - 38, "The course and its records cannot be restored.",
                       8, 0xc8d8e8ff);
        MakerButton *yes = b.buttons.add(mkBox(viewW * 0.5f - 96, py + 10, 88, 20),
                                         BCMD_CONFIRM_DELETE, "Delete", 0, b.confirmFocus == 1);
        mkDrawButton(*yes, b.mouseX, b.mouseY, 9.5f);
        MakerButton *no = b.buttons.add(mkBox(viewW * 0.5f + 8, py + 10, 88, 20),
                                        BCMD_CONFIRM_CANCEL, "Cancel", 0, b.confirmFocus == 0);
        mkDrawButton(*no, b.mouseX, b.mouseY, 9.5f);
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
        musicEnabled = !musicEnabled;
        if (musicEnabled)
            playMusic(3080012186760326ULL);
        else
            stopMusic();
        writeProgress();
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
