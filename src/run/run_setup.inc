    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--benchmark") == 0) {
            std::string reportPath = "benchmark_report.txt";
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                reportPath = argv[i + 1];
            }
            return Benchmark::runAll(reportPath) ? 0 : 1;
        }
    }

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::string verifyDir;
    bool profileFps = false;
    bool verifyAct1 = false;
    bool verifyAct2 = false;
    bool verifyAct3 = false;
    bool verifyAct4 = false;
    bool verifyEndless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verify-screenshots") == 0 && i + 1 < argc) {
            verifyDir = argv[++i];
        } else if (std::strcmp(argv[i], "--profile-fps") == 0) {
            profileFps = true;
        } else if (std::strcmp(argv[i], "--verify-act1") == 0) {
            verifyAct1 = true;
        } else if (std::strcmp(argv[i], "--verify-act2") == 0) {
            verifyAct2 = true;
        } else if (std::strcmp(argv[i], "--verify-act3") == 0) {
            verifyAct3 = true;
        } else if (std::strcmp(argv[i], "--verify-act4") == 0) {
            verifyAct4 = true;
        } else if (std::strcmp(argv[i], "--verify-endless") == 0) {
            verifyEndless = true;
        }
    }

    constexpr unsigned int W = SCREEN_WIDTH;
    constexpr unsigned int H = SCREEN_HEIGHT;
    sf::RenderWindow window(sf::VideoMode({W, H}), "Vault Heist: The Living Vault");
    int targetFps = 60;
    window.setFramerateLimit(static_cast<unsigned int>(targetFps));

    sf::Font font;
    bool fontLoaded = font.openFromFile("assets/fonts/arial.ttf") ||
                      font.openFromFile("../../assets/fonts/arial.ttf") ||
                      font.openFromFile("out/build/x64-debug/assets/fonts/arial.ttf") ||
                      font.openFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/fonts/arial.ttf") ||
                      font.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (!fontLoaded) std::cerr << "Warning: Arial Font could not be loaded!\n";

    sf::Font menuMonoFont;
    bool menuMonoLoaded = menuMonoFont.openFromFile("assets/fonts/Orbitron-Medium.ttf") ||
                          menuMonoFont.openFromFile("../../assets/fonts/Orbitron-Medium.ttf") ||
                          menuMonoFont.openFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/fonts/Orbitron-Medium.ttf") ||
                          menuMonoFont.openFromFile("C:/Windows/Fonts/consola.ttf") ||
                          menuMonoFont.openFromFile("C:/Windows/Fonts/cour.ttf");
    const sf::Font& menuFont = menuMonoLoaded ? menuMonoFont : font;

    sf::Texture vaultBgTex;
    bool vaultBgLoaded = vaultBgTex.loadFromFile("assets/images/vault_door_bg.png") ||
                         vaultBgTex.loadFromFile("../../assets/images/vault_door_bg.png") ||
                         vaultBgTex.loadFromFile("out/build/x64-debug/assets/images/vault_door_bg.png") ||
                         vaultBgTex.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/images/vault_door_bg.png");
    if (vaultBgLoaded) vaultBgTex.setSmooth(true);
    std::optional<sf::Sprite> vaultBgSprite;
    if (vaultBgLoaded) {
        vaultBgSprite.emplace(vaultBgTex);
        sf::Vector2u texSz = vaultBgTex.getSize();
        if (texSz.x > 0 && texSz.y > 0) {
            vaultBgSprite->setScale({
                static_cast<float>(W) / static_cast<float>(texSz.x),
                static_cast<float>(H) / static_cast<float>(texSz.y)
            });
        }
    }

    sf::Texture missionBgTex;
    bool missionBgLoaded = missionBgTex.loadFromFile("assets/images/mission_bg.png") ||
                           missionBgTex.loadFromFile("../../assets/images/mission_bg.png") ||
                           missionBgTex.loadFromFile("out/build/x64-debug/assets/images/mission_bg.png") ||
                           missionBgTex.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/images/mission_bg.png");
    if (missionBgLoaded) missionBgTex.setSmooth(true);
    std::optional<sf::Sprite> missionBgSprite;
    if (missionBgLoaded) {
        missionBgSprite.emplace(missionBgTex);
        sf::Vector2u mTexSz = missionBgTex.getSize();
        if (mTexSz.x > 0 && mTexSz.y > 0) {
            missionBgSprite->setScale({
                static_cast<float>(W) / static_cast<float>(mTexSz.x),
                static_cast<float>(H) / static_cast<float>(mTexSz.y)
            });
        }
    }

    sf::SoundBuffer successBuf, errorBuf, levelCompleteBuf, levelFailBuf, missionStartBuf, menuClickBuf, campaignClickBuf, escBuf, unlockLevelsBuf, lockLevelsBuf;
    bool successLoaded = successBuf.loadFromFile("assets/sounds/Success_correct-action.wav") ||
                         successBuf.loadFromFile("../../assets/sounds/Success_correct-action.wav") ||
                         successBuf.loadFromFile("out/build/x64-debug/assets/sounds/Success_correct-action.wav") ||
                         successBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/Success_correct-action.wav");
    bool errorLoaded = errorBuf.loadFromFile("assets/sounds/Wrong_error-action.wav") ||
                       errorBuf.loadFromFile("../../assets/sounds/Wrong_error-action.wav") ||
                       errorBuf.loadFromFile("out/build/x64-debug/assets/sounds/Wrong_error-action.wav") ||
                       errorBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/Wrong_error-action.wav");
    bool levelCompleteLoaded = levelCompleteBuf.loadFromFile("assets/sounds/Level_completevictory.mp3") ||
                               levelCompleteBuf.loadFromFile("../../assets/sounds/Level_completevictory.mp3") ||
                               levelCompleteBuf.loadFromFile("out/build/x64-debug/assets/sounds/Level_completevictory.mp3") ||
                               levelCompleteBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/Level_completevictory.mp3");
    bool levelFailLoaded = levelFailBuf.loadFromFile("assets/sounds/Level_failalarm.wav") ||
                           levelFailBuf.loadFromFile("../../assets/sounds/Level_failalarm.wav") ||
                           levelFailBuf.loadFromFile("out/build/x64-debug/assets/sounds/Level_failalarm.wav") ||
                           levelFailBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/Level_failalarm.wav");
    bool missionStartLoaded = missionStartBuf.loadFromFile("assets/sounds/starting_new_mission.wav") ||
                              missionStartBuf.loadFromFile("../../assets/sounds/starting_new_mission.wav") ||
                              missionStartBuf.loadFromFile("out/build/x64-debug/assets/sounds/starting_new_mission.wav") ||
                              missionStartBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/starting_new_mission.wav");
    bool menuClickLoaded = menuClickBuf.loadFromFile("assets/sounds/click_hover_sound_for_main_menu.mp3") ||
                           menuClickBuf.loadFromFile("../../assets/sounds/click_hover_sound_for_main_menu.mp3") ||
                           menuClickBuf.loadFromFile("out/build/x64-debug/assets/sounds/click_hover_sound_for_main_menu.mp3") ||
                           menuClickBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/click_hover_sound_for_main_menu.mp3");
    bool campaignClickLoaded = campaignClickBuf.loadFromFile("assets/sounds/click_hover_sound_in_acts_menu_and_levels.mp3") ||
                               campaignClickBuf.loadFromFile("../../assets/sounds/click_hover_sound_in_acts_menu_and_levels.mp3") ||
                               campaignClickBuf.loadFromFile("out/build/x64-debug/assets/sounds/click_hover_sound_in_acts_menu_and_levels.mp3") ||
                               campaignClickBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/click_hover_sound_in_acts_menu_and_levels.mp3");
    bool escLoaded = escBuf.loadFromFile("assets/sounds/ESC.mp3") ||
                     escBuf.loadFromFile("../../assets/sounds/ESC.mp3") ||
                     escBuf.loadFromFile("out/build/x64-debug/assets/sounds/ESC.mp3") ||
                     escBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/ESC.mp3");
    bool unlockLevelsLoaded = unlockLevelsBuf.loadFromFile("assets/sounds/unlock_levels.mp3") ||
                              unlockLevelsBuf.loadFromFile("../../assets/sounds/unlock_levels.mp3") ||
                              unlockLevelsBuf.loadFromFile("out/build/x64-debug/assets/sounds/unlock_levels.mp3") ||
                              unlockLevelsBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/unlock_levels.mp3");
    bool lockLevelsLoaded = lockLevelsBuf.loadFromFile("assets/sounds/lock_levels.mp3") ||
                            lockLevelsBuf.loadFromFile("../../assets/sounds/lock_levels.mp3") ||
                            lockLevelsBuf.loadFromFile("out/build/x64-debug/assets/sounds/lock_levels.mp3") ||
                            lockLevelsBuf.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/lock_levels.mp3");

    std::optional<sf::Sound> successSnd, errorSnd, levelCompleteSnd, levelFailSnd, missionStartSnd, menuClickSnd, campaignClickSnd, escSnd, unlockLevelsSnd, lockLevelsSnd;
    if (successLoaded) successSnd.emplace(successBuf);
    if (errorLoaded) errorSnd.emplace(errorBuf);
    if (levelCompleteLoaded) levelCompleteSnd.emplace(levelCompleteBuf);
    if (levelFailLoaded) levelFailSnd.emplace(levelFailBuf);
    if (missionStartLoaded) missionStartSnd.emplace(missionStartBuf);
    if (menuClickLoaded) menuClickSnd.emplace(menuClickBuf);
    if (campaignClickLoaded) campaignClickSnd.emplace(campaignClickBuf);
    if (escLoaded) escSnd.emplace(escBuf);
    if (unlockLevelsLoaded) unlockLevelsSnd.emplace(unlockLevelsBuf);
    if (lockLevelsLoaded) lockLevelsSnd.emplace(lockLevelsBuf);

    sf::Music bgm;
    bool bgmLoaded = bgm.openFromFile("assets/sounds/BGM.mp3") ||
                     bgm.openFromFile("../../assets/sounds/BGM.mp3") ||
                     bgm.openFromFile("out/build/x64-debug/assets/sounds/BGM.mp3") ||
                     bgm.openFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/sounds/BGM.mp3");
    if (bgmLoaded) { bgm.setLooping(true); bgm.setVolume(35.f); bgm.play(); }

    // Persistent score logs (Campaign scores.txt & Endless endless_leaderboard.txt)
    ScoreLog scoreLog;
    scoreLog.load("scores.txt");
    EndlessLeaderboard endlessLb;
    endlessLb.load("endless_leaderboard.txt");
    const std::string endlessSavePath = "endless_save.txt";
    const std::string saveGamePath = "savegame.dat";

    // 20 Campaign Levels across 4 Wings (5 levels per Wing) — loaded from levels/actN/level_N_M.json
    const int TOTAL_LEVELS = 20;
    const int LEVEL_IDS[TOTAL_LEVELS] = {
        LEVEL_1_1, LEVEL_1_2, LEVEL_1_3, LEVEL_1_4, LEVEL_1_5,
        LEVEL_2_1, LEVEL_2_2, LEVEL_2_3, LEVEL_2_4, LEVEL_2_5,
        LEVEL_3_1, LEVEL_3_2, LEVEL_3_3, LEVEL_3_4, LEVEL_3_5,
        LEVEL_4_1, LEVEL_4_2, LEVEL_4_3, LEVEL_4_4, LEVEL_4_5
    };
    LevelData levelData[TOTAL_LEVELS];
    LevelInfo levels[TOTAL_LEVELS];
    for (int i = 0; i < TOTAL_LEVELS; ++i) {
        std::string loadErr;
        if (!loadLevelById(LEVEL_IDS[i], levelData[i], loadErr)) {
            std::cerr << "Error loading level " << LEVEL_IDS[i] << ": " << loadErr << "\n";
            return 1;
        }
        int opt = computeOptimalSteps(levelData[i]);
        int cap = (levelData[i].id == LEVEL_2_5) ? act2BossCapacityFor(opt, 1) : ScoringEngine::capacityFor(opt);
        levels[i] = LevelInfo(levelData[i].id, levelData[i].act, levelData[i].title,
                              levelData[i].objective, levelData[i].controls,
                              cap, (i == 0), 0);
    }

    // Real configurable settings (Item 4)
    bool soundEnabled   = true;
    bool showNodeBadges = true;
    bool showBgGrid     = true;
    bool autoPlayEvents = true;

    auto playSfx = [&](std::optional<sf::Sound>& snd) {
        if (soundEnabled && snd.has_value()) { snd->play(); }
    };

    // Puzzle tracking state
    bool act1Reconstructed = false;
    bool act1Mirrored = false;
    int act1SeqIdx = 0;
    int act1TravMode = TRAV_PREORDER;
    int act1TravCounter = 0;
    int act1PostIdx = 0;
    int act1StepperVal = 1;
    int act1ConfirmedHeight[32];
    int act1LcaStage = 0;
    int act1LcaA = -1;
    int act1LcaB = -1;
    int act1RoutePath[16];
    int act1RouteLen = 0;
    int act1RouteIdx = 0;
    int act1SwappedKeys[32];
    int act1SwappedCount = 0;

    int act2TargetIdx = 0;
    int act2CurKey = -9999;
    int act2DelStage = 0;
    bool act2TwoChildPhase2Pending = false;
    LayoutResult act2MidLayout;
    EventBatch act2Phase2Batch;
    Act2RangeStep act2RqSteps[32];
    int act2RqCount = 0;
    int act2RqIdx = 0;
    int act2PrunedKeys[32];
    int act2PrunedCount = 0;
    int act2LastPrunedId = -1;
    std::string act2ReportedStr = "";
    int act2BossStage = 0;

    // Multi-selection markers for rendering
    int markedIds[16];
    int markedCount = 0;
    auto clearMarked = [&]() {
        markedCount = 0;
        for (int i = 0; i < 16; ++i) markedIds[i] = -1;
    };
    auto addMarkedId = [&](int id) {
        if (id < 0 || markedCount >= 16) return;
        for (int i = 0; i < markedCount; ++i) {
            if (markedIds[i] == id) return;
        }
        markedIds[markedCount++] = id;
    };

    // Game state & shared Phase 7 foundation systems
    int currentScreen = SCREEN_SPLASH;
    int menuSelectedIdx = 0;
    int mainMenuHoveredIdx = -1;
    int campaignHoveredId = -1;
    int selectedActIdx = 0;
    int selectedLevelInAct = 0;
    int currentLevelIdx = -1;
    std::string status = "Welcome to Vault Heist!";
    ScoringEngine scoring;
    HintSystem hintSys;
    DragDropController dragDrop;
    int alarmSteps = 0, score = SCORE_BASE;
    float splashTimer = 0.0f;
    float uiAnimTimer = 0.0f;
    float dialAngle = 0.0f;
    sf::Clock deltaClock;

    // Act 3 State (real AVLTree + live rotation/size validation)
    int manualSubPuzzle = 1;
    int manualDoubleStep = 0;
    int clicked31Mask = 0;
    bool act3PivotPicked = false;
    int act3PivotKey = -1;
    bool act3NextPuzzlePending = false;
    int act3LastComps = 0;
    int act3LastHeight = 0;
    bool act3SizeOk = true;

    AVLTree avl;
    int avlQueuePos = 0;

    int bossQueryIdx = 0;

    // Act 4 State
    MaxHeap act4Heap;
    int heapArr[30];
    int activeHeapSize = 0;
    int sortedArr[30];
    int sortedCount = 0;
    int buildStepIdx = -1;
    int hlIndex1 = -1, hlIndex2 = -1;
    bool act4Built = false;

    // Act 1 (General Binary Tree) State
    bool layoutDirty = true;
    BinaryTree act1Tree;
    BinaryTree act1RefTree;
    int act1Hl1 = -9999, act1Hl2 = -9999;
    std::string act1TraversalStr = "";
    auto resetAct1 = [&]() {
        buildBinaryTreeFromData(act1Tree, levelData[0]);
        act1Hl1 = -9999;
        act1Hl2 = -9999;
        act1TraversalStr = "";
        layoutDirty = true;
    };
    resetAct1();

    // Act 2 (BST) State & Shared GUI Controllers
    BST act2Bst;
    int act2Hl1 = -9999, act2Hl2 = -9999;
    std::string act2ExtraInfo = "";
    LayoutResult bstLayout, btLayout, avlLayout, heapLayout;
    Renderer guiRenderer;
    AnimationController animCtrl;
    UndoStack undoStack;
    ToastQueue toastQueue;
    auto pushToast = [&](const std::string& msg, int kind) {
        if (kind == TOAST_KIND_SUCCESS) playSfx(successSnd);
        else if (kind == TOAST_KIND_PENALTY) playSfx(errorSnd);
        toastQueue.push(msg, kind);
    };
    UiCache uiCache;
    UiCache menuUiCache;
    int cachedLayoutScreen = -1;
    int cachedLayoutLid = -1;

    // Live FPS readout state (Part A)
    sf::Clock fpsReportClock;
    int fpsFrameAccum = 0;
    std::string liveFpsStr = "FPS: --";

    // Cached render strings (Part A: avoid per-frame std::to_string & concatenation)
    bool campCachedUnl[20];
    int campCachedStars[20];
    std::string campCachedLabels[20];
    for (int i = 0; i < 20; ++i) { campCachedUnl[i] = false; campCachedStars[i] = -1; }

    // Demo unlock-all state (non-global, password-protected)
    bool demoAllUnlocked = false;
    bool demoPromptOpen = false;
    bool resetConfirmPending = false;
    char demoPasswordBuf[33];
    demoPasswordBuf[0] = '\0';
    int demoPasswordLen = 0;

    int cachedHudLid = -1;
    std::string cachedHudObj;
    std::string cachedHudCtrlLine;
    int cachedHudAlarm = -1, cachedHudCap = -1, cachedHudScore = -1;
    std::string cachedHudAlarmStr, cachedHudScoreStr;
    int cachedPromptLid = -1, cachedPromptSeq = -1, cachedPromptAvl = -1;
    int cachedPromptMode = -1, cachedPromptPost = -1, cachedPromptLca = -1;
    int cachedPromptAct2A = -9999, cachedPromptAct2B = -9999;
    int cachedPromptAct3A = -9999, cachedPromptAct3B = -9999;
    std::string cachedPromptHead;
    int cachedBriefLid = -1, cachedBriefCap = -1;
    std::string cachedBriefAct, cachedBriefObj, cachedBriefInt;

    int cachedResIdx = -1, cachedResScore = -1, cachedResAlarm = -1, cachedResBatches = -1, cachedResEvs = -1, cachedResStars = -1;
    std::string cachedResStats;

    std::string cachedAct1Stats;
    std::string cachedAct2Stats;

    int cachedLbCount = -1;
    std::string cachedLbRank[10];
    std::string cachedLbScore[10];
    std::string cachedLbAlarm[10];

    int cachedSetFpsVal = -1;
    std::string cachedSetResStr = "Display Resolution: " + std::to_string(W) + " x " + std::to_string(H);
    std::string cachedSetFpsLimStr;
    std::string cachedSetFpsBtnStr;

    int selectedNodeId = -1;
    bool pendingWin = false;
    bool showingBriefing = false;

    // Shared 1600x900 Canvas & Event Log Rectangles + Camera Views (Part B)
    const float CANVAS_X = 30.f,   CANVAS_Y = 86.f, CANVAS_W = 1140.f, CANVAS_H = 560.f;
    const float LOG_X    = 1185.f, LOG_Y    = 86.f, LOG_W    = 385.f,  LOG_H    = 560.f;

    CanvasCamera levelCam, demoCam, sbAvlCam, sbHeapCam, endlessCam;
    levelCam.init(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, static_cast<float>(W), static_cast<float>(H));
    demoCam.init(30.f, 110.f, 1140.f, 470.f, static_cast<float>(W), static_cast<float>(H));
    sbAvlCam.init(30.f, 95.f, 560.f, 540.f, static_cast<float>(W), static_cast<float>(H));
    sbHeapCam.init(605.f, 95.f, 565.f, 540.f, static_cast<float>(W), static_cast<float>(H));
    endlessCam.init(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, static_cast<float>(W), static_cast<float>(H));

    // Endless Mode state (Step F)
    EndlessSession endlessSession;
    bool endlessPaused = false;
    bool endlessShowLastResult = false;
    EndlessLeaderboardEntry endlessLastEntry;
    std::string endlessLastReason = "";
    unsigned int endlessLastSeed = 1337u;

    auto doSaveGame = [&]() {
        SaveData data;
        for (int i = 0; i < TOTAL_LEVELS; ++i) {
            data.unlocked[i] = levels[i].unlocked;
            data.stars[i] = levels[i].stars;
        }
        data.seed = endlessLastSeed;
        data.endlessScore = endlessSession.getOpsCompleted();
        data.endlessWave = 1 + (data.endlessScore / 10);
        BinaryTree emptyBt;
        BST emptyBst;
        SaveManager::saveToFile(saveGamePath, data, emptyBt, emptyBst, avl, nullptr, 0, nullptr, 0);
    };

    auto doLoadGame = [&]() {
        SaveData data;
        BinaryTree dummyBt;
        BST dummyBst;
        AVLTree loadedAvl;
        if (SaveManager::loadFromFile(saveGamePath, data, dummyBt, dummyBst, loadedAvl)) {
            for (int i = 0; i < TOTAL_LEVELS; ++i) {
                levels[i].unlocked = data.unlocked[i];
                levels[i].stars = data.stars[i];
                campCachedUnl[i] = false;
                campCachedStars[i] = -1;
                campCachedLabels[i].clear();
            }
            endlessLastSeed = data.seed;
            endlessLastEntry.seed = data.seed;
            endlessLastEntry.opsCompleted = data.endlessScore;
            avl.clear();
            copyAvlPreorder(loadedAvl.getRoot(), avl);
        }
    };
    doLoadGame();

    auto submitDemoPassword = [&]() {
        if (std::strcmp(demoPasswordBuf, "lambda@fadi") == 0) {
            if (!demoAllUnlocked) {
                for (int i = 0; i < TOTAL_LEVELS; ++i) {
                    levels[i].unlocked = true;
                    campCachedUnl[i] = false;
                    campCachedLabels[i].clear();
                }
                demoAllUnlocked = true;
                pushToast("[DEMO] All levels unlocked", TOAST_KIND_SUCCESS);
                playSfx(unlockLevelsSnd);
            } else {
                levels[0].unlocked = true;
                campCachedUnl[0] = false;
                campCachedLabels[0].clear();
                for (int i = 1; i < TOTAL_LEVELS; ++i) {
                    levels[i].unlocked = false;
                    campCachedUnl[i] = false;
                    campCachedLabels[i].clear();
                }
                demoAllUnlocked = false;
                pushToast("[DEMO] Levels locked", TOAST_KIND_SUCCESS);
                playSfx(lockLevelsSnd);
            }
            doSaveGame();
        } else {
            pushToast("Wrong password", TOAST_KIND_PENALTY);
            playSfx(errorSnd);
        }
        demoPromptOpen = false;
        demoPasswordLen = 0;
        demoPasswordBuf[0] = '\0';
    };
    int cachedEndlessOps = -1, cachedEndlessNodes = -1, cachedEndlessRateTenths = -1;
    std::string cachedEndlessSub = "";
    int cachedEndlessOpType = -1, cachedEndlessOpKey = -999999;
    bool cachedEndlessPaused = false;
    std::string cachedEndlessPrompt = "";

    auto layoutBstForScreen = [&](bool isDemo) {
        if (isDemo) LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, bstLayout);
        else        LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, bstLayout);
    };
    auto layoutBtForScreen = [&](bool isDemo) {
        if (isDemo) LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, btLayout);
        else        LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, btLayout);
    };
    auto layoutAvlForScreen = [&](float ax, float ay, float aw, float ah) {
        LayoutEngine::layoutAVL(avl, ax, ay, aw, ah, avlLayout);
    };
    auto layoutHeapForScreen = [&](bool useRealHeap, float ax, float ay, float aw, float ah) {
        if (useRealHeap) {
            LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount, ax, ay, aw, ah, heapLayout);
        } else {
            LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount, ax, ay, aw, ah, heapLayout);
        }
    };

    auto resetAct2 = [&]() {
        buildBstFromData(act2Bst, levelData[5]);
        act2Hl1 = -9999;
        act2Hl2 = -9999;
        act2ExtraInfo = "";
        selectedNodeId = -1;
        pendingWin = false;
        clearMarked();
        animCtrl.clearCurrent();
        layoutBstForScreen(false);
        layoutDirty = true;
    };
    resetAct2();

    auto syncScoringHud = [&]() {
        alarmSteps = scoring.alarm();
        score = scoring.score();
    };

    auto triggerAlarmFullFail = [&]() {
        playSfx(levelFailSnd);
        if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            levels[currentLevelIdx].stars = 0;
            pendingWin = false;
            currentScreen = SCREEN_RESULT;
            status = "ALARM FULL! Guards arrived. Level failed! Click Retry.";
            pushToast("ALARM FULL! Guards arrived!", TOAST_KIND_PENALTY);
        }
    };

    int wrongFlashNodeId = -1;
    float wrongFlashTimer = 0.f;
    auto triggerWrongFlash = [&](int nodeId) {
        wrongFlashNodeId = nodeId;
        wrongFlashTimer = 0.4f;
    };

    auto raisePenalty = [&](int penaltyKind, const std::string& msg) {
        int added = scoring.applyPenalty(penaltyKind);
        syncScoringHud();
        status = msg;
        std::string toastMsg = "+" + std::to_string(added) + " Alarm";
        if (penaltyKind == PENALTY_WRONG_ROTATION) {
            toastMsg += ", -" + std::to_string(SCORE_ROTATION_PENALTY) + " pts";
        }
        toastMsg += ": " + msg;
        pushToast(toastMsg, TOAST_KIND_PENALTY);
        if (scoring.isAlarmFull()) triggerAlarmFullFail();
    };

    auto finishLevelWin = [&](int starsAwarded, const std::string& msg) {
        playSfx(levelCompleteSnd);
        if (currentLevelIdx < 0) return;
        pendingWin = false;
        levels[currentLevelIdx].stars = maxInt(levels[currentLevelIdx].stars, starsAwarded);
        if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
        doSaveGame();
        status = msg;

        ScoreEntry se;
        se.levelTitle = levels[currentLevelIdx].title;
        se.score = score;
        se.stars = starsAwarded;
        se.alarm = alarmSteps;
        scoreLog.append(se);

        currentScreen = SCREEN_RESULT;
        pushToast("Level Complete! +" + std::to_string(score) + " pts", TOAST_KIND_SUCCESS);
    };

    auto currentHintStage = [&]() -> int {
        if (currentLevelIdx < 0) return 0;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        if (lid == LEVEL_1_1) return act1TravMode;
        if (lid == LEVEL_1_2) {
            int rootSlot = -1;
            for (int i = 0; i < btLayout.nodeCount; ++i) {
                if (btLayout.nodes[i].isRoot) { rootSlot = i; break; }
            }
            return (rootSlot >= 0 && dragDrop.tokenInSlot(rootSlot) >= 0) ? 1 : 0;
        }
        if (lid == LEVEL_1_3) return (act1PostIdx >= act1Tree.getSize()) ? 1 : 0;
        if (lid == LEVEL_1_4) return (act1LcaStage < 2) ? 0 : (act1LcaStage == 2 ? 1 : 2);
        if (lid == LEVEL_1_5) return 0;
        if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_3 || lid == LEVEL_2_4) return 0;
        if (lid == LEVEL_2_5) return act2BossStage;
        if (lid == LEVEL_3_2) return manualSubPuzzle - 1;
        if (lid == LEVEL_3_3) return (manualDoubleStep == 0) ? 0 : 1;
        if (lid == LEVEL_3_4) return avlQueuePos;
        if (lid == LEVEL_3_5) return bossQueryIdx;
        if (lid == LEVEL_4_1) {
            int firstIdx = (d.initialArrayCount / 2) - 1;
            return maxInt(0, firstIdx - buildStepIdx);
        }
        if (lid == LEVEL_4_5) return act4Built ? 1 : 0;
        return 0;
    };

    auto useHint = [&]() {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || showingBriefing) return;
        if (!hintSys.isAvailable()) {
            pushToast("Hint already used on this level!", TOAST_KIND_INFO);
            return;
        }
        int stage = currentHintStage();
        if (hintSys.use(scoring, levelData[currentLevelIdx], stage)) {
            status = "HINT (-1 star cap): " + hintSys.shownText();
            pushToast("Hint used (max 2 stars this level)", TOAST_KIND_INFO);
        }
    };

    auto startLevel = [&](int idx) {
        currentLevelIdx = idx;
        currentScreen = SCREEN_LEVEL_PLAY;
        const LevelData& d = levelData[idx];
        int opt = computeOptimalSteps(d);
        scoring.begin(opt);
        levels[idx].alarmCapacity = scoring.capacity();
        hintSys.reset();
        dragDrop.clear();
        syncScoringHud();

        sortedCount = 0;
        hlIndex1 = -1;
        hlIndex2 = -1;

        int lid = d.id;
        act1Reconstructed = false;
        act1Mirrored = false;
        act1SeqIdx = 0;
        act1PostIdx = 0;
        act1StepperVal = 1;
        for (int i = 0; i < 32; ++i) act1ConfirmedHeight[i] = 0;
        act1LcaStage = 0;
        act1LcaA = -1;
        act1LcaB = -1;
        act1RouteLen = 0;
        act1RouteIdx = 0;
        act1SwappedCount = 0;
        for (int i = 0; i < 32; ++i) act1SwappedKeys[i] = 0;
        act2TargetIdx = 0;
        act2CurKey = -9999;
        act2DelStage = 0;
        act2TwoChildPhase2Pending = false;
        act2Phase2Batch.clear();
        act2RqCount = 0;
        act2RqIdx = 0;
        act2PrunedCount = 0;
        act2LastPrunedId = -1;
        act2ReportedStr = "";
        act2BossStage = 0;
        manualSubPuzzle = 1;
        manualDoubleStep = 0;
        clicked31Mask = 0;
        act3PivotPicked = false;
        act3PivotKey = -1;
        act3NextPuzzlePending = false;
        act3LastComps = 0;
        act3LastHeight = 0;
        act3SizeOk = true;
        avlQueuePos = 0;
        bossQueryIdx = 0;
        selectedNodeId = -1;
        pendingWin = false;
        showingBriefing = true;
        clearMarked();
        toastQueue.clear();
        animCtrl.clearAll();
        undoStack.clear();
        levelCam.reset();
        layoutDirty = true;

        if (lid == LEVEL_1_1) {
            buildBinaryTreeFromData(act1Tree, d);
            act1TravMode = (act1TravCounter++) % 4;
            act1Hl1 = -9999; act1Hl2 = -9999; act1TraversalStr = "";
            status = std::string(traversalModeName(act1TravMode)) + " - click all rooms in exact order!";
        } else if (lid == LEVEL_1_2) {
            act1Tree.clear();
            act1RefTree.clear();
            act1RefTree.buildFromPreorderInorder(d.preorder, d.inorder, d.preorderCount);
            LayoutEngine::layoutBinaryTree(act1RefTree, CANVAS_X, CANVAS_Y + 75.f, CANVAS_W, CANVAS_H - 95.f, btLayout);
            layoutDirty = false;
            int nTok = btLayout.nodeCount;
            for (int i = 0; i < nTok; ++i) {
                int sId = dragDrop.addSlot(btLayout.nodes[i].x, btLayout.nodes[i].y, 26.f, DRAG_ANY_VALUE);
                dragDrop.setSlotLabel(sId, "?");
            }
            float traySpan = (nTok > 1) ? 700.f : 0.f;
            float trayStartX = CANVAS_X + CANVAS_W * 0.5f - traySpan * 0.5f;
            float trayStepX = (nTok > 1) ? (traySpan / static_cast<float>(nTok - 1)) : 0.f;
            for (int i = 0; i < d.preorderCount && i < nTok; ++i) {
                float tx = trayStartX + static_cast<float>(i) * trayStepX;
                dragDrop.addToken(d.preorder[i], tx, CANVAS_Y + 46.f, 22.f);
            }
            act1Hl1 = -9999; act1Hl2 = -9999; act1TraversalStr = "";
            status = d.startStatus;
        } else if (lid == LEVEL_1_3) {
            buildBinaryTreeFromData(act1Tree, d);
            int postCnt = 0;
            int* postSeq = act1Tree.postorderRecursive(postCnt);
            act1Hl1 = (postSeq && postCnt > 0) ? postSeq[0] : -9999;
            act1Hl2 = -9999;
            delete[] postSeq;
            act1TraversalStr = "";
            status = (act1Hl1 != -9999)
                ? ("Postorder height check [1/" + std::to_string(postCnt) + "]: enter height of highlighted room " +
                   std::to_string(act1Hl1) + " (leaf = 1) and press Submit!")
                : d.startStatus;
        } else if (lid == LEVEL_1_4 || lid == LEVEL_1_5) {
            buildBinaryTreeFromData(act1Tree, d);
            act1Hl1 = -9999; act1Hl2 = -9999; act1TraversalStr = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_1 || lid == LEVEL_2_2) {
            buildBstFromData(act2Bst, d);
            act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
            act2Hl1 = act2CurKey;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_3) {
            buildBstFromData(act2Bst, d);
            act2Hl1 = (d.deleteSequenceCount > 0) ? d.deleteSequence[0] : -9999;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_4) {
            buildBstFromData(act2Bst, d);
            buildAct2RangeDecisions(act2Bst.getRoot(), d.rangeLo, d.rangeHi, act2RqSteps, act2RqCount, 32);
            act2RqIdx = 0;
            act2Hl1 = (act2RqCount > 0) ? act2RqSteps[0].nodeKey : -9999;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_5) {
            buildBstFromData(act2Bst, d);
            int startCap = act2BossCapacityFor(opt, act2Bst.getHeight());
            scoring.setCapacity(startCap);
            levels[idx].alarmCapacity = scoring.capacity();
            act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
            act2Hl1 = act2CurKey;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_3_1) {
            buildUnbalancedAvlFromData(avl, d);
            status = d.startStatus;
        } else if (lid == LEVEL_3_2 || lid == LEVEL_3_3) {
            avl.clear();
            if (d.puzzleCount > 0) buildPuzzleAvl(avl, d.puzzles[0]);
            status = d.startStatus;
        } else if (lid == LEVEL_3_4) {
            avl.clear();
            for (int i = 0; i < d.initialKeyCount; ++i) avl.insert(d.initialKeys[i]);
            avlQueuePos = 0;
            if (d.insertCount > 0) {
                status = "Incoming key " + std::to_string(d.inserts[0].key) + "! " + d.startStatus;
            } else {
                status = d.startStatus;
            }
        } else if (lid == LEVEL_3_5) {
            avl.clear();
            for (int i = 0; i < d.initialKeyCount; ++i) avl.insert(d.initialKeys[i]);
            int badKey = -1;
            act3SizeOk = validateAvlSubtreeSizes(avl.getRoot(), badKey);
            act3LastHeight = avl.getHeight();
            bossQueryIdx = 0;
            status = d.startStatus;
        } else if (lid == LEVEL_4_1 || lid == LEVEL_4_2) {
            activeHeapSize = minInt(30, d.initialArrayCount);
            for (int i = 0; i < activeHeapSize; ++i) heapArr[i] = d.initialArray[i];
            buildStepIdx = findNextHeapViolation(heapArr, activeHeapSize, activeHeapSize / 2 - 1);
            hlIndex1 = buildStepIdx;
            hlIndex2 = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
            status = d.startStatus;
        } else if (lid == LEVEL_4_3 || lid == LEVEL_4_4) {
            act4Heap.buildMaxHeap(d.initialArray, d.initialArrayCount);
            act4Built = true;
            hlIndex1 = 0;
            if (lid == LEVEL_4_4) {
                status = "Steps: 0/" + std::to_string(opt) + " (" + std::to_string(opt) +
                         " extractions, O(log n) sift-down each = Theta(n log n)). Click root [0] (" +
                         std::to_string(act4Heap.getMax()) + ")!";
            } else {
                status = d.startStatus;
            }
        } else if (lid == LEVEL_4_5) {
            activeHeapSize = minInt(30, d.initialArrayCount);
            for (int i = 0; i < activeHeapSize; ++i) heapArr[i] = d.initialArray[i];
            act4Built = false;
            hlIndex1 = (activeHeapSize >= 2) ? (activeHeapSize / 2 - 1) : -1;
            status = d.startStatus;
        }
    };

    auto pushUndoSnapshot = [&](bool useRealHeap) {
        GameSnapshot snap;
        UndoStack::captureStructures(act1Tree, act2Bst, avl, act4Heap,
                                     heapArr, activeHeapSize, sortedArr, sortedCount,
                                     useRealHeap, snap);
        snap.alarmSteps = scoring.alarm();
        snap.score = scoring.score();
        snap.stepCount = scoring.steps();
        snap.penaltyCount = scoring.penaltyEvents();
        snap.penaltyPts = scoring.penaltyPoints();
        snap.bonus = scoring.bonusPoints();
        snap.hintUsed = scoring.hintWasUsed();
        snap.act1Reconstructed = act1Reconstructed;
        snap.act2Step21 = act2TargetIdx;
        snap.act2DelStage = act2DelStage;
        snap.act2BossStage = act2BossStage;
        snap.act2Clicked24Mask = act2RqIdx;
        snap.avlQueuePos = avlQueuePos;
        snap.bossQueryIdx = (currentLevelIdx >= 0 && (levelData[currentLevelIdx].id == LEVEL_3_2 || levelData[currentLevelIdx].id == LEVEL_3_3))
                                ? (manualSubPuzzle * 10 + manualDoubleStep)
                                : bossQueryIdx;
        snap.buildStepIdx = buildStepIdx;
        snap.act4Built = act4Built;
        snap.act1Hl1 = act1Hl1;
        snap.act1Hl2 = act1Hl2;
        snap.act2Hl1 = act2Hl1;
        snap.act2Hl2 = act2Hl2;
        snap.hlIndex1 = hlIndex1;
        snap.hlIndex2 = hlIndex2;
        snap.status = status;
        undoStack.push(snap);
    };

    auto performUndo = [&]() {
        GameSnapshot snap;
        if (!undoStack.pop(snap)) {
            status = "Nothing to undo!";
            pushToast("Nothing to undo", TOAST_KIND_WARNING);
            return;
        }
        UndoStack::restoreStructures(snap, act1Tree, act2Bst, avl, act4Heap,
                                     heapArr, activeHeapSize, sortedArr, sortedCount);
        bool keepHint = scoring.hintWasUsed() || snap.hintUsed;
        scoring.restoreState(snap.alarmSteps, snap.stepCount, snap.penaltyCount,
                             snap.penaltyPts, snap.bonus, keepHint);
        syncScoringHud();
        act1Reconstructed = snap.act1Reconstructed;
        act2TargetIdx = snap.act2Step21;
        act2DelStage = snap.act2DelStage;
        act2BossStage = snap.act2BossStage;
        act2RqIdx = snap.act2Clicked24Mask;
        act2TwoChildPhase2Pending = false;
        act2Phase2Batch.clear();
        act3PivotPicked = false;
        act3PivotKey = -1;
        act3NextPuzzlePending = false;
        avlQueuePos = snap.avlQueuePos;
        bossQueryIdx = snap.bossQueryIdx;
        buildStepIdx = snap.buildStepIdx;
        act4Built = snap.act4Built;
        act1Hl1 = snap.act1Hl1;
        act1Hl2 = snap.act1Hl2;
        act2Hl1 = snap.act2Hl1;
        act2Hl2 = snap.act2Hl2;
        act2CurKey = (act2Hl1 != -9999) ? act2Hl1 : (act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999);
        hlIndex1 = snap.hlIndex1;
        hlIndex2 = snap.hlIndex2;
        pendingWin = false;
        selectedNodeId = -1;
        clearMarked();
        if (currentLevelIdx >= 0 && currentLevelIdx < TOTAL_LEVELS) {
            const LevelData& curD = levelData[currentLevelIdx];
            if (curD.id == LEVEL_2_4) {
                buildAct2RangeDecisions(act2Bst.getRoot(), curD.rangeLo, curD.rangeHi, act2RqSteps, act2RqCount, 32);
                act2PrunedCount = 0;
                act2LastPrunedId = -1;
                act2ReportedStr = "";
                for (int ri = 0; ri < act2RqIdx && ri < act2RqCount; ++ri) {
                    if (act2RqSteps[ri].expectedAction == RQ_ACT_REPORT) {
                        addMarkedId(act2RqSteps[ri].nodeId);
                        if (!act2ReportedStr.empty()) act2ReportedStr += ", ";
                        act2ReportedStr += std::to_string(act2RqSteps[ri].nodeKey);
                    } else {
                        const TreeNode* subRoot = findTreeNodeConst(act2Bst.getRoot(), act2RqSteps[ri].subtreeRootKey);
                        collectSubtreeKeys(subRoot, act2PrunedKeys, act2PrunedCount, 32);
                        act2LastPrunedId = act2RqSteps[ri].subtreeRootId;
                    }
                }
            } else if (curD.id == LEVEL_2_5) {
                scoring.setCapacity(act2BossCapacityFor(scoring.optimal(), act2Bst.getHeight()));
                levels[currentLevelIdx].alarmCapacity = scoring.capacity();
            } else if (curD.id == LEVEL_3_1) {
                buildUnbalancedAvlFromData(avl, curD);
            } else if (curD.id == LEVEL_3_2 || curD.id == LEVEL_3_3) {
                manualSubPuzzle = maxInt(1, snap.bossQueryIdx / 10);
                manualDoubleStep = snap.bossQueryIdx % 10;
                if (manualSubPuzzle - 1 < curD.puzzleCount) {
                    buildPuzzleAvl(avl, curD.puzzles[manualSubPuzzle - 1]);
                    if (curD.id == LEVEL_3_3 && manualDoubleStep == 2 && avl.getRoot()) {
                        AVLNode* r = avl.getRoot();
                        EventBatch dummy;
                        if (avl.balanceFactor(r) > 1 && r->left) r->left = avl.rotateLeft(r->left, dummy, ROTATION_LR);
                        else if (avl.balanceFactor(r) < -1 && r->right) r->right = avl.rotateRight(r->right, dummy, ROTATION_RL);
                        avl.updateHeightAndSize(r, dummy);
                    }
                }
            }
        }
        animCtrl.clearCurrent();
        layoutDirty = true;
        status = "Restored previous state (" + std::to_string(undoStack.getCount()) + " snapshots left).";
        pushToast("Undo restored previous state", TOAST_KIND_WARNING);
    };

    auto startAct2TwoChildPhase2 = [&]() {
        if (!act2TwoChildPhase2Pending) return;
        act2TwoChildPhase2Pending = false;
        animCtrl.loadOperation(act2Phase2Batch, act2MidLayout, bstLayout, true, autoPlayEvents);
        pushToast("Phase 2: Unlinking inorder successor node!", TOAST_KIND_SUCCESS);
        if (currentLevelIdx >= 0 && act2DelStage >= levelData[currentLevelIdx].deleteSequenceCount) {
            pendingWin = true;
            status = "Unlinking successor node! Completing Steal & Collapse...";
        } else if (currentLevelIdx >= 0) {
            int nextKey = levelData[currentLevelIdx].deleteSequence[act2DelStage];
            status = "Deleted 2-child node! Target " + std::to_string(act2DelStage + 1) + "/" +
                     std::to_string(levelData[currentLevelIdx].deleteSequenceCount) +
                     ": classify room " + std::to_string(nextKey) + "!";
        }
    };

    auto loadNextAct3Puzzle = [&]() {
        if (!act3NextPuzzlePending || currentLevelIdx < 0) return;
        act3NextPuzzlePending = false;
        const LevelData& curD = levelData[currentLevelIdx];
        if (manualSubPuzzle < curD.puzzleCount) {
            buildPuzzleAvl(avl, curD.puzzles[manualSubPuzzle]);
            manualSubPuzzle++;
            manualDoubleStep = 0;
            act3PivotPicked = false;
            act3PivotKey = -1;
            selectedNodeId = -1;
            animCtrl.clearCurrent();
            layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
            layoutDirty = false;
            if (curD.id == LEVEL_3_2) {
                status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(curD.puzzleCount) +
                         ": Click the unbalanced pivot node (|BF| > 1), then pick [LL] or [RR]!";
            } else {
                status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(curD.puzzleCount) +
                         ": Identify the zig-zag case ([Case: LR] or [Case: RL])!";
            }
            pushToast("Loaded Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(curD.puzzleCount),
                            TOAST_KIND_SUCCESS);
        }
    };

    auto completePendingWin = [&]() {
        if (!pendingWin || currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        if (scoring.isAlarmFull()) {
            triggerAlarmFullFail();
            return;
        }
        int lid = levels[currentLevelIdx].id;
        int stars = scoring.stars();
        if (lid == LEVEL_1_1) finishLevelWin(stars, std::string(traversalModeShort(act1TravMode)) + " sequence decoded the blueprint protocol!");
        else if (lid == LEVEL_1_5) finishLevelWin(stars, "Vault mirrored! Inverted security tree verified.");
        else if (lid == LEVEL_2_1) finishLevelWin(stars, "Target key located via root-to-target BST comparisons!");
        else if (lid == LEVEL_2_2) finishLevelWin(stars, "Key 25 inserted into BST and duplicate 40 rejected!");
        else if (lid == LEVEL_2_3) finishLevelWin(stars, "Steal & Collapse complete: Leaf, One-Child, and Two-Children cases mastered!");
        else if (lid == LEVEL_2_4) finishLevelWin(stars, "Range Raid complete! Reported {" + act2ReportedStr + "} (+" + std::to_string(scoring.bonusPoints()) + " prune bonus)!");
        else if (lid == LEVEL_2_5) {
            int h = act2Bst.getHeight();
            pushToast("ACT 2 CLEARED: Sorted keys caused O(n) degeneration (Height=" +
                            std::to_string(h) + ")!", TOAST_KIND_WARNING);
            pushToast("Wing 3 Unlocked: AVL rotations prevent Lazy Architect O(n) chains!", TOAST_KIND_SUCCESS);
            finishLevelWin(stars,
                           "LAZY ARCHITECT EXPOSED! Sorted keys built a Height=" + std::to_string(h) +
                           " degenerate chain, turning searches into O(n) walks under a shrinking alarm. "
                           "Wing 3 (AVL Trees) is now unlocked to enforce balance and O(log n) speed!");
        }
        else if (lid == LEVEL_3_2) finishLevelWin(stars, "Single rotations (LL & RR) repaired using real AVLTree rotations!");
        else if (lid == LEVEL_3_3) finishLevelWin(stars, "Double rotations (LR & RL) repaired using real AVLTree rotations!");
        else if (lid == LEVEL_3_4) finishLevelWin(stars, "All AVL rotation types predicted & auto-balanced cleanly!");
        else if (lid == LEVEL_3_5) {
            finishLevelWin(stars,
                           "Act 3 Boss defeated! Stole k-th most valuable treasures in " +
                           std::to_string(act3LastComps) + " comparisons <= Height " +
                           std::to_string(act3LastHeight) + " (O(log n), Size Check: " +
                           std::string(act3SizeOk ? "PASS" : "FAIL") + ")! Wing 4 unlocked!");
        }
        else if (lid == LEVEL_4_1) finishLevelWin(stars, "Bottom-up Max Heap activated in O(n)!");
        else if (lid == LEVEL_4_2) finishLevelWin(stars, "Sabotaged root sifted down; Max Heap restored!");
        else if (lid == LEVEL_4_3 || lid == LEVEL_4_4 || lid == LEVEL_4_5) {
            if (!validateAct4SortedResult(levelData[currentLevelIdx], sortedArr, sortedCount)) {
                pendingWin = false;
                raisePenalty(PENALTY_ACT4_INVALID, "Sorted array mismatch against expected_sorted_result!");
                return;
            }
            if (lid == LEVEL_4_3) {
                finishLevelWin(stars, "All treasures extracted in ascending order!");
            } else if (lid == LEVEL_4_4) {
                int optS = computeOptimalSteps(levelData[currentLevelIdx]);
                finishLevelWin(stars, "Race won! Steps: " + std::to_string(scoring.steps()) + "/" +
                                      std::to_string(optS) + " (" + std::to_string(optS) +
                                      " extractions, O(log n) each = Theta(n log n))!");
            } else {
                finishLevelWin(stars, "Final Vault unlocked! In-place ascending Heap Sort complete!");
            }
        }
        else finishLevelWin(stars, "Level Complete!");
    };

    auto stepAnimForward = [&]() {
        if (animCtrl.stepForward()) {
            Event cur = animCtrl.getCurrentEvent();
            status = "Step: " + AnimationController::formatEvent(cur);
        } else if (act2TwoChildPhase2Pending && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            startAct2TwoChildPhase2();
        } else if (act3NextPuzzlePending && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            loadNextAct3Puzzle();
        } else if (pendingWin && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            completePendingWin();
        } else {
            status = "Event batch complete!";
        }
    };

    auto handleAct3SingleRotation = [&](int choice) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_3_2) return;
        if (!act3PivotPicked || act3PivotKey < 0) {
            pushToast("Click the unbalanced pivot node (|BF| > 1) first!", TOAST_KIND_WARNING);
            status = "Step 1/2: Click the unbalanced pivot node (RED |BF| > 1) on the tree first!";
            return;
        }
        AVLNode* pivot = findAvlNodeMutable(avl.getRoot(), act3PivotKey);
        if (!pivot) return;
        int bf = avl.balanceFactor(pivot);
        int expectedChoice = (bf > 1 && pivot->left && avl.balanceFactor(pivot->left) >= 0) ? 1
                           : ((bf < -1 && pivot->right && avl.balanceFactor(pivot->right) <= 0) ? 2 : 0);
        if (choice != expectedChoice) {
            raisePenalty(PENALTY_WRONG_ROTATION,
                         "Wrong rotation choice! Pivot " + std::to_string(act3PivotKey) +
                         " has BF=" + std::to_string(bf) + " (pick " +
                         std::string(expectedChoice == 1 ? "[LL: Rotate Right]" : "[RR: Rotate Left]") + ").");
            return;
        }

        pushUndoSnapshot(false);
        LayoutResult before;
        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
        EventBatch batch;
        batch.append(Event(EVENT_OPERATION_STARTED, choice == 1 ? "single LL rotate right" : "single RR rotate left"));
        {
            Event alert(EVENT_BALANCE_ALERT, "balance alert");
            alert.nodeId = pivot->key;
            alert.newValue = bf;
            batch.append(alert);
        }
        AVLNode* oldParent = pivot->parent;
        bool wasLeft = (oldParent && oldParent->left == pivot);
        AVLNode* newSub = (choice == 1) ? avl.rotateRight(pivot, batch, ROTATION_LL)
                                        : avl.rotateLeft(pivot, batch, ROTATION_RR);
        if (!oldParent) {
            avl.setRoot(newSub);
        } else {
            if (wasLeft) oldParent->left = newSub;
            else oldParent->right = newSub;
            newSub->parent = oldParent;
            for (AVLNode* anc = oldParent; anc; anc = anc->parent) {
                avl.updateHeightAndSize(anc, batch);
            }
        }
        batch.append(Event(EVENT_OPERATION_COMPLETED, choice == 1 ? "LL rotation complete" : "RR rotation complete"));

        int badKey = -1;
        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
            raisePenalty(PENALTY_WRONG_ROTATION, "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
            return;
        }

        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
        scoring.addStep(1);
        syncScoringHud();
        act3PivotPicked = false;
        act3PivotKey = -1;
        selectedNodeId = -1;

        pushToast(std::string(choice == 1 ? "LL (Rotate Right)" : "RR (Rotate Left)") +
                        " repaired puzzle " + std::to_string(manualSubPuzzle) + "!", TOAST_KIND_SUCCESS);
        if (scoring.isAlarmFull()) {
            triggerAlarmFullFail();
        } else if (manualSubPuzzle < d.puzzleCount) {
            act3NextPuzzlePending = true;
            status = "Rotation animated! Advancing to Puzzle " + std::to_string(manualSubPuzzle + 1) + "/" +
                     std::to_string(d.puzzleCount) + "...";
        } else {
            pendingWin = true;
            status = "Final single rotation complete! Verifying AVL balance...";
        }
    };

    auto handleAct3DoubleCase = [&](int choice) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_3_3) return;
        if (manualDoubleStep != 0) {
            pushToast("Case already identified! Click the pivot node on the tree.", TOAST_KIND_WARNING);
            return;
        }
        AVLNode* r = avl.getRoot();
        int rbf = avl.balanceFactor(r);
        int expectedCase = (rbf > 1 && r && r->left && avl.balanceFactor(r->left) < 0) ? 1
                         : ((rbf < -1 && r && r->right && avl.balanceFactor(r->right) > 0) ? 2 : 0);
        if (choice != expectedCase) {
            raisePenalty(PENALTY_WRONG_ROTATION,
                         "Wrong zig-zag case! Inspect BF signs on root and heavy child (+2/-1 = LR, -2/+1 = RL).");
            return;
        }
        manualDoubleStep = 1;
        int childKey = (expectedCase == 1 && r->left) ? r->left->key
                     : ((expectedCase == 2 && r->right) ? r->right->key : -1);
        pushToast(std::string("Identified ") + (expectedCase == 1 ? "LR" : "RL") +
                        " case! Now click child pivot " + std::to_string(childKey), TOAST_KIND_SUCCESS);
        status = std::string(expectedCase == 1 ? "LR" : "RL") +
                 " case confirmed! Step 2/3: Click child pivot node " + std::to_string(childKey) + " first!";
    };

    auto handleAct3Prediction = [&](const char* pred) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_3_4 || avlQueuePos >= d.insertCount) return;

        int incoming = d.inserts[avlQueuePos].key;
        const std::string& expectedRot = d.inserts[avlQueuePos].expectedRotation;
        if (expectedRot != pred) {
            raisePenalty(PENALTY_WRONG_ROTATION,
                         "Wrong prediction '" + std::string(pred) + "' for incoming key " +
                         std::to_string(incoming) + "! Check which ancestor hits |BF| = 2.");
            return;
        }

        pushUndoSnapshot(false);
        LayoutResult before;
        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
        EventBatch batch;
        avl.insert(incoming, batch);

        int badKey = -1;
        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
            raisePenalty(PENALTY_WRONG_ROTATION, "Subtree size mismatch at node " + std::to_string(badKey) + "!");
            return;
        }

        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
        scoring.addStep(1);
        scoring.addBonus(50);
        syncScoringHud();
        pushToast("Correct (" + expectedRot + ")! Inserted & balanced key " + std::to_string(incoming),
                        TOAST_KIND_SUCCESS);

        avlQueuePos++;
        if (avlQueuePos >= d.insertCount) {
            pendingWin = true;
            status = "All incoming keys balanced! Playing final AVL events...";
        } else {
            status = "Inserted " + std::to_string(incoming) + "! Predict rotation for next key [" +
                     std::to_string(d.inserts[avlQueuePos].key) + "]: LL, RR, LR, RL, or None!";
        }
        if (scoring.isAlarmFull()) triggerAlarmFullFail();
    };

    auto handleAct2CompareChoice = [&](int choice) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        if (lid != LEVEL_2_1 && lid != LEVEL_2_2 && lid != LEVEL_2_5) return;

        layoutDirty = true;
        const TreeNode* curNode = findTreeNodeConst(act2Bst.getRoot(), act2CurKey);
        if (!curNode) return;

        bool isSearchMode = (lid == LEVEL_2_1) || (lid == LEVEL_2_5 && act2BossStage == 1);
        int key = isSearchMode
                      ? ((lid == LEVEL_2_1)
                             ? ((act2TargetIdx < d.targetKeyCount) ? d.targetKeys[act2TargetIdx] : -1)
                             : ((d.kQueryCount > 0) ? d.kQueries[0]
                                : (d.targetKeyCount > 0 ? d.targetKeys[d.targetKeyCount - 1] : -1)))
                      : ((act2TargetIdx < d.targetKeyCount) ? d.targetKeys[act2TargetIdx] : -1);
        if (key < 0) return;

        int expectedChoice = (key < curNode->value) ? 1 : ((key > curNode->value) ? 3 : 2);
        if (choice != expectedChoice) {
            std::string hintDir = (expectedChoice == 1)
                                      ? ("[< Go Left] (" + std::to_string(key) + " < " + std::to_string(curNode->value) + ")")
                                      : (expectedChoice == 3
                                             ? ("[Go Right >] (" + std::to_string(key) + " > " + std::to_string(curNode->value) + ")")
                                             : (isSearchMode ? "[= Found]" : "[= Reject Dup]"));
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Wrong choice at room " + std::to_string(curNode->value) + " for key " +
                         std::to_string(key) + "! Expected " + hintDir + ".");
            return;
        }

        pushUndoSnapshot(false);

        if (isSearchMode) {
            if (choice == 1 || choice == 3) {
                const TreeNode* nextNode = (choice == 1) ? curNode->left : curNode->right;
                if (!nextNode) {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL, "Target key " + std::to_string(key) + " is not in that branch!");
                    return;
                }
                addMarkedId(curNode->id);
                scoring.addStep(1);
                syncScoringHud();

                EventBatch batch;
                Event v(EVENT_VISIT, "visit");
                v.nodeId = curNode->id;
                v.newValue = curNode->value;
                batch.append(v);
                Event c(EVENT_COMPARE, (choice == 1) ? "go left" : "go right");
                c.nodeId = curNode->id;
                c.oldValue = curNode->value;
                c.newValue = key;
                batch.append(c);
                layoutBstForScreen(false);
                animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);

                act2CurKey = nextNode->value;
                act2Hl1 = act2CurKey;
                pushToast("Compared " + std::to_string(key) + (choice == 1 ? " < " : " > ") +
                                std::to_string(curNode->value) + " -> room " + std::to_string(act2CurKey),
                                TOAST_KIND_SUCCESS);
                status = "At room " + std::to_string(act2CurKey) + ": compare target " + std::to_string(key) +
                         " vs " + std::to_string(act2CurKey) + " ([< Go Left], [= Found], [Go Right >])!";
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
            } else {
                addMarkedId(curNode->id);
                scoring.addStep(1);
                syncScoringHud();

                LayoutResult before;
                LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                EventBatch batch;
                act2Bst.search(key, batch);
                layoutBstForScreen(false);
                animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);

                if (lid == LEVEL_2_1) {
                    act2TargetIdx++;
                    if (act2TargetIdx < d.targetKeyCount) {
                        clearMarked();
                        act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                        act2Hl1 = act2CurKey;
                        pushToast("Found target " + std::to_string(key) + "!", TOAST_KIND_SUCCESS);
                        status = "Found " + std::to_string(key) + "! Next target: " +
                                 std::to_string(d.targetKeys[act2TargetIdx]) + ". Start at root " +
                                 std::to_string(act2CurKey) + "!";
                    } else {
                        pendingWin = true;
                        pushToast("Found target key " + std::to_string(key) + "!", TOAST_KIND_SUCCESS);
                        status = "Target " + std::to_string(key) + " located! Playing BST search events...";
                    }
                } else {
                    pendingWin = true;
                    pushToast("Located " + std::to_string(key) + " on degenerate chain!", TOAST_KIND_SUCCESS);
                    status = "Found key " + std::to_string(key) + " on height-" +
                             std::to_string(act2Bst.getHeight()) + " degenerate chain! Finishing boss...";
                }
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
            }
            return;
        }

        // Insert mode (2.2 or 2.5 Stage 0)
        if (choice == 2) {
            addMarkedId(curNode->id);
            scoring.addStep(1);
            syncScoringHud();

            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.insert(key, batch); // rejects duplicate and emits EVENT_OPERATION_FAILED
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            pushToast("Duplicate key " + std::to_string(key) + " REJECTED (no duplicates in BST)!",
                            TOAST_KIND_WARNING);

            act2TargetIdx++;
            if (act2TargetIdx >= d.targetKeyCount) {
                pendingWin = true;
                status = "Duplicate key " + std::to_string(key) + " rejected! BST invariant preserved.";
            } else {
                clearMarked();
                act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                act2Hl1 = act2CurKey;
                status = "Duplicate " + std::to_string(key) + " rejected! Next incoming key: " +
                         std::to_string(d.targetKeys[act2TargetIdx]) + " - start at root " +
                         std::to_string(act2CurKey) + "!";
            }
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
            return;
        }

        const TreeNode* nextNode = (choice == 1) ? curNode->left : curNode->right;
        if (nextNode) {
            addMarkedId(curNode->id);
            scoring.addStep(1);
            syncScoringHud();

            EventBatch batch;
            Event v(EVENT_VISIT, "visit");
            v.nodeId = curNode->id;
            v.newValue = curNode->value;
            batch.append(v);
            Event c(EVENT_COMPARE, (choice == 1) ? "go left" : "go right");
            c.nodeId = curNode->id;
            c.oldValue = curNode->value;
            c.newValue = key;
            batch.append(c);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);

            act2CurKey = nextNode->value;
            act2Hl1 = act2CurKey;
            pushToast("Compared " + std::to_string(key) + (choice == 1 ? " < " : " > ") +
                            std::to_string(curNode->value) + " -> room " + std::to_string(act2CurKey),
                            TOAST_KIND_SUCCESS);
            status = "At room " + std::to_string(act2CurKey) + ": guide incoming key " + std::to_string(key) + "!";
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        } else {
            scoring.addStep(1);
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.insert(key, batch);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);

            if (lid == LEVEL_2_5) {
                int h = act2Bst.getHeight();
                int newCap = act2BossCapacityFor(scoring.optimal(), h);
                scoring.setCapacity(newCap);
                levels[currentLevelIdx].alarmCapacity = scoring.capacity();
                pushToast("Inserted " + std::to_string(key) + " (Height=" + std::to_string(h) +
                                ")! Alarm cap shrank to " + std::to_string(newCap) + "!", TOAST_KIND_WARNING);
            } else {
                pushToast("Inserted key " + std::to_string(key) + " under room " +
                                std::to_string(curNode->value) + "!", TOAST_KIND_SUCCESS);
            }
            syncScoringHud();

            act2TargetIdx++;
            clearMarked();
            if (act2TargetIdx >= d.targetKeyCount) {
                if (lid == LEVEL_2_5) {
                    act2BossStage = 1;
                    act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                    act2Hl1 = act2CurKey;
                    int searchKey = (d.kQueryCount > 0) ? d.kQueries[0] : key;
                    status = "Degenerate Chain built (Height=" + std::to_string(act2Bst.getHeight()) +
                             ", Cap=" + std::to_string(scoring.capacity()) +
                             ")! Now search for target " + std::to_string(searchKey) + " from root " +
                             std::to_string(act2CurKey) + "!";
                } else {
                    act2Hl1 = key;
                    pendingWin = true;
                    status = "Inserted key " + std::to_string(key) + "! Playing final BST insert events...";
                }
            } else {
                act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                act2Hl1 = act2CurKey;
                status = "Key " + std::to_string(key) + " inserted! Incoming key " +
                         std::to_string(d.targetKeys[act2TargetIdx]) + " (" +
                         std::to_string(act2TargetIdx + 1) + "/" + std::to_string(d.targetKeyCount) +
                         ") - start at root " + std::to_string(act2CurKey) + "!";
            }
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        }
    };

    auto handleAct2DeleteCase = [&](int pickedCase) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_2_3 || act2DelStage >= d.deleteSequenceCount) return;

        layoutDirty = true;
        if (act2TwoChildPhase2Pending) startAct2TwoChildPhase2();

        int targetKey = d.deleteSequence[act2DelStage];
        const TreeNode* u = findTreeNodeConst(act2Bst.getRoot(), targetKey);
        if (!u) return;
        int trueCase = bstDeleteCase(u);
        if (pickedCase != trueCase) {
            const char* pickedName = (pickedCase == 1) ? "Leaf (0 children)"
                                   : (pickedCase == 2) ? "One-Child (1 child)"
                                                       : "Two-Children (2 children)";
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Wrong case! Room " + std::to_string(targetKey) + " is not " + pickedName + " right now.");
            return;
        }

        pushUndoSnapshot(false);
        scoring.addStep(1);
        syncScoringHud();

        if (trueCase == 1 || trueCase == 2) {
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.deleteKey(targetKey, batch);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            act2DelStage++;
            if (act2DelStage < d.deleteSequenceCount) {
                act2Hl1 = d.deleteSequence[act2DelStage];
                pushToast("Deleted " + std::to_string(targetKey) + " (" +
                                (trueCase == 1 ? "Leaf" : "One-Child") + " case)!", TOAST_KIND_SUCCESS);
                status = "Deleted " + std::to_string(targetKey) + "! Target " +
                         std::to_string(act2DelStage + 1) + "/" + std::to_string(d.deleteSequenceCount) +
                         ": Room " + std::to_string(act2Hl1) + " - classify [Leaf], [One-Child], or [Two-Children]!";
            } else {
                act2Hl1 = -9999;
                pendingWin = true;
                pushToast("Deleted final room " + std::to_string(targetKey) + "!", TOAST_KIND_SUCCESS);
                status = "Deleted final room " + std::to_string(targetKey) + "! Playing deletion animation...";
            }
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        } else {
            // Two-Children case: Phase 1 highlights successor + copies value; Phase 2 unlinks successor.
            const TreeNode* succ = findBstMinNodeConst(u->right);
            int uId = u->id;
            int succId = succ ? succ->id : -1;
            int succVal = succ ? succ->value : -1;
            int succChildId = (succ && succ->right) ? succ->right->id : -1;

            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);

            act2MidLayout = before;
            int uLayoutIdx = act2MidLayout.findNodeIndexById(uId);
            if (uLayoutIdx >= 0 && succ) act2MidLayout.nodes[uLayoutIdx].key = succVal;

            EventBatch batchPhase1;
            batchPhase1.append(Event(EVENT_OPERATION_STARTED, "delete 2-child"));
            {
                Event v1(EVENT_VISIT, "target node");
                v1.nodeId = uId;
                v1.newValue = targetKey;
                batchPhase1.append(v1);
            }
            if (succ) {
                Event v2(EVENT_VISIT, "inorder successor");
                v2.nodeId = succId;
                v2.newValue = succVal;
                batchPhase1.append(v2);

                Event sw(EVENT_SWAP_VALUE, "copy successor");
                sw.nodeId = uId;
                sw.otherNodeId = succId;
                sw.oldValue = targetKey;
                sw.newValue = succVal;
                batchPhase1.append(sw);
            }
            batchPhase1.append(Event(EVENT_OPERATION_COMPLETED, "successor value copied"));

            act2Phase2Batch.clear();
            act2Phase2Batch.append(Event(EVENT_OPERATION_STARTED, "unlink successor"));
            if (succ) {
                Event v3(EVENT_VISIT, "locate successor");
                v3.nodeId = succId;
                v3.newValue = succVal;
                act2Phase2Batch.append(v3);

                Event ul(EVENT_UNLINK, "unlink successor");
                ul.nodeId = succId;
                ul.otherNodeId = succChildId;
                ul.oldValue = succVal;
                act2Phase2Batch.append(ul);
            }
            act2Phase2Batch.append(Event(EVENT_OPERATION_COMPLETED, "delete 2-child complete"));

            EventBatch tmpReal;
            act2Bst.deleteKey(targetKey, tmpReal);
            layoutBstForScreen(false);

            act2DelStage++;
            act2Hl1 = (act2DelStage < d.deleteSequenceCount) ? d.deleteSequence[act2DelStage] : -9999;
            act2TwoChildPhase2Pending = true;
            animCtrl.loadOperation(batchPhase1, before, act2MidLayout, true, autoPlayEvents);
            pushToast("Phase 1: Highlighting inorder successor " + std::to_string(succVal) +
                            " and swapping value into " + std::to_string(targetKey) + "!", TOAST_KIND_SUCCESS);
            status = "Two-Children Case: Phase 1 highlights successor " + std::to_string(succVal) +
                     " & swaps value; Phase 2 will unlink the successor node!";
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        }
    };

    auto handleAct2RangeDecision = [&](int actionCode) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_2_4 || act2RqIdx >= act2RqCount) return;

        layoutDirty = true;
        const Act2RangeStep& step = act2RqSteps[act2RqIdx];
        if (actionCode != step.expectedAction) {
            std::string expStr = (step.expectedAction == RQ_ACT_REPORT)
                                     ? "Report Room"
                                     : ((step.expectedAction == RQ_ACT_PRUNE_LEFT) ? "Prune Left" : "Prune Right");
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Wrong Range decision at room " + std::to_string(step.nodeKey) +
                         " for [" + std::to_string(d.rangeLo) + ".." + std::to_string(d.rangeHi) +
                         "]! Expected [" + expStr + "].");
            return;
        }

        pushUndoSnapshot(false);
        scoring.addStep(1);

        if (actionCode == RQ_ACT_REPORT) {
            addMarkedId(step.nodeId);
            if (!act2ReportedStr.empty()) act2ReportedStr += ", ";
            act2ReportedStr += std::to_string(step.nodeKey);

            EventBatch batch;
            Event v(EVENT_VISIT, "report in-range");
            v.nodeId = step.nodeId;
            v.newValue = step.nodeKey;
            batch.append(v);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);
            pushToast("Reported in-range key " + std::to_string(step.nodeKey) + "!", TOAST_KIND_SUCCESS);
        } else {
            const TreeNode* subRoot = findTreeNodeConst(act2Bst.getRoot(), step.subtreeRootKey);
            collectSubtreeKeys(subRoot, act2PrunedKeys, act2PrunedCount, 32);
            act2LastPrunedId = step.subtreeRootId;
            scoring.addBonus(50);

            EventBatch batch;
            Event p(EVENT_HIGHLIGHT_SUBTREE, "prune subtree");
            p.nodeId = step.subtreeRootId;
            p.otherNodeId = step.nodeId;
            p.newValue = step.subtreeRootKey;
            batch.append(p);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);
            pushToast("Pruned subtree " + std::to_string(step.subtreeRootKey) +
                            " outside [" + std::to_string(d.rangeLo) + ".." + std::to_string(d.rangeHi) +
                            "]! +50 bonus", TOAST_KIND_SUCCESS);
        }

        syncScoringHud();
        act2RqIdx++;
        if (act2RqIdx >= act2RqCount) {
            act2Hl1 = -9999;
            pendingWin = true;
            status = "Range Raid [" + std::to_string(d.rangeLo) + ".." + std::to_string(d.rangeHi) +
                     "] complete! Reported {" + act2ReportedStr + "} (+" +
                     std::to_string(scoring.bonusPoints()) + " prune bonus)!";
        } else {
            act2Hl1 = act2RqSteps[act2RqIdx].nodeKey;
            status = "At room " + std::to_string(act2Hl1) + " [" + std::to_string(act2RqIdx + 1) + "/" +
                     std::to_string(act2RqCount) + "]: reported {" +
                     (act2ReportedStr.empty() ? std::string("none") : act2ReportedStr) +
                     "} - choose [Report Room], [Prune Left], or [Prune Right]!";
        }
        if (scoring.isAlarmFull()) triggerAlarmFullFail();
    };

    auto cycleAct1TravMode = [&]() {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        if (levelData[currentLevelIdx].id != LEVEL_1_1) return;
        if (act1SeqIdx > 0 || pendingWin) {
            pushToast("Cannot switch traversal mid-sequence", TOAST_KIND_WARNING);
            return;
        }
        act1TravMode = (act1TravMode + 1) % 4;
        status = std::string(traversalModeName(act1TravMode)) + " - click all rooms in exact order!";
        pushToast(std::string("Protocol: ") + traversalModeShort(act1TravMode), TOAST_KIND_SUCCESS);
    };

    auto submitAct1HeightOrDiameter = [&]() {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_1_3) return;
        layoutDirty = true;

        int postCnt = 0;
        int* postSeq = act1Tree.postorderRecursive(postCnt);
        if (act1PostIdx < postCnt && postSeq) {
            int targetKey = postSeq[act1PostIdx];
            const TreeNode* node = findTreeNodeConst(act1Tree.getRoot(), targetKey);
            int trueH = treeHeightNodes(node);
            if (act1StepperVal == trueH) {
                act1ConfirmedHeight[act1PostIdx] = trueH;
                if (node) addMarkedId(node->id);
                act1PostIdx++;
                scoring.addStep(1);
                syncScoringHud();
                if (scoring.isAlarmFull()) {
                    triggerAlarmFullFail();
                } else if (act1PostIdx < postCnt) {
                    act1Hl1 = postSeq[act1PostIdx];
                    act1StepperVal = 1;
                    pushToast("Height(" + std::to_string(targetKey) + ") = " + std::to_string(trueH) + " verified!",
                                    TOAST_KIND_SUCCESS);
                    status = "Room " + std::to_string(targetKey) + " height = " + std::to_string(trueH) +
                             ". Now enter height of room " + std::to_string(act1Hl1) + " [" +
                             std::to_string(act1PostIdx + 1) + "/" + std::to_string(postCnt) + "]!";
                } else {
                    act1Hl1 = act1Tree.getRoot() ? act1Tree.getRoot()->value : -9999;
                    act1StepperVal = 1;
                    pushToast("All room heights verified! Now enter Diameter.", TOAST_KIND_SUCCESS);
                    status = "All room heights verified! Final step: enter the tree's Diameter (longest path in nodes) and press Submit!";
                }
            } else {
                raisePenalty(PENALTY_WRONG_TRAVERSAL,
                             "Wrong height " + std::to_string(act1StepperVal) + " for room " +
                             std::to_string(targetKey) + "! Leaf = 1, parent = 1 + max(left, right).");
            }
        } else {
            int trueDiam = act1Tree.diameter();
            if (act1StepperVal == trueDiam) {
                scoring.addStep(1);
                syncScoringHud();
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
                else finishLevelWin(scoring.stars(),
                                    "Postorder heights & tree diameter (" + std::to_string(trueDiam) + " nodes) verified!");
            } else {
                raisePenalty(PENALTY_WRONG_TRAVERSAL,
                             "Wrong diameter " + std::to_string(act1StepperVal) +
                             "! Longest path (in nodes) through the root is leftHeight + rightHeight + 1.");
            }
        }
        delete[] postSeq;
    };

    auto handleAct1BlueprintDrop = [&](const DropResult& res) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_1_2 || act1Reconstructed) return;
        if (res.outcome != DROP_SNAPPED) return;

        scoring.addStep(1);
        syncScoringHud();

        int nSlots = dragDrop.slotCount();
        int filledCount = 0;
        for (int i = 0; i < nSlots; ++i) {
            if (dragDrop.tokenInSlot(i) >= 0) ++filledCount;
        }
        if (filledCount < nSlots) {
            status = "Placed token " + std::to_string(res.tokenValue) + " (" + std::to_string(filledCount) +
                     "/" + std::to_string(nSlots) + " slots filled). Keep filling all blueprint slots!";
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
            return;
        }

        BinaryTree playerTree;
        buildTreeFromSlots(btLayout, dragDrop, playerTree);
        if (playerTree.isSameTree(act1RefTree)) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act1Tree.clear();
            act1Tree.buildFromPreorderInorder(d.preorder, d.inorder, d.preorderCount, batch);
            act1Reconstructed = true;
            dragDrop.clear();
            layoutBtForScreen(false);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
            else finishLevelWin(scoring.stars(), "Blueprint decoded! Reconstructed tree matches Preorder + Inorder.");
        } else {
            for (int i = 0; i < nSlots && i < btLayout.nodeCount; ++i) {
                int tid = dragDrop.tokenInSlot(i);
                const DragToken* tok = dragDrop.token(tid);
                if (tok && tok->value != btLayout.nodes[i].key) {
                    dragDrop.returnTokenHome(tid);
                }
            }
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Blueprint mismatch! Misplaced tokens returned to tray - check how Inorder splits left and right subtrees.");
        }
    };

    // Minimal non-choice structural triggers (e.g., 1.5 Auto Mirror, 4.5 Build Heap)
    auto triggerStructuralAction = [&](int code) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        layoutDirty = true;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        bool useRealHeapSnap = (lid == LEVEL_4_3 || lid == LEVEL_4_4 || (lid == LEVEL_4_5 && act4Built));
        pushUndoSnapshot(useRealHeapSnap);

        if (lid == LEVEL_1_5 && code == 1 && !act1Mirrored && !pendingWin) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            if (act1SwappedCount > 0) {
                buildBinaryTreeFromData(act1Tree, d);
            }
            EventBatch batch;
            act1Tree.mirror(batch);
            layoutBtForScreen(false);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            act1Mirrored = true;
            int rem = maxInt(1, 3 - act1SwappedCount);
            scoring.addStep(rem);
            syncScoringHud();
            pendingWin = true;
            status = "Auto Mirror triggered! Playing vault inversion events...";
            pushToast("Auto Mirror running!", TOAST_KIND_SUCCESS);
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        } else if (lid == LEVEL_4_5 && code == 1 && !act4Built) {
            LayoutResult before;
            LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount,
                                          CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act4Heap.buildMaxHeap(heapArr, activeHeapSize, batch);
            act4Built = true;
            hlIndex1 = 0;
            hlIndex2 = -1;
            layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
            animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
            scoring.addStep(1);
            syncScoringHud();
            status = "Max Heap built! Phase 2: Click root [0] repeatedly to extract all treasures!";
            pushToast("Max Heap built! Now click root [0] to extract", TOAST_KIND_SUCCESS);
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        }
    };
