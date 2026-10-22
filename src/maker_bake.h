#pragma once

// Bakes an editable course into the authored Level layout the campaign runtime already plays:
// copied stage background layers, user tile layers, generated terrain geometry and objects cloned
// from the project classes with their edited behaviour parameters.

enum { MAKER_BG_MARGIN = 512, MAKER_VIEW_W = 1400, MAKER_VIEW_H = 480, MAKER_CELL = 8 };

char *makerString(Arena &m, const char *s) {
    int n = lstrlenA(s);
    char *p = (char *)m.alloc(n + 1);
    memcpy(p, s, n);
    return p;
}

// Copy-on-write: cloned objects share their pair arrays with the project class.
void makerSetProp(Props &p, const char *key, const char *value, Arena &m) {
    int at = -1;
    for (int i = 0; i < p.count; i++)
        if (eq(p.pairs[i].key, key))
            at = i;
    Pair *pairs = m.array<Pair>(p.count + (at < 0 ? 1 : 0));
    if (p.count)
        memcpy(pairs, p.pairs, sizeof(Pair) * p.count);
    if (at < 0) {
        at = p.count++;
        pairs[at].key = makerString(m, key);
    }
    pairs[at].value = makerString(m, value);
    p.pairs = pairs;
}

void makerSetNumber(Props &p, const char *key, float value, Arena &m) {
    char text[32];
    mkFormat(text, value, 3);
    makerSetProp(p, key, text, m);
}

void makerSetInt(Props &p, const char *key, int value, Arena &m) {
    char text[16];
    wsprintfA(text, "%d", value);
    makerSetProp(p, key, text, m);
}

Props *makerBehaviorProps(Object &o, const char *type) {
    for (int i = 0; i < o.behaviorCount; i++)
        if (eq(o.behaviors[i].type, type))
            return &o.behaviors[i].props;
    return 0;
}

void makerAddBehavior(Object &o, const char *type, Arena &m) {
    if (o.has(type))
        return;
    Behavior *list = m.array<Behavior>(o.behaviorCount + 1);
    if (o.behaviorCount)
        memcpy(list, o.behaviors, sizeof(Behavior) * o.behaviorCount);
    memset(list + o.behaviorCount, 0, sizeof(Behavior));
    list[o.behaviorCount].type = makerString(m, type);
    o.behaviors = list;
    o.behaviorCount++;
}

struct MakerObjectList {
    Object *items;
    int count, capacity;
    u64 nextId;

    Object *push(const Object &source) {
        if (count >= capacity)
            return 0;
        Object &o = items[count++];
        o = source;
        return &o;
    }
};

// Clone a project class with private behaviour storage so edited parameters stay per instance.
Object *makerAddClass(MakerObjectList &list, u64 klass, Rect rect, int layer, Arena &m) {
    Object *c = classById(klass);
    if (!c)
        return 0;
    Object *o = list.push(*c);
    if (!o)
        return 0;
    o->behaviors = m.array<Behavior>(c->behaviorCount);
    if (c->behaviorCount)
        memcpy(o->behaviors, c->behaviors, sizeof(Behavior) * c->behaviorCount);
    o->id = list.nextId++;
    o->paused = 0;
    o->visible = 1;
    o->rect = rect;
    o->angle = 0;
    o->layer = layer;
    return o;
}

Rect makerCentered(float x, float y, float w, float h) {
    return Rect{x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f};
}

// Particle emitters are sized per area; keep their density close to the authored examples.
void makerScaleParticles(Object &o, float scale, Arena &m) {
    scale = clamp(scale, 0.2f, 4);
    makerSetInt(o.props, "max-particles", (int)maxf(1, o.props.num("max-particles", 50) * scale),
                m);
    makerSetNumber(o.props, "particles-per-second",
                   maxf(0.5f, o.props.num("particles-per-second", 10) * scale), m);
}

