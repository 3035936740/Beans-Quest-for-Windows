#pragma once

// Course-maker catalog. Atlas pieces are harvested from the authored stages of every world, so any
// course may mix Grasslands, Sky Ruins or Wizard's Lair bricks. Default collision comes from piece
// opacity and from the authored one-way lines that sit on top of the same piece.

Arena makerScratch;

void makerScratchReset() {
    if (!makerScratch.data)
        makerScratch.init(20 * 1024 * 1024);
    makerScratch.reset();
}

// Authored stage data without GPU uploads: textures are skipped, objects are optional.
struct MakerScan {
    Vec size;
    u64 id;
    Props props;
    int layerCount;
    Layer *layers;
    int group;
    int tileCount;
    Tile *tiles;
    int geometryCount;
    Geometry *geometry;
    int objectCount;
    Object *objects;
};

int makerGroupFromName(const char *name) {
    if (name[0] == 'w' && name[1] == 'o' && name[2] == 'r' && name[3] == 'l' && name[4] == 'd' &&
        name[5] >= '1' && name[5] <= '5')
        return name[5] - '0';
    return 0;
}

bool makerScanLevel(const char *file, MakerScan &s, Arena &memory, bool withObjects) {
    memset(&s, 0, sizeof(s));
    char path[160];
    lstrcpyA(path, "data/Media/Levels/");
    lstrcatA(path, file);
    u32 n;
    u8 *b = readFile(path, &n, memory, false);
    if (!b)
        return false;
    Reader r(b, n, memory);
    r.take(16);
    s.size = *(Vec *)r.take(8);
    s.id = r.q();
    s.props = r.props();
    s.layerCount = r.i();
    s.layers = memory.array<Layer>(s.layerCount);
    for (int i = 0; i < s.layerCount; i++)
        memcpy(s.layers + i, r.take(44), 44);
    r.take(16);
    int textures = r.i();
    if (textures == -1) {
        r.s();
        s.group = makerGroupFromName(r.s());
    } else
        for (int i = 0; i < textures; i++) {
            r.take(16);
            int h[10];
            memcpy(h, r.take(40), 40);
            r.take(h[9]);
            for (int mip = 1; mip < h[2]; mip++)
                r.take(r.i());
        }
    s.tileCount = r.i();
    s.tiles = memory.array<Tile>(s.tileCount);
    for (int i = 0; i < s.tileCount; i++) {
        Tile &t = s.tiles[i];
        memcpy(&t, r.take(60), 60);
        if (t.bytes < 0 || t.bytes % 64)
            return false;
        t.vertices = (Vertex *)r.take(t.bytes);
    }
    r.take(16);
    s.geometryCount = r.i();
    s.geometry = memory.array<Geometry>(s.geometryCount);
    for (int i = 0; i < s.geometryCount; i++) {
        Geometry &g = s.geometry[i];
        g.kind = r.i();
        g.flags = r.i();
        g.count = r.i();
        g.points = memory.array<Vec>(g.count);
        for (int j = 0; j < g.count; j++) {
            g.points[j].x = (float)r.i();
            g.points[j].y = (float)r.i();
        }
    }
    if (withObjects) {
        s.objectCount = r.i();
        s.objects = memory.array<Object>(s.objectCount);
        for (int i = 0; i < s.objectCount; i++) {
            s.objects[i] = objectRead(r);
            s.objects[i].layer = r.i();
        }
    }
    return true;
}

// GPU pages of the worldN texture groups, uploaded once per session and shared by the editor
// and by baked custom levels (Level::sharedTextures keeps releaseLevel away from them).
struct MakerAtlas {
    int state, count;
    Texture pages[4];
};

MakerAtlas makerAtlases[6];

struct MakerAtlasFile {
    u8 *bytes;
    int count;
    u8 *pixels[4];
    int width[4], height[4];
};

bool makerOpenAtlas(int group, MakerAtlasFile &file) {
    memset(&file, 0, sizeof(file));
    if (group < 1 || group > 5)
        return false;
    MakerAtlas &atlas = makerAtlases[group];
    char path[96];
    wsprintfA(path, "data/Media/Texture Groups/world%d.ptex", group);
    u32 n;
    file.bytes = makerReadWhole(path, &n);
    if (!file.bytes || n < 24) {
        atlas.state = -1;
        mkFree(file.bytes);
        file.bytes = 0;
        return false;
    }
    Reader r(file.bytes, n, permanent);
    r.take(16);
    int count = r.i();
    if (count < 1 || count > 4) {
        atlas.state = -1;
        return false;
    }
    for (int i = 0; i < count; i++) {
        if (n - r.pos < 56)
            fatal("Damaged game resource.");
        u8 *start = file.bytes + r.pos;
        int h[10];
        memcpy(h, start + 16, 40);
        file.pixels[i] = start + 56;
        file.width[i] = h[3];
        file.height[i] = h[4];
        if (atlas.state != 1)
            atlas.pages[i] = textureRead(r);
        else {
            r.take(56);
            r.take(h[9]);
            for (int mip = 1; mip < h[2]; mip++)
                r.take(r.i());
        }
    }
    file.count = count;
    atlas.count = count;
    atlas.state = 1;
    return true;
}

void makerCloseAtlas(MakerAtlasFile &file) {
    mkFree(file.bytes);
    file.bytes = 0;
}

Texture *makerAtlasPage(int group, int page) {
    if (group < 1 || group > 5)
        return 0;
    MakerAtlas &atlas = makerAtlases[group];
    if (atlas.state == 0) {
        MakerAtlasFile file;
        makerOpenAtlas(group, file);
        makerCloseAtlas(file);
    }
    if (atlas.state != 1 || page < 0 || page >= atlas.count)
        return 0;
    return &atlas.pages[page];
}

enum MakerPieceCategory {
    PIECE_BLOCK,
    PIECE_PLATFORM,
    PIECE_SLOPE,
    PIECE_DECOR,
    PIECE_BIG,
    PIECE_FILL,
    PIECE_CATEGORIES,
    PIECE_HIDDEN = 255
};

const char *makerPieceCategoryNames[PIECE_CATEGORIES] = {"Blocks", "Platforms", "Slopes",
                                                         "Decor",  "Large",     "Fill"};

struct MakerPiece {
    float uv[8];
    short w, h;
    u8 group, texture, category, collide;
    short surface;
    u8 opaque, reserved;
    int uses, onewayHits, surfaceSum, sizeUses;
};

enum { MAKER_MAX_PIECES = 2048, MAKER_PIECE_HASH = 4096 };

MakerPiece *makerPieces;
int makerPieceCount;
int *makerPieceHash;
int *makerCategoryList[PIECE_CATEGORIES];
int makerCategoryCount[PIECE_CATEGORIES];
bool makerCatalogReady;

bool makerSameUv(const float *a, const float *b) {
    for (int i = 0; i < 8; i++)
        if (a[i] != b[i])
            return false;
    return true;
}

int makerFindPiece(int group, int texture, int w, int h, const float *uv, bool add) {
    struct {
        int group, texture, w, h;
        float uv[8];
    } key;

    key.group = group;
    key.texture = texture;
    key.w = w;
    key.h = h;
    memcpy(key.uv, uv, sizeof(key.uv));
    u32 hash = hashBytes(&key, sizeof(key));
    for (u32 probe = 0; probe < MAKER_PIECE_HASH; probe++) {
        u32 slot = (hash + probe) & (MAKER_PIECE_HASH - 1);
        int index = makerPieceHash[slot];
        if (index < 0) {
            if (!add || makerPieceCount >= MAKER_MAX_PIECES)
                return -1;
            MakerPiece &p = makerPieces[makerPieceCount];
            memset(&p, 0, sizeof(p));
            memcpy(p.uv, uv, sizeof(p.uv));
            p.w = (short)w;
            p.h = (short)h;
            p.group = (u8)group;
            p.texture = (u8)texture;
            makerPieceHash[slot] = makerPieceCount;
            return makerPieceCount++;
        }
        MakerPiece &p = makerPieces[index];
        if (p.group == group && p.texture == texture && p.w == w && p.h == h &&
            makerSameUv(p.uv, uv))
            return index;
    }
    return -1;
}

int makerRound(float v) {
    return (int)(v + (v < 0 ? -0.5f : 0.5f));
}

int makerFloor(float v) {
    int i = (int)v;
    return (float)i > v ? i - 1 : i;
}

int makerCeil(float v) {
    int i = (int)v;
    return (float)i < v ? i + 1 : i;
}

struct MakerQuadRef {
    int piece;
    float l, b, r, t;
};

void makerHarvestLevel(const char *file) {
    makerScratchReset();
    MakerScan s;
    if (!makerScanLevel(file, s, makerScratch, false) || !s.group)
        return;
    int quadCount = 0;
    for (int i = 0; i < s.tileCount; i++)
        quadCount += s.tiles[i].bytes / 64;
    MakerQuadRef *quads = makerScratch.array<MakerQuadRef>(quadCount);
    int used = 0;
    for (int i = 0; i < s.tileCount; i++) {
        Tile &t = s.tiles[i];
        if (t.layer < 0 || t.layer >= s.layerCount || s.layers[t.layer].flags[2] ||
            !s.layers[t.layer].flags[1])
            continue;
        for (int q = 0; q + 4 <= t.bytes / 16; q += 4) {
            Vertex *v = t.vertices + q;
            float l = v[0].x, b = v[0].y, r = v[3].x, top = v[3].y;
            int w = makerRound(r - l), h = makerRound(top - b);
            if (w < 4 || h < 4 || w > 512 || h > 512 || v[1].y != b || v[2].x != l)
                continue;
            float uv[8] = {v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v, v[3].u, v[3].v};
            int piece = makerFindPiece(s.group, t.texture, w, h, uv, true);
            if (piece < 0)
                continue;
            makerPieces[piece].uses++;
            MakerQuadRef &ref = quads[used++];
            ref.piece = piece;
            ref.l = l;
            ref.b = b;
            ref.r = r;
            ref.t = top;
        }
    }
    // Authored one-way lines are drawn on top of platform art; count every piece that carries one.
    for (int g = 0; g < s.geometryCount; g++) {
        Geometry &line = s.geometry[g];
        if (line.kind != 2)
            continue;
        for (int j = 1; j < line.count; j++) {
            Vec a = line.points[j - 1], b = line.points[j];
            float x0 = minf(a.x, b.x), x1 = maxf(a.x, b.x), y = maxf(a.y, b.y);
            for (int q = 0; q < used; q++) {
                MakerQuadRef &ref = quads[q];
                float overlap = minf(x1, ref.r) - maxf(x0, ref.l);
                if (overlap < (ref.r - ref.l) * 0.5f || ref.t < y - 2 || ref.t > y + 24)
                    continue;
                MakerPiece &p = makerPieces[ref.piece];
                p.onewayHits++;
                p.surfaceSum += makerRound(maxf(0, ref.t - y));
            }
        }
    }
}

