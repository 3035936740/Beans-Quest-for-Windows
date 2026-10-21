#pragma once

// Real OpenGL coverage, density and native-frame comparisons for all audited
// scenery families. Test worlds, pixels and captures remain in the test runner.
void responsiveBackgroundFamily(Level &scene, const BackgroundExtensionPolicy &p, Rect view,
                                bool extend) {
    for (int i = 0; i < scene.tileCount; i++) {
        Tile &tile = scene.tiles[i];
        bool member = false;
        for (int q = 0; q < tile.bytes / (int)sizeof(Vertex); q += 4)
            member |= backgroundSceneryOwnsQuad(p, scene, tile, q);
        if (member)
            drawTile(tile, &scene, extend ? &view : 0);
    }
}

int responsiveBackgroundMissing(const u8 *row, const u8 *clear) {
    int missing = 0;
    for (int x = 0; x < 512; x++)
        missing +=
            row[x * 4] == clear[0] && row[x * 4 + 1] == clear[1] && row[x * 4 + 2] == clear[2];
    return missing;
}

void responsiveBackgroundChecks() {
    Arena memory = {};
    memory.init(24 * 1024 * 1024);
    Level scene = {};
    const float widths[] = {320, 512, 864, 1536, 3030, 4096, 8192};
    u8 row[512 * 4], clear[4];
    u8 *before = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    u8 *after = (u8 *)HeapAlloc(GetProcessHeap(), 0, 512 * 384 * 4);
    static char report[131072];
    report[0] = 0;
    int families = 0, cases = 0, oldMissing = 0, newMissing = 0, changedInside = 0;
    bool density = true, immutable = true, upperBands = true, tallCoverage = true;
    for (int level = 0; level < levelCount; level++) {
        bool needed = false;
        for (int i = 0; i < backgroundExtensionPolicyCount; i++)
            needed |= backgroundExtensionPolicies[i].scene == levels[level].id &&
                      backgroundHorizonPolicy(backgroundExtensionPolicies[i]);
        if (!needed)
            continue;
        readLevelData(levels[level].file, scene, memory);
        u32 original = backgroundExtensionGeometryHash(scene);
        for (int i = 0; i < backgroundExtensionPolicyCount; i++) {
            const BackgroundExtensionPolicy &p = backgroundExtensionPolicies[i];
            if (p.scene != scene.id || !backgroundHorizonPolicy(p))
                continue;
            families++;
            const Vertex *q = scene.tiles[p.tile].vertices + p.vertex;
            Rect source = backgroundExtensionSourceRect(q);
            for (int w = 0; w < 7; w++)
                for (int phase = 0; phase < 3; phase++) {
                    float centre = (p.bounds.l + p.bounds.r) * .5f + (phase == 1   ? -2.25f
                                                                      : phase == 2 ? 3.125f
                                                                                   : 0) *
                                                                         p.stepX;
                    Rect view = {centre - widths[w] * .5f, source.b, centre + widths[w] * .5f,
                                 source.t};
                    backgroundExtensionCheckBegin(view);
                    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, clear);
                    responsiveBackgroundFamily(scene, p, view, false);
                    int probeY = p.mode == 7 ? 367 : 16;
                    glReadPixels(0, probeY, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, row);
                    int nativeHoles = responsiveBackgroundMissing(row, clear);
                    oldMissing += nativeHoles;
                    if (widths[w] == 512)
                        backgroundExtensionCheckRead(before);
                    backgroundExtensionCheckBegin(view);
                    responsiveBackgroundFamily(scene, p, view, true);
                    glReadPixels(0, probeY, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, row);
                    int holes = responsiveBackgroundMissing(row, clear);
                    newMissing += holes;
                    if (widths[w] == 512) {
                        backgroundExtensionCheckRead(after);
                        for (int y = 0; y < 384; y++)
                            for (int x = 0; x < 512; x++) {
                                float wx = view.l + (x + .5f) * (view.r - view.l) / 512;
                                float wy = view.b + (y + .5f) * (view.t - view.b) / 384;
                                if (wx < p.bounds.l || wx >= p.bounds.r || wy < p.bounds.b ||
                                    wy >= p.bounds.t)
                                    continue;
                                int at = (y * 512 + x) * 4;
                                bool same = true;
                                for (int c = 0; c < 4; c++)
                                    same &= before[at + c] == after[at + c];
                                changedInside += !same;
                            }
                    }
                    float scale = backgroundSceneryScale(p, view);
                    float visiblePeriods = (view.r - view.l) / (p.stepX * scale);
                    density &= visiblePeriods <= maxf(864 / p.stepX, 3) + .001f;
                    char line[180], id[24];
                    idstr(scene.id, id);
                    wsprintfA(
                        line,
                        "scene=%s tile=%d width=%d phase=%d nativeHoles=%d holes=%d periods100=%d\r\n",
                        id, p.tile, (int)widths[w], phase, nativeHoles, holes,
                        (int)(visiblePeriods * 100));
                    lstrcatA(report, line);
                    cases++;
                }
            if (!backgroundSceneryLowestBand(p)) {
                Rect below = {source.l - 100, p.bounds.b - 100, source.r + 100, p.bounds.b - 10};
                backgroundExtensionCheckBegin(below);
                glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, clear);
                glBindTexture(GL_TEXTURE_2D, scene.textures[p.texture].gl);
                glEnable(GL_TEXTURE_2D);
                glColor4f(1, 1, 1, 1);
                drawResponsiveScenery(p, scene.tiles[p.tile], below);
                glReadPixels(0, 16, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, row);
                upperBands &= responsiveBackgroundMissing(row, clear) == 512;
            }
            if (backgroundSceneryLowestBand(p) || p.mode == 7) {
                bool ceiling = p.mode == 7;
                Rect tall = {source.l - 1000, ceiling ? source.t + 10 : source.b - 1536,
                             source.r + 1000, ceiling ? source.t + 1536 : source.b - 10};
                backgroundExtensionCheckBegin(tall);
                glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, clear);
                responsiveBackgroundFamily(scene, p, tall, true);
                glReadPixels(0, 192, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, row);
                tallCoverage &= responsiveBackgroundMissing(row, clear) == 0;
            }
        }
        immutable &= original == backgroundExtensionGeometryHash(scene);
    }
    check(
        families >= 30 && cases == families * 21 && oldMissing > 10000 && newMissing == 0,
        "All audited mountain/wood/grass horizon families cover7 viewport widths and3 camera phases; original finite artwork reproduces gaps but responsive rendering leaves no missing base columns");
    check(
        density,
        "Ultra-wide distant scenery stays within three full image periods or its original small-texture density, plus clipped edge pieces");
    check(changedInside == 0,
          "Every sampled authored pixel inside ordinary512-wide scenery frames remains unchanged");
    check(
        immutable && upperBands,
        "Scenery rendering changes no source vertices and upper horizon bands never paint over lower bands");
    check(
        tallCoverage,
        "Tall-window horizon bases and inverted ceiling backdrops continue opaque edges without stacking another row of scenery");
    atomicWrite("test-output/background-responsive-coverage.txt", report, lstrlenA(report));
    HeapFree(GetProcessHeap(), 0, before);
    HeapFree(GetProcessHeap(), 0, after);
    releaseLevel(scene, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
}