// Write edited values into the cloned behaviours. Paired parameters (oscillator axis, travel,
// start and direction; period and reversal; pendulum start pose; wind direction, strength and
// cap) are always written together.
void makerApplyParams(Object &obj, const MakerObject &mo, const MakerObjectDef &d, Arena &m) {
    for (int i = 0; i < d.paramCount; i++) {
        const MakerParamDef &p = d.params[i];
        bool set = (mo.setMask & (1u << i)) != 0;
        Props *b = makerBehaviorProps(obj, p.behavior);
        if (!b)
            continue;
        float v = makerParam(mo, d, i);
        switch (p.write) {
        case WRITE_GEM_STYLE:
            if (set)
                makerSetInt(obj.props, "maker-gem-style", makerRound(v), m);
            break;
        case WRITE_POTION_FORM:
            makerSetInt(obj.props, "maker-potion-form", makerRound(v), m);
            break;
        case WRITE_SET:
            if (set)
                makerSetNumber(*b, p.key, v, m);
            break;
        case WRITE_ACTIVITY:
            // Waiting actors start paused; "only near" actors also sleep again when Bean leaves.
            if (set) {
                int mode = makerRound(v);
                obj.paused = mode == 1;
                makerSetProp(*b, p.key, mode == 2 ? "1" : "0", m);
            }
            break;
        case WRITE_DEFEAT: {
            int colour = makerRound(v);
            if (set) {
                makerSetProp(*b, p.key, colour > 0 ? "1" : "0", m);
                if (colour > 0)
                    makerSetInt(*b, p.key2, 1 << ((colour - 1) & 3), m);
            }
            break;
        }
        case WRITE_MASK0:
            if (set) {
                int colour = makerRound(v);
                makerSetInt(*b, p.key, colour > 0 ? 1 << ((colour - 1) & 3) : 0, m);
            }
            break;
        case WRITE_SPREAD:
            if (set)
                makerSetProp(*b, p.key, makerSpreads[makerRound(v) & 3], m);
            break;
        case WRITE_HITS: {
            // One point per hit, from below and, when chosen, from any other side.
            int sides = makerParamIndex(d, WRITE_BREAK_SIDES);
            bool sidesSet = sides >= 0 && (mo.setMask & (1u << sides));
            if (set || sidesSet) {
                bool any = sides >= 0 && makerParam(mo, d, sides) > 0.5f;
                makerSetInt(*b, p.key, makerRound(v), m);
                makerSetInt(*b, p.key2, 1, m);
                makerSetInt(*b, "hp-on-other-hit", any ? 1 : 0, m);
            }
            break;
        }
        case WRITE_PARTICLES:
            if (set)
                makerScaleParticles(obj, v / 100, m);
            break;
        case WRITE_BOTH:
            if (set) {
                makerSetNumber(*b, p.key, v, m);
                makerSetNumber(*b, p.key2, v, m);
            }
            break;
        case WRITE_AXIS: {
            int range = makerParamIndex(d, WRITE_RANGE), sign = makerParamIndex(d, WRITE_SIGN),
                start = makerParamIndex(d, WRITE_START);
            u32 group = (1u << i) | (range >= 0 ? 1u << range : 0) | (sign >= 0 ? 1u << sign : 0) |
                        (start >= 0 ? 1u << start : 0);
            if (mo.setMask & group) {
                MakerOscillator osc = makerOscillator(mo, d);
                float lo, hi;
                makerOscillatorOffsets(osc, &lo, &hi);
                makerSetNumber(*b, "move-x", osc.vertical ? 0 : osc.sign, m);
                makerSetNumber(*b, "move-y", osc.vertical ? osc.sign : 0, m);
                makerSetNumber(*b, "min-move", lo, m);
                makerSetNumber(*b, "max-move", hi, m);
            }
            break;
        }
        case WRITE_SWING_ANGLE: {
            int dir = makerParamIndex(d, WRITE_SWING_DIR), amp = makerParamByKey(d, "MaxAngle"),
                period = makerParamByKey(d, "Period");
            bool dirSet = dir >= 0 && (mo.setMask & (1u << dir));
            bool shapeSet = (amp >= 0 && (mo.setMask & (1u << amp))) ||
                            (period >= 0 && (mo.setMask & (1u << period)));
            // A new swing angle or time keeps the authored start pose unless one was chosen.
            if (set || dirSet || shapeSet) {
                float limit = amp >= 0 ? makerParam(mo, d, amp) : b->num("MaxAngle", 60),
                      time = period >= 0 ? makerParam(mo, d, period) : b->num("Period", 2);
                bool right = dir >= 0 && makerParam(mo, d, dir) > 0.5f;
                makerSetNumber(*b, p.key, makerSwingOffset(v, limit, time, right), m);
            }
            break;
        }
        case WRITE_BULLET_SPEED:
            if (set)
                makerSetNumber(*b, p.key, v, m);
            break;
        case WRITE_DIAGONAL:
            if (set) {
                int heading = makerRound(v) & 3;
                makerSetNumber(*b, "move-x", heading & 1 ? 1.0f : -1.0f, m);
                makerSetNumber(*b, "move-y", heading & 2 ? 1.0f : -1.0f, m);
            }
            break;
        case WRITE_PERIOD: {
            int sign = makerParamIndex(d, WRITE_PERIOD_SIGN);
            bool signSet = sign >= 0 && (mo.setMask & (1u << sign));
            if (set || signSet) {
                bool reverse = sign >= 0 && makerParam(mo, d, sign) > 0.5f;
                makerSetNumber(*b, p.key, reverse ? -v : v, m);
            }
            break;
        }
        case WRITE_MASK:
            if (set)
                makerSetInt(*b, p.key, 1 << (makerRound(v) & 3), m);
            break;
        case WRITE_TURRET: {
            const float x[4] = {1, 0, -1, 0}, y[4] = {0, 1, 0, -1};
            int direction = makerRound(v) & 3;
            makerSetNumber(*b, p.key, x[direction], m);
            makerSetNumber(*b, p.key2, y[direction], m);
            makerSetProp(*b, "targetingMode", "static", m);
            break;
        }
        case WRITE_FORCE_DIR: {
            int strength = makerParamIndex(d, WRITE_FORCE), cap = makerParamIndex(d, WRITE_CAP);
            float force = strength >= 0 ? makerParam(mo, d, strength) : 700;
            float limit = cap >= 0 ? makerParam(mo, d, cap) : 0;
            int direction = makerRound(v) & 3, fadeIndex = makerParamIndex(d, WRITE_FADE);
            bool vertical = direction < 2;
            float sign = direction == 0 || direction == 3 ? 1.0f : -1.0f;
            float fade = fadeIndex >= 0 ? makerParam(mo, d, fadeIndex) / 100 : 1;
            // A capped axis takes the force as a magnitude and the cap's sign as the direction;
            // an uncapped one takes a signed force.
            float corner = limit > 0 ? force : sign * force;
            // Corners on the far side from the wind's source keep `fade` of the strength.
            const char *corners[4] = {"ne", "nw", "se", "sw"};
            const bool farUp[4] = {true, true, false, false},
                       farRight[4] = {true, false, true, false};
            for (int c = 0; c < 4; c++) {
                bool distant = vertical ? farUp[c] == (sign > 0) : farRight[c] == (sign > 0);
                float value = corner * (distant ? fade : 1);
                char key[24];
                wsprintfA(key, "%s-force-x", corners[c]);
                makerSetNumber(*b, key, vertical ? 0 : value, m);
                wsprintfA(key, "%s-force-y", corners[c]);
                makerSetNumber(*b, key, vertical ? value : 0, m);
            }
            makerSetProp(*b, "use-velocity-cap-x", !vertical && limit > 0 ? "1" : "0", m);
            makerSetProp(*b, "use-velocity-cap-y", vertical && limit > 0 ? "1" : "0", m);
            makerSetNumber(*b, "velocity-cap-x", vertical ? 0 : sign * limit, m);
            makerSetNumber(*b, "velocity-cap-y", vertical ? sign * limit : 0, m);
            break;
        }
        default:
            break;
        }
    }
}

struct MakerBgPlan {
    int source;
    float scrollY, period, periodY;
    int x0, x1, y1;
    bool caps, keepTiles;
};

bool makerBackgroundObject(const Object &o) {
    return o.has("BackgroundBehaviour") || o.base == 5;
}

int makerSourceMainLayer(const MakerScan &s) {
    int bean = -1;
    for (int i = 0; i < s.objectCount; i++)
        if (s.objects[i].klass == MAKER_CLASS_BEAN)
            bean = s.objects[i].layer;
    for (int i = 0; i < s.layerCount; i++) {
        const Layer &l = s.layers[i];
        int tiles = 0;
        for (int t = 0; t < s.tileCount; t++)
            tiles += s.tiles[t].layer == i;
        if (l.scroll.x == 1 && l.scroll.y == 1 && l.autoScroll.x == 0 && l.autoScroll.y == 0 &&
            !l.flags[2] && (tiles > 20 || i == bean))
            return i;
    }
    return s.layerCount;
}

