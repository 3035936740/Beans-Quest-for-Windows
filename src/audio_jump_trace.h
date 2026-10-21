#pragma once

// Test-only jump audio event tracing; release builds contain only no-op hooks.
enum AudioJumpReject {
    AUDIO_JUMP_ACCEPTED = 0,
    AUDIO_JUMP_DISABLED = 1,
    AUDIO_JUMP_UNINITIALIZED = 2,
    AUDIO_JUMP_CLIP_MISSING = 3,
    AUDIO_JUMP_OBJECT_MUTED = 4
};

enum AudioJumpFinish {
    AUDIO_JUMP_RUNNING = 0,
    AUDIO_JUMP_COMPLETE = 1,
    AUDIO_JUMP_STOPPED = 2,
    AUDIO_JUMP_PREEMPTED = 3,
    AUDIO_JUMP_SHUTDOWN = 4,
    AUDIO_JUMP_REJECTED = 5
};

#if BQ_ENABLE_TESTS

// Diagnostic-only history. The mixer never opens or writes a file. Call the
// flush on the main thread after shutdownAudio; context also runs only on the
// main-thread sound("jump") call, never on the audio worker.
enum { AUDIO_JUMP_TRACE_CAPACITY = 2048 };

struct AudioJumpTrace {
    u32 sequence, tick, endTick;
    u64 stageId;
    int x100, y100, jumps, scene, contextFlags;
    int rejectReason, slot;
    u32 voiceSerial, clipFrames, mixedFrames, nonzeroFrames;
    int finishReason;
    u32 waveOutError, lastWaveOutError;
};

typedef void (*AudioJumpTraceContextFn)(AudioJumpTrace &);
AudioJumpTraceContextFn audioJumpTraceContext;
AudioJumpTrace audioJumpTraces[AUDIO_JUMP_TRACE_CAPACITY];
u32 audioJumpTraceSequence, audioJumpVoiceTrace[32];

AudioJumpTrace *audioJumpTraceForVoice(int slot) {
    u32 sequence = audioJumpVoiceTrace[slot];
    if (!sequence)
        return 0;
    AudioJumpTrace &trace = audioJumpTraces[(sequence - 1) & (AUDIO_JUMP_TRACE_CAPACITY - 1)];
    return trace.sequence == sequence ? &trace : 0;
}

AudioJumpTrace &audioJumpTraceBegin(int rejectReason) {
    u32 sequence = ++audioJumpTraceSequence;
    if (!sequence)
        sequence = ++audioJumpTraceSequence;
    AudioJumpTrace &trace = audioJumpTraces[(sequence - 1) & (AUDIO_JUMP_TRACE_CAPACITY - 1)];
    memset(&trace, 0, sizeof(trace));
    trace.sequence = sequence;
    trace.tick = GetTickCount();
    trace.rejectReason = rejectReason;
    trace.slot = -1;
    trace.waveOutError = trace.lastWaveOutError = (u32)audioError;
    if (rejectReason) {
        trace.endTick = trace.tick;
        trace.finishReason = AUDIO_JUMP_REJECTED;
    }
    if (audioJumpTraceContext)
        audioJumpTraceContext(trace);
    return trace;
}

void audioJumpTraceFinishVoice(int slot, int reason) {
    AudioJumpTrace *trace = audioJumpTraceForVoice(slot);
    if (trace && trace->finishReason == AUDIO_JUMP_RUNNING) {
        trace->finishReason = reason;
        trace->endTick = GetTickCount();
        trace->lastWaveOutError = (u32)audioError;
    }
    audioJumpVoiceTrace[slot] = 0;
}

void audioJumpTraceMixedFrame(int slot, int left, int right) {
    AudioJumpTrace *trace = audioJumpTraceForVoice(slot);
    if (!trace)
        return;
    trace->mixedFrames++;
    if (left || right)
        trace->nonzeroFrames++;
}

void resetAudioJumpTrace() {
    if (audioInitialized)
        EnterCriticalSection(&audioLock);
    memset(audioJumpTraces, 0, sizeof(audioJumpTraces));
    memset(audioJumpVoiceTrace, 0, sizeof(audioJumpVoiceTrace));
    audioJumpTraceSequence = 0;
    if (audioInitialized)
        LeaveCriticalSection(&audioLock);
}

bool flushAudioJumpTrace(const char *path = "test-output/live-jump-audio.csv") {
    // Shutdown joins the worker first. Refuse an active worker rather than
    // holding up real-time mixing with diagnostic file I/O.
    if (audioRunning)
        return false;
    CreateDirectoryA("test-output", 0);
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const char *header =
        "sequence,tick_ms,end_tick_ms,stage_id,x100,y100,jumps,scene,context_flags,reject_reason,slot,voice_serial,clip_frames,mixed_frames,nonzero_frames,finish_reason,waveout_error,last_waveout_error\r\n";
    DWORD written;
    bool ok = WriteFile(file, header, lstrlenA(header), &written, 0) &&
              written == (DWORD)lstrlenA(header);
    u32 count = audioJumpTraceSequence < AUDIO_JUMP_TRACE_CAPACITY ? audioJumpTraceSequence
                                                                   : AUDIO_JUMP_TRACE_CAPACITY;
    u32 first = audioJumpTraceSequence - count + 1;
    for (u32 n = 0; n < count && ok; n++) {
        u32 sequence = first + n;
        const AudioJumpTrace &trace =
            audioJumpTraces[(sequence - 1) & (AUDIO_JUMP_TRACE_CAPACITY - 1)];
        if (trace.sequence != sequence)
            continue;
        char stage[24], line[512];
        idstr(trace.stageId, stage);
        int length =
            wsprintfA(line, "%u,%u,%u,%s,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%d,%u,%u\r\n",
                      trace.sequence, trace.tick, trace.endTick, stage, trace.x100, trace.y100,
                      trace.jumps, trace.scene, trace.contextFlags, trace.rejectReason, trace.slot,
                      trace.voiceSerial, trace.clipFrames, trace.mixedFrames, trace.nonzeroFrames,
                      trace.finishReason, trace.waveOutError, trace.lastWaveOutError);
        ok = WriteFile(file, line, length, &written, 0) && written == (DWORD)length;
    }
    CloseHandle(file);
    return ok;
}

void audioJumpTraceRejected(const char *name, int reason) {
    if (!eq(name, "jump"))
        return;
    if (audioInitialized)
        EnterCriticalSection(&audioLock);
    audioJumpTraceBegin(reason);
    if (audioInitialized)
        LeaveCriticalSection(&audioLock);
}

void audioJumpTraceStarted(const char *name, int slot, u32 serial, u32 frames) {
    if (!eq(name, "jump"))
        return;
    AudioJumpTrace &trace = audioJumpTraceBegin(AUDIO_JUMP_ACCEPTED);
    trace.slot = slot;
    trace.voiceSerial = serial;
    trace.clipFrames = frames;
    audioJumpVoiceTrace[slot] = trace.sequence;
}
#else
inline void audioJumpTraceRejected(const char *, int) {}

inline void audioJumpTraceStarted(const char *, int, u32, u32) {}

inline void audioJumpTraceFinishVoice(int, int) {}

inline void audioJumpTraceMixedFrame(int, int, int) {}
#endif
