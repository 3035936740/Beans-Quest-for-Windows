#pragma once

// Regression coverage for spider animation. Runs only in the isolated BQ_ENABLE_TESTS executable.

void spiderAnimationChecks() {
    Actor *spider = actor(8234400683372717661ULL);
    Animation *idle = exactEnemyAnimation(spider, "idle");
    check(
        spider && spider->count == 2 && idle && idle->count == 4 &&
            absf(animationLength(idle) - 0.5f) < 0.0001f,
        "Original spider resource contains a four-frame eight-fps idle cycle beside its unrelated unnamed preview frame");
    bool idleSelected = true;
    const float directions[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    for (int i = 0; i < 4; i++)
        for (int reverse = 0; reverse < 2; reverse++)
            idleSelected &= eq(oscillatorAnimationName(spider, directions[i][0], directions[i][1],
                                                       reverse != 0, "idle"),
                               "idle");
    check(
        idleSelected && !exactEnemyAnimation(spider, "left") &&
            !exactEnemyAnimation(spider, "right") && !exactEnemyAnimation(spider, "up") &&
            !exactEnemyAnimation(spider, "down"),
        "Spider rails preserve authored idle instead of selecting the unnamed preview through a false directional-animation lookup");
    check(
        idle && animationFrame(idle, 0, ANIMATION_LOOP) == 0 &&
            animationFrame(idle, 0.13f, ANIMATION_LOOP) == 1 &&
            animationFrame(idle, 0.38f, ANIMATION_LOOP) == 3,
        "Spider idle visibly advances all original animation frames instead of staying frozen on the atlas preview");
}
