#pragma once

// Regression coverage for eye native. Runs only in the isolated BQ_ENABLE_TESTS executable.

void eyeNativeChecks() {
    Object &source = *classById(17501560557495693516ULL);
    Actor *art = actor(source.id);
    Animation *face = exactEnemyAnimation(art, "");
    Animation *blank = exactEnemyAnimation(art, "idle");
    check(
        source.behaviorCount == 2 && source.has("BoxPhysicsBehaviour") &&
            source.has("DeadlyBehaviour") && face && face->count == 1 &&
            face->frames[0].texture >= 0 && blank && blank->count == 1 &&
            blank->frames[0].texture < 0,
        "Original 40px spiked block has a baked single-frame face and no separate eye tracking behavior");
    Entity block;
    initializeEntity(block, source, Rect{108, 172, 148, 212});
    block.state.timer = 0;
    cloudRenderBegin();
    drawEntity(block);
    u32 first = cloudRenderPixels(Rect{108, 172, 148, 212}, false);
    u32 visible = cloudRenderPixels(Rect{108, 172, 148, 212}, true);
    cloudRenderBegin();
    block.state.timer = 3;
    drawEntity(block);
    u32 later = cloudRenderPixels(Rect{108, 172, 148, 212}, false);
    check(
        visible > 100 && first == later,
        "Actual spiked-block artwork retains its original static red eyes instead of selecting transparent idle");
    Entity eye;
    initializeEntity(eye, *classById(4615236400452776625ULL), Rect{232, 168, 280, 216});
    cloudRenderBegin();
    eye.state.timer = 0;
    drawEntity(eye);
    u32 eyeFirst = cloudRenderPixels(Rect{232, 168, 280, 216}, false);
    cloudRenderBegin();
    eye.state.timer = 0.11f;
    drawEntity(eye);
    u32 eyeNext = cloudRenderPixels(Rect{232, 168, 280, 216}, false);
    check(eyeFirst != eyeNext,
          "The distinct round spike-eye enemy advances its original four-frame eye animation");
    Entity mega;
    initializeEntity(mega, *classById(481780730859ULL), Rect{336, 132, 456, 252});
    cloudRenderBegin();
    mega.state.timer = 0;
    drawEntity(mega);
    u32 megaFirst = cloudRenderPixels(Rect{336, 132, 456, 252}, false);
    cloudRenderBegin();
    mega.state.timer = 0.07f;
    drawEntity(mega);
    u32 megaNext = cloudRenderPixels(Rect{336, 132, 456, 252}, false);
    check(megaFirst != megaNext,
          "The separate mega killer block retains its authored two-frame eye animation");
    cloudRenderBegin();
    drawEntity(block);
    drawEntity(eye);
    drawEntity(mega);
    SwapBuffers(gameDC);
    capture("test-output/eye-native-original-classes.bmp");
}
