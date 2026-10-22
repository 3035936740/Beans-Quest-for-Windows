#pragma once

// Fixed-function OpenGL drawing, layer transforms, animation selection and authored draw order.
#include "game.h"
#include "background_native.h"
#include "enemy_animation.h"
#include "particles_native.h"
#include "background_extension.h"
GLuint fontTexture;
float uiTime;
HDC gameDC;

void initFont() {
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = 256;
    bi.bmiHeader.biHeight = -256;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void *pixels;
    HDC dc = CreateCompatibleDC(0);
    HBITMAP bm = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &pixels, 0, 0);
    HGDIOBJ old = SelectObject(dc, bm);
    memset(pixels, 0, 256 * 256 * 4);
    HFONT font =
        CreateFontA(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH, "Courier New");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    for (int i = 32; i < 127; i++) {
        char c = (char)i;
        TextOutA(dc, (i % 16) * 16, (i / 16) * 24, &c, 1);
    }
    GdiFlush();
    u8 *b = (u8 *)pixels;
    for (int i = 0; i < 256 * 256; i++) {
        b[i * 4 + 3] = b[i * 4];
        b[i * 4] = b[i * 4 + 1] = b[i * 4 + 2] = 255;
    }
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    SelectObject(dc, oldFont);
    DeleteObject(font);
    SelectObject(dc, old);
    DeleteObject(bm);
    DeleteDC(dc);
}

void box(float x, float y, float w, float h, float r, float g, float b, float a = 1) {
    glDisable(GL_TEXTURE_2D);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void text(float x, float y, const char *s, float h = 12, float r = 1, float g = 1, float b = 1) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glColor4f(r, g, b, 1);
    float start = x;
    glBegin(GL_QUADS);
    for (; *s; s++) {
        u8 c = (u8)*s;
        if (c == '\n') {
            y -= h + 3;
            x = start;
            continue;
        }
        if (c < 32 || c > 126)
            c = '?';
        float u = (c % 16) * 16 / 256.0f, vv = (c / 16) * 24 / 256.0f;
        float w = h * 0.6666667f;
        glTexCoord2f(u, vv + 24 / 256.0f);
        glVertex2f(x, y);
        glTexCoord2f(u + 16 / 256.0f, vv + 24 / 256.0f);
        glVertex2f(x + w, y);
        glTexCoord2f(u + 16 / 256.0f, vv);
        glVertex2f(x + w, y + h);
        glTexCoord2f(u, vv);
        glVertex2f(x, y + h);
        x += h * 0.5f;
    }
    glEnd();
}

void centered(float y, const char *s, float h = 12, float r = 1, float g = 1, float b = 1) {
    text(viewW * 0.5f - lstrlenA(s) * h * 0.25f, y, s, h, r, g, b);
}

// Resolve the authored animation frame, then draw its UVs on the unchanged actor rectangle.
void drawActor(Actor *a, Rect r, const char *name, float time, float angle = 0, bool flip = false,
               int forcedFrame = -1, float alpha = 1, AnimationEndAction endAction = ANIMATION_LOOP,
               float speed = 1, bool reverse = false, float tintR = 1, float tintG = 1,
               float tintB = 1) {
    Animation *an = animation(a, name);
    if (!an || !an->count)
        return;
    int frame = forcedFrame >= 0 ? (int)clamp((float)forcedFrame, 0, (float)an->count - 1)
                                 : animationFrame(an, time, endAction, speed, reverse);
    Frame &f = an->frames[frame];
    if (f.texture < 0 || f.texture >= a->textureCount)
        return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, a->textures[f.texture].gl);
    glColor4f(tintR, tintG, tintB, alpha);
    Vec c = center(r);
    glPushMatrix();
    glTranslatef(c.x, c.y, 0);
    if (angle)
        glRotatef(angle, 0, 0, 1);
    float l = r.l - c.x, rr = r.r - c.x, b = r.b - c.y, t = r.t - c.y;
    int bl = flip ? 1 : 0, br = flip ? 0 : 1, tl = flip ? 3 : 2, tr = flip ? 2 : 3;
    glBegin(GL_QUADS);
    glTexCoord2fv(f.uv + bl * 2);
    glVertex2f(l, b);
    glTexCoord2fv(f.uv + br * 2);
    glVertex2f(rr, b);
    glTexCoord2fv(f.uv + tr * 2);
    glVertex2f(rr, t);
    glTexCoord2fv(f.uv + tl * 2);
    glVertex2f(l, t);
    glEnd();
    glPopMatrix();
}

