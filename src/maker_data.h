#pragma once

// Course-maker data model and storage. Courses live in saves/maker as versioned, checksummed files
// written atomically; play records sit in the course header and reset whenever its content changes.

enum {
    MAKER_SLOTS = 60,
    MAKER_PAGE = 12,
    MAKER_MAX_TILES = 8000,
    MAKER_MAX_OBJECTS = 1500,
    MAKER_MAX_PARAMS = 12,
    MAKER_NAME = 32,
    MAKER_THUMB_W = 96,
    MAKER_THUMB_H = 32,
    MAKER_MIN_W = 512,
    MAKER_MAX_W = 16000,
    MAKER_MIN_H = 384,
    MAKER_MAX_H = 4800
};

enum MakerLayer { MAKER_BACK, MAKER_MAIN, MAKER_FRONT, MAKER_LAYERS };

// Slopes name the solid corner: BR fills the bottom-right half and rises to the right.
enum MakerCollide {
    COLLIDE_NONE,
    COLLIDE_SOLID,
    COLLIDE_ONEWAY,
    COLLIDE_SLOPE_BR,
    COLLIDE_SLOPE_BL,
    COLLIDE_SLOPE_TR,
    COLLIDE_SLOPE_TL,
    COLLIDE_KINDS
};

enum MakerFlags { MAKER_VERIFIED = 1 };

const u64 MAKER_CLASS_BEAN = 11120285848841938423ULL;
const u64 MAKER_CLASS_CAMERA = 6094896174703657971ULL;
const u64 MAKER_CLASS_DIAMOND = 314060601782828169ULL;
const u64 MAKER_CLASS_AXOLOTL = 13485834099348442826ULL;
const u64 MAKER_CLASS_POTION = 8048939671444015ULL;
const u64 MAKER_CLASS_GOAL = 11867101362052293448ULL;
const u64 MAKER_CLASS_CHECKPOINT = 11580609897187444548ULL;
const u64 MAKER_CLASS_SWITCH = 2001572250164425163ULL;
const u64 MAKER_CLASS_SWITCH_ICON = 10007346385132406622ULL;

// Fill tiles stretch one atlas strip over any rectangle.
enum MakerTileFlags { TILE_OPAQUE = 1, TILE_FILL = 2 };

// One placed atlas piece. Corner UVs follow the level vertex order: bottom-left, bottom-right,
// top-left, top-right. Group 1..5 selects the worldN texture group, texture its atlas page.
// Opaque blocks replace each other in place; overlays such as grass stack above them.
struct MakerTile {
    float l, b, r, t;
    float uv[8];
    u8 group, texture, layer, collide;
    short surface; // one-way surface depth below the top edge
    short flags;
};

// One placed object. The class identifies its catalog entry; params are catalog-defined values.
struct MakerObject {
    u64 klass;
    float x, y, w, h, angle;
    u32 setMask;
    float param[MAKER_MAX_PARAMS];
};

enum { MAKER_MUSIC_THEME = 0, MAKER_MUSIC_SILENT = 1 };

struct MakerHeader {
    u32 magic, version;
    u64 uid;
    u64 music; // a project music id, or the theme/silent markers above
    u32 revision;
    char name[MAKER_NAME];
    int theme, background, effect;
    int width, height;
    float startX, startY;
    int jumpBest, jumpPar, coinGoal;
    int flags;
    int tileCount, objectCount;
    int cleared, bestJumps, bestCoins, axolotl;
    int coins, hasAxolotl, goals;
    u8 thumb[MAKER_THUMB_W * MAKER_THUMB_H];
    int beanSpeed, beanJump;        // percent of Bean's normal run speed and jump height, 0 = 100
    int camera;                     // 0 follows both axes, 1 horizontal only, 2 vertical only
    int cameraAheadX, cameraAheadY; // camera offset in percent of the view
    int weatherAmount;              // percent of the weather's normal density, 0 = 100
    u32 reserved[10];
};

