#pragma once

// State records for wizard presentation; live physics ownership stays with the calling scene.
// FinalBossBehaviourHead fields +54/+58 and its current animation instance.
// Kept separate from the version-eight motion record for save compatibility.
struct WizardPresentationState {
    float cooldown, animationAge;
    int animation;
};

WizardPresentationState wizardPresentation;

void resetWizardPresentation() {
    wizardPresentation = WizardPresentationState{1.5f, 0, -1};
}

const char *wizardHeadAnimation() {
    switch (wizardPresentation.animation) {
    case 0:
        return "trolling1";
    case 1:
        return "trolling2";
    case 2:
        return "trolling3";
    case 3:
        return "hurt";
    default:
        return "";
    }
}

void wizardHeadHurt() {
    wizardPresentation.animation = 3;
    wizardPresentation.animationAge = 0;
}
