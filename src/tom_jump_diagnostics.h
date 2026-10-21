#pragma once

// Shared tom jump diagnostics behavior used by the Windows reconstruction.
#if BQ_ENABLE_TESTS

// Diagnostic-only causal history. Only the main physics thread writes this
// bounded ring. It neither changes a body/voice nor calls an output device.
enum { TOM_JUMP_DIAGNOSTIC_CAPACITY = 2048 };

struct TomJumpDiagnostic {
    u32 sequence, step, tick_ms;
    int simulation_ms;
    u64 stage_id;
    int x100, y100, before_vy100, prestep_vy100, after_vy100, dt_us;
    int solved, eligible, head, normal_y1000, contact_y100, contact_owner;
    int gate_mask, ground, support, spring, context_flags, boss_mode, input_mask;
    int branch_entered, launch_committed, launch100, exit_branch;
    u32 sound_sequence_before, sound_sequence_after, jump_requests, rejected_requests;
};

TomJumpDiagnostic tomJumpDiagnostics[TOM_JUMP_DIAGNOSTIC_CAPACITY], tomJumpDiagnosticCurrent,
    tomJumpDiagnosticLast;
u32 tomJumpDiagnosticSequence, tomJumpDiagnosticStep;
float tomJumpDiagnosticAge, tomJumpDiagnosticLastAge;
bool tomJumpDiagnosticActive;

void resetTomJumpDiagnostics() {
    memset(tomJumpDiagnostics, 0, sizeof(tomJumpDiagnostics));
    memset(&tomJumpDiagnosticCurrent, 0, sizeof(tomJumpDiagnosticCurrent));
    memset(&tomJumpDiagnosticLast, 0, sizeof(tomJumpDiagnosticLast));
    tomJumpDiagnosticSequence = tomJumpDiagnosticStep = 0;
    tomJumpDiagnosticAge = tomJumpDiagnosticLastAge = 0;
    tomJumpDiagnosticActive = false;
}

void tomJumpDiagnosticBegin(float dt, bool left, bool right) {
    memset(&tomJumpDiagnosticCurrent, 0, sizeof(tomJumpDiagnosticCurrent));
    tomJumpDiagnosticActive = true;
    tomJumpDiagnosticAge += dt;
    TomJumpDiagnostic &d = tomJumpDiagnosticCurrent;
    d.step = ++tomJumpDiagnosticStep;
    d.tick_ms = GetTickCount();
    d.simulation_ms = (int)(tomJumpDiagnosticAge * 1000);
    d.stage_id =
        currentStage >= 0 && currentStage < stageCount ? levels[stageList[currentStage]].id : 0;
    d.before_vy100 = (int)(entities[playerIndex].state.vy * 100);
    d.dt_us = (int)(dt * 1000000);
    d.sound_sequence_before = audioJumpTraceSequence;
    d.input_mask = (left ? 1 : 0) | (right ? 2 : 0);
    d.support = d.spring = d.contact_owner = -1;
}

void tomJumpDiagnosticPostSolve(b2Contact *contact, float incomingVy) {
    if (!tomJumpDiagnosticActive || playerIndex < 0)
        return;
    b2Fixture *fixtures[] = {contact->GetFixtureA(), contact->GetFixtureB()};
    int side = fixtures[0]->GetBody() == bodies[playerIndex]   ? 0
               : fixtures[1]->GetBody() == bodies[playerIndex] ? 1
                                                               : -1;
    if (side < 0)
        return;
    b2WorldManifold m;
    contact->GetWorldManifold(&m);
    float ny = side == 0 ? -m.normal.y : m.normal.y;
    TomJumpDiagnostic &d = tomJumpDiagnosticCurrent;
    d.solved++;
    if (ny < -.5f)
        d.head++;
    if (nativeTomCanAutoJump(incomingVy, ny)) {
        d.eligible++;
        d.normal_y1000 = (int)(ny * 1000);
        d.contact_y100 = (int)(m.points[0].y / metres * 100);
        d.contact_owner = (int)fixtures[1 - side]->GetBody()->GetUserData() - 1;
    }
}

void tomJumpDiagnosticDecision() {
    if (!tomJumpDiagnosticActive)
        return;
    TomJumpDiagnostic &d = tomJumpDiagnosticCurrent;
    d.prestep_vy100 = (int)(preStepVy * 100);
    d.ground = onGround;
    d.support = supportEntity;
    d.spring = playerSpringContact;
    d.gate_mask = (deathTimer <= 0 ? 1 : 0) |
                  ((!playerWalkMode || playerSpringContact >= 0) ? 2 : 0) |
                  (((onGround && preStepVy <= 0) || playerSpringContact >= 0) ? 4 : 0) |
                  (!killablePlayerStompedThisStep() ? 8 : 0);
    d.context_flags = (debugMode ? 1 : 0) | (playerWalkMode ? 2 : 0) |
                      (endingScene.active ? 4 : 0) | (deathTimer > 0 ? 8 : 0) |
                      (playerMagnetContact ? 16 : 0) | (killablePlayerStompedThisStep() ? 32 : 0) |
                      (soundEnabled ? 64 : 0) | (audioInitialized ? 128 : 0) |
                      (readyTimer > 0 ? 256 : 0) | (respawnBusy(respawnTransition) ? 512 : 0) |
                      ((pinballPlayerStepNative.pending || pinballLaunchedThisStepNative ||
                        springPlayerContactThisStepNative || trampolineContactThisStepNative ||
                        trampolineLaunchedThisStepNative)
                           ? 1024
                           : 0);
}

