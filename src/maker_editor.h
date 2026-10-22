#pragma once

// Course editor. Painting, erasing, selection, undo and the property panels operate on the
// editable MakerLevel; the canvas draws it directly and test play bakes it on demand.

enum MakerTool { TOOL_PEN, TOOL_ERASE, TOOL_SELECT, TOOL_HAND, TOOL_COUNT };

enum MakerTab {
    TAB_RECENT,
    TAB_BLOCKS,
    TAB_PLATFORMS,
    TAB_SLOPES,
    TAB_DECOR,
    TAB_LARGE,
    TAB_FILLS,
    TAB_ITEMS,
    TAB_ENEMIES,
    TAB_GIZMOS,
    TAB_AREAS,
    TAB_COUNT
};

const char *makerTabNames[TAB_COUNT] = {"Recent", "Blocks", "Platforms", "Slopes", "Decor", "Large",
                                        "Fills",  "Items",  "Enemies",   "Gizmos", "Areas"};

bool edPieceTab(int tab) {
    return tab >= TAB_BLOCKS && tab <= TAB_FILLS;
}

enum MakerDrag {
    DRAG_NONE,
    DRAG_PAN,
    DRAG_PAINT,
    DRAG_ERASE,
    DRAG_RECT,
    DRAG_MARQUEE,
    DRAG_MOVE,
    DRAG_START,
    DRAG_RESIZE,
    DRAG_BOUNDS
};

enum MakerCommand {
    CMD_NONE,
    CMD_BACKDROP,
    CMD_PLAY,
    CMD_PLAY_HERE,
    CMD_TOOL,
    CMD_LAYER,
    CMD_UNDO,
    CMD_REDO,
    CMD_GRID,
    CMD_VIEW_COLLISION,
    CMD_SETTINGS,
    CMD_SAVE,
    CMD_EXIT,
    CMD_HELP,
    CMD_TAB,
    CMD_WORLD,
    CMD_ITEM,
    CMD_SCROLL,
    CMD_PICKER,
    CMD_BRUSH_COLLIDE,
    CMD_BRUSH_ROTATE,
    CMD_BRUSH_FLIP_X,
    CMD_BRUSH_FLIP_Y,
    CMD_BRUSH_TRIM,
    CMD_PROP_DEC,
    CMD_PROP_INC,
    CMD_PROP_SIZE,
    CMD_PROP_ROTATE,
    CMD_PROP_DELETE,
    CMD_PROP_CLOSE,
    CMD_SET_NAME,
    CMD_SET_THEME,
    CMD_SET_BACKGROUND,
    CMD_SET_MUSIC,
    CMD_SET_EFFECT,
    CMD_SET_WIDTH,
    CMD_SET_HEIGHT,
    CMD_SET_BEST,
    CMD_SET_PAR,
    CMD_SET_GEMS,
    CMD_SET_BEAN_SPEED,
    CMD_SET_BEAN_JUMP,
    CMD_SET_WEATHER_AMOUNT,
    CMD_SET_CAMERA,
    CMD_SET_AHEAD_X,
    CMD_SET_AHEAD_Y,
    CMD_SET_DONE,
    CMD_WHEEL // a panel row that the mouse wheel adjusts; arg is a setting id or 1000 + parameter
};

struct MakerBrush {
    bool object, stretch; // stretch: a fill brick that Shift+drag spans over a whole rectangle
    int def, piece, collideMode;
    int trim; // part of the brick that is painted, see makerTrims
    MakerTile tile;
    float w, h, angle;
    u32 setMask;
    float param[MAKER_MAX_PARAMS];
};

enum MakerOpKind {
    OP_ADD_TILE,
    OP_DEL_TILE,
    OP_SET_TILE,
    OP_ADD_OBJECT,
    OP_DEL_OBJECT,
    OP_SET_OBJECT,
    OP_SET_START,
    OP_SET_SIZE
};

struct MakerOp {
    int kind, group, index;

    union {
        struct {
            MakerTile before, after;
        } tile;

        struct {
            MakerObject before, after;
        } object;

        float values[4];
    };
};

enum { MAKER_UNDO = 32768, MAKER_RECENT = 12, MAKER_PALETTE_MAX = 2200 };

struct MakerPaletteItem {
    int kind, index; // 0 piece, 1 object definition, 2 recent brush
};

struct MakerEditor {
    MakerLevel level;
    int slot;
    bool open, dirty, edited;
    float camX, camY, zoom, age, weatherAge;
    int tool, layer, grid;
    MakerBrush brush, recent[MAKER_RECENT];
    int recentCount;
    int tab, world, scroll, pickerScroll;
    bool picker, showCollision, dimOthers, help, settings, nameEditing;
    int propObject;
    float mouseX, mouseY;
    int drag, dragButton;
    float dragWorldX, dragWorldY, dragMouseX, dragMouseY, dragCamX, dragCamY;
    float lastPaintX, lastPaintY;
    bool lastPaintValid;
    int resizeEdge, boundsEdge;
    int eraseMode; // per stroke: -1 undecided, 0 bricks and items only, 1 areas too
    float lastClickTime, lastClickX, lastClickY;
    int hoverId, hoverArg;
    float hoverTime;
    u8 *tileSel, *objectSel;
    int selTiles, selObjects;
    MakerTile *clipTiles;
    MakerObject *clipObjects;
    int clipTileCount, clipObjectCount;
    MakerOp *ops;
    int opCount, opTop, group;
    bool collisionDirty, backgroundDirty, waterDirty;
    Level collision, background, water;
    Arena collisionArena, backgroundArena, waterArena;
    u32 waterGeneration;
    float waterAge;
    MakerButtons buttons;
    MakerPaletteItem palette[MAKER_PALETTE_MAX];
    int paletteCount;
    float toolbarH, paletteH, propsW;
    bool loading;
    int entityCost;
};

MakerEditor makerEd;

float makerZoomSteps[] = {0.125f, 0.1875f, 0.25f, 0.375f, 0.5f, 0.75f, 1, 1.5f, 2, 3};
const int makerZoomCount = sizeof(makerZoomSteps) / sizeof(makerZoomSteps[0]);

void edSaveCourse();
void makerOpenBrowser();
void makerStartTest(bool fromCursor);
int edResizeHandle(float sx, float sy);

float edCanvasBottom() {
    return makerEd.paletteH;
}

float edCanvasTop() {
    return viewH - makerEd.toolbarH;
}

float edWorldX(float sx) {
    return makerEd.camX + sx / makerEd.zoom;
}

float edWorldY(float sy) {
    return makerEd.camY + (sy - edCanvasBottom()) / makerEd.zoom;
}

float edScreenX(float wx) {
    return (wx - makerEd.camX) * makerEd.zoom;
}

float edScreenY(float wy) {
    return (wy - makerEd.camY) * makerEd.zoom + edCanvasBottom();
}

bool edInCanvas(float x, float y) {
    float right = makerEd.propObject >= 0 ? viewW - makerEd.propsW : viewW;
    return x >= 0 && x < right && y >= edCanvasBottom() && y < edCanvasTop();
}

Rect edObjectRect(const MakerObject &o) {
    return makerCentered(o.x, o.y, o.w, o.h);
}

// ---------------------------------------------------------------------------------------------
// Edits and undo. Every change goes through these primitives so undo replays exactly.

void edClearSelection() {
    if (makerEd.tileSel)
        memset(makerEd.tileSel, 0, MAKER_MAX_TILES);
    if (makerEd.objectSel)
        memset(makerEd.objectSel, 0, MAKER_MAX_OBJECTS);
    makerEd.selTiles = makerEd.selObjects = 0;
}

void edTouched() {
    MakerHeader &h = makerEd.level.head;
    makerEd.dirty = true;
    makerEd.collisionDirty = true;
    makerEd.waterDirty = true;
    if (!makerEd.edited) {
        makerEd.edited = true;
        h.revision++;
    }
    h.flags &= ~MAKER_VERIFIED;
    h.cleared = h.bestJumps = h.bestCoins = h.axolotl = 0;
}

void edBeginGroup() {
    makerEd.group++;
}

MakerOp *edRecord(int kind, int index) {
    if (makerEd.opTop >= MAKER_UNDO) {
        // Drop the oldest half of the history in one move.
        int keep = MAKER_UNDO / 2;
        mkMove(makerEd.ops, makerEd.ops + (makerEd.opTop - keep), sizeof(MakerOp) * keep);
        makerEd.opTop = keep;
    }
    MakerOp &op = makerEd.ops[makerEd.opTop++];
    makerEd.opCount = makerEd.opTop;
    memset(&op, 0, sizeof(op));
    op.kind = kind;
    op.index = index;
    op.group = makerEd.group;
    return &op;
}

void edInsertTileRaw(int index, const MakerTile &t) {
    MakerLevel &l = makerEd.level;
    mkMove(l.tiles + index + 1, l.tiles + index, sizeof(MakerTile) * (l.tileCount - index));
    l.tiles[index] = t;
    l.tileCount++;
}

void edRemoveTileRaw(int index) {
    MakerLevel &l = makerEd.level;
    mkMove(l.tiles + index, l.tiles + index + 1, sizeof(MakerTile) * (l.tileCount - index - 1));
    l.tileCount--;
}

void edInsertObjectRaw(int index, const MakerObject &o) {
    MakerLevel &l = makerEd.level;
    mkMove(l.objects + index + 1, l.objects + index, sizeof(MakerObject) * (l.objectCount - index));
    l.objects[index] = o;
    l.objectCount++;
}

void edRemoveObjectRaw(int index) {
    MakerLevel &l = makerEd.level;
    mkMove(l.objects + index, l.objects + index + 1,
           sizeof(MakerObject) * (l.objectCount - index - 1));
    l.objectCount--;
}

bool edAddTile(const MakerTile &t) {
    MakerLevel &l = makerEd.level;
    if (l.tileCount >= MAKER_MAX_TILES) {
        mkToast("Tile limit reached", 0xff8080ff);
        return false;
    }
    edRecord(OP_ADD_TILE, l.tileCount)->tile.after = t;
    edInsertTileRaw(l.tileCount, t);
    edTouched();
    return true;
}

void edRemoveTile(int index) {
    edRecord(OP_DEL_TILE, index)->tile.before = makerEd.level.tiles[index];
    edRemoveTileRaw(index);
    edClearSelection();
    edTouched();
}

void edSetTile(int index, const MakerTile &t) {
    MakerOp *op = edRecord(OP_SET_TILE, index);
    op->tile.before = makerEd.level.tiles[index];
    op->tile.after = t;
    makerEd.level.tiles[index] = t;
    edTouched();
}

bool edAddObject(const MakerObject &o) {
    MakerLevel &l = makerEd.level;
    if (l.objectCount >= MAKER_MAX_OBJECTS) {
        mkToast("Object limit reached", 0xff8080ff);
        return false;
    }
    edRecord(OP_ADD_OBJECT, l.objectCount)->object.after = o;
    edInsertObjectRaw(l.objectCount, o);
    edTouched();
    return true;
}

void edRemoveObject(int index) {
    edRecord(OP_DEL_OBJECT, index)->object.before = makerEd.level.objects[index];
    edRemoveObjectRaw(index);
    if (makerEd.propObject == index)
        makerEd.propObject = -1;
    else if (makerEd.propObject > index)
        makerEd.propObject--;
    edClearSelection();
    edTouched();
}

void edSetObject(int index, const MakerObject &o) {
    MakerOp *op = edRecord(OP_SET_OBJECT, index);
    op->object.before = makerEd.level.objects[index];
    op->object.after = o;
    makerEd.level.objects[index] = o;
    edTouched();
}

// Bean always starts inside the course.
void edClampStart(float *x, float *y) {
    const MakerHeader &h = makerEd.level.head;
    *x = clamp(*x, 16, h.width - 16.0f);
    *y = clamp(*y, 16, h.height - 16.0f);
}

void edSetStart(float x, float y) {
    MakerHeader &h = makerEd.level.head;
    edClampStart(&x, &y);
    MakerOp *op = edRecord(OP_SET_START, 0);
    op->values[0] = h.startX;
    op->values[1] = h.startY;
    op->values[2] = x;
    op->values[3] = y;
    h.startX = x;
    h.startY = y;
    edTouched();
}

void edSetSize(int w, int h) {
    MakerHeader &head = makerEd.level.head;
    w = (int)clamp((float)w, MAKER_MIN_W, MAKER_MAX_W);
    h = (int)clamp((float)h, MAKER_MIN_H, MAKER_MAX_H);
    if (w == head.width && h == head.height)
        return;
    MakerOp *op = edRecord(OP_SET_SIZE, 0);
    op->values[0] = (float)head.width;
    op->values[1] = (float)head.height;
    op->values[2] = (float)w;
    op->values[3] = (float)h;
    head.width = w;
    head.height = h;
    makerEd.backgroundDirty = true;
    edTouched();
    // A smaller course pulls the start back inside and reports anything left outside.
    float sx = head.startX, sy = head.startY;
    edClampStart(&sx, &sy);
    if (sx != head.startX || sy != head.startY)
        edSetStart(sx, sy);
    int outside = 0;
    const MakerLevel &l = makerEd.level;
    for (int i = 0; i < l.objectCount; i++)
        outside +=
            l.objects[i].x < 0 || l.objects[i].x > w || l.objects[i].y < 0 || l.objects[i].y > h;
    for (int i = 0; i < l.tileCount; i++)
        outside += l.tiles[i].l >= w || l.tiles[i].b >= h;
    if (outside) {
        char text[64];
        wsprintfA(text, "%d bricks or items are outside the course", outside);
        mkToast(text, 0xffd080ff, 3);
    }
}

void edApply(const MakerOp &op, bool forward) {
    MakerHeader &h = makerEd.level.head;
    switch (op.kind) {
    case OP_ADD_TILE:
        if (forward)
            edInsertTileRaw(op.index, op.tile.after);
        else
            edRemoveTileRaw(op.index);
        break;
    case OP_DEL_TILE:
        if (forward)
            edRemoveTileRaw(op.index);
        else
            edInsertTileRaw(op.index, op.tile.before);
        break;
    case OP_SET_TILE:
        makerEd.level.tiles[op.index] = forward ? op.tile.after : op.tile.before;
        break;
    case OP_ADD_OBJECT:
        if (forward)
            edInsertObjectRaw(op.index, op.object.after);
        else
            edRemoveObjectRaw(op.index);
        break;
    case OP_DEL_OBJECT:
        if (forward)
            edRemoveObjectRaw(op.index);
        else
            edInsertObjectRaw(op.index, op.object.before);
        break;
    case OP_SET_OBJECT:
        makerEd.level.objects[op.index] = forward ? op.object.after : op.object.before;
        break;
    case OP_SET_START:
        h.startX = forward ? op.values[2] : op.values[0];
        h.startY = forward ? op.values[3] : op.values[1];
        break;
    case OP_SET_SIZE:
        h.width = (int)(forward ? op.values[2] : op.values[0]);
        h.height = (int)(forward ? op.values[3] : op.values[1]);
        makerEd.backgroundDirty = true;
        break;
    }
}

// Undo of a mass tile deletion: the popped indices ascend, so the removed tiles are merged
// back in one pass instead of shifting the array once per tile. Returns the ops consumed.
int edUndoDeletedTiles(int group) {
    int end = makerEd.opTop, start = end;
    while (start > 0 && makerEd.ops[start - 1].group == group &&
           makerEd.ops[start - 1].kind == OP_DEL_TILE &&
           (start == end || makerEd.ops[start - 1].index > makerEd.ops[start].index))
        start--;
    int count = end - start;
    if (count < 8)
        return 0;
    MakerLevel &l = makerEd.level;
    int write = l.tileCount + count - 1, read = l.tileCount - 1, next = start;
    // ops[start] holds the highest final index, ops[end - 1] the lowest.
    while (next < end) {
        if (write == makerEd.ops[next].index)
            l.tiles[write--] = makerEd.ops[next++].tile.before;
        else
            l.tiles[write--] = l.tiles[read--];
    }
    l.tileCount += count;
    makerEd.opTop = start;
    return count;
}

// Redo of the same deletion: remove every recorded index in one compaction pass.
int edRedoDeletedTiles(int group) {
    int start = makerEd.opTop, end = start;
    while (end < makerEd.opCount && makerEd.ops[end].group == group &&
           makerEd.ops[end].kind == OP_DEL_TILE &&
           (end == start || makerEd.ops[end].index < makerEd.ops[end - 1].index))
        end++;
    int count = end - start;
    if (count < 8)
        return 0;
    MakerLevel &l = makerEd.level;
    int write = 0, next = end - 1;
    for (int read = 0; read < l.tileCount; read++) {
        if (next >= start && makerEd.ops[next].index == read) {
            next--;
            continue;
        }
        l.tiles[write++] = l.tiles[read];
    }
    l.tileCount = write;
    makerEd.opTop = end;
    return count;
}

// History steps wait until the current stroke or drag is finished.
void edUndo() {
    if (makerEd.drag != DRAG_NONE)
        return;
    if (makerEd.opTop <= 0) {
        mkToast("Nothing to undo");
        return;
    }
    int group = makerEd.ops[makerEd.opTop - 1].group;
    while (makerEd.opTop > 0 && makerEd.ops[makerEd.opTop - 1].group == group)
        if (!edUndoDeletedTiles(group))
            edApply(makerEd.ops[--makerEd.opTop], false);
    edClearSelection();
    makerEd.propObject = -1;
    edTouched();
    sound("switch", 0.8f, 0.5f);
}

void edRedo() {
    if (makerEd.drag != DRAG_NONE)
        return;
    if (makerEd.opTop >= makerEd.opCount) {
        mkToast("Nothing to redo");
        return;
    }
    int group = makerEd.ops[makerEd.opTop].group;
    while (makerEd.opTop < makerEd.opCount && makerEd.ops[makerEd.opTop].group == group)
        if (!edRedoDeletedTiles(group))
            edApply(makerEd.ops[makerEd.opTop++], true);
    edClearSelection();
    makerEd.propObject = -1;
    edTouched();
    sound("switch", 1.2f, 0.5f);
}

// ---------------------------------------------------------------------------------------------
// Brushes.

// Corner order is bottom-left, bottom-right, top-left, top-right.
void edRotateUv(float *uv) {
    float o[8];
    memcpy(o, uv, sizeof(o));
    const int from[4] = {2, 0, 3, 1};
    for (int i = 0; i < 4; i++) {
        uv[i * 2] = o[from[i] * 2];
        uv[i * 2 + 1] = o[from[i] * 2 + 1];
    }
}

void edFlipUv(float *uv, bool horizontal) {
    float o[8];
    memcpy(o, uv, sizeof(o));
    const int fromX[4] = {1, 0, 3, 2}, fromY[4] = {2, 3, 0, 1};
    const int *from = horizontal ? fromX : fromY;
    for (int i = 0; i < 4; i++) {
        uv[i * 2] = o[from[i] * 2];
        uv[i * 2 + 1] = o[from[i] * 2 + 1];
    }
}

int edRotateCollide(int c) {
    // Counter-clockwise quarter turn of the solid corner.
    const int map[COLLIDE_KINDS] = {COLLIDE_NONE,     COLLIDE_SOLID,    COLLIDE_ONEWAY,
                                    COLLIDE_SLOPE_TR, COLLIDE_SLOPE_BR, COLLIDE_SLOPE_TL,
                                    COLLIDE_SLOPE_BL};
    return c >= 0 && c < COLLIDE_KINDS ? map[c] : c;
}

int edFlipCollide(int c, bool horizontal) {
    const int mapX[COLLIDE_KINDS] = {COLLIDE_NONE,     COLLIDE_SOLID,    COLLIDE_ONEWAY,
                                     COLLIDE_SLOPE_BL, COLLIDE_SLOPE_BR, COLLIDE_SLOPE_TL,
                                     COLLIDE_SLOPE_TR};
    const int mapY[COLLIDE_KINDS] = {COLLIDE_NONE,     COLLIDE_SOLID,    COLLIDE_ONEWAY,
                                     COLLIDE_SLOPE_TR, COLLIDE_SLOPE_TL, COLLIDE_SLOPE_BR,
                                     COLLIDE_SLOPE_BL};
    if (c < 0 || c >= COLLIDE_KINDS)
        return c;
    return horizontal ? mapX[c] : mapY[c];
}

const char *edCollideName(int c) {
    const char *names[COLLIDE_KINDS] = {"Decor",    "Solid",      "Platform", "Slope /",
                                        "Slope \\", "Ceiling \\", "Ceiling /"};
    return c >= 0 && c < COLLIDE_KINDS ? names[c] : "Auto";
}

// Halves and quarters of any brick, as fractions of its width and height (left, bottom, right,
// top) in the brick's own orientation.
enum { MAKER_TRIMS = 9 };

const char *makerTrimNames[MAKER_TRIMS] = {"Whole",      "Left 1/2", "Right 1/2",
                                           "Bottom 1/2", "Top 1/2",  "BL 1/4",
                                           "BR 1/4",     "TL 1/4",   "TR 1/4"};
const float makerTrims[MAKER_TRIMS][4] = {
    {0, 0, 1, 1},       {0, 0, 0.5f, 1},    {0.5f, 0, 1, 1},    {0, 0, 1, 0.5f},   {0, 0.5f, 1, 1},
    {0, 0, 0.5f, 0.5f}, {0.5f, 0, 1, 0.5f}, {0, 0.5f, 0.5f, 1}, {0.5f, 0.5f, 1, 1}};