// SecretRevealerController::OnUpdate 0x2f1744 changes LevelLayer alpha,
// so every sprite and tile on the revealed foreground shares its opacity.
float secretLayerAlpha(int layer, Level *scene = 0) {
    if (scene)
        return 1;
    float alpha = 1;
    for (int e = 0; e < entityCount; e++)
        if (entities[e].kind == SECRET && layer == entities[e].layer)
            alpha = minf(alpha, 1 - entities[e].state.aux);
    return clamp(alpha, 0, 1);
}

// The three courtyard skyline panels were authored at x4384..6208 in the
// shared .75-parallax layer. Their 1824-unit span belongs to the final sky
// region x6176..8000, after the last arena. Anchor only those panels to world
// space; otherwise changing the viewport exposes them through earlier rooms.
// Guard both the atlas UV and exact panel geometry, leaving pillars untouched.
bool courtyardSkylineTile(const Level &scene, const Tile &tile) {
    if (scene.id != 976871008727ULL || tile.layer != 2 || tile.z != 0 || tile.texture != 0 ||
        tile.bytes != 4 * sizeof(Vertex) || backgroundExtensionUvHash(tile.vertices) != 3017110361u)
        return false;
    Rect r = backgroundExtensionSourceRect(tile.vertices);
    return r.b == 0 && r.t == 230 && r.r - r.l == 608 &&
           (r.l == 4384 || r.l == 4992 || r.l == 5600);
}

void drawTile(Tile &t, Level *scene = 0, const Rect *visibleBounds = 0,
              const Vec *foregroundShift = 0) {
    Level &level = scene ? *scene : current;
    if (t.texture < 0 || t.texture >= level.textureCount)
        return;
    float alpha = secretLayerAlpha(t.layer, scene);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, level.textures[t.texture].gl);
    glColor4f(1, 1, 1, alpha);
    const BackgroundExtensionPolicy *responsive[backgroundExtensionPolicyCount];
    int responsiveCount = backgroundSceneryTilePolicies(level, t, visibleBounds, responsive);
    float courtyardX = 0;
    if (foregroundShift && visibleBounds && courtyardSkylineTile(level, t))
        courtyardX = 1792 + foregroundShift->x + visibleBounds->l;
    glBegin(GL_QUADS);
    for (int i = 0; i < t.bytes / 16; i += 4) {
        if (backgroundScenerySuppressQuad(level, t, i, responsive, responsiveCount))
            continue;
        const int order[4] = {0, 1, 3, 2};
        for (int j = 0; j < 4; j++) {
            Vertex &v = t.vertices[i + order[j]];
            glTexCoord2f(v.u, v.v);
            glVertex2f(v.x + courtyardX, v.y);
        }
    }
    glEnd();
    drawBackgroundExtensions(level, t, visibleBounds);
}

// Camera::GetParalaxPosition, native 0x155cd8. Inputs here are viewport origins;
// the original formula operates on camera centres and auto-scroll dimensions.
float layerCameraAxis(float camera, float view, float extent, float scroll, float autoSize,
                      float size, bool centre, float offset) {
    float position = camera + view * 0.5f - offset;
    if (autoSize > 0)
        return view * 0.5f + (autoSize - view) * clamp(position / maxf(1, extent), 0, 1);
    return position * scroll + (1 - scroll) * (centre ? size : view) * 0.5f;
}

