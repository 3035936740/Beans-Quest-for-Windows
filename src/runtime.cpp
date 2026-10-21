// Minimal x86 compiler and constructor helpers required by the CRT-free executable.
// Small, self-contained x86 compiler helpers. No CRT startup or CRT DLL required.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef void(__cdecl *Initializer)();
#pragma section(".CRT$XCA", read)
#pragma section(".CRT$XCZ", read)
extern "C" __declspec(allocate(".CRT$XCA")) Initializer constructorsBegin[] = {0};
extern "C" __declspec(allocate(".CRT$XCZ")) Initializer constructorsEnd[] = {0};

extern "C" void runConstructors() {
    for (Initializer *p = constructorsBegin + 1; p < constructorsEnd; p++)
        if (*p)
            (*p)();
}

extern "C" int __cdecl atexit(void(__cdecl *)()) {
    return 0;
}

void *operator new(unsigned int n) {
    void *p = HeapAlloc(GetProcessHeap(), 0, n);
    if (!p)
        ExitProcess(1);
    return p;
}

void operator delete(void *p) noexcept {
    if (p)
        HeapFree(GetProcessHeap(), 0, p);
}

void operator delete(void *p, unsigned int) noexcept {
    if (p)
        HeapFree(GetProcessHeap(), 0, p);
}

extern "C" int __cdecl _purecall() {
    ExitProcess(1);
}

extern "C" float bqSqrt(float x) {
    float r;
    __asm {
    fld x
    fsqrt
    fstp r
    }
    return r;
}

extern "C" float bqSin(float x) {
    float r;
    __asm {
    fld x
    fsin
    fstp r
    }
    return r;
}

extern "C" float bqCos(float x) {
    float r;
    __asm {
    fld x
    fcos
    fstp r
    }
    return r;
}

extern "C" float bqAtan2(float y, float x) {
    float r;
    __asm {
    fld y
    fld x
    fpatan
    fstp r
    }
    return r;
}

extern "C" float bqFloor(float x) {
    int r;
    __asm {
    fld x
    fistp r
    }
    if (r > x)-- r;
    return (float)r;
}

extern "C" float bqExp(float x) {
    int n = 0;
    while (x > 0.5f || x < -0.5f) {
        x *= 0.5f;
        n++;
    }
    float term = 1, result = 1;
    for (int i = 1; i <= 12; i++) {
        term *= x / i;
        result += term;
    }
    while (n--)
        result *= result;
    return result;
}

// x87 implementation keeps the original completion easing's fractional
// exponent without adding a CRT dependency to the XP executable.
extern "C" float bqPow(float x, float y) {
    if (x <= 0)
        return y == 0 ? 1.0f : 0.0f;
    float result;
    __asm {
    fld y
    fld x
    fyl2x
    fld st(0)
    frndint
    fxch st(1)
    fsub st(0),st(1)
    f2xm1
    fld1
    faddp st(1),st(0)
    fscale
    fstp st(1)
    fstp result
    }
    return result;
}

extern "C" void *__cdecl memset(void *dst, int c, unsigned int n) {
    unsigned char *p = (unsigned char *)dst;
    while (n--)
        *p++ = (unsigned char)c;
    return dst;
}

extern "C" void *__cdecl memcpy(void *dst, const void *src, unsigned int n) {
    unsigned char *p = (unsigned char *)dst;
    const unsigned char *q = (const unsigned char *)src;
    while (n--)
        *p++ = *q++;
    return dst;
}

extern "C" __declspec(naked) void _allshl() {
    __asm {
    cmp cl,64
    jae zero
    cmp cl,32
    jae upper
    shld edx,eax,cl
    shl eax,cl
    ret
upper:
    mov edx,eax
    xor eax,eax
    and cl,31
    shl edx,cl
    ret
zero:
    xor eax,eax
    xor edx,edx
    ret
    }
}

extern "C" __declspec(naked) void _aullshr() {
    __asm {
    cmp cl,64
    jae zero
    cmp cl,32
    jae lower
    shrd eax,edx,cl
    shr edx,cl
    ret
lower:
    mov eax,edx
    xor edx,edx
    and cl,31
    shr eax,cl
    ret
zero:
    xor eax,eax
    xor edx,edx
    ret
    }
}

extern "C" __declspec(naked) void _ftol2_sse() {
    __asm {
    sub esp,16
    fnstcw word ptr [esp]
    mov ax,word ptr [esp]
    or ax,0c00h
    mov word ptr [esp+2],ax
    fldcw word ptr [esp+2]
    fistp qword ptr [esp+8]
    fldcw word ptr [esp]
    mov eax,dword ptr [esp+8]
    mov edx,dword ptr [esp+12]
    add esp,16
    ret
    }
}
