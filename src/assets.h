#pragma once

// Parses authored project, level, texture and animation resources and owns the shared actor cache.
#include "platform.h"
#include "world_card_texture_repair.h"

struct Behavior {
    u64 id;
    char *type;
    Props props;
};

struct Object {
    u64 id, base, klass;
    int paused, visible, z;
    Rect rect;
    float angle;
    Props props;
    int behaviorCount;
    Behavior *behaviors;
    int layer;

    const Props *behavior(const char *name) const {
        for (int i = 0; i < behaviorCount; i++)
            if (eq(behaviors[i].type, name))
                return &behaviors[i].props;
        return 0;
    }

    bool has(const char *name) const {
        return behavior(name) != 0;
    }
};

Object objectRead(Reader &r) {
    Object o = {};
    o.id = r.q();
    o.base = r.q();
    o.klass = r.q();
    o.paused = r.i();
    o.visible = r.i();
    o.z = r.i();
    o.rect = *(Rect *)r.take(16);
    o.angle = r.f();
    o.props = r.props();
    o.behaviorCount = r.i();
    o.behaviors = r.arena->array<Behavior>(o.behaviorCount);
    for (int i = 0; i < o.behaviorCount; i++) {
        o.behaviors[i].id = r.q();
        o.behaviors[i].type = r.s();
        o.behaviors[i].props = r.props();
    }
    return o;
}

struct LevelInfo {
    u64 id;
    char *file;
    Props props;
};

struct MusicInfo {
    u64 id;
    char *file;
    Props props;
};

LevelInfo *levels;
int levelCount, stageCount;
int stageList[80], legacyStageList[80];
MusicInfo *musicInfo;
int musicCount;
Object *classes;
int classCount;
Props soundFiles;

// Read the binary project tables and reconstruct authored stage progression from next-level links.
void loadProject() {
    u32 n;
    u8 *b = readFile("data/Media/project.pgame", &n, permanent);
    Reader r(b, n, permanent);
    r.take(16);
    r.props();
    levelCount = r.i();
    levels = permanent.array<LevelInfo>(levelCount);
    for (int i = 0; i < levelCount; i++) {
        levels[i].id = r.q();
        levels[i].file = r.s();
        levels[i].props = r.props();
        if (levels[i].props.num("bean-world") > 0)
            stageList[stageCount++] = i;
    }
    for (int i = 0; i < stageCount; i++)
        for (int j = i + 1; j < stageCount; j++) {
            const Props &a = levels[stageList[i]].props;
            const Props &b2 = levels[stageList[j]].props;
            if (a.num("bean-world") * 100 + a.num("bean-level") >
                b2.num("bean-world") * 100 + b2.num("bean-level")) {
                int t = stageList[i];
                stageList[i] = stageList[j];
                stageList[j] = t;
            }
        }
    for (int i = 0; i < stageCount; i++)
        legacyStageList[i] = stageList[i];
    // Preserve the authored progression. Internal world IDs were retained when
    // Dusty Desert moved before Crystal Peaks in the released game.
    int ordered[80], count = 0;
    u64 next = 6838399004045653085ULL;
    while (next && count < stageCount) {
        int found = -1;
        for (int i = 0; i < levelCount; i++)
            if (levels[i].id == next) {
                found = i;
                break;
            }
        if (found < 0)
            break;
        bool duplicate = false;
        for (int i = 0; i < count; i++)
            if (ordered[i] == found)
                duplicate = true;
        if (duplicate)
            break;
        ordered[count++] = found;
        next = idnum(levels[found].props.get("next-level"));
    }
    if (count != stageCount)
        fatal("Incomplete authored stage progression.");
    for (int i = 0; i < stageCount; i++)
        stageList[i] = ordered[i];
    soundFiles = r.props();
    musicCount = r.i();
    musicInfo = permanent.array<MusicInfo>(musicCount);
    for (int i = 0; i < musicCount; i++) {
        musicInfo[i].id = r.q();
        musicInfo[i].file = r.s();
        musicInfo[i].props = r.props();
    }
    classCount = r.i();
    classes = permanent.array<Object>(classCount);
    for (int i = 0; i < classCount; i++)
        classes[i] = objectRead(r);
    if (r.pos != r.size)
        fatal("Unsupported project resource version.");
}

struct Texture {
    GLuint gl;
    int width, height;
};