Vec layerCameraPosition(Level &scene, int layer, float x, float y) {
    Layer &l = scene.layers[layer];
    Vec result = {layerCameraAxis(x, viewW, scene.size.x, l.scroll.x, l.autoScroll.x, l.size.x,
                                  l.flags[2] != 0, l.offset.x),
                  layerCameraAxis(y, viewH, scene.size.y, l.scroll.y, l.autoScroll.y, l.size.y,
                                  l.flags[2] != 0, l.offset.y)};
    return result;
}

float pixelCamera(float value, float scale) {
    float p = value * scale;
    return (float)(int)(p + (p < 0 ? -0.5f : 0.5f)) / scale;
}

Vec layerTranslation(Level &scene, int layer, float x, float y) {
    Vec c = layerCameraPosition(scene, layer, x, y);
    c.x = viewW * 0.5f - pixelCamera(c.x, screenScale);
    c.y = viewH * 0.5f - pixelCamera(c.y, screenScale);
    return c;
}

Rect backgroundRect(Object &o, Rect r, float time, float cameraX, float cameraY) {
    const Props *p = o.behavior("BackgroundBehaviour");
    if (!p)
        return r;
    float w = r.r - r.l, h = r.t - r.b;
    bool useCamera = p->flag("use-camera-x", true);
    float margin = p->num("wrap-x", w);
    if (!useCamera && margin > w * .5f) {
        // Spread a fixed set of sky actors across a wider wrap interval. Small
        // clouds retain their size; the authored overlapping cloud bank widens
        // with its spacing so it cannot develop holes or multiply into rows.
        float scale = maxf(1, viewW / maxf(1, 2 * margin - w));
        float cx = (r.l + r.r) * .5f * scale;
        // This shared class is the audited four-piece continuous cloud bank.
        // Ordinary clouds have intentional gaps and retain their original size.
        bool continuousBank = o.klass == 159837608269ULL;
        float width = continuousBank ? w * scale : w;
        r.l = cx - width * .5f;
        r.r = cx + width * .5f;
        margin *= scale;
    }
    r = shifted(r, p->num("move-x") * p->num("speed", 30) * time,
                p->num("move-y") * p->num("speed", 30) * time);
    Vec c = center(r);
    // Native OnActivate 0x11e5cc and 0x11e624 both query use-camera-x:
    // preserve that released behavior, including its use on the Y axis.
    float x = backgroundWrappedAxis(c.x, cameraX + viewW * 0.5f, viewW, margin, useCamera);
    float y =
        backgroundWrappedAxis(c.y, cameraY + viewH * 0.5f, viewH, p->num("wrap-y", h), useCamera);
    return shifted(r, x - c.x, y - c.y);
}

float particleAnimationTime(Actor *art, const char *name, float age, float life, float start,
                            float scale) {
    // ParticleInstance::Update 0x2af18c: animation position is a normalized
    // lifetime fraction, not seconds. The authored start is a fraction too.
    float phase = start + age / maxf(0.0001f, life) * scale;
    phase -= (int)phase;
    if (phase < 0)
        phase += 1;
    return phase * animationLength(animation(art, name));
}