// Plan how each source background layer repeats to cover any course size and window aspect.
int makerPlanBackground(const MakerScan &s, float width, float height, MakerBgPlan *plan) {
    int count = 0, mainLayer = makerSourceMainLayer(s);
    for (int i = 0; i < mainLayer && count < 16; i++) {
        const Layer &layer = s.layers[i];
        if (layer.flags[2] || !layer.flags[1])
            continue;
        bool keep = !(layer.scroll.x == 1 && layer.scroll.y == 1 && layer.autoScroll.x == 0 &&
                      layer.autoScroll.y == 0);
        int quads = 0, objects = 0;
        float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
        for (int t = 0; t < s.tileCount; t++) {
            const Tile &tile = s.tiles[t];
            if (tile.layer != i)
                continue;
            for (int q = 0; q + 4 <= tile.bytes / 16; q += 4) {
                const Vertex *v = tile.vertices + q;
                x0 = minf(x0, v[0].x);
                y0 = minf(y0, v[0].y);
                x1 = maxf(x1, v[3].x);
                y1 = maxf(y1, v[3].y);
                quads++;
            }
        }
        for (int o = 0; o < s.objectCount; o++)
            objects += s.objects[o].layer == i && makerBackgroundObject(s.objects[o]);
        if (!keep)
            quads = 0;
        if (!quads && !objects)
            continue;
        MakerBgPlan &p = plan[count++];
        memset(&p, 0, sizeof(p));
        p.source = i;
        p.keepTiles = quads > 0;
        p.scrollY = layer.scroll.y;
        if (!quads)
            continue;
        if (layer.autoScroll.y <= 0 && y1 - y0 <= 512)
            p.scrollY = 0;
        p.caps = quads == 1 && x1 - x0 >= 400;
        float need0 = -(float)MAKER_BG_MARGIN,
              need1 = maxf((layer.autoScroll.x > 0
                                ? layer.autoScroll.x
                                : layer.scroll.x * (width - MAKER_VIEW_W) + MAKER_VIEW_W) +
                               MAKER_BG_MARGIN,
                           (float)(MAKER_VIEW_W + MAKER_BG_MARGIN));
        p.period = x1 - x0;
        if (p.period > 0 && (x0 > need0 || x1 < need1)) {
            p.x0 = makerFloor((need0 - x0) / p.period);
            p.x1 = makerCeil((need1 - x1) / p.period);
        }
        p.periodY = y1 - y0;
        if (layer.autoScroll.y <= 0 && (p.scrollY > 0 || quads > 30) && p.periodY > 0) {
            float needY = p.scrollY > 0
                              ? p.scrollY * (height - MAKER_VIEW_H) + MAKER_VIEW_H + MAKER_BG_MARGIN
                              : (float)(MAKER_VIEW_H + 64);
            if (y1 < needY)
                p.y1 = makerCeil((needY - y1) / p.periodY);
        }
    }
    return count;
}

struct MakerTextureTable {
    int index[6][4];
    Texture textures[24];
    int count;

    void reset() {
        for (int g = 0; g < 6; g++)
            for (int p = 0; p < 4; p++)
                index[g][p] = -2;
        count = 0;
    }

    int map(int group, int page) {
        if (group < 1 || group > 5 || page < 0 || page > 3)
            return -1;
        if (index[group][page] == -2) {
            Texture *t = makerAtlasPage(group, page);
            if (t && count < 24) {
                textures[count] = *t;
                index[group][page] = count++;
            } else
                index[group][page] = -1;
        }
        return index[group][page];
    }
};

struct MakerTileBuilder {
    Tile *tiles;
    int count, capacity;
    Vertex *pool;
    int used, poolSize;
    Tile *open;

    // Consecutive quads that share layer, depth and texture are drawn in one tile batch.
    void quad(int layer, int z, int texture, float l, float b, float r, float t, const float *uv) {
        if (texture < 0 || used + 4 > poolSize)
            return;
        if (!open || open->layer != layer || open->z != z || open->texture != texture ||
            open->vertices + open->count * 4 != pool + used) {
            if (count >= capacity)
                return;
            open = tiles + count++;
            memset(open, 0, sizeof(Tile));
            open->layer = layer;
            open->z = z;
            open->texture = texture;
            open->meta[0] = 5126;
            open->meta[2] = 5126;
            open->meta[3] = 8;
            open->meta[4] = 16;
            open->vertices = pool + used;
            open->rect = Rect{l, b, r, t};
        }
        Vertex *v = pool + used;
        const float xs[4] = {l, r, l, r}, ys[4] = {b, b, t, t};
        for (int i = 0; i < 4; i++) {
            v[i].x = xs[i];
            v[i].y = ys[i];
            v[i].u = uv[i * 2];
            v[i].v = uv[i * 2 + 1];
        }
        used += 4;
        open->count++;
        open->bytes = open->count * 64;
        open->rect.l = minf(open->rect.l, l);
        open->rect.b = minf(open->rect.b, b);
        open->rect.r = maxf(open->rect.r, r);
        open->rect.t = maxf(open->rect.t, t);
    }

    void raw(int layer, int z, int texture, const Vertex *v, float dx, float dy) {
        float uv[8] = {v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v, v[3].u, v[3].v};
        quad(layer, z, texture, v[0].x + dx, v[0].y + dy, v[3].x + dx, v[3].y + dy, uv);
    }
};

struct MakerGeometryBuilder {
    Geometry *items;
    int count, capacity;

    Geometry *add(int kind, int points, Arena &m) {
        if (count >= capacity)
            return 0;
        Geometry &g = items[count++];
        g.kind = kind;
        g.flags = kind == 1 ? 1 : 0;
        g.count = points;
        g.points = m.array<Vec>(points);
        return &g;
    }
};

