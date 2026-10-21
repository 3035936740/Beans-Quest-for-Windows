#pragma once

// Sparse patterned walls and story-local scenery need different invariants:
// openings must repeat, while future environments must never repeat globally.
void patternedBackdropChecks() {
    Arena memory = {};
    memory.init(24 * 1024 * 1024);
    Level scene = {};
    readLevelData(levels[stageList[stageById(901687619158ULL)]].file, scene, memory);
    const BackgroundExtensionPolicy *pattern = 0;
    for (int i = 0; i < backgroundExtensionPolicyCount; i++)
        if (backgroundExtensionPolicies[i].scene == scene.id &&
            backgroundExtensionPolicies[i].mode == 8)
            pattern = backgroundExtensionPolicies + i;
    check(pattern && pattern->columns == 8 && pattern->rows == 5,
          "Displayed4-10 Brutal Forces has its proven eight-column/five-row sparse masonry motif");
    if (!pattern) {
        releaseLevel(scene, memory);
        VirtualFree(memory.data, 0, MEM_RELEASE);
        return;
    }
    const BackgroundExtensionPolicy &p = *pattern;
    bool cells[64 * 64] = {};
    int nx = (int)((p.bounds.r - p.bounds.l) / p.stepX);
    int ny = (int)((p.bounds.t - p.bounds.b) / p.stepY);
    int count = 0;
    u32 geometry = backgroundExtensionGeometryHash(scene);
    for (int i = 0; i < scene.tileCount; i++)
        for (int j = 0; j < scene.tiles[i].bytes / (int)sizeof(Vertex); j += 4) {
            if (!backgroundSceneryOwnsQuad(p, scene, scene.tiles[i], j))
                continue;
            Rect r = backgroundExtensionSourceRect(scene.tiles[i].vertices + j);
            int x = (int)((r.l - p.bounds.l) / p.stepX), y = (int)((r.b - p.bounds.b) / p.stepY);
            cells[y * nx + x] = true;
            count++;
        }
    bool exact = nx * ny <= 64 * 64;
    for (int y = 0; y < ny; y++)
        for (int x = 0; x < nx; x++)
            exact &= cells[y * nx + x] == backgroundPatternCell(p, x, y);
    check(
        exact && count == 208,
        "Sparse motif reproduces all208 authored wall cells and all32 intentional openings, including partial cycles");
    u8 *before = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    u8 *after = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    const Vec sizes[] = {{512, 384}, {3030, 320}, {7055, 320}, {320, 1536}, {8192, 320}};
    bool pixels = true, ordinary = true, density = true;
    int samples = 0, extended = 0, openings = 0;
    char rows[5000] = {};
    for (int size = 0; size < 5; size++)
        for (int phase = 0; phase < 3; phase++) {
            Rect view = {phase ? -650.0f : 0, phase == 2 ? -900.0f : 0, 0, 0};
            view.r = view.l + sizes[size].x;
            view.t = view.b + sizes[size].y;
            backgroundExtensionCheckBegin(view);
            u8 clear[4];
            glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, clear);
            responsiveBackgroundFamily(scene, p, view, false);
            backgroundExtensionCheckRead(before);
            backgroundExtensionCheckBegin(view);
            responsiveBackgroundFamily(scene, p, view, true);
            backgroundExtensionCheckRead(after);
            if (!size && !phase)
                ordinary &= hashBytes(before, 512 * 384 * 4) == hashBytes(after, 512 * 384 * 4);
            float scale = backgroundSceneryScale(p, view);
            density &= sizes[size].x / (p.stepX * p.columns * scale) <= 3.001f;
            int left = backgroundExtensionFloor((view.l - p.bounds.l) / (p.stepX * scale));
            int right = backgroundExtensionFloor((view.r - p.bounds.l) / (p.stepX * scale));
            int bottom = backgroundExtensionFloor((view.b - p.bounds.b) / (p.stepY * scale));
            int top = backgroundExtensionFloor((view.t - p.bounds.b) / (p.stepY * scale));
            for (int y = bottom; y <= top; y++)
                for (int x = left; x <= right; x++) {
                    float wx = p.bounds.l + (x + .5f) * p.stepX * scale;
                    float wy = p.bounds.b + (y + .5f) * p.stepY * scale;
                    if (wx <= view.l || wx >= view.r || wy <= view.b || wy >= view.t)
                        continue;
                    int px = (int)((wx - view.l) * 512 / (view.r - view.l));
                    int py = (int)((wy - view.b) * 384 / (view.t - view.b));
                    int at = (py * 512 + px) * 4;
                    bool drawn = after[at] != clear[0] || after[at + 1] != clear[1] ||
                                 after[at + 2] != clear[2];
                    bool expected = backgroundPatternCell(p, x, y);
                    pixels &= drawn == expected;
                    samples++;
                    openings += !expected;
                    if (expected && (wx < p.bounds.l || wx >= p.bounds.r || wy < p.bounds.b ||
                                     wy >= p.bounds.t))
                        extended++;
                }
            char line[200];
            wsprintfA(line, "size=%d phase=%d samples=%d extended=%d openings=%d pixels=%d\r\n",
                      size, phase, samples, extended, openings, pixels);
            lstrcatA(rows, line);
        }
    check(
        pixels && extended > 100 && openings > 20,
        "Actual OpenGL sparse walls preserve windows and continue beyond every source edge across15 wide/tall/negative-camera cases");
    check(
        ordinary && density && geometry == backgroundExtensionGeometryHash(scene),
        "4-10 native-frame pixels and geometry remain unchanged; ultra-wide uniform scaling limits full window motifs to three without distorting bricks");
    atomicWrite("test-output/backdrop-pattern-coverage.txt", rows, lstrlenA(rows));
    HeapFree(GetProcessHeap(), 0, before);
    HeapFree(GetProcessHeap(), 0, after);
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
}

