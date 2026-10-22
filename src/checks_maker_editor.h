#pragma once

// Exercise the editor and custom-course runtime using only the isolated test save directory.
u32 makerEditorIconPixels(int def, float age, int *visible = 0) {
    glViewport(0, 0, 128, 128);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 128, 0, 128, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(.04f, .06f, .09f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    makerEd.age = age;
    edDrawDefIcon(def, Rect{8, 8, 120, 120}, age);
    static u8 pixels[128 * 128 * 4];
    glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int count = 0;
    for (int i = 0; i < 128 * 128; i++)
        count += pixels[i * 4] != pixels[0] || pixels[i * 4 + 1] != pixels[1] ||
                 pixels[i * 4 + 2] != pixels[2];
    if (visible)
        *visible = count;
    return hashBytes(pixels, sizeof(pixels));
}

void makerEditorArtworkPreview() {
    int oldW = clientW, oldH = clientH;
    resizeWindow(1024, 720);
    nativeViewportProjection();
    glClearColor(.04f, .06f, .09f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    int index = 0;
    for (int i = 0; i < makerObjectDefCount; i++) {
        const MakerObjectDef &d = makerObjectDefs[i];
        if (d.category != OBJ_GIZMO && !eq(d.name, "Spiked Block"))
            continue;
        float x = 8 + (index % 7) * 72.0f, y = viewH - 82 - (index / 7) * 76.0f;
        mkFrame(x, y, 68, 72, 0x4d667fff);
        edDrawDefIcon(i, Rect{x + 5, y + 16, x + 63, y + 68}, 1.3f);
        mkTextCentered(x + 34, y + 7, d.name, 6, 0xffffffff);
        index++;
    }
    SwapBuffers(gameDC);
    capture("test-output/maker-gizmos-preview.bmp");
    resizeWindow(oldW, oldH);
}

void makerEditorSwitchChecks() {
    edOpen(MAKER_SLOTS - 1, true, 2);
    MakerLevel &course = makerEd.level;
    course.tileCount = course.objectCount = 0;
    course.head.startX = 256;
    course.head.startY = 248;
    MakerObject sw = {};
    sw.klass = MAKER_CLASS_SWITCH;
    sw.x = 256;
    sw.y = 192;
    sw.w = 32;
    sw.h = 48;
    sw.setMask = 1;
    sw.param[0] = 1; // Group B, so the test also checks edited switch-group wiring.
    edBeginGroup();
    edAddObject(sw);
    MakerObject gate = {};
    gate.klass = 9010792101233554060ULL;
    gate.x = 384;
    gate.y = 192;
    gate.w = gate.h = 32;
    gate.setMask = 1;
    gate.param[0] = 1;
    edAddObject(gate);
    check(makerFreeSpawn(course, Vec{256, 192}).y >= 224,
          "Custom-course spawn recovery also lifts Bean out of a switch's solid backing");
    makerStartRun(course, MAKER_SLOTS - 1, true, false, Vec{0, 0});
    readyTimer = transitionTime = 0;
    int index = -1, block = -1, icons = 0;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].kind == SWITCH)
            index = i;
        if (entities[i].kind == ALTERNATOR)
            block = i;
        icons += entities[i].source->klass == MAKER_CLASS_SWITCH_ICON;
    }
    check(index >= 0 && block >= 0 && icons == 1 && current.geometryCount > 0,
          "Custom switch bakes its authored animated centre icon and solid backing alongside its linked block");
    if (index >= 0 && block >= 0) {
        int sensors = 0;
        for (b2Fixture *f = bodies[index]->GetFixtureList(); f; f = f->GetNext())
            sensors += f->IsSensor();
        check(sensors == 2 && entities[index].mask == 2 && entities[block].mask == 2,
              "Baked switch keeps both native trigger faces and the selected group mask");
        float minimum = 1e6f;
        for (int n = 0; n < 90; n++) {
            tick(1 / 120.0f, false, false);
            minimum = minf(minimum, entities[playerIndex].state.y);
        }
        check(switchOn(entities[index]) && !bodies[block]->IsActive() && minimum >= 216,
              "Falling Bean presses the upper switch face, toggles the matching gate and bounces on its solid backing instead of passing through");
        State &bean = entities[playerIndex].state;
        bean.x = 256;
        bean.y = 152;
        bean.vx = 0;
        bean.vy = 180;
        bean.timer = .1f;
        restorePhysics();
        float maximum = -1e6f;
        for (int n = 0; n < 45; n++) {
            tick(1 / 120.0f, false, false);
            maximum = maxf(maximum, bean.y);
        }
        check(!switchOn(entities[index]) && bodies[block]->IsActive() && maximum <= 168,
              "Bean also presses the lower active face and stays below the solid switch backing");
    }
    makerRun = MakerRunState{};
    makerPlayLevel = 0;
}

