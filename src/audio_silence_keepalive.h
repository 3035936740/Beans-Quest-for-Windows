#pragma once

// Maintains the verified silent-output workaround for affected headphone devices.
#include <mmddk.h>

// Some AB13X USB outputs mute their analogue path after digital silence.
// This is an endpoint compatibility measure, not an original game effect.
// Keep ordinary mixed buffers byte-identical and fill only wholly silent
// buffers while SFX remain enabled. A user A/B test found that a continuous
// 10Hz signal prevents the missing jumps without the audible440Hz test tone.
// At the observed31% session volume,832 units reach about260 output units,
// matching that successful256-unit comparison. Other outputs stay unchanged.
struct AudioSilenceKeepAlive {
    bool matched;
    wchar_t deviceName[MAXPNAMELEN];
    u32 phase, frames;
};

AudioSilenceKeepAlive audioSilenceKeepAlive;

wchar_t audioDeviceLower(wchar_t c) {
    return c >= L'A' && c <= L'Z' ? c + (L'a' - L'A') : c;
}

bool audioDeviceNeedsSilenceKeepAlive(const wchar_t *name) {
    const wchar_t model[] = L"ab13x usb audio";
    for (int i = 0; name[i]; i++) {
        if (i && ((name[i - 1] >= L'a' && name[i - 1] <= L'z') ||
                  (name[i - 1] >= L'A' && name[i - 1] <= L'Z') ||
                  (name[i - 1] >= L'0' && name[i - 1] <= L'9')))
            continue;
        int j = 0;
        while (model[j] && name[i + j] && audioDeviceLower(name[i + j]) == model[j])
            j++;
        if (!model[j] &&
            !(audioDeviceLower(name[i + j]) >= L'a' && audioDeviceLower(name[i + j]) <= L'z') &&
            !(name[i + j] >= L'0' && name[i + j] <= L'9'))
            return true;
    }
    return false;
}

void configureAudioSilenceKeepAlive(HWAVEOUT device) {
    memset(&audioSilenceKeepAlive, 0, sizeof(audioSilenceKeepAlive));
    WAVEOUTCAPSW caps = {};
    UINT deviceId = WAVE_MAPPER;
    if (!device || waveOutGetID(device, &deviceId) != MMSYSERR_NOERROR)
        return;
    // Modern mapper handles can retain WAVE_MAPPER as their device ID.
    // Query its preferred output using the Windows 2000/XP WinMM message.
    if (deviceId == WAVE_MAPPER) {
        DWORD preferred = WAVE_MAPPER, flags = 0;
        if (waveOutMessage((HWAVEOUT)(UINT_PTR)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET,
                           (DWORD_PTR)&preferred, (DWORD_PTR)&flags) != MMSYSERR_NOERROR)
            return;
        deviceId = preferred;
    }
    if (deviceId != WAVE_MAPPER &&
        waveOutGetDevCapsW(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
        memcpy(audioSilenceKeepAlive.deviceName, caps.szPname, sizeof(caps.szPname));
        audioSilenceKeepAlive.deviceName[MAXPNAMELEN - 1] = 0;
        audioSilenceKeepAlive.matched =
            audioDeviceNeedsSilenceKeepAlive(audioSilenceKeepAlive.deviceName);
    }
}

bool fillAudioOutputSilence(short *output, int frames, bool effectsEnabled,
                            AudioSilenceKeepAlive &state) {
    if (!state.matched || !effectsEnabled || frames <= 0) {
        state.phase = 0;
        return false;
    }
    bool silent = true;
    for (int i = 0; i < frames * 2; i++)
        if (output[i]) {
            silent = false;
            break;
        }
    const u32 period = 2205;
    u32 phase = state.phase % period;
    if (silent) {
        for (int i = 0; i < frames; i++) {
            int signedPhase = phase <= 1102 ? (int)phase : (int)phase - (int)period;
            short value = (short)(int)(832 * sine(signedPhase * (6.28318530718f / 2205)));
            output[i * 2] = output[i * 2 + 1] = value;
            if (++phase == period)
                phase = 0;
        }
        state.frames += (u32)frames;
    } else
        phase = (phase + (u32)frames) % period;
    state.phase = phase;
    return silent;
}
