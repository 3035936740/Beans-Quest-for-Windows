#pragma once

// Regression coverage for opening. Runs only in the isolated BQ_ENABLE_TESTS executable.
#include "opening.h"

// Numeric fixtures exercise recovered ARM ranges, not an invented scene.
void openingChecks() {
    OpeningState missing;
    Props empty = {};
    Level none = {};
    check(!openingConfigure(missing, empty, none) && missing.missingBindings == 511,
          "Opening refuses to fabricate missing native actor class bindings");
    check(openingAdvance(missing, 30) == 0 && missing.age == 0,
          "Unconfigured opening cannot silently run or report completion");
    bool classAbsent = true;
    for (int i = 0; i < classCount; i++)
        if (classes[i].has("CutsceneOpeningController"))
            classAbsent = false;
    check(classAbsent, "Supplied project contains no authored Opening controller class");
    OpeningState s = {};
    s.configured = true;
    s.actor[OPENING_WIZARD].initial = Vec{600, 500};
    s.actor[OPENING_EMILIO].position = Vec{250, 100};
    s.actor[OPENING_MARIA].initial = s.actor[OPENING_MARIA].position = Vec{350, 120};
    s.actor[OPENING_VORTEX].visible = true;
    openingSetShotStart(s, Vec{550, 300});
    openingAdvance(s, 1.5f);
    check(s.actor[OPENING_WIZARD].position.x == 600 && s.actor[OPENING_WIZARD].position.y == 404,
          "Opening native 0..3 second target descends 192 pixels linearly");
    openingAdvance(s, 1.5f);
    check(!(s.events & OPENING_SHOCK),
          "Opening strict boundary at three seconds does not fire its crossing early");
    u32 event = openingAdvance(s, 0.01f);
    check((event & OPENING_SHOCK) && eq(s.actor[OPENING_EMILIO].animation, "shocked") &&
              eq(s.actor[OPENING_MARIA].animation, "shocked"),
          "Opening crossing three seconds selects both authored shocked animations");
    s.age = 5;
    event = openingAdvance(s, 0.5f);
    check(s.actor[OPENING_FIREBALL].position.x == 400 &&
              s.actor[OPENING_FIREBALL].position.y == 200,
          "Opening shot interpolates actual hand coordinates to Emilio during seconds five to six");
    event = openingAdvance(s, 0.51f);
    check(
        (event & OPENING_HIT) && eq(s.actor[OPENING_EMILIO].animation, "hit") &&
            s.actor[OPENING_FIREBALL].position.x == -128,
        "Opening projectile impact selects hit pose and removes the shot at native offscreen position");
    s.age = 7;
    openingAdvance(s, 0.5f);
    check(s.actor[OPENING_BAT].position.x == 427 && s.actor[OPENING_BAT].position.y == 214,
          "Opening bat approaches Maria from wizard target minus 96 pixels");
    s.age = 8;
    openingAdvance(s, 0.5f);
    check(s.actor[OPENING_MARIA].position.x == s.actor[OPENING_BAT].position.x &&
              s.actor[OPENING_MARIA].position.y == s.actor[OPENING_BAT].position.y - 16,
          "Opening abductor carries Maria exactly sixteen pixels below its position");
    s.age = 13;
    openingAdvance(s, 0.5f);
    check(s.actor[OPENING_WIZARD].position.x == 807 && s.actor[OPENING_WIZARD].position.y == 308,
          "Opening wizard returns from plus 350 to plus 64 during seconds thirteen to fourteen");
    s.age = 14;
    openingAdvance(s, 0.5f);
    check(s.actor[OPENING_VORTEX].scale == 0.5f,
          "Opening vortex grows linearly during seconds fourteen to fifteen");
    s.age = 15.25f;
    openingAdvance(s, 0.25f);
    check(s.actor[OPENING_VORTEX].scale == 0.5f,
          "Opening vortex shrinks over its native half-second interval");
    openingAdvance(s, 0.26f);
    check(!s.actor[OPENING_VORTEX].visible, "Opening vortex becomes hidden after 15.75 seconds");
    s.age = 18.9f;
    event = openingAdvance(s, 0.2f);
    check((event & OPENING_BEAN_REVEAL) && s.actor[OPENING_BEAN].position.x == 250 &&
              s.actor[OPENING_BEAN].position.y == 100 && s.actor[OPENING_EMILIO].position.x == -128,
          "Opening reveal moves Bean to Emilio and removes the human actor at nineteen seconds");
    s.age = 21.9f;
    check((openingAdvance(s, 0.2f) & OPENING_CHANGE_LEVEL) != 0 && openingAdvance(s, 0.1f) == 0,
          "Opening emits its native twenty-two-second level transition once");
}
