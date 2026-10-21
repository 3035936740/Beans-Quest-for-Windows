#pragma once

// Player transformation sequence after the final encounter.

// EndOfGameCutsceneController 0x1b2d38 / OnActivate 0x1b5db4.
// Origin is Maria's authored position; Emilio's x is replaced by Bean's
// current x in DoAlternate 0x1b8adc, while his authored y is retained.
struct EndingTransformationInput {
    float mariaX, mariaY, emilioY, beanX, cameraX;
};

struct EndingTransformationPose {
    float mariaX, mariaY, emilioX, emilioY, cameraX, mariaScaleX;
    const char *mariaAnimation;
    const char *emilioAnimation;
    float mariaIdleAdvance, mariaWalkAdvance, emilioIdleAdvance, emilioWalkAdvance;
};

float endingTransformationClamp(float value, float low, float high) {
    return value < low ? low : value > high ? high : value;
}

float endingTransformationInterval(float age, float begin, float end) {
    return endingTransformationClamp(age - begin, 0, end - begin);
}

float endingTransformationLerp(float a, float b, float factor) {
    return a + (b - a) * endingTransformationClamp(factor, 0, 1);
}

EndingTransformationPose endingTransformationPose(const EndingTransformationInput &in, float age) {
    float t = age < 0 ? 0 : age;
    EndingTransformationPose p = {};
    p.cameraX = endingTransformationLerp(in.cameraX, in.mariaX + 128, t);
    p.mariaX = endingTransformationLerp(in.mariaX, in.mariaX + 118, (t - 4) * 0.5f);
    p.mariaY = in.mariaY;
    p.emilioX = t <= 0.4f
                    ? endingTransformationLerp(in.beanX, in.mariaX + 256, t / 0.4f)
                    : endingTransformationLerp(in.mariaX + 256, in.mariaX + 138, (t - 4) * 0.5f);
    p.emilioY = in.emilioY;
    p.mariaScaleX = t <= 2 ? -1 : 1;
    // The controller changes retained instances; it never restarts walking.
    p.mariaAnimation = t > 4 && t <= 6 ? "walking" : "";
    p.emilioAnimation = (t > 0 && t <= 0.4f) || (t > 4 && t <= 6) ? "walking" : "";
    p.mariaWalkAdvance = endingTransformationInterval(t, 4, 6);
    p.mariaIdleAdvance = t - p.mariaWalkAdvance;
    p.emilioWalkAdvance =
        endingTransformationInterval(t, 0, 0.4f) + endingTransformationInterval(t, 4, 6);
    p.emilioIdleAdvance = t - p.emilioWalkAdvance;
    return p;
}

enum EndingTransformationEvent {
    ENDING_EMILIO_WALK = 1,
    ENDING_EMILIO_IDLE = 2,
    ENDING_MARIA_WALK = 4,
    ENDING_MARIA_IDLE = 8,
    ENDING_BOSS_ACHIEVEMENT = 16,
    ENDING_CREDITS = 32
};

unsigned int endingTransformationEvents(float oldAge, float newAge) {
    unsigned int events = 0;
    if ((oldAge <= 0 && newAge > 0) || (oldAge <= 4 && newAge > 4))
        events |= ENDING_EMILIO_WALK;
    if ((oldAge <= 0.4f && newAge > 0.4f) || (oldAge <= 6 && newAge > 6))
        events |= ENDING_EMILIO_IDLE;
    if (oldAge <= 4 && newAge > 4)
        events |= ENDING_MARIA_WALK;
    if ((oldAge <= 0 && newAge > 0) || (oldAge <= 6 && newAge > 6))
        events |= ENDING_MARIA_IDLE;
    if (oldAge < 7 && newAge >= 7)
        events |= ENDING_BOSS_ACHIEVEMENT;
    if (oldAge < 8 && newAge >= 8)
        events |= ENDING_CREDITS;
    return events;
}
