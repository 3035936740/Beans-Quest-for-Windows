#pragma once

// Course-maker drawing primitives. Glyphs are rasterised at the physical pixel size so small editor
// labels stay crisp at every window scale; widgets are laid out per frame and hit-tested by events.

struct MakerGlyphFont {
    GLuint texture;
    int pixels, cell, atlasW, atlasH, columns;
    bool bold;
    float advance[96], lineHeight;
};

MakerGlyphFont makerFonts[8];
int makerFontCount, makerFontEvict;

MakerGlyphFont *makerFont(float size, bool bold) {
    int pixels = (int)(size * screenScale + 0.5f);
    pixels = pixels < 7 ? 7 : pixels > 48 ? 48 : pixels;
    for (int i = 0; i < makerFontCount; i++)
        if (makerFonts[i].pixels == pixels && makerFonts[i].bold == bold)
            return makerFonts + i;
    MakerGlyphFont *f;
    if (makerFontCount < 8)
        f = makerFonts + makerFontCount++;
    else {
        f = makerFonts + makerFontEvict;
        makerFontEvict = (makerFontEvict + 1) % 8;
        glDeleteTextures(1, &f->texture);
    }
    memset(f, 0, sizeof(*f));
    f->pixels = pixels;
    f->bold = bold;
    f->cell = pixels * 3 / 2 + 4;
    f->atlasW = 1024;
    f->columns = f->atlasW / f->cell;
    int rows = (96 + f->columns - 1) / f->columns;
    f->atlasH = 64;
    while (f->atlasH < rows * f->cell)
        f->atlasH *= 2;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = f->atlasW;
    bi.bmiHeader.biHeight = -f->atlasH;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void *pixelsOut = 0;
    HDC dc = CreateCompatibleDC(0);
    HBITMAP bitmap = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &pixelsOut, 0, 0);
    if (!bitmap || !pixelsOut) {
        DeleteDC(dc);
        return f;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    memset(pixelsOut, 0, f->atlasW * f->atlasH * 4);
    HFONT font = CreateFontA(-pixels, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, "Tahoma");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    TEXTMETRICA metrics;
    GetTextMetricsA(dc, &metrics);
    f->lineHeight = (float)metrics.tmHeight;
    for (int c = 32; c < 128; c++) {
        char ch = (char)(c == 127 ? '?' : c);
        SIZE extent;
        GetTextExtentPoint32A(dc, &ch, 1, &extent);
        f->advance[c - 32] = (float)extent.cx;
        int slot = c - 32;
        TextOutA(dc, (slot % f->columns) * f->cell + 2, (slot / f->columns) * f->cell + 1, &ch, 1);
    }
    GdiFlush();
    u8 *bytes = (u8 *)pixelsOut;
    for (int i = 0; i < f->atlasW * f->atlasH; i++) {
        bytes[i * 4 + 3] = bytes[i * 4 + 2];
        bytes[i * 4] = bytes[i * 4 + 1] = bytes[i * 4 + 2] = 255;
    }
    glGenTextures(1, &f->texture);
    glBindTexture(GL_TEXTURE_2D, f->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, f->atlasW, f->atlasH, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixelsOut);
    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return f;
}

