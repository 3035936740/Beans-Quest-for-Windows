#pragma once

// Stage-clear presentation, scoring and completion award calculations.

// LevelCompleteController::OnUpdate, ARM 0x247aa0. Coordinates are the
// original bottom-up scene coordinates; age starts when Show() is called.
struct StageClearNativeRect {
    float l, b, r, t;
};

struct StageClearNativePose {
    StageClearNativeRect stageClearRect, beanRect;
    float remainingUiOffsetY;
    bool focusReady;
};

// Native 0x2486fc..24878c and 0x248820..2488a8. This is a cubic-phase
// oscillation, not the conventional easeOutElastic or a monotonic tween.
inline float stageClearNativeElastic(float p) {
    if (p <= 0)
        return 0;
    if (p >= 1)
        return 1;
    const float pi = 3.14159265358979323846f;
    float cubic = p * 2.5f;
    cubic = p * cubic;
    float phase = 0.2f + p * cubic;
    phase = (p * pi) * phase;
    float tail = 1 - p;
    float envelope = 1 + tail * 1.2f;
    return (p + bqSin(phase) * bqPow(tail, 2.2f)) * envelope;
}

inline float stageClearNativeSine(float p) {
    if (p <= 0)
        return 0;
    if (p >= 1)
        return 1;
    float phase = (p * 3.14159265358979323846f) * 0.5f;
    return bqSin(phase);
}

inline StageClearNativeRect stageClearNativeMove(StageClearNativeRect r, float x, float y) {
    float dx = x - (r.l + r.r) * 0.5f, dy = y - (r.b + r.t) * 0.5f;
    return StageClearNativeRect{r.l + dx, r.b + dy, r.r + dx, r.t + dy};
}

inline StageClearNativePose stageClearNativePose(float age, StageClearNativeRect screen,
                                                 StageClearNativeRect banner,
                                                 StageClearNativeRect bean) {
    float width = screen.r - screen.l, height = screen.t - screen.b;
    float cx = (screen.l + screen.r) * 0.5f, cy = (screen.b + screen.t) * 0.5f;
    float p = stageClearNativeElastic(age * 4);
    float x = (cx - width) * (1 - p) + cx * p;
    float bannerY = (banner.b + banner.t) * 0.5f;
    float rise = stageClearNativeSine((age - 1) * 2);
    float y = (float)((1 - (double)rise) * cy + (double)rise * bannerY);
    float bx = (bean.l + bean.r) * 0.5f, by = (bean.b + bean.t) * 0.5f;
    float beanP = stageClearNativeElastic((age - 1.4f) / 0.6f);
    StageClearNativePose result = {
        stageClearNativeMove(banner, x, y),
        stageClearNativeMove(bean, bx, by * beanP + (by - height) * (1 - beanP)),
        (float)((1 - (double)rise) * (-2 * height)), age >= 2};
    return result;
}

inline StageClearNativePose stageClearNativePose(float age) {
    return stageClearNativePose(age, StageClearNativeRect{0, 0, 512, 384},
                                StageClearNativeRect{115, 293, 435, 337},
                                StageClearNativeRect{30, 196, 182, 324});
}
