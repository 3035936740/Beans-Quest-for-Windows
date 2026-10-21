#pragma once

// State records for wizard motion; live physics ownership stays with the calling scene.

// Plain 32-bit values only: the save format owns serialization and migration.
// Offsets in the comments refer to FinalBossBehaviour in the original ARM ELF.
struct WizardMotionState {
    float age;          // +0x11c, never reset at an arena transition
    float baseX, baseY; // +0x138/+0x13c, before the sine displacement
    float periodX, periodY, amplitudeX, amplitudeY, offsetX, offsetY;
    float cycleAge; // +0x164, independent from body/animation age
    float initialBeanX;
    float leftAge, rightAge;
    float prespawnAge;
    int initialized, shooting, cycleCount, pendingCount, pending[3];
    // Native uses the level's MT19937. These legacy checkpoint fields retain
    // binary compatibility; retry must keep the live engine rather than rewind it.
    unsigned int random[624];
    int randomIndex;
};

WizardMotionState wizardMotion;
// Kept outside the saved motion block, matching the native level-RNG lifetime.
SourceRandomState wizardRetryRandom;
bool wizardRetryRandomValid;

void rememberWizardRetryRandom() {
    wizardRetryRandomValid = wizardMotion.initialized != 0;
    if (wizardRetryRandomValid) {
        memcpy(wizardRetryRandom.values, wizardMotion.random, sizeof(wizardMotion.random));
        wizardRetryRandom.index = wizardMotion.randomIndex;
    }
}

void retainWizardRetryRandom() {
    if (wizardRetryRandomValid) {
        memcpy(wizardMotion.random, wizardRetryRandom.values, sizeof(wizardMotion.random));
        wizardMotion.randomIndex = wizardRetryRandom.index;
    }
}

// FinalBoss +0x154 and +0x14c/+0x150. Stored separately so legacy
// WizardMotionState has the same binary layout in checkpoints before v11.
struct WizardCameraBlendState {
    float remaining, fromX, fromY;
};

WizardCameraBlendState wizardCameraBlend;