// Atlas coordinate of a point inside a tile from its bottom-left, bottom-right, top-left and
// top-right corner coordinates.
void edLerpUv(const float *uv, float fx, float fy, float *out) {
    for (int k = 0; k < 2; k++) {
        float bottom = uv[k] + (uv[2 + k] - uv[k]) * fx,
              top = uv[4 + k] + (uv[6 + k] - uv[4 + k]) * fx;
        out[k] = bottom + (top - bottom) * fy;
    }
}

// The tile a brush paints, with its trim applied. A trimmed platform keeps its line only when
// the line stays inside the trimmed part.
MakerTile edTrimTile(const MakerBrush &b) {
    MakerTile t = b.tile;
    if (b.object || b.stretch || b.trim <= 0 || b.trim >= MAKER_TRIMS)
        return t;
    const float *f = makerTrims[b.trim];
    float w = t.r - t.l, h = t.t - t.b;
    const float corners[4][2] = {{f[0], f[1]}, {f[2], f[1]}, {f[0], f[3]}, {f[2], f[3]}};
    for (int i = 0; i < 4; i++)
        edLerpUv(b.tile.uv, corners[i][0], corners[i][1], t.uv + i * 2);
    t.l = t.b = 0;
    t.r = w * (f[2] - f[0]);
    t.t = h * (f[3] - f[1]);
    if (t.collide == COLLIDE_ONEWAY) {
        int surface = makerRound(t.surface - h * (1 - f[3]));
        if (surface < 0 || surface >= t.t)
            t.collide = COLLIDE_NONE;
        t.surface = (short)(surface < 0 ? 0 : surface);
    }
    return t;
}

MakerTile edBrushTile() {
    return edTrimTile(makerEd.brush);
}

const int makerGridSteps[] = {32, 16, 8, 4, 2, 1};
const int makerGridStepCount = sizeof(makerGridSteps) / sizeof(makerGridSteps[0]);

int edGridForSize(float width, float height) {
    float size = minf(width, height), distance = 1e30f;
    int grid = 32;
    for (int i = 0; i < makerGridStepCount; i++) {
        float delta = absf(size - makerGridSteps[i]);
        // Descending steps keep the larger grid on an exact midpoint (24 -> 32).
        if (delta < distance) {
            distance = delta;
            grid = makerGridSteps[i];
        }
    }
    return grid;
}

void edAutoBrushGrid() {
    const MakerBrush &b = makerEd.brush;
    MakerTile t = edBrushTile();
    makerEd.grid = edGridForSize(b.object ? b.w : t.r - t.l,
                                 b.object ? b.h : t.t - t.b);
}

// Slopes keep their whole triangle and fills stretch instead, so neither is trimmed.
bool edBrushTrimmable(const MakerBrush &b) {
    return !b.object && !b.stretch && b.tile.collide < COLLIDE_SLOPE_BR &&
           b.tile.r - b.tile.l >= 8 && b.tile.t - b.tile.b >= 8;
}

void edCycleBrushTrim(int direction) {
    MakerBrush &b = makerEd.brush;
    if (!edBrushTrimmable(b)) {
        mkToast(b.stretch ? "Fills stretch with Shift+drag instead" : "This brick cannot be cut");
        return;
    }
    b.trim = (b.trim + direction + MAKER_TRIMS) % MAKER_TRIMS;
    edAutoBrushGrid();
    char text[48];
    wsprintfA(text, "Brick part: %s", makerTrimNames[b.trim]);
    mkToast(text);
}

void edPieceBrush(int piece) {
    const MakerPiece &p = makerPieces[piece];
    MakerBrush &b = makerEd.brush;
    // The chosen part stays when switching bricks, so halves and quarters can be mixed quickly.
    int trim = b.object ? 0 : b.trim;
    memset(&b, 0, sizeof(b));
    b.object = false;
    b.piece = piece;
    b.def = -1;
    b.collideMode = -1;
    MakerTile &t = b.tile;
    t.r = p.w;
    t.t = p.h;
    memcpy(t.uv, p.uv, sizeof(t.uv));
    t.group = p.group;
    t.texture = p.texture;
    t.collide = p.collide;
    t.surface = p.surface;
    t.flags = p.opaque ? TILE_OPAQUE : 0;
    b.stretch = p.category == PIECE_FILL;
    if (b.stretch)
        t.flags |= TILE_FILL;
    b.trim = edBrushTrimmable(b) ? trim : 0;
    edAutoBrushGrid();
}

void edDefBrush(int def) {
    MakerBrush &b = makerEd.brush;
    memset(&b, 0, sizeof(b));
    b.object = true;
    b.def = def;
    b.piece = -1;
    makerDefaultSize(makerObjectDefs[def], &b.w, &b.h);
    edAutoBrushGrid();
}

bool edSameBrush(const MakerBrush &a, const MakerBrush &b) {
    if (a.object != b.object)
        return false;
    if (a.object)
        return a.def == b.def && a.angle == b.angle;
    return a.tile.group == b.tile.group && a.tile.texture == b.tile.texture &&
           a.tile.r == b.tile.r && a.tile.t == b.tile.t && a.trim == b.trim &&
           makerSameUv(a.tile.uv, b.tile.uv);
}

void edRememberBrush() {
    for (int i = 0; i < makerEd.recentCount; i++)
        if (edSameBrush(makerEd.recent[i], makerEd.brush)) {
            mkMove(makerEd.recent + 1, makerEd.recent, sizeof(MakerBrush) * i);
            makerEd.recent[0] = makerEd.brush;
            return;
        }
    int n = makerEd.recentCount < MAKER_RECENT ? makerEd.recentCount : MAKER_RECENT - 1;
    mkMove(makerEd.recent + 1, makerEd.recent, sizeof(MakerBrush) * n);
    makerEd.recent[0] = makerEd.brush;
    makerEd.recentCount = n + 1;
}

// Back and front layers are scenery: solid blocks there only collide when chosen explicitly.
int edBrushCollide(int layer) {
    const MakerBrush &b = makerEd.brush;
    if (b.collideMode >= 0)
        return b.collideMode;
    int c = edTrimTile(b).collide;
    if (layer != MAKER_MAIN && c == COLLIDE_SOLID)
        return COLLIDE_NONE;
    return c;
}

void edCycleBrushCollide() {
    MakerBrush &b = makerEd.brush;
    if (b.object)
        return;
    b.collideMode = b.collideMode < 0 ? COLLIDE_SOLID : b.collideMode + 1;
    if (b.collideMode >= COLLIDE_KINDS)
        b.collideMode = -1;
    char text[64];
    wsprintfA(text, "Brush collision: %s",
              b.collideMode < 0 ? "Auto" : edCollideName(b.collideMode));
    mkToast(text);
}

void edRotateBrush(int turns) {
    MakerBrush &b = makerEd.brush;
    turns = (turns % 4 + 4) % 4;
    if (b.object) {
        const MakerObjectDef &d = makerObjectDefs[b.def];
        if (!(d.flags & OBJ_ROTATE))
            return;
        b.angle = (float)(((int)b.angle + turns * 90) % 360);
        if (turns % 2) {
            float w = b.w;
            b.w = b.h;
            b.h = w;
        }
        edAutoBrushGrid();
        return;
    }
    for (int i = 0; i < turns; i++) {
        edRotateUv(b.tile.uv);
        float w = b.tile.r;
        b.tile.r = b.tile.t;
        b.tile.t = w;
        b.tile.collide = (u8)edRotateCollide(b.tile.collide);
        if (b.collideMode >= 0)
            b.collideMode = edRotateCollide(b.collideMode);
        b.tile.surface = 0;
    }
    edAutoBrushGrid();
}

void edFlipBrush(bool horizontal) {
    MakerBrush &b = makerEd.brush;
    if (b.object)
        return;
    edFlipUv(b.tile.uv, horizontal);
    b.tile.collide = (u8)edFlipCollide(b.tile.collide, horizontal);
    if (b.collideMode >= 0)
        b.collideMode = edFlipCollide(b.collideMode, horizontal);
    if (!horizontal)
        b.tile.surface = 0;
}

// ---------------------------------------------------------------------------------------------
// Queries.

// Water, wind, walls and other areas are large and see-through.
bool edIsArea(const MakerObject &o) {
    const MakerObjectDef *d = makerDefFor(o.klass);
    return d && (d->flags & (OBJ_FREE | OBJ_INVISIBLE));
}

// Topmost item under a point. Areas answer only when asked and only after every item, so
// bricks, fish and gems inside water stay reachable.
int edObjectAt(float wx, float wy, bool areas = false) {
    for (int pass = 0; pass < (areas ? 2 : 1); pass++)
        for (int i = makerEd.level.objectCount - 1; i >= 0; i--) {
            const MakerObject &o = makerEd.level.objects[i];
            if (edIsArea(o) != (pass == 1))
                continue;
            if (mkInside(edObjectRect(o), wx, wy))
                return i;
        }
    return -1;
}

// Areas are grabbed by their frame or name tag; thin ones anywhere.
int edAreaHandleAt(float wx, float wy) {
    float tolerance = 5 / makerEd.zoom;
    for (int i = makerEd.level.objectCount - 1; i >= 0; i--) {
        const MakerObject &o = makerEd.level.objects[i];
        if (!edIsArea(o))
            continue;
        Rect r = edObjectRect(o);
        if (wx < r.l - tolerance || wx >= r.r + tolerance || wy < r.b - tolerance ||
            wy >= r.t + tolerance)
            continue;
        const MakerObjectDef *d = makerDefFor(o.klass);
        float tagW = (mkTextWidth(d->name, 8) + 6) / makerEd.zoom,
              tagH = (mkLineHeight(8) + 2) / makerEd.zoom;
        bool thin = r.r - r.l < tolerance * 5 || r.t - r.b < tolerance * 5;
        bool edge = wx < r.l + tolerance || wx >= r.r - tolerance || wy < r.b + tolerance ||
                    wy >= r.t - tolerance;
        bool tag = wx < r.l + tagW && wy >= r.t - tagH;
        if (thin || edge || tag)
            return i;
    }
    return -1;
}

// What a click picks up: an item under the cursor, else an area by its handle.
int edGrabAt(float wx, float wy) {
    int o = edObjectAt(wx, wy);
    return o >= 0 ? o : edAreaHandleAt(wx, wy);
}

// Topmost tile of one layer; on the main layer overlays sit above opaque blocks.
int edTileAt(float wx, float wy, int layer) {
    MakerLevel &l = makerEd.level;
    for (int pass = 0; pass < 2; pass++)
        for (int i = l.tileCount - 1; i >= 0; i--) {
            const MakerTile &t = l.tiles[i];
            if (t.layer != layer)
                continue;
            bool opaque = (t.flags & TILE_OPAQUE) != 0;
            if (layer == MAKER_MAIN && (pass == 0) == opaque)
                continue;
            if (layer != MAKER_MAIN && pass == 1)
                continue;
            if (wx >= t.l && wx < t.r && wy >= t.b && wy < t.t)
                return i;
        }
    return -1;
}

int edTopTileAt(float wx, float wy) {
    const int order[3] = {MAKER_FRONT, MAKER_MAIN, MAKER_BACK};
    for (int i = 0; i < 3; i++) {
        int t = edTileAt(wx, wy, order[i]);
        if (t >= 0)
            return t;
    }
    return -1;
}

bool edStartAt(float wx, float wy) {
    const MakerHeader &h = makerEd.level.head;
    return absf(wx - h.startX) <= 16 && absf(wy - h.startY) <= 16;
}

int edEntityCost() {
    int n = 3;
    for (int i = 0; i < makerEd.level.objectCount; i++) {
        const MakerObject &o = makerEd.level.objects[i];
        const MakerObjectDef *d = makerDefFor(o.klass);
        if (d)
            n += makerObjectEntities(o, *d);
    }
    return n;
}

float edSnapCoord(float w, float size) {
    float g = (float)makerEd.grid;
    return makerFloor((w - size * 0.5f + g * 0.5f) / g) * g;
}

Vec edObjectBrushCenter(float wx, float wy) {
    const MakerBrush &b = makerEd.brush;
    // The switch's 32x48 artwork includes its projecting button; snap its solid 32x32 core.
    float w = b.w, h = makerObjectDefs[b.def].klass == MAKER_CLASS_SWITCH ? 32 : b.h;
    return Vec{edSnapCoord(wx, w) + w * .5f, edSnapCoord(wy, h) + h * .5f};
}

// ---------------------------------------------------------------------------------------------
// Painting.

// Place a brush tile at its final rectangle on the current layer. An identical tile there is
// kept, and an opaque block replaces the opaque block it lands on.
bool edPlaceTile(MakerTile t) {
    t.layer = (u8)makerEd.layer;
    t.collide = (u8)edBrushCollide(makerEd.layer);
    const MakerHeader &head = makerEd.level.head;
    if (t.r < -64 || t.l > head.width + 64 || t.t < -64 || t.b > head.height + 64)
        return false;
    MakerLevel &l = makerEd.level;
    for (int i = l.tileCount - 1; i >= 0; i--) {
        const MakerTile &o = l.tiles[i];
        if (o.layer != t.layer || o.l != t.l || o.b != t.b || o.r != t.r || o.t != t.t)
            continue;
        bool same = o.group == t.group && o.texture == t.texture && makerSameUv(o.uv, t.uv);
        if (same) {
            if (o.collide == t.collide)
                return false;
            edSetTile(i, t);
            return true;
        }
        if ((o.flags & TILE_OPAQUE) && (t.flags & TILE_OPAQUE)) {
            edSetTile(i, t);
            return true;
        }
    }
    return edAddTile(t);
}

bool edPaintTile(float wx, float wy) {
    MakerTile t = edBrushTile();
    float w = t.r - t.l, h = t.t - t.b;
    t.l = edSnapCoord(wx, w);
    t.b = edSnapCoord(wy, h);
    t.r = t.l + w;
    t.t = t.b + h;
    return edPlaceTile(t);
}

bool edPlaceObject(float wx, float wy) {
    const MakerBrush &b = makerEd.brush;
    const MakerObjectDef &d = makerObjectDefs[b.def];
    Vec position = edObjectBrushCenter(wx, wy);
    float x = position.x, y = position.y;
    if (d.flags & OBJ_START) {
        edClampStart(&x, &y);
        if (makerEd.level.head.startX == x && makerEd.level.head.startY == y)
            return false;
        edSetStart(x, y);
        return true;
    }
    MakerLevel &l = makerEd.level;
    for (int i = 0; i < l.objectCount; i++) {
        MakerObject &o = l.objects[i];
        if (o.klass != d.klass)
            continue;
        if ((d.flags & OBJ_UNIQUE)) {
            if (o.x == x && o.y == y)
                return false;
            MakerObject moved = o;
            moved.x = x;
            moved.y = y;
            edSetObject(i, moved);
            return true;
        }
        if (absf(o.x - x) < 1 && absf(o.y - y) < 1)
            return false;
    }
    MakerObject o;
    memset(&o, 0, sizeof(o));
    o.klass = d.klass;
    o.x = x;
    o.y = y;
    o.w = b.w;
    o.h = b.h;
    o.angle = b.angle;
    o.setMask = b.setMask;
    memcpy(o.param, b.param, sizeof(o.param));
    int cost = makerObjectEntities(o, d);
    if (makerEd.entityCost + cost > MAX_ENTITIES - 160) {
        mkToast("Too many objects for one course", 0xff8080ff);
        return false;
    }
    if (edAddObject(o)) {
        makerEd.entityCost += cost;
        return true;
    }
    return false;
}

bool edPaintAt(float wx, float wy) {
    return makerEd.brush.object ? edPlaceObject(wx, wy) : edPaintTile(wx, wy);
}

// Strokes step along the cursor path in grid-sized steps so fast drags leave no gaps.
void edPaintStroke(float wx, float wy) {
    float step = (float)makerEd.grid;
    if (!makerEd.brush.object) {
        MakerTile t = edBrushTile();
        step = maxf(step, minf(t.r - t.l, t.t - t.b));
    } else
        step = maxf(step, minf(makerEd.brush.w, makerEd.brush.h));
    if (!makerEd.lastPaintValid) {
        edPaintAt(wx, wy);
        makerEd.lastPaintX = wx;
        makerEd.lastPaintY = wy;
        makerEd.lastPaintValid = true;
        return;
    }
    float dx = wx - makerEd.lastPaintX, dy = wy - makerEd.lastPaintY;
    float distance = root(dx * dx + dy * dy);
    int steps = (int)(distance / maxf(1, step * 0.5f)) + 1;
    if (steps > 256)
        steps = 256;
    for (int i = 1; i <= steps; i++)
        edPaintAt(makerEd.lastPaintX + dx * i / steps, makerEd.lastPaintY + dy * i / steps);
    makerEd.lastPaintX = wx;
    makerEd.lastPaintY = wy;
}

// What the eraser takes at a point: items, then bricks, then area frames, then area interiors
// unless the stroke already took bricks or items, so wiping bricks inside water keeps the water.
int edEraseTarget(float wx, float wy, bool allLayers, int mode, int *tile) {
    *tile = -1;
    int o = edObjectAt(wx, wy);
    if (o >= 0)
        return o;
    *tile = allLayers ? edTopTileAt(wx, wy) : edTileAt(wx, wy, makerEd.layer);
    if (*tile >= 0)
        return -1;
    o = edAreaHandleAt(wx, wy);
    if (o < 0 && mode != 0)
        o = edObjectAt(wx, wy, true);
    return o;
}

bool edEraseAt(float wx, float wy, bool allLayers) {
    int t, o = edEraseTarget(wx, wy, allLayers, makerEd.eraseMode, &t);
    if (o >= 0) {
        if (makerEd.eraseMode < 0)
            makerEd.eraseMode = edIsArea(makerEd.level.objects[o]) ? 1 : 0;
        edRemoveObject(o);
        makerEd.entityCost = edEntityCost();
        return true;
    }
    if (t >= 0) {
        if (makerEd.eraseMode < 0)
            makerEd.eraseMode = 0;
        edRemoveTile(t);
        return true;
    }
    return false;
}

void edEraseStroke(float wx, float wy, bool allLayers) {
    if (!makerEd.lastPaintValid) {
        if (edEraseAt(wx, wy, allLayers))
            sound("james-cloud", 1.3f, 0.35f);
        makerEd.lastPaintX = wx;
        makerEd.lastPaintY = wy;
        makerEd.lastPaintValid = true;
        return;
    }
    float dx = wx - makerEd.lastPaintX, dy = wy - makerEd.lastPaintY;
    int steps = (int)(root(dx * dx + dy * dy) / 6) + 1;
    if (steps > 256)
        steps = 256;
    for (int i = 1; i <= steps; i++)
        edEraseAt(makerEd.lastPaintX + dx * i / steps, makerEd.lastPaintY + dy * i / steps,
                  allLayers);
    makerEd.lastPaintX = wx;
    makerEd.lastPaintY = wy;
}

Rect edDragRect(float x0, float y0, float x1, float y1) {
    float g = (float)makerEd.grid;
    float l = makerFloor(minf(x0, x1) / g) * g, b = makerFloor(minf(y0, y1) / g) * g;
    float r = (makerFloor(maxf(x0, x1) / g) + 1) * g, t = (makerFloor(maxf(y0, y1) / g) + 1) * g;
    return Rect{l, b, r, t};
}

// Shift-drag repeats fixed-size objects and tiles, or sizes one resizable area/assembly.
void edFillRect(Rect r) {
    edBeginGroup();
    MakerBrush &b = makerEd.brush;
    if (b.object) {
        const MakerObjectDef &d = makerObjectDefs[b.def];
        if (d.flags & (OBJ_RESIZE_X | OBJ_RESIZE_Y)) {
            float cw = d.cellW > 0 ? d.cellW : 16, ch = d.cellH > 0 ? d.cellH : 16;
            float w = (d.flags & OBJ_RESIZE_X) ? maxf(cw, makerRound((r.r - r.l) / cw) * cw) : b.w;
            float h = (d.flags & OBJ_RESIZE_Y) ? maxf(ch, makerRound((r.t - r.b) / ch) * ch) : b.h;
            MakerBrush saved = b;
            b.w = w;
            b.h = h;
            edPlaceObject(r.l + w * 0.5f, r.b + h * 0.5f);
            b = saved;
        } else if (d.flags & (OBJ_UNIQUE | OBJ_START))
            edPlaceObject((r.l + r.r) * 0.5f, (r.b + r.t) * 0.5f);
        else {
            float w = b.w, h = d.klass == MAKER_CLASS_SWITCH ? 32 : b.h;
            int attempts = 0;
            for (float y = r.b; y + h <= r.t + .5f && attempts < 4000; y += h)
                for (float x = r.l; x + w <= r.r + .5f && attempts < 4000; x += w) {
                    attempts++;
                    edPlaceObject(x + w * .5f, y + h * .5f);
                }
        }
        return;
    }
    if (b.stretch) {
        // One fill spans the whole rectangle.
        MakerTile t = b.tile;
        t.l = r.l;
        t.b = r.b;
        t.r = r.r;
        t.t = r.t;
        if (edPlaceTile(t))
            sound("coin", 1.4f, 0.25f);
        return;
    }
    MakerTile part = edBrushTile();
    float w = part.r - part.l, h = part.t - part.b;
    int placed = 0;
    for (float y = r.b; y + h <= r.t + 0.5f && placed < 4000; y += h)
        for (float x = r.l; x + w <= r.r + 0.5f && placed < 4000; x += w)
            placed += edPaintTile(x + w * 0.5f, y + h * 0.5f);
    if (placed)
        sound("coin", 1.4f, 0.25f);
}

// ---------------------------------------------------------------------------------------------
// Selection, clipboard and eyedropper.

