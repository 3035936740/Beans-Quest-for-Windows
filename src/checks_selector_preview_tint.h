#pragma once

// Regression coverage for selector preview tint. Runs only in the isolated BQ_ENABLE_TESTS executable.

void selectorPreviewTintSky(Level &level) {
    cloudRenderBegin();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(-256, 0, 0);
    drawTile(level.tiles[21], &level);
    drawTile(level.tiles[54], &level);
    drawTile(level.tiles[55], &level);
    glPopMatrix();
    glFinish();
}

int selectorPreviewTintSeam() {
    u8 left[4] = {}, right[4] = {};
    int y = viewportY + (int)(350 * viewportH / 384.0f);
    glReadBuffer(GL_BACK);
    glReadPixels(viewportX + (int)(255.5f * viewportW / 512.0f), y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                 left);
    glReadPixels(viewportX + (int)(256.5f * viewportW / 512.0f), y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                 right);
    return (int)(absf((float)left[0] - right[0]) + absf((float)left[1] - right[1]) +
                 absf((float)left[2] - right[2]));
}

void selectorPreviewTintChecks() {
    Arena memory = {};
    memory.init(32 * 1024 * 1024);
    Level scan = {};
    int repaired = 0;
    bool guarded = true, scope = true;
    for (int i = 0; i < levelCount; i++) {
        readLevelData(levels[i].file, scan, memory);
        u32 oldGeometry = 0, oldTiles = 0;
        for (int j = 0; j < scan.geometryCount; j++)
            oldGeometry ^= hashBytes(scan.geometry[j].points, scan.geometry[j].count * sizeof(Vec));
        for (int j = 0; j < scan.tileCount; j++)
            if (j != 55 || scan.id != 669776786229ULL)
                oldTiles ^= hashBytes(scan.tiles[j].vertices, scan.tiles[j].bytes);
        bool changed = repairFifthSelectorTint(scan);
        if (changed)
            repaired++;
        guarded &= changed == (scan.id == 669776786229ULL);
        guarded &= !repairFifthSelectorTint(scan);
        u32 newGeometry = 0, newTiles = 0;
        for (int j = 0; j < scan.geometryCount; j++)
            newGeometry ^= hashBytes(scan.geometry[j].points, scan.geometry[j].count * sizeof(Vec));
        for (int j = 0; j < scan.tileCount; j++)
            if (j != 55 || scan.id != 669776786229ULL)
                newTiles ^= hashBytes(scan.tiles[j].vertices, scan.tiles[j].bytes);
        scope &= oldGeometry == newGeometry && oldTiles == newTiles;
    }
    check(
        levelCount == 74 && repaired == 1 && guarded && scope,
        "The World5 selector tint repair applies once to its authored short overlay across74 scenes without changing terrain or other tile artwork");
    readLevelData(levelById(669776786229ULL)->file, scan, memory);
    selectorPreviewTintSky(scan);
    int before = selectorPreviewTintSeam();
    repairFifthSelectorTint(scan);
    selectorPreviewTintSky(scan);
    int after = selectorPreviewTintSeam();
    char proof[160];
    wsprintfA(
        proof,
        "original_edge_rgb_difference=%d\r\ncorrected_edge_rgb_difference=%d\r\nviewport=%dx%d\r\n",
        before, after, viewportW, viewportH);
    atomicWrite("test-output/selector-preview-tint-pixels.txt", proof, lstrlenA(proof));
    check(
        before > 15 && after == 0,
        "Actual OpenGL sky pixels have no vertical colour jump at the former512-unit orange-overlay edge, with the original defect as a negative control");
    releaseLevel(scan, memory);
    VirtualFree(memory.data, 0, MEM_RELEASE);
    navigate(669776786229ULL);
    page.age = 2;
    transitionTime = 0;
    page.minimized = true;
    page.minimizePhase = 1;
    Tile &tint = page.level.tiles[55];
    check(
        tint.rect.r == page.level.size.x && tint.vertices[1].x == 864 && tint.vertices[3].x == 864,
        "The production selector loader extends the same tint across both the sky and columns of the entire864-unit World5 preview");
    render();
    capture("test-output/selector-world5-preview-fixed.bmp");
    startStage(0);
    readyTimer = 0;
}
