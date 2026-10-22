#pragma once

// Campaign entity state, authored object construction, progress, checkpoints and camera lifecycle.
#include "assets.h"
#include "audio.h"
#include "effects.h"
#include "stomp_native.h"
#include "wizard_arena_native.h"
#include "collectable_native.h"
#include "respawn_transition.h"
#include "ready_native.h"
#include "maker_state.h"
RespawnTransition respawnTransition;

enum Kind {
    DECOR,
    PLAYER,
    COIN,
    SALAMANDER,
    CHECKPOINT,
    EXIT,
    SPRING,
    SPIKE,
    ENEMY,
    BLOCK,
    PLATFORM,
    TRAMPOLINE,
    SWITCH,
    ALTERNATOR,
    FORCE,
    WATER,
    MAGNET,
    BOUNCER,
    TURRET,
    BOSS,
    POTION,
    SECRET,
    HELPER,
    PERCH
};

enum { MAX_ENTITIES = 2048, MAX_PROJECTILES = 128 };

struct State {
    float x, y, vx, vy, angle, timer, aux;
    int gone, active, hp;
};

#include "mechanism_event_state_native.h"
#include "ordinary_jump_state_native.h"
OrdinaryJumpStateNative ordinaryJumpStateNative;
bool ordinaryJumpLaunchedNative;
void recordCampaignOrdinaryJumpNative(Vec normal, Vec point, float boost, bool stomp);
#include "jumper_animation_state_native.h"
#include "fish_animation_state_native.h"
#include "helper_animation_state_native.h"

// Walk SaveState0x32586c persists these four independent behaviour fields.
struct WalkSavedNative {
    float speed;
    int reverseEdge, right, canWalk;
};

struct Entity {
    Object *source;
    Kind kind;
    Rect local, hit;
    int layer, z;
    Actor *art;
    const Props *move;
    const Props *physics;
    float originX, originY, speed, moveX, moveY, lo, hi, radius, period, phase, jumpPower,
        turnCooldown, lastLaunchSpeed;
    int mask, alternation, dynamic, walk, reverseEdge, orbit, rotate, breakable, crumble, touch,
        deadly, spiky, parent, motion, circle, bossSpawn;
    State state;
    JumperAnimationNative jumperAnimation;
    FishAnimationNative fishAnimation;
    HelperAnimationNative helperAnimation;
    int walkRight, walkCanWalk;
};

#include "coin_animation_native.h"

WalkSavedNative captureWalkNative(const Entity &e) {
    return e.walk ? WalkSavedNative{e.speed, e.reverseEdge, e.walkRight, e.walkCanWalk}
                  : WalkSavedNative{};
}

void restoreWalkNative(Entity &e, const WalkSavedNative &s) {
    if (e.walk) {
        e.speed = s.speed;
        e.reverseEdge = s.reverseEdge;
        e.walkRight = s.right;
        e.walkCanWalk = s.canWalk;
        e.state.aux = e.walkRight ? e.speed : -e.speed;
    }
}

void resetWalkNative(Entity &e) {
    if (e.walk) {
        const Props &b = *e.source->behavior("WalkBehaviour");
        restoreWalkNative(e,
                          WalkSavedNative{b.num("walk-speed", 75), b.flag("reverse-at-edge", true),
                                          !b.flag("walk-left"), 0});
    }
}

#include "jumper_animation_native.h"
#include "fish_animation_native.h"
#include "helper_animation_native.h"
#include "killable_death_native.h"
#include "goal_launch.h"
#include "ending_transformation_native.h"

struct EndingSceneRuntime {
    EndingTransformationInput input;
    float age;
    int active, emilio, maria;
    u64 next;
};

EndingSceneRuntime endingScene;
float endingCreditsFade;
bool playerWalkMode, playerWalkMoving;
float playerWalkAge, playerWalkIdleAge;

void resetEndingScene() {
    memset(&endingScene, 0, sizeof(endingScene));
    endingCreditsFade = 0;
    playerWalkMode = playerWalkMoving = false;
    playerWalkAge = playerWalkIdleAge = 0;
}

Entity entities[MAX_ENTITIES];
int entityCount, legacyEntityCount, playerIndex = -1, currentStage, coins, jumps, salamander,
                                    alternationBits, deaths;
float elapsed, camX, camY, deathTimer, winTimer, noticeTimer;
const char *notice = "";
bool complete;
bool checkpointSavePendingNative;

// Actor::Initialize 0xeaec4/eaef0 and PreUpdate 0xe61ec preserve the
// selected visual animation while its owner/layer simulation is paused.
bool nativeActorAnimateWhilePaused(const Entity &e) {
    return e.source && e.source->props.flag("animate-while-paused");
}

// All30 authored users are enemies whose timer is visual-only. Do not let a
// future flag on an orbit/rail/controller advance a shared movement clock.
bool nativeActorPauseAnimationClockSafe(const Entity &e) {
    return e.kind == ENEMY && !e.move && !e.orbit && !e.motion && !e.rotate;
}

void advanceNativePausedActorAnimation(Entity &e, float dt) {
    if (dt > 0 && !e.state.gone && nativeActorAnimateWhilePaused(e) &&
        nativeActorPauseAnimationClockSafe(e)) {
        e.state.timer += dt;
        advanceJumperAnimationNative(e, dt);
        advanceFishAnimationNative(e, dt);
    }
}

void advanceNativePausedStageAnimations(float dt) {
    for (int i = 0; i < entityCount; i++)
        advanceNativePausedActorAnimation(entities[i], dt);
}

float deathAge, deathDuration = 2.7f, deathX, deathY;
bool playerInWater;
bool debugMode;

void entityEffect(Entity &e, const char *behavior, const char *property, int domain = 0) {
    const Props *p = e.source->behavior(behavior);
    Object *base = effectClass(e.source->klass);
    if (p)
        emitEffect(idnum(p->get(property)), e.state.x, e.state.y, e.layer,
                   (e.local.r - e.local.l) / maxf(1, base ? base->rect.r - base->rect.l : 32),
                   domain);
}

enum Screen {
    TITLE,
    PLAY,
    PAUSE,
    SELECT,
    COMPLETE,
    CREDITS,
    SPLASH,
    WORLD,
    OPTIONS,
    GRAPHICS,
    CONTROLS,
    INTRO,
    ENDING,
    MAKER_BROWSE,
    MAKER_EDIT
};

Screen screen = SPLASH;
int menuSelection, selectedStage;
ReadyNativeState readyOpening = {};
bool readyPointerDown;
float readyTimer;
bool interfaceEnabled;
void stageInterface(bool resume);
void completeInterface();
void rebuildPhysics();
void restorePhysics();
void restoreLegacyWalkerGravityNative();
void restoreLegacyJumperGravityNative();
void restoreLegacyRotorGravityNative();
void restoreLegacyFreeEnemyGravityNative();
void deactivateGoalBodies();
void configureFrogs(bool reset);
void checkpointRestoreAnimations();
void resetWizardMotion(Entity &boss, unsigned int seed);
void resetNativeMotion(int index, bool legacy = false);
void nativeContinuousAdvanceScene(Level &scene, float scale, float time, float dt);
void nativeContinuousResetScene(Level &scene);
void nativeContinuousRebaseScene(Level &scene, float time);
void endGameInterface();
void openPause(bool animate = true);

float listNumber(const char *values, int index, float fallback = 0) {
    for (int i = 0; i < index && *values; i++) {
        while (*values && *values != ',')
            values++;
        if (*values)
            values++;
    }
    return number(values, fallback);
}

int initialAlternation() {
    return 0;
}

#include "spike_animation_native.h"
bool beginEndingSequence();
void touchAlternatorAlternateNative(int mask);
struct TouchLoadRuntimeNative;
TouchLoadRuntimeNative *captureTouchLoadRuntimeNative();
void restoreTouchLoadRuntimeNative(TouchLoadRuntimeNative *saved);
void alternateBodiesNative(int mask);

void alternateScene(int mask) {
    alternationBits ^= mask;
    alternateBodiesNative(mask);
    touchAlternatorAlternateNative(mask);
    for (int i = 0; i < entityCount; i++)
        alternateSpikeAnimation(entities[i], mask);
    if ((mask & 1) && !endingScene.active)
        for (int i = 0; i < current.objectCount; i++)
            if (current.objects[i].has("EndOfGameCutsceneController")) {
                beginEndingSequence();
                break;
            }
}

struct Bullet {
    State state;
    int klass;
    float life;
};

Bullet bullets[MAX_PROJECTILES];
#include "projectile_animation_state_native.h"

// EnemyBullet/FinalBossBullet SaveState retain first-step and object ignore IDs.
struct ShotSavedContacts {
    u32 ignored[(MAX_ENTITIES + 31) / 32];
    int first;
};

void captureShotContacts(ShotSavedContacts *out, ShotSavedContacts *extra = 0);
void restoreShotContacts(const ShotSavedContacts *saved, const ShotSavedContacts *extra = 0);
void captureProjectileExtraIgnoresNative(ProjectileExtraIgnoresNative &saved);
void restoreProjectileExtraIgnoresNative(const ProjectileExtraIgnoresNative &saved);
void resetProjectileLaunchContactsNative(int slot, int owner);
#include "bullet_source_state.h"
#include "source_track_native.h"
#include "bullet_source_animation_native.h"
BulletSourceAnimationNativeState checkpointSourceVisuals[MAX_ENTITIES];
bool haveCheckpointSourceVisuals;
u32 sourceVisualCheckpointChecksum;
#include "wizard_motion_state.h"
#include "wizard_pose.h"
#include "wizard_presentation_state.h"
#include "diagonal_state.h"
WizardArmMotion wizardArms[2];

struct Progress {
    u32 magic, version;
    int unlocked, lastStage, music, sound;
    int bestCoins[64], bestJumps[64], axolotls[64], cleared[64];
    u32 checksum;
};

Progress progress;
// Tom OnActivate caches min(personal record, authored par); LoadState only restores current jumps.
int stageJumpTarget;

int jumpTargetForStage(int stage) {
    if (makerRun.active) {
        int par = (int)current.props.num("bean-jump-par");
        return makerRun.recordJumps ? (int)minf((float)par, (float)makerRun.recordJumps) : par;
    }
    if (stage < 0 || stage >= stageCount)
        return 0;
    int par = (int)levels[stageList[stage]].props.num("bean-jump-par");
    return progress.cleared[stage] ? (int)minf((float)par, (float)progress.bestJumps[stage]) : par;
}

struct LegacyMotionState {
    float originX, originY, turnCooldown;
    int valid;
};

struct MotionState {
    float originX, originY, turnCooldown;
    int valid;
    float lastLaunchSpeed;
};

#include "pinball_state_native.h"
#include "trampoline_state_native.h"

struct LegacySnapshot {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    u32 checksum;
};

struct CameraState {
    Rect window;
    float x, y;
    int valid;
};

struct SnapshotV4 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    LegacyMotionState motion[MAX_ENTITIES];
    u32 checksum;
};

struct SnapshotV6 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    LegacyMotionState motion[MAX_ENTITIES];
    CameraState camera;
    u32 checksum;
};

struct SnapshotV7 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    u32 checksum;
};

struct SnapshotV8 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    u32 checksum;
};

struct SnapshotV10 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    u32 checksum;
};

struct SnapshotV11 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    u32 checksum;
};

struct SnapshotV12 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    u32 checksum;
};

struct SnapshotV13 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    u32 checksum;
};

struct SnapshotV14 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    u32 checksum;
};

struct SnapshotV15 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    u32 checksum;
};

struct SnapshotV16 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    u32 checksum;
};

struct SnapshotV17 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    Bullet extraBullets[MAX_PROJECTILES - 64];
    ShotSavedContacts extraShots[MAX_PROJECTILES - 64];
    u32 checksum;
};

