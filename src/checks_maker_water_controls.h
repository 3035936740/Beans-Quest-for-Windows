#pragma once

// Real course bakes, editor commands, physics contacts and framebuffer comparisons.
int makerWaterChangedPixels(const u8 *before, int lo, int hi) {
    static u8 pixels[512 * 384 * 4];
    glReadPixels(0, 0, 512, 384, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int count = 0;
    for (int y = lo; y < hi; y++)
        for (int x = 0; x < 512; x++) {
            int i = (y * 512 + x) * 4;
            count += pixels[i] != before[i] || pixels[i + 1] != before[i + 1] ||
                     pixels[i + 2] != before[i + 2];
        }
    return count;
}

void makerWaterRenderChecks() {
    int oldW = clientW, oldH = clientH;
    resizeNativeViewport(1024, 768);
    screenScale = 1;
    viewportW = 512;
    viewportH = 384;
    viewW = 512;
    viewH = 384;
    nativeViewportProjection();
    edOpen(MAKER_SLOTS - 1, true, 4);
    MakerLevel &course = makerEd.level;
    course.tileCount = course.objectCount = 0;
    course.head.startX = 48;
    course.head.startY = 320;
    MakerObject water = {};
    water.klass = 667806108659ULL;
    water.x = 256;
    water.y = 144;
    water.w = 320;
    water.h = 160;
    edBeginGroup();
    edAddObject(water);
    water.x = 464;
    water.w = 64;
    water.h = 32;
    edAddObject(water);
    makerStartRun(course, MAKER_SLOTS - 1, true, false, Vec{0, 0});
    readyTimer = transitionTime = 0;
    camX = camY = 0;
    int waterIndex = -1, waves = 0, bubbles = 0;
    bool order = true;
    for (int i = 0; i < entityCount; i++) {
        Entity &e = entities[i];
        if (e.kind == WATER && waterIndex < 0)
            waterIndex = i;
        if (e.source->klass == 528389847068ULL || e.source->klass == 677112438672ULL) {
            waves += e.source->klass == 528389847068ULL;
            bubbles += e.source->klass == 677112438672ULL;
            order &= e.layer == current.layerCount - 1 && e.z > -10;
        }
    }
    check(waves == 2 && bubbles == 1 && order,
          "Deep and shallow custom pools place native particles in front of the water tint");
    check(waterIndex >= 0 && bodies[waterIndex]->GetFixtureList()->IsSensor() &&
              entities[waterIndex].source->behavior("WaterAreaBehaviour")->num("bean-density") == 15,
          "Custom water retains its original buoyancy sensor while changing only decoration");
    for (int frame = 1; frame <= 240; frame++)
        nativeContinuousAdvanceScene(current, 1, frame / 60.0f, 1 / 60.0f);
    for (int i = 0; i < entityCount; i++) {
        entities[i].state.timer = 4;
        if (entities[i].source->klass == 528389847068ULL ||
            entities[i].source->klass == 677112438672ULL)
            entities[i].state.gone = 1;
    }
    static u8 baseline[512 * 384 * 4];
    makerWeatherCheckClear();
    drawWorld();
    glReadPixels(0, 0, 512, 384, GL_RGBA, GL_UNSIGNED_BYTE, baseline);
    for (int kind = 0; kind < 2; kind++) {
        for (int i = 0; i < entityCount; i++) {
            u64 klass = entities[i].source->klass;
            if (klass == 528389847068ULL || klass == 677112438672ULL)
                entities[i].state.gone = klass != (kind ? 677112438672ULL : 528389847068ULL);
        }
        makerWeatherCheckClear();
        drawWorld();
        check(makerWaterChangedPixels(baseline, kind ? 64 : 215, kind ? 215 : 232) > 10,
              kind ? "Actual custom-course bubbles remain visible after the water fill is drawn"
                   : "Actual custom-course ripples brighten the surface after the water fill is drawn");
    }
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->base == 5)
            entities[i].state.gone = 0;
    makerWeatherCheckClear();
    drawWorld();
    SwapBuffers(gameDC);
    capture("test-output/maker-water-play.bmp");

    makerRun = MakerRunState{};
    makerPlayLevel = 0;
    edRefreshWater();
    Object *source = &makerEd.water.objects[0];
    for (int frame = 1; frame <= 240; frame++) {
        makerEd.waterAge = frame / 60.0f;
        for (int i = 0; i < makerEd.water.objectCount; i++) {
            Object &o = makerEd.water.objects[i];
            if (o.base != 5)
                continue;
            Vec c = center(o.rect);
            Rect bounds = shifted(nativeParticleEmitterBounds(o.rect, 1), -c.x, -c.y);
            nativeContinuousFor(o, bounds, 1, makerEd.waterAge);
        }
    }
    Rect r = edObjectRect(course.objects[0]);
    makerWeatherCheckClear();
    edDrawWater(r, 1, -1);
    glReadPixels(0, 0, 512, 384, GL_RGBA, GL_UNSIGNED_BYTE, baseline);
    edDrawWater(r, 1, 0);
    check(makerWaterChangedPixels(baseline, 215, 232) > 10 &&
              makerWaterChangedPixels(baseline, 64, 215) > 10,
          "Editor preview renders animated surface ripples and underwater bubbles over its real gradient");
    Rect moved = shifted(r, 32, 48);
    moved.r += 64;
    makerWeatherCheckClear();
    edDrawWater(moved, 1, -1);
    glReadPixels(0, 0, 512, 384, GL_RGBA, GL_UNSIGNED_BYTE, baseline);
    edDrawWater(moved, 1, 0);
    check(source == &makerEd.water.objects[0] &&
              makerWaterChangedPixels(baseline, 263, 280) > 10,
          "Water move/resize preview keeps a stable particle source and follows the preview surface");
    makerWeatherCheckClear();
    edDrawWater(r, 1, 0);
    edDrawWater(edObjectRect(course.objects[1]), 1, 1);
    SwapBuffers(gameDC);
    capture("test-output/maker-water-editor.bmp");
    check(glGetError() == GL_NO_ERROR, "Water course and editor decoration render without GL errors");
    resizeNativeViewport(oldW, oldH);
}