int makerAlphaAt(const MakerAtlasFile &file, int page, const MakerPiece &p, float fx, float fy) {
    float u =
        (1 - fy) * (p.uv[0] + (p.uv[2] - p.uv[0]) * fx) + fy * (p.uv[4] + (p.uv[6] - p.uv[4]) * fx);
    float v =
        (1 - fy) * (p.uv[1] + (p.uv[3] - p.uv[1]) * fx) + fy * (p.uv[5] + (p.uv[7] - p.uv[5]) * fx);
    int w = file.width[page], h = file.height[page];
    int x = (int)(u * w), y = (int)(v * h);
    x = x < 0 ? 0 : x >= w ? w - 1 : x;
    y = y < 0 ? 0 : y >= h ? h - 1 : y;
    return file.pixels[page][(y * w + x) * 4 + 3];
}

// Opacity, slope orientation and top surface from a 12x12 alpha sample of the piece.
void makerClassifyPiece(MakerPiece &p, const MakerAtlasFile &file) {
    int page = p.texture;
    if (page >= file.count) {
        p.category = PIECE_HIDDEN;
        return;
    }
    float tw = (float)file.width[page], th = (float)file.height[page];
    float bx = (p.uv[2] - p.uv[0]) * tw, by = (p.uv[3] - p.uv[1]) * th;
    float lx = (p.uv[4] - p.uv[0]) * tw, ly = (p.uv[5] - p.uv[1]) * th;
    float bottom = root(bx * bx + by * by), left = root(lx * lx + ly * ly);
    // Authored fills stretch a texel, a strip or a small patch over large areas in many sizes;
    // each becomes one fill brick that stretches to any size.
    bool fill = bottom < 2 || left < 2 || bottom * 2.5f < p.w || left * 2.5f < p.h;
    const int N = 12;
    int opaque = 0, in[4] = {}, inSolid[4] = {}, out[4] = {}, outSolid[4] = {}, rowSolid[N] = {};
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            float fx = (i + 0.5f) / N, fy = (j + 0.5f) / N;
            int alpha = makerAlphaAt(file, page, p, fx, fy);
            if (alpha >= 200)
                opaque++;
            if (alpha >= 128)
                rowSolid[j]++;
            // 0: bottom-right, 1: bottom-left, 2: top-right, 3: top-left solid halves. Slopes
            // need opaque art; translucent shadow triangles stay decoration.
            float d[4] = {fx - fy, (1 - fx) - fy, fy - (1 - fx), fy - fx};
            for (int k = 0; k < 4; k++) {
                if (absf(d[k]) < 0.1f)
                    continue;
                if (d[k] > 0) {
                    in[k]++;
                    inSolid[k] += alpha >= 200;
                } else {
                    out[k]++;
                    outSolid[k] += alpha >= 64;
                }
            }
        }
    p.opaque = opaque * 100 >= N * N * 97;
    if (fill) {
        p.category = PIECE_FILL;
        p.collide = p.opaque ? COLLIDE_SOLID : COLLIDE_NONE;
        return;
    }
    int slope = -1;
    if (p.w <= 64 && p.h <= 64)
        for (int k = 0; k < 4; k++)
            if (in[k] && out[k] && inSolid[k] * 100 >= in[k] * 85 &&
                outSolid[k] * 100 <= out[k] * 15)
                slope = k;
    int surface = 0;
    for (int j = N - 1; j >= 0; j--)
        if (rowSolid[j] * 2 >= N) {
            surface = makerRound((1 - (j + 1) / (float)N) * p.h);
            break;
        }
    bool big = p.w > 128 || p.h > 128;
    bool platform = p.onewayHits > 0 && p.onewayHits * 4 >= p.uses;
    if (slope >= 0) {
        p.category = PIECE_SLOPE;
        p.collide = (u8)(COLLIDE_SLOPE_BR + slope);
    } else if (platform) {
        p.category = big ? PIECE_BIG : PIECE_PLATFORM;
        p.collide = COLLIDE_ONEWAY;
        int learned = makerRound((float)p.surfaceSum / p.onewayHits);
        p.surface = (short)(learned > 0 ? learned : surface < p.h / 2 ? surface : 0);
    } else if (!big && p.opaque) {
        p.category = PIECE_BLOCK;
        p.collide = COLLIDE_SOLID;
    } else {
        p.category = big ? PIECE_BIG : PIECE_DECOR;
        p.collide = COLLIDE_NONE;
    }
}

bool makerPieceBefore(const MakerPiece &a, const MakerPiece &b) {
    if (a.group != b.group)
        return a.group < b.group;
    if (a.uses != b.uses)
        return a.uses > b.uses;
    return a.w * a.h < b.w * b.h;
}

// Harvest every stage once per session; the authored data never changes while the game runs.
void makerBuildCatalog() {
    if (makerCatalogReady)
        return;
    makerPieces = (MakerPiece *)mkAlloc(sizeof(MakerPiece) * MAKER_MAX_PIECES);
    makerPieceHash = (int *)mkAlloc(sizeof(int) * MAKER_PIECE_HASH);
    for (int i = 0; i < MAKER_PIECE_HASH; i++)
        makerPieceHash[i] = -1;
    makerPieceCount = 0;
    for (int i = 0; i < levelCount; i++)
        if (levels[i].props.num("bean-world") > 0)
            makerHarvestLevel(levels[i].file);
    for (int group = 1; group <= 5; group++) {
        MakerAtlasFile file;
        bool open = makerOpenAtlas(group, file);
        for (int i = 0; i < makerPieceCount; i++)
            if (makerPieces[i].group == group) {
                if (open)
                    makerClassifyPiece(makerPieces[i], file);
                else
                    makerPieces[i].category = PIECE_HIDDEN;
            }
        makerCloseAtlas(file);
    }
    // A fill strip is harvested once per authored size. Keep one brick per strip, sized like its
    // most common use and at most four cells per side.
    for (int i = 0; i < makerPieceCount; i++) {
        MakerPiece &p = makerPieces[i];
        if (p.category != PIECE_FILL)
            continue;
        p.sizeUses = p.uses;
        for (int j = 0; j < i; j++) {
            MakerPiece &q = makerPieces[j];
            if (q.category != PIECE_FILL || q.group != p.group || q.texture != p.texture ||
                !makerSameUv(q.uv, p.uv))
                continue;
            if (p.uses > q.sizeUses) {
                q.w = p.w;
                q.h = p.h;
                q.sizeUses = p.uses;
            }
            q.uses += p.uses;
            p.category = PIECE_HIDDEN;
            break;
        }
    }
    for (int i = 0; i < makerPieceCount; i++)
        if (makerPieces[i].category == PIECE_FILL) {
            makerPieces[i].w = (short)minf(128, makerPieces[i].w);
            makerPieces[i].h = (short)minf(128, makerPieces[i].h);
        }
    for (int c = 0; c < PIECE_CATEGORIES; c++) {
        makerCategoryList[c] = (int *)mkAlloc(sizeof(int) * (makerPieceCount + 1));
        makerCategoryCount[c] = 0;
        for (int i = 0; i < makerPieceCount; i++)
            if (makerPieces[i].category == c) {
                int n = makerCategoryCount[c]++, *list = makerCategoryList[c];
                while (n > 0 && makerPieceBefore(makerPieces[i], makerPieces[list[n - 1]])) {
                    list[n] = list[n - 1];
                    n--;
                }
                list[n] = i;
            }
    }
    makerScratchReset();
    makerCatalogReady = true;
}

// The palette entry of a placed fill, whatever size it was stretched to.
int makerFindFill(int group, int texture, const float *uv) {
    for (int i = 0; i < makerCategoryCount[PIECE_FILL]; i++) {
        int index = makerCategoryList[PIECE_FILL][i];
        const MakerPiece &p = makerPieces[index];
        if (p.group == group && p.texture == texture && makerSameUv(p.uv, uv))
            return index;
    }
    return -1;
}

// Theme order follows the internal world numbers used by the texture groups.
const char *makerThemeNames[6] = {"",          "Grasslands",   "Crystal Peaks",
                                  "Sky Ruins", "Dusty Desert", "Wizard's Lair"};
const u64 makerThemeMusic[6] = {0,
                                1677388375869728ULL,
                                15828032761193764437ULL,
                                382014585102ULL,
                                1714869376956766ULL,
                                9149035873077978ULL};

struct MakerBackground {
    const char *name;
    int theme;
    u64 level;
};