struct MakerLevel {
    MakerHeader head;
    MakerTile *tiles;
    MakerObject *objects;
    int tileCount, objectCount;
};

struct MakerProfile {
    u32 magic, version;
    int unlocked, seen, lastSlot, page;
    int reserved[8];
    u32 checksum;
};

const u32 MAKER_MAGIC = 0x4b4d5142;   // "BQMK"
const u32 MAKER_PROFILE = 0x504d5142; // "BQMP"
const u32 MAKER_VERSION = 2;          // 2: twelve parameters per object

MakerProfile makerProfile;
bool makerProfileLoaded;

void makerPrepareHeader(MakerLevel &level);

void makerAllocate(MakerLevel &level) {
    if (!level.tiles)
        level.tiles = (MakerTile *)mkAlloc(sizeof(MakerTile) * MAKER_MAX_TILES);
    if (!level.objects)
        level.objects = (MakerObject *)mkAlloc(sizeof(MakerObject) * MAKER_MAX_OBJECTS);
}

void makerDirectory(char *out) {
    savePath(out, "maker");
    CreateDirectoryA(saveRoot, 0);
    CreateDirectoryA(out, 0);
}

void makerCoursePath(int slot, char *out) {
    char dir[MAX_PATH], name[32];
    makerDirectory(dir);
    wsprintfA(name, "/course%02d.bqm", slot + 1);
    lstrcpyA(out, dir);
    lstrcatA(out, name);
}