Texture textureRead(Reader &r, u64 sceneId = 0, int textureIndex = -1) {
    Texture t = {};
    r.take(16);
    int h[10];
    memcpy(h, r.take(40), 40);
    t.width = h[3];
    t.height = h[4];
    if (t.width < 1 || t.width > 4096 || t.height < 1 || t.height > 4096 || h[7] != 6408 ||
        h[8] != 5121 || h[9] != t.width * t.height * 4)
        fatal("Unsupported texture format.");
    void *pixels = r.take(h[9]);
    repairWorldCardTexture(sceneId, textureIndex, t.width, t.height, (u8 *)pixels);
    glGenTextures(1, &t.gl);
    glBindTexture(GL_TEXTURE_2D, t.gl);
    // OpenGLESTexture2D::Load 0x39a370/0x39a390 uses the authored MAG/MIN
    // filters. Linear particle atlases also ship their complete mip chain.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, h[5]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, h[6]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.width, t.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels);
    int w = t.width, hh = t.height;
    for (int mip = 1; mip < h[2]; mip++) {
        w = w > 1 ? w / 2 : 1;
        hh = hh > 1 ? hh / 2 : 1;
        int bytes = r.i();
        if (bytes != w * hh * 4)
            fatal("Unsupported texture mip level.");
        glTexImage2D(GL_TEXTURE_2D, mip, GL_RGBA, w, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     r.take(bytes));
    }
    return t;
}

struct Frame {
    int duration, texture;
    float uv[8];
};

struct Animation {
    char *name;
    int duration, count;
    Frame *frames;
};

struct Actor {
    u64 id;
    int textureCount;
    Texture *textures;
    int count;
    Animation *animations;
};

#include "potion_texture_alignment.h"
Actor actors[512];
int actorCount;

// Cache each actor resource once; frame normalization changes only the cached potion UV layout.
Actor *actor(u64 id) {
    for (int i = 0; i < actorCount; i++)
        if (actors[i].id == id)
            return actors + i;
    if (actorCount >= 512)
        fatal("Actor cache is full.");
    Actor *a = actors + actorCount++;
    a->id = id;
    char path[128], num[24];
    idstr(id, num);
    lstrcpyA(path, "data/Media/Anims/");
    lstrcatA(path, num);
    lstrcatA(path, ".panims");
    u32 n;
    u8 *b = readFile(path, &n, permanent, false);
    if (!b)
        return a;
    Reader r(b, n, permanent);
    r.take(16);
    a->textureCount = r.i();
    a->textures = permanent.array<Texture>(a->textureCount);
    for (int i = 0; i < a->textureCount; i++)
        a->textures[i] = textureRead(r);
    a->count = r.i();
    a->animations = permanent.array<Animation>(a->count);
    for (int i = 0; i < a->count; i++) {
        Animation &an = a->animations[i];
        an.name = r.s();
        an.duration = r.i();
        an.count = r.i();
        an.frames = permanent.array<Frame>(an.count);
        memcpy(an.frames, r.take(an.count * sizeof(Frame)), an.count * sizeof(Frame));
    }
    if (r.pos != r.size)
        fatal("Unsupported animation data.");
    alignPotionTextureFrames(*a);
    alignSwitchTextureFrames(*a);
    return a;
}

Animation *animation(Actor *a, const char *name) {
    if (!a || !a->count)
        return 0;
    for (int i = 0; i < a->count; i++)
        if (eq(a->animations[i].name, name))
            return a->animations + i;
    return a->animations;
}

enum AnimationEndAction { ANIMATION_LOOP = 0, ANIMATION_STOP = 1, ANIMATION_PING_PONG = 2 };

float animationLength(const Animation *a) {
    if (!a || a->duration <= 0)
        return 0;
    int ticks = 0;
    for (int i = 0; i < a->count; i++)
        ticks += a->frames[i].duration;
    return (float)ticks / a->duration;
}

// Native Animation::GetFrameData (0x103b1c) chooses the last frame whose
// cumulative start time is <= the instance time. Individual frames have weights.
int animationFrame(const Animation *a, float time, AnimationEndAction action = ANIMATION_LOOP,
                   float speed = 1, bool reverse = false) {
    if (!a || a->count <= 1 || a->duration <= 0)
        return 0;
    float length = animationLength(a);
    if (length <= 0)
        return 0;
    time *= speed;
    if (reverse)
        time = length - time;
    if (action == ANIMATION_STOP)
        time = clamp(time, 0, length);
    else {
        float cycle = action == ANIMATION_PING_PONG ? length * 2 : length;
        time -= (int)(time / cycle) * cycle;
        if (time < 0)
            time += cycle;
        if (action == ANIMATION_PING_PONG && time > length)
            time = cycle - time;
    }
    if (time >= length)
        return a->count - 1;
    int endTick = 0;
    for (int i = 0; i < a->count; i++) {
        endTick += a->frames[i].duration;
        if (time < (float)endTick / a->duration)
            return i;
    }
    return a->count - 1;
}

