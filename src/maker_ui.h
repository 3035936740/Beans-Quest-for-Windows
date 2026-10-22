#pragma once

// Course-maker drawing primitives. Text uses the game's Vera face rasterised at the physical pixel
// size so small labels stay crisp at every window scale; panels, buttons and plates are drawn with
// the game's own interface sprites. Widgets are laid out per frame and hit-tested by events.

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
                             DEFAULT_PITCH | FF_SWISS, "Bitstream Vera Sans");
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

// Labels on the game's art: bold text with a one-pixel dark outline, like the menu captions.
void mkLabel(float x, float y, const char *s, float size, u32 color, u32 outline = 0x000000ff) {
    float o = 1 / screenScale;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx || dy)
                mkText(x + dx * o, y + dy * o, s, size, outline, true);
    mkText(x, y, s, size, color, true);
}

void mkLabelCentered(float cx, float cy, const char *s, float size, u32 color,
                     u32 outline = 0x000000ff) {
    mkLabel(cx - mkTextWidth(s, size, true) * 0.5f, cy - mkLineHeight(size, true) * 0.5f, s, size,
            color, outline);
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

bool mkKeyDown(int vk) {
    return (GetKeyState(vk) & 0x8000) != 0;
}

bool mkInside(Rect r, float x, float y) {
    return x >= r.l && x < r.r && y >= r.b && y < r.t;
}

Rect mkBox(float x, float y, float w, float h) {
    Rect r = {x, y, x + w, y + h};
    return r;
}

// ---------------------------------------------------------------------------------------------
// Game art. Panels, buttons, plates and badges reuse the game's interface sprites, so the maker
// shares the look of the pause menu, the stage-clear panel, the options menu and level select.

const u64 MKART_DARK_PANEL = 3527966682630614238ULL;  // pause menu frame
const u64 MKART_GREEN_PANEL = 8452152779744324638ULL; // stage-clear panel
const u64 MKART_BLACK_BOX = 448679042902946987ULL;    // stage-clear score box
const u64 MKART_PLATE = 12885853053182338668ULL;      // pause button caption plate
const u64 MKART_PILL = 8341643102278276ULL;           // pause caption capsule
const u64 MKART_HUD_PLATE = 10907988386711136615ULL;  // HUD gem counter backing
const u64 MKART_BUTTON = 10034553859779494184ULL;     // options menu button
const u64 MKART_CHECK = 3607268909778171ULL;          // options menu tick box
const u64 MKART_ARROW_LEFT = 17172113777224097406ULL; // touch control arrows
const u64 MKART_ARROW_RIGHT = 17488510565532112144ULL;
const u64 MKART_BACK = 17680337948191929366ULL;  // back button
const u64 MKART_PLAY = 13029306334769558266ULL;  // pause menu resume button
const u64 MKART_WRENCH = 1849765333517764545ULL; // title screen options button
const u64 MKART_EXIT = 7519969047596594ULL;      // title screen quit button
const u64 MKART_MUSIC = 4707166783708749431ULL;  // title screen music toggle
const u64 MKART_GRID = 16989260046005718766ULL;  // pause menu level select button
const u64 MKART_CLOSE = 120660973073ULL;         // level select minimise button
const u64 MKART_STAR = 9962941732219544898ULL;   // level select award star
const u64 MKART_SHADE = 16322964057250454020ULL; // 70% black dot stretched over scenes
const u64 MKART_CLOUD = 159837608269ULL;         // menu background clouds
const u64 MKART_CLOUD_SMALL[3] = {18085559403622411025ULL, 5037571874453516171ULL,
                                  5028962873047051163ULL};

// Level-select buttons in each world's colour, indexed by theme (internal world number).
const u64 MKART_WORLD_BUTTON[6] = {7603106210754899351ULL, 7603106210754899351ULL,
                                   3381796535512737044ULL, 196804610196ULL,
                                   567898218697ULL,        173550608266ULL};
// World title logos from level select, in the same order.
const u64 MKART_WORLD_TITLE[6] = {2648011722103052292ULL,  2648011722103052292ULL,
                                  10990406113345927767ULL, 7910687170783311935ULL,
                                  4899196007946967ULL,     6838280465874436ULL};

struct MakerArt {
    GLuint texture;
    float uv[8], w, h; // corner coordinates (bottom-left, bottom-right, top-left, top-right), size
};

bool mkArt(u64 klass, const char *anim, MakerArt &out, float time = 0) {
    Actor *a = actor(klass);
    Animation *an = animation(a, anim);
    if (!an || !an->count)
        return false;
    const Frame &f = an->frames[animationFrame(an, time)];
    if (f.texture < 0 || f.texture >= a->textureCount)
        return false;
    const Texture &t = a->textures[f.texture];
    out.texture = t.gl;
    memcpy(out.uv, f.uv, sizeof(out.uv));
    float bx = (f.uv[2] - f.uv[0]) * t.width, by = (f.uv[3] - f.uv[1]) * t.height;
    float lx = (f.uv[4] - f.uv[0]) * t.width, ly = (f.uv[5] - f.uv[1]) * t.height;
    out.w = root(bx * bx + by * by);
    out.h = root(lx * lx + ly * ly);
    return out.w > 0 && out.h > 0;
}

// Texture coordinate of a point inside a frame, from its four corner coordinates.
void mkUvAt(const float *uv, float fx, float fy, float *out) {
    for (int k = 0; k < 2; k++) {
        float bottom = uv[k] + (uv[2 + k] - uv[k]) * fx,
              top = uv[4 + k] + (uv[6 + k] - uv[4 + k]) * fx;
        out[k] = bottom + (top - bottom) * fy;
    }
}

// Pixel art stays crisp when each source pixel covers a whole number of screen pixels.
float mkPixelScale(float desired) {
    float pixels = (float)(int)(desired * screenScale + 0.5f);
    return (pixels < 1 ? 1 : pixels) / screenScale;
}

// Stretch a sprite over a rectangle keeping its border: corners keep their shape at `scale`
// logical units per source pixel, edges and centre stretch.
void mkNineArt(const MakerArt &a, Rect r, float insetX, float insetY, float scale,
               u32 tint = 0xffffffff) {
    if (r.r <= r.l || r.t <= r.b)
        return;
    float cx = minf(insetX * scale, (r.r - r.l) * 0.5f),
          cy = minf(insetY * scale, (r.t - r.b) * 0.5f);
    float xs[4] = {mkSnap(r.l), mkSnap(r.l + cx), mkSnap(r.r - cx), mkSnap(r.r)};
    float ys[4] = {mkSnap(r.b), mkSnap(r.b + cy), mkSnap(r.t - cy), mkSnap(r.t)};
    float ux = cx / scale / a.w, uy = cy / scale / a.h;
    float fx[4] = {0, ux, 1 - ux, 1}, fy[4] = {0, uy, 1 - uy, 1};
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, a.texture);
    mkColor(tint);
    glBegin(GL_QUADS);
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++) {
            if (xs[i + 1] <= xs[i] || ys[j + 1] <= ys[j])
                continue;
            float uv[2];
            mkUvAt(a.uv, fx[i], fy[j], uv);
            glTexCoord2f(uv[0], uv[1]);
            glVertex2f(xs[i], ys[j]);
            mkUvAt(a.uv, fx[i + 1], fy[j], uv);
            glTexCoord2f(uv[0], uv[1]);
            glVertex2f(xs[i + 1], ys[j]);
            mkUvAt(a.uv, fx[i + 1], fy[j + 1], uv);
            glTexCoord2f(uv[0], uv[1]);
            glVertex2f(xs[i + 1], ys[j + 1]);
            mkUvAt(a.uv, fx[i], fy[j + 1], uv);
            glTexCoord2f(uv[0], uv[1]);
            glVertex2f(xs[i], ys[j + 1]);
        }
    glEnd();
}

