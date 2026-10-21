#pragma once

// Regression coverage for wizard pose. Runs only in the isolated BQ_ENABLE_TESTS executable.

void wizardPoseChecks() {
    Vec body = {400, 200};
    WizardArmMotion motion = wizardArmInitial();
    WizardArmPose left = wizardArmPose(body, true, 3, motion),
                  right = wizardArmPose(body, false, 3, motion);
    check(left.shoulder.x == 429 && left.shoulder.y == 182 && left.hand.x == 395 &&
              left.hand.y == 186,
          "Wizard left shoulder and hand use the four authored pixel offsets");
    check(right.shoulder.x == 379 && right.shoulder.y == 188 && right.hand.x == 345 &&
              right.hand.y == 192,
          "Wizard right arm uses its asymmetric original shoulder and hand offsets");
    check(left.sleeve.x == 412 && left.sleeve.y == 184 && right.sleeve.x == 362 &&
              right.sleeve.y == 190,
          "Wizard sleeves lie exactly midway between the body anchors and hands");
    check(left.sleeveZ == 2 && left.handZ == 4 && right.sleeveZ == -4 && right.handZ == -2,
          "Wizard back arm, body and front arm retain the native z ordering");
    motion.clock = 0.25f;
    WizardArmPose bob = wizardArmPose(body, true, 0, motion);
    check(
        absf(bob.hand.x - 395.309017f) < 0.001f && absf(bob.hand.y - 189) < 0.001f,
        "Wizard normal hand bob uses the native ten-second horizontal and two-second vertical periods");
    motion.clock = 0;
    WizardArmPose alternate = wizardArmPose(body, true, 2, motion);
    check(absf(alternate.hand.x - (395 + 2.1213203f)) < 0.001f &&
              absf(alternate.hand.y - 191) < 0.001f,
          "Wizard alternate hand pose uses its separate three- and five-pixel waves");
    motion.clock = 0;
    wizardArmShot(motion, Vec{-1, 0});
    WizardArmPose kicked = wizardArmPose(body, true, 1, motion);
    motion.recoilAge = 0.25f;
    WizardArmPose recovering = wizardArmPose(body, true, 1, motion);
    motion.recoilAge = 0.5f;
    WizardArmPose rested = wizardArmPose(body, true, 1, motion);
    check(
        absf(kicked.hand.x - rested.hand.x - 10) < 0.001f &&
            absf(recovering.hand.x - rested.hand.x - 2.928932f) < 0.001f,
        "Wizard shot recoil moves ten pixels opposite the shot and eases back over half a second");
    Vec aim = wizardHandAim(Vec{0, 0}, Vec{30, 64});
    check(absf(aim.x - 0.6f) < 0.0001f && absf(aim.y - 0.8f) < 0.0001f,
          "Wizard hand aims at the native point 24 pixels below Bean's centre");
    motion = wizardArmInitial();
    wizardArmTick(motion, 0.2f, 0);
    bool idleAge = motion.recoilAge == 0.5f;
    wizardArmTick(motion, 0.2f, 1);
    check(idleAge && absf(motion.clock - 0.4f) < 0.0001f && absf(motion.recoilAge - 0.7f) < 0.0001f,
          "Wizard arm clocks advance continuously while recoil age advances only in firing mode");
}
