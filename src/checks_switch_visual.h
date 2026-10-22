#pragma once

// Compare real rendered body silhouettes, excluding the intentionally different button ends.
// Moving an entire pose by a pixel fails this check; normalizing its atlas sampling does not.
void switchVisualChecks() {
    Actor *a = actor(MAKER_CLASS_SWITCH);
    int oldW = clientW, oldH = clientH;
    const int widths[] = {256, 320, 384, 400, 512, 640, 800};
    bool aligned = true, shared = true;
    Animation *on = animation(a, "on"), *off = animation(a, "off");
    for (int i = 0; i < 8; i++)
        shared &= on->frames[0].uv[i] == off->frames[0].uv[i];
    for (int size = 0; size < 7; size++) {
        int width = widths[size], height = width * 3 / 4;
        u8 *pixels = (u8 *)mkAlloc(width * height * 4);
        u8 *mask = (u8 *)mkAlloc(width * height);
        for (int phase = 0; phase < 4; phase++) {
            float x = 128 + phase * .125f, y = 96 + phase * .125f;
            for (int pose = 0; pose < 2; pose++) {
                glViewport(0, 0, width, height);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0, 256, 0, 192, -1, 1);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glDisable(GL_SCISSOR_TEST);
                glClearColor(1, 0, 1, 1);
                glClear(GL_COLOR_BUFFER_BIT);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                drawActor(a, makerCentered(x, y, 32, 48), pose ? "on" : "off", 0);
                glReadBuffer(GL_BACK);
                glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                for (int py = 0; py < height; py++)
                    for (int px = 0; px < width; px++) {
                        float wy = (py + .5f) * 192 / height;
                        if (wy < y - 16 || wy >= y + 16)
                            continue;
                        int i = py * width + px;
                        u8 opaque = pixels[i * 4] != 255 || pixels[i * 4 + 1] != 0 ||
                                    pixels[i * 4 + 2] != 255;
                        if (pose)
                            aligned &= opaque == mask[i];
                        else
                            mask[i] = opaque;
                    }
            }
        }
        mkFree(pixels);
        mkFree(mask);
    }
    check(shared && aligned,
          "Switch on/off body silhouettes align at seven GL scales and four fractional positions without translating either pose");
    resizeNativeViewport(oldW, oldH);
}