void mkNine(u64 klass, const char *anim, Rect r, float insetX, float insetY, float scale,
            u32 tint = 0xffffffff) {
    MakerArt a;
    if (mkArt(klass, anim, a))
        mkNineArt(a, r, insetX, insetY, scale, tint);
}

// Recolour only the editor's small button frame, retaining its original value and alpha.
// Each cached up/down texture is extracted from the atlas; campaign artwork stays untouched.
MakerArt makerThemeButtonArt[6][2];

void mkThemeButtonPixel(u8 *pixel, int theme) {
    float r = pixel[0] / 255.0f, g = pixel[1] / 255.0f, b = pixel[2] / 255.0f;
    float hi = maxf(r, maxf(g, b)), lo = minf(r, minf(g, b)), delta = hi - lo;
    if (delta < .01f || hi <= 0)
        return;
    float hue = hi == r ? (g - b) / delta : hi == g ? 2 + (b - r) / delta : 4 + (r - g) / delta;
    const float target[] = {0, 102, 310, 191, 22, 45};
    const float saturation[] = {1, .9f, .78f, .85f, 1, 1};
    hue += (target[theme] - 80) / 60;
    while (hue < 0)
        hue += 6;
    while (hue >= 6)
        hue -= 6;
    float sat = minf(1, delta / hi * saturation[theme]);
    int sector = (int)hue;
    float f = hue - sector, p = hi * (1 - sat), q = hi * (1 - sat * f),
          t = hi * (1 - sat * (1 - f));
    const float rgb[6][3] = {{hi, t, p}, {q, hi, p}, {p, hi, t},
                              {p, q, hi}, {t, p, hi}, {hi, p, q}};
    for (int i = 0; i < 3; i++)
        pixel[i] = (u8)(rgb[sector][i] * 255 + .5f);
}

