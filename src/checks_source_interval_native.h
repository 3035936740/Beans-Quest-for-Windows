#pragma once

// Regression coverage for source interval native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void sourceIntervalNativeChecks() {
    SourceRandomState *saved = new SourceRandomState;
    *saved = sourceRandom;
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scene = {};
    int stage = stageById(7138745870511843ULL), sources = 0;
    bool fixed = true, onePointTwo = false;
    seedSourceRandom(5489);
    if (stage >= 0) {
        readLevelData(levels[stageList[stage]].file, scene, memory);
        for (int pass = 0; pass < 8; pass++)
            for (int i = 0; i < scene.objectCount; i++) {
                const Props *p = scene.objects[i].behavior("BulletSource");
                if (!p)
                    continue;
                if (!pass)
                    sources++;
                volatile float expected = p->num("timeMin", 3);
                onePointTwo |= absf(expected - 1.2f) < 0.000001f;
                float actual = sourceInterval(*p);
                fixed &= actual == expected;
            }
    } else
        fixed = false;
    check(
        sources == 4 && onePointTwo && fixed && sourceRandom.index == 624,
        "The four original Inner Chambers emitters keep all32 fixed interval samples including1.2seconds without consuming any MT words");
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    Pair equalRoundedPairs[] = {{(char *)"timeMin", (char *)"1.20000001"},
                                {(char *)"timeMax", (char *)"1.20000002"}};
    Props equalRounded = {2, equalRoundedPairs};
    volatile float roundedLow = equalRounded.num("timeMin"),
                   roundedHigh = equalRounded.num("timeMax");
    seedSourceRandom(5489);
    float roundedValue = sourceInterval(equalRounded);
    check(
        roundedLow == roundedHigh && roundedValue == roundedLow && sourceRandom.index == 624,
        "Different decimal bounds rounding to the same native binary32 interval remain a constant distribution without random consumption");
    Pair randomPairs[] = {{(char *)"timeMin", (char *)"0.25"}, {(char *)"timeMax", (char *)"2.25"}};
    Props randomProps = {2, randomPairs};
    const u32 words[] = {3499211612u, 581869302u, 3890346734u, 3586334585u};
    bool sequence = true, range = true;
    seedSourceRandom(5489);
    for (int i = 0; i < 4; i++) {
        volatile float unit = (float)words[i] * 2.3283064365386962890625e-10f;
        volatile float expected = 0.25f + unit * 2;
        float actual = sourceInterval(randomProps);
        sequence &= absf(actual - expected) < 0.000001f;
        range &= actual >= 0.25f && actual < 2.25f;
    }
    check(
        sequence && range && sourceRandom.index == 4,
        "True variable intervals consume one MT word per sample and match the independent first four MT19937 seed5489 vectors within their authored range");
    char trace[160];
    wsprintfA(
        trace,
        "fixed_source_count=%d fixed_calls=32 native_equal_bound_draws=0\r\nvariable_vector_calls=4 variable_index=%d sequence=%d range=%d\r\n",
        sources, sourceRandom.index, sequence, range);
    atomicWrite("test-output/source-interval-native.txt", trace, lstrlenA(trace));
    // The current scene and live source RNG belong to the caller. This check
    // uses a separate parsed resource scene and must not reset either one.
    sourceRandom = *saved;
    delete saved;
}