// Stage backgrounds verified to tile cleanly for any course size and window aspect.
const MakerBackground makerBackgrounds[] = {{"Grassland Hills", 1, 6838399004045653085ULL},
                                            {"Grassland Sunset", 1, 13151310487044325505ULL},
                                            {"Grassland Sky", 1, 5100914154076244783ULL},
                                            {"Grassland Fortress", 1, 1066389851804853660ULL},
                                            {"Crystal Peaks", 2, 15974012089803411243ULL},
                                            {"Crystal Dusk", 2, 11491836511846746094ULL},
                                            {"Crystal Caves", 2, 5687107557475985288ULL},
                                            {"Starry Night", 2, 13052395652604067555ULL},
                                            {"Ice Bricks", 2, 3682577195148464396ULL},
                                            {"Crystal Spires", 2, 3442460187460865148ULL},
                                            {"Sky Ruins", 3, 161899780601ULL},
                                            {"Ruin Pillars", 3, 191561700160ULL},
                                            {"Ruin Peaks", 3, 899263731133ULL},
                                            {"Ruin Blaze", 3, 685488901231ULL},
                                            {"Ruin Bricks", 3, 905968796386ULL},
                                            {"Ruin Furnace", 3, 901687619158ULL},
                                            {"Stormy Night", 3, 641851699177ULL},
                                            {"Desert Oasis", 4, 297352608196ULL},
                                            {"Desert Dunes", 4, 176011402805ULL},
                                            {"Desert Palms", 4, 900179179108ULL},
                                            {"Moonlight Oasis", 4, 5770908237787719ULL},
                                            {"Desert Caverns", 4, 418098865551ULL},
                                            {"Sandstorm Sky", 4, 787600693931ULL},
                                            {"Wizard's Keep", 5, 116499217023ULL},
                                            {"Castle Sky", 5, 806701038719ULL},
                                            {"Castle Dusk", 5, 881271676947ULL},
                                            {"Castle Bricks", 5, 739118707375ULL},
                                            {"Pendulum Hall", 5, 2986143812017068ULL},
                                            {"Deep Blue", 5, 8458860001749117ULL}};
const int makerBackgroundCount = sizeof(makerBackgrounds) / sizeof(makerBackgrounds[0]);

int makerThemeBackground(int theme) {
    for (int i = 0; i < makerBackgroundCount; i++)
        if (makerBackgrounds[i].theme == theme)
            return i;
    return 0;
}

struct MakerEffect {
    const char *name;
    u64 klass;
};

const MakerEffect makerEffects[] = {{"None", 0},
                                    {"Rain", 310706908189ULL},
                                    {"Cherry Blossoms", 7847174900699792ULL},
                                    {"Sandstorm", 2778726716156712ULL},
                                    {"Starry Sky", 10170490229994225569ULL},
                                    {"Distant Birds", 697074819893ULL}};
const int makerEffectCount = sizeof(makerEffects) / sizeof(makerEffects[0]);

u64 makerCourseMusic(const MakerHeader &h) {
    if (h.music == MAKER_MUSIC_SILENT)
        return 0;
    if (h.music == MAKER_MUSIC_THEME)
        return makerThemeMusic[(int)clamp((float)h.theme, 1, 5)];
    return h.music;
}

// Music names without the authoring suffix; the goal jingle is not offered as a course track.
void makerMusicName(u64 id, char *out, int size) {
    if (id == MAKER_MUSIC_THEME) {
        lstrcpynA(out, "Theme music", size);
        return;
    }
    if (id == MAKER_MUSIC_SILENT) {
        lstrcpynA(out, "Silence", size);
        return;
    }
    lstrcpynA(out, "Unknown track", size);
    for (int i = 0; i < musicCount; i++)
        if (musicInfo[i].id == id) {
            lstrcpynA(out, musicInfo[i].props.get("name", "Track"), size);
            int n = lstrlenA(out);
            if (n > 6 && eq(out + n - 6, " Final"))
                out[n - 6] = 0;
        }
}

u64 makerNextMusic(u64 id, int direction) {
    u64 list[32];
    int count = 0;
    list[count++] = MAKER_MUSIC_THEME;
    for (int i = 0; i < musicCount && count < 31; i++)
        if (musicInfo[i].id != 16755430903372978986ULL)
            list[count++] = musicInfo[i].id;
    list[count++] = MAKER_MUSIC_SILENT;
    int at = 0;
    for (int i = 0; i < count; i++)
        if (list[i] == id)
            at = i;
    return list[(at + direction + count) % count];
}

enum MakerObjectCategory { OBJ_ITEM, OBJ_ENEMY, OBJ_GIZMO, OBJ_AREA, OBJ_CATEGORIES };

const char *makerObjectCategoryNames[OBJ_CATEGORIES] = {"Items", "Enemies", "Gizmos", "Areas"};

enum MakerObjectFlags {
    OBJ_RESIZE_X = 1,
    OBJ_RESIZE_Y = 2,
    OBJ_ROTATE = 4,
    OBJ_UNIQUE = 8,
    OBJ_WATER = 16,
    OBJ_WIND = 32,
    OBJ_SECRET = 64,
    OBJ_FREE = 128,
    OBJ_INVISIBLE = 256,
    OBJ_TURRET = 512,
    OBJ_START = 1024,
    OBJ_PARTICLES = 2048
};

enum MakerParamKind { PARAM_NUMBER, PARAM_TOGGLE, PARAM_CHOICE };

enum MakerParamWrite {
    WRITE_SET,          // behaviour.key = value
    WRITE_BOTH,         // behaviour.key = behaviour.key2 = value
    WRITE_RANGE,        // oscillator travel; with AXIS, SIGN and START into move-x/y, min/max-move
    WRITE_AXIS,         // oscillator axis
    WRITE_SIGN,         // oscillator first direction
    WRITE_START,        // where the oscillator starts, percent of its travel from the left/bottom
    WRITE_PERIOD,       // magnitude of a signed period; combined with WRITE_PERIOD_SIGN
    WRITE_PERIOD_SIGN,  // reversal of a signed period
    WRITE_MASK,         // choice index -> alternation bit
    WRITE_TURRET,       // direction -> class, angle and static target vector
    WRITE_FORCE_DIR,    // wind direction; combined with WRITE_FORCE and WRITE_CAP
    WRITE_FORCE,        // wind strength
    WRITE_CAP,          // wind speed cap, 0 disables it
    WRITE_FADE,         // wind strength left at the far end, percent of the source end
    WRITE_SWING_ANGLE,  // pendulum start angle; with WRITE_SWING_DIR into TimeOffset
    WRITE_SWING_DIR,    // pendulum first swing direction
    WRITE_BULLET_SPEED, // per-source projectile speed, defaulting to the bullet class speed
    WRITE_DIAGONAL,     // diagonal swimmer heading -> move-x/move-y signs
    WRITE_ACTIVITY,     // always / wait for Bean / only near Bean -> paused and auto-deactivate
    WRITE_DEFEAT,       // nothing or a colour flipped on defeat -> alternate-on-death and mask
    WRITE_MASK0,        // none or a colour -> alternation mask 0 or one bit
    WRITE_SPREAD,       // single shot or a fan -> extraShots angle list
    WRITE_HITS,         // hits to break; with WRITE_BREAK_SIDES into hp and damage per hit
    WRITE_BREAK_SIDES,  // breakable from below only or from any side
    WRITE_PARTICLES,    // particle amount in percent of the area-scaled density
    WRITE_GEM_STYLE,    // Maker-only gem appearance: random or one of the five authored clips
    WRITE_POTION_FORM   // Maker potion transformation target, retaining Emilio as the default
};

enum MakerParamFlags { PARAM_USE_DEFAULT = 1 };

struct MakerParamDef {
    const char *label;
    u8 kind, write, flags;
    const char *behavior, *key, *key2;
    float min, max, step, def;
    const char *choices; // labels separated by '|'
    const char *unit;    // shown after numbers
};

struct MakerObjectDef {
    const char *name;
    u64 klass;
    u8 category;
    unsigned short flags;
    short cellW, cellH, defW, defH;
    const MakerParamDef *params;
    int paramCount;
    const char *hint;
};

#define MAKER_PARAMS(list) list, (int)(sizeof(list) / sizeof(list[0]))

// Shared rows, written out per list: activity needs ProximityActivateBehaviour, defeat and
// stomp rows KillableEnemyBehaviour, the hurt row DeadlyBehaviour. Rows whose behaviour a class
// lacks stay hidden for that object.
const MakerParamDef makerWalkerParams[] = {
    {"Direction", PARAM_CHOICE, WRITE_SET, 0, "WalkBehaviour", "walk-left", 0, 0, 1, 1, 0,
     "Right|Left", 0},
    {"Walk speed", PARAM_NUMBER, WRITE_SET, 0, "WalkBehaviour", "walk-speed", 0, 10, 300, 5, 75, 0,
     "px/s"},
    {"Turn at edges", PARAM_TOGGLE, WRITE_SET, 0, "WalkBehaviour", "reverse-at-edge", 0, 0, 1, 1, 1,
     "No|Yes", 0},
    {"Wake range", PARAM_NUMBER, WRITE_BOTH, 0, "ProximityActivateBehaviour", "distance-x",
     "distance-y", 32, 1024, 32, 100, 0, "px"},
    {"Active", PARAM_CHOICE, WRITE_ACTIVITY, 0, "ProximityActivateBehaviour", "auto-deactivate", 0,
     0, 2, 1, 0, "Always|Wait for Bean|Only near", 0},
    {"On defeat", PARAM_CHOICE, WRITE_DEFEAT, 0, "KillableEnemyBehaviour", "alternate-on-death",
     "alternation-mask", 0, 4, 1, 0, "Nothing|Flip A|Flip B|Flip C|Flip D", 0},
    {"Stomp bounce", PARAM_NUMBER, WRITE_SET, 0, "KillableEnemyBehaviour", "jump-boost", 0, 0, 96,
     2, 0, 0, "px"}};