// Trace the boundary of the solid raster into counter-clockwise loops (holes run clockwise).
// At a vertex shared only by diagonal neighbours the walk turns left, keeping regions apart.
void makerTraceSolids(const MakerLevel &course, MakerGeometryBuilder &out, Arena &m) {
    const MakerHeader &h = course.head;
    float ox = -64, oy = -64;
    int gw = (h.width + 128) / MAKER_CELL + 1, gh = (h.height + 128) / MAKER_CELL + 1;
    makerScratchReset();
    u8 *solid = (u8 *)makerScratch.alloc(gw * gh);
    bool any = false;
    for (int i = 0; i < course.tileCount + course.objectCount; i++) {
        Rect r;
        if (i < course.tileCount) {
            const MakerTile &t = course.tiles[i];
            if (t.collide != COLLIDE_SOLID)
                continue;
            r = Rect{t.l, t.b, t.r, t.t};
        } else {
            const MakerObject &o = course.objects[i - course.tileCount];
            if (o.klass != MAKER_CLASS_SWITCH)
                continue;
            // Authored switches have two trigger sensors, plus a separate 32px map solid.
            // Merge that backing into terrain so adjacent bricks do not create internal seams.
            r = makerCentered(o.x, o.y, 32, 32);
        }
        int x0 = makerRound((r.l - ox) / MAKER_CELL), x1 = makerRound((r.r - ox) / MAKER_CELL);
        int y0 = makerRound((r.b - oy) / MAKER_CELL), y1 = makerRound((r.t - oy) / MAKER_CELL);
        x0 = x0 < 0 ? 0 : x0;
        y0 = y0 < 0 ? 0 : y0;
        x1 = x1 > gw ? gw : x1;
        y1 = y1 > gh ? gh : y1;
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++) {
                solid[y * gw + x] = 1;
                any = true;
            }
    }
    if (!any)
        return;
    int vw = gw + 1, vh = gh + 1;
    int *head = (int *)makerScratch.alloc(sizeof(int) * vw * vh);
    for (int i = 0; i < vw * vh; i++)
        head[i] = -1;
    int edgeCapacity = 0;
    for (int y = 0; y < gh; y++)
        for (int x = 0; x < gw; x++)
            if (solid[y * gw + x]) {
                edgeCapacity += (y == 0 || !solid[(y - 1) * gw + x]);
                edgeCapacity += (x == gw - 1 || !solid[y * gw + x + 1]);
                edgeCapacity += (y == gh - 1 || !solid[(y + 1) * gw + x]);
                edgeCapacity += (x == 0 || !solid[y * gw + x - 1]);
            }
    int *edgeFrom = (int *)makerScratch.alloc(sizeof(int) * edgeCapacity);
    u8 *edgeDir = (u8 *)makerScratch.alloc(edgeCapacity);
    int *edgeNext = (int *)makerScratch.alloc(sizeof(int) * edgeCapacity);
    u8 *edgeUsed = (u8 *)makerScratch.alloc(edgeCapacity);
    int edges = 0;
    // Directions: 0 right, 1 up, 2 left, 3 down.
    const int dx[4] = {1, 0, -1, 0}, dy[4] = {0, 1, 0, -1};
    for (int y = 0; y < gh; y++)
        for (int x = 0; x < gw; x++) {
            if (!solid[y * gw + x])
                continue;
            int from[4] = {-1, -1, -1, -1};
            if (y == 0 || !solid[(y - 1) * gw + x])
                from[0] = y * vw + x;
            if (x == gw - 1 || !solid[y * gw + x + 1])
                from[1] = y * vw + x + 1;
            if (y == gh - 1 || !solid[(y + 1) * gw + x])
                from[2] = (y + 1) * vw + x + 1;
            if (x == 0 || !solid[y * gw + x - 1])
                from[3] = (y + 1) * vw + x;
            for (int d = 0; d < 4; d++)
                if (from[d] >= 0) {
                    edgeFrom[edges] = from[d];
                    edgeDir[edges] = (u8)d;
                    edgeNext[edges] = head[from[d]];
                    head[from[d]] = edges;
                    edges++;
                }
        }
    int *corners = (int *)makerScratch.alloc(sizeof(int) * (edges + 2));
    for (int start = 0; start < edges; start++) {
        if (edgeUsed[start])
            continue;
        int cornerCount = 0, e = start, guard = edges + 1;
        while (e >= 0 && !edgeUsed[e] && guard-- > 0) {
            edgeUsed[e] = 1;
            int d = edgeDir[e];
            int vx = edgeFrom[e] % vw + dx[d], vy = edgeFrom[e] / vw + dy[d];
            int vertex = vy * vw + vx;
            int best = -1, bestRank = 9;
            for (int c = head[vertex]; c >= 0; c = edgeNext[c]) {
                if (edgeUsed[c] && c != start)
                    continue;
                int turn = (edgeDir[c] - d + 4) & 3; // 1 left, 0 straight, 3 right
                int rank = turn == 1 ? 0 : turn == 0 ? 1 : turn == 3 ? 2 : 3;
                if (rank < bestRank) {
                    bestRank = rank;
                    best = c;
                }
            }
            if (best >= 0 && edgeDir[best] != d)
                corners[cornerCount++] = vertex;
            e = best == start ? -1 : best;
            if (best == start)
                break;
        }
        if (cornerCount < 3)
            continue;
        Geometry *g = out.add(1, cornerCount + 1, m);
        if (!g)
            return;
        for (int i = 0; i <= cornerCount; i++) {
            int vertex = corners[i % cornerCount];
            g->points[i].x = ox + (vertex % vw) * MAKER_CELL;
            g->points[i].y = oy + (vertex / vw) * MAKER_CELL;
        }
    }
}