void mkDeriveArt(const MakerArt &source, MakerArt &out, int theme) {
    glBindTexture(GL_TEXTURE_2D, source.texture);
    GLint aw = 0, ah = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &aw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &ah);
    int w = (int)(source.w + .5f), h = (int)(source.h + .5f), tw = 1, th = 1;
    while (tw < w)
        tw *= 2;
    while (th < h)
        th *= 2;
    u8 *atlas = (u8 *)HeapAlloc(GetProcessHeap(), 0, aw * ah * 4);
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tw * th * 4);
    if (!atlas || !pixels)
        fatal("Theme button allocation failed.");
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float uv[2];
            mkUvAt(source.uv, (x + .5f) / w, (y + .5f) / h, uv);
            int sx = (int)clamp((float)(int)(uv[0] * aw), 0, (float)aw - 1),
                sy = (int)clamp((float)(int)(uv[1] * ah), 0, (float)ah - 1);
            u8 *pixel = pixels + (y * tw + x) * 4;
            memcpy(pixel, atlas + (sy * aw + sx) * 4, 4);
            if (theme > 0)
                mkThemeButtonPixel(pixel, theme);
            else {
                pixel[3] = pixel[0] < 32 && pixel[1] < 32 && pixel[2] < 32 && pixel[3] > 128 ? 1 : 0;
                pixel[0] = pixel[1] = pixel[2] = 255;
            }
        }
    if (!theme) {
        // Keep only the black area connected to the score box's centre, excluding its outer
        // black outline. This uses the actual sprite silhouette rather than an estimated radius.
        int *queue = (int *)HeapAlloc(GetProcessHeap(), 0, w * h * sizeof(int));
        if (!queue)
            fatal("Photo mask allocation failed.");
        int read = 0, count = 0, start = (h / 2) * tw + w / 2;
        if (pixels[start * 4 + 3] == 1) {
            queue[count++] = start;
            pixels[start * 4 + 3] = 255;
        }
        while (read < count) {
            int at = queue[read++], x = at % tw, y = at / tw;
            const int next[] = {x > 0 ? at - 1 : -1, x + 1 < w ? at + 1 : -1,
                                 y > 0 ? at - tw : -1, y + 1 < h ? at + tw : -1};
            for (int i = 0; i < 4; i++)
                if (next[i] >= 0 && pixels[next[i] * 4 + 3] == 1) {
                    pixels[next[i] * 4 + 3] = 255;
                    queue[count++] = next[i];
                }
        }
        for (int i = 0; i < tw * th; i++)
            if (pixels[i * 4 + 3] != 255)
                pixels[i * 4 + 3] = 0;
        HeapFree(GetProcessHeap(), 0, queue);
    }
    out.w = (float)w;
    out.h = (float)h;
    const float uv[8] = {0, 0, w / (float)tw, 0, 0, h / (float)th,
                         w / (float)tw, h / (float)th};
    memcpy(out.uv, uv, sizeof(uv));
    glGenTextures(1, &out.texture);
    glBindTexture(GL_TEXTURE_2D, out.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    HeapFree(GetProcessHeap(), 0, atlas);
    HeapFree(GetProcessHeap(), 0, pixels);
}