// The orbit beetle walks like a beetle and carries a spike ball on its own orbit.
const MakerParamDef makerOrbitBeetleParams[] = {
    {"Direction", PARAM_CHOICE, WRITE_SET, 0, "WalkBehaviour", "walk-left", 0, 0, 1, 1, 0,
     "Right|Left", 0},
    {"Walk speed", PARAM_NUMBER, WRITE_SET, 0, "WalkBehaviour", "walk-speed", 0, 10, 300, 5, 75, 0,
     "px/s"},
    {"Turn at edges", PARAM_TOGGLE, WRITE_SET, 0, "WalkBehaviour", "reverse-at-edge", 0, 0, 1, 1, 1,
     "No|Yes", 0},
    {"Wake range", PARAM_NUMBER, WRITE_BOTH, 0, "ProximityActivateBehaviour", "distance-x",
     "distance-y", 32, 1024, 32, 100, 0, "px"},
    {"Active", PARAM_CHOICE, WRITE_ACTIVITY, 0, "ProximityActivateBehaviour", "auto-deactivate", 0,
     0, 2, 1, 0, "Always|Wait for Bean|Only near", 0},
    {"On defeat", PARAM_CHOICE, WRITE_DEFEAT, 0, "KillableEnemyBehaviour", "alternate-on-death",
     "alternation-mask", 0, 4, 1, 0, "Nothing|Flip A|Flip B|Flip C|Flip D", 0},
    {"Stomp bounce", PARAM_NUMBER, WRITE_SET, 0, "KillableEnemyBehaviour", "jump-boost", 0, 0, 96,
     2, 0, 0, "px"},
    {"Ball radius", PARAM_NUMBER, WRITE_SET, 0, "OrbitingObjectBehaviour", "radius", 0, 16, 160, 4,
     40, 0, "px"},
    {"Ball lap time", PARAM_NUMBER, WRITE_PERIOD, 0, "OrbitingObjectBehaviour", "period", 0, 0.5f,
     12, 0.1f, 2, 0, "s"},
    {"Ball reverse", PARAM_TOGGLE, WRITE_PERIOD_SIGN, 0, "OrbitingObjectBehaviour", "period", 0, 0,
     1, 1, 0, "No|Yes", 0},
    {"Ball start", PARAM_NUMBER, WRITE_SET, 0, "OrbitingObjectBehaviour", "start-angle", 0, 0, 345,
     15, 0, 0, "deg"}};

// The object appears where it is placed; the path spreads around that start point.
const MakerParamDef makerOscillatorParams[] = {
    {"Axis", PARAM_CHOICE, WRITE_AXIS, 0, "OscillatingBehaviour", "move-x", "move-y", 0, 1, 1, 0,
     "Horizontal|Vertical", 0},
    {"Travel", PARAM_NUMBER, WRITE_RANGE, 0, "OscillatingBehaviour", "min-move", "max-move", 0,
     1920, 16, 128, 0, "px"},
    {"Start point", PARAM_NUMBER, WRITE_START, 0, "OscillatingBehaviour", 0, 0, 0, 100, 10, 50, 0,
     "%"},
    {"First move", PARAM_CHOICE, WRITE_SIGN, 0, "OscillatingBehaviour", "move-x", "move-y", 0, 1, 1,
     0, "Right/up|Left/down", 0},
    {"Speed", PARAM_NUMBER, WRITE_SET, 0, "OscillatingBehaviour", "speed", 0, 10, 400, 5, 50, 0,
     "px/s"},
    {"Wake range", PARAM_NUMBER, WRITE_BOTH, 0, "ProximityActivateBehaviour", "distance-x",
     "distance-y", 32, 1024, 32, 100, 0, "px"},
    {"Active", PARAM_CHOICE, WRITE_ACTIVITY, 0, "ProximityActivateBehaviour", "auto-deactivate", 0,
     0, 2, 1, 0, "Always|Wait for Bean|Only near", 0},
    {"On defeat", PARAM_CHOICE, WRITE_DEFEAT, 0, "KillableEnemyBehaviour", "alternate-on-death",
     "alternation-mask", 0, 4, 1, 0, "Nothing|Flip A|Flip B|Flip C|Flip D", 0},
    {"Stomp bounce", PARAM_NUMBER, WRITE_SET, 0, "KillableEnemyBehaviour", "jump-boost", 0, 0, 96,
     2, 0, 0, "px"},
    {"Hurts enemies", PARAM_TOGGLE, WRITE_SET, 0, "DeadlyBehaviour", "kill-enemies", 0, 0, 1, 1, 0,
     "No|Yes", 0}};

const MakerParamDef makerOrbitParams[] = {
    {"Radius", PARAM_NUMBER, WRITE_SET, 0, "OrbitBehaviour", "radius", 0, 16, 240, 4, 40, 0, "px"},
    {"Lap time", PARAM_NUMBER, WRITE_PERIOD, 0, "OrbitBehaviour", "period", 0, 0.5f, 12, 0.1f, 4, 0,
     "s"},
    {"Reverse", PARAM_TOGGLE, WRITE_PERIOD_SIGN, 0, "OrbitBehaviour", "period", 0, 0, 1, 1, 0,
     "No|Yes", 0},
    {"Start angle", PARAM_NUMBER, WRITE_SET, 0, "OrbitBehaviour", "start-angle", 0, 0, 345, 15, 0,
     0, "deg"},
    {"On defeat", PARAM_CHOICE, WRITE_DEFEAT, 0, "KillableEnemyBehaviour", "alternate-on-death",
     "alternation-mask", 0, 4, 1, 0, "Nothing|Flip A|Flip B|Flip C|Flip D", 0},
    {"Stomp bounce", PARAM_NUMBER, WRITE_SET, 0, "KillableEnemyBehaviour", "jump-boost", 0, 0, 96,
     2, 0, 0, "px"}};

const MakerParamDef makerJumperParams[] = {
    {"Jump delay", PARAM_NUMBER, WRITE_SET, 0, "EnemyJumperBehaviour", "jump-delay", 0, 0.1f, 4,
     0.05f, 0.5f, 0, "s"},
    {"Jump height", PARAM_NUMBER, WRITE_SET, 0, "EnemyJumperBehaviour", "jump-height", 0, 20, 200,
     5, 60, 0, "px"},
    {"Hop speed", PARAM_NUMBER, WRITE_SET, 0, "EnemyJumperBehaviour", "jump-x-velocity", 0, 0, 300,
     5, 100, 0, "px/s"},
    {"Wake range", PARAM_NUMBER, WRITE_BOTH, 0, "ProximityActivateBehaviour", "distance-x",
     "distance-y", 32, 1024, 32, 100, 0, "px"},
    {"Active", PARAM_CHOICE, WRITE_ACTIVITY, 0, "ProximityActivateBehaviour", "auto-deactivate", 0,
     0, 2, 1, 0, "Always|Wait for Bean|Only near", 0},
    {"On defeat", PARAM_CHOICE, WRITE_DEFEAT, 0, "KillableEnemyBehaviour", "alternate-on-death",
     "alternation-mask", 0, 4, 1, 0, "Nothing|Flip A|Flip B|Flip C|Flip D", 0},
    {"Stomp bounce", PARAM_NUMBER, WRITE_SET, 0, "KillableEnemyBehaviour", "jump-boost", 0, 0, 96,
     2, 0, 0, "px"}};

const MakerParamDef makerFishParams[] = {
    {"Swim speed", PARAM_NUMBER, WRITE_SET, 0, "DiagonalBehaviour", "speed", 0, 10, 300, 5, 100, 0,
     "px/s"},
    {"Heading", PARAM_CHOICE, WRITE_DIAGONAL, 0, "DiagonalBehaviour", "move-x", "move-y", 0, 3, 1,
     0, "Down-left|Down-right|Up-left|Up-right", 0},
    {"Wake range", PARAM_NUMBER, WRITE_BOTH, 0, "ProximityActivateBehaviour", "distance-x",
     "distance-y", 32, 1024, 32, 288, 0, "px"},
    {"Active", PARAM_CHOICE, WRITE_ACTIVITY, 0, "ProximityActivateBehaviour", "auto-deactivate", 0,
     0, 2, 1, 0, "Always|Wait for Bean|Only near", 0},
    {"On defeat", PARAM_CHOICE, WRITE_DEFEAT, 0, "KillableEnemyBehaviour", "alternate-on-death",
     "alternation-mask", 0, 4, 1, 0, "Nothing|Flip A|Flip B|Flip C|Flip D", 0},
    {"Stomp bounce", PARAM_NUMBER, WRITE_SET, 0, "KillableEnemyBehaviour", "jump-boost", 0, 0, 96,
     2, 0, 0, "px"}};

const MakerParamDef makerSpringParams[] = {{"Power", PARAM_NUMBER, WRITE_SET, 0, "SpringBehaviour",
                                            "jump-power", 0, 100, 600, 10, 192, 0, 0},
                                           {"Object power", PARAM_NUMBER, WRITE_SET, 0,
                                            "SpringBehaviour", "physics-power", 0, 0, 800, 10, 200,
                                            0, 0}};

const MakerParamDef makerTrampolineParams[] = {{"Extra bounce", PARAM_NUMBER, WRITE_SET, 0,
                                                "TrampolineBehaviour", "jump-mod", 0, 0, 400, 10, 0,
                                                0, "px/s"}};

const MakerParamDef makerBouncerParams[] = {
    {"Power", PARAM_NUMBER, WRITE_SET, 0, "PinballBouncerBehaviour", "jump-power", 0, 60, 300, 10,
     130, 0, 0},
    {"Object power", PARAM_NUMBER, WRITE_SET, 0, "PinballBouncerBehaviour", "physics-power", 0, 0,
     400, 5, 75, 0, 0}};

// Stiffness is the hinge torque that resists tilting; spin power keeps a spinner turning.
const MakerParamDef makerSeesawParams[] = {
    {"Self-levelling", PARAM_TOGGLE, WRITE_SET, 0, "SeesawBehaviour", "auto-correct", 0, 0, 1, 1, 1,
     "No|Yes", 0},
    {"Stiffness", PARAM_NUMBER, WRITE_SET, 0, "SeesawBehaviour", "rotational-friction", 0, 0, 20,
     0.5f, 1, 0, 0},
    {"Level speed", PARAM_NUMBER, WRITE_SET, 0, "SeesawBehaviour", "max-adjust-speed", 0, 0.5f, 20,
     0.5f, 5, 0, 0}};

