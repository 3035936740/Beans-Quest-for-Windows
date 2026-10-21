#pragma once

// Opening presentation state and its transition into gameplay.
#include "assets.h"

// Pure, source-bound portion of CutsceneOpeningController @0x18a7ac /
// 0x18e13c. The supplied APK has no scene or class with this controller.
// This module deliberately does not invent a replacement scene or route.
// Axolotl-container orbit/particles and full Boss arm integration remain open.
enum OpeningRole {
    OPENING_BEAN,
    OPENING_EMILIO,
    OPENING_MARIA,
    OPENING_WIZARD,
    OPENING_BAT,
    OPENING_FIREBALL,
    OPENING_VORTEX,
    OPENING_AXOLOTL,
    OPENING_EXPLOSION,
    OPENING_ROLE_COUNT
};

const char *openingRoleNames[OPENING_ROLE_COUNT] = {
    "bean", "emilio", "maria", "wizard", "bat", "fireball", "vortex", "axolotl", "explosion"};

enum OpeningEvent {
    OPENING_SHOCK = 1,
    OPENING_SHOOT = 2,
    OPENING_HIT = 4,
    OPENING_MARIA_HIT = 8,
    OPENING_VORTEX_START = 16,
    OPENING_VORTEX_HIDE = 32,
    OPENING_BEAN_REVEAL = 64,
    OPENING_CHANGE_LEVEL = 128
};

struct OpeningActorState {
    u64 klass;
    Vec initial, position;
    float scale;
    const char *animation;
    bool visible;
};

struct OpeningState {
    OpeningActorState actor[OPENING_ROLE_COUNT];
    float age;
    u32 events;
    u64 next, music;
    Vec shotStart, batReturn;
    bool configured;
    u32 missingBindings;
};

float openingRemap(float time, float start, float end, float from, float to) {
    float t = clamp((time - start) / maxf(0.00001f, end - start), 0, 1);
    return from + (to - from) * t;
}