struct SnapshotV18 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    Bullet extraBullets[MAX_PROJECTILES - 64];
    ShotSavedContacts extraShots[MAX_PROJECTILES - 64];
    ProjectileAnimationStateNative projectileAnimations;
    u32 checksum;
};

struct SnapshotV19 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    Bullet extraBullets[MAX_PROJECTILES - 64];
    ShotSavedContacts extraShots[MAX_PROJECTILES - 64];
    ProjectileAnimationStateNative projectileAnimations;
    ProjectileExtraIgnoresNative projectileIgnores;
    u32 checksum;
};

// PhysicsObject::Save/LoadPhysicsState preserve pending force/torque and
// the independent gravity flag. Body identity follows the existing entity,
// projectile and oscillator-proxy slots; terrain is static and has no force.
enum { NATIVE_PHYSICS_CHECKPOINT_BODIES = MAX_ENTITIES * 2 + MAX_PROJECTILES };

struct BodyPhysicsCheckpointNative {
    Vec force;
    float torque, gravityScale;
    int valid;
};

void captureBodyPhysicsNative(BodyPhysicsCheckpointNative *records);
void restoreBodyPhysicsNative(const BodyPhysicsCheckpointNative *records);

struct SnapshotV21 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    Bullet extraBullets[MAX_PROJECTILES - 64];
    ShotSavedContacts extraShots[MAX_PROJECTILES - 64];
    ProjectileAnimationStateNative projectileAnimations;
    ProjectileExtraIgnoresNative projectileIgnores;
    BodyPhysicsCheckpointNative physics[NATIVE_PHYSICS_CHECKPOINT_BODIES];
    u32 checksum;
};

struct SnapshotV22 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    Bullet extraBullets[MAX_PROJECTILES - 64];
    ShotSavedContacts extraShots[MAX_PROJECTILES - 64];
    ProjectileAnimationStateNative projectileAnimations;
    ProjectileExtraIgnoresNative projectileIgnores;
    BodyPhysicsCheckpointNative physics[NATIVE_PHYSICS_CHECKPOINT_BODIES];
    HelperAnimationNative helperAnimations[MAX_ENTITIES];
    u32 checksum;
};

struct SnapshotV23 {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    Bullet extraBullets[MAX_PROJECTILES - 64];
    ShotSavedContacts extraShots[MAX_PROJECTILES - 64];
    ProjectileAnimationStateNative projectileAnimations;
    ProjectileExtraIgnoresNative projectileIgnores;
    BodyPhysicsCheckpointNative physics[NATIVE_PHYSICS_CHECKPOINT_BODIES];
    HelperAnimationNative helperAnimations[MAX_ENTITIES];
    OrdinaryJumpStateNative ordinaryJump;
    u32 checksum;
};

struct Snapshot {
    u32 magic, version;
    int stage, count, coins, jumps, salamander, bits;
    float elapsed;
    State state[MAX_ENTITIES];
    Bullet bullets[64];
    MotionState motion[MAX_ENTITIES];
    CameraState camera;
    BulletSourceState sources[MAX_ENTITIES];
    SourceRandomState sourceRng;
    WizardMotionState wizard;
    WizardArmMotion arms[2];
    DiagonalRuntimeState diagonal[MAX_ENTITIES];
    OscillatorRuntimeState oscillator[MAX_ENTITIES];
    WizardPresentationState presentation;
    WizardCameraBlendState bossCamera;
    ShotSavedContacts shots[64];
    BulletSourceAnimationNativeState sourceVisuals[MAX_ENTITIES];
    PinballPendingNative pinball;
    TrampolineStateNative trampoline;
    JumperAnimationNative jumperAnimations[MAX_ENTITIES];
    FishAnimationNative fishAnimations[MAX_ENTITIES];
    Bullet extraBullets[MAX_PROJECTILES - 64];
    ShotSavedContacts extraShots[MAX_PROJECTILES - 64];
    ProjectileAnimationStateNative projectileAnimations;
    ProjectileExtraIgnoresNative projectileIgnores;
    BodyPhysicsCheckpointNative physics[NATIVE_PHYSICS_CHECKPOINT_BODIES];
    HelperAnimationNative helperAnimations[MAX_ENTITIES];
    OrdinaryJumpStateNative ordinaryJump;
    WalkSavedNative walking[MAX_ENTITIES];
    u32 checksum;
};

// Version25 changes walker gravity migration semantics, not the wire layout.
typedef Snapshot SnapshotV24;
typedef Snapshot SnapshotV25;
typedef Snapshot SnapshotV26;
typedef Snapshot SnapshotV27;
#include "oneway_checkpoint_state_native.h"
Snapshot checkpoint;
State initialState[MAX_ENTITIES];
MotionState initialMotion[MAX_ENTITIES];
bool haveCheckpoint;
CameraState cameraState;
const Props *cameraFollow;
Vec cameraDefault;
int cameraTarget;
char saveRoot[MAX_PATH] = "saves";
bool saveFailed;

u32 hashBytes(const void *data, u32 n) {
    const u8 *p = (const u8 *)data;
    u32 h = 2166136261u;
    while (n--) {
        h ^= *p++;
        h *= 16777619u;
    }
    return h;
}

void savePath(char *p, const char *file) {
    lstrcpyA(p, saveRoot);
    lstrcatA(p, "/");
    lstrcatA(p, file);
}

void tell(const char *text) {
    notice = text;
    noticeTimer = 3;
}

// Persist normal progress independently of transient physics state. Save-path ownership selects campaign or isolated tests.
void writeProgress() {
    if (debugMode)
        return;
    progress.magic = 0x50535142;
    progress.version = 2;
    progress.lastStage = currentStage;
    progress.music = musicEnabled;
    progress.sound = soundEnabled;
    progress.checksum = hashBytes(&progress, sizeof(progress) - 4);
    char p[MAX_PATH];
    savePath(p, "progress.dat");
    if (!atomicWrite(p, &progress, sizeof(progress))) {
        saveFailed = true;
        tell("Cannot write saves. Move the game to a writable folder.");
    }
}

bool exactRead(const char *path, void *out, u32 bytes) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    DWORD n = 0;
    bool ok = GetFileSize(f, 0) == bytes && ReadFile(f, out, bytes, &n, 0) && n == bytes;
    CloseHandle(f);
    return ok;
}

// Reject incompatible or malformed snapshots before reading entity and physics state from them.
bool validSnapshot(const Snapshot &s) {
    if (s.version >= 23 && !validOrdinaryJumpStateNative(s.ordinaryJump))
        return false;
    if (s.version >= 18 && !validProjectileAnimationStateNative(s.projectileAnimations, s.count))
        return false;
    if (s.version >= 20)
        for (int i = 0; i < NATIVE_PHYSICS_CHECKPOINT_BODIES; i++) {
            const BodyPhysicsCheckpointNative &b = s.physics[i];
            if (b.valid < 0 || b.valid > 1)
                return false;
            if (b.valid &&
                !(b.force.x >= -100000000 && b.force.x <= 100000000 && b.force.y >= -100000000 &&
                  b.force.y <= 100000000 && b.torque >= -100000000 && b.torque <= 100000000 &&
                  b.gravityScale >= 0 && b.gravityScale <= 100000))
                return false;
        }
    bool usedProjectileAnimations[MAX_PROJECTILES] = {};
    if (s.version >= 13 && !validPinballPendingNative(s.pinball, s.count))
        return false;
    if (s.version >= 22)
        for (int i = 0; i < s.count && i < MAX_ENTITIES; i++)
            if (!validHelperAnimationNative(s.helperAnimations[i]))
                return false;
    if (s.version >= 16)
        for (int i = 0; i < s.count && i < MAX_ENTITIES; i++)
            if (!validFishAnimationNative(s.fishAnimations[i]))
                return false;
    if (s.version >= 15)
        for (int i = 0; i < s.count && i < MAX_ENTITIES; i++)
            if (!validJumperAnimationNative(s.jumperAnimations[i]))
                return false;
    if (s.version >= 14 && !validTrampolineStateNative(s.trampoline, s.count))
        return false;
    if (s.magic != 0x43535142 || (s.version < 2 || s.version > 28) || s.stage < 0 ||
        s.stage >= stageCount || s.count < 1 || s.count > MAX_ENTITIES || s.coins < 0 ||
        s.coins > 100000 || s.jumps < 0 || s.jumps > 10000000 || s.salamander < 0 ||
        s.salamander > 1 || !(s.elapsed >= 0 && s.elapsed < 100000000) ||
        s.checksum != hashBytes(&s, sizeof(s) - 4))
        return false;
    for (int i = 0; i < s.count; i++) {
        const State &t = s.state[i];
        if (!(t.x >= -100000 && t.x <= 100000 && t.y >= -100000 && t.y <= 100000 &&
              t.vx >= -100000 && t.vx <= 100000 && t.vy >= -100000 && t.vy <= 100000 &&
              t.timer >= -100000 && t.timer <= 100000 && t.angle >= -100000 && t.angle <= 100000 &&
              t.aux >= -100000 && t.aux <= 100000))
            return false;
    }
    for (int i = 0; i < s.count; i++) {
        const MotionState &m = s.motion[i];
        if (m.valid && !(m.originX >= -100000 && m.originX <= 100000 && m.originY >= -100000 &&
                         m.originY <= 100000 && m.turnCooldown >= 0 && m.turnCooldown <= 10 &&
                         m.lastLaunchSpeed >= 0 && m.lastLaunchSpeed <= 100000))
            return false;
    }
    if (s.camera.valid) {
        const CameraState &c = s.camera;
        if (!(c.x >= -100000 && c.x <= 100000 && c.y >= -100000 && c.y <= 100000 &&
              c.window.l >= -100000 && c.window.r <= 100000 && c.window.b >= -100000 &&
              c.window.t <= 100000 && c.window.r >= c.window.l && c.window.t >= c.window.b))
            return false;
    }
    for (int i = 0; i < s.count; i++)
        if (s.sources[i].initialized &&
            (!(s.sources[i].clock >= -100000 && s.sources[i].clock < 100000) ||
             !(s.sources[i].interval > 0 && s.sources[i].interval < 100000) ||
             !(s.sources[i].angle >= -100000 && s.sources[i].angle <= 100000)))
            return false;
    if (s.version >= 8 && (s.sourceRng.index < 0 || s.sourceRng.index > 624 ||
                           s.wizard.randomIndex < 0 || s.wizard.randomIndex > 624))
        return false;
    if (s.version >= 8) {
        const WizardMotionState &w = s.wizard;
        if (w.initialized) {
            if (w.pendingCount < 0 || w.pendingCount > 3 || w.cycleCount < 0 ||
                w.cycleCount > 100000 ||
                !(w.periodX > 0 && w.periodY > 0 && w.age >= 0 && w.age < 100000000 &&
                  w.baseX >= -100000 && w.baseX <= 100000 && w.baseY >= -100000 &&
                  w.baseY <= 100000 && w.cycleAge >= 0 && w.cycleAge < 100000))
                return false;
            for (int i = 0; i < w.pendingCount; i++)
                if (w.pending[i] < 0 || w.pending[i] >= s.count)
                    return false;
        }
        for (int i = 0; i < 2; i++)
            if (!(s.arms[i].clock >= 0 && s.arms[i].clock < 100000000 && s.arms[i].recoilAge >= 0 &&
                  s.arms[i].recoilAge < 100000000))
                return false;
    }
    if (s.version >= 9) {
        if (s.presentation.animation < -1 || s.presentation.animation > 3 ||
            !(s.presentation.animationAge >= 0 && s.presentation.animationAge < 100000000 &&
              s.presentation.cooldown >= 0 && s.presentation.cooldown < 100000000))
            return false;
        for (int i = 0; i < s.count; i++) {
            const DiagonalRuntimeState &d = s.diagonal[i];
            const OscillatorRuntimeState &o = s.oscillator[i];
            if (!(d.directionX >= -1 && d.directionX <= 1 && d.directionY >= -1 &&
                  d.directionY <= 1 && d.effectiveSpeed >= 0 && d.effectiveSpeed < 100000 &&
                  o.effectiveSpeed >= 0 && o.effectiveSpeed < 100000 && o.clock >= -100000000 &&
                  o.clock <= 100000000))
                return false;
        }
    }
    if (s.version >= 11 &&
        !(s.bossCamera.remaining >= -100000000 && s.bossCamera.remaining <= 1.001f &&
          s.bossCamera.fromX >= -100000 && s.bossCamera.fromX <= 100000 &&
          s.bossCamera.fromY >= -100000 && s.bossCamera.fromY <= 100000))
        return false;
    if (s.version >= 12) {
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            const ShotSavedContacts &shot = i < 64 ? s.shots[i] : s.extraShots[i - 64];
            if (shot.first < 0 || shot.first > 1)
                return false;
        }
        for (int i = 0; i < s.count; i++) {
            const BulletSourceAnimationNativeState &v = s.sourceVisuals[i];
            if (v.phase < SOURCE_VISUAL_DEFAULT || v.phase > SOURCE_VISUAL_POST ||
                v.initialized < 0 || v.initialized > 1 || v.reconstructed < 0 ||
                v.reconstructed > 1 ||
                !(v.idleAge >= 0 && v.idleAge < 100000000 && v.preAge >= -100000 &&
                  v.preAge < 100000000 && v.postAge >= 0 && v.postAge < 100000000))
                return false;
        }
    }
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        const Bullet &b = i < 64 ? s.bullets[i] : s.extraBullets[i - 64];
        if (b.life <= 0)
            continue;
        if (b.klass < 0 || b.klass >= classCount || !(b.life < 30))
            return false;
        if (s.version >= 18) {
            int member = s.projectileAnimations.binding[i];
            if (member >= 0) {
                const ProjectileAnimationMemberNative &m = s.projectileAnimations.members[member];
                if (usedProjectileAnimations[member] || m.owner + 1 != b.state.active ||
                    m.klass != b.klass)
                    return false;
                usedProjectileAnimations[member] = true;
            }
        }
        if (b.state.gone && (b.state.gone != 2 || !classes[b.klass].has("KillableEnemyBehaviour") ||
                             !(b.state.aux >= 0 && b.state.aux < 100000000)))
            return false;
    }
    if (s.version >= 24)
        for (int i = 0; i < s.count; i++) {
            const WalkSavedNative &w = s.walking[i];
            if (!(w.speed >= 0 && w.speed < 100000) || w.reverseEdge < 0 || w.reverseEdge > 1 ||
                w.right < 0 || w.right > 1 || w.canWalk < 0 || w.canWalk > 1)
                return false;
        }
    return true;
}