void edSelectRect(Rect r, bool add) {
    if (!add)
        edClearSelection();
    MakerLevel &l = makerEd.level;
    for (int i = 0; i < l.objectCount; i++) {
        Rect o = edObjectRect(l.objects[i]);
        if (o.l >= r.l && o.r <= r.r && o.b >= r.b && o.t <= r.t && !makerEd.objectSel[i]) {
            makerEd.objectSel[i] = 1;
            makerEd.selObjects++;
        }
    }
    for (int i = 0; i < l.tileCount; i++) {
        const MakerTile &t = l.tiles[i];
        if (t.layer == makerEd.layer && t.l >= r.l && t.r <= r.r && t.b >= r.b && t.t <= r.t &&
            !makerEd.tileSel[i]) {
            makerEd.tileSel[i] = 1;
            makerEd.selTiles++;
        }
    }
}

void edDeleteSelection() {
    if (!makerEd.selTiles && !makerEd.selObjects)
        return;
    edBeginGroup();
    MakerLevel &l = makerEd.level;
    u8 *tiles = (u8 *)mkAlloc(MAKER_MAX_TILES), *objects = (u8 *)mkAlloc(MAKER_MAX_OBJECTS);
    memcpy(tiles, makerEd.tileSel, MAKER_MAX_TILES);
    memcpy(objects, makerEd.objectSel, MAKER_MAX_OBJECTS);
    for (int i = l.objectCount - 1; i >= 0; i--)
        if (objects[i])
            edRemoveObject(i);
    // Record descending removals exactly as one-by-one deletes would, then compact once.
    int write = 0;
    for (int i = l.tileCount - 1; i >= 0; i--)
        if (tiles[i])
            edRecord(OP_DEL_TILE, i)->tile.before = l.tiles[i];
    for (int i = 0; i < l.tileCount; i++)
        if (!tiles[i])
            l.tiles[write++] = l.tiles[i];
    if (write != l.tileCount)
        edTouched();
    l.tileCount = write;
    mkFree(tiles);
    mkFree(objects);
    edClearSelection();
    makerEd.propObject = -1;
    makerEd.entityCost = edEntityCost();
    sound("james-cloud", 1, 0.4f);
}

void edCopySelection() {
    MakerLevel &l = makerEd.level;
    if (!makerEd.clipTiles) {
        makerEd.clipTiles = (MakerTile *)mkAlloc(sizeof(MakerTile) * MAKER_MAX_TILES);
        makerEd.clipObjects = (MakerObject *)mkAlloc(sizeof(MakerObject) * MAKER_MAX_OBJECTS);
    }
    makerEd.clipTileCount = makerEd.clipObjectCount = 0;
    for (int i = 0; i < l.tileCount; i++)
        if (makerEd.tileSel[i])
            makerEd.clipTiles[makerEd.clipTileCount++] = l.tiles[i];
    for (int i = 0; i < l.objectCount; i++)
        if (makerEd.objectSel[i])
            makerEd.clipObjects[makerEd.clipObjectCount++] = l.objects[i];
    char text[64];
    wsprintfA(text, "Copied %d tiles, %d objects", makerEd.clipTileCount, makerEd.clipObjectCount);
    mkToast(text);
}

// Paste so the clipboard's lower-left corner lands on the grid cell under the cursor.
void edPaste() {
    if (!makerEd.clipTileCount && !makerEd.clipObjectCount)
        return;
    float minX = 1e9f, minY = 1e9f;
    for (int i = 0; i < makerEd.clipTileCount; i++) {
        minX = minf(minX, makerEd.clipTiles[i].l);
        minY = minf(minY, makerEd.clipTiles[i].b);
    }
    for (int i = 0; i < makerEd.clipObjectCount; i++) {
        Rect r = edObjectRect(makerEd.clipObjects[i]);
        minX = minf(minX, r.l);
        minY = minf(minY, r.b);
    }
    float g = (float)makerEd.grid;
    float dx = makerFloor(edWorldX(makerEd.mouseX) / g) * g - minX,
          dy = makerFloor(edWorldY(makerEd.mouseY) / g) * g - minY;
    edBeginGroup();
    edClearSelection();
    MakerLevel &l = makerEd.level;
    for (int i = 0; i < makerEd.clipTileCount; i++) {
        MakerTile t = makerEd.clipTiles[i];
        t.l += dx;
        t.r += dx;
        t.b += dy;
        t.t += dy;
        if (edAddTile(t)) {
            makerEd.tileSel[l.tileCount - 1] = 1;
            makerEd.selTiles++;
        }
    }
    // Items beyond the course's object budget would not appear in play, so they are left out.
    int skipped = 0;
    makerEd.entityCost = edEntityCost();
    for (int i = 0; i < makerEd.clipObjectCount; i++) {
        MakerObject o = makerEd.clipObjects[i];
        o.x += dx;
        o.y += dy;
        const MakerObjectDef *d = makerDefFor(o.klass);
        if (!d || (d->flags & OBJ_UNIQUE))
            continue;
        int cost = makerObjectEntities(o, *d);
        if (makerEd.entityCost + cost > MAX_ENTITIES - 160) {
            skipped++;
            continue;
        }
        if (edAddObject(o)) {
            makerEd.entityCost += cost;
            makerEd.objectSel[l.objectCount - 1] = 1;
            makerEd.selObjects++;
        }
    }
    if (skipped) {
        char text[64];
        wsprintfA(text, "%d items left out: the object budget is full", skipped);
        mkToast(text, 0xff8080ff, 3);
    }
    makerEd.entityCost = edEntityCost();
    makerEd.tool = TOOL_SELECT;
    sound("coin", 1.2f, 0.3f);
}

// Items answer first, then the topmost brick, then the area around the point.
void edEyedropper(float wx, float wy) {
    int o = edGrabAt(wx, wy), t = o < 0 ? edTopTileAt(wx, wy) : -1;
    if (o < 0 && t < 0)
        o = edObjectAt(wx, wy, true);
    if (o >= 0) {
        const MakerObject &mo = makerEd.level.objects[o];
        for (int d = 0; d < makerObjectDefCount; d++)
            if (makerObjectDefs[d].klass == mo.klass) {
                edDefBrush(d);
                makerEd.brush.w = mo.w;
                makerEd.brush.h = mo.h;
                makerEd.brush.angle = mo.angle;
                makerEd.brush.setMask = mo.setMask;
                memcpy(makerEd.brush.param, mo.param, sizeof(mo.param));
                edAutoBrushGrid();
                edRememberBrush();
                makerEd.tool = TOOL_PEN;
                mkToast(makerObjectDefs[d].name);
                return;
            }
    }
    if (t >= 0) {
        const MakerTile &tile = makerEd.level.tiles[t];
        MakerBrush &b = makerEd.brush;
        memset(&b, 0, sizeof(b));
        b.def = -1;
        b.stretch = (tile.flags & TILE_FILL) != 0;
        b.piece = b.stretch ? makerFindFill(tile.group, tile.texture, tile.uv)
                            : makerFindPiece(tile.group, tile.texture, makerRound(tile.r - tile.l),
                                             makerRound(tile.t - tile.b), tile.uv, false);
        b.tile = tile;
        b.tile.r -= b.tile.l;
        b.tile.t -= b.tile.b;
        b.tile.l = b.tile.b = 0;
        b.collideMode = tile.collide;
        makerEd.layer = tile.layer;
        makerEd.tool = TOOL_PEN;
        edAutoBrushGrid();
        edRememberBrush();
        mkToast(b.stretch ? "Picked fill" : "Picked tile");
    }
}

// ---------------------------------------------------------------------------------------------
// Palette.

void edBuildPalette() {
    makerEd.paletteCount = 0;
    int tab = makerEd.tab;
    if (tab == TAB_RECENT) {
        for (int i = 0; i < makerEd.recentCount; i++)
            makerEd.palette[makerEd.paletteCount++] = MakerPaletteItem{2, i};
        return;
    }
    if (edPieceTab(tab)) {
        int category = tab - TAB_BLOCKS;
        for (int i = 0;
             i < makerCategoryCount[category] && makerEd.paletteCount < MAKER_PALETTE_MAX; i++) {
            int piece = makerCategoryList[category][i];
            if (makerEd.world && makerPieces[piece].group != makerEd.world)
                continue;
            makerEd.palette[makerEd.paletteCount++] = MakerPaletteItem{0, piece};
        }
        return;
    }
    int category = tab - TAB_ITEMS;
    for (int i = 0; i < makerObjectDefCount; i++)
        if (makerObjectDefs[i].category == category)
            makerEd.palette[makerEd.paletteCount++] = MakerPaletteItem{1, i};
}

void edPickPalette(int index) {
    if (index < 0 || index >= makerEd.paletteCount)
        return;
    MakerPaletteItem item = makerEd.palette[index];
    if (item.kind == 0)
        edPieceBrush(item.index);
    else if (item.kind == 1)
        edDefBrush(item.index);
    else
        makerEd.brush = makerEd.recent[item.index];
    edAutoBrushGrid();
    if (item.kind != 2)
        edRememberBrush();
    if (makerEd.tool != TOOL_PEN)
        makerEd.tool = TOOL_PEN;
    sound("switch", 1.1f, 0.4f);
}

// ---------------------------------------------------------------------------------------------
// Drawing helpers.

const char *edAnimation(Actor *a, Object *c) {
    if (!a || !a->count)
        return "";
    const char *preferred = c ? c->props.get("default-animation") : "";
    // Empty names are real authored default clips. Many gizmos also have an "idle" clip
    // with texture -1, so skipping the empty name makes their artwork disappear.
    if (c) {
        if (c->has("CrumblingPlatformBehaviour"))
            preferred = "on";
        else if (c->has("MagnetBehaviour"))
            preferred = "idle";
        else if (c->has("CoinGroupBehaviour"))
            preferred = "diamond 1";
        else if (c->has("AlternatorSwitchBehaviour"))
            preferred = c->behavior("AlternatorSwitchBehaviour")->flag("Alternation") ? "on" : "off";
        else if (c->has("AlternatorBlockBehaviour") || c->has("AlternatorTouchBlockBehaviour"))
            preferred = "on";
        else if (c->has("WalkBehaviour"))
            preferred = c->behavior("WalkBehaviour")->flag("walk-left") ? "walk-left" : "walk-right";
        else if (c->has("EnemyJumperBehaviour"))
            preferred = "standing";
    }
    for (int i = 0; i < a->count; i++)
        if (eq(a->animations[i].name, preferred) && a->animations[i].count &&
            a->animations[i].frames[0].texture >= 0)
            return preferred;
    const char *names[] = {preferred, "idle", "on",         "walk-right", "standing",
                           "up",      "life", "idle-shoot", "sleeping",   "flying"};
    for (int n = 0; n < 10; n++) {
        if (!*names[n])
            continue;
        for (int i = 0; i < a->count; i++)
            if (eq(a->animations[i].name, names[n]) && a->animations[i].count &&
                a->animations[i].frames[0].texture >= 0)
                return names[n];
    }
    if (a->count > 1 && !*a->animations[0].name)
        return a->animations[1].name;
    return a->animations[0].name;
}

// Keep the editor's start marker, palette icon and placement ghost in the same fixed pose.
// Test play uses the normal player renderer and retains all of Bean's animations.
void edDrawStartBean(Rect r, float alpha = 1) {
    Actor *bean = actor(MAKER_CLASS_BEAN);
    drawActor(bean, r, "jump", 0, 0, false, 0, alpha);
}

// Random previews are stable while editing and never consume the gameplay random stream.
void edDrawGem(Actor *a, Rect r, int style, float time, float alpha = 1) {
    Vec c = center(r);
    int cell[] = {makerRound(c.x), makerRound(c.y)};
    int variant = style > 0 ? style - 1 : (int)(hashBytes(cell, sizeof(cell)) % 5);
    const char *name = coinAnimationVariantNameNative(variant);
    time += coinAnimationInitialTimeNative(c.x, c.y, animationLength(animation(a, name)));
    drawActor(a, r, name, time, 0, false, -1, alpha);
}

void edDrawSwitchInsert(Rect r, float time, float alpha = 1) {
    Vec c = center(r);
    drawActor(actor(MAKER_CLASS_SWITCH_ICON),
              makerCentered(c.x, c.y, (r.r - r.l) * .5f, (r.t - r.b) / 3),
              "shifting", time, 0, false, -1, alpha);
}

void edBindPage(int group, int page) {
    Texture *t = makerAtlasPage(group, page);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, t ? t->gl : 0);
}

void edQuad(float l, float b, float r, float t, const float *uv) {
    glTexCoord2f(uv[0], uv[1]);
    glVertex2f(l, b);
    glTexCoord2f(uv[2], uv[3]);
    glVertex2f(r, b);
    glTexCoord2f(uv[6], uv[7]);
    glVertex2f(r, t);
    glTexCoord2f(uv[4], uv[5]);
    glVertex2f(l, t);
}

void edDrawTile(const MakerTile &t, float dx, float dy, float alpha) {
    edBindPage(t.group, t.texture);
    glColor4f(1, 1, 1, alpha);
    glBegin(GL_QUADS);
    edQuad(t.l + dx, t.b + dy, t.r + dx, t.t + dy, t.uv);
    glEnd();
}

void edFitRect(float w, float h, Rect box, Rect *out) {
    float s = minf((box.r - box.l) / maxf(1, w), (box.t - box.b) / maxf(1, h));
    s = minf(s, 2);
    Vec c = center(box);
    *out = makerCentered(c.x, c.y, w * s, h * s);
}

// Transparency backdrop so translucent fills such as shadows and water tints stay visible.
void edChecker(Rect r, float cell) {
    mkRect(r.l, r.b, r.r - r.l, r.t - r.b, 0x8a8f98ff);
    glBegin(GL_QUADS);
    mkColor(0xc4c9d0ff);
    for (float y = r.b; y < r.t; y += cell)
        for (float x = r.l + ((int)((y - r.b) / cell) & 1) * cell; x < r.r; x += cell * 2) {
            float x1 = minf(x + cell, r.r), y1 = minf(y + cell, r.t);
            glVertex2f(x, y);
            glVertex2f(x1, y);
            glVertex2f(x1, y1);
            glVertex2f(x, y1);
        }
    glEnd();
}

void edDrawPieceIcon(const MakerPiece &p, Rect box) {
    Rect r;
    edFitRect(p.w, p.h, box, &r);
    if (p.category == PIECE_FILL)
        edChecker(r, 4);
    edBindPage(p.group, p.texture);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    edQuad(r.l, r.b, r.r, r.t, p.uv);
    glEnd();
}

u32 edAreaColor(const MakerObjectDef &d) {
    if (d.flags & OBJ_WATER)
        return 0x2f6fd8ff;
    if (d.flags & OBJ_WIND)
        return 0x6fe0c0ff;
    if (d.flags & OBJ_SECRET)
        return 0xb070f0ff;
    if (d.flags & OBJ_PARTICLES)
        return 0xf0e060ff;
    // Invisible walls: red stops enemies, cyan stops Bean, orange turns back moving objects.
    if (d.klass == 794748294457ULL)
        return 0x50d0ffff;
    if (d.klass == 4480186787647947ULL)
        return 0xffa040ff;
    return 0xff5050ff;
}

// A pendulum is an anchor plus spawned links and an end piece. Use the full assembly for
// palette fitting and canvas artwork; motion guides are drawn separately on hover/selection.
Rect edPendulumParts(const MakerObject &o, const MakerObjectDef &d, bool draw, float alpha = 1) {
    Rect bounds = edObjectRect(o);
    Object *c = makerClass(d.klass);
    const Props *b = c ? c->behavior("WreckingBallControllerBehaviour") : 0;
    if (!b)
        return bounds;
    u64 link = idnum(b->get("LinkClass")), end = idnum(b->get("EndClass"));
    Object *lc = makerClass(link), *ec = makerClass(end);
    float half = lc ? (lc->rect.t - lc->rect.b) * .5f : 4,
          spacing = b->num("LinkSpacing", 1), distance = o.h * .5f;
    int count = makerRound(makerParam(o, d, 0));
    float angle = makerParam(o, d, makerParamIndex(d, WRITE_SWING_ANGLE)) * .017453293f;
    float ux = bqSin(angle), uy = -bqCos(angle);
    for (int i = 0; i <= count; i++) {
        Object *part = i < count ? lc : ec;
        distance += i < count ? half + spacing
                             : (ec ? (ec->rect.t - ec->rect.b) * .5f : 16) +
                                   b->num("LastLinkSpacing", -6);
        if (part) {
            Rect r = shifted(part->rect, o.x + ux * distance - center(part->rect).x,
                             o.y + uy * distance - center(part->rect).y);
            bounds = Rect{minf(bounds.l, r.l), minf(bounds.b, r.b),
                          maxf(bounds.r, r.r), maxf(bounds.t, r.t)};
            if (draw) {
                Actor *a = actor(i < count ? link : end);
                drawActor(a, r, edAnimation(a, part), 0, angle * 57.29578f, false, -1, alpha);
            }
        }
        if (i < count)
            distance += half;
    }
    return bounds;
}

void edDrawDefIcon(int defIndex, Rect box, float time) {
    const MakerObjectDef &d = makerObjectDefs[defIndex];
    if (d.flags & OBJ_START) {
        Rect r;
        edFitRect(32, 32, box, &r);
        edDrawStartBean(r);
        return;
    }
    if (d.flags & (OBJ_FREE | OBJ_INVISIBLE)) {
        if ((d.flags & OBJ_INVISIBLE) && !(d.flags & OBJ_FREE)) {
            Actor *a = actor(d.klass);
            const char *name = edAnimation(a, makerClass(d.klass));
            Animation *an = animation(a, name);
            if (an && an->count && an->frames[0].texture >= 0) {
                Rect r;
                edFitRect(32, 32, box, &r);
                drawActor(a, r, name, 0);
                return;
            }
        }
        u32 c = edAreaColor(d);
        float inset = (box.r - box.l) * 0.18f;
        mkRect(box.l + inset, box.b + inset, box.r - box.l - inset * 2, box.t - box.b - inset * 2,
               (c & 0xffffff00) | 0x90);
        mkFrame(box.l + inset, box.b + inset, box.r - box.l - inset * 2, box.t - box.b - inset * 2,
                c | 0xff);
        char letter[2] = {d.name[0], 0};
        mkTextCentered((box.l + box.r) * 0.5f, (box.b + box.t) * 0.5f, letter, 10, 0xffffffff,
                       true);
        return;
    }
    u64 klass = d.klass;
    Object *c = makerClass(klass);
    Actor *a = actor(klass);
    Rect cr = c ? c->rect : Rect{-16, -16, 16, 16};
    if (c && c->has("WreckingBallControllerBehaviour")) {
        MakerObject o = {};
        o.klass = klass;
        o.w = cr.r - cr.l;
        o.h = cr.t - cr.b;
        Rect bounds = edPendulumParts(o, d, false), fitted;
        edFitRect(bounds.r - bounds.l, bounds.t - bounds.b, box, &fitted);
        float scale = (fitted.r - fitted.l) / maxf(1, bounds.r - bounds.l);
        glPushMatrix();
        glTranslatef(fitted.l, fitted.b, 0);
        glScalef(scale, scale, 1);
        glTranslatef(-bounds.l, -bounds.b, 0);
        drawActor(a, edObjectRect(o), edAnimation(a, c), 0);
        edPendulumParts(o, d, true);
        glPopMatrix();
        return;
    }
    float w = d.cellW > 0 ? d.cellW : cr.r - cr.l, h = d.cellH > 0 ? d.cellH : cr.t - cr.b;
    Rect r;
    edFitRect(w, h, box, &r);
    bool breakable = c && c->has("BreakableBehaviour");
    drawActor(a, r, edAnimation(a, c), breakable ? 0 : time, 0, false, breakable ? 0 : -1);
    if (klass == MAKER_CLASS_SWITCH)
        edDrawSwitchInsert(r, time);
}

void edDrawBrushIcon(const MakerBrush &b, Rect box, float time) {
    if (b.object) {
        if (makerObjectDefs[b.def].klass == MAKER_CLASS_DIAMOND) {
            Rect r;
            edFitRect(32, 32, box, &r);
            int style = b.setMask & 1 ? makerRound(clamp(b.param[0], 0, 5)) : 0;
            edDrawGem(actor(MAKER_CLASS_DIAMOND), r, style, time);
            return;
        }
        edDrawDefIcon(b.def, box, time);
        return;
    }
    MakerTile t = edTrimTile(b);
    Rect r;
    edFitRect(t.r - t.l, t.t - t.b, box, &r);
    edBindPage(t.group, t.texture);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    edQuad(r.l, r.b, r.r, r.t, t.uv);
    glEnd();
}

void edDashedLine(float x0, float y0, float x1, float y1, u32 color) {
    float dx = x1 - x0, dy = y1 - y0, length = root(dx * dx + dy * dy);
    int dashes = (int)(length / 8) + 1;
    glDisable(GL_TEXTURE_2D);
    mkColor(color);
    glBegin(GL_LINES);
    for (int i = 0; i < dashes; i += 2) {
        float a = (float)i / dashes, b = minf(1, (float)(i + 1) / dashes);
        glVertex2f(x0 + dx * a, y0 + dy * a);
        glVertex2f(x0 + dx * b, y0 + dy * b);
    }
    glEnd();
}

void edCircle(float x, float y, float radius, u32 color) {
    glDisable(GL_TEXTURE_2D);
    mkColor(color);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 40; i++) {
        float a = i * 6.2831853f / 40;
        glVertex2f(x + bqCos(a) * radius, y + bqSin(a) * radius);
    }
    glEnd();
}

