#pragma once

// Regression coverage for world card texture. Runs only in the isolated BQ_ENABLE_TESTS executable.

void worldCardTextureBegin() {
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, 512, 384);
    glClearColor(0.05f, 0.35f, 0.45f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 512, 0, 384, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void worldCardTextureChecks() {
    Arena rawMemory = {};
    rawMemory.init(24 * 1024 * 1024);
    u32 fileSize = 0;
    u8 *file = readFile("data/Media/Levels/18006219643615701355.plevel", &fileSize, rawMemory);
    u32 fileHash = hashBytes(file, fileSize);
    Reader reader(file, fileSize, rawMemory);
    reader.take(32);
    reader.props();
    int layers = reader.i();
    reader.take(layers * 44);
    reader.take(16);
    int textureCount = reader.i();
    reader.take(16);
    int h[10];
    memcpy(h, reader.take(40), 40);
    u8 *original = (u8 *)reader.take(h[9]);
    check(
        textureCount == 2 && h[2] == 1 && h[3] == 1024 && h[4] == 1024,
        "Real world-select atlas is the expected two-texture resource with a single-level1024x1024 first texture");
    if (textureCount != 2 || h[2] != 1 || h[3] != 1024 || h[4] != 1024) {
        VirtualFree(rawMemory.data, 0, MEM_RELEASE);
        return;
    }
    reader.take(16);
    int h1[10];
    memcpy(h1, reader.take(40), 40);
    u8 *original1 = (u8 *)reader.take(h1[9]);
    u8 *fixed = (u8 *)HeapAlloc(GetProcessHeap(), 0, h[9]);
    u8 *gpu = (u8 *)HeapAlloc(GetProcessHeap(), 0, h[9]);
    memcpy(fixed, original, h[9]);
    u32 originalHash = hashBytes(original, h[9]);
    bool guards = !repairWorldCardTexture(0, 0, 1024, 1024, fixed) &&
                  !repairWorldCardTexture(18006219643615701355ULL, 1, 1024, 1024, fixed) &&
                  !repairWorldCardTexture(18006219643615701355ULL, 0, 512, 1024, fixed) &&
                  !repairWorldCardTexture(18006219643615701355ULL, 0, 1024, 512, fixed) &&
                  hashBytes(fixed, h[9]) == originalHash;
    for (int i = 0; i < 6; i++)
        for (int c = 0; c < 4; c++) {
            int at = ((399 + i / 3) * 1024 + i % 3) * 4 + c;
            fixed[at] ^= 1;
            u32 changedGuardHash = hashBytes(fixed, h[9]);
            guards &= !repairWorldCardTexture(18006219643615701355ULL, 0, 1024, 1024, fixed) &&
                      hashBytes(fixed, h[9]) == changedGuardHash;
            fixed[at] = original[at];
        }
    check(
        guards,
        "World-card repair rejects other scenes, texture indices, dimensions, and even a one-byte change to any expected texel without modifying input");
    bool applied = repairWorldCardTexture(18006219643615701355ULL, 0, 1024, 1024, fixed),
         outside = true, cleared = true;
    int bytesChanged = 0;
    for (int y = 0; y < 1024; y++)
        for (int x = 0; x < 1024; x++)
            for (int c = 0; c < 4; c++) {
                int at = (y * 1024 + x) * 4 + c;
                bool target = x <= 2 && (y == 399 || y == 400);
                if (fixed[at] != original[at])
                    bytesChanged++;
                if (target)
                    cleared &= fixed[at] == 0;
                else
                    outside &= fixed[at] == original[at];
            }
    check(
        applied && outside && cleared && bytesChanged == 24 &&
            !repairWorldCardTexture(18006219643615701355ULL, 0, 1024, 1024, fixed),
        "Exact six RGBA texels become transparent, including two padding copies; all other4194280bytes and adjacent frame pixels remain identical, and reapplication is inert");
    Arena sceneMemory = {};
    sceneMemory.init(24 * 1024 * 1024);
    Level scene = {};
    readLevelData("18006219643615701355.plevel", scene, sceneMemory);
    glBindTexture(GL_TEXTURE_2D, scene.textures[0].gl);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, gpu);
    bool loaded = hashBytes(gpu, h[9]) == hashBytes(fixed, h[9]);
    glBindTexture(GL_TEXTURE_2D, scene.textures[1].gl);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, gpu);
    loaded &= hashBytes(gpu, h1[9]) == hashBytes(original1, h1[9]);
    check(
        loaded,
        "Real readLevelData uploads only the precise repaired first atlas; its second atlas remains byte-identical on the GPU");
    GLuint rawTexture;
    glGenTextures(1, &rawTexture);
    glBindTexture(GL_TEXTURE_2D, rawTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, h[5]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, h[6]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, original);
    const float scales[] = {0.5f, 0.75f, 1, 2};
    bool renders = true;
    char renderLog[4096] = {};
    u8 *before = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    u8 *after = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    for (int s = 0; s < 4; s++) {
        Tile tile = scene.tiles[6];
        Vertex vertices[4];
        memcpy(vertices, tile.vertices, sizeof(vertices));
        tile.vertices = vertices;
        for (int v = 0; v < 4; v++) {
            vertices[v].x = 40 + (vertices[v].x - 1472) * 2 * scales[s];
            vertices[v].y = 20 + (vertices[v].y - 112) * 2 * scales[s];
        }
        GLuint repaired = scene.textures[0].gl;
        scene.textures[0].gl = rawTexture;
        worldCardTextureBegin();
        drawTile(tile, &scene);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, 512, 384, GL_RGBA, GL_UNSIGNED_BYTE, before);
        scene.textures[0].gl = repaired;
        worldCardTextureBegin();
        drawTile(tile, &scene);
        glReadPixels(0, 0, 512, 384, GL_RGBA, GL_UNSIGNED_BYTE, after);
        int changed = 0;
        int unexpected = 0;
        for (int y = 0; y < 384; y++)
            for (int x = 0; x < 512; x++) {
                int at = (y * 512 + x) * 4;
                bool differs = false;
                for (int c = 0; c < 3; c++)
                    differs |= before[at + c] != after[at + c];
                if (differs) {
                    changed++;
                    float tx = 1 + (x + 0.5f - 40) / scales[s],
                          ty = 387 + (y + 0.5f - 20) / scales[s];
                    bool sourceRed = false;
                    for (int sample = 0; sample < 6; sample++) {
                        const u8 *p = original + ((399 + sample / 3) * 1024 + sample % 3) * 4;
                        sourceRed |=
                            before[at] == p[0] && before[at + 1] == p[1] && before[at + 2] == p[2];
                    }
                    bool background = after[at] == after[0] && after[at + 1] == after[1] &&
                                      after[at + 2] == after[2];
                    bool target =
                        tx >= 1 && tx <= 3 && ty >= 399 && ty <= 401 && sourceRed && background;
                    renders &= target;
                    if (!target) {
                        unexpected++;
                        if (unexpected < 8) {
                            char line[128];
                            wsprintfA(
                                line,
                                "scale%d unexpected%d,%d source100=%d,%d rgb=%d,%d,%d -> %d,%d,%d\r\n",
                                s, x, y, (int)(tx * 100), (int)(ty * 100), before[at],
                                before[at + 1], before[at + 2], after[at], after[at + 1],
                                after[at + 2]);
                            lstrcatA(renderLog, line);
                        }
                    }
                }
            }
        char summary[80];
        wsprintfA(summary, "scale%d changed=%d unexpected=%d\r\n", s, changed, unexpected);
        lstrcatA(renderLog, summary);
        renders &= changed > 0;
        char path[96];
        wsprintfA(path, "test-output/world-card-clean-scale-%d.bmp", s);
        int oldX = viewportX, oldY = viewportY, oldW = viewportW, oldH = viewportH;
        viewportX = viewportY = 0;
        viewportW = 512;
        viewportH = 384;
        SwapBuffers(gameDC);
        capture(path);
        viewportX = oldX;
        viewportY = oldY;
        viewportW = oldW;
        viewportH = oldH;
    }
    atomicWrite("test-output/world-card-render-diff.txt", renderLog, lstrlenA(renderLog));
    check(
        renders,
        "Actual OpenGL card rendering at0.5x,0.75x,1x,2x removes only the reported speck; every neighbouring frame/background pixel stays identical");
    u32 freshSize;
    u8 *fresh = readFile("data/Media/Levels/18006219643615701355.plevel", &freshSize, rawMemory);
    check(
        freshSize == fileSize && hashBytes(fresh, freshSize) == fileHash,
        "Loading and derived GPU repair leave the packaged original world scene file byte-identical");
    HeapFree(GetProcessHeap(), 0, after);
    HeapFree(GetProcessHeap(), 0, before);
    glDeleteTextures(1, &rawTexture);
    releaseLevel(scene, sceneMemory);
    VirtualFree(sceneMemory.data, 0, MEM_RELEASE);
    HeapFree(GetProcessHeap(), 0, gpu);
    HeapFree(GetProcessHeap(), 0, fixed);
    VirtualFree(rawMemory.data, 0, MEM_RELEASE);
}