bool readCheckpointRawNative(const char *path) {
    if (exactRead(path, &checkpoint, sizeof(checkpoint)) && validSnapshot(checkpoint))
        return true;
    SnapshotV23 *v23 = (SnapshotV23 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV23));
    if (!v23)
        return false;
    bool from23 = exactRead(path, v23, sizeof(*v23)) && v23->magic == 0x43535142 &&
                  v23->version == 23 && v23->checksum == hashBytes(v23, sizeof(*v23) - 4);
    if (from23) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v23, sizeof(*v23) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from23 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v23);
    if (from23)
        return true;
    SnapshotV22 *v22 = (SnapshotV22 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV22));
    if (!v22)
        return false;
    bool from22 = exactRead(path, v22, sizeof(*v22)) && v22->magic == 0x43535142 &&
                  v22->version == 22 && v22->checksum == hashBytes(v22, sizeof(*v22) - 4);
    if (from22) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v22, sizeof(*v22) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from22 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v22);
    if (from22)
        return true;
    SnapshotV21 *v21 = (SnapshotV21 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV21));
    if (!v21)
        return false;
    bool from21 = exactRead(path, v21, sizeof(*v21)) && v21->magic == 0x43535142 &&
                  (v21->version == 20 || v21->version == 21) &&
                  v21->checksum == hashBytes(v21, sizeof(*v21) - 4);
    if (from21) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v21, sizeof(*v21) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from21 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v21);
    if (from21)
        return true;
    SnapshotV19 *v19 = (SnapshotV19 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV19));
    if (!v19)
        return false;
    bool from19 = exactRead(path, v19, sizeof(*v19)) && v19->magic == 0x43535142 &&
                  v19->version == 19 && v19->checksum == hashBytes(v19, sizeof(*v19) - 4);
    if (from19) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v19, sizeof(*v19) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from19 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v19);
    if (from19)
        return true;
    SnapshotV18 *v18 = (SnapshotV18 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV18));
    if (!v18)
        return false;
    bool from18 = exactRead(path, v18, sizeof(*v18)) && v18->magic == 0x43535142 &&
                  v18->version == 18 && v18->checksum == hashBytes(v18, sizeof(*v18) - 4);
    if (from18) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v18, sizeof(*v18) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from18 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v18);
    if (from18)
        return true;
    SnapshotV17 *v17 = (SnapshotV17 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV17));
    if (!v17)
        return false;
    bool from17 = exactRead(path, v17, sizeof(*v17)) && v17->magic == 0x43535142 &&
                  v17->version == 17 && v17->checksum == hashBytes(v17, sizeof(*v17) - 4);
    if (from17) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v17, sizeof(*v17) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from17 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v17);
    if (from17)
        return true;
    SnapshotV16 *v16 = (SnapshotV16 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV16));
    if (!v16)
        return false;
    bool from16 = exactRead(path, v16, sizeof(*v16)) && v16->magic == 0x43535142 &&
                  v16->version == 16 && v16->checksum == hashBytes(v16, sizeof(*v16) - 4);
    if (from16) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v16, sizeof(*v16) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from16 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v16);
    if (from16)
        return true;
    SnapshotV15 *v15 = (SnapshotV15 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV15));
    if (!v15)
        return false;
    bool from15 = exactRead(path, v15, sizeof(*v15)) && v15->magic == 0x43535142 &&
                  v15->version == 15 && v15->checksum == hashBytes(v15, sizeof(*v15) - 4);
    if (from15) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v15, sizeof(*v15) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from15 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v15);
    if (from15)
        return true;
    SnapshotV14 *v14 = (SnapshotV14 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV14));
    if (!v14)
        return false;
    bool from14 = exactRead(path, v14, sizeof(*v14)) && v14->magic == 0x43535142 &&
                  v14->version == 14 && v14->checksum == hashBytes(v14, sizeof(*v14) - 4);
    if (from14) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v14, sizeof(*v14) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from14 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v14);
    if (from14)
        return true;
    SnapshotV13 *v13 = (SnapshotV13 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV13));
    if (!v13)
        return false;
    bool from13 = exactRead(path, v13, sizeof(*v13)) && v13->magic == 0x43535142 &&
                  v13->version == 13 && v13->checksum == hashBytes(v13, sizeof(*v13) - 4);
    if (from13) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v13, sizeof(*v13) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from13 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v13);
    if (from13)
        return true;
    SnapshotV12 *v12 = (SnapshotV12 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV12));
    if (!v12)
        return false;
    bool from12 = exactRead(path, v12, sizeof(*v12)) && v12->magic == 0x43535142 &&
                  v12->version == 12 && v12->checksum == hashBytes(v12, sizeof(*v12) - 4);
    if (from12) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v12, sizeof(*v12) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from12 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v12);
    if (from12)
        return true;
    SnapshotV11 *v11 = (SnapshotV11 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV11));
    if (!v11)
        return false;
    bool from11 = exactRead(path, v11, sizeof(*v11)) && v11->magic == 0x43535142 &&
                  v11->version == 11 && v11->checksum == hashBytes(v11, sizeof(*v11) - 4);
    if (from11) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v11, sizeof(*v11) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from11 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v11);
    if (from11)
        return true;
    SnapshotV10 *v10 = (SnapshotV10 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV10));
    if (!v10)
        return false;
    bool from10 = exactRead(path, v10, sizeof(*v10)) && v10->magic == 0x43535142 &&
                  (v10->version == 9 || v10->version == 10) &&
                  v10->checksum == hashBytes(v10, sizeof(*v10) - 4);
    if (from10) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v10, sizeof(*v10) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from10 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v10);
    if (from10)
        return true;
    SnapshotV8 *v8 = (SnapshotV8 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV8));
    if (!v8)
        return false;
    bool from8 = exactRead(path, v8, sizeof(*v8)) && v8->magic == 0x43535142 && v8->version == 8 &&
                 v8->checksum == hashBytes(v8, sizeof(*v8) - 4);
    if (from8) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v8, sizeof(*v8) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        from8 = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v8);
    if (from8)
        return true;
    SnapshotV7 *v7 = (SnapshotV7 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV7));
    if (!v7)
        return false;
    bool currentLegacy = exactRead(path, v7, sizeof(*v7)) && v7->magic == 0x43535142 &&
                         v7->version == 7 && v7->checksum == hashBytes(v7, sizeof(*v7) - 4);
    if (currentLegacy) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v7, sizeof(*v7) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        currentLegacy = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v7);
    if (currentLegacy)
        return true;
    SnapshotV6 *v6 = (SnapshotV6 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV6));
    if (!v6)
        return false;
    bool converted = exactRead(path, v6, sizeof(*v6)) && v6->magic == 0x43535142 &&
                     (v6->version == 5 || v6->version == 6) && v6->count > 0 &&
                     v6->count <= MAX_ENTITIES && v6->checksum == hashBytes(v6, sizeof(*v6) - 4);
    if (converted) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v6, sizeof(LegacySnapshot) - 4);
        for (int i = 0; i < v6->count; i++) {
            LegacyMotionState &m = v6->motion[i];
            checkpoint.motion[i] = MotionState{m.originX, m.originY, m.turnCooldown, m.valid, 0};
        }
        checkpoint.camera = v6->camera;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        converted = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, v6);
    if (converted)
        return true;
    SnapshotV4 *v4 = (SnapshotV4 *)HeapAlloc(GetProcessHeap(), 0, sizeof(SnapshotV4));
    if (!v4)
        return false;
    bool migrated = exactRead(path, v4, sizeof(*v4)) && v4->magic == 0x43535142 &&
                    v4->version == 4 && v4->checksum == hashBytes(v4, sizeof(*v4) - 4);
    if (migrated && v4->count > 0 && v4->count <= MAX_ENTITIES) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, v4, sizeof(LegacySnapshot) - 4);
        for (int i = 0; i < v4->count; i++) {
            LegacyMotionState &m = v4->motion[i];
            checkpoint.motion[i] = MotionState{m.originX, m.originY, m.turnCooldown, m.valid, 0};
        }
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        migrated = validSnapshot(checkpoint);
    } else
        migrated = false;
    HeapFree(GetProcessHeap(), 0, v4);
    if (migrated)
        return true;
    LegacySnapshot *old = (LegacySnapshot *)HeapAlloc(GetProcessHeap(), 0, sizeof(LegacySnapshot));
    if (!old)
        return false;
    bool ok = exactRead(path, old, sizeof(*old)) && old->magic == 0x43535142 &&
              (old->version == 2 || old->version == 3) &&
              old->checksum == hashBytes(old, sizeof(*old) - 4);
    if (ok) {
        memset(&checkpoint, 0, sizeof(checkpoint));
        memcpy(&checkpoint, old, sizeof(*old) - 4);
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
        ok = validSnapshot(checkpoint);
    }
    HeapFree(GetProcessHeap(), 0, old);
    return ok;
}

bool readCheckpoint(const char *path) {
    bool read = readCheckpointRawNative(path);
    readOneWayCheckpointNative(path, read ? checkpoint.checksum : 0);
    return read;
}

