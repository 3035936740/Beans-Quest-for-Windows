#pragma once

// Layout inspection uses the real rendered buttons and screenshots at normal, wide and narrow sizes.
void makerLayoutPreviews() {
    makerBuildCatalog();
    soundEnabled = musicEnabled = false;
    edOpen(MAKER_SLOTS - 1, true, 3);
    makerEd.loading = false;
    makerEd.mouseX = makerEd.mouseY = -100;
    MakerObject potion = {};
    potion.klass = MAKER_CLASS_POTION;
    potion.x = 256;
    potion.y = 144;
    potion.w = 32;
    potion.h = 40;
    edBeginGroup();
    edAddObject(potion);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const int sizes[][2] = {{1024, 576}, {1920, 1080}, {800, 600}, {640, 360}};
    for (int size = 0; size < 4; size++) {
        resizeWindow(sizes[size][0], sizes[size][1]);
        edTick(.016f);
        for (int state = 0; state < 5; state++) {
            makerEd.tab = state == 1 ? TAB_AREAS : TAB_PLATFORMS;
            makerEd.picker = state == 2;
            makerEd.settings = state == 3;
            makerEd.propObject = state == 4 ? makerEd.level.objectCount - 1 : -1;
            nativeViewportProjection();
            glReadBuffer(GL_BACK);
            glClear(GL_COLOR_BUFFER_BIT);
            edRender();
            bool inside = true;
            for (int i = 0; i < makerEd.buttons.count; i++) {
                const MakerButton &b = makerEd.buttons.items[i];
                if (b.id == CMD_BACKDROP || b.id == CMD_WHEEL)
                    continue;
                inside &= b.r.l >= 0 && b.r.r <= viewW && b.r.b >= 0 && b.r.t <= viewH;
                if (b.id == CMD_PLAY || b.id == CMD_SAVE || b.id == CMD_EXIT || b.id == CMD_SETTINGS)
                    inside &= b.r.b >= edCanvasTop() + edPanelPadding() - .01f;
                if (b.id == CMD_TAB && !makerEd.picker)
                    inside &= b.r.t <= edCanvasBottom() - edPanelPadding() + .01f;
                if (b.id == CMD_PROP_DELETE)
                    inside &= b.r.b >= edCanvasBottom() + 2 + edPanelPadding() + 2.99f;
            }
            char label[128];
            wsprintfA(label, "%dx%d editor state %d keeps controls inside the screen and clear of panel rims",
                      sizes[size][0], sizes[size][1], state);
            check(inside, label);
            SwapBuffers(gameDC);
            char path[128];
            wsprintfA(path, "test-output/maker-layout-%dx%d-%d.bmp", sizes[size][0], sizes[size][1], state);
            capture(path);
        }
    }
    makerEd.picker = false;
    makerEd.settings = false;
    makerEd.propObject = -1;
    resizeWindow(1024, 768);
    memset(&makerBrowser, 0, sizeof(makerBrowser));
    makerBrowser.confirm = -1;
    makerBrowser.mouseX = makerBrowser.mouseY = -100;
    makerBrowser.themePicker = true;
    nativeViewportProjection();
    glClear(GL_COLOR_BUFFER_BIT);
    brRender();
    SwapBuffers(gameDC);
    capture("test-output/maker-theme-buttons.bmp");
    makerBrowser.themePicker = false;
    makerBrowser.exists[0] = true;
    MakerHeader &head = makerBrowser.heads[0];
    head = makerEd.level.head;
    head.cleared = 1;
    head.bestJumps = head.jumpBest;
    head.bestCoins = head.coins;
    head.axolotl = 1;
    head.flags |= MAKER_VERIFIED;
    MakerPhoto &photo = makerPhotos[0];
    photo.ready = true;
    photo.slot = 0;
    photo.view = Rect{0, 0, BR_PHOTO_W, BR_PHOTO_H};
    photo.tileCount = (int)minf((float)makerEd.level.tileCount, (float)BR_PHOTO_TILES);
    memcpy(photo.tiles, makerEd.level.tiles, sizeof(MakerTile) * photo.tileCount);
    photo.objectCount = 0;
    nativeViewportProjection();
    glClear(GL_COLOR_BUFFER_BIT);
    mkSky(0);
    makerBrowser.exists[1] = true;
    makerBrowser.heads[1] = head;
    makerBrowser.heads[1].theme = 2;
    brCard(0, Rect{100, 110, 300, 220});
    brCard(1, Rect{307, 110, 507, 220});
    brCardAward(0, Rect{100, 110, 300, 220});
    brCardAward(1, Rect{307, 110, 507, 220});
    SwapBuffers(gameDC);
    capture("test-output/maker-award-front.bmp");
    glReadBuffer(GL_BACK);
    GLint stencil = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencil);
    check(stencil >= 8, "Course photo masking has an eight-bit stencil for the score box's actual black silhouette");
    check(glGetError() == GL_NO_ERROR, "Editor layouts, code-recoloured theme buttons and front-layer award render without GL errors");
}