void makerGridChecks() {
    // Select through the actual recent-brush palette path, then paint with the resulting snap.
    makerBuildCatalog();
    const int cases[][3] = {{16, 32, 16}, {64, 16, 16}, {4, 32, 4}, {64, 64, 32},
                            {32, 96, 32}, {2, 32, 2}, {1, 32, 1}, {24, 32, 32},
                            {20, 64, 16}, {12, 32, 16}, {6, 32, 8}, {3, 32, 4}};
    edOpen(MAKER_SLOTS - 1, true, 2);
    MakerLevel &course = makerEd.level;
    course.objectCount = course.tileCount = 0;
    makerEd.opCount = makerEd.opTop = 0;
    bool sizes = true;
    for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        MakerBrush b = {};
        b.def = b.piece = -1;
        b.tile.r = (float)cases[i][0];
        b.tile.t = (float)cases[i][1];
        b.tile.group = 2;
        makerEd.recent[0] = b;
        makerEd.recentCount = 1;
        makerEd.palette[0] = MakerPaletteItem{2, 0};
        makerEd.paletteCount = 1;
        makerEd.grid = 32;
        edPickPalette(0);
        sizes &= makerEd.grid == cases[i][2];
    }
    check(sizes, "Brush selection uses the shorter side: 16x32/64x16->16, 4x32->4, 64x64->32, with midpoint sizes rounded upward");
    bool realPieces = false;
    for (int i = 0; i < makerPieceCount; i++)
        if (makerPieces[i].category == PIECE_BLOCK && makerPieces[i].w == 16 && makerPieces[i].h == 16) {
            makerEd.brush.trim = 0;
            edPieceBrush(i);
            realPieces = makerEd.grid == 16;
            break;
        }
    check(realPieces, "Selecting an actual 16x16 catalog block switches the toolbar grid to 16");

    MakerBrush &brush = makerEd.brush;
    brush = MakerBrush{};
    brush.tile.r = 8;
    brush.tile.t = 32;
    brush.tile.group = 2;
    brush.tile.collide = COLLIDE_SOLID;
    edCycleBrushTrim(1);
    check(makerEd.grid == 4, "Selecting a half of an 8x32 brick automatically chooses grid 4");
    edCycleBrushTrim(-1);
    check(makerEd.grid == 8, "Returning to the whole 8x32 brick restores grid 8");
    bool cycle = true;
    const int steps[] = {16, 8, 4, 2, 1, 32};
    makerEd.grid = 32;
    for (int i = 0; i < 6; i++) {
        edCommand(CMD_GRID, 0);
        cycle &= makerEd.grid == steps[i];
    }
    check(cycle, "Grid button cycles through 32/16/8/4/2/1 and back to 32");

    brush.tile.r = 4;
    brush.tile.t = 32;
    edAutoBrushGrid();
    edBeginGroup();
    edPaintTile(18, 144);
    edPaintTile(22, 144);
    check(course.tileCount == 2 && course.tiles[0].r == course.tiles[1].l && makerEd.grid == 4,
          "Consecutive thin bricks paint into adjacent four-pixel cells using the automatically selected grid");
    makerEd.grid = 2;
    edPaintTile(26, 144);
    check(makerEd.grid == 2, "Manual grid overrides remain active while painting the same brush");
    makerEd.grid = 32;
    edEyedropper(18, 144);
    check(makerEd.grid == 4, "Eyedropper picks the existing thin brick and its matching four-pixel grid");
}