void makerBakeGeometry(const MakerLevel &course, Level &l, Arena &m) {
    int platforms = 0, slopes = 0;
    for (int i = 0; i < course.tileCount; i++) {
        platforms += course.tiles[i].collide == COLLIDE_ONEWAY;
        slopes += course.tiles[i].collide >= COLLIDE_SLOPE_BR;
    }
    MakerGeometryBuilder out;
    out.capacity = 4096 + platforms + slopes;
    out.items = m.array<Geometry>(out.capacity);
    out.count = 0;
    makerTraceSolids(course, out, m);
    // One-way tops: equal heights that touch merge into one line, like the authored platforms.
    if (platforms) {
        makerScratchReset();
        Vec *seg = makerScratch.array<Vec>(platforms * 2);
        int n = 0;
        for (int i = 0; i < course.tileCount; i++) {
            const MakerTile &t = course.tiles[i];
            if (t.collide != COLLIDE_ONEWAY)
                continue;
            float y = (float)makerRound(t.t - t.surface);
            seg[n * 2] = Vec{(float)makerRound(t.l), y};
            seg[n * 2 + 1] = Vec{(float)makerRound(t.r), y};
            int k = n++;
            while (k > 0 &&
                   (seg[(k - 1) * 2].y > seg[k * 2].y ||
                    (seg[(k - 1) * 2].y == seg[k * 2].y && seg[(k - 1) * 2].x > seg[k * 2].x))) {
                Vec a = seg[k * 2], b = seg[k * 2 + 1];
                seg[k * 2] = seg[(k - 1) * 2];
                seg[k * 2 + 1] = seg[(k - 1) * 2 + 1];
                seg[(k - 1) * 2] = a;
                seg[(k - 1) * 2 + 1] = b;
                k--;
            }
        }
        for (int i = 0; i < n;) {
            float y = seg[i * 2].y, x0 = seg[i * 2].x, x1 = seg[i * 2 + 1].x;
            int j = i + 1;
            while (j < n && seg[j * 2].y == y && seg[j * 2].x <= x1 + 0.5f) {
                x1 = maxf(x1, seg[j * 2 + 1].x);
                j++;
            }
            Geometry *g = out.add(2, 2, m);
            if (g) {
                g->points[0] = Vec{x0, y};
                g->points[1] = Vec{x1, y};
            }
            i = j;
        }
    }
    for (int i = 0; i < course.tileCount; i++) {
        const MakerTile &t = course.tiles[i];
        if (t.collide < COLLIDE_SLOPE_BR || t.collide >= COLLIDE_KINDS)
            continue;
        float l = (float)makerRound(t.l), b = (float)makerRound(t.b), r = (float)makerRound(t.r),
              top = (float)makerRound(t.t);
        Vec tri[3];
        if (t.collide == COLLIDE_SLOPE_BR) {
            tri[0] = Vec{l, b};
            tri[1] = Vec{r, b};
            tri[2] = Vec{r, top};
        } else if (t.collide == COLLIDE_SLOPE_BL) {
            tri[0] = Vec{l, b};
            tri[1] = Vec{r, b};
            tri[2] = Vec{l, top};
        } else if (t.collide == COLLIDE_SLOPE_TR) {
            tri[0] = Vec{r, b};
            tri[1] = Vec{r, top};
            tri[2] = Vec{l, top};
        } else {
            tri[0] = Vec{l, b};
            tri[1] = Vec{r, top};
            tri[2] = Vec{l, top};
        }
        Geometry *g = out.add(1, 4, m);
        if (g) {
            for (int k = 0; k < 4; k++)
                g->points[k] = tri[k % 3];
        }
    }
    l.geometryCount = out.count;
    l.geometry = out.items;
}

int makerObjectEntities(const MakerObject &o, const MakerObjectDef &d) {
    if (d.flags & OBJ_START)
        return 0;
    Object *c = makerClass(makerEffectiveClass(o, d));
    int n = 1;
    if (c && c->base == 7 && d.cellW > 0 && d.cellH > 0)
        n = (int)maxf(1, (float)makerRound(o.w / d.cellW)) *
            (int)maxf(1, (float)makerRound(o.h / d.cellH));
    if (c && c->has("WreckingBallControllerBehaviour"))
        n += 2 + makerRound(makerParam(o, d, 0));
    if (c && c->has("OrbitingObjectBehaviour"))
        n++;
    if (o.klass == MAKER_CLASS_SWITCH)
        n++;
    if (d.flags & OBJ_WATER)
        n += 2;
    if (d.flags & OBJ_WIND)
        n += 3;
    return n;
}

// Original Moonlight Oasis water: a 64px rotated atlas gradient above the flat deep tint.
// Its visual surface is two pixels above the authored area, with waves in the top 16px.
Rect makerWaterWaveRect(Rect r) {
    return Rect{r.l, r.t - 14, r.r, r.t + 2};
}

const float makerWaterGradientUv[8] = {611.0f / 1024, 259.0f / 1024,
                                      611.0f / 1024, 260.0f / 1024,
                                      547.0f / 1024, 259.0f / 1024,
                                      547.0f / 1024, 260.0f / 1024};
const float makerWaterDeepUv[8] = {581.0f / 1024, 262.0f / 1024,
                                  582.0f / 1024, 262.0f / 1024,
                                  581.0f / 1024, 263.0f / 1024,
                                  582.0f / 1024, 263.0f / 1024};

void makerAddWaterParticles(MakerObjectList &list, Rect rect, int layer, Arena &m) {
    Object *waves = makerAddClass(list, 528389847068ULL, makerWaterWaveRect(rect), layer, m);
    if (waves)
        makerScaleParticles(*waves, (rect.r - rect.l) / 384, m);
    if (rect.r - rect.l > 48 && rect.t - rect.b > 40) {
        Object *bubbles = makerAddClass(list, 677112438672ULL,
                                      Rect{rect.l + 16, rect.b + 8, rect.r - 16, rect.t + 2},
                                      layer, m);
        if (bubbles)
            makerScaleParticles(*bubbles, (rect.r - rect.l) / 320, m);
    }
}

