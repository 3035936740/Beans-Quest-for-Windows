#pragma once

// CRT-free platform utilities, bounded resource readers, arenas and atomic file writes.
#ifndef BQ_ENABLE_TESTS
#define BQ_ENABLE_TESTS 0
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned __int64 u64;
extern "C" int _fltused = 0;
extern "C" void *__cdecl memset(void *dst, int c, unsigned int n);
extern "C" void *__cdecl memcpy(void *dst, const void *src, unsigned int n);
extern "C" float bqAtan2(float, float);
extern "C" float bqCos(float);
extern "C" float bqSin(float);
extern "C" float bqPow(float, float);

float absf(float x) {
    return x < 0 ? -x : x;
}

float minf(float a, float b) {
    return a < b ? a : b;
}

float maxf(float a, float b) {
    return a > b ? a : b;
}

float clamp(float x, float lo, float hi) {
    return minf(maxf(x, lo), hi);
}

float root(float x) {
    if (x <= 0)
        return 0;
    float r = x > 1 ? x : 1;
    for (int i = 0; i < 16; i++)
        r = (r + x / r) * 0.5f;
    return r;
}

float sine(float x) {
    while (x > 3.14159265f)
        x -= 6.2831853f;
    while (x < -3.14159265f)
        x += 6.2831853f;
    float y = 1.27323954f * x - 0.405284735f * x * absf(x);
    return 0.225f * (y * absf(y) - y) + y;
}

float cosine(float x) {
    return sine(x + 1.57079633f);
}

// Resource identifiers and property names require exact byte equality. Using
// Windows' locale collation here repeated an expensive API call for every
// property of every water particle on every frame.
bool eq(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

bool contains(const char *s, const char *sub) {
    for (; *s; s++) {
        const char *a = s;
        const char *b = sub;
        while (*a && *a == *b) {
            a++;
            b++;
        }
        if (!*b)
            return true;
    }
    return false;
}

float number(const char *s, float fallback = 0) {
    if (!s || !*s)
        return fallback;
    bool neg = *s == '-';
    if (neg || *s == '+')
        s++;
    float a = 0, scale = 1;
    while (*s >= '0' && *s <= '9')
        a = a * 10 + (*s++ - '0');
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            scale *= 0.1f;
            a += (*s++ - '0') * scale;
        }
    }
    return neg ? -a : a;
}

u64 idnum(const char *s) {
    u64 r = 0;
    for (; *s >= '0' && *s <= '9'; s++)
        r = (r << 3) + (r << 1) + (*s - '0');
    return r;
}

u64 div10(u64 n, int *rem) {
    u64 q = 0;
    u32 r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | (u32)((n >> i) & 1);
        if (r >= 10) {
            r -= 10;
            q |= (u64)1 << i;
        }
    }
    *rem = (int)r;
    return q;
}

void idstr(u64 id, char *out) {
    char tmp[24];
    int n = 0;
    do {
        int rem;
        id = div10(id, &rem);
        tmp[n++] = (char)('0' + rem);
    } while (id);
    for (int i = 0; i < n; i++)
        out[i] = tmp[n - i - 1];
    out[n] = 0;
}

void fatal(const char *s) {
#if BQ_ENABLE_TESTS
    if (contains(GetCommandLineA(), "--self-test")) {
        CreateDirectoryA("test-output", 0);
        HANDLE f = CreateFileA("test-output/fatal.txt", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD done;
            WriteFile(f, s, lstrlenA(s), &done, 0);
            CloseHandle(f);
        }
    } else
#endif
        MessageBoxA(0, s, "Bean's Quest", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}

struct Arena {
    u8 *data;
    u32 used, capacity;

    void init(u32 size) {
        capacity = size;
        data = (u8 *)VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!data)
            fatal("Not enough memory.");
        used = 0;
    }

    void reset() {
        used = 0;
    }

    void *alloc(u32 n) {
        n = (n + 7) & ~7u;
        if (n > capacity - used)
            fatal("Resource memory limit exceeded.");
        void *p = data + used;
        used += n;
        memset(p, 0, n);
        return p;
    }

    template <class T> T *array(int n) {
        if (n < 0 || n > 1000000)
            fatal("Invalid resource count.");
        return (T *)alloc(sizeof(T) * n);
    }
};