void makerHeldButtonChecks() {
    navigate(8808768056724772154ULL);
    const u64 buttons[] = {831859999871ULL, 982191480885ULL};
    const float ages[] = {.01f, .19f, .61f, 1.33f, 2.7f};
    static u8 pixels[256 * 256 * 4];
    for (int which = 0; which < 2; which++) {
        int index = -1;
        for (int i = 0; i < page.level.objectCount; i++)
            if (page.nodes[i].object->id == buttons[which])
                index = i;
        bool stable = index >= 0;
        u32 first = 0;
        if (index >= 0) {
            UiNode &node = page.nodes[index];
            node.disabled = false;
            pointerHeld = true;
            pointerDragged = false;
            pointerNode = index;
            pointerScene = page.level.id;
            for (int age = 0; age < 5; age++) {
                page.age = ages[age];
                glViewport(0, 0, 256, 256);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0, 256, 0, 256, -1, 1);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                Vec c = center(visualRect(page, node));
                glTranslatef(128 - c.x, 128 - c.y, 0);
                glClearColor(.2f, .3f, .4f, 1);
                glClear(GL_COLOR_BUFFER_BIT);
                drawUiNode(page, node, index);
                glReadBuffer(GL_BACK);
                glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                u32 hash = hashBytes(pixels, sizeof(pixels));
                if (!age)
                    first = hash;
                stable &= hash == first;
            }
        }
        check(stable, which ? "Holding the original minimize button keeps one identical pressed frame over 2.7 seconds"
                            : "Holding the original back button keeps one identical pressed frame over 2.7 seconds");
    }
    pointerHeld = pointerDragged = false;
    pointerNode = -1;
    nativeViewportProjection();
}

void makerChapterPressChecks() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    debugMode = true;
    makerProfileLoaded = true;
    makerProfile.unlocked = makerProfile.seen = 1;
    navigate(18006219643615701355ULL);
    transitionTime = 0;
    const int sizes[][2] = {{1024, 576}, {1280, 720}, {1920, 1080}, {800, 600}};
    for (int size = 0; size < 4; size++) {
        resizeWindow(sizes[size][0], sizes[size][1]);
        int bytes = viewportW * viewportH * 4;
        u8 *before = (u8 *)mkAlloc(bytes), *after = (u8 *)mkAlloc(bytes);
        bool bounds = true, drags = true, visible = true;
        for (int i = 0; i < page.level.objectCount; i++) {
            UiNode &node = page.nodes[i];
            if (node.world <= 0)
                continue;
            node.disabled = false;
            Vec c = center(node.rect);
            page.cameraX = c.x - viewW * .5f + .37f;
            page.cameraY = .23f;
            page.age = 2;
            pointerHeld = pointerDragged = false;
            nativeViewportProjection();
            drawOriginalScene(page);
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, before);
            pointerHeld = true;
            pointerScene = page.level.id;
            pointerNode = i;
            drawOriginalScene(page);
            glReadPixels(0, 0, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, after);
            float dx, dy;
            layerOffset(page, node.object->layer, &dx, &dy);
            Rect r = shifted(node.rect, dx, dy);
            int changed = 0;
            for (int y = 0; y < viewportH; y++)
                for (int x = 0; x < viewportW; x++) {
                    int at = (y * viewportW + x) * 4;
                    if (memcmp(before + at, after + at, 3) == 0)
                        continue;
                    changed++;
                    float px = (x + .5f) / screenScale, py = (y + .5f) / screenScale;
                    bounds &= px >= r.l && px <= r.r && py >= r.b && py <= r.t;
                }
            visible &= changed > 100;
            if (size == 0 && (node.world == 6 || node.world == 7)) {
                SwapBuffers(gameDC);
                char path[96];
                wsprintfA(path, "test-output/maker-chapter-pressed-%d.bmp", node.world);
                capture(path);
                glReadBuffer(GL_BACK);
            }
            pointerDragged = true;
            drawOriginalScene(page);
            glReadPixels(0, 0, viewportW, viewportH, GL_RGBA, GL_UNSIGNED_BYTE, after);
            drags &= memcmp(before, after, bytes) == 0;
        }
        char label[144];
        wsprintfA(label, "%dx%d all seven chapter press shades align with their image bounds at fractional camera positions", sizes[size][0], sizes[size][1]);
        check(bounds && visible, label);
        check(drags, "Dragging chapters cancels every press shade, including Maker, without a residual dark frame");
        mkFree(before);
        mkFree(after);
    }
    pointerHeld = pointerDragged = false;
    pointerNode = -1;
    resizeWindow(1024, 768);
    debugMode = false;
}

