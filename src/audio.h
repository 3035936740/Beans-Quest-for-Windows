#pragma once

// Caches decoded clips and mixes concurrent effects and music through the XP-compatible waveOut device.
#include "platform.h"

// One XP-compatible output device mixes music and concurrent, cached effects.
// No file opens, allocations or PlaySound restarts occur on a gameplay event.
struct AudioClip {
    u8 *bytes;
    short *pcm;
    u32 frames;
};

struct SoundClip {
    const char *name;
    AudioClip clip;
};

struct AudioVoice {
    AudioClip *clip;
    u32 position, serial, fraction, rate;
    float gain;
};

SoundClip soundClips[32];
int soundClipCount;
AudioVoice audioVoices[32];
AudioClip musicClip, victoryClip;
bool musicOwned;
HWAVEOUT musicDevice;
u64 playingMusic;
u32 musicPosition, voiceSerial;
bool musicEnabled = true, soundEnabled = true, audioInitialized;
volatile bool audioRunning;
// Main-thread event filter for native Object::SetMuted. The audio worker
// must keep mixing accepted voices and silence compatibility independently.
bool objectSoundsMutedNative;

struct ScopedObjectSoundMuteNative {
    bool previous;

    ScopedObjectSoundMuteNative() : previous(objectSoundsMutedNative) {
        objectSoundsMutedNative = true;
    }

    ~ScopedObjectSoundMuteNative() {
        objectSoundsMutedNative = previous;
    }
};

float musicGain = 1;
CRITICAL_SECTION audioLock;
HANDLE audioEvent, audioThread;

enum { AUDIO_FRAMES = 512, AUDIO_BUFFERS = 4 };

short audioBuffers[AUDIO_BUFFERS][AUDIO_FRAMES * 2];
WAVEHDR audioHeaders[AUDIO_BUFFERS];
u32 audioFileReads, audioVoicesStarted, audioSamplesMixed, audioNonzeroSamples;
MMRESULT audioError;
#include "audio_jump_trace.h"
#include "audio_silence_keepalive.h"

bool loadAudioClip(const char *name, AudioClip &clip) {
    char path[128];
    lstrcpyA(path, "data/audio/");
    lstrcatA(path, name);
    lstrcatA(path, ".wav");
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD size = GetFileSize(file, 0), done = 0;
    audioFileReads++;
    if (size < 44 || size > 20000000) {
        CloseHandle(file);
        return false;
    }
    u8 *bytes = (u8 *)HeapAlloc(GetProcessHeap(), 0, size);
    if (!bytes) {
        CloseHandle(file);
        return false;
    }
    bool ok = ReadFile(file, bytes, size, &done, 0) && done == size;
    CloseHandle(file);
    WAVEFORMATEX format = {};
    short *pcm = 0;
    u32 length = 0;
    if (ok && *(u32 *)bytes == 0x46464952 && *(u32 *)(bytes + 8) == 0x45564157) {
        for (u32 p = 12; p + 8 <= size;) {
            u32 n = *(u32 *)(bytes + p + 4);
            if (n > size - p - 8)
                break;
            if (*(u32 *)(bytes + p) == 0x20746d66 && n >= 16)
                memcpy(&format, bytes + p + 8, 16);
            if (*(u32 *)(bytes + p) == 0x61746164) {
                pcm = (short *)(bytes + p + 8);
                length = n;
            }
            p += 8 + n + (n & 1);
        }
    }
    if (!pcm || !length || length % 4 || format.wFormatTag != WAVE_FORMAT_PCM ||
        format.nChannels != 2 || format.wBitsPerSample != 16 || format.nSamplesPerSec != 22050) {
        HeapFree(GetProcessHeap(), 0, bytes);
        return false;
    }
    clip.bytes = bytes;
    clip.pcm = pcm;
    clip.frames = length / 4;
    return true;
}

