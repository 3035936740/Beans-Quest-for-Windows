#pragma once

// Supported 16:9 window sizes and resolution preference handling.

struct WindowResolution {
    int width, height;
    const char *caption;
};

enum { WINDOW_RESOLUTION_COUNT = 4, DEFAULT_WINDOW_RESOLUTION = 2 };

const WindowResolution windowResolutions[WINDOW_RESOLUTION_COUNT] = {{512, 288, "512 x 288"},
                                                                     {768, 432, "768 x 432"},
                                                                     {1024, 576, "1024 x 576"},
                                                                     {1280, 720, "1280 x 720"}};

int initialWindowResolution(int workWidth, int workHeight, int frameWidth, int frameHeight) {
    int choice = DEFAULT_WINDOW_RESOLUTION;
    while (choice > 0 && (windowResolutions[choice].width + frameWidth > workWidth ||
                          windowResolutions[choice].height + frameHeight > workHeight))
        choice--;
    return choice;
}
