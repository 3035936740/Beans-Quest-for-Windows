#pragma once

// Wizard encounter presentation and its authored state transitions.
int wizardRandomInt(int low, int high);

void wizardTroll(int choice = -1) {
    // Troll 0x1d9f58 chooses an animation before testing whether the head is
    // idle. The separate sound cooldown is strictly greater than 1.5 seconds.
    if (choice < 0)
        choice = wizardRandomInt(0, 2);
    if (wizardPresentation.animation < 0) {
        wizardPresentation.animation = choice;
        wizardPresentation.animationAge = 0;
    }
    if (wizardPresentation.cooldown > 1.5f) {
        wizardPresentation.cooldown = 0;
        sound(wizardRandomInt(0, 1) ? "malmo-laugh2" : "malmo-laugh1");
    }
}

void wizardPresentationTick(float dt) {
    WizardPresentationState &h = wizardPresentation;
    h.cooldown += dt;
    h.animationAge += dt;
    if (h.animation < 0)
        return;
    float length =
        h.animation == 3
            ? 1.0f
            : animationLength(animation(actor(106556998914888ULL), wizardHeadAnimation()));
    if (h.animationAge >= length) {
        h.animation = -1;
        h.animationAge = 0;
    }
}