void makerControlChecks() {
    makerBuildCatalog();
    bool themes = true;
    for (int theme = 1; theme <= 5; theme++) {
        makerEd.world = 0;
        makerEd.scroll = 20;
        edOpen(MAKER_SLOTS - 1, true, theme);
        themes &= makerEd.world == theme && makerEd.scroll == 0 &&
                  makerEd.level.head.theme == theme;
    }
    check(themes, "Every new-course theme opens its matching world palette instead of All or an old scroll position");
    MakerLevel &course = makerEd.level;
    course.objectCount = course.tileCount = 0;
    makerEd.opCount = makerEd.opTop = 0;
    makerEd.entityCost = edEntityCost();
    edClearSelection();
    makerEd.grid = 32;
    int sw = (int)(makerDefFor(MAKER_CLASS_SWITCH) - makerObjectDefs);
    edDefBrush(sw);
    bool snapped = true;
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++) {
            float x = 112 + col * 32.0f, y = 112 + row * 32.0f;
            Vec preview = edObjectBrushCenter(x, y);
            edBeginGroup();
            bool placed = edPlaceObject(x, y);
            const MakerObject &o = course.objects[course.objectCount - 1];
            snapped &= placed && preview.x == x && preview.y == y &&
                       o.x == x && o.y == y && o.w == 32 && o.h == 48;
        }
    check(snapped, "Switch placement and ghost share the 32px solid-core grid across four rows and columns");

    edOpen(MAKER_SLOTS - 1, true, 1);
    course.objectCount = course.tileCount = 0;
    makerEd.entityCost = edEntityCost();
    int block = (int)(makerDefFor(11234016302369745107ULL) - makerObjectDefs);
    edDefBrush(block);
    edFillRect(Rect{96, 160, 288, 224});
    check(course.objectCount == 12, "Shift-fill places all twelve fixed touch blocks in a six by two rectangle");
    edFillRect(Rect{96, 160, 288, 224});
    check(course.objectCount == 12, "Repeating the same Shift-fill does not duplicate existing objects");
    edUndo();
    check(course.objectCount == 0, "One undo removes the complete object rectangle as one edit");
    edRedo();
    check(course.objectCount == 12, "One redo restores the complete object rectangle");

    makerEd.buttons.clear();
    makerEd.tool = TOOL_SELECT;
    makerEd.zoom = 1;
    makerEd.camX = makerEd.camY = 0;
    makerEd.paletteH = 60;
    makerEd.toolbarH = 22;
    makerEd.propObject = -1;
    edClearSelection();
    BYTE prior[256], modifiers[256];
    GetKeyboardState(prior);
    memset(modifiers, 0, sizeof(modifiers));
    modifiers[VK_SHIFT] = 0x80;
    SetKeyboardState(modifiers); // This test thread's queue only; no user-window input.
    bool selection = mkKeyDown(VK_SHIFT);
    makerEd.lastClickTime = makerEd.age;
    for (int i = 0; i < 3; i++) {
        float x = edScreenX(course.objects[i].x), y = edScreenY(course.objects[i].y);
        // Deliberately trigger the former double-click path with Shift held.
        makerEd.lastClickX = x;
        makerEd.lastClickY = y;
        edMouseDown(x, y, 0);
        edMouseUp(x, y, 0);
        selection &= makerEd.selObjects == i + 1;
    }
    SetKeyboardState(prior);
    edSelectRect(Rect{96, 160, 288, 224}, true);
    check(selection && makerEd.selObjects == 12,
          "Shift-click including rapid clicks and additive box selection retain all earlier objects");
    edCopySelection();
    check(makerEd.clipObjectCount == 12, "Copy includes the whole selected object rectangle");
    edDefBrush((int)(makerDefFor(667806108659ULL) - makerObjectDefs));
    edFillRect(Rect{320, 96, 512, 224});
    check(course.objectCount == 13 && course.objects[12].w == 192 && course.objects[12].h == 128,
          "Shift-fill still creates one sized water area rather than repeated water objects");
}