void edArrow(float x, float y, float dx, float dy, float length, u32 color) {
    float ex = x + dx * length, ey = y + dy * length;
    mkLine(x, y, ex, ey, color, 2);
    float px = -dy, py = dx;
    mkTriangle(ex + dx * 6, ey + dy * 6, ex + px * 5, ey + py * 5, ex - px * 5, ey - py * 5, color);
}

// Behaviour previews: patrol paths, orbits, pendulum chains and firing directions.
void edDrawObjectExtras(const MakerObject &o, const MakerObjectDef &d, bool strong) {
    u32 path = strong ? 0xffe070ff : 0xffe07070;
    Object *c = makerClass(makerEffectiveClass(o, d));
    if (!c)
        return;
    if (makerParamIndex(d, WRITE_AXIS) >= 0) {
        // The object starts where it stands; the path spreads behind and ahead of it.
        MakerOscillator osc = makerOscillator(o, d);
        float before = osc.start * osc.travel, after = osc.travel - before;
        float ux = osc.vertical ? 0.0f : 1.0f, uy = osc.vertical ? 1.0f : 0.0f;
        float x0 = o.x - ux * before, y0 = o.y - uy * before, x1 = o.x + ux * after,
              y1 = o.y + uy * after;
        if (osc.travel > 0) {
            edDashedLine(x0, y0, x1, y1, path);
            if (strong) {
                mkRect(x0 - 2, y0 - 2, 4, 4, path);
                mkRect(x1 - 2, y1 - 2, 4, 4, path);
                edArrow(o.x, o.y, ux * osc.sign, uy * osc.sign, 20, 0xffffffd0);
            }
        }
    }
    if (c->has("OrbitBehaviour")) {
        // The placed position is the orbit centre; the marker shows where the flight begins.
        float radius = makerParam(o, d, 0), start = makerParam(o, d, 3) * 0.017453293f;
        edCircle(o.x, o.y, absf(radius), path);
        mkRect(o.x - 2, o.y - 2, 4, 4, path);
        mkRect(o.x + bqCos(start) * radius - 3, o.y + bqSin(start) * radius - 3, 6, 6, path);
    }
    if (c->has("OrbitingObjectBehaviour")) {
        const Props *b = c->behavior("OrbitingObjectBehaviour");
        int radiusIndex = makerParamByKey(d, "radius"),
            angleIndex = makerParamByKey(d, "start-angle");
        float radius = radiusIndex >= 0 ? makerParam(o, d, radiusIndex) : b->num("radius", 40);
        float angle =
            (angleIndex >= 0 ? makerParam(o, d, angleIndex) : b->num("start-angle")) * 0.017453293f;
        u64 child = idnum(b->get("class"));
        Object *cc = makerClass(child);
        Rect cr = cc ? cc->rect : Rect{-16, -16, 16, 16};
        float cx = o.x + bqCos(angle) * radius, cy = o.y + bqSin(angle) * radius;
        edCircle(o.x, o.y, radius, path);
        drawActor(actor(child), shifted(cr, cx - center(cr).x, cy - center(cr).y),
                  edAnimation(actor(child), cc), makerEd.age);
    }
    if (c->has("WreckingBallControllerBehaviour")) {
        // The chain hangs at its start angle; dashed lines mark both ends of the swing.
        const Props *b = c->behavior("WreckingBallControllerBehaviour");
        u64 link = idnum(b->get("LinkClass")), end = idnum(b->get("EndClass"));
        Object *lc = makerClass(link), *ec = makerClass(end);
        float linkHalf = lc ? (lc->rect.t - lc->rect.b) * 0.5f : 4,
              spacing = b->num("LinkSpacing", 1), distance = o.h * 0.5f;
        int links = makerRound(makerParam(o, d, 0)), dir = makerParamIndex(d, WRITE_SWING_DIR);
        float maxAngle = makerParam(o, d, 2) * 0.017453293f,
              start = makerParam(o, d, makerParamIndex(d, WRITE_SWING_ANGLE)) * 0.017453293f;
        float ux = bqSin(start), uy = -bqCos(start);
        for (int i = 0; i < links; i++) {
            distance += linkHalf + spacing;
            distance += linkHalf;
        }
        float endHalf = ec ? (ec->rect.t - ec->rect.b) * 0.5f : 16;
        distance += endHalf + b->num("LastLinkSpacing", -6);
        float ex = o.x + ux * distance, ey = o.y + uy * distance;
        float sx = bqSin(maxAngle) * distance, sy = bqCos(maxAngle) * distance;
        edDashedLine(o.x, o.y, o.x - sx, o.y - sy, path);
        edDashedLine(o.x, o.y, o.x + sx, o.y - sy, path);
        if (strong) {
            float turn = dir >= 0 && makerParam(o, d, dir) > 0.5f ? 1.0f : -1.0f;
            edArrow(ex, ey, -uy * turn, ux * turn, 20, 0xffffffd0);
        }
    }
    int heading = makerParamIndex(d, WRITE_DIAGONAL);
    if (heading >= 0 && strong) {
        int h = makerRound(makerParam(o, d, heading)) & 3;
        edArrow(o.x, o.y, (h & 1 ? 1 : -1) * 0.7071f, (h & 2 ? 1 : -1) * 0.7071f, 24, 0xffffffd0);
    }
    if (d.flags & OBJ_TURRET) {
        // Longer arrows for faster bullets.
        const float dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
        int direction = makerRound(makerParam(o, d, 0)) & 3,
            speedIndex = makerParamIndex(d, WRITE_BULLET_SPEED);
        float speed = speedIndex >= 0 ? makerParam(o, d, speedIndex) : 125;
        edArrow(o.x + dx[direction] * 18, o.y + dy[direction] * 18, dx[direction], dy[direction],
                strong ? 16 + speed * 0.12f : 20, path);
    }
    if (d.flags & OBJ_WIND) {
        const float dx[4] = {0, 0, -1, 1}, dy[4] = {1, -1, 0, 0};
        int direction = makerRound(makerParam(o, d, 0)) & 3;
        for (int i = -1; i <= 1; i++) {
            float ox = dy[direction] != 0 ? i * o.w * 0.25f : 0,
                  oy = dx[direction] != 0 ? i * o.h * 0.25f : 0;
            edArrow(o.x + ox - dx[direction] * 16, o.y + oy - dy[direction] * 16, dx[direction],
                    dy[direction], 20, 0xffffffc0);
        }
    }
}

void edRefreshWater() {
    Arena &m = makerEd.waterArena;
    if (!m.data)
        m.init(8 * 1024 * 1024);
    m.reset();
    makerEd.water = Level{};
    makerEd.water.objectCount = makerEd.level.objectCount * 2;
    makerEd.water.objects = m.array<Object>(makerEd.water.objectCount);
    makerEd.waterGeneration++;
    for (int i = 0; i < makerEd.level.objectCount; i++) {
        const MakerObject &o = makerEd.level.objects[i];
        const MakerObjectDef *d = makerDefFor(o.klass);
        if (!d || !(d->flags & OBJ_WATER))
            continue;
        MakerObjectList list = {};
        list.items = makerEd.water.objects + i * 2;
        list.capacity = 2;
        list.nextId = ((u64)makerEd.waterGeneration << 32) + i * 2 + 1;
        makerAddWaterParticles(list, edObjectRect(o), 0, m);
    }
    makerEd.waterAge = 0;
    makerEd.waterDirty = false;
}

void edDrawWater(Rect r, float alpha, int index) {
    float top = r.t + 2, split = maxf(r.b, top - 64);
    float uv[8];
    memcpy(uv, makerWaterGradientUv, sizeof(uv));
    uv[0] = uv[2] = (547 + top - split) / 1024;
    edBindPage(4, 0);
    glColor4f(1, 1, 1, alpha);
    glBegin(GL_QUADS);
    edQuad(r.l, split, r.r, top, uv);
    if (split > r.b)
        edQuad(r.l, r.b, r.r, split, makerWaterDeepUv);
    glEnd();
    if (index < 0 || index * 2 + 1 >= makerEd.water.objectCount)
        return;
    for (int k = 0; k < 2; k++) {
        Object &source = makerEd.water.objects[index * 2 + k];
        if (source.base != 5)
            continue;
        Rect emitter = k ? Rect{r.l + 16, r.b + 8, r.r - 16, r.t + 2} : makerWaterWaveRect(r);
        if (emitter.r > emitter.l && emitter.t > emitter.b)
            drawParticles(source, actor(source.klass), emitter, makerEd.waterAge, 1, alpha);
    }
}

void edDrawObject(const MakerObject &o, float dx, float dy, float alpha, int index = -1) {
    const MakerObjectDef *d = makerDefFor(o.klass);
    if (!d)
        return;
    MakerObject m = o;
    m.x += dx;
    m.y += dy;
    Rect r = edObjectRect(m);
    if (d->flags & (OBJ_FREE | OBJ_INVISIBLE)) {
        u32 c = edAreaColor(*d);
        bool textured = false;
        if (d->flags & OBJ_WATER) {
            edDrawWater(r, alpha, index);
            textured = true;
        }
        if ((d->flags & OBJ_INVISIBLE) && !(d->flags & OBJ_FREE)) {
            Actor *a = actor(d->klass);
            const char *name = edAnimation(a, makerClass(d->klass));
            Animation *an = animation(a, name);
            textured = an && an->count && an->frames[0].texture >= 0;
            if (textured)
                for (float y = r.b; y < r.t - .5f; y += 32)
                    for (float x = r.l; x < r.r - .5f; x += 32)
                        drawActor(a, Rect{x, y, minf(x + 32, r.r), minf(y + 32, r.t)}, name,
                                  0, 0, false, 0, alpha);
        }
        // Show the authored wall tiles while editing, with a frame identifying their collision role.
        if (!textured)
            mkRect(r.l, r.b, r.r - r.l, r.t - r.b, (c & 0xffffff00) | (u32)(int)(80 * alpha));
        mkFrame(r.l, r.b, r.r - r.l, r.t - r.b, (c & 0xffffff00) | (u32)(int)(255 * alpha),
                1 / makerEd.zoom);
        if (r.r - r.l >= 20 / makerEd.zoom) {
            // The name tag keeps its screen size at every zoom; it is also the area's handle.
            glPushMatrix();
            glTranslatef(r.l + 3 / makerEd.zoom, r.t - (mkLineHeight(8) + 1) / makerEd.zoom, 0);
            glScalef(1 / makerEd.zoom, 1 / makerEd.zoom, 1);
            mkText(0, 0, d->name, 8, 0xffffff00 | (u32)(int)(255 * alpha));
            glPopMatrix();
        }
        if (d->flags & OBJ_WIND)
            edDrawObjectExtras(m, *d, false);
        return;
    }
    u64 klass = makerEffectiveClass(m, *d);
    Object *c = makerClass(klass);
    Actor *a = actor(klass);
    const char *anim = edAnimation(a, c);
    if (d->klass == 9010792101233554060ULL || d->klass == 11234016302369745107ULL) {
        int solid = makerParamIndex(*d, WRITE_SET);
        anim = solid >= 0 && makerParam(m, *d, solid) < 0.5f ? "off" : "on";
    }
    // Spikes that start retracted are shown retracted.
    int out = makerParamByKey(*d, "alternation");
    if (out >= 0 && makerParamUsable(*d, out) && makerParam(m, *d, out) < 0.5f)
        anim = "off";
    float angle = makerEffectiveAngle(m, *d);
    if (c && c->base == 7 && d->cellW > 0) {
        // Only square spike cells rotate, so the cell grid never changes orientation.
        float cw = d->cellW, ch = d->cellH;
        for (float y = r.b; y < r.t - 0.5f; y += ch)
            for (float x = r.l; x < r.r - 0.5f; x += cw) {
                Rect cell = {x, y, minf(x + cw, r.r), minf(y + ch, r.t)};
                if (d->klass == MAKER_CLASS_DIAMOND)
                    edDrawGem(a, cell, makerRound(makerParam(m, *d, 0)), makerEd.age, alpha);
                else
                    drawActor(a, cell, anim, makerEd.age, angle, false, -1, alpha);
            }
    } else {
        bool breakable = c && c->has("BreakableBehaviour");
        drawActor(a, r, anim, breakable ? 0 : makerEd.age, angle, false, breakable ? 0 : -1, alpha);
    }
    if (c && c->has("WreckingBallControllerBehaviour"))
        edPendulumParts(m, *d, true, alpha);
    if (klass == MAKER_CLASS_SWITCH)
        edDrawSwitchInsert(r, makerEd.age, alpha);
    // Editor-only colour tag of the switch group: switches, switch blocks, spikes that follow a
    // group and enemies that flip one when defeated. In play every group looks alike.
    int tag = makerParamIndex(*d, WRITE_DEFEAT), shift = 0;
    if (tag < 0)
        tag = makerParamIndex(*d, WRITE_MASK0);
    if (tag < 0) {
        tag = makerParamIndex(*d, WRITE_MASK);
        shift = 1;
    }
    int colour =
        tag >= 0 && makerParamUsable(*d, tag) ? makerRound(makerParam(m, *d, tag)) + shift : 0;
    const u32 colours[4] = {0xff4060ff, 0x4080ffff, 0x40d060ff, 0xffd040ff};
    u32 tagColour = colour > 0 ? colours[(colour - 1) & 3] : 0;
    if (klass == MAKER_CLASS_POTION)
        tagColour = makerParam(m, *d, 0) > .5f ? 0x40d060ff : 0xb050e0ff;
    if (tagColour) {
        float size = 7 / makerEd.zoom;
        mkRect(r.r - size, r.t - size, size, size,
               (tagColour & 0xffffff00) | (u32)(int)(255 * alpha));
        mkFrame(r.r - size, r.t - size, size, size, 0x000000c0, 1 / makerEd.zoom);
    }
    // Actors that wait for Bean carry a small sleep mark.
    int activity = makerParamIndex(*d, WRITE_ACTIVITY);
    if (activity >= 0 && makerParamUsable(*d, activity) &&
        makerRound(makerParam(m, *d, activity)) == 1) {
        glPushMatrix();
        glTranslatef(r.l, r.t, 0);
        glScalef(1 / makerEd.zoom, 1 / makerEd.zoom, 1);
        mkTextShadow(0, 0, "zz", 7, 0xc8e4ffff, true);
        glPopMatrix();
    }
}

// Parallax backdrop in screen space, sharing the play-time tiling and layer camera formula.
void edDrawBackground() {
    Level &bg = makerEd.background;
    float bottom = edCanvasBottom(), w = viewW, h = edCanvasTop() - bottom;
    mkGradient(0, bottom, w, h, 0x2c3e50ff, 0x4a6fa5ff);
    if (!bg.layerCount)
        return;
    float centerX = makerEd.camX + w / makerEd.zoom * 0.5f,
          centerY = makerEd.camY + h / makerEd.zoom * 0.5f;
    float left = centerX - w * 0.5f, low = centerY - h * 0.5f;
    for (int layer = 0; layer < bg.layerCount; layer++) {
        Layer &l = bg.layers[layer];
        float cx =
            layerCameraAxis(left, w, bg.size.x, l.scroll.x, l.autoScroll.x, l.size.x, false, 0);
        float cy =
            layerCameraAxis(low, h, bg.size.y, l.scroll.y, l.autoScroll.y, l.size.y, false, 0);
        glPushMatrix();
        glTranslatef(w * 0.5f - cx, bottom + h * 0.5f - cy, 0);
        int last = -2147483647;
        for (;;) {
            int next = 2147483647;
            for (int i = 0; i < bg.tileCount; i++)
                if (bg.tiles[i].layer == layer && bg.tiles[i].z > last && bg.tiles[i].z < next)
                    next = bg.tiles[i].z;
            if (next == 2147483647)
                break;
            for (int i = 0; i < bg.tileCount; i++)
                if (bg.tiles[i].layer == layer && bg.tiles[i].z == next)
                    drawTile(bg.tiles[i], &bg, 0);
            last = next;
        }
        for (int i = 0; i < bg.objectCount; i++) {
            Object &o = bg.objects[i];
            if (o.layer != layer || o.base != 1)
                continue;
            Rect r = backgroundRect(o, o.rect, makerEd.age, cx - w * 0.5f, cy - h * 0.5f);
            Actor *a = actor(o.klass);
            drawActor(a, r, edAnimation(a, &o), makerEd.age, o.angle);
        }
        glPopMatrix();
    }
}

// Weather stays in screen space at every editor zoom and is clipped away from the UI panels.
void edDrawWeather() {
    float bottom = edCanvasBottom(), height = edCanvasTop() - bottom;
    glPushAttrib(GL_SCISSOR_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(viewportX, viewportY + (int)(bottom * screenScale), viewportW,
              (int)(height * screenScale));
    for (int i = 0; i < makerEd.background.objectCount; i++) {
        Object &o = makerEd.background.objects[i];
        if (!o.props.flag("maker-weather"))
            continue;
        o.rect = makerWeatherEmitterRect(o, viewW, height);
        drawParticles(o, actor(o.klass), shifted(o.rect, 0, bottom), makerEd.weatherAge);
    }
    glPopAttrib();
}

void edEnsureCollision() {
    if (!makerEd.collisionDirty)
        return;
    makerEd.collisionDirty = false;
    if (!makerEd.collisionArena.data)
        makerEd.collisionArena.init(8 * 1024 * 1024);
    makerEd.collisionArena.reset();
    memset(&makerEd.collision, 0, sizeof(makerEd.collision));
    makerBakeGeometry(makerEd.level, makerEd.collision, makerEd.collisionArena);
}

void edDrawCollision() {
    edEnsureCollision();
    Level &c = makerEd.collision;
    glDisable(GL_TEXTURE_2D);
    glLineWidth(2);
    for (int i = 0; i < c.geometryCount; i++) {
        Geometry &g = c.geometry[i];
        mkColor(g.kind == 2 ? 0x40f0ffff : g.count == 4 ? 0xffa030ff : 0xff3030ff);
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < g.count; j++)
            glVertex2f(g.points[j].x, g.points[j].y);
        glEnd();
    }
    glLineWidth(1);
}

void edDrawGrid(float l, float b, float r, float t) {
    float g = (float)makerEd.grid;
    while (g * makerEd.zoom < 6)
        g *= 2;
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    for (float x = makerFloor(l / g) * g; x <= r; x += g) {
        bool major = makerRound(x) % 128 == 0;
        glColor4f(1, 1, 1, major ? 0.16f : 0.07f);
        glVertex2f(x, b);
        glVertex2f(x, t);
    }
    for (float y = makerFloor(b / g) * g; y <= t; y += g) {
        bool major = makerRound(y) % 128 == 0;
        glColor4f(1, 1, 1, major ? 0.16f : 0.07f);
        glVertex2f(l, y);
        glVertex2f(r, y);
    }
    glEnd();
}

// Visible tiles of one layer, batched into one quad list per atlas page run.
void edDrawTiles(int layer, float alpha, float dx, float dy, bool selectedOnly) {
    MakerLevel &l = makerEd.level;
    float vl = edWorldX(0) - dx, vr = edWorldX(viewW) - dx, vb = edWorldY(edCanvasBottom()) - dy,
          vt = edWorldY(edCanvasTop()) - dy;
    int boundGroup = -1, boundPage = -1;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < l.tileCount; i++) {
            const MakerTile &t = l.tiles[i];
            if (t.layer != layer || (selectedOnly && !makerEd.tileSel[i]))
                continue;
            if (t.r < vl || t.l > vr || t.t < vb || t.b > vt)
                continue;
            bool opaque = (t.flags & TILE_OPAQUE) != 0;
            if (layer == MAKER_MAIN ? (pass == 0) != opaque : pass == 1)
                continue;
            if (!selectedOnly && makerEd.drag == DRAG_MOVE && makerEd.tileSel[i])
                continue;
            if (t.group != boundGroup || t.texture != boundPage) {
                if (boundGroup >= 0)
                    glEnd();
                edBindPage(t.group, t.texture);
                glColor4f(1, 1, 1, alpha);
                glBegin(GL_QUADS);
                boundGroup = t.group;
                boundPage = t.texture;
            }
            edQuad(t.l + dx, t.b + dy, t.r + dx, t.t + dy, t.uv);
        }
    if (boundGroup >= 0)
        glEnd();
}

// Snapped move delta for the current drag, in world units.
void edMoveDelta(float *dx, float *dy) {
    float g = (float)makerEd.grid;
    float wx = edWorldX(makerEd.mouseX), wy = edWorldY(makerEd.mouseY);
    *dx = makerRound((wx - makerEd.dragWorldX) / g) * g;
    *dy = makerRound((wy - makerEd.dragWorldY) / g) * g;
}

// Preview and commit share the same snapped size, without changing the course during a drag.
// Relative movement preserves the initial grip offset instead of jumping to the pointer.
void edBoundsDragSize(float wx, float wy, int *w, int *h) {
    const MakerHeader &head = makerEd.level.head;
    *w = head.width;
    *h = head.height;
    if (makerEd.boundsEdge & 1)
        *w = (int)clamp((float)(makerRound((head.width + wx - makerEd.dragWorldX) / 32) * 32),
                        MAKER_MIN_W, MAKER_MAX_W);
    if (makerEd.boundsEdge & 2)
        *h = (int)clamp((float)(makerRound((head.height + wy - makerEd.dragWorldY) / 32) * 32),
                        MAKER_MIN_H, MAKER_MAX_H);
}