// Mix accepted voices while holding the audio lock; the worker never reads live gameplay state.
void mixAudio(short *output, int frames) {
    EnterCriticalSection(&audioLock);
    for (int frame = 0; frame < frames; frame++) {
        int left = 0, right = 0;
        if (musicEnabled && musicClip.frames) {
            left = musicClip.pcm[musicPosition * 2] / 4;
            right = musicClip.pcm[musicPosition * 2 + 1] / 4;
            if (musicGain != 1) {
                left = (int)(left * musicGain);
                right = (int)(right * musicGain);
            }
            if (++musicPosition >= musicClip.frames)
                musicPosition = 0;
        }
        if (soundEnabled)
            for (int i = 0; i < 32; i++) {
                AudioVoice &v = audioVoices[i];
                if (!v.clip)
                    continue;
                int sampleLeft = v.clip->pcm[v.position * 2],
                    sampleRight = v.clip->pcm[v.position * 2 + 1];
                if (v.fraction && v.position + 1 < v.clip->frames) {
                    float f = (float)v.fraction / 65536;
                    sampleLeft += (int)((v.clip->pcm[(v.position + 1) * 2] - sampleLeft) * f);
                    sampleRight += (int)((v.clip->pcm[(v.position + 1) * 2 + 1] - sampleRight) * f);
                }
                int voiceLeft = sampleLeft * 3 / 4, voiceRight = sampleRight * 3 / 4;
                if (v.gain != 1) {
                    voiceLeft = (int)(voiceLeft * v.gain);
                    voiceRight = (int)(voiceRight * v.gain);
                }
                left += voiceLeft;
                right += voiceRight;
                audioJumpTraceMixedFrame(i, voiceLeft, voiceRight);
                u32 advance = v.fraction + (v.rate ? v.rate : 65536);
                v.position += advance >> 16;
                v.fraction = advance & 65535;
                if (v.position >= v.clip->frames) {
                    audioJumpTraceFinishVoice(i, AUDIO_JUMP_COMPLETE);
                    v.clip = 0;
                }
            }
        output[frame * 2] = (short)clamp((float)left, -32768, 32767);
        output[frame * 2 + 1] = (short)clamp((float)right, -32768, 32767);
        if (left || right)
            audioNonzeroSamples++;
        audioSamplesMixed++;
    }
    fillAudioOutputSilence(output, frames, soundEnabled, audioSilenceKeepAlive);
    LeaveCriticalSection(&audioLock);
}

DWORD WINAPI audioWorker(void *) {
    while (audioRunning) {
        for (int i = 0; i < AUDIO_BUFFERS; i++)
            if (!(audioHeaders[i].dwFlags & WHDR_INQUEUE)) {
                mixAudio(audioBuffers[i], AUDIO_FRAMES);
                MMRESULT result = waveOutWrite(musicDevice, audioHeaders + i, sizeof(WAVEHDR));
                if (result != MMSYSERR_NOERROR)
                    audioError = result;
            }
        WaitForSingleObject(audioEvent, 20);
    }
    return 0;
}

void initAudio(const Props &sounds, bool output = true) {
    if (audioInitialized)
        return;
    InitializeCriticalSection(&audioLock);
    audioInitialized = true;
    memset(&audioSilenceKeepAlive, 0, sizeof(audioSilenceKeepAlive));
    for (int i = 0; i < sounds.count && soundClipCount < 32; i++) {
        SoundClip &c = soundClips[soundClipCount];
        c.name = sounds.pairs[i].key;
        if (loadAudioClip(c.name, c.clip))
            soundClipCount++;
    }
    loadAudioClip("16755430903372978986", victoryClip);
    if (!output)
        return;
    audioEvent = CreateEventA(0, FALSE, FALSE, 0);
    if (!audioEvent) {
        audioError = MMSYSERR_NOMEM;
        return;
    }
    WAVEFORMATEX f = {};
    f.wFormatTag = WAVE_FORMAT_PCM;
    f.nChannels = 2;
    f.nSamplesPerSec = 22050;
    f.wBitsPerSample = 16;
    f.nBlockAlign = 4;
    f.nAvgBytesPerSec = 88200;
    audioError =
        waveOutOpen(&musicDevice, WAVE_MAPPER, &f, (DWORD_PTR)audioEvent, 0, CALLBACK_EVENT);
    if (audioError != MMSYSERR_NOERROR) {
        musicDevice = 0;
        return;
    }
    configureAudioSilenceKeepAlive(musicDevice);
    for (int i = 0; i < AUDIO_BUFFERS; i++) {
        WAVEHDR &h = audioHeaders[i];
        h.lpData = (LPSTR)audioBuffers[i];
        h.dwBufferLength = sizeof(audioBuffers[i]);
        audioError = waveOutPrepareHeader(musicDevice, &h, sizeof(h));
        if (audioError != MMSYSERR_NOERROR)
            return;
    }
    audioRunning = true;
    audioThread = CreateThread(0, 0, audioWorker, 0, 0, 0);
    if (!audioThread) {
        audioRunning = false;
        audioError = MMSYSERR_NOMEM;
    }
}

void stopSounds() {
    if (!audioInitialized)
        return;
    EnterCriticalSection(&audioLock);
    for (int i = 0; i < 32; i++)
        audioJumpTraceFinishVoice(i, AUDIO_JUMP_STOPPED);
    memset(audioVoices, 0, sizeof(audioVoices));
    LeaveCriticalSection(&audioLock);
}

