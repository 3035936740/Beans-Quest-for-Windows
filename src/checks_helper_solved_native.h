#pragma once

// Regression coverage for helper solved native. Runs only in the isolated BQ_ENABLE_TESTS executable.
class HelperSolvedProbeNative : public OriginalContacts {
  public:
    int helper, obstacle, hits;

    HelperSolvedProbeNative(int h, int o) : helper(h), obstacle(o), hits(0) {}

    void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {
        b2Body *a = contact->GetFixtureA()->GetBody();
        b2Body *b = contact->GetFixtureB()->GetBody();
        if ((a == bodies[helper] && b == bodies[obstacle]) ||
            (b == bodies[helper] && a == bodies[obstacle]))
            hits++;
        OriginalContacts::PostSolve(contact, impulse);
    }
};

int helperSolvedPrepareNative(int &cloud) {
    int helper = helperAnimationCampaignPrepareNative(669939580618ULL);
    if (helper < 0)
        return -1;
    cloud = entityCount;
    addEntity(*classById(14815823507569313204ULL), Rect{240, 180, 272, 198});
    Entity &pad = entities[cloud];
    Entity &bird = entities[helper];
    Entity &bean = entities[playerIndex];
    bird.state.active = 1;
    bird.state.timer = .2f;
    bird.state.x = 256;
    bird.state.y = hitRect(pad).t - bird.hit.b + .5f;
    bird.state.vx = bird.state.vy = 0;
    bean.state.x = 256;
    bean.state.y = hitRect(pad).t - bean.hit.b + 1;
    bean.state.vx = 0;
    bean.state.vy = -120;
    rebuildPhysics();
    return helper;
}