// Objects for one placed item, including automatic water and wind decoration.
void makerBakeObject(const MakerObject &mo, const MakerObjectDef &d, MakerObjectList &list,
                     MakerTileBuilder &tiles, MakerTextureTable &textures, int objectLayer,
                     int frontLayer, Arena &m) {
    u64 klass = makerEffectiveClass(mo, d);
    Object *c = makerClass(klass);
    if (!c)
        return;
    bool sized = c->base == 7 || c->base == 6 || c->base == 5;
    Rect rect = sized ? makerCentered(mo.x, mo.y, mo.w, mo.h)
                      : shifted(c->rect, mo.x - center(c->rect).x, mo.y - center(c->rect).y);
    int layer = (d.flags & OBJ_SECRET) ? frontLayer : objectLayer;
    Object *o = makerAddClass(list, klass, rect, layer, m);
    if (!o)
        return;
    // Authored actors that wait for Bean (red beetles, hoppers, fish) keep doing so.
    o->paused = c->paused;
    o->angle = makerEffectiveAngle(mo, d);
    if (c->base == 7) {
        makerSetInt(o->props, "actlet-width", d.cellW > 0 ? d.cellW : 32, m);
        makerSetInt(o->props, "actlet-height", d.cellH > 0 ? d.cellH : 32, m);
    }
    makerApplyParams(*o, mo, d, m);
    if (klass == MAKER_CLASS_SWITCH)
        makerAddClass(list, MAKER_CLASS_SWITCH_ICON, makerCentered(mo.x, mo.y, 16, 16),
                      objectLayer, m);
    if (d.flags & OBJ_PARTICLES)
        makerScaleParticles(*o, (mo.w * mo.h) / (float)(d.defW * d.defH), m);
    if (d.flags & OBJ_WATER) {
        float top = rect.t + 2, split = maxf(rect.b, top - 64);
        float uv[8];
        memcpy(uv, makerWaterGradientUv, sizeof(uv));
        // Shallow pools crop the gradient at the bottom instead of compressing it.
        uv[0] = uv[2] = (547 + top - split) / 1024;
        tiles.quad(frontLayer, -10, textures.map(4, 0), rect.l, split, rect.r, top, uv);
        if (split > rect.b)
            tiles.quad(frontLayer, -10, textures.map(4, 0), rect.l, rect.b, rect.r, split,
                       makerWaterDeepUv);
        // Same front layer as the fill, at the authored particle z=1: tint first, sparkles last.
        makerAddWaterParticles(list, rect, frontLayer, m);
    }
    if (d.flags & OBJ_WIND) {
        // The glowing field faces the wind; rotated actors keep unrotated rectangle sizes.
        const float angles[4] = {0, 180, 90, 270};
        int direction = makerRound(makerParam(mo, d, makerParamIndex(d, WRITE_FORCE_DIR))) & 3;
        float angle = angles[direction], w = rect.r - rect.l, h = rect.t - rect.b;
        bool side = direction >= 2;
        float lw = side ? h : w, lh = side ? w : h, top = minf(32, lh * 0.25f);
        float ux = direction == 2   ? -1.0f
                   : direction == 3 ? 1.0f
                                    : 0.0f,
              uy = direction == 0   ? 1.0f
                   : direction == 1 ? -1.0f
                                    : 0.0f;
        Vec c0 = center(rect);
        Vec bodyCenter = Vec{c0.x - ux * top * 0.5f, c0.y - uy * top * 0.5f};
        Vec topCenter = Vec{c0.x + ux * (lh - top) * 0.5f, c0.y + uy * (lh - top) * 0.5f};
        Object *body =
            makerAddClass(list, 197903476657ULL,
                          makerCentered(bodyCenter.x, bodyCenter.y, lw, lh - top), objectLayer, m);
        if (body)
            body->angle = angle;
        Object *cap =
            makerAddClass(list, 711794398021ULL, makerCentered(topCenter.x, topCenter.y, lw, top),
                          objectLayer, m);
        if (cap)
            cap->angle = angle;
        Object *vortex = makerAddClass(list, 716596827098ULL, rect, objectLayer, m);
        if (vortex) {
            vortex->angle = angle;
            makerScaleParticles(*vortex, (w * h) / (64.0f * 256), m);
        }
    }
}

// Bean's 32x32 start box overlaps a solid brick or a slope.
bool makerSpawnBlocked(const MakerLevel &course, Vec p) {
    for (int i = 0; i < course.tileCount; i++) {
        const MakerTile &t = course.tiles[i];
        if (t.collide != COLLIDE_SOLID && t.collide < COLLIDE_SLOPE_BR)
            continue;
        if (t.l < p.x + 15 && t.r > p.x - 15 && t.b < p.y + 15 && t.t > p.y - 15)
            return true;
    }
    for (int i = 0; i < course.objectCount; i++) {
        const MakerObject &o = course.objects[i];
        if (o.klass == MAKER_CLASS_SWITCH && absf(o.x - p.x) < 31 && absf(o.y - p.y) < 31)
            return true;
    }
    return false;
}

// Keep a spawn inside the course and lift it out of solid bricks, which would trap Bean.
Vec makerFreeSpawn(const MakerLevel &course, Vec p) {
    const MakerHeader &h = course.head;
    p.x = clamp(p.x, 16, h.width - 16.0f);
    p.y = clamp(p.y, 16, h.height - 16.0f);
    for (Vec q = p; q.y <= h.height + 64; q.y += 8)
        if (!makerSpawnBlocked(course, q))
            return q;
    return p;
}

Props makerLevelProps(const MakerLevel &course, int slot, Arena &m) {
    const MakerHeader &h = course.head;
    Props p = {};
    char text[32];
    makerSetProp(p, "name", h.name, m);
    makerSetProp(p, "bean-level-name", h.name, m);
    idstr(makerCourseMusic(h), text);
    makerSetProp(p, "music", makerCourseMusic(h) ? text : "", m);
    makerSetInt(p, "bean-world", h.theme, m);
    makerSetInt(p, "bean-level", slot + 1, m);
    int coins = h.coins;
    if (h.coinGoal > 0 && h.coinGoal < coins)
        coins = h.coinGoal;
    makerSetInt(p, "bean-max-coins", coins, m);
    int par = (int)maxf(1, (float)h.jumpPar);
    makerSetInt(p, "bean-jump-par", par, m);
    makerSetInt(p, "bean-jump-best", (int)clamp((float)h.jumpBest, 1, (float)par), m);
    makerSetInt(p, "width", h.width, m);
    makerSetInt(p, "height", h.height, m);
    makerSetProp(p, "grid", "16", m);
    return p;
}

struct MakerBgSource {
    MakerScan src;
    MakerBgPlan plan[16];
    int planCount;
};

void makerOpenBackground(const MakerHeader &h, MakerBgSource &bg, Arena &m) {
    memset(&bg, 0, sizeof(bg));
    int index = (int)clamp((float)h.background, 0, (float)makerBackgroundCount - 1);
    LevelInfo *info = levelById(makerBackgrounds[index].level);
    if (info && makerScanLevel(info->file, bg.src, m, true))
        bg.planCount = makerPlanBackground(bg.src, (float)h.width, (float)h.height, bg.plan);
}

void makerBackgroundLayers(const MakerBgSource &bg, Layer *out) {
    for (int i = 0; i < bg.planCount; i++) {
        out[i] = bg.src.layers[bg.plan[i].source];
        out[i].scroll.y = bg.plan[i].scrollY;
        out[i].offset = Vec{0, 0};
    }
}

