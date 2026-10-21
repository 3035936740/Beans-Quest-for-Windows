#pragma once

// Regression coverage for secret reference native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void secretReferenceNativeChecks(void (*verify)(bool, const char *) = check) {
    char previousRoot[MAX_PATH];
    lstrcpyA(previousRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/secret-reference-saves");
    CreateDirectoryA(saveRoot, 0);
    Progress oldProgress = progress;
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldMusic = musicEnabled,
         oldSound = soundEnabled;
    interfaceEnabled = musicEnabled = soundEnabled = false;
    debugMode = true;
    startStage(stageById(1897451970646878057ULL));
    readyTimer = 0;
    readyOpening.showing = false;
    int a = -1, b = -1;
    for (int i = 0; i < entityCount; i++) {
        if (entities[i].source->id == 16212838492312001151ULL)
            a = i;
        if (entities[i].source->id == 4727802147503249030ULL)
            b = i;
    }
    bool authored = a >= 0 && b >= 0 && playerIndex >= 0;
    verify(
        authored,
        "The original Secret Passage scene supplies its two adjacent shared-layer markers and Tom");
    if (authored) {
        Entity samples[3] = {entities[a], entities[b], entities[playerIndex]};
        b2Body *sampleBodies[3] = {};
        b2World *world = new b2World(b2Vec2(0, 0), true);
        sampleBodies[0] = createNativeSecretBody(*world, samples[0], 0);
        sampleBodies[1] = createNativeSecretBody(*world, samples[1], 1);
        sampleBodies[2] = createPhysicsEntity(*world, samples[2], 2);
        b2Fixture *fixture = sampleBodies[0]->GetFixtureList();
        const b2PolygonShape *polygon = (const b2PolygonShape *)fixture->GetShape();
        b2Vec2 low = sampleBodies[0]->GetWorldPoint(polygon->m_vertices[0]),
               high = sampleBodies[0]->GetWorldPoint(polygon->m_vertices[2]);
        verify(
            sampleBodies[0]->GetType() == b2_staticBody && fixture->IsSensor() &&
                fixture->GetDensity() == 0 && absf(fixture->GetFriction() - .2f) < .00001f &&
                fixture->GetFilterData().categoryBits == 4 &&
                fixture->GetFilterData().maskBits == 0xffff &&
                absf(low.x / metres - 1536) < .001f && absf(low.y / metres - 60) < .001f &&
                absf(high.x / metres - 1632) < .001f && absf(high.y / metres - 220) < .001f,
            "Secret sensors preserve the native static Box fixture, full authored bounds and mapped filter/material");
        sampleBodies[2]->SetTransform(b2Vec2(1632 * metres, 110 * metres), 0);
        sampleBodies[2]->SetLinearVelocity(b2Vec2(0, 0));
        world->Step(1.0f / 120, 10, 10);
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, .1f);
        verify(
            nativeSecretContacts(sampleBodies[0], sampleBodies[2]) == 1 &&
                nativeSecretContacts(sampleBodies[1], sampleBodies[2]) == 1 &&
                absf(samples[0].state.aux - .4f) < .00001f &&
                samples[1].state.aux == samples[0].state.aux,
            "Two simultaneously contacted markers advance their single shared layer once at the native four-per-second rate");
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, .15f);
        sampleBodies[2]->SetTransform(b2Vec2(1648 * metres, 110 * metres), 0);
        world->Step(1.0f / 120, 10, 10);
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, .05f);
        verify(
            nativeSecretContacts(sampleBodies[0], sampleBodies[2]) == 0 &&
                nativeSecretContacts(sampleBodies[1], sampleBodies[2]) == 1 &&
                samples[0].state.aux == 1 && samples[1].state.aux == 1,
            "Leaving the first adjacent marker keeps its layer fully hidden while Tom still touches the second");
        sampleBodies[2]->SetTransform(b2Vec2(2100 * metres, 110 * metres), 0);
        world->Step(1.0f / 120, 10, 10);
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, .1f);
        bool halfway = absf(samples[0].state.aux - .6f) < .00001f;
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, .15f);
        verify(
            halfway && samples[0].state.aux == 0 && samples[1].state.aux == 0,
            "Only leaving the final marker reverses the shared fade continuously and restores opacity in one quarter second");
        sampleBodies[2]->SetTransform(b2Vec2(1527.5f * metres, 110 * metres), 0);
        world->Step(1.0f / 120, 10, 10);
        samples[2].state.x = 1527.5f;
        samples[2].state.y = 110;
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, 1.0f / 120);
        verify(
            !overlaps(hitRect(samples[2]), entityRect(samples[0])) &&
                nativeSecretContacts(sampleBodies[0], sampleBodies[2]) == 1 &&
                samples[0].state.aux > 0,
            "A real Box2D sensor contact at the polygon skin boundary reveals the layer even when the old rectangular overlap misses it");
        sampleBodies[2]->SetTransform(b2Vec2(2100 * metres, 110 * metres), 0);
        b2BodyDef otherDefinition;
        otherDefinition.type = b2_dynamicBody;
        otherDefinition.position.Set(1600 * metres, 110 * metres);
        b2Body *other = world->CreateBody(&otherDefinition);
        b2PolygonShape shape;
        shape.SetAsBox(8 * metres, 10 * metres);
        other->CreateFixture(&shape, 1);
        world->Step(1.0f / 120, 10, 10);
        samples[0].state.aux = samples[1].state.aux = 0;
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, .1f);
        verify(
            samples[0].state.aux == 0 && samples[1].state.aux == 0 &&
                sampleBodies[0]->GetContactList() != 0,
            "Contacts from another dynamic body cannot acquire Tom's shared secret-layer reference");
        samples[0].state.aux = .25f;
        samples[1].state.aux = .75f;
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, 0);
        bool migrated = samples[0].state.aux == .75f && samples[1].state.aux == .75f;
        advanceNativeSecretLayers(samples, 3, sampleBodies, 2, .05f);
        verify(
            migrated && absf(samples[0].state.aux - .55f) < .00001f &&
                samples[0].state.aux == samples[1].state.aux,
            "An older checkpoint's separate fades migrate to its previously rendered maximum and remain synchronized without new save fields");
        delete world;
        Entity &tom = entities[playerIndex];
        tom.state.x = 1632;
        tom.state.y = 110;
        tom.state.vx = tom.state.vy = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == SECRET)
                entities[i].state.aux = .6f;
        restorePhysics();
        restorePhysics();
        simulateBodies(1.0f / 120);
        advanceNativeSecretLayers(entities, entityCount, bodies, playerIndex, 1.0f / 120);
        verify(
            bodies[a] && bodies[b] && nativeSecretContacts(bodies[a], bodies[playerIndex]) == 1 &&
                nativeSecretContacts(bodies[b], bodies[playerIndex]) == 1 &&
                absf(entities[a].state.aux - (.6f + 4.0f / 120)) < .00001f &&
                entities[a].state.aux == entities[b].state.aux,
            "Repeated production physics restores rebuild real marker contacts without losing fade or applying duplicate reference updates");
    }
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, previousRoot);
    progress = oldProgress;
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    musicEnabled = oldMusic;
    soundEnabled = oldSound;
}