// Colours are 0xRRGGBBAA.
void mkColor(u32 c, float alphaScale = 1) {
    glColor4f(((c >> 24) & 255) / 255.0f, ((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f,
              (c & 255) / 255.0f * alphaScale);
}

float mkSnap(float v) {
    float p = v * screenScale;
    return (float)(int)(p + (p < 0 ? -0.5f : 0.5f)) / screenScale;
}

float mkTextWidth(const char *s, float size, bool bold = false) {
    MakerGlyphFont *f = makerFont(size, bold);
    float w = 0;
    for (; *s; s++) {
        u8 c = (u8)*s;
        if (c >= 32 && c < 128)
            w += f->advance[c - 32];
    }
    return w / screenScale;
}

float mkLineHeight(float size, bool bold = false) {
    return makerFont(size, bold)->lineHeight / screenScale;
}

// Draw one line with its line box starting at y (bottom) and x (left).
void mkText(float x, float y, const char *s, float size, u32 color, bool bold = false) {
    MakerGlyphFont *f = makerFont(size, bold);
    if (!f->texture)
        return;
    float px = mkSnap(x) * screenScale, top = mkSnap(y) * screenScale + f->lineHeight + 1;
    float inv = 1 / screenScale, cell = (float)f->cell;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, f->texture);
    mkColor(color);
    glBegin(GL_QUADS);
    for (; *s; s++) {
        u8 c = (u8)*s;
        if (c < 32 || c >= 128)
            continue;
        int slot = c - 32;
        float u0 = (slot % f->columns) * cell / f->atlasW,
              v0 = (slot / f->columns) * cell / f->atlasH;
        float u1 = u0 + cell / f->atlasW, v1 = v0 + cell / f->atlasH;
        float l = (px - 2) * inv, r = (px - 2 + cell) * inv, t = top * inv, b = (top - cell) * inv;
        glTexCoord2f(u0, v1);
        glVertex2f(l, b);
        glTexCoord2f(u1, v1);
        glVertex2f(r, b);
        glTexCoord2f(u1, v0);
        glVertex2f(r, t);
        glTexCoord2f(u0, v0);
        glVertex2f(l, t);
        px += f->advance[slot];
    }
    glEnd();
}

void mkTextShadow(float x, float y, const char *s, float size, u32 color, bool bold = false) {
    mkText(x + 1 / screenScale, y - 1 / screenScale, s, size, 0x000000a0, bold);
    mkText(x, y, s, size, color, bold);
}

// Word-wrapped text, first line at y and later lines stepping down; returns the lines drawn.
int mkTextWrapped(float x, float y, const char *s, float size, u32 color, float maxWidth,
                  float lineStep, int maxLines) {
    int lines = 0;
    while (*s && lines < maxLines) {
        char line[128];
        int n = 0, fit = 0;
        while (s[n] && n < 127) {
            line[n] = s[n];
            line[n + 1] = 0;
            if (mkTextWidth(line, size) > maxWidth)
                break;
            n++;
            if (!s[n] || s[n] == ' ')
                fit = n;
        }
        if (!fit)
            fit = n > 0 ? n : 1;
        lstrcpynA(line, s, fit + 1);
        mkText(x, y - lines * lineStep, line, size, color);
        lines++;
        s += fit;
        while (*s == ' ')
            s++;
    }
    return lines;
}

// Copy text shortened with an ellipsis so it fits maxWidth.
void mkFitText(char *out, int size, const char *s, float maxWidth, float textSize, bool bold) {
    lstrcpynA(out, s, size);
    if (mkTextWidth(out, textSize, bold) <= maxWidth)
        return;
    for (int n = lstrlenA(out); n > 0; n--) {
        lstrcpynA(out, s, n + 1);
        if (n + 3 < size)
            lstrcatA(out, "...");
        if (mkTextWidth(out, textSize, bold) <= maxWidth)
            return;
    }
    out[0] = 0;
}

void mkTextCentered(float cx, float cy, const char *s, float size, u32 color, bool bold = false) {
    mkText(cx - mkTextWidth(s, size, bold) * 0.5f, cy - mkLineHeight(size, bold) * 0.5f, s, size,
           color, bold);
}

void mkRect(float x, float y, float w, float h, u32 color) {
    glDisable(GL_TEXTURE_2D);
    mkColor(color);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void mkGradient(float x, float y, float w, float h, u32 bottom, u32 top) {
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    mkColor(bottom);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    mkColor(top);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void mkFrame(float x, float y, float w, float h, u32 color, float width = 0) {
    if (width <= 0)
        width = 1 / screenScale;
    mkRect(x, y, w, width, color);
    mkRect(x, y + h - width, w, width, color);
    mkRect(x, y, width, h, color);
    mkRect(x + w - width, y, width, h, color);
}

void mkLine(float x0, float y0, float x1, float y1, u32 color, float width = 1) {
    glDisable(GL_TEXTURE_2D);
    glLineWidth(width);
    mkColor(color);
    glBegin(GL_LINES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glEnd();
    glLineWidth(1);
}

void mkTriangle(float x0, float y0, float x1, float y1, float x2, float y2, u32 color) {
    glDisable(GL_TEXTURE_2D);
    mkColor(color);
    glBegin(GL_TRIANGLES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

bool mkInside(Rect r, float x, float y) {
    return x >= r.l && x < r.r && y >= r.b && y < r.t;
}

Rect mkBox(float x, float y, float w, float h) {
    Rect r = {x, y, x + w, y + h};
    return r;
}

// A clickable widget recorded by the layout pass; events and drawing both use these rectangles.
struct MakerButton {
    Rect r;
    int id, arg;
    const char *label;
    bool on, disabled;
};

struct MakerButtons {
    MakerButton items[160];
    int count;

    void clear() {
        count = 0;
    }

    MakerButton *add(Rect r, int id, const char *label, int arg = 0, bool on = false,
                     bool disabled = false) {
        if (count >= 160)
            return items + 159;
        MakerButton &b = items[count++];
        b.r = r;
        b.id = id;
        b.arg = arg;
        b.label = label;
        b.on = on;
        b.disabled = disabled;
        return &b;
    }

    MakerButton *hit(float x, float y) {
        for (int i = count - 1; i >= 0; i--)
            if (!items[i].disabled && mkInside(items[i].r, x, y))
                return items + i;
        return 0;
    }
};

// Shared look for maker buttons: dark glass with a warm highlight for toggles and hover.
void mkDrawButton(const MakerButton &b, float mouseX, float mouseY, float size = 9) {
    bool hover = !b.disabled && mkInside(b.r, mouseX, mouseY);
    u32 bottom = b.on ? 0xd08a1cff : hover ? 0x3d5a80ff : 0x243447ff;
    u32 top = b.on ? 0xf6c445ff : hover ? 0x5a7fb0ff : 0x34495eff;
    mkGradient(b.r.l, b.r.b, b.r.r - b.r.l, b.r.t - b.r.b, bottom, top);
    mkFrame(b.r.l, b.r.b, b.r.r - b.r.l, b.r.t - b.r.b, b.on ? 0xfff2b0ff : 0x0b1520ff);
    if (b.label && *b.label)
        mkTextCentered((b.r.l + b.r.r) * 0.5f, (b.r.b + b.r.t) * 0.5f, b.label, size,
                       b.disabled ? 0x8090a0ff
                       : b.on     ? 0x2a1600ff
                                  : 0xf2f6faff,
                       b.on);
}

void mkPanel(float x, float y, float w, float h, u32 fill = 0x101a26ee) {
    mkRect(x + 2 / screenScale, y - 2 / screenScale, w, h, 0x00000060);
    mkRect(x, y, w, h, fill);
    mkFrame(x, y, w, h, 0x6c8db5ff);
}

// Format a float with fixed decimals; wsprintf has no floating point support.
void mkFormat(char *out, float value, int decimals) {
    char *p = out;
    if (value < 0) {
        *p++ = '-';
        value = -value;
    }
    int scale = decimals == 0 ? 1 : decimals == 1 ? 10 : decimals == 2 ? 100 : 1000;
    int whole = (int)(value * scale + 0.5f);
    wsprintfA(p, "%d", whole / scale);
    if (decimals > 0) {
        p += lstrlenA(p);
        *p++ = '.';
        int frac = whole % scale;
        for (int d = scale / 10; d > 0; d /= 10) {
            *p++ = (char)('0' + frac / d);
            frac %= d;
        }
        *p = 0;
    }
}

void *mkAlloc(u32 n) {
    void *p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n ? n : 1);
    if (!p)
        fatal("Not enough memory for the course maker.");
    return p;
}

void mkFree(void *p) {
    if (p)
        HeapFree(GetProcessHeap(), 0, p);
}

u8 *mkMoveBuffer;
u32 mkMoveCapacity;

// Overlapping move through a staging buffer: the CRT-free runtime has memcpy but no memmove.
void mkMove(void *dst, const void *src, u32 n) {
    if (dst == src || !n)
        return;
    if (n > mkMoveCapacity) {
        mkFree(mkMoveBuffer);
        mkMoveCapacity = n + (n >> 1);
        mkMoveBuffer = (u8 *)mkAlloc(mkMoveCapacity);
    }
    memcpy(mkMoveBuffer, src, n);
    memcpy(dst, mkMoveBuffer, n);
}

bool mkKeyDown(int vk) {
    return (GetKeyState(vk) & 0x8000) != 0;
}

// A short-lived message shown near the bottom of maker screens.
struct MakerToast {
    char text[96];
    float age, life;
    u32 color;
};

MakerToast makerToast;

void mkToast(const char *text, u32 color = 0xffffffff, float life = 2.2f) {
    lstrcpynA(makerToast.text, text, sizeof(makerToast.text));
    makerToast.age = 0;
    makerToast.life = life;
    makerToast.color = color;
}

void mkDrawToast() {
    if (makerToast.age >= makerToast.life || !makerToast.text[0])
        return;
    float alpha = clamp((makerToast.life - makerToast.age) * 3, 0, 1);
    float size = 10, w = mkTextWidth(makerToast.text, size, true) + 20, h = 20;
    float x = (viewW - w) * 0.5f, y = viewH * 0.5f - 10;
    mkRect(x, y, w, h, (u32)(int)(200 * alpha));
    mkText(x + 10, y + (h - mkLineHeight(size, true)) * 0.5f, makerToast.text, size,
           (makerToast.color & 0xffffff00) | (u32)(int)(255 * alpha), true);
}