// Keep the opposite edge anchored and retain the initial grip offset. The same object is
// used for live artwork, selection handles, property values and the single undoable commit.
MakerObject edResizeDragObject(float wx, float wy) {
    MakerObject o = makerEd.level.objects[makerEd.propObject];
    const MakerObjectDef *d = makerDefFor(o.klass);
    if (!d)
        return o;
    float cw = d->cellW > 0 ? d->cellW : 16, ch = d->cellH > 0 ? d->cellH : 16;
    if (d->flags & OBJ_FREE)
        cw = ch = maxf(8, (float)makerEd.grid);
    float dx = makerRound((wx - makerEd.dragWorldX) / cw) * cw,
          dy = makerRound((wy - makerEd.dragWorldY) / ch) * ch;
    Rect r = edObjectRect(o);
    if (makerEd.resizeEdge == 1)
        r.r = maxf(r.l + cw, r.r + dx);
    if (makerEd.resizeEdge == 2)
        r.l = minf(r.r - cw, r.l + dx);
    if (makerEd.resizeEdge == 3)
        r.t = maxf(r.b + ch, r.t + dy);
    if (makerEd.resizeEdge == 4)
        r.b = minf(r.t - ch, r.b + dy);
    o.w = r.r - r.l;
    o.h = r.t - r.b;
    o.x = (r.l + r.r) * .5f;
    o.y = (r.b + r.t) * .5f;
    return o;
}

void edDrawCanvas() {
    MakerLevel &level = makerEd.level;
    MakerHeader &h = level.head;
    float bottom = edCanvasBottom(), top = edCanvasTop();
    edDrawBackground();
    glPushMatrix();
    glTranslatef(0, bottom, 0);
    glScalef(makerEd.zoom, makerEd.zoom, 1);
    glTranslatef(-makerEd.camX, -makerEd.camY, 0);
    float vl = makerEd.camX, vb = makerEd.camY, vr = edWorldX(viewW), vt = edWorldY(top);
    float moveX = 0, moveY = 0;
    if (makerEd.drag == DRAG_MOVE)
        edMoveDelta(&moveX, &moveY);
    bool dim = makerEd.dimOthers;
    bool resizing = makerEd.drag == DRAG_RESIZE && makerEd.propObject >= 0 &&
                    makerEd.propObject < level.objectCount;
    MakerObject preview = {};
    if (resizing)
        preview = edResizeDragObject(edWorldX(makerEd.mouseX), edWorldY(makerEd.mouseY));
    for (int layer = 0; layer < MAKER_LAYERS; layer++) {
        float alpha = dim && layer != makerEd.layer ? 0.35f : 1;
        if (layer == MAKER_FRONT) {
            // Objects sit between the main and front tile layers, as in play.
            for (int i = 0; i < level.objectCount; i++) {
                if (makerEd.drag == DRAG_MOVE && makerEd.objectSel[i])
                    continue;
                const MakerObject &o =
                    resizing && i == makerEd.propObject ? preview : level.objects[i];
                Rect r = edObjectRect(o);
                const MakerObjectDef *d = makerDefFor(o.klass);
                Object *c = d ? makerClass(d->klass) : 0;
                if (c && c->has("WreckingBallControllerBehaviour"))
                    r = edPendulumParts(o, *d, false);
                if (r.r < vl - 64 || r.l > vr + 64 || r.t < vb - 64 || r.b > vt + 64)
                    continue;
                edDrawObject(o, 0, 0, 1, i);
            }
            float sx = h.startX, sy = h.startY;
            if (makerEd.drag == DRAG_START) {
                sx = edSnapCoord(edWorldX(makerEd.mouseX), 32) + 16;
                sy = edSnapCoord(edWorldY(makerEd.mouseY), 32) + 16;
                edClampStart(&sx, &sy);
            }
            edDrawStartBean(makerCentered(sx, sy, 32, 32));
            // Readable at every zoom, so the start is easy to find on a whole-course view.
            glPushMatrix();
            glTranslatef(sx, sy + 17, 0);
            glScalef(1 / makerEd.zoom, 1 / makerEd.zoom, 1);
            mkText(-mkTextWidth("START", 7, true) * 0.5f, 0, "START", 7, 0x80ff80ff, true);
            glPopMatrix();
            if (makerEd.layer == MAKER_BACK && !dim)
                alpha = 0.45f;
        }
        edDrawTiles(layer, alpha, 0, 0, false);
    }
    if (makerEd.drag == DRAG_MOVE) {
        for (int layer = 0; layer < MAKER_LAYERS; layer++)
            edDrawTiles(layer, 0.8f, moveX, moveY, true);
        for (int i = 0; i < level.objectCount; i++)
            if (makerEd.objectSel[i])
                edDrawObject(level.objects[i], moveX, moveY, 0.8f, i);
    }
    // Course bounds: dim the outside and mark the resizable right and top edges.
    int boundsW = h.width, boundsH = h.height;
    if (makerEd.drag == DRAG_BOUNDS)
        edBoundsDragSize(edWorldX(makerEd.mouseX), edWorldY(makerEd.mouseY), &boundsW, &boundsH);
    float W = (float)boundsW, H = (float)boundsH, big = 100000;
    mkRect(-big, -big, big * 2, big, 0x00000070);
    mkRect(-big, H, big * 2, big, 0x00000070);
    mkRect(-big, 0, big, H, 0x00000070);
    mkRect(W, 0, big, H, 0x00000070);
    mkFrame(0, 0, W, H, 0xffffffa0, 1.5f / makerEd.zoom);
    float grip = 10 / makerEd.zoom;
    mkRect(W - grip * 0.3f, H * 0.5f - grip, grip * 0.6f, grip * 2,
           makerEd.boundsEdge & 1 ? 0xffe070ff : 0xffffffb0);
    mkRect(W * 0.5f - grip, H - grip * 0.3f, grip * 2, grip * 0.6f,
           makerEd.boundsEdge & 2 ? 0xffe070ff : 0xffffffb0);
    if (makerEd.dimOthers || makerEd.tool == TOOL_PEN || makerEd.tool == TOOL_SELECT)
        edDrawGrid(maxf(vl, -64), maxf(vb, -64), minf(vr, W + 64), minf(vt, H + 64));
    if (makerEd.showCollision)
        edDrawCollision();
    // Hover, selection and brush feedback.
    float wx = edWorldX(makerEd.mouseX), wy = edWorldY(makerEd.mouseY);
    bool canvas = edInCanvas(makerEd.mouseX, makerEd.mouseY) && !makerEd.settings &&
                  !makerEd.picker && !makerEd.help;
    for (int i = 0; i < level.objectCount; i++) {
        const MakerObject &o = resizing && i == makerEd.propObject ? preview : level.objects[i];
        const MakerObjectDef *d = makerDefFor(o.klass);
        bool selected = makerEd.objectSel[i] || makerEd.propObject == i;
        if (d && (selected || mkInside(edObjectRect(o), wx, wy)))
            edDrawObjectExtras(o, *d, selected);
        if (selected) {
            Rect r = shifted(edObjectRect(o), makerEd.drag == DRAG_MOVE ? moveX : 0,
                             makerEd.drag == DRAG_MOVE ? moveY : 0);
            mkFrame(r.l, r.b, r.r - r.l, r.t - r.b, 0xffe070ff, 1.5f / makerEd.zoom);
        }
    }
    for (int i = 0; i < level.tileCount; i++)
        if (makerEd.tileSel[i]) {
            const MakerTile &t = level.tiles[i];
            float ox = makerEd.drag == DRAG_MOVE ? moveX : 0,
                  oy = makerEd.drag == DRAG_MOVE ? moveY : 0;
            mkFrame(t.l + ox, t.b + oy, t.r - t.l, t.t - t.b, 0xffe070c0, 1 / makerEd.zoom);
        }
    // Resize handles of a single selected resizable object.
    if (makerEd.propObject >= 0 && makerEd.propObject < level.objectCount) {
        const MakerObject &o = resizing ? preview : level.objects[makerEd.propObject];
        const MakerObjectDef *d = makerDefFor(o.klass);
        if (d && (d->flags & (OBJ_RESIZE_X | OBJ_RESIZE_Y))) {
            Rect r = edObjectRect(o);
            float s = 4 / makerEd.zoom;
            if (d->flags & OBJ_RESIZE_X) {
                mkRect(r.r - s, (r.b + r.t) * 0.5f - s, s * 2, s * 2, 0xffffffff);
                mkRect(r.l - s, (r.b + r.t) * 0.5f - s, s * 2, s * 2, 0xffffffff);
            }
            if (d->flags & OBJ_RESIZE_Y) {
                mkRect((r.l + r.r) * 0.5f - s, r.t - s, s * 2, s * 2, 0xffffffff);
                mkRect((r.l + r.r) * 0.5f - s, r.b - s, s * 2, s * 2, 0xffffffff);
            }
        }
    }
    if (makerEd.drag == DRAG_RECT && !makerEd.brush.object && makerEd.brush.stretch) {
        Rect r = edDragRect(makerEd.dragWorldX, makerEd.dragWorldY, wx, wy);
        MakerTile t = makerEd.brush.tile;
        t.l = r.l;
        t.b = r.b;
        t.r = r.r;
        t.t = r.t;
        edDrawTile(t, 0, 0, 0.6f);
    }
    if (makerEd.drag == DRAG_RECT || makerEd.drag == DRAG_MARQUEE) {
        Rect r = edDragRect(makerEd.dragWorldX, makerEd.dragWorldY, wx, wy);
        if (makerEd.drag == DRAG_RECT && makerEd.brush.object) {
            const MakerBrush &b = makerEd.brush;
            const MakerObjectDef &d = makerObjectDefs[b.def];
            if (!(d.flags & (OBJ_RESIZE_X | OBJ_RESIZE_Y | OBJ_UNIQUE | OBJ_START))) {
                float w = b.w, h = d.klass == MAKER_CLASS_SWITCH ? 32 : b.h;
                int count = 0;
                for (float y = r.b; y + h <= r.t + .5f && count < 4000; y += h)
                    for (float x = r.l; x + w <= r.r + .5f && count < 4000; x += w) {
                        MakerObject o = {};
                        o.klass = d.klass;
                        Vec p = edObjectBrushCenter(x + w * .5f, y + h * .5f);
                        o.x = p.x;
                        o.y = p.y;
                        o.w = b.w;
                        o.h = b.h;
                        o.angle = b.angle;
                        o.setMask = b.setMask;
                        memcpy(o.param, b.param, sizeof(o.param));
                        edDrawObject(o, 0, 0, .55f);
                        count++;
                    }
            }
        }
        if (makerEd.drag == DRAG_MARQUEE)
            r = Rect{minf(makerEd.dragWorldX, wx), minf(makerEd.dragWorldY, wy),
                     maxf(makerEd.dragWorldX, wx), maxf(makerEd.dragWorldY, wy)};
        mkRect(r.l, r.b, r.r - r.l, r.t - r.b, 0xffe07030);
        mkFrame(r.l, r.b, r.r - r.l, r.t - r.b, 0xffe070ff, 1 / makerEd.zoom);
    }
    if (canvas && makerEd.drag == DRAG_NONE) {
        bool erase = makerEd.tool == TOOL_ERASE, alt = mkKeyDown(VK_MENU),
             shift = mkKeyDown(VK_SHIFT);
        // The pen picks up what it is over (same rules as a click) instead of painting there.
        int grab = -1;
        bool start = false;
        if (makerEd.tool == TOOL_PEN && !alt && !shift && !mkKeyDown(VK_CONTROL)) {
            grab = edResizeHandle(makerEd.mouseX, makerEd.mouseY) ? makerEd.propObject
                                                                  : edGrabAt(wx, wy);
            start = grab < 0 && edStartAt(wx, wy);
        }
        if (grab >= 0 || start) {
            Rect r = start ? makerCentered(h.startX, h.startY, 32, 32)
                           : edObjectRect(level.objects[grab]);
            mkFrame(r.l, r.b, r.r - r.l, r.t - r.b, 0xffffffd0, 1.5f / makerEd.zoom);
        } else if (makerEd.tool == TOOL_PEN && !alt) {
            const MakerBrush &b = makerEd.brush;
            if (b.object) {
                MakerObject ghost;
                memset(&ghost, 0, sizeof(ghost));
                ghost.klass = makerObjectDefs[b.def].klass;
                ghost.w = b.w;
                ghost.h = b.h;
                ghost.angle = b.angle;
                ghost.setMask = b.setMask;
                memcpy(ghost.param, b.param, sizeof(ghost.param));
                Vec position = edObjectBrushCenter(wx, wy);
                ghost.x = position.x;
                ghost.y = position.y;
                if (makerObjectDefs[b.def].flags & OBJ_START)
                    edDrawStartBean(makerCentered(ghost.x, ghost.y, 32, 32), 0.55f);
                else
                    edDrawObject(ghost, 0, 0, 0.55f);
                Rect r = edObjectRect(ghost);
                mkFrame(r.l, r.b, r.r - r.l, r.t - r.b, 0xffffff90, 1 / makerEd.zoom);
            } else {
                MakerTile t = edBrushTile();
                float tw = t.r - t.l, th = t.t - t.b;
                t.l = edSnapCoord(wx, tw);
                t.b = edSnapCoord(wy, th);
                t.r = t.l + tw;
                t.t = t.b + th;
                edDrawTile(t, 0, 0, 0.6f);
                int collide = edBrushCollide(makerEd.layer);
                u32 c = collide == COLLIDE_SOLID      ? 0xff4040ff
                        : collide == COLLIDE_ONEWAY   ? 0x40f0ffff
                        : collide >= COLLIDE_SLOPE_BR ? 0xffa030ff
                                                      : 0xffffff90;
                mkFrame(t.l, t.b, tw, th, c, 1 / makerEd.zoom);
                if (collide == COLLIDE_ONEWAY)
                    mkRect(t.l, t.t - t.surface - 1 / makerEd.zoom, tw, 2 / makerEd.zoom, c);
            }
        } else if (erase || makerEd.tool == TOOL_SELECT) {
            int t = -1, o;
            bool startHover = false;
            if (erase)
                o = edEraseTarget(wx, wy, shift, -1, &t);
            else {
                o = edGrabAt(wx, wy);
                startHover = o < 0 && edStartAt(wx, wy);
                if (o < 0 && !startHover)
                    t = edTileAt(wx, wy, makerEd.layer);
            }
            Rect r = {0, 0, 0, 0};
            bool any = true;
            if (o >= 0)
                r = edObjectRect(level.objects[o]);
            else if (startHover)
                r = makerCentered(h.startX, h.startY, 32, 32);
            else if (t >= 0)
                r = Rect{level.tiles[t].l, level.tiles[t].b, level.tiles[t].r, level.tiles[t].t};
            else
                any = false;
            if (any) {
                u32 c = erase ? 0xff4040ff : 0xffffffc0;
                if (erase)
                    mkRect(r.l, r.b, r.r - r.l, r.t - r.b, 0xff404050);
                mkFrame(r.l, r.b, r.r - r.l, r.t - r.b, c, 1.5f / makerEd.zoom);
            }
        }
    }
    glPopMatrix();
    edDrawWeather();
}

// ---------------------------------------------------------------------------------------------
// Panels.

struct MakerToolSpec {
    const char *label;
    int id, arg;
    bool on;
};

// Width of a row of labelled buttons; null labels are group gaps.
float edRowWidth(const MakerToolSpec *specs, int count, float size, float gap) {
    float w = 0;
    for (int i = 0; i < count; i++)
        w += specs[i].label ? mkTextWidth(specs[i].label, size, specs[i].on) + 10 + gap : 5;
    return w;
}

void edToolbar() {
    MakerButtons &bt = makerEd.buttons;
    float h = makerEd.toolbarH, y = viewH - h, x = 4, size = 8.5f;
    mkGradient(0, y, viewW, h, 0x16212dff, 0x223244ff);
    mkRect(0, y, viewW, 1 / screenScale, 0x000000ff);
    typedef MakerToolSpec Spec;
    const Spec left[] = {{"> Play", CMD_PLAY, 0, false},
                         {"Pen", CMD_TOOL, TOOL_PEN, makerEd.tool == TOOL_PEN},
                         {"Erase", CMD_TOOL, TOOL_ERASE, makerEd.tool == TOOL_ERASE},
                         {"Select", CMD_TOOL, TOOL_SELECT, makerEd.tool == TOOL_SELECT},
                         {"Hand", CMD_TOOL, TOOL_HAND, makerEd.tool == TOOL_HAND},
                         {0, 0, 0, false},
                         {"Back", CMD_LAYER, MAKER_BACK, makerEd.layer == MAKER_BACK},
                         {"Main", CMD_LAYER, MAKER_MAIN, makerEd.layer == MAKER_MAIN},
                         {"Front", CMD_LAYER, MAKER_FRONT, makerEd.layer == MAKER_FRONT},
                         {0, 0, 0, false},
                         {"Undo", CMD_UNDO, 0, false},
                         {"Redo", CMD_REDO, 0, false}};
    char gridLabel[16];
    wsprintfA(gridLabel, "Grid %d", makerEd.grid);
    const Spec right[] = {{"Exit", CMD_EXIT, 0, false},
                          {"Save", CMD_SAVE, 0, false},
                          {"Course", CMD_SETTINGS, 0, makerEd.settings},
                          {"?", CMD_HELP, 0, makerEd.help},
                          {"Hitbox", CMD_VIEW_COLLISION, 0, makerEd.showCollision},
                          {gridLabel, CMD_GRID, 0, false}};
    const int leftCount = (int)(sizeof(left) / sizeof(left[0])),
              rightCount = (int)(sizeof(right) / sizeof(right[0]));
    // Narrow windows (1024x768 gives a 512-unit view) shrink the labels before dropping any.
    while (size > 6.5f &&
           edRowWidth(left, leftCount, size, 2) + edRowWidth(right, rightCount, size, 2) + 16 >
               viewW)
        size -= 0.5f;
    for (int i = 0; i < leftCount; i++) {
        if (!left[i].label) {
            x += 5;
            continue;
        }
        float w = mkTextWidth(left[i].label, size, left[i].on) + 10;
        MakerButton *b =
            bt.add(mkBox(x, y + 2, w, h - 4), left[i].id, left[i].label, left[i].arg, left[i].on);
        mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
        x += w + 2;
    }
    float rx = viewW - 4;
    for (int i = 0; i < rightCount; i++) {
        float w = mkTextWidth(right[i].label, size, right[i].on) + 10;
        rx -= w;
        if (rx < x + 40)
            break;
        MakerButton *b = bt.add(mkBox(rx, y + 2, w, h - 4), right[i].id, right[i].label,
                                right[i].arg, right[i].on);
        mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
        rx -= 2;
    }
    // Course name between the two groups when there is room.
    char title[64];
    wsprintfA(title, "C-%02d  %s%s", makerEd.slot + 1, makerEd.level.head.name,
              makerEd.dirty ? " *" : "");
    float tw = mkTextWidth(title, 9, true);
    if (rx - x > tw + 12)
        mkText((x + rx - tw) * 0.5f, y + (h - mkLineHeight(9, true)) * 0.5f, title, 9, 0xfff2b0ff,
               true);
}

