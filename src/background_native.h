#pragma once

// Reads and applies original background and parallax parameters.

// BackgroundBehaviour::OnPostUpdate, 0x11ec38..0x11ece0. Wrap the
// actor's centre, not its outer edges. Authored clouds already overlap.
float backgroundWrappedAxis(float position, float cameraCentre, float view, float margin,
                            bool useCamera) {
    float half = useCamera ? view * 0.5f : 0, lower = cameraCentre - half - margin,
          period = 2 * (half + margin);
    if (period <= 0)
        return position;
    float phase = position - lower;
    bool negative = phase < 0;
    phase -= (int)(phase / period) * period;
    if (negative)
        phase += period;
    return lower + phase;
}