// Unique course identity without 64-bit multiplication (no CRT helpers are linked).
u64 makerNewUid() {
    static u32 counter;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    u64 time = ((u64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    u64 mix = time ^ ((u64)GetTickCount() << 21) ^ ((u64)GetCurrentProcessId() << 40) ^
              ((u64)(++counter) << 9);
    mix ^= mix >> 29;
    mix ^= mix << 17;
    return mix | (1ULL << 63);
}

void makerWriteProfile() {
    makerProfile.magic = MAKER_PROFILE;
    makerProfile.version = 1;
    makerProfile.checksum = hashBytes(&makerProfile, sizeof(makerProfile) - 4);
    char dir[MAX_PATH], path[MAX_PATH];
    makerDirectory(dir);
    lstrcpyA(path, dir);
    lstrcatA(path, "/maker.dat");
    atomicWrite(path, &makerProfile, sizeof(makerProfile));
}

void makerReadProfile() {
    if (makerProfileLoaded)
        return;
    makerProfileLoaded = true;
    char dir[MAX_PATH], path[MAX_PATH];
    savePath(dir, "maker");
    lstrcpyA(path, dir);
    lstrcatA(path, "/maker.dat");
    MakerProfile p;
    if (exactRead(path, &p, sizeof(p)) && p.magic == MAKER_PROFILE && p.version == 1 &&
        p.checksum == hashBytes(&p, sizeof(p) - 4))
        makerProfile = p;
    else
        memset(&makerProfile, 0, sizeof(makerProfile));
    makerProfile.lastSlot = (int)clamp((float)makerProfile.lastSlot, 0, MAKER_SLOTS - 1);
    makerProfile.page = (int)clamp((float)makerProfile.page, 0, MAKER_SLOTS / MAKER_PAGE - 1);
}

bool makerUnlocked() {
    makerReadProfile();
    return makerProfile.unlocked != 0;
}

void makerUnlock() {
    makerReadProfile();
    if (makerProfile.unlocked)
        return;
    makerProfile.unlocked = 1;
    makerProfile.seen = 0;
    makerWriteProfile();
}

u8 *makerReadWhole(const char *path, u32 *size) {
    *size = 0;
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE)
        return 0;
    u32 n = GetFileSize(f, 0);
    if (n == INVALID_FILE_SIZE || n > 64 * 1024 * 1024) {
        CloseHandle(f);
        return 0;
    }
    u8 *b = (u8 *)mkAlloc(n + 1);
    DWORD done = 0;
    BOOL ok = ReadFile(f, b, n, &done, 0);
    CloseHandle(f);
    if (!ok || done != n) {
        mkFree(b);
        return 0;
    }
    *size = n;
    return b;
}

bool makerValidHeader(const MakerHeader &h) {
    return h.magic == MAKER_MAGIC && h.version == MAKER_VERSION && h.tileCount >= 0 &&
           h.tileCount <= MAKER_MAX_TILES && h.objectCount >= 0 &&
           h.objectCount <= MAKER_MAX_OBJECTS && h.width >= MAKER_MIN_W && h.width <= MAKER_MAX_W &&
           h.height >= MAKER_MIN_H && h.height <= MAKER_MAX_H;
}

bool makerLoadHeader(int slot, MakerHeader &out) {
    char path[MAX_PATH];
    makerCoursePath(slot, path);
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    DWORD n = 0;
    bool ok = ReadFile(f, &out, sizeof(out), &n, 0) && n == sizeof(out);
    CloseHandle(f);
    out.name[MAKER_NAME - 1] = 0;
    return ok && makerValidHeader(out);
}

bool makerLoadLevel(int slot, MakerLevel &level) {
    makerAllocate(level);
    char path[MAX_PATH];
    makerCoursePath(slot, path);
    u32 size;
    u8 *bytes = makerReadWhole(path, &size);
    if (!bytes)
        return false;
    bool ok = false;
    if (size >= sizeof(MakerHeader) + 4) {
        MakerHeader h;
        memcpy(&h, bytes, sizeof(h));
        u32 expected = sizeof(MakerHeader) + h.tileCount * sizeof(MakerTile) +
                       h.objectCount * sizeof(MakerObject) + 4;
        if (makerValidHeader(h) && size == expected &&
            *(u32 *)(bytes + size - 4) == hashBytes(bytes, size - 4)) {
            level.head = h;
            level.head.name[MAKER_NAME - 1] = 0;
            level.tileCount = h.tileCount;
            level.objectCount = h.objectCount;
            memcpy(level.tiles, bytes + sizeof(MakerHeader), h.tileCount * sizeof(MakerTile));
            memcpy(level.objects, bytes + sizeof(MakerHeader) + h.tileCount * sizeof(MakerTile),
                   h.objectCount * sizeof(MakerObject));
            ok = true;
        }
    }
    mkFree(bytes);
    return ok;
}

bool makerSaveLevel(int slot, MakerLevel &level) {
    makerPrepareHeader(level);
    level.head.magic = MAKER_MAGIC;
    level.head.version = MAKER_VERSION;
    level.head.tileCount = level.tileCount;
    level.head.objectCount = level.objectCount;
    u32 size = sizeof(MakerHeader) + level.tileCount * sizeof(MakerTile) +
               level.objectCount * sizeof(MakerObject) + 4;
    u8 *bytes = (u8 *)mkAlloc(size);
    memcpy(bytes, &level.head, sizeof(MakerHeader));
    memcpy(bytes + sizeof(MakerHeader), level.tiles, level.tileCount * sizeof(MakerTile));
    memcpy(bytes + sizeof(MakerHeader) + level.tileCount * sizeof(MakerTile), level.objects,
           level.objectCount * sizeof(MakerObject));
    *(u32 *)(bytes + size - 4) = hashBytes(bytes, size - 4);
    char path[MAX_PATH];
    makerCoursePath(slot, path);
    bool ok = atomicWrite(path, bytes, size);
    mkFree(bytes);
    return ok;
}

void makerDeleteSlot(int slot) {
    char path[MAX_PATH];
    makerCoursePath(slot, path);
    DeleteFileA(path);
}

// Content edits invalidate verification and every play record of the course.
void makerResetRecords(MakerHeader &h) {
    h.cleared = h.bestJumps = h.bestCoins = h.axolotl = 0;
    h.flags &= ~MAKER_VERIFIED;
    h.revision++;
}