Arena permanent, levelArena;

// Bound file reads by their declared size and allocate storage from the caller-owned arena.
u8 *readFile(const char *path, u32 *size, Arena &a, bool required = true) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) {
        if (required)
            fatal(path);
        *size = 0;
        return 0;
    }
    *size = GetFileSize(f, 0);
    if (*size > 128 * 1024 * 1024) {
        CloseHandle(f);
        fatal("Resource file too large.");
    }
    u8 *b = (u8 *)a.alloc(*size + 1);
    DWORD done = 0;
    BOOL ok = ReadFile(f, b, *size, &done, 0);
    CloseHandle(f);
    if (!ok || done != *size)
        fatal("Cannot read resource file.");
    return b;
}

// Write to a sibling temporary file first so an interrupted save cannot leave a partially written target.
bool atomicWrite(const char *path, const void *data, u32 size) {
    char tmp[MAX_PATH];
    lstrcpyA(tmp, path);
    lstrcatA(tmp, ".tmp");
    HANDLE f = CreateFileA(tmp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    DWORD done = 0;
    bool ok = WriteFile(f, data, size, &done, 0) && done == size;
    if (ok)
        ok = FlushFileBuffers(f) != 0;
    CloseHandle(f);
    if (ok)
        ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok)
        DeleteFileA(tmp);
    return ok;
}

struct Pair {
    char *key;
    char *value;
};

struct Props {
    int count;
    Pair *pairs;

    const char *get(const char *key, const char *fallback = "") const {
        for (int i = 0; i < count; i++)
            if (eq(pairs[i].key, key))
                return pairs[i].value;
        return fallback;
    }

    float num(const char *key, float fallback = 0) const {
        return number(get(key), fallback);
    }

    bool flag(const char *key, bool fallback = false) const {
        return num(key, fallback ? 1.0f : 0.0f) != 0;
    }
};

struct Reader {
    u8 *b;
    u32 size, pos;
    Arena *arena;

    Reader(u8 *bytes, u32 n, Arena &a) : b(bytes), size(n), pos(0), arena(&a) {}

    void *take(u32 n) {
        if (n > size - pos)
            fatal("Damaged game resource.");
        void *p = b + pos;
        pos += n;
        return p;
    }

    int i() {
        return *(int *)take(4);
    }

    u64 q() {
        return *(u64 *)take(8);
    }

    float f() {
        return *(float *)take(4);
    }

    char *s() {
        int n = i();
        if (n < 0 || n > 1000000)
            fatal("Invalid resource string.");
        char *p = (char *)arena->alloc(n + 1);
        memcpy(p, take(n), n);
        return p;
    }

    Props props() {
        Props p;
        p.count = i();
        p.pairs = arena->array<Pair>(p.count);
        for (int j = 0; j < p.count; j++) {
            p.pairs[j].key = s();
            p.pairs[j].value = s();
        }
        return p;
    }
};

struct Vec {
    float x, y;
};

Vec v(float x, float y) {
    Vec a = {x, y};
    return a;
}

struct Rect {
    float l, b, r, t;
};

bool overlaps(Rect a, Rect b) {
    return a.l < b.r && a.r > b.l && a.b < b.t && a.t > b.b;
}

Rect shifted(Rect a, float x, float y) {
    a.l += x;
    a.r += x;
    a.b += y;
    a.t += y;
    return a;
}

Vec center(Rect a) {
    return v((a.l + a.r) * 0.5f, (a.b + a.t) * 0.5f);
}

#include "viewport_native.h"
