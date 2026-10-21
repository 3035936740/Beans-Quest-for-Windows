#pragma once

// Applies scene-specific selector presentation repairs.

// Original World5 selector authors a 20%-orange overlay only512 units wide
// inside an864-unit scene. Extend its existing quad so its tint cannot end
// halfway through the sky and the decorative columns. Resource files stay intact.
bool repairFifthSelectorTint(Level &level) {
    if (level.id != 669776786229ULL || level.tileCount <= 55 || level.size.x != 864 ||
        level.size.y != 384)
        return false;
    Tile &tile = level.tiles[55];
    if (tile.layer != 5 || tile.z != 0 || tile.texture != 0 || tile.count != 1 ||
        tile.bytes != 4 * sizeof(Vertex) || tile.rect.l != 0 || tile.rect.b != 0 ||
        tile.rect.r != 512 || tile.rect.t != 384)
        return false;
    Vertex *v = tile.vertices;
    if (v[0].x != 0 || v[0].y != 0 || v[1].x != 512 || v[1].y != 0 || v[2].x != 0 ||
        v[2].y != 384 || v[3].x != 512 || v[3].y != 384)
        return false;
    tile.rect.r = level.size.x;
    v[1].x = v[3].x = level.size.x;
    return true;
}