void drawEffects(int domain, int layer) {
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++) {
        EffectParticle &p = effectParticles[i];
        if (!p.definition || p.domain != domain || p.layer != layer || p.age < p.delay)
            continue;
        // Native 0x2af220/0x2af3e8 chooses one remap branch; overlapping
        // intervals do not multiply, and short authored fades keep their span.
        float age = p.age - p.delay, phase = age / p.life,
              alpha = p.alpha * (domain == 0 ? secretLayerAlpha(layer) : 1);
        if (p.fadeIn > 0 && phase < p.fadeIn)
            alpha *= clamp(phase / p.fadeIn, 0, 1);
        else if (p.fadeOut < 1 && phase > p.fadeOut)
            alpha *= clamp((1 - phase) / (1 - p.fadeOut), 0, 1);
        const char *anim = p.definition->get("animation");
        glBlendFunc(GL_SRC_ALPHA, p.additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
        drawActor(
            p.art, Rect{p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.x + p.w * 0.5f, p.y + p.h * 0.5f},
            anim,
            particleAnimationTime(p.art, anim, age, p.life, p.animationStart, p.animationScale),
            p.angle, false, -1, alpha);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void drawParticles(Object &o, Actor *art, Rect r, float time, float effectScale = 1,
                   float layerAlpha = 1) {
    if (!art || !art->count)
        return;
    Vec origin = center(r);
    Rect bounds =
        shifted(nativeParticleEmitterBounds(r, effectScale, o.angle), -origin.x, -origin.y);
    NativeContinuousPool &pool = nativeContinuousFor(o, bounds, effectScale, time);
    if (o.props.flag("additive"))
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (int i = 0; i < pool.capacity; i++) {
        NativeContinuousParticle &particle = pool.particles[i];
        if (particle.definition < 0)
            continue;
        const NativeContinuousDef &d = pool.definitions[particle.definition];
        float age = particle.age - d.delay;
        if (age < 0 || particle.life <= 0)
            continue;
        Vec position = nativeContinuousPosition(particle, d);
        float x = origin.x + position.x, y = origin.y + position.y, phase = age / particle.life,
              fade = 1;
        if (d.fadeIn > 0 && phase < d.fadeIn)
            fade = phase / d.fadeIn;
        else if (d.fadeOut < 1 && phase > d.fadeOut)
            fade = maxf(0, (1 - phase) / maxf(0.00001f, 1 - d.fadeOut));
        float w = particle.size.x, h = particle.size.y,
              angle = particle.angle + particle.spin * age - 90;
        drawActor(art, Rect{x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f}, d.animation,
                  particleAnimationTime(art, d.animation, age, particle.life,
                                        particle.animationStart, d.animationScale),
                  angle, false, -1, fade * d.alpha * layerAlpha);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

float crumblingAnimationTime(Entity &e, float sceneAge) {
    // OnActivate 0x194a94 calls Actor::GetPosition (vtable +0x20), then
    // SetTime(fmod((position.y-position.x)/500, animation length)).
    // Actor::PreUpdate advances the instance even while SetOff hides it.
    Animation *a = animation(e.art, "on");
    float length = animationLength(a);
    float time = sceneAge + (e.originY - e.originX) / 500;
    if (length > 0) {
        time -= (int)(time / length) * length;
        if (time < 0)
            time += length;
    }
    return time;
}

float beanAnimationTime(Entity &e) {
    // TomPlayerBehaviour::OnUpdate 0x311a6c-0x311ad0 maps the middle
    // +/- half of the most recent launch velocity across the jump animation.
    float launch = e.lastLaunchSpeed > 0 ? e.lastLaunchSpeed : e.jumpPower;
    return clamp(0.5f - e.state.vy / maxf(1, launch), 0, 1) *
           animationLength(animation(e.art, "jump"));
}

int wizardPartZOffset(int part) {
    const int offsets[] = {-4, -2, 1, 2, 4};
    return offsets[part];
}

void drawWizardPart(Entity &e, int part, Level *scene = 0) {
    if (e.state.gone || (e.state.active == 4 && e.state.hp == 0))
        return;
    if (part == 2) {
        float x = e.state.x, y = e.state.y;
        drawActor(actor(106556998914888ULL), Rect{x - 53, y - 38, x + 43, y + 90},
                  wizardHeadAnimation(), wizardPresentation.animationAge, 0, false, -1,
                  secretLayerAlpha(e.layer, scene),
                  wizardPresentation.animation < 0 ? ANIMATION_LOOP : ANIMATION_STOP);
        return;
    }
    bool left = part >= 3;
    WizardArmPose pose =
        wizardArmPose(Vec{e.state.x, e.state.y}, left, 1, wizardArms[left ? 1 : 0]);
    drawActor(actor(part == 0 || part == 3 ? 761998489631ULL
                    : left                 ? 843976466165ULL
                                           : 866681511178ULL),
              wizardPartRect(part == 0 || part == 3 ? pose.sleeve : pose.hand), "", elapsed, 0,
              false, -1, secretLayerAlpha(e.layer, scene));
}

struct ProjectileDrawOrder {
    int layer, z;
};

ProjectileDrawOrder projectileDrawOrder(const Bullet &b) {
    Object &source = classes[b.klass];
    int owner = b.state.active - 1;
    bool boss = source.has("FinalBossBulletBehaviour");
    // Pre-version-nine boss shots lack an owner. Recover their layer without
    // changing the saved Bullet layout; the exact old firing hand is unknown.
    if ((owner < 0 || owner >= entityCount) && boss)
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == BOSS) {
                owner = i;
                break;
            }
    if (owner >= 0 && owner < entityCount) {
        int offset = -1;
        // EnemyBullet::Shoot 0x1bed38: emitter.z-1. Boss Shoot 0x1e9018:
        // HAND.z+1, with native right/left hand offsets -2/+4.
        if (boss)
            offset = b.state.hp == 1 ? -1 : b.state.hp == 2 ? 5 : 1;
        return ProjectileDrawOrder{entities[owner].layer, entities[owner].z + offset};
    }
    return ProjectileDrawOrder{playerIndex >= 0 ? entities[playerIndex].layer : 0, source.z};
}

void drawProjectile(const Bullet &b) {
    Object &o = classes[b.klass];
    Vec c = center(o.rect);
    Rect r = shifted(o.rect, b.state.x - c.x, b.state.y - c.y);
    drawActor(actor(o.id), shotDeathRectNative(b, r),
              o.has("FinalBossBulletBehaviour") ? "idle" : o.props.get("default-animation"),
              projectileAnimationAgeNative(b), b.state.angle, 0, -1,
              secretLayerAlpha(projectileDrawOrder(b).layer));
}

// Select entity-specific pose and animation clocks before rendering in the authored layer.
void drawEntity(Entity &e, Level *scene = 0, float cameraX = 0, float cameraY = 0,
                float sceneAge = 0, bool bossBodyOnly = false) {
    if (!e.source->visible || (e.state.gone && !killableDeathVisibleNative(e)) ||
        e.kind == SECRET || (endingScene.active && e.kind == PLAYER) ||
        (e.crumble && e.state.active == 2) ||
        (e.kind == BOSS && (e.state.active == 4 && e.state.hp == 0)) ||
        (e.kind == PLAYER && !scene && deathTimer > 0))
        return;
    float alpha = secretLayerAlpha(e.layer, scene);
    Level &map = scene ? *scene : current;
    if (!scene) {
        cameraX = camX;
        cameraY = camY;
        sceneAge = elapsed;
    }
    Rect r = killableDeathRectNative(e, entityRect(e));
    if (e.source->props.flag("maker-weather"))
        r = e.source->rect;
    if (e.source->has("FollowCameraBehaviour"))
        r = shifted(r, cameraX, cameraY);
    if (e.source->base == 5) {
        drawParticles(*e.source, e.art, r, e.state.timer, 1, alpha);
        return;
    }
    if (e.source->has("BackgroundBehaviour")) {
        Vec c = layerCameraPosition(map, e.layer, cameraX, cameraY);
        r = backgroundRect(*e.source, r, sceneAge, c.x - viewW * 0.5f, c.y - viewH * 0.5f);
    }
    const char *anim = e.source->props.get("default-animation");
    int frame = -1;
    bool flip = false;
    float animationTime = e.state.timer;
    AnimationEndAction endAction = ANIMATION_LOOP;
    if (e.kind == PLAYER) {
        if (e.state.active == 2)
            anim = "magnet";
        else {
            anim = "jump";
            animationTime = !scene && complete ? 0 : beanAnimationTime(e);
            endAction = ANIMATION_STOP;
        }
        flip = e.state.aux < 0;
    }
    if (e.kind == PLAYER && !scene && playerWalkMode) {
        bool moving = playerWalkMoving;
        anim = moving ? "emilio-walking" : "emilio-standing";
        animationTime = moving ? playerWalkAge : playerWalkIdleAge;
        endAction = ANIMATION_LOOP;
        float cy = (r.b + r.t) * 0.5f;
        r.b = cy + (r.b - cy) * 3;
        r.t = cy + (r.t - cy) * 3;
    }
    if (e.kind == SPIKE) {
        anim = spikeAnimationName(e);
        if (e.state.active == SPIKE_OPENING || e.state.active == SPIKE_CLOSING)
            endAction = ANIMATION_STOP;
    }
    if (endingScene.active && (e.source == entities[endingScene.emilio].source ||
                               e.source == entities[endingScene.maria].source)) {
        EndingTransformationPose pose =
            endingTransformationPose(endingScene.input, endingScene.age);
        bool human = e.source == entities[endingScene.emilio].source;
        anim = human ? pose.emilioAnimation : pose.mariaAnimation;
        animationTime =
            human ? (eq(anim, "walking") ? pose.emilioWalkAdvance : pose.emilioIdleAdvance)
                  : (eq(anim, "walking") ? pose.mariaWalkAdvance : pose.mariaIdleAdvance);
        flip = human || pose.mariaScaleX < 0;
    }
    if (e.kind == MAGNET) {
        anim = e.state.active == 2 ? "active" : "idle";
        animationTime = sceneAge;
    }
    if (e.kind == HELPER) {
        anim = helperAnimationNameNative(e);
        animationTime = helperAnimationAgeNative(e);
        flip = e.state.aux < 0;
    }
    if (e.kind == PERCH && e.state.active == 2) {
        anim = "ringing";
        endAction = ANIMATION_STOP;
    }
    if (e.kind == CHECKPOINT && checkpointSaveVisual(e)) {
        anim = "save";
        endAction = ANIMATION_STOP;
    }
    // PlatformBehaviour only installs a one-way fixture (native 0x2cde34).
    // Its authored default frame is the artwork; the cloud's "idle" is blank.
    if (e.kind == COIN)
        anim = coinAnimationNameNative(e);
    if (e.kind == SALAMANDER)
        anim = "idle";
    if (e.kind == ALTERNATOR)
        anim = activeBlock(e) ? "on" : "off";
    if (e.kind == SWITCH)
        anim = (((alternationBits & e.mask) != 0) != (e.alternation != 0)) ? "on" : "off";
    // BreakableBehaviour freezes the authored life animation at accumulated
    // damage rather than advancing it with wall-clock time (0x139e74/0x13ad50).
    if (e.breakable) {
        float hp = e.source->behavior("BreakableBehaviour")->num("hp", 3);
        animationTime =
            animationLength(animation(e.art, anim)) * maxf(0, hp - e.state.hp) / maxf(1, hp - 1);
        endAction = ANIMATION_STOP;
    }
    // CrumblingPlatform::SetOn chooses "on"; SetOff hides the actor and
    // explodes a separate ParticleEffect. Never stretch "bursting" into its box.
    if (e.crumble) {
        anim = "on";
        animationTime = crumblingAnimationTime(e, sceneAge);
    }
    if (e.kind == ENEMY) {
        if (e.walk)
            anim = e.state.aux < 0 ? "walk-left" : "walk-right";
        else if (e.source->has("EnemyJumperBehaviour"))
            anim = (!scene && jumperWaterStanding(e)) ? "standing"
                   : e.state.hp                       ? "jumping"
                                                      : "standing";
        else if (e.move)
            anim = oscillatorAnimationName(e.art, e.moveX, e.moveY, e.state.aux < 0, anim);
        else if (e.source->has("DiagonalBehaviour")) {
            anim = fishAnimationNameNative(e);
            animationTime = fishAnimationAgeNative(e);
        } else if (!e.deadly)
            anim = "idle";
    }
    if (hasJumperAnimationNative(e)) {
        if (killableDeathVisibleNative(e))
            anim = e.jumperAnimation.selected ? "jumping" : "standing";
        animationTime = jumperAnimationAgeNative(e, eq(anim, "jumping"));
    }
    if (e.kind == SPRING || e.kind == TRAMPOLINE || e.kind == EXIT || e.kind == BOUNCER) {
        const char *triggered = e.kind == BOUNCER ? "hit" : "bounce";
        Animation *a = animation(e.art, triggered);
        if (e.state.active == 2 && a && animationLength(a) > 0 &&
            e.state.timer < animationLength(a)) {
            anim = triggered;
            endAction = ANIMATION_STOP;
        } else if (e.kind == TRAMPOLINE || e.kind == BOUNCER)
            anim = "idle";
    }
    if (e.kind == BOSS && !bossBodyOnly) {
        drawWizardPart(e, 0, scene);
        drawWizardPart(e, 1, scene);
    }
    if (!scene)
        bulletSourceAnimationPose(e, &anim, &animationTime, &endAction);
    drawActor(e.art, r, anim, animationTime, e.state.angle, flip, frame, alpha, endAction);
    if (e.kind == BOSS && !bossBodyOnly) {
        drawWizardPart(e, 2, scene);
        drawWizardPart(e, 3, scene);
        drawWizardPart(e, 4, scene);
    }
}

// Render scene layers in order, sharing camera/parallax transforms between tiles and actors.
void drawWorld() {
    Vec foregroundShift = {};
    if (current.id == 976871008727ULL && current.layerCount > 6)
        foregroundShift = layerTranslation(current, 6, camX, camY);
    ProjectileDrawOrder shots[MAX_PROJECTILES];
    for (int i = 0; i < MAX_PROJECTILES; i++)
        if (bullets[i].life > 0)
            shots[i] = projectileDrawOrder(bullets[i]);
    for (int layer = 0; layer < current.layerCount; layer++) {
        Layer &l = current.layers[layer];
        if (!l.flags[1] || l.flags[2])
            continue;
        Vec shift = layerTranslation(current, layer, camX, camY);
        Rect bounds = {-shift.x, -shift.y, viewW - shift.x, viewH - shift.y};
        glPushMatrix();
        glTranslatef(shift.x, shift.y, 0);
        // Stable merge of tiles and sprites by z within each original layer.
        int lastZ = -2147483647;
        for (;;) {
            int nextZ = 2147483647;
            for (int i = 0; i < current.tileCount; i++)
                if (current.tiles[i].layer == layer && current.tiles[i].z > lastZ &&
                    current.tiles[i].z < nextZ)
                    nextZ = current.tiles[i].z;
            for (int i = 0; i < entityCount; i++)
                if (entities[i].layer == layer) {
                    if (entities[i].z > lastZ && entities[i].z < nextZ)
                        nextZ = entities[i].z;
                    if (entities[i].kind == BOSS)
                        for (int part = 0; part < 5; part++) {
                            int z = entities[i].z + wizardPartZOffset(part);
                            if (z > lastZ && z < nextZ)
                                nextZ = z;
                        }
                }
            for (int i = 0; i < MAX_PROJECTILES; i++)
                if (bullets[i].life > 0 && shots[i].layer == layer && shots[i].z > lastZ &&
                    shots[i].z < nextZ)
                    nextZ = shots[i].z;
            if (nextZ == 2147483647)
                break;
            for (int i = 0; i < current.tileCount; i++)
                if (current.tiles[i].layer == layer && current.tiles[i].z == nextZ)
                    drawTile(current.tiles[i], 0, &bounds, &foregroundShift);
            for (int i = 0; i < entityCount; i++)
                if (entities[i].layer == layer) {
                    if (entities[i].z == nextZ)
                        drawEntity(entities[i], 0, 0, 0, 0, true);
                    if (entities[i].kind == BOSS)
                        for (int part = 0; part < 5; part++)
                            if (entities[i].z + wizardPartZOffset(part) == nextZ)
                                drawWizardPart(entities[i], part);
                }
            for (int i = 0; i < MAX_PROJECTILES; i++)
                if (bullets[i].life > 0 && shots[i].layer == layer && shots[i].z == nextZ)
                    drawProjectile(bullets[i]);
            lastZ = nextZ;
        }
        drawEffects(0, layer);
        glPopMatrix();
    }
}
