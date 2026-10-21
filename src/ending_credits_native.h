#pragma once

// Original ending-credit scene sequencing.

// CutsceneEndingController::OnUpdate 0x180200: an accept first pulses the
// continue prompt. It never skips the camera; a later accept requests exit.
struct EndingCreditsNativeState {
    float startY, scrollSeconds, promptAge, exitAge;
    bool prompt, exiting, autoSleepAllowed;
    u64 next;
};

EndingCreditsNativeState endingCredits;
float endingCreditsFadeDuration = 2;

void resetEndingCredits(float startY, u64 next) {
    endingCredits = EndingCreditsNativeState{};
    endingCredits.startY = startY;
    endingCredits.scrollSeconds = maxf(0, (startY - 192) / 30);
    endingCredits.next = next;
}

float endingCreditsCameraY(float age) {
    return maxf(192, endingCredits.startY - 30 * maxf(0, age));
}

bool acceptEndingCredits(float sceneAge) {
    if (endingCredits.exiting)
        return false;
    if (endingCredits.prompt) {
        if (endingCredits.promptAge > 0.5f && endingCredits.next) {
            endingCredits.exiting = true;
            endingCredits.exitAge = 0;
            return true;
        }
        return false;
    }
    if (sceneAge > 1) {
        endingCredits.prompt = true;
        endingCredits.promptAge = 0;
    }
    return false;
}

bool advanceEndingCredits(float sceneAge, float dt) {
    if (endingCredits.exiting) {
        endingCredits.exitAge += dt;
        return endingCredits.exitAge >= 2;
    }
    if (sceneAge >= endingCredits.scrollSeconds + 10)
        endingCredits.prompt = true;
    if (endingCredits.prompt) {
        endingCredits.promptAge += dt;
        if (endingCredits.promptAge >= 4 && sceneAge < endingCredits.scrollSeconds + 10) {
            endingCredits.prompt = false;
            endingCredits.promptAge = 0;
        }
    }
    if (sceneAge >= endingCredits.scrollSeconds + 20)
        endingCredits.autoSleepAllowed = true;
    return false;
}

float endingCreditsPromptAlpha() {
    return endingCredits.prompt
               ? clamp(1 - bqCos(endingCredits.promptAge * 3.1415927410125732f), 0, 1)
               : 0;
}