const MakerArt *mkThemeButton(int theme, bool down) {
    theme = (int)clamp((float)theme, 1, 5);
    MakerArt &out = makerThemeButtonArt[theme][down ? 1 : 0];
    if (!out.texture) {
        MakerArt source;
        if (!mkArt(MKART_BUTTON, down ? "down" : "up", source))
            return 0;
        mkDeriveArt(source, out, theme);
    }
    return &out;
}

void mkBeginPhotoMask(Rect r) {
    static MakerArt mask;
    if (!mask.texture) {
        MakerArt source;
        if (!mkArt(MKART_BLACK_BOX, "", source))
            return;
        mkDeriveArt(source, mask, 0);
    }
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xff);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, .5f);
    mkNineArt(mask, r, 7, 7, mkPixelScale(.5f));
    glDisable(GL_ALPHA_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0);
    glStencilFunc(GL_EQUAL, 1, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

// Whole sprite stretched over a rectangle.
void mkSprite(u64 klass, const char *anim, Rect r, float alpha = 1, float time = 0,
              float angle = 0) {
    drawActor(actor(klass), r, anim, time, angle, false, -1, alpha);
}

// Whole sprite centred in a box at the largest size that keeps its proportions.
void mkSpriteFit(u64 klass, const char *anim, Rect box, float alpha = 1) {
    MakerArt a;
    if (!mkArt(klass, anim, a))
        return;
    float s = minf((box.r - box.l) / a.w, (box.t - box.b) / a.h);
    float cx = (box.l + box.r) * 0.5f, cy = (box.b + box.t) * 0.5f;
    mkSprite(
        klass, anim,
        Rect{cx - a.w * s * 0.5f, cy - a.h * s * 0.5f, cx + a.w * s * 0.5f, cy + a.h * s * 0.5f},
        alpha);
}

// Additive pass that brightens the art just drawn in a rectangle, for hover feedback.
void mkGlow(u64 klass, const char *anim, Rect r, float insetX, float insetY, float scale,
            float amount) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    mkNine(klass, anim, r, insetX, insetY, scale, 0xffffff00 | (u32)(int)(255 * amount));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Dark panel from the pause menu frame. Its centre is half transparent; `fill` darkens it with
// the game's shade (values above 1 add a second layer) so dense rows stay readable.
void mkDarkPanel(Rect r, float fill = 0.6f) {
    float scale = mkPixelScale(0.5f), in = 3 * scale;
    Rect inner = {r.l + in, r.b + in, r.r - in, r.t - in};
    for (; fill > 0; fill -= 1)
        mkSprite(MKART_SHADE, "", inner, minf(fill, 1));
    mkNine(MKART_DARK_PANEL, "", r, 8, 8, scale);
}

// Bevelled green panel from the stage-clear screen.
void mkGreenPanel(Rect r) {
    mkNine(MKART_GREEN_PANEL, "", r, 16, 16, mkPixelScale(0.5f));
}

// Black score box with its thin white rim, for thumbnails, slots and fields.
void mkInset(Rect r, float alpha = 1) {
    mkNine(MKART_BLACK_BOX, "", r, 7, 7, mkPixelScale(0.5f), 0xffffff00 | (u32)(int)(255 * alpha));
}

// Rounded black caption plate used under the pause menu buttons.
void mkPlate(Rect r, float alpha = 1) {
    float scale = mkPixelScale(minf(1, (r.t - r.b) / 24));
    mkNine(MKART_PLATE, "", r, 12, 12, scale, 0xffffff00 | (u32)(int)(255 * alpha));
}

// Black capsule behind the pause captions, for numbers and values.
void mkPill(Rect r, float alpha = 1) {
    float scale = mkPixelScale(minf(1, (r.t - r.b) / 16));
    mkNine(MKART_PILL, "", r, 8, 8, scale, 0xffffff00 | (u32)(int)(255 * alpha));
}

// Translucent plate behind the HUD gem counter, for status text over the course.
void mkHudPlate(Rect r) {
    mkNine(MKART_HUD_PLATE, "", r, 8, 8, mkPixelScale(minf(1, (r.t - r.b) / 16)));
}

// Darken the screen behind a dialog with the game's stretched shade dot.
void mkShade(float alpha = 1) {
    mkSprite(MKART_SHADE, "", Rect{0, 0, viewW, viewH}, alpha);
}

// Options-menu button art; pressed buttons use the darker frame.
void mkButtonArt(Rect r, bool down, bool hover, bool disabled, int theme = 0) {
    float scale = mkPixelScale(minf(1, (r.t - r.b) / 32));
    if (theme) {
        const MakerArt *art = mkThemeButton(theme, down);
        if (!art)
            return;
        mkNineArt(*art, r, 6, 6, scale, disabled ? 0x8c8c8cff : 0xffffffff);
        if (hover && !disabled) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            mkNineArt(*art, r, 6, 6, scale, 0xffffff2e);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        return;
    }
    mkNine(MKART_BUTTON, down ? "down" : "up", r, 6, 6, scale, disabled ? 0x8c8c8cff : 0xffffffff);
    if (hover && !disabled)
        mkGlow(MKART_BUTTON, down ? "down" : "up", r, 6, 6, scale, 0.18f);
}

// Level-select button in a theme's colour, stretched into a card or button frame. The art has an
// 8 pixel transparent border, so it is drawn that much larger to fill the rectangle.
void mkWorldFrame(Rect r, int theme, const char *anim = "up", float alpha = 1, float glow = 0) {
    u64 klass = MKART_WORLD_BUTTON[(int)clamp((float)theme, 0, 5)];
    float scale = mkPixelScale(minf(0.5f, (r.t - r.b) / 20)), m = 8 * scale;
    Rect art = {r.l - m, r.b - m, r.r + m, r.t + m};
    mkNine(klass, anim, art, 15, 15, scale, 0xffffff00 | (u32)(int)(255 * alpha));
    if (glow > 0)
        mkGlow(klass, anim, art, 15, 15, scale, glow);
}

// The game's focus marker: white corner brackets with a black outline.
void mkBrackets(Rect r, float length = 5) {
    for (int pass = 0; pass < 2; pass++) {
        float width = pass ? 0.75f : 2.0f;
        u32 c = pass ? 0xffffffff : 0x000000ff;
        const Vec corners[] = {{r.l, r.b}, {r.r, r.b}, {r.r, r.t}, {r.l, r.t}};
        for (int i = 0; i < 4; i++) {
            float x = corners[i].x, y = corners[i].y, sx = i == 0 || i == 3 ? 1.0f : -1.0f,
                  sy = i < 2 ? 1.0f : -1.0f;
            mkRect(minf(x, x + sx * length) - width * 0.5f, y - width * 0.5f, length + width, width,
                   c);
            mkRect(x - width * 0.5f, minf(y, y + sy * length) - width * 0.5f, width, length + width,
                   c);
        }
    }
}

// Remainder of v / m in [0, m), for scrolling decorations.
float mkWrap(float v, float m) {
    float q = v / m;
    int whole = (int)q;
    if ((float)whole > q)
        whole--;
    return v - whole * m;
}

// Menu backdrop: the options-menu sky with drifting clouds along the bottom.
void mkSky(float age) {
    mkGradient(0, 0, viewW, viewH, 0xa8e2ffff, 0x57c4ffff);
    for (int i = 0; i < 4; i++) {
        float w = 254 + i * 40.0f, speed = 6 + i * 3.0f;
        float x = mkWrap(age * speed + i * 260, viewW + w) - w,
              y = viewH * (0.55f + 0.1f * (i & 1));
        mkSprite(MKART_CLOUD_SMALL[i % 3], "", mkBox(x, y, w * 0.5f, w * 0.12f), 0.9f);
    }
    float cw = 241, ch = 84;
    for (float x = -mkWrap(age * 4, cw); x < viewW; x += cw)
        mkSprite(MKART_CLOUD, "", mkBox(x, -8, cw, ch));
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

// Text buttons are the options-menu buttons with their black bold captions (destructive ones
// use the menu's red). Toggled buttons stay pressed with a white caption; disabled ones grey out.
void mkDrawButton(const MakerButton &b, float mouseX, float mouseY, float size = 9,
                  u32 caption = 0x000000ff, int theme = 0) {
    bool hover = !b.disabled && mkInside(b.r, mouseX, mouseY);
    bool down = b.on || (hover && mkKeyDown(VK_LBUTTON));
    mkButtonArt(b.r, down, hover, b.disabled, theme);
    if (b.label && *b.label)
        mkTextCentered((b.r.l + b.r.r) * 0.5f, (b.r.b + b.r.t) * 0.5f - (down ? 0.5f : 0), b.label,
                       size,
                       b.disabled ? 0x00000070
                       : down     ? 0xffffffff
                                  : caption,
                       true);
}

// Width of a text button that fits its caption.
float mkButtonWidth(const char *label, float size, float pad = 12) {
    return mkTextWidth(label, size, true) + pad;
}

// Round touch-control arrow; it lights up under the pointer.
void mkDrawArrow(const MakerButton &b, bool left, float mouseX, float mouseY) {
    bool hover = !b.disabled && mkInside(b.r, mouseX, mouseY);
    float side = minf(b.r.r - b.r.l, b.r.t - b.r.b);
    Rect r = mkBox((b.r.l + b.r.r - side) * 0.5f, (b.r.b + b.r.t - side) * 0.5f, side, side);
    mkSprite(left ? MKART_ARROW_LEFT : MKART_ARROW_RIGHT, hover ? "on" : "off", r,
             b.disabled ? 0.3f : 1);
}

// A game icon button (back, play, options...): it shows its pressed frame under the pointer.
void mkDrawIcon(const MakerButton &b, u64 klass, float mouseX, float mouseY) {
    bool hover = !b.disabled && mkInside(b.r, mouseX, mouseY);
    float side = minf(b.r.r - b.r.l, b.r.t - b.r.b);
    Rect r = mkBox((b.r.l + b.r.r - side) * 0.5f, (b.r.b + b.r.t - side) * 0.5f, side, side);
    mkSprite(klass, hover || b.on ? "down" : "up", r, b.disabled ? 0.4f : 1);
}

// Dialog title: a yellow caption on a black plate straddling the panel's top edge.
void mkPanelTitle(float cx, float top, const char *title, float size = 11) {
    float w = mkTextWidth(title, size, true) + 24, h = mkLineHeight(size, true) + 8;
    mkPlate(mkBox(cx - w * 0.5f, top - h * 0.5f, w, h));
    mkTextCentered(cx, top, title, size, 0xffee33ff, true);
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

// Toasts sit on the pause menu's caption plate.
void mkDrawToast() {
    if (makerToast.age >= makerToast.life || !makerToast.text[0])
        return;
    float alpha = clamp((makerToast.life - makerToast.age) * 3, 0, 1);
    float size = 10, w = mkTextWidth(makerToast.text, size, true) + 28, h = 22;
    float x = (viewW - w) * 0.5f, y = viewH * 0.5f - 11;
    mkPlate(mkBox(x, y, w, h), alpha * 0.92f);
    mkText(x + 14, y + (h - mkLineHeight(size, true)) * 0.5f, makerToast.text, size,
           (makerToast.color & 0xffffff00) | (u32)(int)(255 * alpha), true);
}
