#pragma once

// Applies the documented selector geometry correction only to the affected preview scene.

// Released World5 preview contains a solid16x16 loop around its lower-right
// floating coin. The visible cap already has its own one-way surface. Remove
// only this internal obstruction; the adjacent column and cap remain intact.
bool repairFifthSelectorGeometry(Level &level) {
    if (level.id != 669776786229ULL || level.size.x != 864 || level.size.y != 384 ||
        level.geometryCount <= 5)
        return false;
    Geometry &box = level.geometry[0];
    Geometry &cap = level.geometry[5];
    if (box.kind != 1 || box.flags != 1 || box.count != 5 || cap.kind != 2 || cap.flags != 0 ||
        cap.count != 2)
        return false;
    const Vec expected[] = {{400, 96}, {400, 80}, {416, 80}, {416, 96}, {400, 96}};
    for (int i = 0; i < 5; i++)
        if (box.points[i].x != expected[i].x || box.points[i].y != expected[i].y)
            return false;
    if (cap.points[0].x != 384 || cap.points[0].y != 112 || cap.points[1].x != 416 ||
        cap.points[1].y != 112)
        return false;
    box.count = 0;
    return true;
}
