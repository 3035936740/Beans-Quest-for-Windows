#pragma once

// Copies potion frames into matching texture coordinates to keep their silhouette stable at fractional scales.

// This rotated atlas has identical bottle alpha in every idle frame. Desktop
// fixed-function GL can round atlas coordinates differently at fractional
// scales. Give each frame the same local coordinates, copying texels exactly;
// retain all authored frames, durations, filtering and the entity rectangle.
void alignPotionTextureFrames(Actor &a) {
    if (a.id != 8048939671444015ULL || a.textureCount != 1)
        return;
    Texture original = a.textures[0];
    int count = 0;
    for (int i = 0; i < a.count; i++)
        count += a.animations[i].count;
    if (!count)
        return;
    glBindTexture(GL_TEXTURE_2D, original.gl);
    GLint mag, min;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min);
    if (mag != GL_NEAREST || min != GL_NEAREST)
        return;
    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, original.width * original.height * 4);
    if (!pixels)
        fatal("Cannot load potion texture.");
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    Texture *frames = permanent.array<Texture>(count);
    int index = 0;
    for (int i = 0; i < a.count; i++)
        for (int j = 0; j < a.animations[i].count; j++) {
            Frame &f = a.animations[i].frames[j];
            u8 tile[64 * 64 * 4] = {};
            for (int y = 0; y < 40; y++)
                for (int x = 0; x < 32; x++) {
                    float s = (x + .5f) / 32, t = (y + .5f) / 40;
                    int u = (int)((f.uv[0] + s * (f.uv[2] - f.uv[0]) + t * (f.uv[4] - f.uv[0])) *
                                  original.width);
                    int v = (int)((f.uv[1] + s * (f.uv[3] - f.uv[1]) + t * (f.uv[5] - f.uv[1])) *
                                  original.height);
                    if (u < 0 || u >= original.width || v < 0 || v >= original.height)
                        fatal("Unsupported potion frame.");
                    memcpy(tile + ((y + 1) * 64 + x + 1) * 4, pixels + (v * original.width + u) * 4,
                           4);
                }
            Texture &texture = frames[index];
            texture.width = texture.height = 64;
            glGenTextures(1, &texture.gl);
            glBindTexture(GL_TEXTURE_2D, texture.gl);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, tile);
            const float uv[] = {1 / 64.0f, 1 / 64.0f,  33 / 64.0f, 1 / 64.0f,
                                1 / 64.0f, 41 / 64.0f, 33 / 64.0f, 41 / 64.0f};
            memcpy(f.uv, uv, sizeof(uv));
            f.texture = index++;
        }
    HeapFree(GetProcessHeap(), 0, pixels);
    glDeleteTextures(1, &original.gl);
    a.textures = frames;
    a.textureCount = count;
}