void readProgress() {
    haveCheckpointSourceVisuals = false;
    CreateDirectoryA(saveRoot, 0);
    char p[MAX_PATH];
    savePath(p, "progress.dat");
    memset(&progress, 0, sizeof(progress));
    if (!exactRead(p, &progress, sizeof(progress)) || progress.magic != 0x50535142 ||
        (progress.version != 1 && progress.version != 2) ||
        progress.checksum != hashBytes(&progress, sizeof(progress) - 4)) {
        memset(&progress, 0, sizeof(progress));
        progress.music = progress.sound = 1;
    }
    if (progress.version == 1) {
        Progress old = progress;
        memset(progress.bestCoins, 0, sizeof(progress.bestCoins));
        memset(progress.bestJumps, 0, sizeof(progress.bestJumps));
        memset(progress.axolotls, 0, sizeof(progress.axolotls));
        memset(progress.cleared, 0, sizeof(progress.cleared));
        progress.unlocked = 0;
        for (int i = 0; i < stageCount; i++) {
            int newIndex = stageById(levels[legacyStageList[i]].id);
            progress.bestCoins[newIndex] = old.bestCoins[i];
            progress.bestJumps[newIndex] = old.bestJumps[i];
            progress.axolotls[newIndex] = old.axolotls[i];
            progress.cleared[newIndex] = old.cleared[i];
            if (i == old.lastStage)
                progress.lastStage = newIndex;
            if (i <= old.unlocked)
                progress.unlocked = (int)maxf((float)progress.unlocked, (float)newIndex);
        }
        progress.version = 2;
    }
    progress.unlocked = (int)clamp((float)progress.unlocked, 0, (float)stageCount - 1);
    currentStage = (int)clamp((float)progress.lastStage, 0, (float)stageCount - 1);
    musicEnabled = progress.music != 0;
    soundEnabled = progress.sound != 0;
    savePath(p, "checkpoint.dat");
    haveCheckpoint = readCheckpoint(p);
    if (haveCheckpoint && checkpoint.version == 2)
        checkpoint.stage = stageById(levels[legacyStageList[checkpoint.stage]].id);
    if (haveCheckpoint) {
        if (checkpoint.version < 5)
            checkpoint.version = 5;
        checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    }
}

Rect entityRect(const Entity &e) {
    return shifted(e.local, e.state.x, e.state.y);
}

Rect hitRect(const Entity &e) {
    return shifted(e.hit, e.state.x, e.state.y);
}

bool activeBlock(const Entity &e) {
    if (e.state.gone)
        return false;
    if (e.kind == ALTERNATOR)
        return ((alternationBits & e.mask) != 0) != (e.alternation != 0);
    return true;
}

bool solid(const Entity &e) {
    return !(e.crumble && e.state.active == 2) && activeBlock(e) &&
           (e.kind == BLOCK || e.kind == PLATFORM || e.kind == TRAMPOLINE || e.kind == SPRING ||
            e.kind == MAGNET || e.kind == ALTERNATOR || e.kind == TURRET || e.kind == EXIT);
}

float approach(float value, float target, float delta) {
    if (value < target)
        return minf(value + delta, target);
    return maxf(value - delta, target);
}

#include "tom_horizontal_native.h"

bool supportAt(float x, float y) {
    for (int i = 0; i < current.geometryCount; i++) {
        Geometry &g = current.geometry[i];
        for (int j = 1; j < g.count; j++) {
            Vec a = g.points[j - 1], b = g.points[j];
            if (x >= minf(a.x, b.x) && x <= maxf(a.x, b.x) && absf(a.x - b.x) > 0.1f) {
                float h = a.y + (b.y - a.y) * (x - a.x) / (b.x - a.x);
                if (absf(y - h) < 6)
                    return true;
            }
        }
    }
    for (int i = 0; i < entityCount; i++)
        if (solid(entities[i])) {
            Rect r = hitRect(entities[i]);
            if (x >= r.l && x <= r.r && absf(r.t - y) < 6)
                return true;
        }
    return false;
}

// Sweep the four sides of the collision box against the original segment outlines.
// One-way platforms only intercept a downward crossing.
bool moveBody(Entity &e, float dt, bool bounce, bool *hitWall = 0, Entity *environment = 0,
              int environmentCount = -1, Level *scene = 0) {
    Level &map = scene ? *scene : current;
    Entity *things = environment ? environment : entities;
    int count = environmentCount < 0 ? entityCount : environmentCount;
    Rect old = hitRect(e);
    float dx = e.state.vx * dt;
    bool wall = false, land = false;
    float newX = e.state.x + dx;
    for (int i = 0; i < map.geometryCount; i++) {
        Geometry &g = map.geometry[i];
        if (g.kind == 2)
            continue;
        for (int j = 1; j < g.count; j++) {
            Vec a = g.points[j - 1], b = g.points[j];
            if (absf(a.x - b.x) > 0.01f)
                continue;
            if (old.t <= minf(a.y, b.y) + 0.1f || old.b >= maxf(a.y, b.y) - 0.1f)
                continue;
            if (dx > 0 && old.r <= a.x + 0.02f && newX + e.hit.r > a.x) {
                newX = minf(newX, a.x - e.hit.r);
                wall = true;
            }
            if (dx < 0 && old.l >= a.x - 0.02f && newX + e.hit.l < a.x) {
                newX = maxf(newX, a.x - e.hit.l);
                wall = true;
            }
        }
    }
    for (int i = 0; i < count; i++) {
        Entity &o = things[i];
        if (&e == &o || !solid(o) || o.source->has("PlatformBehaviour") || o.crumble)
            continue;
        Rect r = hitRect(o);
        if (old.t <= r.b + 0.1f || old.b >= r.t - 0.1f)
            continue;
        if (dx > 0 && old.r <= r.l + 0.02f && newX + e.hit.r > r.l) {
            newX = minf(newX, r.l - e.hit.r);
            wall = true;
            if (bounce && o.dynamic)
                o.state.vx = 80;
        }
        if (dx < 0 && old.l >= r.r - 0.02f && newX + e.hit.l < r.r) {
            newX = maxf(newX, r.r - e.hit.l);
            wall = true;
            if (bounce && o.dynamic)
                o.state.vx = -80;
        }
    }
    e.state.x = newX;
    if (wall)
        e.state.vx = 0;
    Rect mid = hitRect(e);
    float dy = e.state.vy * dt;
    float newY = e.state.y + dy;
    float launch = e.jumpPower;
    int touched = -1;
    for (int i = 0; i < map.geometryCount; i++) {
        Geometry &g = map.geometry[i];
        for (int j = 1; j < g.count; j++) {
            Vec a = g.points[j - 1], b = g.points[j];
            if (absf(a.x - b.x) < 0.01f)
                continue;
            if (mid.r <= minf(a.x, b.x) + 0.05f || mid.l >= maxf(a.x, b.x) - 0.05f)
                continue;
            float x = clamp(e.state.x, minf(a.x, b.x), maxf(a.x, b.x));
            float h = a.y + (b.y - a.y) * (x - a.x) / (b.x - a.x);
            if (dy <= 0 && old.b >= h - 0.04f && newY + e.hit.b <= h) {
                newY = maxf(newY, h - e.hit.b);
                land = true;
            }
            if (dy > 0 && g.kind != 2 && old.t <= h + 0.04f && newY + e.hit.t >= h) {
                newY = minf(newY, h - e.hit.t);
                e.state.vy = 0;
            }
        }
    }
    for (int i = 0; i < count; i++) {
        Entity &o = things[i];
        if (&e == &o || !solid(o))
            continue;
        Rect r = hitRect(o);
        if (mid.r <= r.l + 0.02f || mid.l >= r.r - 0.02f)
            continue;
        if (absf(o.state.angle) > 0.01f && absf(o.state.angle) < 75) {
            float a = o.state.angle * 0.017453293f;
            float h =
                o.state.y + o.hit.t / cosine(a) + (e.state.x - o.state.x) * sine(a) / cosine(a);
            r.t = h;
            r.b = h - (o.hit.t - o.hit.b);
        }
        if (dy <= 0 && old.b >= r.t - 1.0f && newY + e.hit.b <= r.t) {
            float h = r.t - e.hit.b;
            if (h >= newY) {
                newY = h;
                land = true;
                touched = i;
            }
        }
        if (dy > 0 && !o.source->has("PlatformBehaviour") && !o.crumble && old.t <= r.b + 0.04f &&
            newY + e.hit.t >= r.b) {
            newY = minf(newY, r.b - e.hit.t);
            e.state.vy = 0;
            if (bounce && o.breakable) {
                if (--o.state.hp <= 0) {
                    o.state.gone = 1;
                    sound("smallexplosion");
                }
            }
        }
    }
    e.state.y = newY;
    if (land) {
        if (touched >= 0) {
            Entity &o = things[touched];
            if (o.kind == SPRING) {
                launch = root(2 * 400 * (74 + o.jumpPower));
                if (bounce)
                    sound("megajump");
            }
            if (o.kind == TRAMPOLINE) {
                launch = maxf(launch, -e.state.vy + o.jumpPower);
                if (bounce)
                    sound("megajump");
            }
            if (o.crumble && !o.state.active) {
                o.state.active = 1;
                o.state.timer = 0;
            }
            if (o.dynamic && bounce)
                o.state.vy = -35;
            if (o.kind == MAGNET && bounce) {
                launch = 0;
                e.state.active = 2;
            }
            if (o.touch && bounce) {
                alternateScene(o.mask);
                launch = e.jumpPower;
            }
        }
        e.state.vy = bounce ? launch : 0;
        if (bounce && launch > 0)
            e.lastLaunchSpeed = launch;
        if (bounce && launch > 0) {
            if (e.state.hp == 1 && jumps < 999)
                jumps++;
            if (touched < 0 || things[touched].kind != SPRING)
                sound("jump");
            e.state.timer = 0;
        }
    }
    if (hitWall)
        *hitWall = wall;
    return land;
}

Kind kindOf(Object &o) {
    if (o.has("HelperBirdBehaviour"))
        return HELPER;
    if (o.has("HelperBirdPerchBehaviour"))
        return PERCH;
    if (o.has("SecretRevealerBehaviour"))
        return SECRET;
    if (o.has("TomPlayerBehaviour"))
        return PLAYER;
    if (o.has("CoinGroupBehaviour"))
        return COIN;
    if (o.has("CollectableSalamanderBehaviour"))
        return SALAMANDER;
    if (o.has("CheckpointBehaviour"))
        return CHECKPOINT;
    if (o.has("LevelEndBehaviour"))
        return EXIT;
    if (o.has("SpikeGroupBehaviour"))
        return SPIKE;
    if (o.has("SpringBehaviour"))
        return SPRING;
    if (o.has("TrampolineBehaviour"))
        return TRAMPOLINE;
    if (o.has("AlternatorSwitchBehaviour"))
        return SWITCH;
    if (o.has("AlternatorBlockBehaviour") || o.has("AlternatorTouchBlockBehaviour"))
        return ALTERNATOR;
    if (o.has("MagnetBehaviour"))
        return MAGNET;
    if (o.has("ForceAreaBehaviour"))
        return FORCE;
    if (o.has("WaterAreaBehaviour"))
        return WATER;
    if (o.has("PinballBouncerBehaviour"))
        return BOUNCER;
    if (o.has("FinalBossBehaviour"))
        return BOSS;
    if (o.has("BulletSource"))
        return TURRET;
    if (o.has("KillableEnemyBehaviour") || o.has("DeadlyBehaviour"))
        return ENEMY;
    if (o.has("UnbeanPotionBehaviour"))
        return POTION;
    if (o.has("PlatformBehaviour") || o.has("CrumblingPlatformBehaviour") ||
        o.has("SeesawBehaviour"))
        return PLATFORM;
    if (o.has("BoxPhysicsBehaviour") || o.has("CirclePhysicsBehaviour") ||
        o.has("BoxGroupPhysicsBehaviour"))
        return BLOCK;
    return DECOR;
}