void setMusicGain(float gain) {
    if (audioInitialized)
        EnterCriticalSection(&audioLock);
    musicGain = clamp(gain, 0, 1);
    if (audioInitialized)
        LeaveCriticalSection(&audioLock);
}

void stopMusic() {
    if (!audioInitialized) {
        playingMusic = 0;
        return;
    }
    EnterCriticalSection(&audioLock);
    AudioClip old = musicClip;
    bool owned = musicOwned;
    musicOwned = false;
    memset(&musicClip, 0, sizeof(musicClip));
    playingMusic = 0;
    musicPosition = 0;
    LeaveCriticalSection(&audioLock);
    if (owned && old.bytes)
        HeapFree(GetProcessHeap(), 0, old.bytes);
}

void playMusic(u64 id) {
    if (!musicEnabled || !id) {
        stopMusic();
        return;
    }
    if (id == playingMusic)
        return;
    if (!audioInitialized)
        initAudio(soundFiles);
    char name[24];
    idstr(id, name);
    AudioClip next = {};
    bool owned = id != 16755430903372978986ULL || !victoryClip.frames;
    if (owned) {
        if (!loadAudioClip(name, next))
            return;
    } else
        next = victoryClip;
    EnterCriticalSection(&audioLock);
    AudioClip old = musicClip;
    bool oldOwned = musicOwned;
    musicClip = next;
    musicOwned = owned;
    musicPosition = 0;
    playingMusic = id;
    LeaveCriticalSection(&audioLock);
    if (oldOwned && old.bytes)
        HeapFree(GetProcessHeap(), 0, old.bytes);
}

// Honor the SFX toggle and object mute before allocating a voice for this gameplay event.
void sound(const char *name, float rate = 1, float gain = 1) {
    int rejected = !soundEnabled             ? AUDIO_JUMP_DISABLED
                   : !audioInitialized       ? AUDIO_JUMP_UNINITIALIZED
                   : objectSoundsMutedNative ? AUDIO_JUMP_OBJECT_MUTED
                                             : AUDIO_JUMP_ACCEPTED;
    if (rejected) {
        audioJumpTraceRejected(name, rejected);
        return;
    }
    AudioClip *clip = 0;
    for (int i = 0; i < soundClipCount; i++)
        if (eq(name, soundClips[i].name)) {
            clip = &soundClips[i].clip;
            break;
        }
    if (!clip) {
        audioJumpTraceRejected(name, AUDIO_JUMP_CLIP_MISSING);
        return;
    }
    EnterCriticalSection(&audioLock);
    int slot = 0;
    for (int i = 0; i < 32; i++) {
        if (!audioVoices[i].clip) {
            slot = i;
            break;
        }
        if (audioVoices[i].serial < audioVoices[slot].serial)
            slot = i;
    }
    audioJumpTraceFinishVoice(slot, AUDIO_JUMP_PREEMPTED);
    audioVoices[slot].clip = clip;
    audioVoices[slot].position = 0;
    audioVoices[slot].fraction = 0;
    audioVoices[slot].rate = (u32)(int)(clamp(rate, .5f, 2) * 65536 + .5f);
    audioVoices[slot].gain = clamp(gain, 0, 1);
    audioVoices[slot].serial = ++voiceSerial;
    audioVoicesStarted++;
    audioJumpTraceStarted(name, slot, voiceSerial, clip->frames);
    LeaveCriticalSection(&audioLock);
}

void shutdownAudio() {
    if (!audioInitialized)
        return;
    audioRunning = false;
    if (audioEvent)
        SetEvent(audioEvent);
    if (audioThread) {
        WaitForSingleObject(audioThread, INFINITE);
        CloseHandle(audioThread);
        audioThread = 0;
    }
    for (int i = 0; i < 32; i++)
        audioJumpTraceFinishVoice(i, AUDIO_JUMP_SHUTDOWN);
    if (musicDevice) {
        waveOutReset(musicDevice);
        for (int i = 0; i < AUDIO_BUFFERS; i++)
            if (audioHeaders[i].dwFlags & WHDR_PREPARED)
                waveOutUnprepareHeader(musicDevice, audioHeaders + i, sizeof(WAVEHDR));
        waveOutClose(musicDevice);
        musicDevice = 0;
    }
    if (audioEvent) {
        CloseHandle(audioEvent);
        audioEvent = 0;
    }
    stopMusic();
    if (victoryClip.bytes)
        HeapFree(GetProcessHeap(), 0, victoryClip.bytes);
    memset(&victoryClip, 0, sizeof(victoryClip));
    for (int i = 0; i < soundClipCount; i++)
        HeapFree(GetProcessHeap(), 0, soundClips[i].clip.bytes);
    soundClipCount = 0;
    DeleteCriticalSection(&audioLock);
    audioInitialized = false;
}