void tomJumpDiagnosticBranch(float launch, bool exit = false) {
    if (!tomJumpDiagnosticActive)
        return;
    tomJumpDiagnosticCurrent.branch_entered = 1;
    tomJumpDiagnosticCurrent.launch100 = (int)(launch * 100);
    tomJumpDiagnosticCurrent.exit_branch = exit;
}

void tomJumpDiagnosticCommit() {
    if (tomJumpDiagnosticActive)
        tomJumpDiagnosticCurrent.launch_committed = 1;
}

void tomJumpDiagnosticEnd() {
    if (!tomJumpDiagnosticActive)
        return;
    tomJumpDiagnosticActive = false;
    TomJumpDiagnostic &d = tomJumpDiagnosticCurrent;
    const State &bean = entities[playerIndex].state;
    d.x100 = (int)(bean.x * 100);
    d.y100 = (int)(bean.y * 100);
    d.after_vy100 = (int)(bean.vy * 100);
    d.sound_sequence_after = audioJumpTraceSequence;
    d.jump_requests = d.sound_sequence_after - d.sound_sequence_before;
    for (u32 n = 0; n < d.jump_requests && n < TOM_JUMP_DIAGNOSTIC_CAPACITY; n++) {
        u32 sequence = d.sound_sequence_before + n + 1;
        AudioJumpTrace &trace = audioJumpTraces[(sequence - 1) & (AUDIO_JUMP_TRACE_CAPACITY - 1)];
        if (trace.sequence == sequence && trace.rejectReason)
            d.rejected_requests++;
    }
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS) {
            d.boss_mode = entities[i].state.active;
            break;
        }
    bool pulse = d.after_vy100 > 3000 && d.after_vy100 - d.before_vy100 > 5000;
    bool unexplained = d.eligible && !d.launch_committed && !d.exit_branch &&
                       !(d.context_flags & (2 | 4 | 8 | 16 | 32 | 512 | 1024));
    bool critical = d.launch_committed || d.jump_requests || pulse || unexplained;
    if (!critical && !d.solved && !d.head)
        return;
    bool changed = !tomJumpDiagnosticSequence || d.stage_id != tomJumpDiagnosticLast.stage_id ||
                   d.gate_mask != tomJumpDiagnosticLast.gate_mask ||
                   d.support != tomJumpDiagnosticLast.support ||
                   d.context_flags != tomJumpDiagnosticLast.context_flags ||
                   d.contact_owner != tomJumpDiagnosticLast.contact_owner ||
                   d.head != tomJumpDiagnosticLast.head ||
                   d.eligible != tomJumpDiagnosticLast.eligible;
    if (!critical && !changed && tomJumpDiagnosticAge - tomJumpDiagnosticLastAge < 1)
        return;
    d.sequence = ++tomJumpDiagnosticSequence;
    tomJumpDiagnostics[(d.sequence - 1) & (TOM_JUMP_DIAGNOSTIC_CAPACITY - 1)] = d;
    tomJumpDiagnosticLast = d;
    tomJumpDiagnosticLastAge = tomJumpDiagnosticAge;
}

bool flushTomJumpDiagnostics(const char *path = "test-output/live-jump-physics.csv") {
    CreateDirectoryA("test-output", 0);
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const char *header =
        "sequence,step,tick_ms,simulation_ms,stage_id,x100,y100,before_vy100,prestep_vy100,after_vy100,dt_us,solved,eligible,head,normal_y1000,contact_y100,contact_owner,gate_mask,ground,support,spring,context_flags,boss_mode,input_mask,branch_entered,launch_committed,launch100,exit_branch,sound_sequence_before,sound_sequence_after,jump_requests,rejected_requests\r\n";
    DWORD done;
    bool ok =
        WriteFile(file, header, lstrlenA(header), &done, 0) && done == (DWORD)lstrlenA(header);
    u32 count = tomJumpDiagnosticSequence < TOM_JUMP_DIAGNOSTIC_CAPACITY
                    ? tomJumpDiagnosticSequence
                    : TOM_JUMP_DIAGNOSTIC_CAPACITY,
        first = tomJumpDiagnosticSequence - count + 1;
    for (u32 n = 0; n < count && ok; n++) {
        u32 sequence = first + n;
        TomJumpDiagnostic &d =
            tomJumpDiagnostics[(sequence - 1) & (TOM_JUMP_DIAGNOSTIC_CAPACITY - 1)];
        if (d.sequence != sequence)
            continue;
        char stage[24], row[640];
        idstr(d.stage_id, stage);
        int length = wsprintfA(
            row,
            "%u,%u,%u,%u,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u\r\n",
            d.sequence, d.step, d.tick_ms, d.simulation_ms, stage, d.x100, d.y100, d.before_vy100,
            d.prestep_vy100, d.after_vy100, d.dt_us, d.solved, d.eligible, d.head, d.normal_y1000,
            d.contact_y100, d.contact_owner, d.gate_mask, d.ground, d.support, d.spring,
            d.context_flags, d.boss_mode, d.input_mask, d.branch_entered, d.launch_committed,
            d.launch100, d.exit_branch, d.sound_sequence_before, d.sound_sequence_after,
            d.jump_requests, d.rejected_requests);
        ok = WriteFile(file, row, length, &done, 0) && done == (DWORD)length;
    }
    CloseHandle(file);
    return ok;
}

#else
inline void tomJumpDiagnosticBegin(float, bool, bool) {}

inline void tomJumpDiagnosticPostSolve(b2Contact *, float) {}

inline void tomJumpDiagnosticDecision() {}

inline void tomJumpDiagnosticBranch(float, bool = false) {}

inline void tomJumpDiagnosticCommit() {}

inline void tomJumpDiagnosticEnd() {}
#endif
