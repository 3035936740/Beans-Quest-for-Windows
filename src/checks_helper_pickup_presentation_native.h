#pragma once

// Regression coverage for helper pickup presentation native. Runs only in the isolated BQ_ENABLE_TESTS executable.
int helperPickupParticleCountNative(int domain, u64 effect) {
    int count = 0;
    for (int i = 0; i < MAX_EFFECT_PARTICLES; i++)
        if (effectParticles[i].definition && effectParticles[i].domain == domain &&
            effectParticles[i].art && effectParticles[i].art->id == effect)
            count++;
    return count;
}

void helperPickupPresentationNativeChecks() {
    const u64 stageIds[] = {669939580618ULL, 641851699177ULL};
    for (int stage = 0; stage < 2; stage++)
        for (int facing = -1; facing <= 1; facing += 2) {
            int index = helperAnimationCampaignPrepareNative(stageIds[stage]);
            if (index < 0) {
                check(false, "Authored helper stage resolves for actual pickup presentation");
                continue;
            }
            Entity &bird = entities[index];
            State &bean = entities[playerIndex].state;
            bean.x = bird.state.x;
            bean.y = bird.state.y;
            bean.aux = (float)facing;
            bean.vx = bean.vy = 0;
            restorePhysics();
            clearEffects(0);
            stopSounds();
            soundEnabled = true;
            u64 effect = idnum(bird.source->behavior("HelperBirdBehaviour")->get("effect"));
            int expected = (int)effectClass(effect)->props.num("explosion-size", 1);
            simulateBodies(1.0f / 120);
            int count = helperPickupParticleCountNative(0, effect);
            char label[240];
            wsprintfA(
                label,
                "Actual campaign helper pickup stage%d facing%d selects flight, emits its authored particle burst and one bird voice",
                stage, facing);
            check(bird.state.active == 1 && bird.state.aux == facing && count == expected &&
                      helperCollisionScopeBirdVoicesNative() == 1,
                  label);
            simulateBodies(1.0f / 120);
            helperPostStep(0);
            wsprintfA(
                label,
                "Campaign helper pickup stage%d facing%d does not repeat particles or audio during continued overlap",
                stage, facing);
            check(helperPickupParticleCountNative(0, effect) == expected &&
                      helperCollisionScopeBirdVoicesNative() == 1,
                  label);
            bean.aux = (float)-facing;
            helperPostStep(0);
            wsprintfA(
                label,
                "Campaign following helper stage%d facing%d tracks a later Bean facing change",
                stage, facing);
            check(bird.state.active == 1 && bird.state.aux == -facing, label);
            soundEnabled = false;
            stopSounds();
        }
    interfaceEnabled = true;
    musicEnabled = soundEnabled = false;
    startStage(0);
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    bool isolated = true;
    for (int facing = -1; facing <= 1; facing += 2)
        for (int muted = 0; muted < 2; muted++) {
            navigate(137502285691ULL);
            int index = -1;
            page.level.geometryCount = 0;
            for (int i = 0; i < page.playfieldCount; i++) {
                if (page.playfield[i].kind == HELPER)
                    index = i;
                else
                    page.playfield[i].state.gone = 1;
            }
            if (index < 0) {
                check(false, "Original ending helper resolves for actual pickup presentation");
                continue;
            }
            Entity &bird = page.playfield[index];
            page.bean.state.x = bird.state.x;
            page.bean.state.y = bird.state.y;
            page.bean.state.aux = (float)facing;
            page.bean.state.vx = page.bean.state.vy = 0;
            resetUiPhysics(page);
            clearEffects(1);
            stopSounds();
            soundEnabled = !muted;
            u64 effect = idnum(bird.source->behavior("HelperBirdBehaviour")->get("effect"));
            int expected = (int)effectClass(effect)->props.num("explosion-size", 1);
            int campaignParticles = helperPickupParticleCountNative(0, effect);
            stepUiPhysics(page, 1.0f / 120);
            int count = helperPickupParticleCountNative(1, effect);
            char label[260];
            wsprintfA(
                label,
                "Actual ending helper pickup facing%d muted%d selects flight with authored particles and the owning world's sound policy",
                facing, muted);
            check(bird.state.active == 1 && bird.state.aux == facing && count == expected &&
                      helperCollisionScopeBirdVoicesNative() == (muted ? 0 : 1),
                  label);
            stepUiPhysics(page, 1.0f / 120);
            creditsHelperPostStep(page, 0);
            wsprintfA(
                label,
                "Ending helper pickup facing%d muted%d cannot repeat its burst or voice on continued Bean contact",
                facing, muted);
            check(helperPickupParticleCountNative(1, effect) == expected &&
                      helperCollisionScopeBirdVoicesNative() == (muted ? 0 : 1),
                  label);
            page.bean.state.aux = (float)-facing;
            creditsHelperPostStep(page, 0);
            wsprintfA(label,
                      "Ending following helper facing%d muted%d follows later Bean facing changes",
                      facing, muted);
            check(bird.state.active == 1 && bird.state.aux == -facing, label);
            isolated &= campaignParticles == helperPickupParticleCountNative(0, effect) &&
                        campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
                        saved == hashBytes(&checkpoint, sizeof(checkpoint));
            soundEnabled = false;
            stopSounds();
        }
    check(
        isolated,
        "Ending helper pickup presentation owns domain1 particles and cannot change campaign actors, effects or checkpoint state");
    interfaceEnabled = musicEnabled = soundEnabled = false;
    startStage(0);
}