void makerBackgroundBudget(const MakerBgSource &bg, int *quads, int *tiles) {
    for (int p = 0; p < bg.planCount; p++) {
        const MakerBgPlan &plan = bg.plan[p];
        if (!plan.keepTiles)
            continue;
        int copies = (plan.x1 - plan.x0 + 1) * (plan.y1 + 1) * (plan.caps ? 3 : 1);
        for (int t = 0; t < bg.src.tileCount; t++)
            if (bg.src.tiles[t].layer == plan.source) {
                *quads += bg.src.tiles[t].bytes / 64 * copies;
                *tiles += 1 + copies;
            }
    }
}

void makerBackgroundTiles(const MakerBgSource &bg, MakerTileBuilder &tiles,
                          MakerTextureTable &textures) {
    for (int p = 0; p < bg.planCount; p++) {
        const MakerBgPlan &bp = bg.plan[p];
        if (!bp.keepTiles)
            continue;
        for (int t = 0; t < bg.src.tileCount; t++) {
            const Tile &source = bg.src.tiles[t];
            if (source.layer != bp.source)
                continue;
            int texture = textures.map(bg.src.group, source.texture);
            for (int ky = 0; ky <= bp.y1; ky++)
                for (int kx = bp.x0; kx <= bp.x1; kx++) {
                    float dx = kx * bp.period, dy = ky * bp.periodY;
                    for (int q = 0; q + 4 <= source.bytes / 16; q += 4) {
                        const Vertex *v = source.vertices + q;
                        tiles.raw(p, source.z, texture, v, dx, dy);
                        // A lone sky panel continues its edge colours above and below.
                        if (bp.caps) {
                            Vertex cap[4] = {v[2], v[3], v[2], v[3]};
                            cap[2].y += 1024;
                            cap[3].y += 1024;
                            tiles.raw(p, source.z, texture, cap, dx, dy);
                            Vertex low[4] = {v[0], v[1], v[0], v[1]};
                            low[0].y -= 1024;
                            low[1].y -= 1024;
                            tiles.raw(p, source.z, texture, low, dx, dy);
                        }
                    }
                }
        }
    }
}

void makerBackgroundObjects(const MakerBgSource &bg, MakerObjectList &list, Arena &m) {
    for (int p = 0; p < bg.planCount; p++)
        for (int i = 0; i < bg.src.objectCount; i++) {
            const Object &so = bg.src.objects[i];
            if (so.layer != bg.plan[p].source || !makerBackgroundObject(so))
                continue;
            Object *o = list.push(so);
            if (!o)
                return;
            o->layer = p;
            const Layer &sl = bg.src.layers[bg.plan[p].source];
            // Screen-fixed emitters were authored for a 512-unit view; widen them for any window.
            if (o->base == 5 && sl.scroll.x == 0 && sl.autoScroll.x == 0) {
                float before = o->rect.r - o->rect.l;
                o->rect.l = minf(o->rect.l, -64);
                o->rect.r = maxf(o->rect.r, MAKER_VIEW_W + 64.0f);
                makerScaleParticles(*o, (o->rect.r - o->rect.l) / maxf(64, before), m);
            }
        }
}

// The editor preview and playable course clone the same preset and density settings.
void makerAddWeather(const MakerHeader &h, MakerObjectList &list, int layer, Arena &m) {
    int effect = (int)clamp((float)h.effect, 0, (float)makerEffectCount - 1);
    if (!makerEffects[effect].klass)
        return;
    Object *weather = makerAddClass(list, makerEffects[effect].klass, Rect{0, 0, viewW, viewH},
                                    layer, m);
    if (!weather)
        return;
    makerSetProp(weather->props, "maker-weather", "1", m);
    makerAddBehavior(*weather, "FollowCameraBehaviour", m);
    weather->rect = makerWeatherEmitterRect(*weather, viewW, viewH);
    makerScaleParticles(*weather, (viewW + 64) / 576, m);
    if (h.weatherAmount > 0)
        makerScaleParticles(*weather, h.weatherAmount / 100.0f, m);
    if (weather->klass == 697074819893ULL) {
        // A narrow window must not reduce the sparse flock to one offscreen bird.
        float amount = h.weatherAmount > 0 ? h.weatherAmount / 100.0f : 1;
        makerSetInt(weather->props, "max-particles",
                    (int)maxf(weather->props.num("max-particles"), maxf(1, 3 * amount)), m);
        makerSetNumber(weather->props, "particles-per-second",
                       maxf(weather->props.num("particles-per-second"), amount), m);
        makerSetNumber(weather->props, "initial-time", 1, m);
    }
}

// Background-only scene for the editor backdrop; it reuses the play-time tiling exactly.
void makerBakeBackgroundPreview(const MakerHeader &h, Level &l, Arena &m) {
    if (!m.data)
        m.init(12 * 1024 * 1024);
    m.reset();
    memset(&l, 0, sizeof(l));
    l.sharedTextures = true;
    l.size = Vec{(float)h.width, (float)h.height};
    l.id = h.uid;
    MakerBgSource bg;
    makerOpenBackground(h, bg, m);
    l.layerCount = bg.planCount + 1;
    l.layers = m.array<Layer>(l.layerCount);
    makerBackgroundLayers(bg, l.layers);
    Layer &weatherLayer = l.layers[bg.planCount];
    memset(&weatherLayer, 0, sizeof(weatherLayer));
    weatherLayer.flags[1] = 1;
    weatherLayer.size = Vec{viewW, viewH};
    int quads = 0, tileBudget = 0;
    makerBackgroundBudget(bg, &quads, &tileBudget);
    MakerTextureTable textures;
    textures.reset();
    MakerTileBuilder tiles;
    memset(&tiles, 0, sizeof(tiles));
    tiles.capacity = tileBudget + 1;
    tiles.tiles = m.array<Tile>(tiles.capacity);
    tiles.poolSize = quads * 4 + 4;
    tiles.pool = m.array<Vertex>(tiles.poolSize);
    makerBackgroundTiles(bg, tiles, textures);
    MakerObjectList list;
    list.capacity = bg.src.objectCount + 2;
    list.items = m.array<Object>(list.capacity);
    list.count = 0;
    list.nextId = 1;
    makerBackgroundObjects(bg, list, m);
    makerAddWeather(h, list, bg.planCount, m);
    l.objectCount = list.count;
    l.objects = list.items;
    l.textureCount = textures.count;
    l.textures = m.array<Texture>(textures.count);
    if (textures.count)
        memcpy(l.textures, textures.textures, sizeof(Texture) * textures.count);
    l.tileCount = tiles.count;
    l.tiles = tiles.tiles;
}

