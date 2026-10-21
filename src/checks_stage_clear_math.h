#pragma once

// Regression coverage for stage clear math. Runs only in the isolated BQ_ENABLE_TESTS executable.

void stageClearMathChecks() {
    StageClearNativePose start = stageClearNativePose(0), slide = stageClearNativePose(0.25f);
    check(
        start.stageClearRect.l == -416 && start.stageClearRect.b == 170 &&
            start.remainingUiOffsetY == -768 && !start.focusReady,
        "Native STAGE CLEAR starts left of screen at Show age0 while its separate panel is below the viewport");
    check(slide.stageClearRect.l == 96 && slide.stageClearRect.b == 170 &&
              slide.remainingUiOffsetY == -768,
          "Native STAGE CLEAR finishes horizontal entrance in0.25s then remains centred until1s");
    StageClearNativePose mid = stageClearNativePose(1.25f), panel = stageClearNativePose(1.5f),
                         done = stageClearNativePose(2);
    check(absf(mid.remainingUiOffsetY - (-224.9419556f)) < 0.001f &&
              absf(mid.stageClearRect.b - 256.9741211f) < 0.001f,
          "Original completion panel and banner rise during1–1.5s using the native half-sine ease");
    check(
        panel.remainingUiOffsetY == 0 && panel.stageClearRect.b == 293 && !panel.focusReady,
        "Native completion frame reaches its authored position at1.5s before keyboard focus is assigned");
    check(
        done.beanRect.l == 30 && done.beanRect.b == 196 && done.stageClearRect.l == 96 &&
            done.focusReady,
        "Native portrait ends at its authored rectangle and initial Continue focus becomes available at2s");
    StageClearNativePose beforeBean = stageClearNativePose(1.4f), bean = stageClearNativePose(1.7f);
    check(
        beforeBean.beanRect.b == -188 && bean.beanRect.b > 196 && bean.beanRect.t > 324,
        "Large Bean portrait waits1.4s then visibly overshoots upward rather than sharing the panel tween");
    check(
        absf(stageClearNativeElastic(0.5f) - 1.051015794f) < 0.00001f &&
            stageClearNativeElastic(0) == 0 && stageClearNativeElastic(1) == 1,
        "Original completion oscillation retains its cubic phase,2.2 exponent and1.2 envelope including overshoot");
    StageClearNativePose large = stageClearNativePose(0, StageClearNativeRect{-64, -48, 576, 432},
                                                      StageClearNativeRect{115, 293, 435, 337},
                                                      StageClearNativeRect{30, 196, 182, 324});
    check(
        large.remainingUiOffsetY == -960 && large.stageClearRect.l == -544 &&
            large.beanRect.b == -284,
        "Completion animation derives travel distances from the actual camera AABB rather than fixed512x384 offsets");
}
