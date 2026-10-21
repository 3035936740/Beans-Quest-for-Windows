#pragma once

// Regression coverage for respawn transition. Runs only in the isolated BQ_ENABLE_TESTS executable.
#include "respawn_transition.h"

void respawnTransitionChecks() {
    RespawnTransition s = {};
    check(!respawnBusy(s) && respawnAlpha(s) == 0,
          "Respawn transition initially leaves the scene visible");
    check(!respawnBegin(s, RESPAWN_NONE), "Respawn rejects an empty request");
    check(respawnBegin(s, RESPAWN_CHECKPOINT), "Manual checkpoint starts native fade out");
    check(!respawnBegin(s, RESPAWN_RESTART),
          "Held keys cannot replace a pending checkpoint with restart");
    check(respawnAdvance(s, 0.175f) == RESPAWN_NONE && absf(respawnAlpha(s) - 0.5f) < 0.00001f,
          "Checkpoint fade is linear and does not restore halfway through");
    RespawnRequest action = respawnAdvance(s, 0.175f);
    check(action == RESPAWN_CHECKPOINT && s.phase == RESPAWN_BLACK && respawnAlpha(s) == 1,
          "Checkpoint restore is emitted only under fully opaque black at 0.35 seconds");
    check(respawnAdvance(s, 20) == RESPAWN_NONE && respawnAlpha(s) == 1,
          "Black waits for actual restore completion without repeated actions");
    check(respawnRestored(s) && respawnAlpha(s) == 1 && !respawnRestored(s),
          "Acknowledged restore starts from black exactly once");
    respawnAdvance(s, 0.175f);
    check(absf(respawnAlpha(s) - 0.5f) < 0.00001f && respawnBusy(s),
          "Restored checkpoint reveals through native 0.35-second fade");
    respawnAdvance(s, 0.175f);
    check(!respawnBusy(s) && respawnAlpha(s) == 0 && s.request == RESPAWN_NONE,
          "Completed checkpoint clears the input guard");
    respawnBegin(s, RESPAWN_DEATH);
    respawnAdvance(s, 1.4f);
    check(s.phase == RESPAWN_DEATH_WAIT && respawnAlpha(s) == 0,
          "Death particles are not replaced by the former premature 1.4-second restore");
    s.age = 2;
    check(respawnAdvance(s, 0) == RESPAWN_NONE && s.phase == RESPAWN_DEATH_WAIT,
          "Death recovery respects original strict two-second crossing");
    respawnAdvance(s, 0.001f);
    check(s.phase == RESPAWN_FADE_OUT && respawnAlpha(s) == 0,
          "Death wait finishes before black fade begins");
    action = respawnAdvance(s, 0.35f);
    check(action == RESPAWN_CHECKPOINT && respawnAlpha(s) == 1,
          "Death restores last checkpoint at full black rather than initial stage");
    respawnCancel(s);
    respawnBegin(s, RESPAWN_RESTART);
    check(!respawnRestored(s), "Restart cannot acknowledge restoration before black");
    action = respawnAdvance(s, 5);
    check(action == RESPAWN_RESTART && respawnAlpha(s) == 1,
          "Slow frame cannot skip restart's opaque swap boundary");
    respawnRestored(s);
    check(respawnAlpha(s) == 1, "Slow-frame restart still begins reveal at black");
    respawnAdvance(s, 0.35f);
    respawnBegin(s, RESPAWN_CHECKPOINT);
    respawnAdvance(s, -1);
    check(s.age == 0 && respawnAlpha(s) == 0, "Negative time does not rewind the transition");
    respawnCancel(s);
}