void responsiveCloudChecks() {
    float oldW = viewW, oldH = viewH;
    bool noPop = true, continuous = true, normal = true, fixedSizes = true;
    const float widths[] = {320, 512, 864, 1536, 3030, 4096, 8192};
    const u64 classes[] = {159837608269ULL, 18085559403622411025ULL, 5037571874453516171ULL,
                           5028962873047051163ULL};
    for (int w = 0; w < 7; w++) {
        viewW = widths[w];
        viewH = 384;
        for (int kind = 0; kind < 4; kind++) {
            Object &o = *classById(classes[kind]);
            float width = o.rect.r - o.rect.l;
            Rect previous = backgroundRect(o, shifted(o.rect, width * .5f, 100), 0, 0, 0);
            for (int t = 1; t <= 1000; t++) {
                Rect next = backgroundRect(o, shifted(o.rect, width * .5f, 100), t * .1f, 0, 0);
                bool wrapped = absf(next.l - previous.l) > 4;
                if (wrapped)
                    // A sampled frame can contain the final three pixels of
                    // ordinary leftward motion before the centre wraps.
                    noPop &= (previous.r <= 3.02f || previous.l >= viewW - 3.02f) &&
                             (next.r <= 3.02f || next.l >= viewW - 3.02f);
                if (kind)
                    fixedSizes &= absf(next.r - next.l - width) < .002f;
                previous = next;
            }
        }
        Object &bank = *classById(classes[0]);
        const float ages[] = {0, 40, 80, 160, 1000};
        for (int t = 0; t < 5; t++) {
            Rect pieces[4];
            for (int i = 0; i < 4; i++)
                pieces[i] =
                    backgroundRect(bank, shifted(bank.rect, 241 + i * 480, 84), ages[t], 0, 0);
            for (int x = 0; x < 1024; x++) {
                float point = (x + .5f) * viewW / 1024;
                bool covered = false;
                for (int i = 0; i < 4; i++)
                    covered |= pieces[i].l <= point && pieces[i].r >= point;
                continuous &= covered;
            }
        }
        if (viewW <= 864)
            normal &= absf(backgroundRect(bank, Rect{0, 0, 482, 168}, 0, 0, 0).r - 482) < .001f;
    }
    viewW = oldW;
    viewH = oldH;
    check(
        noPop && fixedSizes,
        "Sparse clouds keep their authored size and every wrap happens fully off-screen at7 viewport widths");
    check(
        continuous && normal,
        "The existing four-piece cloud bank remains continuous through5 animation phases at7 widths without adding actors or changing ordinary-width artwork");
}