const MakerParamDef makerSpinnerParams[] = {
    {"Spin speed", PARAM_NUMBER, WRITE_SET, 0, "RotatingBehaviour", "rotation-speed", 0, -240, 240,
     10, 90, 0, "deg/s"},
    {"Spin power", PARAM_NUMBER, WRITE_SET, 0, "RotatingBehaviour", "rotation-power", 0, 10, 1000,
     10, 150, 0, 0}};

// Each hit removes one point, so the cracks advance hit by hit.
const MakerParamDef makerBreakableParams[] = {
    {"Hits to break", PARAM_NUMBER, WRITE_HITS, 0, "BreakableBehaviour", "hp", "hp-on-bottom-hit",
     1, 9, 1, 1, 0, 0},
    {"Breaks from", PARAM_CHOICE, WRITE_BREAK_SIDES, 0, "BreakableBehaviour", "hp-on-other-hit", 0,
     0, 1, 1, 0, "Below|Any side", 0}};

const MakerParamDef makerCrumbleParams[] = {{"Respawn time", PARAM_NUMBER, WRITE_SET, 0,
                                             "CrumblingPlatformBehaviour", "Time", 0, 0.5f, 30,
                                             0.5f, 3, 0, "s"}};

// Swing time is one sweep from side to side; the start angle and first direction place the
// pendulum in its swing when the course begins.
const MakerParamDef makerPendulumParams[] = {
    {"Chain links", PARAM_NUMBER, WRITE_SET, 0, "WreckingBallControllerBehaviour", "NumLinks", 0, 1,
     16, 1, 6, 0, 0},
    {"Swing time", PARAM_NUMBER, WRITE_SET, 0, "WreckingBallControllerBehaviour", "Period", 0, 0.3f,
     8, 0.1f, 2, 0, "s"},
    {"Swing angle", PARAM_NUMBER, WRITE_SET, 0, "WreckingBallControllerBehaviour", "MaxAngle", 0, 5,
     89, 1, 60, 0, "deg"},
    {"Start angle", PARAM_NUMBER, WRITE_SWING_ANGLE, 0, "WreckingBallControllerBehaviour",
     "TimeOffset", 0, -89, 89, 5, 60, 0, "deg"},
    {"Swing first", PARAM_CHOICE, WRITE_SWING_DIR, 0, "WreckingBallControllerBehaviour",
     "TimeOffset", 0, 0, 1, 1, 0, "Left|Right", 0}};

const MakerParamDef makerCannonParams[] = {
    {"Direction", PARAM_CHOICE, WRITE_TURRET, PARAM_USE_DEFAULT, "BulletSource", "targetX",
     "targetY", 0, 3, 1, 0, "Right|Up|Left|Down", 0},
    {"Fire interval", PARAM_NUMBER, WRITE_BOTH, 0, "BulletSource", "timeMin", "timeMax", 0.2f, 8,
     0.1f, 1.5f, 0, "s"},
    {"Bullet speed", PARAM_NUMBER, WRITE_BULLET_SPEED, 0, "BulletSource", "bulletSpeed", 0, 25, 600,
     25, 125, 0, "px/s"},
    {"Spread", PARAM_CHOICE, WRITE_SPREAD, 0, "BulletSource", "extraShots", 0, 0, 3, 1, 0,
     "Single|3-way 15|3-way 30|5-way", 0},
    {"Phase offset", PARAM_NUMBER, WRITE_SET, 0, "BulletSource", "timeOffset", 0, 0, 8, 0.1f, 0, 0,
     "s"},
    {"Max bullets", PARAM_NUMBER, WRITE_SET, 0, "BulletSource", "bulletPoolSize", 0, 1, 10, 1, 10,
     0, 0},
    {"Wake range", PARAM_NUMBER, WRITE_BOTH, 0, "ProximityActivateBehaviour", "distance-x",
     "distance-y", 64, 1024, 32, 512, 0, "px"},
    {"Active", PARAM_CHOICE, WRITE_ACTIVITY, 0, "ProximityActivateBehaviour", "auto-deactivate", 0,
     0, 2, 1, 0, "Always|Wait for Bean|Only near", 0}};

const MakerParamDef makerSeekerParams[] = {
    {"Fire interval", PARAM_NUMBER, WRITE_BOTH, 0, "BulletSource", "timeMin", "timeMax", 0.2f, 8,
     0.1f, 1.5f, 0, "s"},
    {"Bullet speed", PARAM_NUMBER, WRITE_BULLET_SPEED, 0, "BulletSource", "bulletSpeed", 0, 25, 600,
     25, 200, 0, "px/s"},
    {"Spread", PARAM_CHOICE, WRITE_SPREAD, 0, "BulletSource", "extraShots", 0, 0, 3, 1, 0,
     "Single|3-way 15|3-way 30|5-way", 0},
    {"Aim speed", PARAM_NUMBER, WRITE_SET, 0, "BulletSource", "turnSpeed", 0, 10, 720, 10, 90, 0,
     "deg/s"},
    {"Phase offset", PARAM_NUMBER, WRITE_SET, 0, "BulletSource", "timeOffset", 0, 0, 8, 0.1f, 0, 0,
     "s"},
    {"Max bullets", PARAM_NUMBER, WRITE_SET, 0, "BulletSource", "bulletPoolSize", 0, 1, 10, 1, 3, 0,
     0},
    {"Wake range", PARAM_NUMBER, WRITE_BOTH, 0, "ProximityActivateBehaviour", "distance-x",
     "distance-y", 64, 1024, 32, 384, 0, "px"},
    {"Active", PARAM_CHOICE, WRITE_ACTIVITY, 0, "ProximityActivateBehaviour", "auto-deactivate", 0,
     0, 2, 1, 0, "Always|Wait for Bean|Only near", 0}};

const MakerParamDef makerWindParams[] = {{"Direction", PARAM_CHOICE, WRITE_FORCE_DIR,
                                          PARAM_USE_DEFAULT, "ForceAreaBehaviour", 0, 0, 0, 3, 1, 0,
                                          "Up|Down|Left|Right", 0},
                                         {"Strength", PARAM_NUMBER, WRITE_FORCE, PARAM_USE_DEFAULT,
                                          "ForceAreaBehaviour", 0, 0, 50, 2000, 50, 700, 0, 0},
                                         {"Speed cap", PARAM_NUMBER, WRITE_CAP, PARAM_USE_DEFAULT,
                                          "ForceAreaBehaviour", 0, 0, 0, 800, 16, 256, 0, "px/s"},
                                         {"Far end", PARAM_NUMBER, WRITE_FADE, PARAM_USE_DEFAULT,
                                          "ForceAreaBehaviour", 0, 0, 0, 100, 10, 100, 0, "%"}};

// Buoyancy is the water density times the submerged area; drag scales with speed and area.
const MakerParamDef makerWaterParams[] = {
    {"Bean float", PARAM_NUMBER, WRITE_SET, 0, "WaterAreaBehaviour", "bean-density", 0, 0, 40, 1,
     15, 0, 0},
    {"Bean drag", PARAM_NUMBER, WRITE_SET, 0, "WaterAreaBehaviour", "bean-linear-drag", 0, 0, 60, 1,
     20, 0, 0},
    {"Object float", PARAM_NUMBER, WRITE_SET, 0, "WaterAreaBehaviour", "density", 0, 0, 8, 0.25f,
     1.75f, 0, 0},
    {"Object drag", PARAM_NUMBER, WRITE_SET, 0, "WaterAreaBehaviour", "linear-drag", 0, 0, 30, 1, 5,
     0, 0}};

const MakerParamDef makerSwitchParams[] = {{"Switch group", PARAM_CHOICE, WRITE_MASK, 0,
                                            "AlternatorSwitchBehaviour", "AlternationMask", 0, 0, 3,
                                            1, 0, "Group A|Group B|Group C|Group D", 0}};

const MakerParamDef makerSwitchBlockParams[] = {
    {"Switch group", PARAM_CHOICE, WRITE_MASK, 0, "AlternatorBlockBehaviour", "AlternationMask", 0,
     0, 3, 1, 0, "Group A|Group B|Group C|Group D", 0},
    {"Solid at start", PARAM_TOGGLE, WRITE_SET, 0, "AlternatorBlockBehaviour", "Alternation", 0, 0,
     1, 1, 1, "No|Yes", 0}};

const MakerParamDef makerTouchBlockParams[] = {
    {"Switch group", PARAM_CHOICE, WRITE_MASK, 0, "AlternatorTouchBlockBehaviour",
     "AlternationMask", 0, 0, 3, 1, 1, "Group A|Group B|Group C|Group D", 0},
    {"Solid at start", PARAM_TOGGLE, WRITE_SET, 0, "AlternatorTouchBlockBehaviour", "Alternation",
     0, 0, 1, 1, 1, "No|Yes", 0}};

// Spikes can retract and extend with a switch colour, like switch blocks.
const MakerParamDef makerSpikeParams[] = {
    {"Switch group", PARAM_CHOICE, WRITE_MASK0, 0, "SpikeGroupBehaviour", "alternation-mask", 0, 0,
     4, 1, 0, "None|Group A|Group B|Group C|Group D", 0},
    {"Out at start", PARAM_TOGGLE, WRITE_SET, 0, "SpikeGroupBehaviour", "alternation", 0, 0, 1, 1,
     1, "No|Yes", 0}};

const MakerParamDef makerCheckpointParams[] = {
    {"Rearm width", PARAM_NUMBER, WRITE_SET, 0, "CheckpointBehaviour", "reactivate-dist-x", 0, 64,
     960, 16, 240, 0, "px"},
    {"Rearm height", PARAM_NUMBER, WRITE_SET, 0, "CheckpointBehaviour", "reactivate-dist-y", 0, 32,
     960, 16, 160, 0, "px"}};

