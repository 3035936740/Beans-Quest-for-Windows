#pragma once

// Tracks respawn fade timing separately from the restored entity and physics snapshot.

// Recovered from libBeansQuest.so, not a replacement for save-state APIs.
// TomPlayerBehaviour::PrePhysicsStep @0x3115d8 waits across 2.0 seconds;
// HUDController::Restart @0x22230c calls LoadLastSaveState(true,true).
// LevelTask::LoadLastSaveState @0x269748 / LoadResetState @0x2699f0:
// black alpha 0 -> 1, callback, black alpha 1 -> 0; both use 0x3eb33333.
// FadeTask::Fading_Tick @0x1cbe0c interpolates linearly.
enum RespawnRequest { RESPAWN_NONE, RESPAWN_DEATH, RESPAWN_CHECKPOINT, RESPAWN_RESTART };

enum RespawnPhase {
    RESPAWN_IDLE,
    RESPAWN_DEATH_WAIT,
    RESPAWN_FADE_OUT,
    RESPAWN_BLACK,
    RESPAWN_FADE_IN
};

struct RespawnTransition {
    RespawnPhase phase;
    RespawnRequest request;
    float age;
    bool swapIssued;
};

const float respawnNativeFadeSeconds = 0.349999994f;
const float respawnNativeDeathWaitSeconds = 2.0f;
const float respawnNativeDeathSoundSeconds = 0.366659999f;

inline void respawnCancel(RespawnTransition &s) {
    s.phase = RESPAWN_IDLE;
    s.request = RESPAWN_NONE;
    s.age = 0;
    s.swapIssued = false;
}

inline bool respawnBusy(const RespawnTransition &s) {
    return s.phase != RESPAWN_IDLE;
}

inline bool respawnBegin(RespawnTransition &s, RespawnRequest request) {
    if (request == RESPAWN_NONE)
        return false;
    if (respawnBusy(s)) {
        // A deliberate checkpoint/restart skips the death wait. Retain the
        // current opacity if the death fade has already started; do not let
        // repeated manual requests restart an existing manual transition.
        if (s.request != RESPAWN_DEATH ||
            (request != RESPAWN_CHECKPOINT && request != RESPAWN_RESTART))
            return false;
        if (s.phase == RESPAWN_DEATH_WAIT)
            s.age = 0;
        else if (s.phase == RESPAWN_FADE_IN)
            s.age = respawnNativeFadeSeconds - s.age;
        else if (s.phase == RESPAWN_BLACK)
            s.age = respawnNativeFadeSeconds;
        s.age = s.age < 0 ? 0 : s.age > respawnNativeFadeSeconds ? respawnNativeFadeSeconds : s.age;
        s.request = request;
        s.phase = RESPAWN_FADE_OUT;
        s.swapIssued = false;
        return true;
    }
    s.request = request;
    s.phase = request == RESPAWN_DEATH ? RESPAWN_DEATH_WAIT : RESPAWN_FADE_OUT;
    s.age = 0;
    s.swapIssued = false;
    return true;
}

inline float respawnAlpha(const RespawnTransition &s) {
    float p = s.age / respawnNativeFadeSeconds;
    if (p < 0)
        p = 0;
    if (p > 1)
        p = 1;
    return s.phase == RESPAWN_BLACK      ? 1
           : s.phase == RESPAWN_FADE_OUT ? p
           : s.phase == RESPAWN_FADE_IN  ? 1 - p
                                         : 0;
}

// Call once per visual frame, including while PLAY physics are suspended or a
// pause menu is visible. Returned action is emitted once, ONLY under opaque
// black. Deliberately do not carry a slow frame's excess dt across the swap:
// the original callback and LevelTask pending load run in separate task ticks.
inline RespawnRequest respawnAdvance(RespawnTransition &s, float dt) {
    if (!(dt > 0))
        return RESPAWN_NONE;
    if (s.phase == RESPAWN_BLACK || s.phase == RESPAWN_IDLE)
        return RESPAWN_NONE;
    s.age += dt;
    if (s.phase == RESPAWN_DEATH_WAIT) {
        // Original checks old <= 2 && new > 2, not early at exactly 2.
        if (s.age > respawnNativeDeathWaitSeconds) {
            s.phase = RESPAWN_FADE_OUT;
            s.age = 0;
        }
    } else if (s.phase == RESPAWN_FADE_OUT && s.age >= respawnNativeFadeSeconds) {
        s.phase = RESPAWN_BLACK;
        s.age = 0;
        s.swapIssued = true;
        return s.request == RESPAWN_RESTART ? RESPAWN_RESTART : RESPAWN_CHECKPOINT;
    } else if (s.phase == RESPAWN_FADE_IN && s.age >= respawnNativeFadeSeconds)
        respawnCancel(s);
    return RESPAWN_NONE;
}

// Acknowledge only AFTER the immediate save/reset API has restored the scene
// and camera. The first frame of the restored scene is still completely black.
inline bool respawnRestored(RespawnTransition &s) {
    if (s.phase != RESPAWN_BLACK || !s.swapIssued)
        return false;
    s.phase = RESPAWN_FADE_IN;
    s.age = 0;
    s.swapIssued = false;
    return true;
}