void makerEditorChecks() {
    soundEnabled = musicEnabled = false;
    memset(keys, 0, sizeof(keys));
    makerBuildCatalog();
    check(edOpen(MAKER_SLOTS - 1, true, 2), "Fresh isolated editor course opens");
    MakerLevel &course = makerEd.level;
    course.objectCount = course.tileCount = 0;
    const MakerObjectDef *gem = makerDefFor(MAKER_CLASS_DIAMOND);
    check(gem && makerParamUsable(*gem, 0), "Gem property panel exposes the shape selector");
    if (!gem)
        return;
    // Select each style through the real property command and save six two-cell gem groups.
    for (int style = 0; style <= 5; style++) {
        MakerObject o = {};
        o.klass = MAKER_CLASS_DIAMOND;
        o.x = 160 + style * 96.0f;
        o.y = 144;
        o.w = 64;
        o.h = 32;
        edBeginGroup();
        edAddObject(o);
        edSelectSingleObject(style);
        for (int step = 0; step < style; step++)
            edCommand(CMD_PROP_INC, 0);
        check(makerRound(makerParam(course.objects[style], *gem, 0)) == style,
              "Property command selects the requested gem appearance");
    }
    edUndo();
    bool undo = makerRound(makerParam(course.objects[5], *gem, 0)) == 4;
    edRedo();
    check(undo && makerRound(makerParam(course.objects[5], *gem, 0)) == 5,
          "Gem appearance changes undo and redo without losing their selected shape");
    edSelectSingleObject(5);
    edCopySelection();
    makerEd.mouseX = edScreenX(800);
    makerEd.mouseY = edScreenY(144);
    edPaste();
    edEyedropper(course.objects[6].x, course.objects[6].y);
    check(course.objectCount == 7 && course.objects[6].param[0] == 5 &&
              (makerEd.brush.setMask & 1) && makerEd.brush.param[0] == 5,
          "Copy/paste and eyedropper retain the chosen gem shape in the placement brush");
    bool stored = makerSaveLevel(MAKER_SLOTS - 1, course);
    MakerLevel loaded = {};
    stored &= makerLoadLevel(MAKER_SLOTS - 1, loaded);
    check(stored && loaded.objectCount == 7 && loaded.objects[5].param[0] == 5 &&
              loaded.objects[0].setMask == 0 && loaded.head.version == MAKER_VERSION,
          "Shape choices survive actual checksummed course save/load; old unset gems remain random");
    if (stored) {
        Arena memory = {};
        memory.init(32 * 1024 * 1024);
        Level baked = {};
        makerBakeCourse(loaded, MAKER_SLOTS - 1, baked, memory, 0);
        int groups = 0, randomVariants = 0;
        bool selected = true;
        for (int i = 0; i < baked.objectCount; i++) {
            Object &o = baked.objects[i];
            if (!o.has("CoinGroupBehaviour"))
                continue;
            int style = (int)o.props.num("maker-gem-style");
            groups++;
            for (int n = 0; n < (style ? 2 : 40); n++) {
                Entity e;
                initializeEntity(e, o, Rect{o.rect.l, o.rect.b, o.rect.l + 32, o.rect.b + 32});
                int variant = coinAnimationVariantNative(e.state);
                selected &= style == 0 || variant == style - 1;
                if (!style)
                    randomVariants |= 1 << variant;
                State saved = e.state;
                migrateCoinAnimationNative(e, saved);
                selected &= e.state.aux == saved.aux && e.state.timer == saved.timer;
            }
        }
        check(groups == 7 && selected && randomVariants == 31,
              "Saved courses bake and initialize all five fixed clips plus native random gems; checkpoint migration preserves each clip");
        check(!*makerClass(MAKER_CLASS_DIAMOND)->props.get("maker-gem-style"),
              "Custom appearance properties never mutate the original campaign gem class");
        releaseLevel(baked, memory);
        VirtualFree(memory.data, 0, MEM_RELEASE);
    }
    mkFree(loaded.tiles);
    mkFree(loaded.objects);

    // Every palette entry must produce real pixels, including actors whose idle is texture -1.
    int breakable = -1;
    for (int i = 0; i < makerObjectDefCount; i++) {
        const MakerObjectDef &d = makerObjectDefs[i];
        if (d.category == OBJ_GIZMO || eq(d.name, "Spiked Block")) {
            int visible;
            makerEditorIconPixels(i, 1.3f, &visible);
            char label[128];
            wsprintfA(label, "%s palette artwork is visible in the real GL framebuffer", d.name);
            check(visible > 20, label);
        }
        if (eq(d.name, "Breakable Block"))
            breakable = i;
    }
    check(makerEditorIconPixels(0, 0) == makerEditorIconPixels(0, 2.3f),
          "Bean start icon remains on one identical rendered frame as editor time advances");
    check(breakable >= 0 && makerEditorIconPixels(breakable, 0) ==
                               makerEditorIconPixels(breakable, 2.3f),
          "Breakable block retains its undamaged rendered frame instead of cycling cracks");

    // The real down/move/up handlers exercise preview/cancel/commit at different zooms.
    course.objectCount = course.tileCount = 0;
    makerEd.opCount = makerEd.opTop = 0;
    edClearSelection();
    makerEd.buttons.clear();
    makerEd.tool = TOOL_SELECT;
    makerEd.propObject = -1;
    makerEd.toolbarH = 22;
    makerEd.paletteH = 60;
    const float zooms[] = {.25f, .75f, 1.5f};
    bool bounds = true, resize = true;
    for (int z = 0; z < 3; z++) {
        makerEd.opCount = makerEd.opTop = 0;
        course.head.width = 1024;
        course.head.height = 512;
        makerEd.zoom = zooms[z];
        makerEd.camX = 1024 - viewW * .5f / makerEd.zoom;
        makerEd.camY = 512 - 160 / makerEd.zoom;
        makerEd.age += 1;
        float x = edScreenX(1024) - 2, y = edScreenY(512) - 2;
        int history = makerEd.opTop;
        edMouseDown(x, y, 0);
        edMouseMove(x + 96 * makerEd.zoom, y + 64 * makerEd.zoom);
        check(makerEd.drag == DRAG_BOUNDS, "Corner grip starts a course-boundary drag");
        int w, h;
        edBoundsDragSize(edWorldX(makerEd.mouseX), edWorldY(makerEd.mouseY), &w, &h);
        bounds &= makerEd.drag == DRAG_BOUNDS && w == 1120 && h == 576 &&
                  course.head.width == 1024 && makerEd.opTop == history;
        edKey(VK_ESCAPE);
        bounds &= course.head.width == 1024 && makerEd.opTop == history;
        makerEd.age += 1;
        edMouseDown(x, y, 0);
        edMouseMove(x + 96 * makerEd.zoom, y + 64 * makerEd.zoom);
        edMouseUp(makerEd.mouseX, makerEd.mouseY, 0);
        bounds &= course.head.width == w && course.head.height == h;
        edUndo();
        bounds &= course.head.width == 1024 && course.head.height == 512;

        MakerObject wall = {};
        wall.klass = 867386300540ULL;
        wall.x = 1024;
        wall.y = 400;
        wall.w = 96;
        wall.h = 64;
        makerEd.camY = 400 - 100 / makerEd.zoom;
        course.objects[0] = wall;
        course.objectCount = 1;
        edSelectSingleObject(0);
        makerEd.propsW = 100;
        makerEd.age += 1;
        x = edScreenX(1072) - 2;
        y = edScreenY(400);
        history = makerEd.opTop;
        edMouseDown(x, y, 0);
        edMouseMove(x + 64 * makerEd.zoom, y);
        check(makerEd.drag == DRAG_RESIZE, "Selected wall grip starts an object resize drag");
        MakerObject preview = edResizeDragObject(edWorldX(makerEd.mouseX), edWorldY(makerEd.mouseY));
        resize &= makerEd.drag == DRAG_RESIZE && preview.w == 160 &&
                  course.objects[0].w == 96 && makerEd.opTop == history;
        edMouseUp(makerEd.mouseX, makerEd.mouseY, 0);
        resize &= course.objects[0].w == 160 && edObjectRect(course.objects[0]).l == 976;
        edUndo();
        resize &= course.objects[0].w == 96;
        course.objectCount = 0;
        makerEd.propObject = -1;
    }
    check(bounds, "Course bounds preview follows held pointer at three zooms, cancels without edits, and commits one undoable snapped size");
    check(resize, "Enemy Wall resizing previews continuously at three zooms, anchors the opposite edge, and commits one undoable size");
    makerEditorSwitchChecks();
    makerEditorArtworkPreview();
    check(glGetError() == GL_NO_ERROR, "Editor artwork rendering completes without OpenGL errors");
}