const MakerParamDef makerHelperParams[] = {
    {"Lift speed", PARAM_NUMBER, WRITE_SET, 0, "HelperBirdBehaviour", "max-fly-speed", 0, 20, 400,
     10, 60, 0, "px/s"},
    {"Lift accel", PARAM_NUMBER, WRITE_SET, 0, "HelperBirdBehaviour", "fly-acceleration", 0, 100,
     5000, 100, 1000, 0, 0}};

const MakerParamDef makerBoxParams[] = {
    {"Weight", PARAM_NUMBER, WRITE_SET, 0, "BoxPhysicsBehaviour", "density", 0, 0.1f, 10, 0.1f,
     2.5f, 0, 0},
    {"Bounciness", PARAM_NUMBER, WRITE_SET, 0, "BoxPhysicsBehaviour", "restitution", 0, 0, 1, 0.05f,
     0.5f, 0, 0},
    {"Friction", PARAM_NUMBER, WRITE_SET, 0, "BoxPhysicsBehaviour", "friction", 0, 0, 1, 0.05f,
     0.2f, 0, 0},
    {"Falls", PARAM_TOGGLE, WRITE_SET, 0, "BoxPhysicsBehaviour", "is-gravitated", 0, 0, 1, 1, 1,
     "No|Yes", 0},
    {"Keep upright", PARAM_TOGGLE, WRITE_SET, 0, "BoxPhysicsBehaviour", "fixed-rotation", 0, 0, 1,
     1, 0, "No|Yes", 0},
    {"Hurts enemies", PARAM_TOGGLE, WRITE_SET, 0, "DeadlyBehaviour", "kill-enemies", 0, 0, 1, 1, 0,
     "No|Yes", 0}};

const MakerParamDef makerBallParams[] = {
    {"Weight", PARAM_NUMBER, WRITE_SET, 0, "CirclePhysicsBehaviour", "density", 0, 0.1f, 10, 0.1f,
     2, 0, 0},
    {"Bounciness", PARAM_NUMBER, WRITE_SET, 0, "CirclePhysicsBehaviour", "restitution", 0, 0, 1,
     0.05f, 0.5f, 0, 0},
    {"Friction", PARAM_NUMBER, WRITE_SET, 0, "CirclePhysicsBehaviour", "friction", 0, 0, 1, 0.05f,
     0.2f, 0, 0},
    {"Falls", PARAM_TOGGLE, WRITE_SET, 0, "CirclePhysicsBehaviour", "is-gravitated", 0, 0, 1, 1, 1,
     "No|Yes", 0},
    {"Keep upright", PARAM_TOGGLE, WRITE_SET, 0, "CirclePhysicsBehaviour", "fixed-rotation", 0, 0,
     1, 1, 0, "No|Yes", 0}};

const MakerParamDef makerParticleParams[] = {{"Amount", PARAM_NUMBER, WRITE_PARTICLES, 0,
                                              "ParticleDefSettings", 0, 0, 25, 400, 25, 100, 0,
                                              "%"}};

// Append a parameter rather than changing the course format: old gems keep random appearances.
const MakerParamDef makerGemParams[] = {
    {"Shape", PARAM_CHOICE, WRITE_GEM_STYLE, PARAM_USE_DEFAULT, "CoinGroupBehaviour", 0, 0,
     0, 5, 1, 0, "Random|Diamond|Octagon|Rhombus|Coin|Triangle", 0}};

const MakerParamDef makerPotionParams[] = {
    {"Transform into", PARAM_CHOICE, WRITE_POTION_FORM, PARAM_USE_DEFAULT,
     "UnbeanPotionBehaviour", 0, 0, 0, 1, 1, 0, "Emilio|Bean", 0}};

const u64 MAKER_CLASS_CANNON_RIGHT = 303150949257ULL;
const u64 MAKER_CLASS_CANNON_LEFT = 117211337098ULL;

const MakerObjectDef makerObjectDefs[] = {
    {"Bean Start", 0, OBJ_ITEM, OBJ_START, 0, 0, 32, 32, 0, 0, "Where Bean begins the course"},
    {"Gem", MAKER_CLASS_DIAMOND, OBJ_ITEM, OBJ_RESIZE_X | OBJ_RESIZE_Y, 32, 32, 32, 32,
     MAKER_PARAMS(makerGemParams), "Choose a shape; drag the edges to make rows"},
    {"Axolotl", MAKER_CLASS_AXOLOTL, OBJ_ITEM, OBJ_UNIQUE, 0, 0, 0, 0, 0, 0,
     "The hidden salamander, one per course"},
    {"Potion", MAKER_CLASS_POTION, OBJ_ITEM, 0, 0, 0, 0, 0, MAKER_PARAMS(makerPotionParams),
     "Choose Emilio (walking) or Bean (bouncing)"},
    {"Checkpoint", MAKER_CLASS_CHECKPOINT, OBJ_ITEM, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerCheckpointParams), "Save lamp"},
    {"Goal Spring", MAKER_CLASS_GOAL, OBJ_ITEM, 0, 0, 0, 0, 0, 0, 0, "Finishes the course"},
    {"Helper Bird", 937947234821ULL, OBJ_ITEM, 0, 0, 0, 0, 0, MAKER_PARAMS(makerHelperParams),
     "Carries Bean upwards"},
    {"Bird Perch", 678114862604ULL, OBJ_ITEM, 0, 0, 0, 0, 0, 0, 0, "Bell that recalls the bird"},
    {"Blue Beetle", 14264481342587988014ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerWalkerParams), "Walker"},
    {"Red Beetle", 13516539842961264564ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerWalkerParams), "Fast walker"},
    {"Ruin Turtle", 970132910526ULL, OBJ_ENEMY, 0, 0, 0, 0, 0, MAKER_PARAMS(makerWalkerParams),
     "Armoured walker"},
    {"Orbit Beetle", 3216410667731258ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerOrbitBeetleParams), "Walker guarded by a circling spike ball"},
    {"Bird", 17108215598456197958ULL, OBJ_ENEMY, 0, 0, 0, 0, 0, MAKER_PARAMS(makerOscillatorParams),
     "Patrols back and forth"},
    {"Spider", 8234400683372717661ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerOscillatorParams), "Hangs and patrols"},
    {"Bat", 4746640408543148145ULL, OBJ_ENEMY, 0, 0, 0, 0, 0, MAKER_PARAMS(makerOrbitParams),
     "Circles around a point"},
    {"Frog", 6425677159578671338ULL, OBJ_ENEMY, 0, 0, 0, 0, 0, MAKER_PARAMS(makerJumperParams),
     "Hops towards Bean"},
    {"Red Hopper", 15190319732989319544ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerJumperParams), "Hopping enemy"},
    {"Purple Hopper", 3692840978977075ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerJumperParams), "Hopping enemy"},
    {"Fish", 706166303646ULL, OBJ_ENEMY, 0, 0, 0, 0, 0, MAKER_PARAMS(makerFishParams),
     "Swims diagonally and bounces off walls"},
    {"Spike Eye", 4615236400452776625ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerOscillatorParams), "Deadly patroller"},
    {"Mega Crusher", 481780730859ULL, OBJ_ENEMY, 0, 0, 0, 0, 0, MAKER_PARAMS(makerOscillatorParams),
     "Huge deadly block"},
    {"Spiked Block", 17501560557495693516ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerBoxParams), "Deadly pushable block"},
    {"Cannon", MAKER_CLASS_CANNON_RIGHT, OBJ_ENEMY, OBJ_TURRET, 0, 0, 0, 0,
     MAKER_PARAMS(makerCannonParams), "Fires in a straight line"},
    {"Seeker Turret", 12795324931407506847ULL, OBJ_ENEMY, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerSeekerParams), "Aims at Bean"},
    {"Spikes", 17084625859160385508ULL, OBJ_GIZMO, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_ROTATE, 16, 16,
     32, 16, MAKER_PARAMS(makerSpikeParams), "Press R to rotate; can follow a switch group"},
    {"Spike Ball", 16498888813477113186ULL, OBJ_GIZMO, OBJ_RESIZE_X | OBJ_RESIZE_Y, 32, 32, 32, 32,
     MAKER_PARAMS(makerSpikeParams), "Spike balls; can follow a switch group"},
    {"Spring", 4105260327524925064ULL, OBJ_GIZMO, OBJ_RESIZE_X, 32, 48, 32, 48,
     MAKER_PARAMS(makerSpringParams), "Launches Bean upwards"},
    {"Trampoline", 271445704564135871ULL, OBJ_GIZMO, OBJ_RESIZE_X, 64, 48, 64, 48,
     MAKER_PARAMS(makerTrampolineParams), "Keeps Bean's bounce height"},
    {"Bouncer", 11607860035091918041ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerBouncerParams),
     "Pinball bumper"},
    {"Seesaw", 17963251161137489403ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerSeesawParams),
     "Tilts under weight"},
    {"Spinner", 949589400903ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerSpinnerParams),
     "Rotating seesaw"},
    {"Cloud Platform", 6181314796472045350ULL, OBJ_GIZMO, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerOscillatorParams), "Moving one-way cloud"},
    {"Moving Block", 10681414275196194647ULL, OBJ_GIZMO, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerOscillatorParams), "Large moving block"},
    {"Small Mover", 978266912024ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerOscillatorParams),
     "Small moving block"},
    {"Crumbling Cloud", 14815823507569313204ULL, OBJ_GIZMO, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerCrumbleParams), "Bursts after a bounce, then returns"},
    {"Breakable Block", 12987116682223282109ULL, OBJ_GIZMO, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerBreakableParams), "Cracks with every hit"},
    {"Crate", 2913132245228912724ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerBoxParams),
     "Pushable block"},
    {"Floating Block", 889195010686ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerBoxParams),
     "Weightless physics block"},
    {"Temple Slab", 14016699050793761200ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerBoxParams),
     "Long physics slab"},
    {"Boulder", 13937897602416943507ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerBallParams),
     "Rolling boulder"},
    {"Beach Ball", 14441582509727370349ULL, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerBallParams),
     "Very bouncy ball"},
    {"Pendulum", 10736719071832527943ULL, OBJ_GIZMO, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerPendulumParams), "Swinging spike ball"},
    {"Swing Platform", 16200670544389243959ULL, OBJ_GIZMO, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerPendulumParams), "Swinging platform"},
    {"Magnet Floor", 670936219271ULL, OBJ_GIZMO, OBJ_RESIZE_X | OBJ_RESIZE_Y, 32, 32, 96, 32, 0, 0,
     "Holds Bean in place"},
    {"Switch", MAKER_CLASS_SWITCH, OBJ_GIZMO, 0, 0, 0, 0, 0, MAKER_PARAMS(makerSwitchParams),
     "Toggles switch blocks"},
    {"Switch Block", 9010792101233554060ULL, OBJ_GIZMO, OBJ_RESIZE_X | OBJ_RESIZE_Y, 32, 32, 32, 32,
     MAKER_PARAMS(makerSwitchBlockParams), "Solid while its group is on"},
    {"Touch Block", 11234016302369745107ULL, OBJ_GIZMO, 0, 0, 0, 0, 0,
     MAKER_PARAMS(makerTouchBlockParams), "Flips when touched"},
    {"Enemy Wall", 867386300540ULL, OBJ_GIZMO, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_INVISIBLE, 32, 32,
     32, 64, 0, 0, "Barrier; stops enemies and blocks"},
    {"Bean Wall", 794748294457ULL, OBJ_GIZMO, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_INVISIBLE, 32, 32,
     32, 64, 0, 0, "Barrier; stops only Bean"},
    {"Mover Wall", 4480186787647947ULL, OBJ_GIZMO, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_INVISIBLE, 32,
     32, 32, 64, 0, 0, "Invisible; turns back moving platforms and patrollers"},
    {"Water", 667806108659ULL, OBJ_AREA, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_FREE | OBJ_WATER, 16, 16,
     192, 96, MAKER_PARAMS(makerWaterParams), "Swimmable water with waves"},
    {"Wind", 101248216037ULL, OBJ_AREA, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_FREE | OBJ_WIND, 16, 16,
     64, 256, MAKER_PARAMS(makerWindParams), "Force field"},
    {"Secret Area", 17963820257441817288ULL, OBJ_AREA,
     OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_FREE | OBJ_SECRET | OBJ_INVISIBLE, 16, 16, 128, 96, 0, 0,
     "Fades the front layer while Bean is inside"},
    {"Bubbles", 677112438672ULL, OBJ_AREA, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_FREE | OBJ_PARTICLES,
     16, 16, 128, 128, MAKER_PARAMS(makerParticleParams), "Rising bubbles"},
    {"Vortex", 716596827098ULL, OBJ_AREA, OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_FREE | OBJ_PARTICLES,
     16, 16, 64, 192, MAKER_PARAMS(makerParticleParams), "Swirling sparks"},
    {"Twinkles", 10170490229994225569ULL, OBJ_AREA,
     OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_FREE | OBJ_PARTICLES, 16, 16, 256, 160,
     MAKER_PARAMS(makerParticleParams), "Twinkling stars"}};
