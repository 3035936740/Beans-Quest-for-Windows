#pragma once

// Extends backgrounds for Windows viewports using scene-specific policies.

// Continuation is render-only and restricted to audited original background
// geometry. Periods come from consecutive authored copies, never sprite size
// guesses applied to foregrounds or physics objects.
struct BackgroundExtensionPolicy {
    u64 scene;
    int tile, vertex, mode, layer, z, texture;
    u32 uvHash, geometryHash, alternateGeometryHash;
    Rect bounds;
    float stepX, stepY;
    u64 cells;
    int columns, rows;
};

#include "background_extension_policies.h"

u32 backgroundExtensionUvHash(const Vertex *q) {
    float uv[8];
    for (int i = 0; i < 4; i++) {
        uv[i * 2] = q[i].u;
        uv[i * 2 + 1] = q[i].v;
    }
    return hashBytes(uv, sizeof(uv));
}

Rect backgroundExtensionSourceRect(const Vertex *q) {
    return Rect{q[0].x, q[0].y, q[3].x, q[3].y};
}

bool backgroundExtensionMatches(const BackgroundExtensionPolicy &p, Level &scene, Tile &tile) {
    if (scene.id != p.scene || p.tile < 0 || p.tile >= scene.tileCount ||
        &tile != scene.tiles + p.tile || tile.layer != p.layer || tile.z != p.z ||
        tile.texture != p.texture || tile.bytes < (p.vertex + 4) * (int)sizeof(Vertex))
        return false;
    u32 geometry = hashBytes(tile.vertices + p.vertex, 4 * sizeof(Vertex));
    return (geometry == p.geometryHash ||
            (p.alternateGeometryHash && geometry == p.alternateGeometryHash)) &&
           backgroundExtensionUvHash(tile.vertices + p.vertex) == p.uvHash;
}

void backgroundExtensionQuad(const Vertex *q, Rect r, float left, float bottom, float right,
                             float top) {
    const float fx[] = {left, right, right, left}, fy[] = {bottom, bottom, top, top};
    const float xs[] = {r.l, r.r, r.r, r.l}, ys[] = {r.b, r.b, r.t, r.t};
    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++) {
        float x = fx[i], y = fy[i];
        float u = (1 - y) * (q[0].u + (q[1].u - q[0].u) * x) + y * (q[2].u + (q[3].u - q[2].u) * x),
              v = (1 - y) * (q[0].v + (q[1].v - q[0].v) * x) + y * (q[2].v + (q[3].v - q[2].v) * x);
        glTexCoord2f(u, v);
        glVertex2f(xs[i], ys[i]);
    }
    glEnd();
}

int backgroundExtensionCuts(float *cuts, float low, float high, float a, float b, float c,
                            float d) {
    cuts[0] = low;
    cuts[1] = high;
    int n = 2;
    const float extra[] = {a, b, c, d};
    for (int i = 0; i < 4; i++) {
        float x = extra[i];
        if (x <= low || x >= high)
            continue;
        bool duplicate = false;
        for (int j = 0; j < n; j++)
            duplicate |= cuts[j] == x;
        if (!duplicate)
            cuts[n++] = x;
    }
    for (int i = 1; i < n; i++) {
        float x = cuts[i];
        int j = i;
        while (j && cuts[j - 1] > x) {
            cuts[j] = cuts[j - 1];
            j--;
        }
        cuts[j] = x;
    }
    return n;
}

int backgroundExtensionFloor(float f) {
    int i = (int)f;
    return f < i ? i - 1 : i;
}

// Distant silhouettes may be widened on extreme windows, rather than adding
// dozens of identical peaks. Ordinary window sizes retain the authored pixels.
float backgroundSceneryScale(const BackgroundExtensionPolicy &p, Rect view) {
    if (p.mode == 8)
        return maxf(1, (view.r - view.l) / maxf(864, p.stepX * p.columns * 3));
    return p.mode >= 4 ? maxf(1, (view.r - view.l) / maxf(864, p.stepX * 3)) : 1;
}

bool backgroundHorizonPolicy(const BackgroundExtensionPolicy &p) {
    return p.mode >= 4 && p.mode <= 7;
}

bool backgroundSceneryOwnsQuad(const BackgroundExtensionPolicy &p, Level &scene, Tile &tile,
                               int vertex) {
    if (p.mode < 4 || p.scene != scene.id || p.layer != tile.layer || p.z != tile.z ||
        p.texture != tile.texture || p.tile < 0 || p.tile >= scene.tileCount ||
        vertex + 4 > tile.bytes / (int)sizeof(Vertex) ||
        !backgroundExtensionMatches(p, scene, scene.tiles[p.tile]))
        return false;
    const Vertex *q = tile.vertices + vertex;
    Rect r = backgroundExtensionSourceRect(q);
    return backgroundExtensionUvHash(q) == p.uvHash && absf(r.r - r.l - p.stepX) < .001f &&
           absf(r.t - r.b - p.stepY) < .001f && r.l >= p.bounds.l && r.r <= p.bounds.r &&
           r.b >= p.bounds.b && r.t <= p.bounds.t;
}