// Orbit::UpdatePosition0x284afc keeps the signed period (negative reverses).
float nativeOrbitAngle(float clock, float period, float phase) {
    return (period != 0 ? clock * 6.2831854820251465f / period : 0) + phase;
}

void positionNativeOrbit(Entity &e) {
    float a = nativeOrbitAngle(e.state.timer, e.period, e.phase);
    e.state.x = e.originX + bqCos(a) * e.radius;
    e.state.y = e.originY + bqSin(a) * e.radius;
}

// OrbitingObject::OnActivate0x28b368..374 clears its local clock and
// immediately positions the child. No first physics frame is required.
void positionOrbitingChildNative(Entity &child, const Entity &parent) {
    float a = nativeOrbitAngle(child.state.timer, child.period, child.phase);
    child.state.x = parent.state.x + bqCos(a) * child.radius;
    child.state.y = parent.state.y + bqSin(a) * child.radius;
}

// Combine the authored class and instance data once; collision geometry and visual geometry stay distinct.
void initializeEntity(Entity &e, Object &o, Rect r) {
    memset(&e, 0, sizeof(e));
    e.parent = -1;
    e.source = &o;
    e.kind = kindOf(o);
    Vec c = center(r);
    e.originX = e.state.x = c.x;
    e.originY = e.state.y = c.y;
    e.local = shifted(r, -c.x, -c.y);
    e.hit = e.local;
    e.state.angle = o.angle;
    e.state.hp = 3;
    e.layer = o.layer;
    e.z = o.z;
    e.art = actor(o.klass);
    e.jumpPower = root(2 * 400 * 74);
    e.state.active = 1;
    const Props *bp = o.behavior("BoxPhysicsBehaviour");
    if (!bp) {
        bp = o.behavior("CirclePhysicsBehaviour");
        e.circle = bp != 0;
    }
    if (!bp)
        bp = o.behavior("BoxGroupPhysicsBehaviour");
    if (!bp)
        bp = o.behavior("AlternatorBlockBehaviour");
    if (!bp)
        bp = o.behavior("AlternatorTouchBlockBehaviour");
    if (!bp)
        bp = o.behavior("CrumblingPlatformBehaviour");
    if (!bp)
        bp = o.behavior("TomPlayerBehaviour");
    if (!bp)
        bp = o.behavior("SpikeGroupBehaviour");
    if (!bp)
        bp = o.behavior("SeesawBehaviour");
    if (!bp)
        bp = o.behavior("LevelEndBehaviour");
    if (!bp)
        bp = o.behavior("PinballBouncerBehaviour");
    if (!bp)
        bp = o.behavior("CollectableSalamanderBehaviour");
    if (!bp)
        bp = o.behavior("UnbeanPotionBehaviour");
    e.physics = bp;
    if (bp) {
        e.hit.l += bp->num("adjust-left");
        e.hit.r += bp->num("adjust-right");
        e.hit.b += bp->num("adjust-bottom");
        e.hit.t += bp->num("adjust-top");
        e.dynamic = !bp->flag("static-position", false) &&
                    (e.kind == BLOCK || e.kind == ENEMY || e.kind == DECOR);
    }
    if (e.kind == PLAYER) {
        e.state.hp = 0;
        e.jumpPower = root(2 * absf(bp->num("gravity", -400)) * bp->num("jump-height", 74));
        e.state.aux = 1;
    }
    e.lastLaunchSpeed = e.jumpPower;
    if (e.kind == HELPER) {
        e.state.active = 0;
        e.state.aux = 1;
        e.dynamic = 0;
    }
    if (e.kind == PERCH) {
        e.state.active = 0;
        e.state.hp = 0;
    }
    // WaterAreaBehaviour::OnActivate 0x32c1d0 lowers the box by four pixels.
    if (e.kind == WATER) {
        e.originY = e.state.y -= 4;
    }
    if (e.kind == COIN || e.kind == SALAMANDER || e.kind == POTION)
        e.circle = 1;
    if (e.kind == ENEMY) {
        e.deadly = o.has("DeadlyBehaviour");
        e.spiky = e.deadly;
        e.state.active = o.has("ProximityActivateBehaviour") ? 0 : 1;
    }
    if (e.kind == SPRING) {
        const Props *b = o.behavior("SpringBehaviour");
        e.jumpPower = b->num("jump-power", 200);
    }
    if (e.kind == TRAMPOLINE) {
        e.jumpPower = o.behavior("TrampolineBehaviour")->num("jump-mod");
    }
    if (e.kind == ALTERNATOR) {
        const Props *b = o.behavior("AlternatorBlockBehaviour");
        if (!b) {
            b = o.behavior("AlternatorTouchBlockBehaviour");
            e.touch = 1;
            e.state.timer = .1f;
        }
        e.mask = (int)b->num("AlternationMask", 1);
        e.alternation = (int)b->num("Alternation", 1);
    }
    if (e.kind == SWITCH) {
        const Props *b = o.behavior("AlternatorSwitchBehaviour");
        e.mask = (int)b->num("AlternationMask", 1);
        e.alternation = (int)b->num("Alternation", 0);
    }
    if (o.has("WalkBehaviour")) {
        const Props *b = o.behavior("WalkBehaviour");
        e.walk = 1;
        e.walkRight = !b->flag("walk-left");
        e.walkCanWalk = 0;
        e.reverseEdge = b->flag("reverse-at-edge", true);
        e.speed = b->num("walk-speed", 75);
        e.state.aux = e.speed * (b->flag("walk-left") ? -1 : 1);
        e.state.vx = 0;
    }
    // Original diagonal motion is a normalized constant-speed vector. Store
    // both signs in checkpointed state, so restoring cannot reverse a fish.
    if (o.has("DiagonalBehaviour")) {
        const Props *b = o.behavior("DiagonalBehaviour");
        float x = b->num("move-x", -1), y = b->num("move-y", -1), length = root(x * x + y * y);
        e.moveX = length > 0 ? x / length : 0;
        e.moveY = length > 0 ? y / length : 0;
        e.speed = b->num("speed", 100);
        e.state.aux = (float)((x < 0 ? 1 : 0) | (y < 0 ? 2 : 0));
        e.state.vx = e.moveX * e.speed;
        e.state.vy = e.moveY * e.speed;
    }
    if (o.has("EnemyJumperBehaviour")) {
        e.state.hp = 0;
        e.state.aux = 0;
    }
    if (o.has("OscillatingBehaviour")) {
        e.move = o.behavior("OscillatingBehaviour");
        e.moveX = e.move->num("move-x");
        e.moveY = e.move->num("move-y");
        float length = root(e.moveX * e.moveX + e.moveY * e.moveY);
        if (length > 0.0000001f) {
            e.moveX /= length;
            e.moveY /= length;
        }
        e.speed = e.move->num("speed", 0);
        e.lo = e.move->num("min-move", 0);
        e.hi = e.move->num("max-move", 0);
        e.state.aux = 1;
    }
    if (o.has("OrbitBehaviour")) {
        const Props *b = o.behavior("OrbitBehaviour");
        e.orbit = 1;
        e.radius = b->num("radius", 100);
        e.period = b->num("period", 1);
        e.phase = b->num("start-angle") * 0.017453293f;
    }
    if (o.has("RotatingBehaviour")) {
        e.rotate = 1;
        e.speed = o.behavior("RotatingBehaviour")->num("rotation-speed", 90);
    }
    if (o.has("SeesawBehaviour")) {
        e.rotate = 2;
        e.radius = 12;
        e.period = 3;
    }
    if (o.has("BreakableBehaviour")) {
        e.breakable = 1;
        e.state.hp = (int)o.behavior("BreakableBehaviour")->num("hp", 3);
    }
    if (o.has("CrumblingPlatformBehaviour")) {
        e.crumble = 1;
        e.state.active = 0;
    }
    if (e.kind == SPIKE) {
        const Props *sp = o.behavior("SpikeGroupBehaviour");
        e.mask = (int)sp->num("alternation-mask");
        e.alternation = (int)sp->num("alternation", 1);
        resetSpikeAnimation(e, alternationBits);
    }
    if (e.kind == BOSS) {
        e.state.hp = 5;
        e.state.active = 0;
    }
    // The native proximity controller pauses any parent actor, including
    // moving blocks; limiting it to enemy kinds let rails move prematurely.
    // ProximityActivate::OnActivate 0x2dc4f8 initializes its sticky flag
    // from !owner.IsPaused(). Only authored paused actors wait for proximity.
    if (o.has("ProximityActivateBehaviour"))
        e.state.active = o.paused ? 0 : 1;
    if (e.orbit)
        positionNativeOrbit(e);
    if (e.kind == COIN) {
        // Only custom courses set this property; authored gems retain native random selection.
        int style = (int)o.props.num("maker-gem-style");
        initializeCoinAnimationNative(e, style >= 1 && style <= 5 ? style - 1 : -1);
    }
}

void addEntity(Object &o, Rect r) {
    if (entityCount >= MAX_ENTITIES)
        fatal("Too many level entities.");
    initializeEntity(entities[entityCount], o, r);
    if (entities[entityCount].kind == PLAYER)
        playerIndex = entityCount;
    entityCount++;
}

Object *classById(u64 id) {
    for (int i = 0; i < classCount; i++)
        if (classes[i].id == id)
            return classes + i;
    return 0;
}

#include "projectile_animation_native.h"

void attachObject(int parent, u64 classId, float distance, int motion, float period,
                  float amplitude, float phase) {
    Object *base = classById(classId);
    if (!base)
        return;
    Object *copy = levelArena.array<Object>(1);
    *copy = *base;
    copy->layer =
        entities[parent]
            .layer; // SpawnObject retains the spawned class Z; only its scene layer is inherited.
    Vec c = center(base->rect);
    Rect local = shifted(base->rect, -c.x, -c.y);
    addEntity(*copy, shifted(local, entities[parent].originX, entities[parent].originY - distance));
    Entity &e = entities[entityCount - 1];
    e.parent = parent;
    e.motion = motion;
    if (motion) {
        e.radius = distance;
        e.period = period;
        e.phase = phase;
        e.speed = amplitude;
        e.dynamic = 0;
    }
    e.state.active = 1;
}

// WreckingBall::UpdatePositions0x33f40c uses pi*time/Period: Period is
// a half cycle. Its independent saved controller clock drives all links.
void positionWreckingChild(Entity &e) {
    if (e.parent < 0)
        return;
    const Entity &parent = entities[e.parent];
    float a = bqCos((parent.state.timer + e.phase) * 3.1415927410125732f / e.period) * e.speed *
              0.01745329238474369f;
    e.state.x = parent.state.x + bqSin(a) * e.radius;
    e.state.y = parent.state.y - bqCos(a) * e.radius;
}