const int makerObjectDefCount = sizeof(makerObjectDefs) / sizeof(makerObjectDefs[0]);

const MakerObjectDef *makerDefFor(u64 klass) {
    for (int i = 0; i < makerObjectDefCount; i++)
        if (makerObjectDefs[i].klass == klass)
            return makerObjectDefs + i;
    return 0;
}

Object *makerClass(u64 klass) {
    return klass ? classById(klass) : 0;
}

// Base-1 actors keep their authored rectangle; resizable groups and areas use the object size.
Rect makerClassRect(u64 klass) {
    Object *c = makerClass(klass);
    if (!c)
        return Rect{-16, -16, 16, 16};
    return c->rect;
}

bool makerChoiceLabel(const char *choices, int index, char *out, int size) {
    if (!choices)
        return false;
    for (int i = 0; i < index && *choices; i++) {
        while (*choices && *choices != '|')
            choices++;
        if (*choices)
            choices++;
    }
    int n = 0;
    while (choices[n] && choices[n] != '|' && n < size - 1) {
        out[n] = choices[n];
        n++;
    }
    out[n] = 0;
    return n > 0;
}

float makerAcos(float x) {
    x = clamp(x, -1, 1);
    return bqAtan2(root(1 - x * x), x);
}

// Where an authored oscillation starts, in percent of its travel from the left or bottom end.
// The behaviour starts at its origin and keeps offsets along a signed axis.
float makerAuthoredStart(const Props &b) {
    float lo = b.num("min-move"), hi = b.num("max-move");
    if (hi - lo <= 0)
        return 50;
    bool reverse = b.num("move-x") < 0 || b.num("move-y") < 0;
    return clamp((reverse ? hi : -lo) / (hi - lo) * 100, 0, 100);
}

// Authored pendulum phase: the angle is MaxAngle * cos(pi * (t + TimeOffset) / Period), so one
// Period is a single sweep from side to side.
float makerAuthoredSwingPhase(const Props &b) {
    float period = b.num("Period", 2);
    return period > 0 ? 3.1415927f * b.num("TimeOffset") / period : 0;
}

// TimeOffset that starts a pendulum at an angle, swinging left (angle falling) or right first.
float makerSwingOffset(float angle, float maxAngle, float period, bool right) {
    float phase = makerAcos(maxAngle > 0 ? angle / maxAngle : 1);
    if (right)
        phase = 2 * 3.1415927f - phase;
    return phase * period / 3.1415927f;
}

int makerParamByKey(const MakerObjectDef &d, const char *key) {
    for (int i = 0; i < d.paramCount; i++)
        if (d.params[i].key && eq(d.params[i].key, key))
            return i;
    return -1;
}

// Extra shot angles for each spread choice, in degrees from the main shot.
const char *makerSpreads[4] = {"", "15,-15", "30,-30", "15,-15,30,-30"};

int makerSpreadIndex(const char *extra) {
    for (int i = 0; i < 4; i++)
        if (eq(extra, makerSpreads[i]))
            return i;
    return 0;
}

// A parameter applies when the object's class carries the behaviour it edits.
bool makerParamUsable(const MakerObjectDef &d, int index) {
    if (index < 0 || index >= d.paramCount || index >= MAKER_MAX_PARAMS)
        return false;
    Object *c = makerClass(d.klass);
    return c && d.params[index].behavior && c->has(d.params[index].behavior);
}

float makerParamDefault(const MakerObjectDef &d, int index) {
    if (index < 0 || index >= d.paramCount)
        return 0;
    const MakerParamDef &p = d.params[index];
    if (p.flags & PARAM_USE_DEFAULT)
        return p.def;
    Object *c = makerClass(d.klass);
    const Props *b = c ? c->behavior(p.behavior) : 0;
    if (!b)
        return p.def;
    float v = p.def;
    switch (p.write) {
    case WRITE_SET:
    case WRITE_BOTH:
        v = b->num(p.key, p.def);
        break;
    case WRITE_ACTIVITY:
        v = b->flag("auto-deactivate") ? 2.0f : c->paused ? 1.0f : 0.0f;
        break;
    case WRITE_DEFEAT:
    case WRITE_MASK0: {
        bool on = p.write == WRITE_MASK0 || b->flag(p.key);
        int mask = (int)b->num(p.write == WRITE_MASK0 ? p.key : p.key2);
        v = !on || mask <= 0 ? 0.0f : mask >= 8 ? 4.0f : mask >= 4 ? 3.0f : mask >= 2 ? 2.0f : 1.0f;
        break;
    }
    case WRITE_SPREAD:
        v = (float)makerSpreadIndex(b->get(p.key));
        break;
    case WRITE_HITS:
        v = (float)makerCeil(b->num(p.key, 3) / maxf(1, b->num(p.key2, 3)));
        break;
    case WRITE_BREAK_SIDES:
        v = b->num(p.key) > 0 ? 1.0f : 0.0f;
        break;
    case WRITE_RANGE:
        v = maxf(0, b->num(p.key2) - b->num(p.key));
        break;
    case WRITE_START:
        v = makerAuthoredStart(*b);
        break;
    case WRITE_SWING_ANGLE:
        v = b->num("MaxAngle", 60) * bqCos(makerAuthoredSwingPhase(*b));
        break;
    case WRITE_SWING_DIR:
        v = bqSin(makerAuthoredSwingPhase(*b)) >= -0.0001f ? 0.0f : 1.0f;
        break;
    case WRITE_BULLET_SPEED: {
        Object *bullet = classById(idnum(b->get("bulletClass")));
        const Props *flight = bullet ? bullet->behavior("EnemyBulletBehaviour") : 0;
        v = flight ? flight->num("speed", p.def) : p.def;
        break;
    }
    case WRITE_DIAGONAL:
        v = (float)((b->num("move-x", -1) > 0 ? 1 : 0) + (b->num("move-y", -1) > 0 ? 2 : 0));
        break;
    case WRITE_AXIS:
        v = b->num("move-y") != 0 && b->num("move-x") == 0 ? 1.0f : 0.0f;
        break;
    case WRITE_SIGN:
        v = b->num("move-x") < 0 || b->num("move-y") < 0 ? 1.0f : 0.0f;
        break;
    case WRITE_PERIOD:
        v = absf(b->num(p.key, p.def));
        break;
    case WRITE_PERIOD_SIGN:
        v = b->num(p.key) < 0 ? 1.0f : 0.0f;
        break;
    case WRITE_MASK: {
        int mask = (int)b->num(p.key, 1);
        v = mask >= 8 ? 3.0f : mask >= 4 ? 2.0f : mask >= 2 ? 1.0f : 0.0f;
        break;
    }
    default:
        break;
    }
    if (p.kind != PARAM_NUMBER)
        v = (float)makerRound(v);
    return clamp(v, p.min, p.max);
}

