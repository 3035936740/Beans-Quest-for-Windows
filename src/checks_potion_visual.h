#pragma once

// Regression coverage for potion visual. Runs only in the isolated BQ_ENABLE_TESTS executable.

void potionTextureCopyChecks(Actor &a) {
    u32 n;
    u8 *bytes = readFile("data/Media/Anims/8048939671444015.panims", &n, permanent);
    Reader r(bytes, n, permanent);
    r.take(16);
    int textures = r.i();
    r.take(16);
    int header[10];
    memcpy(header, r.take(40), 40);
    u8 *original = (u8 *)r.take(header[9]);
    int animations = r.i();
    bool exact = textures == 1 && header[2] == 1 && animations == a.count;
    int count = 0;
    for (int i = 0; i < animations; i++) {
        char *name = r.s();
        int fps = r.i(), frames = r.i();
        exact &= eq(name, a.animations[i].name) && fps == a.animations[i].duration &&
                 frames == a.animations[i].count;
        for (int j = 0; j < frames; j++) {
            Frame old = *(Frame *)r.take(sizeof(Frame));
            Frame &f = a.animations[i].frames[j];
            u8 tile[64 * 64 * 4];
            glBindTexture(GL_TEXTURE_2D, a.textures[f.texture].gl);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tile);
            exact &= f.duration == old.duration;
            for (int y = 0; y < 40; y++)
                for (int x = 0; x < 32; x++) {
                    float s = (x + .5f) / 32, t = (y + .5f) / 40;
                    int u = (int)((old.uv[0] + s * (old.uv[2] - old.uv[0]) +
                                   t * (old.uv[4] - old.uv[0])) *
                                  header[3]);
                    int v = (int)((old.uv[1] + s * (old.uv[3] - old.uv[1]) +
                                   t * (old.uv[5] - old.uv[1])) *
                                  header[4]);
                    for (int c = 0; c < 4; c++)
                        exact &= tile[((y + 1) * 64 + x + 1) * 4 + c] ==
                                 original[(v * header[3] + u) * 4 + c];
                }
            count++;
        }
    }
    check(exact && count == 5 && r.pos == r.size,
          "All5 potion frames retain every original RGBA texel, animation name, duration and rate");
}

void potionVisualChecks() {
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(0);
    readyTimer = 0;
    Entity potion;
    Object *source = classById(8048939671444015ULL);
    initializeEntity(potion, *source, source->rect);
    potion.state.x = 128;
    potion.state.y = 96;
    Animation *idle = animation(potion.art, "idle");
    check(idle && idle->count == 4 && idle->duration == 7,
          "Original potion keeps its four-frame7fps liquid animation");
    potionTextureCopyChecks(*potion.art);
    int oldX = viewportX, oldY = viewportY, oldW = viewportW, oldH = viewportH;
    char rows[12000] = "width,phase,frame,mask_hash,min_x,min_y,max_x,max_y\r\n";
    bool stable = true, animated = true;
    const int widths[] = {256, 320, 384, 400, 512, 640, 800};
    for (int scale = 0; scale < 7; scale++)
        for (int phase = 0; phase < 4; phase++) {
            int width = widths[scale], height = width * 3 / 4;
            u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), 0, width * height * 4);
            u8 *mask = (u8 *)HeapAlloc(GetProcessHeap(), 0, width * height);
            u32 firstMask = 0, firstColor = 0;
            potion.state.x = 128 + phase * .125f;
            potion.state.y = 96 + phase * .125f;
            for (int frame = 0; frame < 4; frame++) {
                glViewport(0, 0, width, height);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0, 256, 0, 192, -1, 1);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glClearColor(1, 0, 1, 1);
                glClear(GL_COLOR_BUFFER_BIT);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                potion.state.timer = (frame + .25f) / 7;
                drawEntity(potion);
                glFinish();
                glReadBuffer(GL_BACK);
                glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                int left = width, right = -1, bottom = height, top = -1;
                for (int y = 0; y < height; y++)
                    for (int x = 0; x < width; x++) {
                        int i = y * width + x;
                        bool set = pixels[i * 4] != 255 || pixels[i * 4 + 1] != 0 ||
                                   pixels[i * 4 + 2] != 255;
                        mask[i] = set ? 1 : 0;
                        if (set) {
                            left = x < left ? x : left;
                            right = x > right ? x : right;
                            bottom = y < bottom ? y : bottom;
                            top = y > top ? y : top;
                        }
                    }
                u32 hash = hashBytes(mask, width * height),
                    color = hashBytes(pixels, width * height * 4);
                if (!frame) {
                    firstMask = hash;
                    firstColor = color;
                } else {
                    stable &= hash == firstMask;
                    animated &= color != firstColor;
                }
                char row[200];
                wsprintfA(row, "%d,%d,%d,%08x,%d,%d,%d,%d\r\n", width, phase, frame, hash, left,
                          bottom, right, top);
                lstrcatA(rows, row);
                if (width == 320 && phase == 0) {
                    viewportX = viewportY = 0;
                    viewportW = width;
                    viewportH = height;
                    SwapBuffers(gameDC);
                    char path[120];
                    wsprintfA(path, "test-output/potion-visual-frame%d.bmp", frame);
                    capture(path);
                }
            }
            HeapFree(GetProcessHeap(), 0, pixels);
            HeapFree(GetProcessHeap(), 0, mask);
        }
    viewportX = oldX;
    viewportY = oldY;
    viewportW = oldW;
    viewportH = oldH;
    check(
        stable,
        "Actual OpenGL potion silhouette stays fixed across all4frames at7 viewport scales and4 fractional positions");
    check(animated,
          "The original potion liquid still visibly animates instead of freezing the sprite");
    atomicWrite("test-output/potion-visual-observations.csv", rows, lstrlenA(rows));
    atomicWrite("test-output/potion-visual-results.txt", testLog, lstrlenA(testLog));
    startStage(0);
    readyTimer = 0;
}
