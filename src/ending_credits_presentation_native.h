#pragma once

// Ending-credit presentation, scrolling and transition timing.
#pragma float_control(precise, on, push)

// CutsceneEndingController::OnUpdate0x180200; input never gates this timeline.
// Coordinates are authored actor centres, not a shared sprite baseline.
struct EndingCreditsPresentationInput {
    float startY, emilioX, emilioY, mariaX, mariaY;
};

struct EndingCreditsPresentation {
    float tintR, tintG, tintB, tintA, backdropR, backdropG, backdropB, backdropA;
    float emilioX, emilioY, mariaX, mariaY, mariaScaleX, nameAlpha, endAlpha;
    const char *characterAnimation;
    float walkingAge, defaultAge;
};

float endingCreditsPresentationClamp(float n) {
    return n < 0 ? 0 : n > 1 ? 1 : n;
}

__declspec(noinline) EndingCreditsPresentation
    endingCreditsPresentation(const EndingCreditsPresentationInput &in, float age) {
    float t = age < 0 ? 0 : age, scroll = (in.startY - 192) / 30;
    float q = endingCreditsPresentationClamp((t - 6) / 6),
          move = endingCreditsPresentationClamp((t - scroll - 2) / 3),
          mid = (in.emilioX + in.mariaX) * 0.5f;
    EndingCreditsPresentation p = {};
    p.tintR = p.tintG = p.tintB = 1 - q;
    p.tintA = 1;
    p.backdropR = p.backdropG = p.backdropB = 1;
    p.backdropA = q;
    p.emilioX = in.emilioX + (mid - 10 - in.emilioX) * move;
    p.emilioY = in.emilioY;
    p.mariaX = in.mariaX + (mid + 10 - in.mariaX) * move;
    p.mariaY = in.mariaY;
    p.mariaScaleX = -1;
    p.nameAlpha = 1 - endingCreditsPresentationClamp(t - scroll - 2);
    p.endAlpha = endingCreditsPresentationClamp((t - scroll - 7) * 0.5f);
    p.characterAnimation = t > scroll + 2 && t <= scroll + 5 ? "walking" : "";
    p.walkingAge = move * 3;
    p.defaultAge = t - p.walkingAge;
    return p;
}

#pragma float_control(pop)