float makerParam(const MakerObject &o, const MakerObjectDef &d, int index) {
    if (index < 0 || index >= d.paramCount)
        return 0;
    const MakerParamDef &p = d.params[index];
    bool set = index < MAKER_MAX_PARAMS && (o.setMask & (1u << index));
    float v = set ? clamp(o.param[index], p.min, p.max) : makerParamDefault(d, index);
    if (p.write == WRITE_SWING_ANGLE) {
        // The start angle stays inside the swing; unset, it follows the authored phase.
        int amp = makerParamByKey(d, "MaxAngle");
        float limit = amp >= 0 ? makerParam(o, d, amp) : 60;
        if (!set) {
            Object *c = makerClass(d.klass);
            const Props *b = c ? c->behavior(p.behavior) : 0;
            v = limit * (b ? bqCos(makerAuthoredSwingPhase(*b)) : 1);
        }
        v = clamp(v, -limit, limit);
    }
    return v;
}

int makerParamIndex(const MakerObjectDef &d, int write) {
    for (int i = 0; i < d.paramCount; i++)
        if (d.params[i].write == write)
            return i;
    return -1;
}

// An oscillator in course terms: the object starts where it is placed, its path spreads
// `start` of the travel behind that point (toward the left or bottom) and the rest ahead.
struct MakerOscillator {
    bool vertical;
    float travel, start, sign; // sign: 1 moves right or up first, -1 left or down
};

MakerOscillator makerOscillator(const MakerObject &o, const MakerObjectDef &d) {
    MakerOscillator m;
    int axis = makerParamIndex(d, WRITE_AXIS), range = makerParamIndex(d, WRITE_RANGE),
        sign = makerParamIndex(d, WRITE_SIGN), start = makerParamIndex(d, WRITE_START);
    m.vertical = axis >= 0 && makerParam(o, d, axis) > 0.5f;
    m.travel = range >= 0 ? makerParam(o, d, range) : 0;
    m.start = start >= 0 ? makerParam(o, d, start) / 100 : 0.5f;
    m.sign = sign >= 0 && makerParam(o, d, sign) > 0.5f ? -1.0f : 1.0f;
    return m;
}

// Behaviour offsets along the signed axis that keep the path in place around the start point.
void makerOscillatorOffsets(const MakerOscillator &m, float *lo, float *hi) {
    float before = m.start * m.travel, after = m.travel - before;
    *lo = m.sign > 0 ? -before : -after;
    *hi = m.sign > 0 ? after : before;
}

u64 makerEffectiveClass(const MakerObject &o, const MakerObjectDef &d) {
    if (d.flags & OBJ_TURRET) {
        int direction = makerRound(makerParam(o, d, 0));
        return direction == 2 ? MAKER_CLASS_CANNON_LEFT : MAKER_CLASS_CANNON_RIGHT;
    }
    return d.klass;
}

float makerEffectiveAngle(const MakerObject &o, const MakerObjectDef &d) {
    if (d.flags & OBJ_TURRET) {
        const float angles[4] = {0, 90, 0, 270};
        return angles[makerRound(makerParam(o, d, 0)) & 3];
    }
    return o.angle;
}

// Size an object initially, keeping actlet groups whole cells and actors at authored size.
void makerDefaultSize(const MakerObjectDef &d, float *w, float *h) {
    if (d.flags & (OBJ_RESIZE_X | OBJ_RESIZE_Y)) {
        *w = d.defW;
        *h = d.defH;
        return;
    }
    if (d.flags & OBJ_START) {
        *w = *h = 32;
        return;
    }
    Rect r = makerClassRect(d.klass);
    *w = r.r - r.l;
    *h = r.t - r.b;
}

bool makerHazard(const MakerObjectDef *d) {
    return d && (d->category == OBJ_ENEMY || d->klass == 17084625859160385508ULL ||
                 d->klass == 16498888813477113186ULL);
}

void makerThumbFill(u8 *thumb, float s, float ox, float oy, float l, float b, float r, float t,
                    u8 code) {
    int x0 = (int)(ox + l * s), x1 = (int)(ox + r * s), y0 = (int)(oy + b * s),
        y1 = (int)(oy + t * s);
    if (x1 <= x0)
        x1 = x0 + 1;
    if (y1 <= y0)
        y1 = y0 + 1;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (x >= 0 && x < MAKER_THUMB_W && y >= 0 && y < MAKER_THUMB_H)
                thumb[y * MAKER_THUMB_W + x] = code;
}

enum MakerThumbCode {
    THUMB_EMPTY,
    THUMB_SKY,
    THUMB_BACK,
    THUMB_SOLID,
    THUMB_DECOR,
    THUMB_FRONT,
    THUMB_HAZARD,
    THUMB_ITEM,
    THUMB_GOAL,
    THUMB_OBJECT,
    THUMB_START
};

// Statistics and the schematic thumbnail are stored in the header for the course browser.
void makerPrepareHeader(MakerLevel &level) {
    MakerHeader &h = level.head;
    if (!h.name[0])
        lstrcpyA(h.name, "Untitled");
    h.coins = h.hasAxolotl = h.goals = 0;
    for (int i = 0; i < level.objectCount; i++) {
        MakerObject &o = level.objects[i];
        if (o.klass == MAKER_CLASS_DIAMOND)
            h.coins += (int)maxf(1, (float)makerRound(o.w / 32)) *
                       (int)maxf(1, (float)makerRound(o.h / 32));
        if (o.klass == MAKER_CLASS_AXOLOTL)
            h.hasAxolotl = 1;
        if (o.klass == MAKER_CLASS_GOAL)
            h.goals++;
    }
    memset(h.thumb, THUMB_EMPTY, sizeof(h.thumb));
    float s = minf(MAKER_THUMB_W / (float)h.width, MAKER_THUMB_H / (float)h.height);
    float ox = (MAKER_THUMB_W - h.width * s) * 0.5f, oy = (MAKER_THUMB_H - h.height * s) * 0.5f;
    makerThumbFill(h.thumb, s, ox, oy, 0, 0, (float)h.width, (float)h.height, THUMB_SKY);
    for (int pass = 0; pass < 3; pass++)
        for (int i = 0; i < level.tileCount; i++) {
            MakerTile &t = level.tiles[i];
            int layer = t.layer == MAKER_BACK ? 0 : t.layer == MAKER_MAIN ? 1 : 2;
            if (layer != pass)
                continue;
            u8 code = layer == 0                    ? THUMB_BACK
                      : layer == 2                  ? THUMB_FRONT
                      : t.collide == COLLIDE_NONE   ? THUMB_DECOR
                      : t.collide == COLLIDE_ONEWAY ? THUMB_DECOR
                                                    : THUMB_SOLID;
            makerThumbFill(h.thumb, s, ox, oy, t.l, t.b, t.r, t.t, code);
        }
    for (int i = 0; i < level.objectCount; i++) {
        MakerObject &o = level.objects[i];
        const MakerObjectDef *d = makerDefFor(o.klass);
        bool item = o.klass == MAKER_CLASS_DIAMOND || o.klass == MAKER_CLASS_AXOLOTL ||
                    o.klass == MAKER_CLASS_POTION;
        u8 code = o.klass == MAKER_CLASS_GOAL ? THUMB_GOAL
                  : item                    ? THUMB_ITEM
                  : makerHazard(d)          ? THUMB_HAZARD
                                            : THUMB_OBJECT;
        if (d && (d->flags & (OBJ_INVISIBLE | OBJ_PARTICLES)))
            continue;
        makerThumbFill(h.thumb, s, ox, oy, o.x - o.w * 0.5f, o.y - o.h * 0.5f, o.x + o.w * 0.5f,
                       o.y + o.h * 0.5f, code);
    }
    makerThumbFill(h.thumb, s, ox, oy, h.startX - 16, h.startY - 16, h.startX + 16, h.startY + 16,
                   THUMB_START);
}

// A fresh course: theme defaults, a starting floor, a landing strip and the goal spring.
void makerNewCourse(MakerLevel &level, int theme) {
    makerAllocate(level);
    memset(&level.head, 0, sizeof(level.head));
    MakerHeader &h = level.head;
    h.magic = MAKER_MAGIC;
    h.version = MAKER_VERSION;
    h.uid = makerNewUid();
    lstrcpyA(h.name, "New Course");
    h.theme = (int)clamp((float)theme, 1, 5);
    h.background = makerThemeBackground(h.theme);
    h.music = MAKER_MUSIC_THEME;
    h.width = 2560;
    h.height = 384;
    h.startX = 96;
    h.startY = 112;
    h.jumpBest = 10;
    h.jumpPar = 15;
    level.tileCount = level.objectCount = 0;
    int block = -1;
    if (makerCatalogReady)
        for (int i = 0; i < makerCategoryCount[PIECE_BLOCK]; i++) {
            const MakerPiece &p = makerPieces[makerCategoryList[PIECE_BLOCK][i]];
            if (p.group == h.theme && p.w == 32 && p.h == 32) {
                block = makerCategoryList[PIECE_BLOCK][i];
                break;
            }
        }
    if (block >= 0) {
        const MakerPiece &p = makerPieces[block];
        for (int x = 0; x < h.width; x += 32) {
            if (x >= 384 && x < h.width - 448)
                continue;
            for (int y = 0; y < 64; y += 32) {
                MakerTile &t = level.tiles[level.tileCount++];
                memset(&t, 0, sizeof(t));
                t.l = (float)x;
                t.b = (float)y;
                t.r = (float)(x + 32);
                t.t = (float)(y + 32);
                memcpy(t.uv, p.uv, sizeof(t.uv));
                t.group = p.group;
                t.texture = p.texture;
                t.layer = MAKER_MAIN;
                t.collide = COLLIDE_SOLID;
                t.flags = TILE_OPAQUE;
            }
        }
    }
    MakerObject &goal = level.objects[level.objectCount++];
    memset(&goal, 0, sizeof(goal));
    goal.klass = MAKER_CLASS_GOAL;
    Rect r = makerClassRect(MAKER_CLASS_GOAL);
    goal.w = r.r - r.l;
    goal.h = r.t - r.b;
    goal.x = (float)(h.width - 160);
    goal.y = 64 + goal.h * 0.5f;
}