void makerEraseRectangleChecks() {
    makerBuildCatalog();
    edOpen(MAKER_SLOTS - 1, true, 1);
    MakerLevel &course = makerEd.level;
    MakerTile tile = course.tiles[0];
    course.tileCount = course.objectCount = 0;
    makerEd.opCount = makerEd.opTop = 0;
    makerEd.zoom = 1;
    makerEd.camX = makerEd.camY = 0;
    makerEd.toolbarH = 30;
    makerEd.paletteH = 68;
    makerEd.tool = TOOL_ERASE;
    makerEd.grid = 32;
    makerEd.buttons.clear();
    for (int i = 0; i < 4; i++) {
        tile.l = 96 + i * 32.0f;
        tile.r = tile.l + 32;
        tile.b = 128;
        tile.t = 160;
        tile.layer = MAKER_MAIN;
        edBeginGroup();
        edAddTile(tile);
    }
    tile.layer = MAKER_BACK;
    tile.l = 96;
    tile.r = 128;
    edAddTile(tile);
    MakerObject object = {};
    object.klass = MAKER_CLASS_POTION;
    object.x = 112;
    object.y = 192;
    object.w = 32;
    object.h = 40;
    edAddObject(object);
    object.klass = 667806108659ULL;
    object.x = 160;
    object.y = 160;
    object.w = object.h = 256;
    edAddObject(object);
    int history = makerEd.opTop;
    BYTE prior[256], modifier[256] = {};
    GetKeyboardState(prior);
    modifier[VK_SHIFT] = 0x80;
    SetKeyboardState(modifier);
    float x0 = edScreenX(100), y0 = edScreenY(132),
          x1 = edScreenX(156), y1 = edScreenY(216);
    edMouseDown(x0, y0, 0);
    edMouseMove(x1, y1);
    check(makerEd.drag == DRAG_ERASE_RECT && course.tileCount == 5 && course.objectCount == 2 &&
              makerEd.opTop == history,
          "Shift+Erase previews a held rectangle without deleting the first tile or changing undo history");
    edKey(VK_ESCAPE);
    check(course.tileCount == 5 && course.objectCount == 2,
          "Escape cancels box erase without changing any tiles or objects");
    edMouseDown(x0, y0, 0);
    edMouseMove(x1, y1);
    SetKeyboardState(prior); // Releasing Shift before the pointer still commits this gesture.
    edMouseUp(x1, y1, 0);
    check(course.tileCount == 3 && course.objectCount == 1 &&
              course.objects[0].klass == 667806108659ULL,
          "Box erase removes the two boxed main-layer tiles and potion, retaining the back layer and larger surrounding water");
    edUndo();
    check(course.tileCount == 5 && course.objectCount == 2,
          "One undo restores every tile and object removed by the rectangle");
    edRedo();
    check(course.tileCount == 3 && course.objectCount == 1,
          "One redo reapplies the complete rectangle erase");
}