// A wide-window replacement is drawn once by the representative tile. Other
// copies in that audited family must not leave unscaled silhouettes underneath.
int backgroundSceneryTilePolicies(Level &scene, Tile &tile, const Rect *view,
                                  const BackgroundExtensionPolicy **policies) {
    if (!view)
        return 0;
    int count = 0;
    for (int i = 0; i < backgroundExtensionPolicyCount; i++) {
        const BackgroundExtensionPolicy &p = backgroundExtensionPolicies[i];
        if (p.mode >= 4 && p.scene == scene.id && p.layer == tile.layer && p.z == tile.z &&
            p.texture == tile.texture && backgroundSceneryScale(p, *view) > 1)
            policies[count++] = &p;
    }
    return count;
}

bool backgroundScenerySuppressQuad(Level &scene, Tile &tile, int vertex,
                                   const BackgroundExtensionPolicy **policies, int count) {
    for (int i = 0; i < count; i++)
        if (backgroundSceneryOwnsQuad(*policies[i], scene, tile, vertex))
            return true;
    return false;
}

bool backgroundSceneryLowestBand(const BackgroundExtensionPolicy &p) {
    for (int i = 0; i < backgroundExtensionPolicyCount; i++) {
        const BackgroundExtensionPolicy &other = backgroundExtensionPolicies[i];
        if (other.scene == p.scene && other.layer == p.layer &&
            (backgroundHorizonPolicy(other) || other.mode == 3) && other.bounds.b < p.bounds.b)
            return false;
    }
    return true;
}

bool backgroundPatternCell(const BackgroundExtensionPolicy &p, int x, int y) {
    if (p.columns < 1 || p.rows < 1 || p.columns * p.rows > 64)
        return false;
    x %= p.columns;
    y %= p.rows;
    if (x < 0)
        x += p.columns;
    if (y < 0)
        y += p.rows;
    return ((p.cells >> (y * p.columns + x)) & 1) != 0;
}

void drawPatternedBackdrop(const BackgroundExtensionPolicy &p, Tile &tile, Rect view) {
    if (p.stepX <= 0 || p.stepY <= 0 || p.columns < 1 || p.rows < 1)
        return;
    const Vertex *q = tile.vertices + p.vertex;
    float scale = backgroundSceneryScale(p, view);
    int left = backgroundExtensionFloor((view.l - p.bounds.l) / (p.stepX * scale));
    int right = backgroundExtensionFloor((view.r - p.bounds.l) / (p.stepX * scale));
    int bottom = backgroundExtensionFloor((view.b - p.bounds.b) / (p.stepY * scale));
    int top = backgroundExtensionFloor((view.t - p.bounds.b) / (p.stepY * scale));
    for (int y = bottom; y <= top; y++)
        for (int x = left; x <= right; x++) {
            if (!backgroundPatternCell(p, x, y))
                continue;
            Rect r = {p.bounds.l + x * p.stepX * scale, p.bounds.b + y * p.stepY * scale,
                      p.bounds.l + (x + 1) * p.stepX * scale,
                      p.bounds.b + (y + 1) * p.stepY * scale};
            if (scale == 1 && r.l >= p.bounds.l && r.r <= p.bounds.r && r.b >= p.bounds.b &&
                r.t <= p.bounds.t)
                continue;
            backgroundExtensionQuad(q, r, 0, 0, 1, 1);
        }
}

void drawResponsiveScenery(const BackgroundExtensionPolicy &p, Tile &tile, Rect view) {
    const Vertex *q = tile.vertices + p.vertex;
    Rect source = backgroundExtensionSourceRect(q);
    float scale = backgroundSceneryScale(p, view);
    float origin = p.bounds.l;
    float left = origin + (view.l - origin) / scale;
    float right = origin + (view.r - origin) / scale;
    int first = backgroundExtensionFloor((left - source.l) / p.stepX);
    int last = backgroundExtensionFloor((right - source.l) / p.stepX);
    bool continueDown = view.b < p.bounds.b && backgroundSceneryLowestBand(p);
    for (int x = first; x <= last; x++) {
        Rect r = {source.l + x * p.stepX, source.b, source.l + (x + 1) * p.stepX, source.t};
        bool original = scale == 1 && r.l >= p.bounds.l && r.r <= p.bounds.r;
        r.l = origin + (r.l - origin) * scale;
        r.r = origin + (r.r - origin) * scale;
        if (!original && r.r > view.l && r.l < view.r && r.t > view.b && r.b < view.t) {
            bool reflect = p.mode == 6 && x % 2 != 0;
            backgroundExtensionQuad(q, r, reflect ? 1.0f : 0.0f, 0, reflect ? 0.0f : 1.0f, 1);
        }
        if (p.mode == 4 && continueDown && r.r > view.l && r.l < view.r) {
            Rect fill = {r.l, view.b, r.r, minf(view.t, p.bounds.b)};
            backgroundExtensionQuad(q, fill, 0, .5f / p.stepY, 1, .5f / p.stepY);
        }
        if (p.mode == 7 && view.t > p.bounds.t && r.r > view.l && r.l < view.r) {
            Rect fill = {r.l, maxf(view.b, p.bounds.t), r.r, view.t};
            backgroundExtensionQuad(q, fill, 0, 1 - .5f / p.stepY, 1, 1 - .5f / p.stepY);
        }
    }
    // Fill below the skyline, never stack another mountain row above it. This
    // mode is generated only when the original bottom texels are opaque and
    // constant, so tall windows continue the same ground/fog colour seamlessly.
    if ((p.mode == 5 || p.mode == 6) && continueDown) {
        Rect fill = {view.l, view.b, view.r, minf(view.t, p.bounds.b)};
        if (fill.t > fill.b)
            backgroundExtensionQuad(q, fill, 0.5f, .5f / p.stepY, 0.5f, .5f / p.stepY);
    }
}