void edPaletteBar() {
    MakerButtons &bt = makerEd.buttons;
    float h = makerEd.paletteH, tabH = 15, size = 8;
    mkGradient(0, 0, viewW, h, 0x121b26ff, 0x1d2b3bff);
    mkRect(0, h - 1 / screenScale, viewW, 1 / screenScale, 0x000000ff);
    float tabsWidth = 0;
    for (int t = 0; t < TAB_COUNT; t++)
        tabsWidth += mkTextWidth(makerTabNames[t], size, true) + 9;
    bool worlds = edPieceTab(makerEd.tab);
    while (size > 6.5f && tabsWidth + (worlds ? 150 : 0) > viewW) {
        size -= 0.5f;
        tabsWidth = 0;
        for (int t = 0; t < TAB_COUNT; t++)
            tabsWidth += mkTextWidth(makerTabNames[t], size, true) + 9;
    }
    float x = 3, ty = h - tabH - 1;
    for (int t = 0; t < TAB_COUNT; t++) {
        float w = mkTextWidth(makerTabNames[t], size, makerEd.tab == t) + 8;
        if (t == TAB_ITEMS)
            x += 4;
        MakerButton *b =
            bt.add(mkBox(x, ty, w, tabH), CMD_TAB, makerTabNames[t], t, makerEd.tab == t);
        mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
        x += w + 1;
    }
    if (edPieceTab(makerEd.tab)) {
        const char *worlds[6] = {"All", "W1", "W2", "W3", "W4", "W5"};
        float wx = viewW - 3;
        for (int i = 5; i >= 0; i--) {
            float w = mkTextWidth(worlds[i], size) + 8;
            wx -= w;
            if (wx < x + 4)
                break;
            MakerButton *b =
                bt.add(mkBox(wx, ty, w, tabH), CMD_WORLD, worlds[i], i, makerEd.world == i);
            mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
            wx -= 1;
        }
    }
    edBuildPalette();
    float cell = h - tabH - 6, sx = 16, y0 = 2;
    int visible = (int)((viewW - sx * 2 - 4 - 52) / (cell + 2));
    if (visible < 1)
        visible = 1;
    int maxScroll = makerEd.paletteCount - visible;
    makerEd.scroll = (int)clamp((float)makerEd.scroll, 0, (float)(maxScroll > 0 ? maxScroll : 0));
    MakerButton *lb =
        bt.add(mkBox(2, y0, sx - 2, cell), CMD_SCROLL, "<", -visible, false, makerEd.scroll <= 0);
    mkDrawButton(*lb, makerEd.mouseX, makerEd.mouseY, size);
    int hovered = -1;
    for (int i = 0; i < visible && makerEd.scroll + i < makerEd.paletteCount; i++) {
        int index = makerEd.scroll + i;
        Rect r = mkBox(sx + 2 + i * (cell + 2), y0, cell, cell);
        MakerPaletteItem item = makerEd.palette[index];
        bool current = false;
        if (item.kind == 0)
            current = !makerEd.brush.object && makerEd.brush.piece == item.index;
        else if (item.kind == 1)
            current = makerEd.brush.object && makerEd.brush.def == item.index;
        bool hover = mkInside(r, makerEd.mouseX, makerEd.mouseY);
        if (hover)
            hovered = index;
        mkRect(r.l, r.b, r.r - r.l, r.t - r.b,
               current ? 0x6a5a20ff
               : hover ? 0x34506eff
                       : 0x0d141cff);
        mkFrame(r.l, r.b, r.r - r.l, r.t - r.b, current ? 0xffe070ff : 0x3a4d63ff);
        Rect inner = {r.l + 2, r.b + 2, r.r - 2, r.t - 2};
        if (item.kind == 0)
            edDrawPieceIcon(makerPieces[item.index], inner);
        else if (item.kind == 1)
            edDrawDefIcon(item.index, inner, makerEd.age);
        else
            edDrawBrushIcon(makerEd.recent[item.index], inner, makerEd.age);
        bt.add(r, CMD_ITEM, 0, index);
    }
    if (!makerEd.paletteCount)
        mkText(sx + 6, y0 + cell * 0.5f - 4,
               makerEd.tab == TAB_RECENT ? "Pick items from the other tabs" : "Nothing here yet", 8,
               0x8fa3b8ff);
    float rx = sx + 2 + visible * (cell + 2);
    MakerButton *rb = bt.add(mkBox(rx, y0, sx - 2, cell), CMD_SCROLL, ">", visible, false,
                             makerEd.scroll >= maxScroll);
    mkDrawButton(*rb, makerEd.mouseX, makerEd.mouseY, size);
    MakerButton *all =
        bt.add(mkBox(viewW - 50, y0, 47, cell), CMD_PICKER, "All...", 0, makerEd.picker);
    mkDrawButton(*all, makerEd.mouseX, makerEd.mouseY, size);
    if (hovered >= 0) {
        char label[96];
        MakerPaletteItem item = makerEd.palette[hovered];
        if (item.kind == 0) {
            const MakerPiece &p = makerPieces[item.index];
            if (p.category == PIECE_FILL)
                wsprintfA(label, "Fill  W%d  %s  -  Shift+drag stretches it over an area", p.group,
                          edCollideName(p.collide));
            else
                wsprintfA(label, "%s %dx%d  W%d  %s", makerPieceCategoryNames[p.category], p.w, p.h,
                          p.group, edCollideName(p.collide));
        } else if (item.kind == 1)
            wsprintfA(label, "%s - %s", makerObjectDefs[item.index].name,
                      makerObjectDefs[item.index].hint);
        else
            lstrcpyA(label, "Recent brush");
        float w = mkTextWidth(label, 8) + 10;
        float lx = clamp(makerEd.mouseX - w * 0.5f, 2, viewW - w - 2);
        mkRect(lx, h + 3, w, 14, 0x000000d0);
        mkText(lx + 5, h + 4, label, 8, 0xffffffff);
    }
}

// Status strip on the canvas: cursor, brush part and collision, course budget and warnings.
void edStatus() {
    MakerButtons &bt = makerEd.buttons;
    float y = edCanvasTop() - 13, size = 8;
    // Brush buttons sit on the right; the status text shortens when they would overlap it.
    bool brush = makerEd.tool == TOOL_PEN && !makerEd.brush.object;
    const char *labels[4] = {"Flip V", "Flip H", "Rotate", 0};
    const int ids[4] = {CMD_BRUSH_FLIP_Y, CMD_BRUSH_FLIP_X, CMD_BRUSH_ROTATE, CMD_BRUSH_TRIM};
    char part[32], coll[48];
    wsprintfA(part, "Part: %s", makerTrimNames[makerEd.brush.trim % MAKER_TRIMS]);
    labels[3] = part;
    wsprintfA(coll, "Hitbox: %s%s", edCollideName(edBrushCollide(makerEd.layer)),
              makerEd.brush.collideMode < 0 ? " (auto)" : "");
    float right = viewW - 4 - (makerEd.propObject >= 0 ? makerEd.propsW : 0), buttonsW = 0;
    if (brush) {
        for (int i = 0; i < 4; i++)
            buttonsW += mkTextWidth(labels[i], size) + 10;
        buttonsW += mkTextWidth(coll, size) + 8;
    }
    char text[160];
    float wx = edWorldX(makerEd.mouseX), wy = edWorldY(makerEd.mouseY);
    const char *layers[3] = {"Back", "Main", "Front"};
    wsprintfA(text, "Zoom %d%%   x %d  y %d   Layer %s   Tiles %d   Objects %d/%d",
              makerRound(makerEd.zoom * 100), makerFloor(wx / 32), makerFloor(wy / 32),
              layers[makerEd.layer], makerEd.level.tileCount, makerEd.entityCost,
              MAX_ENTITIES - 160);
    if (mkTextWidth(text, size) + 16 > right - buttonsW)
        wsprintfA(text, "%d%%  x %d y %d  %s", makerRound(makerEd.zoom * 100), makerFloor(wx / 32),
                  makerFloor(wy / 32), layers[makerEd.layer]);
    mkRect(0, y - 1, mkTextWidth(text, size) + 10, 13, 0x00000090);
    mkText(5, y, text, size, 0xd8e4f0ff);
    // Problems that would stop a clear check, listed under the status line.
    const MakerLevel &level = makerEd.level;
    int goals = 0;
    for (int i = 0; i < level.objectCount; i++)
        goals += level.objects[i].klass == MAKER_CLASS_GOAL;
    const char *warnings[3];
    int warningCount = 0;
    if (!goals)
        warnings[warningCount++] = "No Goal Spring yet - the course cannot be finished";
    if (makerSpawnBlocked(level, Vec{level.head.startX, level.head.startY}))
        warnings[warningCount++] = "Bean's start overlaps solid bricks";
    if (makerEd.entityCost > MAX_ENTITIES - 160)
        warnings[warningCount++] = "Too many objects - the last ones will not appear";
    for (int i = 0; i < warningCount; i++) {
        float wy = y - 14 * (i + 1);
        mkRect(0, wy - 1, mkTextWidth(warnings[i], size, true) + 10, 13, 0x500000c0);
        mkText(5, wy, warnings[i], size, 0xff9080ff, true);
    }
    if (!brush)
        return;
    float x = right;
    for (int i = 0; i < 4; i++) {
        float w = mkTextWidth(labels[i], size) + 8;
        x -= w;
        MakerButton *b = bt.add(mkBox(x, y - 1, w, 13), ids[i], labels[i], 0,
                                ids[i] == CMD_BRUSH_TRIM && makerEd.brush.trim > 0,
                                ids[i] == CMD_BRUSH_TRIM && !edBrushTrimmable(makerEd.brush));
        mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
        x -= 2;
    }
    float w = mkTextWidth(coll, size) + 8;
    x -= w;
    MakerButton *b = bt.add(mkBox(x, y - 1, w, 13), CMD_BRUSH_COLLIDE, coll);
    mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
}

void edPropertyPanel() {
    MakerLevel &l = makerEd.level;
    if (makerEd.propObject < 0 || makerEd.propObject >= l.objectCount) {
        makerEd.propObject = -1;
        return;
    }
    MakerButtons &bt = makerEd.buttons;
    MakerObject o = makerEd.drag == DRAG_RESIZE
                        ? edResizeDragObject(edWorldX(makerEd.mouseX), edWorldY(makerEd.mouseY))
                        : l.objects[makerEd.propObject];
    const MakerObjectDef *d = makerDefFor(o.klass);
    if (!d) {
        makerEd.propObject = -1;
        return;
    }
    float w = makerEd.propsW, x = viewW - w, top = edCanvasTop() - 2, bottom = edCanvasBottom() + 2;
    mkPanel(x, bottom, w - 2, top - bottom);
    bt.add(mkBox(x, bottom, w - 2, top - bottom), CMD_BACKDROP, 0, 1);
    float y = top - 16, size = 8.5f;
    mkText(x + 6, y, d->name, 10, 0xfff2b0ff, true);
    MakerButton *close = bt.add(mkBox(x + w - 20, y - 1, 15, 14), CMD_PROP_CLOSE, "x");
    mkDrawButton(*close, makerEd.mouseX, makerEd.mouseY, size);
    y -= 12;
    int hintLines = mkTextWrapped(x + 6, y, d->hint, 7.5f, 0x9fb4c8ff, w - 14, 10, 2);
    y -= 8 + hintLines * 10;
    // One row per value: label on the left, "< value >" on the right; the wheel works over the row.
    // Only settings the object's class supports are listed, and rows tighten to fit them all.
    float minusX = x + w - 114, plusX = x + w - 24, valueX = (minusX + 16 + plusX) * 0.5f,
          labelW = w - 120;
    int rows = (d->flags & OBJ_RESIZE_X ? 1 : 0) + (d->flags & OBJ_RESIZE_Y ? 1 : 0) +
               (d->flags & OBJ_ROTATE ? 1 : 0);
    for (int i = 0; i < d->paramCount; i++)
        rows += makerParamUsable(*d, i);
    float rowH = rows ? clamp((y - (bottom + 26)) / rows, 13, 17) : 17;
    char value[48], shown[48], label[40];
    if (d->flags & (OBJ_RESIZE_X | OBJ_RESIZE_Y)) {
        float cw = d->cellW > 0 ? d->cellW : 16, ch = d->cellH > 0 ? d->cellH : 16;
        for (int axis = 0; axis < 2; axis++) {
            if (!(d->flags & (axis ? OBJ_RESIZE_Y : OBJ_RESIZE_X)))
                continue;
            bt.add(mkBox(x + 2, y - 3, w - 6, rowH), CMD_WHEEL, 0, 2000 + axis);
            mkText(x + 6, y, axis ? "Height" : "Width", size, 0xd8e4f0ff);
            wsprintfA(value, "%d", makerRound((axis ? o.h : o.w) / (axis ? ch : cw)));
            MakerButton *minus = bt.add(mkBox(minusX, y - 2, 16, 14), CMD_PROP_SIZE, "-", axis * 2);
            mkDrawButton(*minus, makerEd.mouseX, makerEd.mouseY, size);
            mkTextCentered(valueX, y + 5, value, 8, 0xffffffff, true);
            MakerButton *plus =
                bt.add(mkBox(plusX, y - 2, 16, 14), CMD_PROP_SIZE, "+", axis * 2 + 1);
            mkDrawButton(*plus, makerEd.mouseX, makerEd.mouseY, size);
            y -= rowH;
        }
    }
    if (d->flags & OBJ_ROTATE) {
        mkText(x + 6, y, "Rotation", size, 0xd8e4f0ff);
        wsprintfA(value, "%d deg", makerRound(o.angle));
        mkTextCentered(valueX, y + 5, value, 8, 0xffffffff, true);
        MakerButton *rotate = bt.add(mkBox(plusX - 12, y - 2, 28, 14), CMD_PROP_ROTATE, "Turn");
        mkDrawButton(*rotate, makerEd.mouseX, makerEd.mouseY, 7.5f);
        y -= rowH;
    }
    for (int i = 0; i < d->paramCount && y > bottom + 24; i++) {
        if (!makerParamUsable(*d, i))
            continue;
        const MakerParamDef &p = d->params[i];
        float v = makerParam(o, *d, i);
        if (p.kind == PARAM_NUMBER) {
            int decimals = p.step >= 1 ? 0 : p.step >= 0.1f ? 1 : 2;
            mkFormat(value, v, decimals);
            if (p.write == WRITE_CAP && v <= 0)
                lstrcpyA(value, "none");
            else if (p.unit) {
                lstrcatA(value, " ");
                lstrcatA(value, p.unit);
            }
        } else if (!makerChoiceLabel(p.choices, makerRound(v), value, sizeof(value)))
            mkFormat(value, v, 0);
        bt.add(mkBox(x + 2, y - 3, w - 6, rowH), CMD_WHEEL, 0, 1000 + i);
        mkFitText(label, sizeof(label), p.label, labelW, size, false);
        mkText(x + 6, y, label, size, (o.setMask & (1u << i)) ? 0xffffffff : 0xb8c8d8ff);
        MakerButton *minus = bt.add(mkBox(minusX, y - 2, 16, 14), CMD_PROP_DEC, "<", i);
        mkDrawButton(*minus, makerEd.mouseX, makerEd.mouseY, size);
        mkFitText(shown, sizeof(shown), value, plusX - minusX - 20, 8, true);
        mkTextCentered(valueX, y + 5, shown, 8, 0xfff2b0ff, true);
        MakerButton *plus = bt.add(mkBox(plusX, y - 2, 16, 14), CMD_PROP_INC, ">", i);
        mkDrawButton(*plus, makerEd.mouseX, makerEd.mouseY, size);
        y -= rowH;
    }
    MakerButton *del = bt.add(mkBox(x + 6, bottom + 6, w - 16, 16), CMD_PROP_DELETE, "Delete");
    mkDrawButton(*del, makerEd.mouseX, makerEd.mouseY, size);
}

void edSettingsPanel() {
    MakerButtons &bt = makerEd.buttons;
    MakerHeader &h = makerEd.level.head;
    bt.add(mkBox(0, 0, viewW, viewH), CMD_BACKDROP, 0, 0);
    mkRect(0, 0, viewW, viewH, 0x00000080);
    const int rowCount = 16;
    float w = minf(360, viewW - 20), rowH = minf(19, (viewH - 70) / (rowCount + 1));
    float ph = rowH * (rowCount + 1) + 30, x = (viewW - w) * 0.5f, y = (viewH - ph) * 0.5f;
    mkPanel(x, y, w, ph);
    bt.add(mkBox(x, y, w, ph), CMD_BACKDROP, 0, 1);
    float size = 9, row = y + ph - 22;
    mkTextCentered(x + w * 0.5f, row + 8, "Course Settings", 11, 0xfff2b0ff, true);
    row -= rowH + 2;
    char value[80];

    struct Row {
        const char *label;
        int id;
    };

    const Row rows[] = {{"Name", CMD_SET_NAME},
                        {"Theme", CMD_SET_THEME},
                        {"Background", CMD_SET_BACKGROUND},
                        {"Music", CMD_SET_MUSIC},
                        {"Weather", CMD_SET_EFFECT},
                        {"Weather amount", CMD_SET_WEATHER_AMOUNT},
                        {"Length", CMD_SET_WIDTH},
                        {"Height", CMD_SET_HEIGHT},
                        {"Gold shoe", CMD_SET_BEST},
                        {"Par jumps", CMD_SET_PAR},
                        {"Gems needed", CMD_SET_GEMS},
                        {"Bean speed", CMD_SET_BEAN_SPEED},
                        {"Bean jump", CMD_SET_BEAN_JUMP},
                        {"Camera", CMD_SET_CAMERA},
                        {"Camera ahead", CMD_SET_AHEAD_X},
                        {"Camera up", CMD_SET_AHEAD_Y}};
    for (int i = 0; i < rowCount; i++) {
        mkText(x + 10, row + 3, rows[i].label, size, 0xd8e4f0ff);
        float vx = x + 100, vw = w - 110;
        switch (rows[i].id) {
        case CMD_SET_NAME:
            wsprintfA(value, "%s%s", h.name,
                      makerEd.nameEditing && ((int)(makerEd.age * 2) & 1) ? "_" : "");
            break;
        case CMD_SET_THEME:
            lstrcpyA(value, makerThemeNames[(int)clamp((float)h.theme, 1, 5)]);
            break;
        case CMD_SET_BACKGROUND:
            lstrcpyA(value, makerBackgrounds[(int)clamp((float)h.background, 0,
                                                        (float)makerBackgroundCount - 1)]
                                .name);
            break;
        case CMD_SET_MUSIC:
            makerMusicName(h.music, value, sizeof(value));
            break;
        case CMD_SET_EFFECT:
            lstrcpyA(
                value,
                makerEffects[(int)clamp((float)h.effect, 0, (float)makerEffectCount - 1)].name);
            break;
        case CMD_SET_WIDTH:
            wsprintfA(value, "%d tiles", h.width / 32);
            break;
        case CMD_SET_HEIGHT:
            wsprintfA(value, "%d tiles", h.height / 32);
            break;
        case CMD_SET_BEST:
            wsprintfA(value, "%d jumps or fewer", h.jumpBest);
            break;
        case CMD_SET_PAR:
            wsprintfA(value, "%d jumps or fewer", h.jumpPar);
            break;
        case CMD_SET_GEMS:
            if (h.coinGoal > 0 && h.coinGoal < h.coins)
                wsprintfA(value, "%d of %d", h.coinGoal, h.coins);
            else
                wsprintfA(value, "All (%d)", h.coins);
            break;
        case CMD_SET_BEAN_SPEED:
        case CMD_SET_BEAN_JUMP: {
            int percent = rows[i].id == CMD_SET_BEAN_SPEED ? h.beanSpeed : h.beanJump;
            wsprintfA(value, "%d%%%s", percent ? percent : 100,
                      percent && percent != 100 ? "" : " (normal)");
            break;
        }
        case CMD_SET_WEATHER_AMOUNT:
            wsprintfA(value, "%d%%", h.weatherAmount ? h.weatherAmount : 100);
            break;
        case CMD_SET_CAMERA: {
            const char *modes[3] = {"Follows Bean", "Horizontal only", "Vertical only"};
            lstrcpyA(value, modes[(int)clamp((float)h.camera, 0, 2)]);
            break;
        }
        case CMD_SET_AHEAD_X:
        case CMD_SET_AHEAD_Y: {
            int ahead = rows[i].id == CMD_SET_AHEAD_X ? h.cameraAheadX : h.cameraAheadY;
            wsprintfA(value, ahead ? "%d%% of the view" : "Centred", ahead);
            break;
        }
        }
        if (rows[i].id == CMD_SET_NAME) {
            MakerButton *b =
                bt.add(mkBox(vx, row, vw, rowH - 3), CMD_SET_NAME, 0, 0, makerEd.nameEditing);
            mkRect(vx, row, vw, rowH - 3, makerEd.nameEditing ? 0x0b2a46ff : 0x0d141cff);
            mkFrame(vx, row, vw, rowH - 3, makerEd.nameEditing ? 0xffe070ff : 0x3a4d63ff);
            mkText(vx + 5, row + 2, value, size, 0xffffffff);
            (void)b;
        } else {
            bt.add(mkBox(x + 4, row - 1, w - 8, rowH - 1), CMD_WHEEL, 0, rows[i].id);
            MakerButton *minus = bt.add(mkBox(vx, row, 18, rowH - 3), rows[i].id, "<", -1);
            mkDrawButton(*minus, makerEd.mouseX, makerEd.mouseY, size);
            mkTextCentered(vx + vw * 0.5f, row + (rowH - 3) * 0.5f, value, size, 0xfff2b0ff, true);
            MakerButton *plus = bt.add(mkBox(vx + vw - 18, row, 18, rowH - 3), rows[i].id, ">", 1);
            mkDrawButton(*plus, makerEd.mouseX, makerEd.mouseY, size);
        }
        row -= rowH;
    }
    MakerButton *done = bt.add(mkBox(x + w * 0.5f - 40, y + 6, 80, 17), CMD_SET_DONE, "Done");
    mkDrawButton(*done, makerEd.mouseX, makerEd.mouseY, size);
}