Vec openingLerp(Vec a, Vec b, float t) {
    t = clamp(t, 0, 1);
    return Vec{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

bool openingCrossed(float oldTime, float time, float boundary) {
    return oldTime <= boundary && time > boundary;
}

bool openingConfigure(OpeningState &s, const Props &config, const Level &scene) {
    memset(&s, 0, sizeof(s));
    s.next = idnum(config.get("next-level"));
    s.music = idnum(config.get("wizard-music"));
    for (int role = 0; role < OPENING_ROLE_COUNT; role++) {
        OpeningActorState &a = s.actor[role];
        a.klass = idnum(config.get(openingRoleNames[role]));
        Object *cls = 0;
        for (int i = 0; i < classCount; i++)
            if (classes[i].id == a.klass) {
                cls = classes + i;
                break;
            }
        if (!a.klass || !cls) {
            s.missingBindings |= 1u << role;
            continue;
        }
        a.initial = center(cls->rect);
        for (int i = 0; i < scene.objectCount; i++)
            if (scene.objects[i].klass == a.klass) {
                a.initial = center(scene.objects[i].rect);
                break;
            }
        a.position = a.initial;
        a.scale = 1;
        a.visible = true;
        a.animation = cls->props.get("default-animation");
    }
    s.actor[OPENING_EMILIO].animation = "playing-guitar";
    s.actor[OPENING_MARIA].animation = "clapping";
    s.actor[OPENING_VORTEX].initial = s.actor[OPENING_VORTEX].position = Vec{384, 192};
    s.configured = s.missingBindings == 0;
    return s.configured;
}

// Caller supplies the actual Boss CutsceneShoot() hand position at the shot
// event. The native code stores that returned vector at +0x6c, not a constant.
void openingSetShotStart(OpeningState &s, Vec position) {
    s.shotStart = position;
}

u32 openingAdvance(OpeningState &s, float dt) {
    if (!s.configured || dt <= 0)
        return 0;
    float old = s.age;
    s.age += dt;
    float t = s.age;
    u32 event = 0;
    const float boundaries[] = {3, 5, 6, 8, 13, 15.75f, 19, 22};
    const u32 bits[] = {OPENING_SHOCK,       OPENING_SHOOT,        OPENING_HIT,
                        OPENING_MARIA_HIT,   OPENING_VORTEX_START, OPENING_VORTEX_HIDE,
                        OPENING_BEAN_REVEAL, OPENING_CHANGE_LEVEL};
    for (int i = 0; i < 8; i++)
        if (openingCrossed(old, t, boundaries[i]) && !(s.events & bits[i]))
            event |= bits[i];
    s.events |= event;
    OpeningActorState &wizard = s.actor[OPENING_WIZARD];
    Vec lower = wizard.initial;
    lower.y -= 192;
    // Native @0x18faec: 0..3 seconds, wizard target descends by 192 pixels.
    if (t > 0 && t < 3)
        wizard.position = openingLerp(wizard.initial, lower, t / 3);
    if (event & OPENING_SHOCK) {
        s.actor[OPENING_EMILIO].animation = "shocked";
        s.actor[OPENING_MARIA].animation = "shocked";
    }
    // Native @0x18fdc8: 5..6 seconds, shot from actual hand to Emilio.
    if (t > 5 && t < 6)
        s.actor[OPENING_FIREBALL].position =
            openingLerp(s.shotStart, s.actor[OPENING_EMILIO].position, t - 5);
    if (event & OPENING_HIT) {
        s.actor[OPENING_FIREBALL].position = Vec{-128, -128};
        s.actor[OPENING_EMILIO].animation = "hit";
    }
    OpeningActorState &bat = s.actor[OPENING_BAT];
    OpeningActorState &maria = s.actor[OPENING_MARIA];
    Vec batStart = {lower.x - 96, lower.y};
    if (t > 7 && t < 8)
        bat.position = openingLerp(batStart, maria.initial, t - 7);
    if (event & OPENING_MARIA_HIT)
        maria.animation = "hit";
    if (t > 8 && t < 9) {
        bat.position = openingLerp(maria.initial, batStart, t - 8);
        maria.position = Vec{bat.position.x, bat.position.y - 16};
    }
    if (openingCrossed(old, t, 10))
        s.batReturn = bat.position;
    // Native @0x18f5a4: wizard and abductor depart in parallel during 10..11.
    if (t > 10 && t < 11) {
        wizard.position = Vec{openingRemap(t, 10, 11, lower.x, lower.x + 350), lower.y};
        bat.position = Vec{s.batReturn.x + 256 * (t - 10), s.batReturn.y};
        maria.position = Vec{bat.position.x, bat.position.y - 16};
    }
    if (t > 13 && t < 14)
        wizard.position = Vec{openingRemap(t, 13, 14, lower.x + 350, lower.x + 64), lower.y};
    OpeningActorState &vortex = s.actor[OPENING_VORTEX];
    if (event & OPENING_VORTEX_START) {
        vortex.scale = 0;
        vortex.position = Vec{384, 192};
    }
    if (t > 14 && t < 15)
        vortex.scale = t - 14;
    if (t > 15.25f && t < 15.75f)
        vortex.scale = 1 - 2 * (t - 15.25f);
    if (event & OPENING_VORTEX_HIDE)
        vortex.visible = false;
    if (t > 16 && t < 17)
        wizard.position = Vec{openingRemap(t, 16, 17, lower.x + 64, lower.x + 350), lower.y};
    if (event & OPENING_BEAN_REVEAL) {
        s.actor[OPENING_BEAN].position = s.actor[OPENING_EMILIO].position;
        s.actor[OPENING_EMILIO].position = Vec{-128, -128};
        s.actor[OPENING_BEAN].visible = true;
    }
    return event;
}

const Props *openingControllerProps(const Level &scene) {
    for (int i = 0; i < scene.objectCount; i++)
        if (scene.objects[i].has("CutsceneOpeningController"))
            return scene.objects[i].behavior("CutsceneOpeningController");
    return 0;
}