void drawBackgroundExtension(const BackgroundExtensionPolicy &p, Level &scene, Tile &tile,
                             Rect view) {
    const Vertex *q = tile.vertices + p.vertex;
    Rect source = backgroundExtensionSourceRect(q), domain = p.bounds;
    float width = source.r - source.l, height = source.t - source.b;
    if (width <= 0 || height <= 0 || view.r <= view.l || view.t <= view.b)
        return;
    if (p.mode == 8) {
        drawPatternedBackdrop(p, tile, view);
        return;
    }
    if (backgroundHorizonPolicy(p)) {
        if (p.stepX > 0 && p.stepY > 0)
            drawResponsiveScenery(p, tile, view);
        return;
    }
    if (p.mode == 1 || p.mode == 2) {
        if (p.stepX <= 0 || p.stepY <= 0)
            return;
        int left = backgroundExtensionFloor((view.l - domain.l) / p.stepX),
            right = backgroundExtensionFloor((view.r - domain.l) / p.stepX);
        int bottom = p.mode == 2 ? backgroundExtensionFloor((view.b - domain.b) / p.stepY) : 0,
            top = p.mode == 2 ? backgroundExtensionFloor((view.t - domain.b) / p.stepY) : 0;
        for (int y = bottom; y <= top; y++)
            for (int x = left; x <= right; x++) {
                Rect r = {domain.l + x * p.stepX, domain.b + y * p.stepY,
                          domain.l + (x + 1) * p.stepX, domain.b + (y + 1) * p.stepY};
                if (p.mode == 1) {
                    r.b = source.b;
                    r.t = source.t;
                    if (r.l >= domain.l && r.r <= domain.r)
                        continue;
                } else if (r.l >= domain.l && r.r <= domain.r && r.b >= domain.b && r.t <= domain.t)
                    continue;
                if (r.r <= view.l || r.l >= view.r || r.t <= view.b || r.b >= view.t)
                    continue;
                backgroundExtensionQuad(q, r, 0, 0, 1, 1);
            }
        return;
    }
    // Constant overlays may have been widened by a native scene controller.
    if (p.mode == 0 && width >= 512 && height == 384 && domain.b == 0 && domain.t == 384 &&
        p.layer > 0)
        domain = source;
    float xs[6], ys[6];
    int nx = backgroundExtensionCuts(xs, view.l, view.r, domain.l, domain.r, source.l, source.r),
        ny = backgroundExtensionCuts(ys, view.b, view.t, domain.b, domain.t, source.b, source.t);
    Texture &texture = scene.textures[tile.texture];
    float pixelsX = maxf(absf((q[1].u - q[0].u) * texture.width),
                         absf((q[1].v - q[0].v) * texture.height)),
          pixelsY = maxf(absf((q[2].u - q[0].u) * texture.width),
                         absf((q[2].v - q[0].v) * texture.height));
    float edgeX = 0.5f / maxf(1, pixelsX), edgeY = 0.5f / maxf(1, pixelsY);
    for (int y = 0; y < ny - 1; y++)
        for (int x = 0; x < nx - 1; x++) {
            Rect r = {xs[x], ys[y], xs[x + 1], ys[y + 1]};
            float cx = (r.l + r.r) * 0.5f, cy = (r.b + r.t) * 0.5f;
            if (cx >= domain.l && cx <= domain.r && cy >= domain.b && cy <= domain.t)
                continue;
            if (p.mode == 3 && r.b >= source.t)
                continue;
            float l = (r.l - source.l) / width, rr = (r.r - source.l) / width,
                  b = (r.b - source.b) / height, t = (r.t - source.b) / height;
            if (r.r <= source.l)
                l = rr = edgeX;
            else if (r.l >= source.r)
                l = rr = 1 - edgeX;
            if (r.t <= source.b)
                b = t = edgeY;
            else if (r.b >= source.t)
                b = t = 1 - edgeY;
            backgroundExtensionQuad(q, r, l, b, rr, t);
        }
}

void drawBackgroundExtensions(Level &scene, Tile &tile, const Rect *visibleBounds) {
    if (!visibleBounds)
        return;
    for (int i = 0; i < backgroundExtensionPolicyCount; i++) {
        const BackgroundExtensionPolicy &p = backgroundExtensionPolicies[i];
        if (backgroundExtensionMatches(p, scene, tile))
            drawBackgroundExtension(p, scene, tile, *visibleBounds);
    }
}