// Construct actors in resource order, then resolve parent attachments so references see complete entities.
void buildEntities() {
    resetEndingScene();
    entityCount = 0;
    playerIndex = -1;
    memset(entities, 0, sizeof(entities));
    for (int i = 0; i < current.objectCount; i++) {
        Object &o = current.objects[i];
        if (o.layer < 0 || o.layer >= current.layerCount)
            continue;
        Layer &layer = current.layers[o.layer];
        if (layer.flags[2] || !layer.flags[1] || !o.visible || o.base == 3 || o.base == 4 ||
            o.base == 2)
            continue;
        bool developer = false;
        for (int j = 0; j < current.objectCount; j++)
            if (current.objects[j].layer == o.layer &&
                current.objects[j].has("DeveloperCommentsController"))
                developer = true;
        if (developer)
            continue;
        if (o.base == 7) {
            float w = o.props.num("actlet-width", 32), h = o.props.num("actlet-height", 32);
            if (w < 1 || h < 1)
                continue;
            for (float y = o.rect.b; y < o.rect.t - 0.5f; y += h)
                for (float x = o.rect.l; x < o.rect.r - 0.5f; x += w) {
                    Rect r = {x, y, minf(x + w, o.rect.r), minf(y + h, o.rect.t)};
                    addEntity(o, r);
                }
        } else
            addEntity(o, o.rect);
    }
    int roots = entityCount;
    for (int i = 0; i < roots; i++) {
        Object &o = *entities[i].source;
        if (o.has("WreckingBallControllerBehaviour")) {
            const Props *b = o.behavior("WreckingBallControllerBehaviour");
            u64 link = idnum(b->get("LinkClass")), end = idnum(b->get("EndClass"));
            Object *linkClass = classById(link);
            float linkHalf = linkClass ? (linkClass->rect.t - linkClass->rect.b) * 0.5f : 4,
                  spacing = b->num("LinkSpacing", 1);
            int n = (int)b->num("NumLinks", 4);
            float period = b->num("Period", 2), amp = b->num("MaxAngle", 60),
                  phase = b->num("TimeOffset"), distance = (o.rect.t - o.rect.b) * 0.5f;
            for (int j = 0; j < n; j++) {
                distance += linkHalf + spacing;
                attachObject(i, link, distance, 1, period, amp, phase);
                distance += linkHalf;
            }
            Object *endClass = classById(end);
            float endHalf = endClass ? (endClass->rect.t - endClass->rect.b) * 0.5f : 16;
            attachObject(i, end, distance + endHalf + b->num("LastLinkSpacing", -6), 1, period, amp,
                         phase);
        }
        if (o.has("OrbitingObjectBehaviour")) {
            const Props *b = o.behavior("OrbitingObjectBehaviour");
            attachObject(i, idnum(b->get("class")), b->num("radius", 40), 2, b->num("period", 2), 0,
                         b->num("start-angle") * 0.017453293f);
        }
        if (o.has("FinalBossBehaviour")) {
            const Props &bp = *o.behavior("FinalBossBehaviour");
            const char *types[] = {"bat-class",    "beetle-class",       "bird-class",
                                   "fish-class",   "green-jumper-class", "jumper-class",
                                   "spider-class", "turtle-class",       "spike-block-class"};
            for (int t = 0; t < 9; t++)
                for (int n = 0; n < 3; n++) {
                    int before = entityCount;
                    attachObject(i, idnum(bp.get(types[t])), 0, 0, 1, 0, 0);
                    if (entityCount > before) {
                        Entity &spawn = entities[entityCount - 1];
                        spawn.bossSpawn = t + 1;
                        spawn.state.gone = 1;
                        // Boss OnActivate0x1e22d8..0x1e2304 overrides the ordinary
                        // bat orbit with radius40,period4,start-angle270 degrees.
                        if (spawn.orbit) {
                            spawn.radius = 40;
                            spawn.period = 4;
                            spawn.phase = 270 * 0.01745329238474369f;
                        }
                    }
                }
        }
    }
    // Append formerly omitted hidden physical actors after the old ordering,
    // including attachments. Existing checkpoint entity indices stay stable.
    legacyEntityCount = entityCount;
    for (int i = 0; i < current.objectCount; i++) {
        Object &o = current.objects[i];
        if (o.visible || o.layer < 0 || o.layer >= current.layerCount || kindOf(o) == DECOR)
            continue;
        Layer &l = current.layers[o.layer];
        if (l.flags[2] || !l.flags[1] || o.base == 3 || o.base == 4 || o.base == 2)
            continue;
        bool developer = false;
        for (int j = 0; j < current.objectCount; j++)
            if (current.objects[j].layer == o.layer &&
                current.objects[j].has("DeveloperCommentsController"))
                developer = true;
        if (developer)
            continue;
        if (o.base == 7) {
            float w = o.props.num("actlet-width", 32), h = o.props.num("actlet-height", 32);
            if (w < 1 || h < 1)
                continue;
            for (float y = o.rect.b; y < o.rect.t - 0.5f; y += h)
                for (float x = o.rect.l; x < o.rect.r - 0.5f; x += w)
                    addEntity(o, Rect{x, y, minf(x + w, o.rect.r), minf(y + h, o.rect.t)});
        } else
            addEntity(o, o.rect);
    }
    if (playerIndex < 0)
        fatal("No Bean player found in level.");
    memset(bullets, 0, sizeof(bullets));
    resetProjectileAnimationsNative();
    for (int i = 0; i < entityCount; i++) {
        Object &o = *entities[i].source;
        const Props *shot = o.behavior("BulletSource");
        if (shot)
            actor(idnum(shot->get("bulletClass")));
        const Props *boss = o.behavior("FinalBossBehaviour");
        if (boss) {
            actor(idnum(boss->get("bullet-class")));
            const u64 limbs[] = {761998489631ULL, 866681511178ULL, 843976466165ULL,
                                 106556998914888ULL};
            for (int j = 0; j < 4; j++)
                actor(limbs[j]);
        }
    }
    memset(&wizardMotion, 0, sizeof(wizardMotion));
    memset(&wizardCameraBlend, 0, sizeof(wizardCameraBlend));
    wizardArms[0] = wizardArms[1] = wizardArmInitial();
    memset(bulletSourceStates, 0, sizeof(bulletSourceStates));
    seedSourceRandom(timeGetTime());
    resetWizardPresentation();
    resetBulletSourceAnimations();
    haveCheckpointSourceVisuals = false;
    for (int i = 0; i < entityCount; i++) {
        Entity &child = entities[i];
        if (child.motion == 1)
            positionWreckingChild(child);
        else if (child.motion == 2 && child.parent >= 0)
            positionOrbitingChildNative(child, entities[child.parent]);
    }
    configureFrogs(true);
    for (int i = 0; i < entityCount; i++) {
        resetNativeMotion(i);
        Entity &e = entities[i];
        if (e.motion && e.parent >= 0)
            e.state.active = entities[e.parent].state.active;
        initialState[i] = e.state;
        initialMotion[i] = MotionState{e.originX, e.originY, e.turnCooldown, 1, e.lastLaunchSpeed};
        const Props *source = e.source->behavior("BulletSource");
        if (source)
            resetBulletSource(i, *source);
    }
    alignBulletSourcesNative();
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            resetWizardMotion(entities[i], timeGetTime());
    rebuildPhysics();
}

void configureCamera() {
    cameraFollow = 0;
    cameraDefault = Vec{256, 192};
    cameraTarget = playerIndex;
    cameraState.valid = 0;
    for (int i = 0; i < current.objectCount; i++) {
        Object &o = current.objects[i];
        if (o.base == 4)
            cameraDefault = center(o.rect);
        if (o.has("FollowObjectBehaviour")) {
            cameraFollow = o.behavior("FollowObjectBehaviour");
            cameraDefault = center(o.rect);
            break;
        }
    }
    if (cameraFollow) {
        u64 object = idnum(cameraFollow->get("ObjectToFollow")),
            klass = idnum(cameraFollow->get("ClassToFollow"));
        for (int i = 0; i < entityCount; i++)
            if ((object && entities[i].source->id == object) ||
                (klass && entities[i].source->klass == klass)) {
                cameraTarget = i;
                break;
            }
    }
}

float followCameraAxis(float position, float width, float view, float extent, float offset,
                       float *low, float *high) {
    if (position < *low) {
        *low = position;
        *high = position + width;
    }
    if (position > *high) {
        *high = position;
        *low = position - width;
    }
    float centre = (*low + *high) * 0.5f + offset * view;
    return extent < view ? extent * 0.5f : clamp(centre, view * 0.5f, extent - view * 0.5f);
}

void updateCamera(bool snap = false) {
    // FinalBoss combat detaches FollowObject; final death keeps that camera
    // for its full three seconds. Do not advance the follower window either.
    if (!snap && cameraState.valid)
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS && entities[i].state.active == 4)
                return;
    if (!snap && cameraState.valid)
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS && !entities[i].state.gone &&
                entities[i].state.active == 3) {
                // Native combat clears FollowObject's target; no dead-zone update.
                const Entity &boss = entities[i];
                camX = listNumber(boss.source->behavior("FinalBossBehaviour")->get("area-offsets"),
                                  5 - boss.state.hp) -
                       viewW * .5f;
                camY = current.size.y < viewH ? (current.size.y - viewH) * .5f
                                              : clamp(camY, 0, current.size.y - viewH);
                cameraState.x = camX;
                cameraState.y = camY;
                return;
            }
    if (playerIndex < 0)
        return;
    bool reset = snap || !cameraState.valid;
    float dx = cameraFollow ? maxf(0, cameraFollow->num("dead-zone-x")) * viewW : 0,
          dy = cameraFollow ? maxf(0, cameraFollow->num("dead-zone-y")) * viewH : 0;
    if (reset) {
        cameraState.window = Rect{cameraDefault.x - dx * 0.5f, cameraDefault.y - dy * 0.5f,
                                  cameraDefault.x + dx * 0.5f, cameraDefault.y + dy * 0.5f};
        camX = cameraDefault.x - viewW * 0.5f;
        camY = cameraDefault.y - viewH * 0.5f;
        cameraState.valid = 1;
    }
    const State &p =
        entities[cameraTarget >= 0 && cameraTarget < entityCount ? cameraTarget : playerIndex]
            .state;
    float x = cameraFollow && cameraFollow->flag("follow-x", true) ? p.x : camX + viewW * 0.5f,
          y = cameraFollow && cameraFollow->flag("follow-y", true) ? p.y : camY + viewH * 0.5f;
    float offsetX = cameraFollow ? cameraFollow->num("offset-x") : 0,
          offsetY = cameraFollow ? cameraFollow->num("offset-y") : 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS && entities[i].state.active == 5)
            offsetX = offsetY = 0;
    camX = followCameraAxis(x, dx, viewW, current.size.x, offsetX, &cameraState.window.l,
                            &cameraState.window.r) -
           viewW * 0.5f;
    camY = followCameraAxis(y, dy, viewH, current.size.y, offsetY, &cameraState.window.b,
                            &cameraState.window.t) -
           viewH * 0.5f;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS && !entities[i].state.gone && entities[i].state.active == 3) {
            const Entity &boss = entities[i];
            camX = listNumber(boss.source->behavior("FinalBossBehaviour")->get("area-offsets"),
                              5 - boss.state.hp) -
                   viewW * 0.5f;
        }
    cameraState.x = camX;
    cameraState.y = camY;
}

void wizardPostCameraStep(float dt);

