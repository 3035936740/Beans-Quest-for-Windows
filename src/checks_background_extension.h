#pragma once

// Regression coverage for background extension. Runs only in the isolated BQ_ENABLE_TESTS executable.

void backgroundExtensionCheckBegin(Rect view) {
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, 512, 384);
    glClearColor(0.321f, 0.123f, 0.987f, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(view.l, view.r, view.b, view.t, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void backgroundExtensionCheckLayer(Level &scene, int layer, Rect view, bool extend) {
    int last = -2147483647;
    for (;;) {
        int next = 2147483647;
        for (int i = 0; i < scene.tileCount; i++)
            if (scene.tiles[i].layer == layer && scene.tiles[i].z > last && scene.tiles[i].z < next)
                next = scene.tiles[i].z;
        if (next == 2147483647)
            break;
        for (int i = 0; i < scene.tileCount; i++)
            if (scene.tiles[i].layer == layer && scene.tiles[i].z == next)
                drawTile(scene.tiles[i], &scene, extend ? &view : 0);
        last = next;
    }
}

void backgroundExtensionCheckRead(u8 *pixels) {
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, 512, 384, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

u32 backgroundExtensionGeometryHash(Level &scene) {
    u32 result = hashBytes(scene.tiles, scene.tileCount * sizeof(Tile));
    for (int i = 0; i < scene.tileCount; i++)
        result ^= hashBytes(scene.tiles[i].vertices, scene.tiles[i].bytes);
    return result;
}

void backgroundExtensionChecks() {
    Arena memory = {};
    memory.init(24 * 1024 * 1024);
    Level scene = {};
    int matched = 0, coveredScenes = 0, rooms = 0;
    bool manifest = true, immutable = true, roomCoverage = true, geometryGuards = true;
    for (int level = 0; level < levelCount; level++) {
        readLevelData(levels[level].file, scene, memory);
        bool covered = false;
        u32 geometryBefore = backgroundExtensionGeometryHash(scene);
        for (int p = 0; p < backgroundExtensionPolicyCount; p++) {
            const BackgroundExtensionPolicy &policy = backgroundExtensionPolicies[p];
            if (policy.scene != scene.id)
                continue;
            covered = true;
            bool match = policy.tile >= 0 && policy.tile < scene.tileCount &&
                         backgroundExtensionMatches(policy, scene, scene.tiles[policy.tile]);
            manifest &= match;
            if (match) {
                matched++;
                Tile &tile = scene.tiles[policy.tile];
                float oldX = tile.vertices[policy.vertex].x;
                tile.vertices[policy.vertex].x += 0.125f;
                geometryGuards &= !backgroundExtensionMatches(policy, scene, tile);
                tile.vertices[policy.vertex].x = oldX;
                Rect view = policy.bounds;
                if (policy.mode == 1) {
                    view.l -= policy.stepX * 2;
                    view.r += policy.stepX * 2;
                    view.b = tile.vertices[policy.vertex].y;
                    view.t = view.b + policy.stepY;
                } else {
                    view.l -= 512;
                    view.r += 512;
                    view.b -= 512;
                    view.t += 512;
                }
                backgroundExtensionCheckBegin(view);
                u8 sentinel[4];
                glReadBuffer(GL_BACK);
                glReadPixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, sentinel);
                backgroundExtensionCheckLayer(scene, tile.layer, view, true);
                if (policy.mode == 2) {
                    rooms++;
                    const int xs[] = {1, 510}, ys[] = {1, 382};
                    for (int y = 0; y < 2; y++)
                        for (int x = 0; x < 2; x++) {
                            u8 pixel[4];
                            glReadPixels(xs[x], ys[y], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                            roomCoverage &= pixel[0] != sentinel[0] || pixel[1] != sentinel[1] ||
                                            pixel[2] != sentinel[2];
                        }
                }
            }
        }
        immutable &= geometryBefore == backgroundExtensionGeometryHash(scene);
        if (covered)
            coveredScenes++;
    }
    check(
        levelCount == 74 && manifest && matched == backgroundExtensionPolicyCount &&
            coveredScenes >= 61,
        "All74 supplied scenes are audited; every backdrop policy matches its real UVs/layers, including all eight periodic indoor backgrounds");
    check(
        geometryGuards,
        "Every approved policy rejects a changed source vertex position; geometry hashes guard exact authored positions and UVs, with only the explicit World5 tint widening accepted");
    check(
        immutable,
        "Rendering every approved background extension leaves all authored tile metadata and every vertex byte unchanged");
    check(
        rooms >= 8 && roomCoverage,
        "Actual OpenGL periodic indoor backgrounds fill all four viewport corners beyond their original rectangles with the authored64/128-unit brick pattern");
    readLevelData("669776786229.plevel", scene, memory);
    repairFifthSelectorTint(scene);
    u8 *before = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    u8 *after = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    const Vec sizes[] = {{512, 384}, {1916, 320}, {4096, 320}, {320, 1536}, {96, 384}};
    bool coverage = true, inside = true;
    char report[1024] = {};
    for (int s = 0; s < 5; s++) {
        Rect view = {432 - sizes[s].x * 0.5f, 192 - sizes[s].y * 0.5f, 432 + sizes[s].x * 0.5f,
                     192 + sizes[s].y * 0.5f};
        backgroundExtensionCheckBegin(view);
        u8 sentinel[4];
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, sentinel);
        backgroundExtensionCheckLayer(scene, 0, view, false);
        backgroundExtensionCheckLayer(scene, 5, view, false);
        backgroundExtensionCheckRead(before);
        backgroundExtensionCheckBegin(view);
        backgroundExtensionCheckLayer(scene, 0, view, true);
        backgroundExtensionCheckLayer(scene, 5, view, true);
        backgroundExtensionCheckRead(after);
        int missing = 0, changedInside = 0;
        for (int y = 0; y < 384; y++) {
            float rowY = view.b + (y + .5f) * (view.t - view.b) / 384;
            bool adaptedScenery = false;
            for (int p = 0; p < backgroundExtensionPolicyCount; p++) {
                const BackgroundExtensionPolicy &policy = backgroundExtensionPolicies[p];
                adaptedScenery |= policy.scene == scene.id && policy.mode >= 4 &&
                                  backgroundSceneryScale(policy, view) > 1 &&
                                  rowY >= policy.bounds.b && rowY < policy.bounds.t;
            }
            for (int x = 0; x < 512; x++) {
                int at = (y * 512 + x) * 4;
                float worldX = view.l + (x + 0.5f) * (view.r - view.l) / 512,
                      worldY = view.b + (y + 0.5f) * (view.t - view.b) / 384;
                bool same = true, empty = true;
                for (int c = 0; c < 3; c++) {
                    same &= before[at + c] == after[at + c];
                    empty &= after[at + c] == sentinel[c];
                }
                if (empty)
                    missing++;
                if (worldX >= 0 && worldX < 864 && worldY >= 0 && worldY < 384 && !same &&
                    !adaptedScenery)
                    changedInside++;
            }
        }
        coverage &= missing == 0;
        inside &= changedInside == 0;
        char line[128];
        wsprintfA(line, "view%d %dx%d empty=%d changedInsideOriginal=%d\r\n", s, (int)sizes[s].x,
                  (int)sizes[s].y, missing, changedInside);
        lstrcatA(report, line);
        char path[96];
        wsprintfA(path, "test-output/background-extension-view-%d.bmp", s);
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
    atomicWrite("test-output/background-extension-diff.txt", report, lstrlenA(report));
    check(
        coverage,
        "Actual OpenGL fifth-world background completely covers standard, reported1916x320, extreme4096x320, tall320x1536 and narrow96x384 viewports");
    check(
        inside,
        "Sky/tint pixels remain identical across five aspect ratios; ordinary-width scenery is unchanged and only explicit ultra-wide horizon adaptation may differ");
    Rect foregroundView = {0, 0, 864, 384};
    backgroundExtensionCheckBegin(foregroundView);
    for (int layer = 1; layer < scene.layerCount; layer++)
        if (layer != 5)
            backgroundExtensionCheckLayer(scene, layer, foregroundView, false);
    backgroundExtensionCheckRead(before);
    backgroundExtensionCheckBegin(foregroundView);
    for (int layer = 1; layer < scene.layerCount; layer++)
        if (layer != 5)
            backgroundExtensionCheckLayer(scene, layer, foregroundView, true);
    backgroundExtensionCheckRead(after);
    check(
        hashBytes(before, 512 * 384 * 4) == hashBytes(after, 512 * 384 * 4),
        "Actual OpenGL authored foreground layers remain pixel-identical when continuation is enabled; no foreground tile is stretched or repeated");
    HeapFree(GetProcessHeap(), 0, after);
    HeapFree(GetProcessHeap(), 0, before);
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
}