void edPicker() {
    MakerButtons &bt = makerEd.buttons;
    bt.add(mkBox(0, 0, viewW, viewH), CMD_BACKDROP, 0, 0);
    mkRect(0, 0, viewW, viewH, 0x00000090);
    float margin = 12, x = margin, y = margin, w = viewW - margin * 2, h = viewH - margin * 2;
    mkPanel(x, y, w, h);
    bt.add(mkBox(x, y, w, h), CMD_BACKDROP, 0, 1);
    float size = 8.5f, tabH = 16, tx = x + 6, ty = y + h - tabH - 6;
    for (int t = 0; t < TAB_COUNT; t++) {
        float tw = mkTextWidth(makerTabNames[t], size, makerEd.tab == t) + 10;
        MakerButton *b =
            bt.add(mkBox(tx, ty, tw, tabH), CMD_TAB, makerTabNames[t], t, makerEd.tab == t);
        mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
        tx += tw + 2;
    }
    if (edPieceTab(makerEd.tab)) {
        const char *worlds[6] = {"All worlds", "Grasslands", "Crystal",
                                 "Sky Ruins",  "Desert",     "Wizard"};
        float wx = x + 6, wy = ty - tabH - 4;
        for (int i = 0; i < 6; i++) {
            float ww = mkTextWidth(worlds[i], size) + 10;
            MakerButton *b =
                bt.add(mkBox(wx, wy, ww, tabH), CMD_WORLD, worlds[i], i, makerEd.world == i);
            mkDrawButton(*b, makerEd.mouseX, makerEd.mouseY, size);
            wx += ww + 2;
        }
        ty = wy;
    }
    edBuildPalette();
    float cell = 44, gx = x + 8, gy = ty - 6;
    int columns = (int)((w - 16) / (cell + 3));
    int rows = (int)((gy - (y + 8)) / (cell + 3));
    if (columns < 1)
        columns = 1;
    if (rows < 1)
        rows = 1;
    int totalRows = (makerEd.paletteCount + columns - 1) / columns;
    makerEd.pickerScroll = (int)clamp((float)makerEd.pickerScroll, 0,
                                      (float)(totalRows - rows > 0 ? totalRows - rows : 0));
    int hovered = -1;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < columns; c++) {
            int index = (makerEd.pickerScroll + r) * columns + c;
            if (index >= makerEd.paletteCount)
                break;
            Rect box = mkBox(gx + c * (cell + 3), gy - (r + 1) * (cell + 3), cell, cell);
            bool hover = mkInside(box, makerEd.mouseX, makerEd.mouseY);
            if (hover)
                hovered = index;
            mkRect(box.l, box.b, cell, cell, hover ? 0x34506eff : 0x0d141cff);
            mkFrame(box.l, box.b, cell, cell, 0x3a4d63ff);
            Rect inner = {box.l + 3, box.b + 3, box.r - 3, box.t - 3};
            MakerPaletteItem item = makerEd.palette[index];
            if (item.kind == 0)
                edDrawPieceIcon(makerPieces[item.index], inner);
            else if (item.kind == 1)
                edDrawDefIcon(item.index, inner, makerEd.age);
            else
                edDrawBrushIcon(makerEd.recent[item.index], inner, makerEd.age);
            bt.add(box, CMD_ITEM, 0, index);
        }
    if (totalRows > rows) {
        char text[32];
        wsprintfA(text, "Rows %d-%d of %d (wheel)", makerEd.pickerScroll + 1,
                  makerEd.pickerScroll + rows, totalRows);
        mkText(x + w - mkTextWidth(text, 8) - 8, y + 4, text, 8, 0x8fa3b8ff);
    }
    if (hovered >= 0) {
        char label[96];
        MakerPaletteItem item = makerEd.palette[hovered];
        if (item.kind == 0) {
            const MakerPiece &p = makerPieces[item.index];
            if (p.category == PIECE_FILL)
                wsprintfA(label, "Fill  %s  %s  -  Shift+drag stretches it over an area",
                          makerThemeNames[p.group], edCollideName(p.collide));
            else
                wsprintfA(label, "%dx%d  %s  %s  used %d times", p.w, p.h, makerThemeNames[p.group],
                          edCollideName(p.collide), p.uses);
        } else if (item.kind == 1)
            wsprintfA(label, "%s - %s", makerObjectDefs[item.index].name,
                      makerObjectDefs[item.index].hint);
        else
            lstrcpyA(label, "Recent brush");
        mkText(x + 8, y + 4, label, 8.5f, 0xffffffff);
    }
}

void edHelp() {
    MakerButtons &bt = makerEd.buttons;
    bt.add(mkBox(0, 0, viewW, viewH), CMD_HELP, 0);
    mkRect(0, 0, viewW, viewH, 0x000000b0);
    const char *lines[] = {
        "Left drag: paint / place      Right drag: erase      Middle drag or Space: pan",
        "Shift + drag: fill a rectangle; a Fill stretches over it (sizes gems, water...)",
        "Drag items to move them; move areas (water, wind...) by their frame or name tag",
        "Alt + click or Q: pick the brick or item under the cursor",
        "Double-click an item: edit it (speed, interval, range...); the wheel steps values",
        "Wheel: zoom   Shift + wheel: scroll   Arrows: pan   Home: whole course",
        "1 / 2 / 3: back, main, front layer   L: dim other layers",
        "P pen  E eraser  S select  H hand   G: grid 32/16/8/4/2/1   K: hitbox view",
        "R / Shift+R: rotate   F: flip   V: flip vertically   C: brick hitbox",
        "T / Shift+T: paint only a half or a quarter of the brick",
        "Ctrl+Z / Ctrl+Y: undo / redo   Ctrl+C X V D: copy cut paste duplicate",
        "Delete: remove selection   Ctrl+A: select the layer   Tab: all items",
        "Enter / F5: test from start   Shift+Enter: test from the cursor",
        "Ctrl+S: save   F2: course settings   M: music   Esc: back to the course list",
        "While testing: F5 or Tab returns to the editor"};
    int n = (int)(sizeof(lines) / sizeof(lines[0]));
    float size = 8.5f, lh = 13, w = 0;
    for (;;) {
        w = 0;
        for (int i = 0; i < n; i++)
            w = maxf(w, mkTextWidth(lines[i], size));
        if (w <= viewW - 30 || size <= 6.5f)
            break;
        size -= 0.5f;
    }
    float x = (viewW - w) * 0.5f, y = viewH * 0.5f + n * lh * 0.5f;
    mkPanel(x - 10, y - n * lh - 30, w + 20, n * lh + 46);
    mkTextCentered(viewW * 0.5f, y + 4, "Course Maker Controls", 11, 0xfff2b0ff, true);
    for (int i = 0; i < n; i++)
        mkText(x, y - (i + 1) * lh - 6, lines[i], size, 0xe8f0f8ff);
    mkTextCentered(viewW * 0.5f, y - n * lh - 22, "Click or press any key to close", 8, 0x8fa3b8ff);
}

// Shortcut hints shown after hovering a toolbar or brush button for a moment.
const char *edTooltipText(int id, int arg) {
    switch (id) {
    case CMD_PLAY:
        return "Test from the start (Enter)  -  Shift+Enter tests from the cursor";
    case CMD_TOOL:
        return arg == TOOL_PEN      ? "Pen (P): paint bricks, place items, drag items to move them"
               : arg == TOOL_ERASE  ? "Eraser (E): the right mouse button erases with any tool"
               : arg == TOOL_SELECT ? "Select (S): box-select, then drag, copy (Ctrl+C) or delete"
                                    : "Hand (H): pan the view; middle drag or Space+drag also pan";
    case CMD_LAYER:
        return arg == MAKER_BACK   ? "Back layer (1): scenery behind Bean, no collision by default"
               : arg == MAKER_MAIN ? "Main layer (2): ground, walls and platforms Bean touches"
                                   : "Front layer (3): scenery drawn in front of Bean";
    case CMD_UNDO:
        return "Undo (Ctrl+Z)";
    case CMD_REDO:
        return "Redo (Ctrl+Y or Ctrl+Shift+Z)";
    case CMD_GRID:
        return "Snap grid (G): 32, 16, 8, 4, 2 or 1 pixel; picking a brush selects its nearest grid";
    case CMD_VIEW_COLLISION:
        return "Show collision outlines (K)";
    case CMD_SETTINGS:
        return "Course settings (F2): name, background, music, size and awards";
    case CMD_HELP:
        return "All controls (F1)";
    case CMD_SAVE:
        return "Save (Ctrl+S)";
    case CMD_EXIT:
        return "Save and return to the course list (Esc)";
    case CMD_BRUSH_ROTATE:
        return "Rotate the brick (R; Shift+R turns the other way)";
    case CMD_BRUSH_FLIP_X:
        return "Mirror the brick (F)";
    case CMD_BRUSH_FLIP_Y:
        return "Flip the brick upside down (V)";
    case CMD_BRUSH_COLLIDE:
        return "Brick collision (C): auto, solid, platform, slopes or none";
    case CMD_BRUSH_TRIM:
        return "Paint a half or a quarter of the brick (T; Shift+T goes back)";
    case CMD_PICKER:
        return "Browse every brick and item (Tab)";
    case CMD_WORLD:
        return "Show bricks from one world; bricks of every world mix freely";
    case CMD_TAB:
        return arg == TAB_FILLS ? "Fills: one brick stretched over any area with Shift+drag"
                                : "Page Up / Page Down switch tabs";
    }
    return 0;
}

void edTooltip() {
    MakerButton *b = makerEd.picker || makerEd.settings || makerEd.help
                         ? 0
                         : makerEd.buttons.hit(makerEd.mouseX, makerEd.mouseY);
    int id = b ? b->id : 0, arg = b ? b->arg : 0;
    if (id != makerEd.hoverId || arg != makerEd.hoverArg) {
        makerEd.hoverId = id;
        makerEd.hoverArg = arg;
        makerEd.hoverTime = 0;
    }
    const char *text = b ? edTooltipText(id, arg) : 0;
    if (!text || makerEd.hoverTime < 0.5f)
        return;
    float size = 8, w = mkTextWidth(text, size) + 10, h = 14;
    float x = clamp(makerEd.mouseX - w * 0.5f, 2, viewW - w - 2), y = b->r.b - h - 3;
    if (y < 2)
        y = b->r.t + 3;
    mkRect(x, y, w, h, 0x000000e0);
    mkFrame(x, y, w, h, 0x6c8db5ff);
    mkText(x + 5, y + (h - mkLineHeight(size)) * 0.5f, text, size, 0xffffffff);
}

void edRender() {
    makerEd.buttons.clear();
    makerEd.toolbarH = 22;
    makerEd.paletteH = viewH < 300 ? 52 : 60;
    makerEd.propsW = minf(196, viewW * 0.38f);
    if (makerEd.loading || !makerCatalogReady) {
        mkGradient(0, 0, viewW, viewH, 0x0d1520ff, 0x24384fff);
        mkTextCentered(viewW * 0.5f, viewH * 0.5f, "Loading the course maker...", 12, 0xffffffff,
                       true);
        return;
    }
    edDrawCanvas();
    edStatus();
    edPropertyPanel();
    edPaletteBar();
    edToolbar();
    if (makerEd.picker)
        edPicker();
    if (makerEd.settings)
        edSettingsPanel();
    if (makerEd.help)
        edHelp();
    edTooltip();
    mkDrawToast();
}

// ---------------------------------------------------------------------------------------------
// Commands and input.

void edClampCamera() {
    float vw = viewW / makerEd.zoom, vh = (edCanvasTop() - edCanvasBottom()) / makerEd.zoom;
    const MakerHeader &h = makerEd.level.head;
    makerEd.camX = clamp(makerEd.camX, -vw * 0.5f - 64, h.width - vw * 0.5f + 64);
    makerEd.camY = clamp(makerEd.camY, -vh * 0.5f - 64, h.height - vh * 0.5f + 64);
}

void edZoomAt(float sx, float sy, int direction) {
    int at = 6;
    for (int i = 0; i < makerZoomCount; i++)
        if (makerZoomSteps[i] == makerEd.zoom)
            at = i;
    at = (int)clamp((float)(at + direction), 0, (float)(makerZoomCount - 1));
    float wx = edWorldX(sx), wy = edWorldY(sy);
    makerEd.zoom = makerZoomSteps[at];
    makerEd.camX = wx - sx / makerEd.zoom;
    makerEd.camY = wy - (sy - edCanvasBottom()) / makerEd.zoom;
    edClampCamera();
}

void edFitCourse() {
    const MakerHeader &h = makerEd.level.head;
    // Leave a margin so the course edges and their resize grips stay clickable.
    float vw = viewW, vh = edCanvasTop() - edCanvasBottom();
    int best = 0;
    for (int i = 0; i < makerZoomCount; i++)
        if (h.width * makerZoomSteps[i] <= vw - 32 && h.height * makerZoomSteps[i] <= vh - 32)
            best = i;
    makerEd.zoom = makerZoomSteps[best];
    makerEd.camX = h.width * 0.5f - vw / makerEd.zoom * 0.5f;
    makerEd.camY = h.height * 0.5f - vh / makerEd.zoom * 0.5f;
    edClampCamera();
}

void edAdjustParam(int index, int direction) {
    MakerLevel &l = makerEd.level;
    if (makerEd.propObject < 0 || makerEd.propObject >= l.objectCount)
        return;
    MakerObject o = l.objects[makerEd.propObject];
    const MakerObjectDef *d = makerDefFor(o.klass);
    if (!d || !makerParamUsable(*d, index))
        return;
    const MakerParamDef &p = d->params[index];
    float v = makerParam(o, *d, index);
    float step = p.step * (mkKeyDown(VK_SHIFT) ? 5 : 1);
    // Numbers land on the step grid, so derived defaults step to tidy values.
    if (p.kind == PARAM_NUMBER)
        v = clamp(makerRound((v + step * direction) / p.step) * p.step, p.min, p.max);
    else {
        int count = (int)(p.max - p.min + 1.5f);
        v = (float)(((makerRound(v - p.min) + direction) % count + count) % count) + p.min;
    }
    o.param[index] = v;
    o.setMask |= 1u << index;
    edBeginGroup();
    edSetObject(makerEd.propObject, o);
    makerEd.entityCost = edEntityCost();
    sound("switch", 1 + direction * 0.1f, 0.35f);
}

void edResizeProp(int which) {
    MakerLevel &l = makerEd.level;
    if (makerEd.propObject < 0)
        return;
    MakerObject o = l.objects[makerEd.propObject];
    const MakerObjectDef *d = makerDefFor(o.klass);
    if (!d)
        return;
    float cw = d->cellW > 0 ? d->cellW : 16, ch = d->cellH > 0 ? d->cellH : 16;
    if (d->flags & OBJ_FREE)
        cw = ch = (float)maxf(16, (float)makerEd.grid);
    float l0 = o.x - o.w * 0.5f, b0 = o.y - o.h * 0.5f;
    if (which / 2 == 0)
        o.w = clamp(o.w + (which % 2 ? cw : -cw), cw, 4096);
    else
        o.h = clamp(o.h + (which % 2 ? ch : -ch), ch, 4096);
    o.x = l0 + o.w * 0.5f;
    o.y = b0 + o.h * 0.5f;
    edBeginGroup();
    edSetObject(makerEd.propObject, o);
    makerEd.entityCost = edEntityCost();
}

void edSelectSingleObject(int index) {
    edClearSelection();
    if (index >= 0) {
        makerEd.objectSel[index] = 1;
        makerEd.selObjects = 1;
    }
    makerEd.propObject = index;
}

void edCycleSetting(int id, int direction) {
    MakerHeader &h = makerEd.level.head;
    bool shift = mkKeyDown(VK_SHIFT);
    switch (id) {
    case CMD_SET_THEME:
        h.theme = (h.theme - 1 + direction + 5) % 5 + 1;
        h.background = makerThemeBackground(h.theme);
        makerEd.backgroundDirty = true;
        if (h.music == MAKER_MUSIC_THEME)
            playMusic(makerCourseMusic(h));
        break;
    case CMD_SET_BACKGROUND:
        h.background = (h.background + direction + makerBackgroundCount) % makerBackgroundCount;
        makerEd.backgroundDirty = true;
        break;
    case CMD_SET_MUSIC:
        h.music = makerNextMusic(h.music, direction);
        playMusic(makerCourseMusic(h));
        break;
    case CMD_SET_EFFECT:
        h.effect = (h.effect + direction + makerEffectCount) % makerEffectCount;
        makerEd.backgroundDirty = true;
        break;
    case CMD_SET_WIDTH:
        edBeginGroup();
        edSetSize(h.width + direction * (shift ? 1024 : 128), h.height);
        break;
    case CMD_SET_HEIGHT:
        edBeginGroup();
        edSetSize(h.width, h.height + direction * (shift ? 320 : 32));
        break;
    case CMD_SET_BEST:
        h.jumpBest = (int)clamp((float)(h.jumpBest + direction * (shift ? 5 : 1)), 1, 999);
        if (h.jumpPar < h.jumpBest)
            h.jumpPar = h.jumpBest;
        break;
    case CMD_SET_PAR:
        h.jumpPar = (int)clamp((float)(h.jumpPar + direction * (shift ? 5 : 1)), 1, 999);
        if (h.jumpBest > h.jumpPar)
            h.jumpBest = h.jumpPar;
        break;
    case CMD_SET_GEMS: {
        int goal = h.coinGoal > 0 && h.coinGoal < h.coins ? h.coinGoal : h.coins;
        goal += direction * (shift ? 5 : 1);
        h.coinGoal = goal >= h.coins || goal <= 0 ? 0 : goal;
        break;
    }
    case CMD_SET_BEAN_SPEED:
    case CMD_SET_BEAN_JUMP: {
        // Bean's movement decides whether the course can be cleared, so a change needs a new
        // clear check.
        int &percent = id == CMD_SET_BEAN_SPEED ? h.beanSpeed : h.beanJump;
        int next =
            (int)clamp((float)((percent ? percent : 100) + direction * (shift ? 50 : 10)), 50, 200);
        percent = next == 100 ? 0 : next;
        edTouched();
        break;
    }
    case CMD_SET_WEATHER_AMOUNT: {
        int next = (int)clamp(
            (float)((h.weatherAmount ? h.weatherAmount : 100) + direction * (shift ? 100 : 25)), 25,
            400);
        h.weatherAmount = next == 100 ? 0 : next;
        makerEd.backgroundDirty = true;
        break;
    }
    case CMD_SET_CAMERA:
        h.camera = (h.camera + direction + 3) % 3;
        break;
    case CMD_SET_AHEAD_X:
    case CMD_SET_AHEAD_Y: {
        int &ahead = id == CMD_SET_AHEAD_X ? h.cameraAheadX : h.cameraAheadY;
        ahead = (int)clamp((float)(ahead + direction * 5), -25, 25);
        break;
    }
    }
    makerEd.dirty = true;
    sound("switch", 1, 0.35f);
}

void edCommand(int id, int arg) {
    switch (id) {
    case CMD_BACKDROP:
        if (arg == 0) {
            makerEd.picker = makerEd.settings = makerEd.help = makerEd.nameEditing = false;
        }
        break;
    case CMD_PLAY:
        makerStartTest(false);
        break;
    case CMD_TOOL:
        makerEd.tool = arg;
        if (arg != TOOL_SELECT)
            edClearSelection();
        break;
    case CMD_LAYER:
        makerEd.layer = arg;
        break;
    case CMD_UNDO:
        edUndo();
        break;
    case CMD_REDO:
        edRedo();
        break;
    case CMD_GRID:
        for (int i = 0; i < makerGridStepCount; i++)
            if (makerEd.grid == makerGridSteps[i]) {
                makerEd.grid = makerGridSteps[(i + 1) % makerGridStepCount];
                break;
            }
        break;
    case CMD_VIEW_COLLISION:
        makerEd.showCollision = !makerEd.showCollision;
        break;
    case CMD_SETTINGS:
        makerPrepareHeader(makerEd.level);
        makerEd.settings = !makerEd.settings;
        makerEd.nameEditing = false;
        break;
    case CMD_SAVE:
        edSaveCourse();
        break;
    case CMD_EXIT:
        if (makerEd.dirty)
            edSaveCourse();
        makerOpenBrowser();
        break;
    case CMD_HELP:
        makerEd.help = !makerEd.help;
        break;
    case CMD_TAB:
        makerEd.tab = arg;
        makerEd.scroll = makerEd.pickerScroll = 0;
        break;
    case CMD_WORLD:
        makerEd.world = arg;
        makerEd.scroll = makerEd.pickerScroll = 0;
        break;
    case CMD_ITEM:
        edPickPalette(arg);
        makerEd.picker = false;
        break;
    case CMD_SCROLL:
        makerEd.scroll += arg;
        break;
    case CMD_PICKER:
        makerEd.picker = !makerEd.picker;
        break;
    case CMD_BRUSH_COLLIDE:
        edCycleBrushCollide();
        break;
    case CMD_BRUSH_ROTATE:
        edRotateBrush(1);
        break;
    case CMD_BRUSH_FLIP_X:
        edFlipBrush(true);
        break;
    case CMD_BRUSH_FLIP_Y:
        edFlipBrush(false);
        break;
    case CMD_BRUSH_TRIM:
        edCycleBrushTrim(mkKeyDown(VK_SHIFT) ? -1 : 1);
        break;
    case CMD_PROP_DEC:
    case CMD_PROP_INC:
        edAdjustParam(arg, id == CMD_PROP_INC ? 1 : -1);
        break;
    case CMD_PROP_SIZE:
        edResizeProp(arg);
        break;
    case CMD_PROP_ROTATE:
        if (makerEd.propObject >= 0) {
            MakerObject o = makerEd.level.objects[makerEd.propObject];
            o.angle = (float)(((int)o.angle + 90) % 360);
            float w = o.w;
            o.w = o.h;
            o.h = w;
            edBeginGroup();
            edSetObject(makerEd.propObject, o);
        }
        break;
    case CMD_PROP_DELETE:
        if (makerEd.propObject >= 0) {
            edBeginGroup();
            edRemoveObject(makerEd.propObject);
            makerEd.propObject = -1;
            makerEd.entityCost = edEntityCost();
        }
        break;
    case CMD_PROP_CLOSE:
        makerEd.propObject = -1;
        edClearSelection();
        break;
    case CMD_SET_NAME:
        makerEd.nameEditing = true;
        break;
    case CMD_SET_DONE:
        makerEd.settings = makerEd.nameEditing = false;
        break;
    default:
        if (id >= CMD_SET_THEME && id <= CMD_SET_AHEAD_Y) {
            makerEd.nameEditing = false;
            edCycleSetting(id, arg);
        }
        break;
    }
}

int edResizeHandle(float sx, float sy) {
    MakerLevel &l = makerEd.level;
    if (makerEd.propObject < 0 || makerEd.propObject >= l.objectCount)
        return 0;
    const MakerObject &o = l.objects[makerEd.propObject];
    const MakerObjectDef *d = makerDefFor(o.klass);
    if (!d || !(d->flags & (OBJ_RESIZE_X | OBJ_RESIZE_Y)))
        return 0;
    Rect r = edObjectRect(o);
    float tolerance = 6;
    float cx = edScreenX((r.l + r.r) * 0.5f), cy = edScreenY((r.b + r.t) * 0.5f);
    if ((d->flags & OBJ_RESIZE_X) && absf(sy - cy) < tolerance) {
        if (absf(sx - edScreenX(r.r)) < tolerance)
            return 1;
        if (absf(sx - edScreenX(r.l)) < tolerance)
            return 2;
    }
    if ((d->flags & OBJ_RESIZE_Y) && absf(sx - cx) < tolerance) {
        if (absf(sy - edScreenY(r.t)) < tolerance)
            return 3;
        if (absf(sy - edScreenY(r.b)) < tolerance)
            return 4;
    }
    return 0;
}