// Build the playable scene in the campaign level arena. Background objects are parsed straight
// into that arena so the copied pointers live exactly as long as the scene.
void makerBakeCourse(MakerLevel &course, int slot, Level &l, Arena &m, const Vec *start) {
    releaseLevel(l, m);
    makerPrepareHeader(course);
    const MakerHeader &h = course.head;
    float width = (float)h.width, height = (float)h.height;
    l.size = Vec{width, height};
    l.id = h.uid;
    l.sharedTextures = true;
    l.props = makerLevelProps(course, slot, m);
    MakerBgSource bg;
    makerOpenBackground(h, bg, m);
    int planCount = bg.planCount;
    // Layers: backgrounds, then the course's back, main, object and front layers.
    l.layerCount = planCount + 4;
    l.layers = m.array<Layer>(l.layerCount);
    makerBackgroundLayers(bg, l.layers);
    for (int i = planCount; i < l.layerCount; i++) {
        Layer &layer = l.layers[i];
        memset(&layer, 0, sizeof(layer));
        layer.id = h.uid ^ (u64)(i + 1);
        layer.scroll = Vec{1, 1};
        layer.size = Vec{width, height};
        layer.flags[0] = layer.flags[1] = 1;
    }
    int back = planCount, mainLayer = planCount + 1, objectsLayer = planCount + 2,
        front = planCount + 3;
    MakerTextureTable textures;
    textures.reset();
    // Each water area has a surface gradient and a deep fill quad.
    int quadBudget = course.tileCount + course.objectCount * 2 + 16, tileBudget = quadBudget;
    makerBackgroundBudget(bg, &quadBudget, &tileBudget);
    MakerTileBuilder tiles;
    memset(&tiles, 0, sizeof(tiles));
    tiles.capacity = tileBudget;
    tiles.tiles = m.array<Tile>(tileBudget);
    tiles.poolSize = quadBudget * 4;
    tiles.pool = m.array<Vertex>(tiles.poolSize);
    makerBackgroundTiles(bg, tiles, textures);
    // Course tiles: opaque main-layer blocks draw first so overlays such as grass stay on top.
    const int layerOf[3] = {back, mainLayer, front};
    for (int layer = 0; layer < MAKER_LAYERS; layer++)
        for (int pass = 0; pass < 2; pass++)
            for (int i = 0; i < course.tileCount; i++) {
                const MakerTile &t = course.tiles[i];
                if (t.layer != layer)
                    continue;
                bool opaque = (t.flags & TILE_OPAQUE) != 0;
                if (layer == MAKER_MAIN ? (pass == 0) != opaque : pass == 1)
                    continue;
                tiles.quad(layerOf[layer], layer == MAKER_MAIN && pass == 1 ? 10 : 0,
                           textures.map(t.group, t.texture), t.l, t.b, t.r, t.t, t.uv);
            }
    MakerObjectList list;
    list.capacity = bg.src.objectCount + course.objectCount * 4 + 8;
    list.items = m.array<Object>(list.capacity);
    list.count = 0;
    list.nextId = 0x6d6b000000000000ULL | ((h.uid & 0xffffff) << 20);
    makerBackgroundObjects(bg, list, m);
    Vec spawn = makerFreeSpawn(course, start ? *start : Vec{h.startX, h.startY});
    Object *bean = makerAddClass(list, MAKER_CLASS_BEAN, makerCentered(spawn.x, spawn.y, 32, 32),
                                 objectsLayer, m);
    if (bean) {
        bean->z = 1;
        // Course rules: run speed scales with its accelerations so Bean keeps the same feel.
        Props *tom = makerBehaviorProps(*bean, "TomPlayerBehaviour");
        if (tom && h.beanSpeed > 0) {
            float scale = clamp(h.beanSpeed / 100.0f, 0.5f, 2);
            makerSetNumber(*tom, "max-velocity", tom->num("max-velocity", 200) * scale, m);
            makerSetNumber(*tom, "move-acceleration", tom->num("move-acceleration", 500) * scale,
                           m);
            makerSetNumber(*tom, "stop-acceleration", tom->num("stop-acceleration", 200) * scale,
                           m);
        }
        if (tom && h.beanJump > 0)
            makerSetNumber(*tom, "jump-height",
                           tom->num("jump-height", 74) * clamp(h.beanJump / 100.0f, 0.5f, 2), m);
    }
    Object *camera = makerAddClass(list, MAKER_CLASS_CAMERA,
                                   makerCentered(spawn.x, spawn.y, 32, 32), objectsLayer, m);
    if (camera) {
        // Like the authored stages, a camera can follow one axis only and look ahead.
        Props *follow = makerBehaviorProps(*camera, "FollowObjectBehaviour");
        if (follow) {
            makerSetProp(*follow, "follow-x", h.camera == 2 ? "0" : "1", m);
            makerSetProp(*follow, "follow-y", h.camera == 1 ? "0" : "1", m);
            makerSetNumber(*follow, "offset-x", clamp(h.cameraAheadX, -25, 25) / 100.0f, m);
            makerSetNumber(*follow, "offset-y", clamp(h.cameraAheadY, -25, 25) / 100.0f, m);
        }
    }
    int budget = MAX_ENTITIES - 96 - list.count;
    for (int i = 0; i < course.objectCount; i++) {
        const MakerObject &mo = course.objects[i];
        const MakerObjectDef *d = makerDefFor(mo.klass);
        if (!d || (d->flags & OBJ_START))
            continue;
        int cost = makerObjectEntities(mo, *d);
        if (cost > budget)
            continue;
        budget -= cost;
        makerBakeObject(mo, *d, list, tiles, textures, objectsLayer, front, m);
    }
    makerAddWeather(h, list, front, m);
    l.objectCount = list.count;
    l.objects = list.items;
    l.textureCount = textures.count;
    l.textures = m.array<Texture>(textures.count);
    if (textures.count)
        memcpy(l.textures, textures.textures, sizeof(Texture) * textures.count);
    l.tileCount = tiles.count;
    l.tiles = tiles.tiles;
    makerBakeGeometry(course, l, m);
    makerScratchReset();
}
