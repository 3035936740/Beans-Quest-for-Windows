#pragma once

// Regression coverage for spike eye animation. Runs only in the isolated BQ_ENABLE_TESTS executable.

void spikeEyeAnimationChecks() {
    Actor *eye = actor(4615236400452776625ULL);
    Animation *idle = exactEnemyAnimation(eye, "idle");
    Animation *preview = exactEnemyAnimation(eye, "");
    check(
        eye && eye->count == 2 && preview && preview->duration == 0 && preview->count == 1 &&
            idle && idle->duration == 10 && idle->count == 4 &&
            absf(animationLength(idle) - 0.4f) < 0.0001f,
        "Original red rail spike eye has a four-frame ten-fps idle animation separate from its frozen unnamed preview");
    bool preserves = true;
    const float axes[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < 4; i++)
        for (int back = 0; back < 2; back++)
            preserves &=
                eq(oscillatorAnimationName(eye, axes[i][0], axes[i][1], back != 0, "idle"), "idle");
    check(
        preserves,
        "All original red spike-eye rail directions retain authored idle when no directional instances exist");
    check(
        idle && animationFrame(idle, 0.01f) == 0 && animationFrame(idle, 0.11f) == 1 &&
            animationFrame(idle, 0.21f) == 2 && animationFrame(idle, 0.31f) == 3 &&
            animationFrame(idle, 0.41f) == 0,
        "Red spike-eye cycles all four source frames and loops rather than displaying its static preview");
    int stage = motionStage(5, 2);
    if (stage >= 0) {
        startStage(stage);
        bool authored = false;
        for (int i = 0; i < entityCount; i++) {
            Entity &e = entities[i];
            if (e.source->klass == 4615236400452776625ULL && e.move) {
                authored = true;
                preserves &= eq(oscillatorAnimationName(e.art, e.moveX, e.moveY, false,
                                                        e.source->props.get("default-animation")),
                                "idle");
            }
        }
        check(
            authored && preserves,
            "Authored fifth-world tower spike eyes keep the source idle animation on their actual rails");
    } else
        check(false, "Authored fifth-world tower spike-eye stage exists");
}