int edBoundsHandle(float sx, float sy) {
    const MakerHeader &h = makerEd.level.head;
    int edge = 0;
    float rx = edScreenX((float)h.width), ty = edScreenY((float)h.height);
    if (absf(sx - rx) < 6 && sy >= edScreenY(0) - 6 && sy <= ty + 6)
        edge |= 1;
    if (absf(sy - ty) < 6 && sx >= edScreenX(0) - 6 && sx <= rx + 6)
        edge |= 2;
    return edge;
}

void edMouseMove(float x, float y) {
    makerEd.mouseX = x;
    makerEd.mouseY = y;
    float wx = edWorldX(x), wy = edWorldY(y);
    switch (makerEd.drag) {
    case DRAG_PAN:
        makerEd.camX = makerEd.dragCamX - (x - makerEd.dragMouseX) / makerEd.zoom;
        makerEd.camY = makerEd.dragCamY - (y - makerEd.dragMouseY) / makerEd.zoom;
        edClampCamera();
        break;
    case DRAG_PAINT:
        edPaintStroke(wx, wy);
        break;
    case DRAG_ERASE:
        edEraseStroke(wx, wy, makerEd.dragButton == 1 || mkKeyDown(VK_SHIFT));
        break;
    default:
        break;
    }
    if (makerEd.drag == DRAG_NONE)
        makerEd.boundsEdge =
            edInCanvas(x, y) && makerEd.tool != TOOL_ERASE ? edBoundsHandle(x, y) : 0;
}

void edMouseDown(float x, float y, int button) {
    makerEd.mouseX = x;
    makerEd.mouseY = y;
    if (makerEd.loading || !makerCatalogReady)
        return;
    if (makerEd.help) {
        makerEd.help = false;
        return;
    }
    if (button == 0) {
        MakerButton *b = makerEd.buttons.hit(x, y);
        if (b) {
            if (makerEd.nameEditing && b->id != CMD_SET_NAME)
                makerEd.nameEditing = false;
            edCommand(b->id, b->arg);
            return;
        }
    } else if (makerEd.picker || makerEd.settings) {
        return;
    }
    if (!edInCanvas(x, y) || makerEd.drag != DRAG_NONE)
        return;
    float wx = edWorldX(x), wy = edWorldY(y);
    makerEd.dragButton = button;
    makerEd.dragMouseX = x;
    makerEd.dragMouseY = y;
    makerEd.dragCamX = makerEd.camX;
    makerEd.dragCamY = makerEd.camY;
    makerEd.dragWorldX = wx;
    makerEd.dragWorldY = wy;
    makerEd.lastPaintValid = false;
    if (button == 2 || (button == 0 && (mkKeyDown(VK_SPACE) || makerEd.tool == TOOL_HAND))) {
        makerEd.drag = DRAG_PAN;
        return;
    }
    if (button == 1) {
        edBeginGroup();
        makerEd.drag = DRAG_ERASE;
        makerEd.eraseMode = -1;
        edEraseStroke(wx, wy, true);
        return;
    }
    bool doubleClick = makerEd.age - makerEd.lastClickTime < 0.35f &&
                       absf(x - makerEd.lastClickX) < 5 && absf(y - makerEd.lastClickY) < 5;
    makerEd.lastClickTime = makerEd.age;
    makerEd.lastClickX = x;
    makerEd.lastClickY = y;
    if (mkKeyDown(VK_MENU)) {
        edEyedropper(wx, wy);
        return;
    }
    bool select = makerEd.tool == TOOL_SELECT, shift = mkKeyDown(VK_SHIFT),
         ctrl = mkKeyDown(VK_CONTROL);
    int object = edGrabAt(wx, wy);
    if (doubleClick && !shift && !ctrl) {
        // Select also opens an area from its interior; the pen keeps painting there.
        int target = object >= 0 || !select ? object : edObjectAt(wx, wy, true);
        if (target >= 0) {
            edSelectSingleObject(target);
            return;
        }
    }
    int edge = makerEd.tool != TOOL_ERASE ? edBoundsHandle(x, y) : 0;
    if (edge && object < 0) {
        makerEd.boundsEdge = edge;
        makerEd.drag = DRAG_BOUNDS;
        return;
    }
    if (makerEd.tool == TOOL_ERASE) {
        edBeginGroup();
        makerEd.drag = DRAG_ERASE;
        makerEd.eraseMode = -1;
        edEraseStroke(wx, wy, shift);
        return;
    }
    int handle = edResizeHandle(x, y);
    if (handle && (select || (!shift && !ctrl))) {
        makerEd.resizeEdge = handle;
        makerEd.drag = DRAG_RESIZE;
        return;
    }
    if (select) {
        if (object >= 0) {
            if (!makerEd.objectSel[object]) {
                if (!shift)
                    edClearSelection();
                makerEd.objectSel[object] = 1;
                makerEd.selObjects++;
            }
            makerEd.propObject = makerEd.selObjects == 1 && !makerEd.selTiles ? object : -1;
            makerEd.drag = DRAG_MOVE;
            return;
        }
        if (edStartAt(wx, wy)) {
            makerEd.drag = DRAG_START;
            return;
        }
        int tile = edTileAt(wx, wy, makerEd.layer);
        if (tile >= 0) {
            if (!makerEd.tileSel[tile]) {
                if (!shift)
                    edClearSelection();
                makerEd.tileSel[tile] = 1;
                makerEd.selTiles++;
            }
            makerEd.propObject = -1;
            makerEd.drag = DRAG_MOVE;
            return;
        }
        if (!shift)
            edClearSelection();
        makerEd.propObject = -1;
        makerEd.drag = DRAG_MARQUEE;
        return;
    }
    // Pen: grab an existing item (or an area by its frame) to move it, otherwise paint.
    if (object >= 0 && !shift && !ctrl) {
        edSelectSingleObject(object);
        makerEd.drag = DRAG_MOVE;
        return;
    }
    if (edStartAt(wx, wy) && !shift) {
        makerEd.drag = DRAG_START;
        return;
    }
    if (mkKeyDown(VK_SHIFT)) {
        makerEd.drag = DRAG_RECT;
        return;
    }
    edBeginGroup();
    makerEd.drag = DRAG_PAINT;
    int before = makerEd.opTop;
    edPaintStroke(wx, wy);
    if (makerEd.opTop != before)
        sound(makerEd.brush.object ? "coin" : "jump", 1.3f, 0.25f);
}

void edMouseUp(float x, float y, int button) {
    makerEd.mouseX = x;
    makerEd.mouseY = y;
    int drag = makerEd.drag;
    if (drag == DRAG_NONE || button != makerEd.dragButton)
        return;
    makerEd.drag = DRAG_NONE;
    float wx = edWorldX(x), wy = edWorldY(y);
    MakerLevel &l = makerEd.level;
    switch (drag) {
    case DRAG_RECT:
        edFillRect(edDragRect(makerEd.dragWorldX, makerEd.dragWorldY, wx, wy));
        makerEd.entityCost = edEntityCost();
        break;
    case DRAG_MARQUEE: {
        Rect r = {minf(makerEd.dragWorldX, wx), minf(makerEd.dragWorldY, wy),
                  maxf(makerEd.dragWorldX, wx), maxf(makerEd.dragWorldY, wy)};
        edSelectRect(r, true);
        if (makerEd.selObjects == 1 && !makerEd.selTiles)
            for (int i = 0; i < l.objectCount; i++)
                if (makerEd.objectSel[i])
                    makerEd.propObject = i;
        break;
    }
    case DRAG_MOVE: {
        float dx, dy;
        edMoveDelta(&dx, &dy);
        if (dx == 0 && dy == 0)
            break;
        edBeginGroup();
        for (int i = 0; i < l.objectCount; i++)
            if (makerEd.objectSel[i]) {
                MakerObject o = l.objects[i];
                o.x += dx;
                o.y += dy;
                edSetObject(i, o);
            }
        for (int i = 0; i < l.tileCount; i++)
            if (makerEd.tileSel[i]) {
                MakerTile t = l.tiles[i];
                t.l += dx;
                t.r += dx;
                t.b += dy;
                t.t += dy;
                edSetTile(i, t);
            }
        sound("switch", 0.9f, 0.3f);
        break;
    }
    case DRAG_START: {
        float sx = edSnapCoord(wx, 32) + 16, sy = edSnapCoord(wy, 32) + 16;
        edBeginGroup();
        edSetStart(sx, sy);
        break;
    }
    case DRAG_RESIZE: {
        if (makerEd.propObject < 0)
            break;
        MakerObject o = edResizeDragObject(wx, wy);
        edBeginGroup();
        edSetObject(makerEd.propObject, o);
        makerEd.entityCost = edEntityCost();
        break;
    }
    case DRAG_BOUNDS: {
        int w, ht;
        edBoundsDragSize(wx, wy, &w, &ht);
        edBeginGroup();
        edSetSize(w, ht);
        break;
    }
    default:
        break;
    }
    makerEd.lastPaintValid = false;
}

// Panel rows under the cursor take the wheel: settings and item values step like their arrows.
bool edWheelPanel(float x, float y, int direction) {
    MakerButton *b = makerEd.buttons.hit(x, y);
    if (!b)
        return false;
    int id = b->id, arg = b->arg;
    if (id == CMD_WHEEL) {
        if (arg >= 2000) {
            id = CMD_PROP_SIZE;
            arg = (arg - 2000) * 2;
        } else if (arg >= 1000) {
            id = CMD_PROP_INC;
            arg -= 1000;
        } else
            id = arg;
    }
    if (id >= CMD_SET_THEME && id <= CMD_SET_AHEAD_Y) {
        makerEd.nameEditing = false;
        edCycleSetting(id, direction);
        return true;
    }
    if (id == CMD_PROP_DEC || id == CMD_PROP_INC) {
        edAdjustParam(arg, direction);
        return true;
    }
    if (id == CMD_PROP_SIZE) {
        edResizeProp(arg / 2 * 2 + (direction > 0 ? 1 : 0));
        return true;
    }
    return false;
}

void edWheel(int delta) {
    float x = makerEd.mouseX, y = makerEd.mouseY;
    int direction = delta > 0 ? 1 : -1;
    if (makerEd.picker) {
        makerEd.pickerScroll -= direction;
        return;
    }
    if (makerEd.help || edWheelPanel(x, y, direction) || makerEd.settings)
        return;
    if (makerEd.propObject >= 0 && x >= viewW - makerEd.propsW)
        return;
    if (y < makerEd.paletteH) {
        makerEd.scroll -= direction * 3;
        return;
    }
    if (mkKeyDown(VK_SHIFT)) {
        makerEd.camX -= direction * 160 / makerEd.zoom;
        edClampCamera();
        return;
    }
    edZoomAt(x, y, direction);
}

void edSelectAll() {
    edClearSelection();
    MakerLevel &l = makerEd.level;
    for (int i = 0; i < l.tileCount; i++)
        if (l.tiles[i].layer == makerEd.layer) {
            makerEd.tileSel[i] = 1;
            makerEd.selTiles++;
        }
    for (int i = 0; i < l.objectCount; i++) {
        makerEd.objectSel[i] = 1;
        makerEd.selObjects++;
    }
    makerEd.tool = TOOL_SELECT;
}

void edCycleSelectionCollide() {
    MakerLevel &l = makerEd.level;
    int next = -1;
    edBeginGroup();
    for (int i = 0; i < l.tileCount; i++)
        if (makerEd.tileSel[i]) {
            MakerTile t = l.tiles[i];
            if (next < 0)
                next = (t.collide + 1) % COLLIDE_KINDS;
            t.collide = (u8)next;
            edSetTile(i, t);
        }
    if (next >= 0) {
        char text[48];
        wsprintfA(text, "Hitbox: %s", edCollideName(next));
        mkToast(text);
    }
}

bool edKey(int key) {
    if (makerEd.loading || !makerCatalogReady)
        return true;
    bool ctrl = mkKeyDown(VK_CONTROL), shift = mkKeyDown(VK_SHIFT);
    if (makerEd.help) {
        makerEd.help = false;
        return true;
    }
    if (makerEd.nameEditing) {
        if (key == VK_RETURN || key == VK_ESCAPE || key == VK_TAB)
            makerEd.nameEditing = false;
        return true;
    }
    if (key == VK_ESCAPE) {
        if (makerEd.picker || makerEd.settings) {
            makerEd.picker = makerEd.settings = false;
            return true;
        }
        if (makerEd.drag != DRAG_NONE) {
            makerEd.drag = DRAG_NONE;
            return true;
        }
        if (makerEd.selTiles || makerEd.selObjects || makerEd.propObject >= 0) {
            edClearSelection();
            makerEd.propObject = -1;
            return true;
        }
        edCommand(CMD_EXIT, 0);
        return true;
    }
    if (key == VK_F11)
        return false;
    if (key == VK_F1) {
        makerEd.help = true;
        return true;
    }
    if (key == VK_F2) {
        edCommand(CMD_SETTINGS, 0);
        return true;
    }
    if (key == VK_RETURN || key == VK_F5) {
        if (!makerEd.settings && !makerEd.picker)
            makerStartTest(shift);
        return true;
    }
    if (key == VK_TAB) {
        makerEd.picker = !makerEd.picker;
        return true;
    }
    if (makerEd.settings || makerEd.picker)
        return true;
    if (ctrl) {
        switch (key) {
        case 'Z':
            if (shift)
                edRedo();
            else
                edUndo();
            break;
        case 'Y':
            edRedo();
            break;
        case 'S':
            edSaveCourse();
            break;
        case 'C':
            edCopySelection();
            break;
        case 'X':
            edCopySelection();
            edDeleteSelection();
            break;
        case 'V':
            edPaste();
            break;
        case 'D':
            edCopySelection();
            edPaste();
            break;
        case 'A':
            edSelectAll();
            break;
        case 'E':
            edCommand(CMD_SETTINGS, 0);
            break;
        }
        return true;
    }
    switch (key) {
    case '1':
    case '2':
    case '3':
        makerEd.layer = key - '1';
        break;
    case 'P':
    case 'B':
        makerEd.tool = TOOL_PEN;
        edClearSelection();
        break;
    case 'E':
        makerEd.tool = TOOL_ERASE;
        edClearSelection();
        break;
    case 'S':
        makerEd.tool = TOOL_SELECT;
        break;
    case 'H':
        makerEd.tool = TOOL_HAND;
        break;
    case 'G':
        edCommand(CMD_GRID, 0);
        break;
    case 'K':
        makerEd.showCollision = !makerEd.showCollision;
        break;
    case 'L':
        makerEd.dimOthers = !makerEd.dimOthers;
        break;
    case 'R':
        if (makerEd.propObject >= 0 && makerEd.tool == TOOL_SELECT) {
            const MakerObjectDef *d = makerDefFor(makerEd.level.objects[makerEd.propObject].klass);
            if (d && (d->flags & OBJ_ROTATE))
                edCommand(CMD_PROP_ROTATE, 0);
        } else
            edRotateBrush(shift ? 3 : 1);
        break;
    case 'F':
        edFlipBrush(true);
        break;
    case 'V':
        edFlipBrush(false);
        break;
    case 'C':
        if (makerEd.selTiles)
            edCycleSelectionCollide();
        else
            edCycleBrushCollide();
        break;
    case 'Q':
        edEyedropper(edWorldX(makerEd.mouseX), edWorldY(makerEd.mouseY));
        break;
    case 'T':
        edCycleBrushTrim(shift ? -1 : 1);
        break;
    case 'M':
        musicEnabled = !musicEnabled;
        if (musicEnabled)
            playMusic(makerCourseMusic(makerEd.level.head));
        else
            stopMusic();
        writeProgress();
        break;
    case VK_DELETE:
    case VK_BACK:
        if (makerEd.selTiles || makerEd.selObjects)
            edDeleteSelection();
        else if (makerEd.propObject >= 0)
            edCommand(CMD_PROP_DELETE, 0);
        break;
    case VK_HOME:
        edFitCourse();
        break;
    case VK_OEM_PLUS:
    case VK_ADD:
        edZoomAt(viewW * 0.5f, (edCanvasBottom() + edCanvasTop()) * 0.5f, 1);
        break;
    case VK_OEM_MINUS:
    case VK_SUBTRACT:
        edZoomAt(viewW * 0.5f, (edCanvasBottom() + edCanvasTop()) * 0.5f, -1);
        break;
    case VK_PRIOR:
        makerEd.tab = (makerEd.tab + TAB_COUNT - 1) % TAB_COUNT;
        makerEd.scroll = 0;
        break;
    case VK_NEXT:
        makerEd.tab = (makerEd.tab + 1) % TAB_COUNT;
        makerEd.scroll = 0;
        break;
    }
    return true;
}

void edChar(int ch) {
    if (!makerEd.nameEditing)
        return;
    MakerHeader &h = makerEd.level.head;
    int n = lstrlenA(h.name);
    if (ch == 8) {
        if (n > 0)
            h.name[n - 1] = 0;
    } else if (ch >= 32 && ch < 127 && n < MAKER_NAME - 1) {
        h.name[n] = (char)ch;
        h.name[n + 1] = 0;
    } else
        return;
    makerEd.dirty = true;
}

void edRefreshBackground() {
    makerBakeBackgroundPreview(makerEd.level.head, makerEd.background, makerEd.backgroundArena);
    nativeContinuousResetScene(makerEd.background);
    makerEd.weatherAge = 0;
    makerEd.backgroundDirty = false;
}

void edTick(float dt) {
    makerEd.age += dt;
    makerEd.hoverTime += dt;
    makerToast.age += dt;
    if (makerEd.waterDirty && makerCatalogReady)
        edRefreshWater();
    makerEd.waterAge += dt;
    if (makerEd.loading) {
        makerBuildCatalog();
        makerEd.loading = false;
        if (!makerEd.brush.object && makerEd.brush.tile.r <= 0 && makerCategoryCount[PIECE_BLOCK])
            edPieceBrush(makerCategoryList[PIECE_BLOCK][0]);
        makerEd.entityCost = edEntityCost();
        if (makerEd.backgroundDirty)
            edRefreshBackground();
        makerEd.weatherAge += dt;
        return;
    }
    if (makerEd.backgroundDirty)
        edRefreshBackground();
    makerEd.weatherAge += dt;
    if (makerEd.nameEditing || makerEd.settings || makerEd.picker)
        return;
    float speed = 520 / makerEd.zoom * dt * (mkKeyDown(VK_SHIFT) ? 3 : 1);
    bool moved = false;
    if (keys[VK_LEFT]) {
        makerEd.camX -= speed;
        moved = true;
    }
    if (keys[VK_RIGHT]) {
        makerEd.camX += speed;
        moved = true;
    }
    if (keys[VK_UP]) {
        makerEd.camY += speed;
        moved = true;
    }
    if (keys[VK_DOWN]) {
        makerEd.camY -= speed;
        moved = true;
    }
    if (moved) {
        edClampCamera();
        if (makerEd.drag == DRAG_PAINT || makerEd.drag == DRAG_ERASE)
            edMouseMove(makerEd.mouseX, makerEd.mouseY);
    }
}

void edSaveCourse() {
    makerPrepareHeader(makerEd.level);
    if (makerSaveLevel(makerEd.slot, makerEd.level)) {
        makerEd.dirty = false;
        makerEd.edited = false;
        mkToast("Course saved", 0x80ff80ff);
        sound("checkpoint-save", 1, 0.5f);
    } else
        mkToast("Could not save the course", 0xff8080ff);
}

// Open a slot in the editor; the camera starts at Bean's start position. A course that cannot be
// read stays untouched on disk and is not opened.
bool edOpen(int slot, bool fresh, int theme) {
    makerAllocate(makerEd.level);
    if (!makerEd.ops) {
        makerEd.ops = (MakerOp *)mkAlloc(sizeof(MakerOp) * MAKER_UNDO);
        makerEd.tileSel = (u8 *)mkAlloc(MAKER_MAX_TILES);
        makerEd.objectSel = (u8 *)mkAlloc(MAKER_MAX_OBJECTS);
        makerEd.zoom = 1;
        makerEd.grid = 32;
        makerEd.tab = TAB_BLOCKS;
        makerEd.layer = MAKER_MAIN;
        makerEd.tool = TOOL_PEN;
    }
    bool loaded = !fresh && makerLoadLevel(slot, makerEd.level);
    if (!fresh && !loaded)
        return false;
    if (!loaded)
        makerNewCourse(makerEd.level, theme);
    makerEd.world = makerEd.level.head.theme;
    makerEd.scroll = makerEd.pickerScroll = 0;
    makerEd.slot = slot;
    makerEd.open = true;
    makerEd.dirty = !loaded;
    makerEd.edited = false;
    makerEd.opCount = makerEd.opTop = 0;
    makerEd.propObject = -1;
    makerEd.drag = DRAG_NONE;
    makerEd.picker = makerEd.settings = makerEd.help = makerEd.nameEditing = false;
    makerEd.collisionDirty = makerEd.backgroundDirty = true;
    makerEd.waterDirty = true;
    edClearSelection();
    makerEd.zoom = 1;
    makerEd.camX = makerEd.level.head.startX - 120;
    makerEd.camY = makerEd.level.head.startY - 90;
    edClampCamera();
    if (makerCatalogReady) {
        if (!makerEd.brush.object && makerEd.brush.tile.r <= 0 && makerCategoryCount[PIECE_BLOCK])
            edPieceBrush(makerCategoryList[PIECE_BLOCK][0]);
        makerEd.entityCost = edEntityCost();
    }
    return true;
}