// Hidden-window front buffers may be stale. Review the back buffer that was actually rendered.
void captureResponsiveBack(const char *file) {
    u32 size = viewportW * viewportH * 4;
    u32 offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    u8 *bytes = (u8 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, offset + size);
    if (!bytes)
        fatal("Cannot capture background preview.");
    BITMAPFILEHEADER *fh = (BITMAPFILEHEADER *)bytes;
    BITMAPINFOHEADER *bi = (BITMAPINFOHEADER *)(bytes + sizeof(BITMAPFILEHEADER));
    fh->bfType = 0x4d42;
    fh->bfSize = offset + size;
    fh->bfOffBits = offset;
    bi->biSize = sizeof(*bi);
    bi->biWidth = viewportW;
    bi->biHeight = viewportH;
    bi->biPlanes = 1;
    bi->biBitCount = 32;
    bi->biCompression = BI_RGB;
    bi->biSizeImage = size;
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, bytes + offset);
    for (u32 i = offset; i < offset + size; i += 4) {
        u8 red = bytes[i];
        bytes[i] = bytes[i + 2];
        bytes[i + 2] = red;
    }
    atomicWrite(file, bytes, offset + size);
    HeapFree(GetProcessHeap(), 0, bytes);
}

void responsiveBackgroundPreviews() {
    int oldW = clientW, oldH = clientH;
    const u64 stages[] = {3442460187460865148ULL, 1065014185637269516ULL, 6838399004045653085ULL,
                          899263731133ULL};
    interfaceEnabled = soundEnabled = musicEnabled = false;
    for (int i = 0; i < 4; i++)
        for (int size = 0; size < 2; size++) {
            resizeNativeViewport(960, size ? 120 : 540);
            startStage(stageById(stages[i]));
            readyTimer = deathTimer = transitionTime = 0;
            screen = PLAY;
            elapsed = 80;
            // Inspect each horizon near the level base rather than at a tall
            // stage's spawn camera, where the mountains can be below the view.
            camX = (current.size.x - viewW) * .5f;
            camY = 0;
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            nativeViewportProjection();
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            drawWorld();
            char file[100];
            wsprintfA(file, "test-output/background-responsive-stage%d-size%d.bmp", i, size);
            captureResponsiveBack(file);
        }
    resizeNativeViewport(oldW, oldH);
}