void helperSolvedNativeChecks() {
    interfaceEnabled = debugMode = musicEnabled = soundEnabled = false;
    stopMusic();
    stopSounds();
    int cloud = -1, helper = helperSolvedPrepareNative(cloud);
    if (helper < 0) {
        check(false, "Original helper stage resolves for solved collision retention");
        return;
    }
    HelperSolvedProbeNative *probe = new HelperSolvedProbeNative(helper, cloud);
    physicsWorld->SetContactListener(probe);
    soundEnabled = true;
    u32 voices = audioVoicesStarted;
    simulateBodies(1.0f / 120);
    bool burst = entities[cloud].state.active == 2 && !bodies[cloud]->IsActive();
    bool solved = probe->hits > 0;
    helperPostStep(1.0f / 120);
    soundEnabled = false;
    check(
        solved && burst,
        "Actual Bean and helper solved contacts hit the same original cloud before its real burst removes both contacts");
    // A cloud emits its own sound; count the helper voice independently.
    AudioClip *clip = 0;
    for (int i = 0; i < soundClipCount; i++)
        if (eq(soundClips[i].name, "bird"))
            clip = &soundClips[i].clip;
    int birdVoices = 0;
    for (int i = 0; i < 32; i++)
        birdVoices += clip && audioVoices[i].clip == clip;
    check(
        solved && entities[helper].state.active == 2 && flyingHelper() < 0,
        "A helper solved collision still ends flight when Bean bursts its supporting cloud before helperPostStep");
    check(
        birdVoices == 1 && audioVoicesStarted == voices + 2,
        "The real cloud-removal collision emits one helper impact chirp in addition to the cloud sound");
    soundEnabled = true;
    helperPostStep(1.0f / 120);
    soundEnabled = false;
    int after = 0;
    for (int i = 0; i < 32; i++)
        after += clip && audioVoices[i].clip == clip;
    check(
        after == birdVoices,
        "Repeated helper post-step processing does not replay a retained collision after returning has begun");
    char trace[220];
    wsprintfA(trace, "solved=%d cloudBurst=%d helperState=%d birdVoices=%d totalVoices=%d\r\n",
              probe->hits, (int)burst, entities[helper].state.active, birdVoices,
              (int)(audioVoicesStarted - voices));
    atomicWrite("test-output/helper-solved-cloud.txt", trace, lstrlenA(trace));
    physicsWorld->SetContactListener(originalContacts);
    delete probe;
    stopSounds();
    helper = helperSolvedPrepareNative(cloud);
    captureCheckpoint(false);
    simulateBodies(1.0f / 120);
    restoreCheckpoint();
    voices = audioVoicesStarted;
    soundEnabled = true;
    helperPostStep(0);
    soundEnabled = false;
    check(
        entities[helper].state.active == 1 && audioVoicesStarted == voices,
        "Checkpoint restore clears an unconsumed helper collision so it cannot detach a newly restored flight or replay its sound");
    helper = helperSolvedPrepareNative(cloud);
    entities[cloud].state.gone = 1;
    int perch = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == PERCH) {
            perch = i;
            break;
        }
    bool sensor = false;
    if (perch >= 0) {
        entities[perch].state.gone = 0;
        entities[helper].state.x = entities[perch].state.x;
        entities[helper].state.y = entities[perch].state.y;
        entities[playerIndex].state.x = current.size.x * .5f;
        entities[playerIndex].state.y = 1000;
        rebuildPhysics();
        voices = audioVoicesStarted;
        soundEnabled = true;
        simulateBodies(1.0f / 120);
        helperPostStep(1.0f / 120);
        soundEnabled = false;
        sensor = entities[helper].state.active == 1 && audioVoicesStarted == voices;
    }
    check(
        sensor,
        "A real helper-perch sensor overlap neither records a solid impact nor ends flight or emits a chirp");
    stopSounds();
    u32 campaign = hashBytes(entities, sizeof(Entity) * entityCount),
        contacts = hashBytes(helperSolvedContactsNative, sizeof(helperSolvedContactsNative)),
        saved = hashBytes(&checkpoint, sizeof(checkpoint));
    interfaceEnabled = true;
    navigate(137502285691ULL);
    int endingHelper = -1;
    for (int i = 0; i < page.playfieldCount; i++) {
        if (page.playfield[i].kind == HELPER)
            endingHelper = i;
        else
            page.playfield[i].state.gone = 1;
    }
    bool independent = false;
    if (endingHelper >= 0 && page.playfieldCount < 511) {
        destroyUiPhysics(page);
        page.level.geometryCount = 0;
        int pad = page.playfieldCount++;
        initializeEntity(page.playfield[pad], *classById(14815823507569313204ULL),
                         Rect{240, 180, 272, 198});
        Entity &bird = page.playfield[endingHelper];
        bird.state.active = 1;
        bird.state.timer = .2f;
        bird.state.x = 264;
        bird.state.y = hitRect(page.playfield[pad]).t - bird.hit.b + .5f;
        bird.state.vx = bird.state.vy = 0;
        page.bean.state.x = 248;
        page.bean.state.y = hitRect(page.playfield[pad]).t - page.bean.hit.b + 1;
        page.bean.state.vx = 0;
        page.bean.state.vy = -120;
        createUiPhysics(page);
        stepUiPhysics(page, 1.0f / 120);
        bool actual = page.contacts->helperSolved[endingHelper] &&
                      page.playfield[pad].state.active == 2 && !page.bodies[pad]->IsActive();
        creditsHelperPostStep(page, 1.0f / 120);
        independent = actual && bird.state.active == 2;
    }
    check(
        independent && campaign == hashBytes(entities, sizeof(Entity) * entityCount) &&
            contacts == hashBytes(helperSolvedContactsNative, sizeof(helperSolvedContactsNative)) &&
            saved == hashBytes(&checkpoint, sizeof(checkpoint)),
        "Ending helper retains its own actual cloud-removal impact and ends flight without touching campaign actors, contact flags or saves");
    interfaceEnabled = false;
    startStage(0);
    readyTimer = 0;
}