void makerPotionFormChecks() {
    edOpen(MAKER_SLOTS - 1, true, 1);
    MakerLevel &course = makerEd.level;
    MakerTile ground = course.tiles[0];
    ground.l = 0;
    ground.r = 512;
    ground.b = 64;
    ground.t = 96;
    course.tiles[0] = ground;
    course.tileCount = 1;
    course.objectCount = 0;
    course.head.startX = 48;
    course.head.startY = 300;
    const MakerObjectDef *d = makerDefFor(MAKER_CLASS_POTION);
    check(d && makerParamUsable(*d, 0), "Potion options expose an Emilio or Bean transformation target");
    MakerObject potion = {};
    potion.klass = MAKER_CLASS_POTION;
    potion.w = 32;
    potion.h = 40;
    potion.y = 192;
    // Reverse spatial order to catch checkpoint code incorrectly choosing the last array entry.
    for (int i = 0; i < 3; i++) {
        potion.x = 160 + i * 96.0f;
        potion.setMask = i == 0 ? 0 : 1;
        potion.param[0] = i == 1 ? 1.0f : 0.0f;
        edBeginGroup();
        edAddObject(potion);
    }
    edSelectSingleObject(1);
    edCommand(CMD_PROP_DEC, 0);
    edCommand(CMD_PROP_INC, 0);
    check(makerParam(course.objects[1], *d, 0) == 1,
          "Potion target is changed through the real property-panel commands");
    bool saved = makerSaveLevel(MAKER_SLOTS - 1, course);
    MakerLevel loaded = {};
    makerAllocate(loaded);
    saved &= makerLoadLevel(MAKER_SLOTS - 1, loaded) &&
             makerParam(loaded.objects[1], *d, 0) == 1;
    check(saved, "Bean-target potions survive course save and reload without a format migration");
    makerStartRun(loaded, MAKER_SLOTS - 1, true, false, Vec{0, 0});
    readyTimer = transitionTime = 0;
    int bottles[3], count = 0;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == POTION && count < 3)
            bottles[count++] = i;
    check(count == 3, "Both potion targets bake into real independently collectible bottle sensors");
    bool forms = count == 3, checkpointForms = count == 3, disk = false, bounce = false;
    const int route[] = {2, 1, 0};
    for (int step = 0; step < count; step++) {
        Entity &bottle = entities[bottles[route[step]]];
        State &bean = entities[playerIndex].state;
        bean.x = bottle.state.x;
        bean.y = bottle.state.y;
        bean.vx = bean.vy = 0;
        deathTimer = 0;
        restorePhysics();
        tick(1 / 120.0f, false, false);
        bool walk = step != 1;
        forms &= bottle.state.gone && !bodies[bottles[route[step]]]->IsActive() &&
                 playerWalkMode == walk;
        for (b2Fixture *f = bodies[playerIndex]->GetFixtureList(); f; f = f->GetNext())
            if (!f->IsSensor())
                forms &= f->GetFilterData().categoryBits == (walk ? 4 : 2);
        captureCheckpoint(false);
        playerWalkMode = !walk;
        restoreCheckpoint();
        checkpointForms &= playerWalkMode == walk;
        if (step == 1) {
            const char *path = "test-output/maker-bean-potion-checkpoint.dat";
            disk = atomicWrite(path, &checkpoint, sizeof(checkpoint)) && readCheckpoint(path);
            if (disk) {
                playerWalkMode = true;
                restoreCheckpoint();
                disk &= !playerWalkMode && checkpoint.version == 28;
            }
            bean.x = 128;
            bean.y = 140;
            bean.vx = bean.vy = 0;
            restorePhysics();
            int before = jumps;
            for (int frame = 0; frame < 120; frame++)
                tick(1 / 120.0f, false, false);
            bounce = jumps > before && !playerWalkMode;
        }
    }
    check(forms, "Real reverse-order contacts transform Emilio to Bean and back, restoring each form's collision category");
    check(checkpointForms, "Checkpoints restore the latest collected potion target rather than object-array order");
    check(disk, "A disk checkpoint reload retains the Bean target with the existing version28 snapshot format");
    check(bounce, "Returning from Emilio to Bean restores automatic jumps on actual terrain contact");
    check(!*makerClass(MAKER_CLASS_POTION)->props.get("maker-potion-form"),
          "Custom potion targets leave the original campaign potion unchanged");
    makerRun = MakerRunState{};
    makerPlayLevel = 0;
    mkFree(loaded.tiles);
    mkFree(loaded.objects);
}
