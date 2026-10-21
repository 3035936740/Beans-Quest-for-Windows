#pragma once

// Authored animation state and timing for coin.

// CoinGroup::OnActivate0x164de8/0x1650a4..e0 uses lrand48()%5 and the
// original diamond1..5 clips. Each actlet owns a separate instance whose
// initial time is positive fmod((worldY-worldX)/200,length)0x165214..270.
u64 coinAnimationRandomNative = 0x1234abcd330eULL;

u32 coinAnimationRandomNextNative(u64 &state) {
    // POSIX48bit LCG, split into16bit limbs to avoid a new CRT dependency.
    u32 a = (u32)state & 65535, b = ((u32)state >> 16) & 65535, c = (u32)(state >> 32) & 65535;
    u32 x = a * 0xe66d + 11, y = b * 0xe66d + (x >> 16), z = a * 0xdeec;
    u32 middle = (y & 65535) + (z & 65535), carry = (y >> 16) + (z >> 16) + (middle >> 16);
    u32 high = (c * 0xe66d + b * 0xdeec + a * 5 + carry) & 65535;
    state = ((u64)high << 32) | ((u64)(middle & 65535) << 16) | (x & 65535);
    return (high << 15) | ((middle & 65535) >> 1);
}

const char *coinAnimationVariantNameNative(int variant) {
    const char *names[] = {"diamond 1", "diamond 2", "diamond 3", "diamond 4", "diamond 5"};
    return names[variant >= 0 && variant < 5 ? variant : 0];
}

bool coinAnimationSavedNative(const State &state) {
    return state.aux >= -5 && state.aux <= -1 && state.aux == (float)(int)state.aux;
}

int coinAnimationVariantNative(const State &state) {
    return coinAnimationSavedNative(state) ? -(int)state.aux - 1 : 0;
}

const char *coinAnimationNameNative(const Entity &e) {
    return coinAnimationVariantNameNative(coinAnimationVariantNative(e.state));
}

float coinAnimationInitialTimeNative(float x, float y, float length) {
    if (length <= 0)
        return 0;
    float phase = (y - x) / 200;
    float remainder = phase - (int)(phase / length) * length;
    return phase < 0 ? length + remainder : remainder;
}

void initializeCoinAnimationNative(Entity &e, int variant = -1) {
    if (e.kind != COIN)
        return;
    if (variant < 0)
        variant = coinAnimationRandomNextNative(coinAnimationRandomNative) % 5;
    // Negative1..5encodes the selected clip in the previously unused gem
    // State::aux. Its per-instance clock already lives in saved State::timer.
    e.state.aux = -(float)(variant + 1);
    e.state.timer = coinAnimationInitialTimeNative(
        e.state.x, e.state.y, animationLength(animation(e.art, coinAnimationNameNative(e))));
}

void migrateCoinAnimationNative(Entity &e, const State &activation) {
    if (e.kind != COIN || coinAnimationSavedNative(e.state))
        return;
    float oldAge = e.state.timer;
    initializeCoinAnimationNative(e, coinAnimationVariantNative(activation));
    e.state.timer += oldAge;
}
