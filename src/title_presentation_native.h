#pragma once

// Shared title presentation behavior used by the Windows reconstruction.

// TitleScreenController::OnActivate @0x306f7c supplies a separate entry
// interval for each referenced object. These are camera units, not pixels
// of the resized desktop client. The final three values are bob period,
// amplitude and phase. All eleven original tracks use signed sine mode.
struct TitleNativeTrack {
    const char *key;
    float start, end, period, amplitude, phase;
};

const TitleNativeTrack titleNativeTracks[] = {{"bean", .85f, 1.7f, 2.5f, 4, 0},
                                              {"title-scroll", 0, .5f, 2.5f, 0, .15f},
                                              {"title-b", .1f, .6f, 1.5f, 0, -1.5f},
                                              {"title-e", .2f, .7f, 1.5f, 0, -1.6f},
                                              {"title-a", .3f, .8f, 1.5f, 0, -1.7f},
                                              {"title-n", .4f, .9f, 1.5f, 0, -1.8f},
                                              {"title-apos", .45f, .95f, 1.5f, 0, -1.85f},
                                              {"title-s", .5f, 1, 1.5f, 0, -1.9f},
                                              {"title-quest", .6f, 1.1f, 2.5f, 0, .15f},
                                              {"title-hat-back", .8f, 1.3f, 1, 0, .25f},
                                              {"title-hat-front", .8f, 1.3f, 1, 0, .25f}};

int titleNativeTrackFor(const Level &level, u64 object) {
    for (int i = 0; i < level.objectCount; i++) {
        const Props *p = level.objects[i].behavior("TitleScreenController");
        if (!p)
            continue;
        for (int j = 0; j < 11; j++)
            if (idnum(p->get(titleNativeTracks[j].key)) == object)
                return j;
    }
    return -1;
}

// OnUpdate @0x305c1c..0x305da0: sine X easing and the original elastic
// Y curve, followed by the continuous signed bob at0x305fb8..0x305fec.
Vec titleNativeOffset(int index, float age, float width, float height) {
    if (index < 0 || index >= 11)
        return Vec{0, 0};
    const TitleNativeTrack &track = titleNativeTracks[index];
    float t = clamp((age - track.start) / (track.end - track.start), 0, 1);
    float dx = index == 0 ? -width * .5f : 0, dy = index == 0 ? -height * 2 : height * .5f;
    if (t >= 1) {
        dx = dy = 0;
    } else if (t > 0) {
        float xEase = bqSin(t * 3.1415927410125732f * .5f);
        float wave = t * 6.2831854820251465f * (.2f + 2.5f * t * t * t);
        float yEase = (t + bqSin(wave) * bqPow(1 - t, 2.2f)) * (1 + 1.2f * (1 - t));
        dx *= 1 - xEase;
        dy *= 1 - yEase;
    }
    dy += track.amplitude * bqSin((age + track.phase) * 6.2831854820251465f / track.period);
    return Vec{dx, dy};
}