// Rebuild the native follow dead zone at load boundaries; helper-specific restore handling is kept explicit.
CameraState cameraLoadStateNative() {
    CameraState result = checkpoint.camera;
    if (!result.valid)
        return result;
    // v21 stores the original Camera centre, independent of window width.
    // Earlier files used view edges; their ordinary follower settles below.
    if (checkpoint.version >= 21) {
        result.x -= viewW * .5f;
        result.y -= viewH * .5f;
    }
    if (!cameraFollow)
        return result;
    float dx = maxf(0, cameraFollow->num("dead-zone-x")) * viewW,
          dy = maxf(0, cameraFollow->num("dead-zone-y")) * viewH;
    Vec oldCentre = cameraState.valid ? Vec{camX + viewW * .5f, camY + viewH * .5f} : cameraDefault;
    result.window = cameraState.valid ? cameraState.window
                                      : Rect{oldCentre.x - dx * .5f, oldCentre.y - dy * .5f,
                                             oldCentre.x + dx * .5f, oldCentre.y + dy * .5f};
    for (int i = 0; i < entityCount && i < checkpoint.count; i++)
        if (entities[i].kind == BOSS &&
            (checkpoint.state[i].active == 3 || checkpoint.state[i].active == 4))
            return result;
    int target = cameraTarget >= 0 && cameraTarget < entityCount ? cameraTarget : playerIndex,
        cameraObject = -1, targetObject = -1;
    for (int i = 0; i < current.objectCount; i++) {
        if (current.objects[i].behavior("FollowObjectBehaviour") == cameraFollow)
            cameraObject = i;
        if (current.objects + i == entities[target].source)
            targetObject = i;
    }
    bool savedFlight = false;
    for (int i = 0; i < entityCount && i < checkpoint.count; i++)
        if (entities[i].kind == HELPER && !checkpoint.state[i].gone &&
            checkpoint.state[i].active == 1)
            savedFlight = true;
    if (savedFlight && target == playerIndex && target < checkpoint.count &&
        checkpoint.version >= 6) {
        // A flight reload must retain its established tracking phase. Centering
        // the window on Bean inserts a fresh half-dead-zone during ascent.
        // The Windows snapshot already has the old boundary; preserve its
        // fractional gap when the viewport changes, without changing physics.
        Rect saved = checkpoint.camera.window;
        const State &bean = checkpoint.state[target];
        result.window = saved;
        float width = saved.r - saved.l, height = saved.t - saved.b;
        if (width > 0) {
            result.window.r = bean.x + (saved.r - bean.x) * (dx / width);
            result.window.l = result.window.r - dx;
        }
        if (height > 0) {
            result.window.t = bean.y + (saved.t - bean.y) * (dy / height);
            result.window.b = result.window.t - dy;
        }
        return result;
    }
    // ActivatePendingObjects appends the resource list to the load list in
    // order (0x268494..49c); DoLoadState walks it (0x26e5e0..600).
    const State &position =
        targetObject >= 0 && targetObject < cameraObject && target < checkpoint.count
            ? checkpoint.state[target]
            : entities[target].state;
    float x = cameraFollow->flag("follow-x", true) ? position.x : oldCentre.x,
          y = cameraFollow->flag("follow-y", true) ? position.y : oldCentre.y;
    result.window = Rect{x - dx * .5f, y - dy * .5f, x + dx * .5f, y + dy * .5f};
    return result;
}

void saveWaterStateNative();
void saveMechanismEventCheckpointNative(State *saved);
void restoreMechanismEventCheckpointNative(const State *saved);

// Capture a coherent task-boundary snapshot, including saved controller phases and stable body state.
void captureCheckpoint(bool disk) {
    checkpointSavePendingNative = false;
    saveWaterStateNative();
    reconcileProjectileAnimationBindingsNative();
    memset(&checkpoint, 0, sizeof(checkpoint));
    checkpoint.magic = 0x43535142;
    checkpoint.version = 28;
    checkpoint.projectileAnimations = projectileAnimationsNative;
    checkpoint.stage = currentStage;
    checkpoint.count = entityCount;
    checkpoint.coins = coins;
    checkpoint.jumps = jumps;
    checkpoint.salamander = salamander;
    checkpoint.bits = alternationBits;
    checkpoint.elapsed = elapsed;
    checkpoint.camera = cameraState;
    checkpoint.camera.x = camX + viewW * .5f;
    checkpoint.camera.y = camY + viewH * .5f;
    memcpy(checkpoint.sources, bulletSourceStates, sizeof(bulletSourceStates));
    checkpoint.sourceRng = sourceRandom;
    checkpoint.wizard = wizardMotion;
    memcpy(checkpoint.arms, wizardArms, sizeof(wizardArms));
    memcpy(checkpoint.diagonal, diagonalRuntime, sizeof(diagonalRuntime));
    memcpy(checkpoint.oscillator, oscillatorRuntime, sizeof(oscillatorRuntime));
    checkpoint.presentation = wizardPresentation;
    checkpoint.bossCamera = wizardCameraBlend;
    checkpoint.bossCamera.fromX += viewW * .5f;
    checkpoint.bossCamera.fromY += viewH * .5f;
    checkpoint.pinball = pinballPendingNative;
    checkpoint.trampoline = trampolineStateNative;
    checkpoint.ordinaryJump = ordinaryJumpStateNative;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        checkpoint.walking[i] = captureWalkNative(e);
        checkpoint.jumperAnimations[i] = e.jumperAnimation;
        checkpoint.fishAnimations[i] = e.fishAnimation;
        checkpoint.helperAnimations[i] = e.helperAnimation;
        checkpoint.state[i] = e.state;
        checkpoint.motion[i] =
            MotionState{e.originX, e.originY, e.turnCooldown, 1, e.lastLaunchSpeed};
    }
    saveMechanismEventCheckpointNative(checkpoint.state);
    memcpy(checkpoint.bullets, bullets, sizeof(checkpoint.bullets));
    memcpy(checkpoint.extraBullets, bullets + 64, sizeof(checkpoint.extraBullets));
    captureShotContacts(checkpoint.shots, checkpoint.extraShots);
    captureProjectileExtraIgnoresNative(checkpoint.projectileIgnores);
    captureBodyPhysicsNative(checkpoint.physics);
    memcpy(checkpoint.sourceVisuals, bulletSourceAnimations, sizeof(bulletSourceAnimations));
    checkpoint.checksum = hashBytes(&checkpoint, sizeof(checkpoint) - 4);
    captureOneWayCheckpointNative(checkpoint.checksum);
    haveCheckpoint = true;
    memcpy(checkpointSourceVisuals, bulletSourceAnimations, sizeof(checkpointSourceVisuals));
    haveCheckpointSourceVisuals = true;
    sourceVisualCheckpointChecksum = checkpoint.checksum;
    // Custom courses keep checkpoints in memory so the campaign save files stay untouched.
    if (disk && !debugMode && !makerRun.active) {
        char p[MAX_PATH];
        savePath(p, "checkpoint.dat");
        if (!atomicWrite(p, &checkpoint, sizeof(checkpoint)) || !writeOneWayCheckpointNative(p)) {
            saveFailed = true;
            tell("Cannot write saves. Move the game to a writable folder.");
        }
        writeProgress();
    }
}

bool checkpointEntitiesCompatible() {
    return (checkpoint.count == entityCount ||
            (checkpoint.version < 11 && checkpoint.count == legacyEntityCount)) &&
           (checkpoint.version < 18 ||
            compatibleProjectileAnimationsNative(checkpoint.projectileAnimations,
                                                 checkpoint.version >= 19));
}

u64 stageMusic() {
    // The final hit stops the boss track before its three-second death scene.
    // The defeated actor stays in the save even after becoming invisible.
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS && entities[i].state.active >= 4)
            return 0;
    return idnum(current.props.get("music"));
}

// Restore actors before rebuilding their bodies and contacts. Never reuse pointers from the previous world.
void restoreCheckpoint() {
    if (!haveCheckpoint || checkpoint.stage != currentStage || !checkpointEntitiesCompatible())
        return;
    TouchLoadRuntimeNative *touchLoadRuntime = captureTouchLoadRuntimeNative();
    checkpointSavePendingNative = false;
    CameraState restoredCamera = cameraLoadStateNative();
    rememberWizardRetryRandom();
    resetEndingScene();
    memset(&goalLaunch, 0, sizeof(goalLaunch));
    if (checkpoint.version >= 8) {
        memcpy(bulletSourceStates, checkpoint.sources, sizeof(bulletSourceStates));
        wizardMotion = checkpoint.wizard;
        memcpy(wizardArms, checkpoint.arms, sizeof(wizardArms));
    } else {
        memset(&wizardMotion, 0, sizeof(wizardMotion));
        memset(&wizardCameraBlend, 0, sizeof(wizardCameraBlend));
        wizardArms[0] = wizardArms[1] = wizardArmInitial();
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->has("BulletSource")) {
                resetBulletSource(i, *entities[i].source->behavior("BulletSource"));
                bulletSourceStates[i].clock = checkpoint.state[i].timer;
            }
    }
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        bool legacyHelper = i >= checkpoint.count ||
                            (checkpoint.version < 6 && (e.kind == HELPER || e.kind == PERCH));
        e.state = legacyHelper ? initialState[i] : checkpoint.state[i];
        if ((e.kind == SWITCH || e.breakable) && decodeMechanismEventAuxNative(e.state.aux) > 0)
            e.state.aux = 0;
        migrateCoinAnimationNative(e, initialState[i]);
        if (checkpoint.version < 8 && e.source->has("ProximityActivateBehaviour") &&
            !e.source->paused)
            e.state.active = 1;
        if (e.kind == WATER) {
            e.state.x = initialState[i].x;
            e.state.y = initialState[i].y;
        }
        MotionState &m = legacyHelper ? initialMotion[i] : checkpoint.motion[i];
        e.originX = m.valid ? m.originX : initialMotion[i].originX;
        e.originY = m.valid ? m.originY : initialMotion[i].originY;
        e.turnCooldown = m.valid ? m.turnCooldown : 0;
        e.lastLaunchSpeed = m.valid && m.lastLaunchSpeed > 0 ? m.lastLaunchSpeed : e.jumpPower;
        if (checkpoint.version < 6) {
            if (e.source->has("FrogSpecialBallBehaviour"))
                e.state.hp = 0;
            if (e.source->has("EnemyJumperBehaviour")) {
                e.state.hp = 0;
                e.state.aux = 0;
            }
        }
    }
    if (checkpoint.version >= 9) {
        memcpy(diagonalRuntime, checkpoint.diagonal, sizeof(diagonalRuntime));
        memcpy(oscillatorRuntime, checkpoint.oscillator, sizeof(oscillatorRuntime));
        wizardPresentation = checkpoint.presentation;
    } else {
        for (int i = 0; i < entityCount; i++)
            resetNativeMotion(i, true);
        resetWizardPresentation();
    }
    for (int i = checkpoint.count; i < entityCount; i++)
        resetNativeMotion(i, true);
    wizardCameraBlend = checkpoint.version >= 11 ? checkpoint.bossCamera : WizardCameraBlendState{};
    if (checkpoint.version >= 21) {
        wizardCameraBlend.fromX -= viewW * .5f;
        wizardCameraBlend.fromY -= viewH * .5f;
    }
    if (checkpoint.version < 11 && wizardMotion.cycleCount == 0 && wizardMotion.cycleAge >= 2.7f &&
        wizardMotion.cycleAge < 4) {
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS && entities[i].state.active == 3) {
                wizardMotion.leftAge = wizardMotion.rightAge = -1;
                break;
            }
    }
    memcpy(bullets, checkpoint.bullets, sizeof(checkpoint.bullets));
    memcpy(bullets + 64, checkpoint.extraBullets, sizeof(checkpoint.extraBullets));
    coins = checkpoint.coins;
    jumps = checkpoint.jumps;
    entities[playerIndex].state.hp =
        readyNativeRestoreCountLatch(entities[playerIndex].state.hp, entities[playerIndex].state.x,
                                     entities[playerIndex].originX, checkpoint.jumps);
    salamander = checkpoint.salamander;
    alternationBits = checkpoint.bits;
    if (checkpoint.version < 10 && current.props.num("bean-world") == 6)
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS) {
                const State &boss = entities[i].state;
                alternationBits = migrateLegacyWizardArenaBits(alternationBits, boss.hp,
                                                               boss.active, boss.gone != 0);
                break;
            }
    if (checkpoint.version < 11)
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == SPIKE)
                resetSpikeAnimation(entities[i], alternationBits);
    elapsed = checkpoint.elapsed;
    nativeContinuousRebaseScene(current, elapsed);
    deathTimer = deathAge = winTimer = readyTimer = 0;
    playerInWater = false;
    clearEffects();
    complete = false;
    screen = PLAY;
    if (checkpoint.version < 8)
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS) {
                resetWizardMotion(entities[i], timeGetTime());
                wizardMotion.initialBeanX = initialState[playerIndex].x;
            }
    if (checkpoint.version >= 12)
        memcpy(bulletSourceAnimations, checkpoint.sourceVisuals, sizeof(bulletSourceAnimations));
    else if (haveCheckpointSourceVisuals && sourceVisualCheckpointChecksum == checkpoint.checksum)
        memcpy(bulletSourceAnimations, checkpointSourceVisuals, sizeof(bulletSourceAnimations));
    else {
        resetBulletSourceAnimations();
        for (int i = 0; i < entityCount; i++)
            if (entities[i].source->has("BulletSource"))
                reconstructBulletSourceAnimationFromClock(i);
    }
    for (int i = 0; i < entityCount; i++) {
        if (checkpoint.version >= 15 && i < checkpoint.count)
            entities[i].jumperAnimation = checkpoint.jumperAnimations[i];
        else
            restoreLegacyJumperAnimationNative(entities[i]);
    }
    for (int i = 0; i < entityCount; i++) {
        if (checkpoint.version >= 16 && i < checkpoint.count)
            entities[i].fishAnimation = checkpoint.fishAnimations[i];
        else
            restoreLegacyFishAnimationNative(entities[i]);
    }
    for (int i = 0; i < entityCount; i++) {
        if (checkpoint.version >= 22 && i < checkpoint.count)
            entities[i].helperAnimation = checkpoint.helperAnimations[i];
        else
            restoreLegacyHelperAnimationNative(entities[i]);
    }
    alignBulletSourcesNative(true);
    if (checkpoint.version >= 19)
        projectileAnimationsNative = checkpoint.projectileAnimations;
    else if (checkpoint.version == 18)
        restoreVersion18ProjectileAnimationsNative(checkpoint.projectileAnimations);
    else {
        resetProjectileAnimationsNative();
        reconcileProjectileAnimationBindingsNative();
    }
    for (int i = 0; i < entityCount; i++)
        if (entities[i].walk) {
            Entity &e = entities[i];
            if (checkpoint.version >= 24 && i < checkpoint.count)
                restoreWalkNative(e, checkpoint.walking[i]);
            else {
                bool right = e.state.aux >= 0;
                resetWalkNative(e);
                e.walkRight = right;
                e.state.aux = right ? e.speed : -e.speed;
            }
        }
    retainWizardRetryRandom();
    playerWalkMode = false;
    int potionOrder = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == POTION && entities[i].state.gone) {
            bool custom = *entities[i].source->props.get("maker-potion-form") != 0;
            int order = custom ? entities[i].state.active : 0;
            if (order > potionOrder) {
                potionOrder = order;
                playerWalkMode = !entities[i].source->props.flag("maker-potion-form");
            }
        }
    restorePhysics();
    restoreTouchLoadRuntimeNative(touchLoadRuntime);
    restoreMechanismEventCheckpointNative(checkpoint.state);
    ordinaryJumpStateNative =
        checkpoint.version >= 23 ? checkpoint.ordinaryJump : OrdinaryJumpStateNative{};
    if (checkpoint.version >= 20)
        restoreBodyPhysicsNative(checkpoint.physics);
    if (checkpoint.version < 25)
        restoreLegacyWalkerGravityNative();
    if (checkpoint.version < 26)
        restoreLegacyJumperGravityNative();
    if (checkpoint.version < 27)
        restoreLegacyRotorGravityNative();
    if (checkpoint.version < 28)
        restoreLegacyFreeEnemyGravityNative();
    if (checkpoint.version >= 13)
        pinballPendingNative = checkpoint.pinball;
    if (checkpoint.version >= 14)
        trampolineStateNative = checkpoint.trampoline;
    if (checkpoint.version >= 12)
        restoreShotContacts(checkpoint.shots, checkpoint.extraShots);
    if (checkpoint.version >= 19)
        restoreProjectileExtraIgnoresNative(checkpoint.projectileIgnores);
    restoreOneWayCheckpointNative(checkpoint.checksum);
    if (restoredCamera.valid) {
        cameraState = restoredCamera;
        camX = cameraState.x;
        camY = cameraState.y;
        updateCamera();
    } else
        updateCamera(true);
    wizardPostCameraStep(0);
    checkpointRestoreAnimations();
    playMusic(stageMusic());
}