// On disk: id, scroll, auto-scroll dimensions, size, scissor/visible/centre.
// Runtime offset is separate (LevelLayer::GetOffset at native +0x64).
struct Layer {
    u64 id;
    Vec scroll, autoScroll, size;
    int flags[3];
    Vec offset;
};

struct Vertex {
    float x, y, u, v;
};

struct Tile {
    int layer, z, flags, texture, count;
    Rect rect;
    int meta[5];
    int bytes;
    Vertex *vertices;
};

struct Geometry {
    int kind, flags, count;
    Vec *points;
};

struct Level {
    Vec size;
    u64 id;
    Props props;
    int layerCount;
    Layer *layers;
    int textureCount;
    Texture *textures;
    int tileCount;
    Tile *tiles;
    int geometryCount;
    Geometry *geometry;
    int objectCount;
    Object *objects;
    bool sharedTextures; // textures belong to the maker atlas cache, not to this scene
};

Level current;

void releaseLevel(Level &l, Arena &memory) {
    if (!l.sharedTextures)
        for (int i = 0; i < l.textureCount; i++)
            glDeleteTextures(1, &l.textures[i].gl);
    memset(&l, 0, sizeof(l));
    memory.reset();
}

void unloadLevel() {
    releaseLevel(current, levelArena);
}

// Read level layers, terrain and object instances into the arena owned by that scene.
void readLevelData(const char *file, Level &l, Arena &memory) {
    releaseLevel(l, memory);
    char path[128];
    lstrcpyA(path, "data/Media/Levels/");
    lstrcatA(path, file);
    u32 n;
    u8 *b = readFile(path, &n, memory);
    Reader r(b, n, memory);
    r.take(16);
    l.size = *(Vec *)r.take(8);
    l.id = r.q();
    l.props = r.props();
    l.layerCount = r.i();
    l.layers = memory.array<Layer>(l.layerCount);
    for (int i = 0; i < l.layerCount; i++)
        memcpy(l.layers + i, r.take(44), 44);
    r.take(16);
    int nt = r.i();
    if (nt == -1) {
        r.s();
        char *tf = r.s();
        lstrcpyA(path, "data/Media/Texture Groups/");
        lstrcatA(path, tf);
        u32 tn;
        u8 *tb = readFile(path, &tn, memory);
        Reader tr(tb, tn, memory);
        tr.take(16);
        l.textureCount = tr.i();
        l.textures = memory.array<Texture>(l.textureCount);
        for (int i = 0; i < l.textureCount; i++)
            l.textures[i] = textureRead(tr);
    } else {
        l.textureCount = nt;
        l.textures = memory.array<Texture>(nt);
        for (int i = 0; i < nt; i++)
            l.textures[i] = textureRead(r, l.id, i);
    }
    l.tileCount = r.i();
    l.tiles = memory.array<Tile>(l.tileCount);
    for (int i = 0; i < l.tileCount; i++) {
        Tile &t = l.tiles[i];
        memcpy(&t, r.take(60), 60);
        t.vertices = (Vertex *)r.take(t.bytes);
        if (t.bytes < 0 || t.bytes % 64)
            fatal("Invalid tile geometry.");
    }
    r.take(16);
    l.geometryCount = r.i();
    l.geometry = memory.array<Geometry>(l.geometryCount);
    for (int i = 0; i < l.geometryCount; i++) {
        Geometry &g = l.geometry[i];
        g.kind = r.i();
        g.flags = r.i();
        g.count = r.i();
        g.points = memory.array<Vec>(g.count);
        for (int j = 0; j < g.count; j++) {
            g.points[j].x = (float)r.i();
            g.points[j].y = (float)r.i();
        }
    }
    l.objectCount = r.i();
    l.objects = memory.array<Object>(l.objectCount);
    for (int i = 0; i < l.objectCount; i++) {
        l.objects[i] = objectRead(r);
        l.objects[i].layer = r.i();
    }
    if (r.pos != r.size)
        fatal("Unsupported level data.");
}

void loadLevelData(const char *file) {
    readLevelData(file, current, levelArena);
}

LevelInfo *levelById(u64 id) {
    for (int i = 0; i < levelCount; i++)
        if (levels[i].id == id)
            return levels + i;
    return 0;
}

int stageById(u64 id) {
    for (int i = 0; i < stageCount; i++)
        if (levels[stageList[i]].id == id)
            return i;
    return -1;
}
