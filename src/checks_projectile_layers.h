#pragma once

// Regression coverage for projectile layers. Runs only in the isolated BQ_ENABLE_TESTS executable.

void projectileLayerChecks() {
    interfaceEnabled = false;
    debugMode = false;
    musicEnabled = soundEnabled = false;
    startStage(stageById(789619714186ULL));
    readyTimer = 0;
    clearEffects();
    int emitter = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->klass == 303150949257ULL) {
            emitter = i;
            break;
        }
    check(emitter >= 0,
          "Original Blaster Climb rightward fire emitter resolves for layer-order regression");
    if (emitter < 0)
        return;
    Entity &cannon = entities[emitter];
    const Props &source = *cannon.source->behavior("BulletSource");
    shoot(cannon, source);
    int slot = -1;
    for (int i = 0; i < 64; i++)
        if (bullets[i].life > 0) {
            slot = i;
            break;
        }
    check(slot >= 0,
          "Actual original fire emitter creates its projectile with a persisted emitter reference");
    if (slot < 0)
        return;
    Bullet &shot = bullets[slot];
    ProjectileDrawOrder order = projectileDrawOrder(shot);
    check(
        order.layer == cannon.layer && order.z == cannon.z - 1,
        "Original EnemyBullet Shoot places fire in its emitter layer exactly one z index behind the muzzle");
    // Keep the actual emitter/fire artwork and frame. Isolate its authored
    // layer so a complete output hash compares the two real overlap orders.
    Tile originalTile = current.tiles[0];
    current.tileCount = 0;
    for (int i = 0; i < entityCount; i++)
        if (i != emitter)
            entities[i].state.gone = 1;
    for (int i = 0; i < current.layerCount; i++)
        current.layers[i].flags[1] = i == cannon.layer;
    camX = cannon.state.x - 256;
    camY = cannon.state.y - 192;
    Vec translation = layerTranslation(current, cannon.layer, camX, camY);
    cloudRenderBegin();
    drawWorld();
    u32 actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    glPushMatrix();
    glTranslatef(translation.x, translation.y, 0);
    drawProjectile(shot);
    drawEntity(cannon);
    glPopMatrix();
    u32 behind = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    glPushMatrix();
    glTranslatef(translation.x, translation.y, 0);
    drawEntity(cannon);
    drawProjectile(shot);
    glPopMatrix();
    u32 over = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    check(
        actual == behind && actual != over,
        "Actual OpenGL fire-at-spawn pixels match projectile-before-cannon and reject the former muzzle-covering order");
    cloudRenderBegin();
    drawWorld();
    SwapBuffers(gameDC);
    capture("test-output/fireball-behind-muzzle.bmp");
    shot.state.x += 48;
    cloudRenderBegin();
    drawWorld();
    actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    glPushMatrix();
    glTranslatef(translation.x, translation.y, 0);
    drawProjectile(shot);
    drawEntity(cannon);
    glPopMatrix();
    behind = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    check(actual == behind,
          "Fire becomes visible outside the muzzle without changing its original layer or z index");
    current.layers[cannon.layer].scroll = Vec{0.5f, 0.75f};
    camX += 37;
    camY += 29;
    translation = layerTranslation(current, cannon.layer, camX, camY);
    cloudRenderBegin();
    drawWorld();
    actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    glPushMatrix();
    glTranslatef(translation.x, translation.y, 0);
    drawProjectile(shot);
    drawEntity(cannon);
    glPopMatrix();
    behind = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    check(
        actual == behind,
        "Projectile consumes its original layer's actual parallax and pixel-rounded camera transform");
    current.layers[cannon.layer].flags[1] = 0;
    cloudRenderBegin();
    drawWorld();
    actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    u32 empty = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    check(
        actual == empty,
        "A hidden emitter layer also hides its live projectile instead of drawing it in a global foreground pass");
    current.layers[cannon.layer].flags[1] = 1;
    current.layers[cannon.layer].flags[2] = 1;
    cloudRenderBegin();
    drawWorld();
    actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    // The port partitions screen-centred HUD layers separately; this is not
    // a native visibility flag and is deliberately not claimed as parity.
    (void)actual;
    current.layers[cannon.layer].flags[2] = 0;
    // A real atlas tile in front must also participate in the same z merge.
    Tile cover = originalTile;
    Vertex vertices[4];
    for (int i = 0; i < 4; i++)
        vertices[i] = originalTile.vertices[i];
    Rect r = {shot.state.x - 24, shot.state.y - 24, shot.state.x + 24, shot.state.y + 24};
    vertices[0].x = vertices[2].x = r.l;
    vertices[1].x = vertices[3].x = r.r;
    vertices[0].y = vertices[1].y = r.b;
    vertices[2].y = vertices[3].y = r.t;
    cover.layer = cannon.layer;
    cover.z = cannon.z + 2;
    cover.vertices = vertices;
    cover.bytes = 64;
    cover.count = 4;
    current.tiles = &cover;
    current.tileCount = 1;
    cloudRenderBegin();
    drawWorld();
    actual = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    cloudRenderBegin();
    glPushMatrix();
    glTranslatef(translation.x, translation.y, 0);
    drawProjectile(shot);
    drawEntity(cannon);
    drawTile(cover);
    glPopMatrix();
    behind = cloudRenderPixels(Rect{0, 0, 512, 384}, false);
    check(actual == behind,
          "Foreground terrain tiles draw after the projectile according to native layer z order");
    current.tileCount = 0;
    captureCheckpoint(false);
    int owner = shot.state.active;
    shot.state.active = 0;
    restoreCheckpoint();
    order = projectileDrawOrder(bullets[slot]);
    check(
        bullets[slot].state.active == owner && order.layer == cannon.layer &&
            order.z == cannon.z - 1,
        "Existing checkpoint layout preserves the fire emitter reference and therefore its native draw order");
    startStage(49);
    readyTimer = 0;
    int boss = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS)
            boss = i;
    if (boss >= 0) {
        Entity &wizard = entities[boss];
        wizard.state.x = 4000;
        wizard.state.y = 250;
        entities[playerIndex].state.x = 3800;
        entities[playerIndex].state.y = 100;
        for (int hand = 0; hand < 2; hand++) {
            memset(bullets, 0, sizeof(bullets));
            wizardShot(wizard, hand ? 1.0f : -1.0f);
            int found = -1;
            for (int i = 0; i < 64; i++)
                if (bullets[i].life > 0) {
                    found = i;
                    break;
                }
            ProjectileDrawOrder o =
                found >= 0 ? projectileDrawOrder(bullets[found]) : ProjectileDrawOrder{-1, -1};
            check(
                found >= 0 && o.layer == wizard.layer && o.z == wizard.z + (hand ? 5 : -1),
                hand
                    ? "Wizard left-hand shot follows native hand.z+1 above the front hand"
                    : "Wizard right-hand shot follows native hand.z+1 between the back hand and body");
        }
        check(
            wizardPartZOffset(0) == -4 && wizardPartZOffset(1) == -2 && wizardPartZOffset(2) == 1 &&
                wizardPartZOffset(3) == 2 && wizardPartZOffset(4) == 4,
            "World draw merge retains native Wizard sleeve, hand and head depths around its body");
    } else
        check(false, "Original Wizard exists for its distinct projectile z policy");
    startStage(0);
}