// Load authored stage resources and reset scene-owned state before creating the new campaign world.
void startStage(int stage, bool resume = false) {
    checkpointSavePendingNative = false;
    resetBurstRandomNative(0, timeGetTime());
    respawnCancel(respawnTransition);
    memset(&goalLaunch, 0, sizeof(goalLaunch));
    clearEffects();
    playerInWater = false;
    deathAge = 0;
    currentStage = (int)clamp((float)stage, 0, (float)stageCount - 1);
    if (makerRun.active)
        makerLoadCurrentLevel();
    else
        loadLevelData(levels[stageList[currentStage]].file);
    stageJumpTarget = jumpTargetForStage(currentStage);
    alternationBits = initialAlternation();
    buildEntities();
    coins = jumps = salamander = 0;
    alternationBits = initialAlternation();
    elapsed = deathTimer = winTimer = readyTimer = 0;
    complete = false;
    screen = PLAY;
    noticeTimer = 0;
    nativeContinuousResetScene(current);
    nativeContinuousAdvanceScene(current, 1, 0, 0);
    configureCamera();
    if (resume && haveCheckpoint && checkpoint.stage == currentStage &&
        checkpointEntitiesCompatible())
        restoreCheckpoint();
    else {
        updateCamera(true);
        captureCheckpoint(true);
    }
    playMusic(stageMusic());
    if (!makerRun.active)
        writeProgress();
    if (interfaceEnabled)
        stageInterface(resume);
}

void restartStage() {
    TouchLoadRuntimeNative *touchLoadRuntime = captureTouchLoadRuntimeNative();
    checkpointSavePendingNative = false;
    stageJumpTarget = jumpTargetForStage(currentStage);
    rememberWizardRetryRandom();
    resetEndingScene();
    resetWizardPresentation();
    memset(&goalLaunch, 0, sizeof(goalLaunch));
    memset(&wizardMotion, 0, sizeof(wizardMotion));
    memset(&wizardCameraBlend, 0, sizeof(wizardCameraBlend));
    wizardArms[0] = wizardArms[1] = wizardArmInitial();
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->has("BulletSource"))
            resetBulletSource(i, *entities[i].source->behavior("BulletSource"));
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        e.state = initialState[i];
        resetWalkNative(e);
        e.jumperAnimation = JumperAnimationNative{};
        e.fishAnimation = FishAnimationNative{};
        e.helperAnimation = HelperAnimationNative{};
        e.originX = initialMotion[i].originX;
        e.originY = initialMotion[i].originY;
        e.turnCooldown = initialMotion[i].turnCooldown;
        e.lastLaunchSpeed = initialMotion[i].lastLaunchSpeed;
        resetNativeMotion(i);
    }
    alignBulletSourcesNative();
    memset(bullets, 0, sizeof(bullets));
    resetProjectileAnimationsNative();
    clearEffects();
    playerInWater = false;
    deathAge = 0;
    coins = jumps = salamander = 0;
    alternationBits = initialAlternation();
    elapsed = deathTimer = winTimer = readyTimer = 0;
    nativeContinuousRebaseScene(current, 0);
    complete = false;
    screen = PLAY;
    restorePhysics();
    restoreTouchLoadRuntimeNative(touchLoadRuntime);
    configureCamera();
    updateCamera(true);
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            resetWizardMotion(entities[i], timeGetTime());
    retainWizardRetryRandom();
    captureCheckpoint(true);
    playMusic(stageMusic());
    if (interfaceEnabled)
        stageInterface(false);
}

void die() {
    if (deathTimer > 0 || complete)
        return;
    deathTimer = deathDuration;
    deathAge = 0;
    Entity &p = entities[playerIndex];
    deathX = p.state.x;
    deathY = p.state.y;
    deaths++;
    entityEffect(p, "TomPlayerBehaviour", "explosion-actor");
    respawnBegin(respawnTransition, RESPAWN_DEATH);
}

// Queue the existing killable-enemy death lifecycle; callers may run while contact events are being consumed.
void killEnemy(Entity &e) {
    if (e.state.gone)
        return;
    e.state.gone = 2;
    e.turnCooldown = 0;
    sound("enemyexplosion");
}

void advanceKillableDeathNative(Entity &e, float dt, int domain = 0) {
    if (!killableDeathVisibleNative(e) || dt <= 0)
        return;
    e.turnCooldown += dt;
    e.state.timer += dt;
    advanceJumperAnimationNative(e, dt);
    advanceFishAnimationNative(e, dt);
    // OnUpdate0x23b974 uses a strict greater-than comparison: at exactly
    // die-time the actor still has its terminal half-width/zero-height pose.
    if (killableDeathAgeNative(e) > killableDeathDurationNative(e)) {
        e.state.gone = 1;
        e.turnCooldown = 0;
        entityEffect(e, "KillableEnemyBehaviour", "explosion-actor", domain);
        // All authored UI Killables have alternate-on-death0. A UI-world
        // death must never dispatch the campaign alternation listeners.
        const Props *k = e.source->behavior("KillableEnemyBehaviour");
        if (!domain && k && k->flag("alternate-on-death"))
            alternateScene((int)k->num("alternation-mask"));
    }
}

void finish() {
    if (complete || deathTimer > 0)
        return;
    complete = true;
    winTimer = 0;
    screen = COMPLETE;
    beginGoalLaunch(entities[playerIndex]);
    deactivateGoalBodies();
    if (interfaceEnabled)
        completeInterface();
    if (makerRun.active)
        makerRecordClear();
    else {
        progress.cleared[currentStage] = 1;
        progress.bestCoins[currentStage] =
            (int)maxf((float)progress.bestCoins[currentStage], (float)coins);
        if (progress.bestJumps[currentStage] == 0 || jumps < progress.bestJumps[currentStage])
            progress.bestJumps[currentStage] = jumps;
        progress.axolotls[currentStage] |= salamander;
        progress.unlocked = (int)maxf((float)progress.unlocked,
                                      minf((float)currentStage + 1, (float)stageCount - 1));
        writeProgress();
    }
    stopMusic();
    sound("fanfare");
    sound("bean-win");
}

void shoot(Entity &turret, const Props &props) {
    u64 klass = idnum(props.get("bulletClass"));
    int index = -1;
    for (int i = 0; i < classCount; i++)
        if (classes[i].id == klass) {
            index = i;
            break;
        }
    if (index < 0)
        return;
    BulletSourceState &aim = bulletSourceStates[(int)(&turret - entities)];
    if (aim.initialized < 2)
        alignBulletSourceAimNative(turret, props, aim, sourceTargetNative(props));
    float angle = aim.angle;
    const Props *bp = classes[index].behavior("EnemyBulletBehaviour");
    // Course-maker sources may override the bullet class speed; authored sources never set it.
    float speed = props.num("bulletSpeed", bp ? bp->num("speed", 125) : 125);
    int owner = (int)(&turret - entities) + 1, active = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (bullets[i].life > 0 && bullets[i].state.active == owner)
            active++;
    const char *extras = props.get("extraShots");
    int ordinal = 0;
    for (;;) {
        float offset = ordinal ? number(extras) : 0;
        float radians = angle + offset * 0.017453293f;
        if (active < props.num("bulletPoolSize", 10))
            for (int i = 0; i < MAX_PROJECTILES; i++)
                if (bullets[i].life <= 0) {
                    int member = freeProjectileAnimationNative(owner - 1, index);
                    if (member < 0)
                        break;
                    Bullet &b = bullets[i];
                    memset(&b, 0, sizeof(b));
                    projectileAnimationsNative.binding[i] = member;
                    b.life = 1;
                    b.klass = index;
                    b.state.active = owner;
                    resetProjectileLaunchContactsNative(i, owner - 1);
                    b.state.x = turret.state.x;
                    b.state.y = turret.state.y;
                    b.state.vx = bqCos(radians) * speed;
                    b.state.vy = bqSin(radians) * speed;
                    actor(klass);
                    active++;
                    break;
                }
        if (ordinal) {
            while (*extras && *extras != ',')
                extras++;
            if (*extras)
                extras++;
        }
        if (!*extras)
            break;
        ordinal++;
    }
}

#include "physics.h"
