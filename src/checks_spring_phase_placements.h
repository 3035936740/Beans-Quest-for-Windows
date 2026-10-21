#pragma once

// Regression coverage for spring phase placements. Runs only in the isolated BQ_ENABLE_TESTS executable.
// All119 authored campaign placements from the original resource inventory.
void springPhasePlacementChecks() {
    const u64 pads[][2] = {{6838399004045653085ULL, 14207676943298465026ULL},
                           {1897451970646878057ULL, 7741631691555791504ULL},
                           {5100914154076244783ULL, 4910979416186904927ULL},
                           {5100914154076244783ULL, 13494570253222860569ULL},
                           {5100914154076244783ULL, 239547265202235845ULL},
                           {1066389851804853660ULL, 15192085006401416814ULL},
                           {11666231891817675739ULL, 11796434861316029058ULL},
                           {11666231891817675739ULL, 5065443265120313715ULL},
                           {11666231891817675739ULL, 4893794901663630889ULL},
                           {4412182657474862604ULL, 16004421696729731187ULL},
                           {4412182657474862604ULL, 1444704288017196508ULL},
                           {4412182657474862604ULL, 9239099314635913337ULL},
                           {4412182657474862604ULL, 11211262927076078539ULL},
                           {1065014185637269516ULL, 7368227478688128454ULL},
                           {1065014185637269516ULL, 63372970572338525ULL},
                           {1065014185637269516ULL, 13176868847997290349ULL},
                           {1065014185637269516ULL, 16118555957327262004ULL},
                           {1065014185637269516ULL, 17738570044272060244ULL},
                           {1065014185637269516ULL, 10752967969666778148ULL},
                           {1065014185637269516ULL, 3248865363654166069ULL},
                           {11491836511846746094ULL, 6308138685501775017ULL},
                           {11491836511846746094ULL, 156581441352662047ULL},
                           {11491836511846746094ULL, 16490441126486112619ULL},
                           {10756198405680061017ULL, 13663031745710850277ULL},
                           {10756198405680061017ULL, 5644105969028023452ULL},
                           {17830145214697761354ULL, 711907599915ULL},
                           {17830145214697761354ULL, 7570760192072728ULL},
                           {17830145214697761354ULL, 7280563105270492236ULL},
                           {17830145214697761354ULL, 4716174639663492275ULL},
                           {3682577195148464396ULL, 2196045137714998226ULL},
                           {3682577195148464396ULL, 8368806636475536559ULL},
                           {3682577195148464396ULL, 10873390957299197087ULL},
                           {5687107557475985288ULL, 16633584729095261427ULL},
                           {5687107557475985288ULL, 15525415029538673513ULL},
                           {13052395652604067555ULL, 15501671823532224794ULL},
                           {13052395652604067555ULL, 3842285668803889461ULL},
                           {13052395652604067555ULL, 15612887706699745889ULL},
                           {3442460187460865148ULL, 2782605271358550010ULL},
                           {3442460187460865148ULL, 5061947983816653801ULL},
                           {3442460187460865148ULL, 18020630334907325156ULL},
                           {3442460187460865148ULL, 16454603072873308044ULL},
                           {3442460187460865148ULL, 894055974221238378ULL},
                           {3442460187460865148ULL, 13903375779594791585ULL},
                           {3442460187460865148ULL, 9460795460870609120ULL},
                           {3442460187460865148ULL, 4397662579757403516ULL},
                           {3442460187460865148ULL, 14284563318777515994ULL},
                           {3442460187460865148ULL, 7451753394747595973ULL},
                           {3442460187460865148ULL, 5712849967649655291ULL},
                           {191561700160ULL, 11110340023ULL},
                           {878686228960ULL, 519968791828ULL},
                           {878686228960ULL, 806500260898ULL},
                           {878686228960ULL, 933581386417ULL},
                           {878686228960ULL, 648839470984ULL},
                           {641851699177ULL, 177088006412ULL},
                           {641851699177ULL, 627870581536ULL},
                           {641851699177ULL, 780625376805ULL},
                           {161899780601ULL, 430491467093ULL},
                           {161899780601ULL, 916657680069ULL},
                           {161899780601ULL, 537791059179ULL},
                           {161899780601ULL, 552568591680ULL},
                           {899263731133ULL, 891603904289ULL},
                           {899263731133ULL, 549264571810ULL},
                           {899263731133ULL, 96751653710ULL},
                           {297352608196ULL, 327906688980ULL},
                           {297352608196ULL, 99803918772ULL},
                           {297352608196ULL, 171109099877ULL},
                           {31661610418ULL, 6738048630064121ULL},
                           {666279532945ULL, 851998581308ULL},
                           {666279532945ULL, 635181417841ULL},
                           {666279532945ULL, 666899096678ULL},
                           {666279532945ULL, 211560402226ULL},
                           {666279532945ULL, 3091871810ULL},
                           {176011402805ULL, 27019916460ULL},
                           {419687582476ULL, 59161866611ULL},
                           {900179179108ULL, 21843506666ULL},
                           {418098865551ULL, 2080994793487812ULL},
                           {685488901231ULL, 907677758652ULL},
                           {685488901231ULL, 868410707499ULL},
                           {685488901231ULL, 5491310075371810ULL},
                           {685488901231ULL, 600789863660ULL},
                           {806701038719ULL, 3752001601804885ULL},
                           {787600693931ULL, 87393363941ULL},
                           {787600693931ULL, 601608388982ULL},
                           {787600693931ULL, 840667217603ULL},
                           {116499217023ULL, 169477971173ULL},
                           {116499217023ULL, 784616769986ULL},
                           {116499217023ULL, 859457817095ULL},
                           {707197359596ULL, 499700067764ULL},
                           {707197359596ULL, 687156778769ULL},
                           {707197359596ULL, 1779136046717616ULL},
                           {789619714186ULL, 943745010853ULL},
                           {789619714186ULL, 6391973698605474ULL},
                           {739118707375ULL, 608722069676ULL},
                           {976871008727ULL, 8166759961929897ULL},
                           {976871008727ULL, 6470982623680852ULL},
                           {976871008727ULL, 6748588149945181ULL},
                           {976871008727ULL, 9961717549391179ULL},
                           {7138745870511843ULL, 133519227830890ULL},
                           {7138745870511843ULL, 2091787059306300ULL},
                           {7138745870511843ULL, 8146484696522784ULL},
                           {7138745870511843ULL, 9949961014171160ULL},
                           {7138745870511843ULL, 4144859578138970ULL},
                           {7138745870511843ULL, 2039030688427487ULL},
                           {881271676947ULL, 880229811191ULL},
                           {881271676947ULL, 730565891702ULL},
                           {2986143812017068ULL, 9378785931199970ULL},
                           {2986143812017068ULL, 5750400783969189ULL},
                           {1966799885083198ULL, 5409579868690298ULL},
                           {1966799885083198ULL, 6791674227071446ULL},
                           {1966799885083198ULL, 761173009952387ULL},
                           {1071369106910786ULL, 7927897418486883ULL},
                           {1071369106910786ULL, 5136626110891109ULL},
                           {1071369106910786ULL, 8165766672908651ULL},
                           {1071369106910786ULL, 93957846226429ULL},
                           {1071369106910786ULL, 9170727071676771ULL},
                           {5679669680221079ULL, 9118800898170110ULL},
                           {5679669680221079ULL, 7938093896111488ULL},
                           {5679669680221079ULL, 9431807191726899ULL},
                           {5679669680221079ULL, 7885796590987663ULL}};
    char oldRoot[MAX_PATH];
    lstrcpyA(oldRoot, saveRoot);
    lstrcpyA(saveRoot, "test-output/spring-placement-saves");
    CreateDirectoryA(saveRoot, 0);
    bool oldInterface = interfaceEnabled, oldDebug = debugMode, oldSound = soundEnabled,
         oldMusic = musicEnabled;
    interfaceEnabled = debugMode = musicEnabled = false;
    soundEnabled = true;
    stopMusic();
    char rows[48000] = {};
    for (int n = 0; n < 119; n++) {
        startStage(stageById(pads[n][0]));
        readyTimer = deathTimer = 0;
        readyOpening.showing = false;
        current.geometryCount = 0;
        int pad = -1;
        for (int i = 0; i < entityCount; i++)
            if (entities[i].kind == SPRING && entities[i].source->id == pads[n][1] &&
                boxGroupMatrixOwner(entities[i])) {
                pad = i;
                break;
            }
        bool ok = false;
        if (pad >= 0) {
            Object *source = entities[pad].source;
            for (int i = 0; i < entityCount; i++)
                if (i != playerIndex && entities[i].source != source)
                    entities[i].state.gone = 1;
            Entity &e = entities[pad];
            Entity &bean = entities[playerIndex];
            Rect r = boxGroupMatrixFixture(e);
            bean.state.x = e.state.x + (r.l + r.r) * .5f;
            bean.state.y = e.state.y + r.t - bean.hit.b + .2f;
            bean.state.vx = 0;
            bean.state.vy = -30;
            bean.state.hp = 1;
            bean.state.timer = .1f;
            jumps = 0;
            rebuildPhysics();
            stopSounds();
            for (int f = 0; f < 40 && !pinballPendingNative.pending; f++)
                tick(1.0f / 120, false, false);
            PinballPendingNative request = pinballPendingNative;
            bool pending = request.pending && request.source == pad && jumps == 0 &&
                           pinballDetailVoiceCount("jump") == 0 &&
                           pinballDetailVoiceCount("megajump") == 1;
            tick(1.0f / 120, false, false);
            ok = pending && pinballLaunchedThisStepNative && jumps == 1 &&
                 absf(bean.lastLaunchSpeed - request.speed) < .001f &&
                 pinballDetailVoiceCount("jump") == 0 && pinballDetailVoiceCount("megajump") == 1;
        }
        char scene[24], id[24], row[450];
        idstr(pads[n][0], scene);
        idstr(pads[n][1], id);
        wsprintfA(
            row,
            "Campaign spring original scene=%s source=%s retains an independent real contact and one deferred launch",
            scene, id);
        check(ok, row);
        lstrcatA(rows, ok ? "PASS " : "FAIL ");
        lstrcatA(rows, row);
        lstrcatA(rows, "\r\n");
    }
    atomicWrite("test-output/spring-phase-all-placements.txt", rows, lstrlenA(rows));
    stopSounds();
    startStage(0);
    readyTimer = 0;
    lstrcpyA(saveRoot, oldRoot);
    interfaceEnabled = oldInterface;
    debugMode = oldDebug;
    soundEnabled = oldSound;
    musicEnabled = oldMusic;
}
