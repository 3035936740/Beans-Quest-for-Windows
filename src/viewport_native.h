#pragma once

// Maps window dimensions to the logical game viewport and pointer coordinates.

// Application::GetScreenScale 0x356af8. The virtual camera spans the whole
// physical screen divided by this stepped scale; it has no aspect-ratio bars.
struct NativeViewportSize {
    int pixelWidth, pixelHeight;
    float scale, width, height;
};

NativeViewportSize nativeViewportSize(int width, int height) {
    NativeViewportSize size = {};
    size.pixelWidth = width > 0 ? width : 1;
    size.pixelHeight = height > 0 ? height : 1;
    float w = (float)size.pixelWidth, h = (float)size.pixelHeight;
    if (h >= 600 && h <= 640)
        size.scale = 2;
    else if (h >= 480 && h < 600)
        size.scale = 1.5f;
    else if (h >= 320) {
        int integerScale = size.pixelHeight / 320;
        size.scale = (float)(integerScale > 1 ? integerScale : 1);
    } else
        size.scale = minf(w / 480, h / 320);
    size.width = w / size.scale;
    size.height = h / size.scale;
    return size;
}

int viewportW = 1024, viewportH = 576, viewportX, viewportY, clientW = 1024, clientH = 576;
float viewW = 1024 / 1.5f, viewH = 384, screenScale = 1.5f;

void resizeNativeViewport(int width, int height) {
    NativeViewportSize size = nativeViewportSize(width, height);
    clientW = viewportW = size.pixelWidth;
    clientH = viewportH = size.pixelHeight;
    viewportX = viewportY = 0;
    screenScale = size.scale;
    viewW = size.width;
    viewH = size.height;
}

Vec nativePointerPosition(float clientX, float clientY) {
    return v((clientX - viewportX) / screenScale, (clientH - clientY - viewportY) / screenScale);
}

void nativeViewportProjection() {
    glViewport(viewportX, viewportY, viewportW, viewportH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, viewW, 0, viewH, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
