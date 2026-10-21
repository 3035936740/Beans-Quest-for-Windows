// Isolated audit runner. It includes the shipped implementation without changing it.
#define BQ_ENABLE_TESTS 1
#define start unusedGameEntry
#include "../src/main.cpp"
#undef start

char report[20000];
int reproduced;

void observation(bool defect, const char *label, const char *detail) {
    lstrcatA(report, defect ? "CONFIRMED " : "NOT REPRODUCED ");
    lstrcatA(report, label);
    lstrcatA(report, "\r\n  ");
    lstrcatA(report, detail);
    lstrcatA(report, "\r\n");
    if (defect)
        reproduced++;
}

#include "recheck_more.h"

extern "C" void WINAPI start() {
    runConstructors();
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(0, path, MAX_PATH);
    for (int i = lstrlenW(path) - 1; i >= 0; i--)
        if (path[i] == '\\') {
            path[i] = 0;
            break;
        }
    lstrcatW(path, L"/../bin");
    if (!SetCurrentDirectoryW(path))
        ExitProcess(2);
    permanent.init(160 * 1024 * 1024);
    levelArena.init(32 * 1024 * 1024);
    initWindow(true);
    loadProject();
    CreateDirectoryA("test-output", 0);
    lstrcpyA(saveRoot, "test-output/recheck");
    CreateDirectoryA(saveRoot, 0);
    musicEnabled = soundEnabled = false;
    progress.unlocked = stageCount - 1;
    interfaceEnabled = false;
    char detail[512];
    startStage(7);
    int on = -1, off = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].source->has("AlternatorBlockBehaviour")) {
            if (entities[i].alternation && on < 0)
                on = i;
            if (!entities[i].alternation && off < 0)
                off = i;
        }
    if (on >= 0) {
        Entity &e = entities[on];
        wsprintfA(detail, "Alternation=%d, initial parity=%d, activeBlock=%d, body active=%d",
                  e.alternation, alternationBits, activeBlock(e),
                  bodies[on] ? bodies[on]->IsActive() : -1);
        observation(!activeBlock(e), "Initial on-block is inactive", detail);
    }
    if (off >= 0) {
        Entity &e = entities[off];
        wsprintfA(detail, "Alternation=%d, initial parity=%d, activeBlock=%d, body active=%d",
                  e.alternation, alternationBits, activeBlock(e),
                  bodies[off] ? bodies[off]->IsActive() : -1);
        observation(activeBlock(e), "Initial off-block is active", detail);
    }

    int toggle = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == SWITCH) {
            toggle = i;
            break;
        }
    if (toggle >= 0) {
        Entity &e = entities[toggle];
        entities[playerIndex].state.x = e.state.x;
        entities[playerIndex].state.y = e.state.y;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        e.state.timer = 1;
        tick(1 / 120.0f, false, false);
        int first = alternationBits;
        entities[playerIndex].state.x = 256;
        entities[playerIndex].state.y = 300;
        entities[playerIndex].state.vx = entities[playerIndex].state.vy = 0;
        tick(1 / 120.0f, false, false);
        entities[playerIndex].state.x = e.state.x;
        entities[playerIndex].state.y = e.state.y;
        e.state.timer = 1;
        tick(1 / 120.0f, false, false);
        int second = alternationBits;
        wsprintfA(detail,
                  "First touch parity=%d; second touch parity=%d (native DoAlternate toggles)",
                  first, second);
        observation(first == second && first != 0, "Second switch touch does not toggle back",
                    detail);
    }

    startStage(49);
    int bossIndex = -1;
    for (int i = 0; i < entityCount; i++)
        if (entities[i].kind == BOSS) {
            bossIndex = i;
            break;
        }
    if (bossIndex >= 0) {
        Entity &boss = entities[bossIndex];
        boss.state.x = 4000;
        boss.state.y = 165;
        boss.state.hp = 5;
        boss.state.timer = 0.72f;
        wizardMotion.pendingCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 3 && wizardMotion.pendingCount < 3)
                wizardMotion.pending[wizardMotion.pendingCount++] = i;
        wizardMaterializeSummons(boss);
        int bird = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 3 && !entities[i].state.gone) {
                bird = i;
                break;
            }
        if (bird >= 0) {
            float savedOrigin = entities[bird].originX;
            captureCheckpoint(true);
            readProgress();
            musicEnabled = soundEnabled = false;
            startStage(49, true);
            float savedX = entities[bird].state.x, loadedOrigin = entities[bird].originX;
            tick(1 / 120.0f, false, false);
            float nextX = entities[bird].state.x;
            wsprintfA(detail,
                      "Saved origin=%d; reloaded origin=%d; saved x=%d; x after one step=%d",
                      (int)savedOrigin, (int)loadedOrigin, (int)savedX, (int)nextX);
            observation(savedOrigin != loadedOrigin && absf(nextX - savedX) > 10,
                        "Checkpoint reload shifts summoned bird", detail);
        }
        boss.state.hp = 5;
        boss.state.x = 4000;
        boss.state.timer = 0.58f;
        wizardMotion.pendingCount = 0;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 2 && wizardMotion.pendingCount < 3)
                wizardMotion.pending[wizardMotion.pendingCount++] = i;
        wizardMaterializeSummons(boss);
        int walker = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].bossSpawn == 2 && !entities[i].state.gone) {
                walker = i;
                break;
            }
        if (walker >= 0) {
            Entity &e = entities[walker];
            const Props *walk = e.source->behavior("WalkBehaviour");
            float expected = walk ? walk->num("walk-speed") : 0;
            wsprintfA(detail, "Class walk-speed=%d; entity speed=%d; actual vx=%d", (int)expected,
                      (int)e.speed, (int)e.state.vx);
            observation(expected > 0 && e.speed == 0 && e.state.vx == 0,
                        "Summoned walking enemy has zero walk speed", detail);
        }
        u64 requested = idnum(current.props.get("level-select"));
        openSelect();
        char expectedId[24], actualId[24];
        idstr(requested, expectedId);
        idstr(page.level.id, actualId);
        wsprintfA(detail, "Authored level-select=%s; opened scene=%s", expectedId, actualId);
        observation(page.level.id != requested, "Wizard stage returns to the wrong selector",
                    detail);
    }
    moreRechecks();
    wsprintfA(detail, "Reproduced observations: %d (on/off block checks share one root cause)\r\n",
              reproduced);
    lstrcatA(report, detail);
    atomicWrite("test-output/recheck/results.txt", report, lstrlenA(report));
    stopMusic();
    ExitProcess(reproduced == 0 ? 0 : 3);
}
