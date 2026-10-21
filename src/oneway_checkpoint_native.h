#pragma once

// Reconstructs saved one-way entry decisions without serializing contact or body pointers.

int oneWayCheckpointBodyKeyNative(b2Body *body) {
    for (int i = 0; i < entityCount; i++) {
        if (bodies[i] == body)
            return i;
        if (oscillatorProxies[i] == body)
            return MAX_ENTITIES + MAX_PROJECTILES + i;
    }
    return -1;
}

int oneWayCheckpointFixtureKeyNative(b2Fixture *fixture, int &owner) {
    owner = (int)fixture->GetBody()->GetUserData() - 1;
    int ordinal = 0;
    if (owner >= 0) {
        for (b2Fixture *f = fixture->GetBody()->GetFixtureList(); f; f = f->GetNext(), ordinal++)
            if (f == fixture)
                return ordinal;
    } else
        for (b2Body *b = physicsWorld->GetBodyList(); b; b = b->GetNext())
            if (!b->GetUserData())
                for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext())
                    if (f->GetUserData() == (void *)1) {
                        if (f == fixture)
                            return ordinal;
                        ordinal++;
                    }
    return -1;
}

b2Fixture *oneWayCheckpointFixtureNative(int owner, int ordinal) {
    if (owner >= 0) {
        if (owner >= entityCount || !bodies[owner])
            return 0;
        for (b2Fixture *f = bodies[owner]->GetFixtureList(); f; f = f->GetNext())
            if (ordinal-- == 0)
                return f;
    } else if (owner == -1 && physicsWorld)
        for (b2Body *b = physicsWorld->GetBodyList(); b; b = b->GetNext())
            if (!b->GetUserData())
                for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext())
                    if (f->GetUserData() == (void *)1 && ordinal-- == 0)
                        return f;
    return 0;
}

void appendOneWaySavedPairNative(b2Fixture *platform, b2Body *body) {
    OneWayCheckpointNative &saved = oneWayCheckpointNative;
    int owner = -1, fixture = oneWayCheckpointFixtureKeyNative(platform, owner),
        other = oneWayCheckpointBodyKeyNative(body);
    if (fixture < 0 || other < 0)
        return;
    bool duplicate = false;
    for (int n = 0; n < saved.count; n++)
        if (saved.pairs[n].platform == owner && saved.pairs[n].fixture == fixture &&
            saved.pairs[n].other == other)
            duplicate = true;
    if (!duplicate && saved.count < 4096)
        saved.pairs[saved.count++] = OneWaySavedPairNative{owner, fixture, other};
}

void captureOneWayCheckpointNative(u32 snapshot) {
    OneWayCheckpointNative &saved = oneWayCheckpointNative;
    memset(&saved, 0, sizeof(saved));
    saved.magic = 0x57535142;
    saved.version = 1;
    saved.snapshot = snapshot;
    for (int i = 0; i < oneWayEntryLimit; i++) {
        OneWayEntry &e = oneWayEntries[i];
        if (e.contact && e.world == physicsWorld && !e.above)
            appendOneWaySavedPairNative(e.platform, e.other);
    }
    for (int i = 0; i < oneWayRestoredRefusalCountNative; i++) {
        OneWayRestoredRefusalNative &p = oneWayRestoredRefusalsNative[i];
        if (p.platform && p.world == physicsWorld)
            appendOneWaySavedPairNative(p.platform, p.other);
    }
    saved.checksum = hashBytes(&saved, sizeof(saved) - 4);
}

bool oneWayCheckpointPathNative(const char *path, char *extension) {
    if (lstrlenA(path) >= MAX_PATH - 8)
        return false;
    lstrcpyA(extension, path);
    lstrcatA(extension, ".oneway");
    return true;
}

bool writeOneWayCheckpointNative(const char *path) {
    char extension[MAX_PATH];
    return oneWayCheckpointPathNative(path, extension) &&
           atomicWrite(extension, &oneWayCheckpointNative, sizeof(oneWayCheckpointNative));
}

void eraseCheckpointFilesNative(const char *path) {
    DeleteFileA(path);
    char extension[MAX_PATH];
    if (oneWayCheckpointPathNative(path, extension))
        DeleteFileA(extension);
    memset(&oneWayCheckpointNative, 0, sizeof(oneWayCheckpointNative));
}

bool validOneWayCheckpointNative(const OneWayCheckpointNative &s, u32 snapshot) {
    if (s.magic != 0x57535142 || s.version != 1 || s.snapshot != snapshot || s.count < 0 ||
        s.count > 4096 || s.checksum != hashBytes(&s, sizeof(s) - 4))
        return false;
    for (int i = 0; i < s.count; i++) {
        const OneWaySavedPairNative &p = s.pairs[i];
        if (p.platform < -1 || p.platform >= checkpoint.count || p.fixture < 0 ||
            p.fixture > 100000 || p.other < 0 || p.other >= NATIVE_PHYSICS_CHECKPOINT_BODIES)
            return false;
    }
    return true;
}

bool readOneWayCheckpointNative(const char *path, u32 snapshot) {
    memset(&oneWayCheckpointNative, 0, sizeof(oneWayCheckpointNative));
    char extension[MAX_PATH];
    if (oneWayCheckpointPathNative(path, extension) &&
        exactRead(extension, &oneWayCheckpointNative, sizeof(oneWayCheckpointNative)) &&
        validOneWayCheckpointNative(oneWayCheckpointNative, snapshot))
        return true;
    memset(&oneWayCheckpointNative, 0, sizeof(oneWayCheckpointNative));
    return false;
}

void restoreOneWayCheckpointNative(u32 snapshot) {
    if (validOneWayCheckpointNative(oneWayCheckpointNative, snapshot)) {
        for (int i = 0; i < oneWayCheckpointNative.count; i++) {
            const OneWaySavedPairNative &p = oneWayCheckpointNative.pairs[i];
            b2Fixture *platform = oneWayCheckpointFixtureNative(p.platform, p.fixture);
            b2Body *other = checkpointBodyNative(p.other);
            if (platform && other)
                seedOneWayRefusalNative(platform, other);
        }
        return;
    }
    // Old saves did not store the one-way refusal. Recover only an already
    // intersecting following bird while Tom is rising through that surface.
    // Solid walls, descending actors and birds fully above remain unchanged.
    if (playerIndex < 0 || entities[playerIndex].state.vy <= 0)
        return;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == HELPER && entities[i].state.active == 1 &&
            !entities[i].state.gone && bodies[i]) {
            for (b2Body *b = physicsWorld->GetBodyList(); b; b = b->GetNext())
                for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext())
                    if (f->GetUserData() == (void *)1 || f->GetUserData() == (void *)2) {
                        b2Vec2 a, c;
                        if (f->GetUserData() == (void *)1) {
                            if (!terrainOneWaySegment(f, &a, &c))
                                continue;
                        } else {
                            int own = (int)b->GetUserData() - 1;
                            if (own < 0 || own >= entityCount)
                                continue;
                            Rect r = boxGroupMatrix(entities[own])
                                         ? boxGroupMatrixFixture(entities[own])
                                         : entities[own].hit;
                            a = b->GetPosition() + b2Vec2(r.l * metres, r.t * metres);
                            c = b->GetPosition() + b2Vec2(r.r * metres, r.t * metres);
                        }
                        b2Vec2 tom(entities[playerIndex].state.x * metres,
                                   entities[playerIndex].state.y * metres);
                        if (!nativeOneWayAbove(a, c, tom))
                            seedOneWayRefusalNative(f, bodies[i]);
                    }
        }
}