// Exercise the actual parallax/foreground transforms, not the untransformed
// source rectangles: ultra-wide camera origins caused the original regression.
void drawCourtyardCheck(Level &scene, float camera, bool anchored) {
    Vec layer = layerTranslation(scene, 2, camera, 0);
    Vec foreground = layerTranslation(scene, 6, camera, 0);
    Rect bounds = {-layer.x, -layer.y, viewW - layer.x, viewH - layer.y};
    glPushMatrix();
    glTranslatef(layer.x, layer.y, 0);
    for (int i = 0; i < scene.tileCount; i++)
        if (courtyardSkylineTile(scene, scene.tiles[i]))
            drawTile(scene.tiles[i], &scene, &bounds, anchored ? &foreground : 0);
    glPopMatrix();
}

void storyBackdropChecks() {
    Arena memory = {};
    memory.init(24 * 1024 * 1024);
    Level scene = {};
    readLevelData(levels[stageList[stageById(976871008727ULL)]].file, scene, memory);
    u32 geometry = backgroundExtensionGeometryHash(scene);
    int panels = 0;
    bool local = true, region = false;
    for (int i = 0; i < backgroundExtensionPolicyCount; i++)
        if (backgroundExtensionPolicies[i].scene == scene.id &&
            backgroundExtensionPolicies[i].layer == 2)
            local = false;
    for (int i = 0; i < scene.tileCount; i++) {
        panels += courtyardSkylineTile(scene, scene.tiles[i]);
        Tile &tile = scene.tiles[i];
        if (tile.layer == 1) {
            Rect r = backgroundExtensionSourceRect(tile.vertices);
            region |= r.l == 6176 && r.r == 8000 && r.b == 0 && r.t == 464;
        }
    }
    check(
        local && panels == 3 && region,
        "Boss courtyard has exactly three guarded skyline panels and the authored final sky region x6176..8000; pillars and other scenes are excluded");
    u8 *before = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    u8 *after = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    float oldW = viewW, oldH = viewH;
    viewW = 512;
    viewH = 384;
    Rect projection = {0, 0, viewW, viewH};
    backgroundExtensionCheckBegin(projection);
    drawCourtyardCheck(scene, 7168, false);
    backgroundExtensionCheckRead(before);
    backgroundExtensionCheckBegin(projection);
    drawCourtyardCheck(scene, 7168, true);
    backgroundExtensionCheckRead(after);
    check(
        hashBytes(before, 512 * 384 * 4) == hashBytes(after, 512 * 384 * 4),
        "Courtyard reference camera preserves every authored skyline pixel and UV without scaling or repetition");
    const float widths[] = {512, 864, 3030, 8192, 16384};
    const float centres[] = {3856, 4368, 4880, 5392, 5904, 6400, 7168, 7424};
    bool confined = true, present = true, detectsOld = false;
    int cases = 0, skylinePixels = 0;
    char report[10000] = {};
    for (int w = 0; w < 5; w++)
        for (int c = 0; c < 8; c++) {
            viewW = widths[w];
            float camera = centres[c] - viewW * .5f;
            Vec foreground = layerTranslation(scene, 6, camera, 0);
            projection.r = viewW;
            backgroundExtensionCheckBegin(projection);
            u8 clear[4];
            glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, clear);
            drawCourtyardCheck(scene, camera, true);
            backgroundExtensionCheckRead(after);
            int pixels = 0, leaks = 0;
            for (int y = 0; y < 384; y++)
                for (int x = 0; x < 512; x++) {
                    int at = (y * 512 + x) * 4;
                    bool drawn = after[at] != clear[0] || after[at + 1] != clear[1] ||
                                 after[at + 2] != clear[2];
                    float worldX = (x + .5f) * viewW / 512 - foreground.x;
                    // A single framebuffer pixel can straddle the world boundary.
                    float tolerance = viewW / 512;
                    leaks += drawn && (worldX < 6176 - tolerance || worldX > 8000 + tolerance);
                    pixels += drawn;
                }
            confined &= leaks == 0;
            if (centres[c] >= 6400 || viewW >= 8192)
                present &= pixels > 100;
            skylinePixels += pixels;
            backgroundExtensionCheckBegin(projection);
            drawCourtyardCheck(scene, camera, false);
            backgroundExtensionCheckRead(before);
            detectsOld |= hashBytes(before, 512 * 384 * 4) != hashBytes(after, 512 * 384 * 4);
            char row[180];
            wsprintfA(row, "width=%d centre=%d pixels=%d leaks=%d\r\n", (int)viewW, (int)centres[c],
                      pixels, leaks);
            lstrcatA(report, row);
            cases++;
        }
    check(
        confined && detectsOld && cases == 40,
        "Actual OpenGL projection confines skyline after x6176 across all five Boss arenas and40 normal/wide/panorama camera cases; regression detects former parallax leakage");
    check(
        present && skylinePixels > 10000,
        "The later courtyard remains visible when in view, including extreme panoramas; the fix does not hide the ending background");
    check(geometry == backgroundExtensionGeometryHash(scene),
          "Courtyard anchoring leaves all authored vertices, metadata and texture UVs unchanged");
    atomicWrite("test-output/boss-city-boundary-coverage.txt", report, lstrlenA(report));
    viewW = oldW;
    viewH = oldH;
    HeapFree(GetProcessHeap(), 0, before);
    HeapFree(GetProcessHeap(), 0, after);
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
}

void backdropRegionPreviews() {
    int oldW = clientW, oldH = clientH;
    interfaceEnabled = musicEnabled = soundEnabled = false;
    const u64 scenes[] = {901687619158ULL, 976871008727ULL};
    for (int stage = 0; stage < 2; stage++)
        for (int size = 0; size < (stage ? 4 : 2); size++) {
            const int heights[] = {320, 96, 32, 540};
            resizeNativeViewport(960, heights[size]);
            startStage(stageById(scenes[stage]));
            readyTimer = deathTimer = transitionTime = 0;
            screen = PLAY;
            if (stage)
                for (int i = 0; i < entityCount; i++)
                    if (entities[i].kind == BOSS)
                        camX = (size == 3 ? 6912 : 3856) - viewW * .5f;
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            nativeViewportProjection();
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            drawWorld();
            char file[100];
            wsprintfA(file, "test-output/backdrop-region-stage%d-size%d.bmp", stage, size);
            captureResponsiveBack(file);
        }
    resizeNativeViewport(oldW, oldH);
}
