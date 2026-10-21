#pragma once

// Repairs the known world-card resource issue while loading the affected scene texture.

// Six opaque stray texels in the supplied Malogoz world-card atlas, including
// the two duplicated left-padding texels. Keep the supplied resource immutable.
bool repairWorldCardTexture(u64 sceneId, int textureIndex, int width, int height, u8 *pixels) {
    if (sceneId != 18006219643615701355ULL || textureIndex != 0 || width != 1024 ||
        height != 1024 || !pixels)
        return false;
    const u8 expected[6][4] = {{163, 34, 22, 255}, {163, 34, 22, 255}, {192, 68, 46, 255},
                               {162, 35, 22, 255}, {162, 35, 22, 255}, {174, 41, 25, 255}};
    for (int i = 0; i < 6; i++) {
        const u8 *pixel = pixels + ((399 + i / 3) * 1024 + i % 3) * 4;
        for (int c = 0; c < 4; c++)
            if (pixel[c] != expected[i][c])
                return false;
    }
    for (int i = 0; i < 6; i++) {
        u8 *pixel = pixels + ((399 + i / 3) * 1024 + i % 3) * 4;
        for (int c = 0; c < 4; c++)
            pixel[c] = 0;
    }
    return true;
}
