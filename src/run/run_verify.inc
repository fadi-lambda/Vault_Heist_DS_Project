
    // Optional automated screenshot verification mode (--verify-screenshots <dir>)
    if (!verifyDir.empty()) {
        auto saveShot = [&](const std::string& name, const UiInput& in) {
            while (const auto ev = window.pollEvent()) {
                (void)ev;
            }
            renderFrame(in, verifyDir + "/" + name + ".png");
        };

        UiInput idle;
        idle.mx = 800.f; idle.my = 450.f;

        currentScreen = SCREEN_MAIN_MENU;
        saveShot("01_main_menu", idle);

        currentScreen = SCREEN_CAMPAIGN_MAP;
        saveShot("02_campaign_map", idle);

        // Unlock & inspect Level 2.2 briefing
        startLevel(6);
        saveShot("03_level_briefing", idle);

        // Dismiss briefing, hover root 50 & trigger a toast
        showingBriefing = false;
        layoutBstForScreen(false);
        if (bstLayout.nodeCount > 0) {
            idle.mx = bstLayout.nodes[bstLayout.nodeCount / 2].x;
            idle.my = bstLayout.nodes[bstLayout.nodeCount / 2].y;
        }
        pushToast("Clicked Leaf 80 cleanly!", TOAST_KIND_SUCCESS);
        toastQueue.update(0.25f);
        saveShot("04_bst_node_play_toast_tooltip", idle);

        // Level 3.4 AVL with BF badges & incoming prompt
        toastQueue.clear();
        startLevel(13);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        idle.mx = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].x : 600.f;
        idle.my = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].y : 300.f;
        saveShot("05_avl_badges", idle);

        // Level 4.1 Heap + synchronised array view
        startLevel(15);
        showingBriefing = false;
        layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        idle.mx = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].x : 600.f;
        idle.my = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].y : 300.f;
        saveShot("06_heap_array_sync", idle);

        currentScreen = SCREEN_LEADERBOARD;
        saveShot("07_leaderboard", idle);

        currentScreen = SCREEN_SETTINGS;
        saveShot("08_settings", idle);

        // Zoomed & panned canvas verification (Part B + Part C Figma grid + screen-fixed HUD/tooltip)
        toastQueue.clear();
        startLevel(6);
        showingBriefing = false;
        layoutBstForScreen(false);
        layoutDirty = false;
        if (bstLayout.nodeCount > 0) {
            sf::Vector2i zoomAnchor = window.mapCoordsToPixel({bstLayout.nodes[0].x, bstLayout.nodes[0].y}, levelCam.view);
            levelCam.handleZoom(window, zoomAnchor, 1.f);
            levelCam.handleZoom(window, zoomAnchor, 1.f);
            levelCam.handleZoom(window, zoomAnchor, 1.f); // ~1.52x zoom toward root 50
            levelCam.view.move({45.f, -22.f});            // pan slightly
            levelCam.clampCenter();
            sf::Vector2i hovPx = window.mapCoordsToPixel({bstLayout.nodes[0].x, bstLayout.nodes[0].y}, levelCam.view);
            idle.mx = static_cast<float>(hovPx.x);
            idle.my = static_cast<float>(hovPx.y);
        }
        saveShot("09_canvas_pan_zoom", idle);

        // Act 1 Levels (1.1 through 1.5) visual verification
        toastQueue.clear();
        startLevel(0); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        saveShot("10_act1_1_traversals", idle);

        startLevel(1); showingBriefing = false;
        saveShot("11_act1_2_blueprint_dragdrop", idle);

        startLevel(2); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        act1StepperVal = 1; submitAct1HeightOrDiameter();
        act1StepperVal = 1; submitAct1HeightOrDiameter();
        saveShot("12_act1_3_heights_diameter", idle);

        startLevel(3); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        if (btLayout.findNodeIndexByKey(20) >= 0) handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(20)].id, 20);
        if (btLayout.findNodeIndexByKey(40) >= 0) handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(40)].id, 40);
        saveShot("13_act1_4_lca_path", idle);

        startLevel(4); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        if (btLayout.findNodeIndexByKey(30) >= 0) handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(30)].id, 30);
        saveShot("14_act1_5_mirror_vault", idle);

        // Act 2 Levels (2.1 through 2.5) visual verification
        toastQueue.clear();
        startLevel(5); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2CompareChoice(1); // 40 < 50 -> move to 30
        saveShot("15_act2_1_search_heist", idle);

        startLevel(6); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2CompareChoice(1); // 25 < 50 -> 30
        handleAct2CompareChoice(1); // 25 < 30 -> 20
        handleAct2CompareChoice(3); // 25 > 20 -> insert 25
        saveShot("16_act2_2_stash_the_loot", idle);

        startLevel(7); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2DeleteCase(1); // Leaf 80
        animCtrl.finishInstant();
        handleAct2DeleteCase(2); // One-Child 70
        animCtrl.finishInstant();
        handleAct2DeleteCase(3); // Two-Children 30 -> Phase 1 (successor swap)
        animCtrl.finishInstant();
        saveShot("17_act2_3_steal_and_collapse", idle);

        startLevel(8); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2RangeDecision(RQ_ACT_REPORT);     // Report 50
        handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT); // Prune 10 (greys out 10)
        handleAct2RangeDecision(RQ_ACT_REPORT);     // Report 35
        animCtrl.finishInstant();
        saveShot("18_act2_4_range_raid", idle);

        startLevel(9); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        for (int s = 0; s < 4; ++s) {
            for (int r = 0; r <= s; ++r) handleAct2CompareChoice(3); // walk down right spine + insert
        }
        animCtrl.finishInstant();
        saveShot("19_act2_5_lazy_architect_boss", idle);

        // Act 3 Levels (3.1 through 3.5) visual verification
        toastQueue.clear();
        startLevel(10); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (avlLayout.findNodeIndexByKey(30) >= 0) handleLevelNodeClick(avlLayout.nodes[avlLayout.findNodeIndexByKey(30)].id, 30);
        saveShot("20_act3_1_read_badges", idle);

        startLevel(11); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[11].puzzleCount > 0) {
            int pk = levelData[11].puzzles[0].rootKey;
            int pi = avlLayout.findNodeIndexByKey(pk);
            if (pi >= 0) handleLevelNodeClick(avlLayout.nodes[pi].id, pk);
        }
        saveShot("21_act3_2_single_rotations", idle);

        startLevel(12); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        handleAct3DoubleCase(1); // Identify LR case -> highlights heavy child pivot step
        saveShot("22_act3_3_double_rotations", idle);

        // Report: force the pure RR (3.2 puzzle 2) and RL (3.3 puzzle 2) cases so all four rotation types are captured
        startLevel(11); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[11].puzzleCount > 1) {
            const LevelPuzzleSpec& sp0 = levelData[11].puzzles[0];
            int si0 = avlLayout.findNodeIndexByKey(sp0.rootKey);
            if (si0 >= 0) handleLevelNodeClick(avlLayout.nodes[si0].id, sp0.rootKey);
            handleAct3SingleRotation(sp0.caseName == "LL" ? 1 : 2);
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
            const LevelPuzzleSpec& sp1 = levelData[11].puzzles[1];
            int si1 = avlLayout.findNodeIndexByKey(sp1.rootKey);
            if (si1 >= 0) handleLevelNodeClick(avlLayout.nodes[si1].id, sp1.rootKey);
        }
        saveShot("22b_act3_2_rr_rotation", idle);

        startLevel(12); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[12].puzzleCount > 1) {
            const LevelPuzzleSpec& dp0 = levelData[12].puzzles[0];
            handleAct3DoubleCase(dp0.caseName == "LR" ? 1 : 2);
            int dc = avlLayout.findNodeIndexByKey(dp0.childKey);
            if (dc >= 0) handleLevelNodeClick(avlLayout.nodes[dc].id, dp0.childKey);
            animCtrl.finishInstant();
            int dr = avlLayout.findNodeIndexByKey(dp0.rootKey);
            if (dr >= 0) handleLevelNodeClick(avlLayout.nodes[dr].id, dp0.rootKey);
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
            handleAct3DoubleCase(levelData[12].puzzles[1].caseName == "LR" ? 1 : 2);
        }
        saveShot("22c_act3_3_rl_rotation", idle);

        startLevel(13); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[13].insertCount >= 2) {
            handleAct3Prediction(levelData[13].inserts[0].expectedRotation.c_str()); animCtrl.finishInstant();
            handleAct3Prediction(levelData[13].inserts[1].expectedRotation.c_str()); animCtrl.finishInstant();
        }
        saveShot("23_act3_4_predict_insert", idle);

        startLevel(14); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[14].kQueryCount > 0) {
            int dummyC = 0;
            int ans1 = kthLargestAvl(avl, levelData[14].kQueries[0], dummyC);
            int idx1 = avlLayout.findNodeIndexByKey(ans1);
            if (idx1 >= 0) handleLevelNodeClick(avlLayout.nodes[idx1].id, ans1);
            animCtrl.finishInstant();
        }
        saveShot("24_act3_5_order_stat_boss", idle);

        // Act 4 Levels (4.3..4.5 ascending sorted tail & 4.4 step/complexity readout)
        toastQueue.clear();
        startLevel(17); showingBriefing = false; layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        for (int e = 0; e < 4; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("25_act4_3_extract_ascending", idle);

        startLevel(18); showingBriefing = false; layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        for (int e = 0; e < 3; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("26_act4_4_race_complexity", idle);

        startLevel(19); showingBriefing = false; layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        triggerStructuralAction(1);
        animCtrl.finishInstant();
        saveShot("26b_act4_5_build_max_heap", idle); // Report: bottom-up buildMaxHeap result before any extraction
        for (int e = 0; e < 4; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("27_act4_5_boss_heapsort", idle);
        // Report: finish every extraction so the array view shows the fully ascending sorted result
        for (int guard = 0; guard < 16 && act4Heap.getSize() > 0 && currentScreen == SCREEN_LEVEL_PLAY; ++guard) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("27b_act4_5_sorted_result", idle);

        // Endless Mode & Endless Leaderboard visual verification (in-memory only)
        endlessLb.load("");
        toastQueue.clear();
        startEndlessMode(424242u);
        for (int s = 0; s < 5; ++s) {
            endlessSession.tick(0.75f);
            triggerEndlessExecute();
            animCtrl.finishInstant();
        }
        saveShot("28_endless_mode_play", idle);

        endlessSession.tick(endlessSession.getOpTimeRemaining() + 0.1f);
        finishEndlessGameOver();
        saveShot("29_endless_leaderboard", idle);

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 1 levels (--verify-act1)
    if (verifyAct1) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };

        // 1.1: All 4 traversal modes + wrong-click +1 alarm penalty
        for (int m = 0; m < 4; ++m) {
            startLevel(0);
            showingBriefing = false;
            act1TravMode = m;
            layoutBtForScreen(false);
            int cnt = 0;
            int* seq = getAct1Traversal(act1Tree, m, cnt);
            if (!seq || cnt != 7) return failCheck("1.1 sequence length != 7");
            if (m == 0) {
                int wrongKey = seq[1];
                int wIdx = btLayout.findNodeIndexByKey(wrongKey);
                handleLevelNodeClick(btLayout.nodes[wIdx].id, wrongKey);
                if (scoring.alarm() != 1) return failCheck("1.1 wrong traversal click must add +1 alarm");
            }
            for (int i = 0; i < cnt; ++i) {
                int nIdx = btLayout.findNodeIndexByKey(seq[i]);
                handleLevelNodeClick(btLayout.nodes[nIdx].id, seq[i]);
            }
            delete[] seq;
            completePendingWin();
            if (currentScreen != SCREEN_RESULT) return failCheck("1.1 did not reach result");
            if (m > 0 && scoring.stars() != 3) return failCheck("1.1 clean traversal should earn 3 stars");
            if (m == 0 && scoring.stars() != 2) return failCheck("1.1 single-penalty run should earn 2 stars");
        }
        std::cout << "PASS 1.1: All 4 traversal modes & +1 penalty verified\n";

        // 1.2: Drag-and-drop reconstruction (wrong placement penalty + clean solve)
        startLevel(1);
        showingBriefing = false;
        for (int s = 0; s < dragDrop.slotCount(); ++s) {
            int tokIdx = (s + 1) % dragDrop.tokenCount();
            const DragToken* tk = dragDrop.token(tokIdx);
            const DropSlot* sl = dragDrop.slot(s);
            dragDrop.press(tk->x, tk->y);
            dragDrop.move(sl->x, sl->y);
            DropResult r = dragDrop.release(sl->x, sl->y);
            handleAct1BlueprintDrop(r);
        }
        if (scoring.penaltyEvents() != 1 || scoring.alarm() != 8) return failCheck("1.2 wrong placement penalty mismatch");

        startLevel(1);
        showingBriefing = false;
        for (int s = 0; s < btLayout.nodeCount; ++s) {
            int wantedKey = btLayout.nodes[s].key;
            int tokIdx = -1;
            for (int t = 0; t < dragDrop.tokenCount(); ++t) {
                if (dragDrop.token(t)->value == wantedKey) { tokIdx = t; break; }
            }
            if (tokIdx < 0) return failCheck("1.2 missing token for key");
            const DragToken* tk = dragDrop.token(tokIdx);
            const DropSlot* sl = dragDrop.slot(s);
            dragDrop.press(tk->x, tk->y);
            dragDrop.move(sl->x, sl->y);
            DropResult r = dragDrop.release(sl->x, sl->y);
            handleAct1BlueprintDrop(r);
        }
        if (currentScreen != SCREEN_RESULT || !act1Reconstructed || scoring.stars() != 3) {
            return failCheck("1.2 clean drag-drop reconstruction failed");
        }
        std::cout << "PASS 1.2: Drag-drop Pre+In reconstruction verified (3 stars)\n";

        // 1.3: Postorder heights + diameter stepper
        startLevel(2);
        showingBriefing = false;
        layoutBtForScreen(false);
        act1StepperVal = 4; // wrong height for leaf
        submitAct1HeightOrDiameter();
        if (scoring.alarm() != 1) return failCheck("1.3 wrong height must add +1 alarm");

        startLevel(2);
        showingBriefing = false;
        layoutBtForScreen(false);
        {
            int postCnt = 0;
            int* postSeq = act1Tree.postorderRecursive(postCnt);
            for (int i = 0; i < postCnt; ++i) {
                act1StepperVal = treeHeightNodes(findTreeNodeConst(act1Tree.getRoot(), postSeq[i]));
                submitAct1HeightOrDiameter();
            }
            delete[] postSeq;
            act1StepperVal = act1Tree.diameter();
            submitAct1HeightOrDiameter();
        }
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) return failCheck("1.3 clean postorder height+diameter failed");
        std::cout << "PASS 1.3: Postorder heights + diameter verified (3 stars)\n";

        // 1.4: Pick 2 rooms -> click LCA -> trace path (no Boss label)
        if (levels[3].title.find("Boss") != std::string::npos) return failCheck("1.4 must not have Boss label");
        startLevel(3);
        showingBriefing = false;
        layoutBtForScreen(false);
        {
            int seq[] = { 20, 80, 50, 20, 30, 50, 70, 80 };
            for (int k : seq) {
                int idx = btLayout.findNodeIndexByKey(k);
                handleLevelNodeClick(btLayout.nodes[idx].id, k);
            }
        }
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) return failCheck("1.4 route trace failed");
        std::cout << "PASS 1.4: 2-room pick + LCA + path trace verified (3 stars)\n";

        // 1.5: Bottom-up swap + penalty on top-down click + Auto Mirror alternative
        startLevel(4);
        showingBriefing = false;
        layoutBtForScreen(false);
        handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(50)].id, 50); // top-down rejected
        if (scoring.alarm() != 1) return failCheck("1.5 top-down click must trigger +1 alarm");

        startLevel(4);
        showingBriefing = false;
        layoutBtForScreen(false);
        {
            int swaps[] = { 30, 70, 50 };
            for (int k : swaps) {
                int idx = btLayout.findNodeIndexByKey(k);
                handleLevelNodeClick(btLayout.nodes[idx].id, k);
            }
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            !act1Tree.getRoot() || act1Tree.getRoot()->left->value != 70) {
            return failCheck("1.5 bottom-up mirror failed");
        }

        startLevel(4);
        showingBriefing = false;
        layoutBtForScreen(false);
        triggerStructuralAction(1);
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            !act1Tree.getRoot() || act1Tree.getRoot()->left->value != 70) {
            return failCheck("1.5 Auto Mirror option failed");
        }
        std::cout << "PASS 1.5: Bottom-up click-swap and Auto Mirror both verified (3 stars)\n";

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 2 levels (--verify-act2)
    if (verifyAct2) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };

        // 2.1: Search Heist (root-to-target L/R/Found + wrong choice +1 penalty)
        startLevel(5);
        showingBriefing = false;
        layoutBstForScreen(false);
        handleAct2CompareChoice(3); // wrong: 40 < 50
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("2.1 wrong direction must add +1 alarm");

        startLevel(5);
        showingBriefing = false;
        handleAct2CompareChoice(1); // at 50 -> Left to 30
        handleAct2CompareChoice(3); // at 30 -> Right to 40
        handleAct2CompareChoice(2); // at 40 -> Found
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 3) {
            return failCheck("2.1 Search Heist clean run failed");
        }
        std::cout << "PASS 2.1: Search Heist root-to-target comparisons verified (3 stars)\n";

        // 2.2: Stash the Loot (walk-to-slot insert 25 + duplicate 40 rejected)
        startLevel(6);
        showingBriefing = false;
        handleAct2CompareChoice(2); // wrong at root 50 when inserting 25
        if (scoring.alarm() != 1) return failCheck("2.2 wrong choice must add +1 alarm");

        startLevel(6);
        showingBriefing = false;
        handleAct2CompareChoice(1); // 25 < 50 -> 30
        handleAct2CompareChoice(1); // 25 < 30 -> 20
        handleAct2CompareChoice(3); // 25 > 20 -> empty right slot -> insert 25
        if (!act2Bst.search(25) || !act2Bst.isValidBST() || act2Bst.getSize() != 8) {
            return failCheck("2.2 insert 25 failed");
        }
        handleAct2CompareChoice(1); // 40 < 50 -> 30
        handleAct2CompareChoice(3); // 40 > 30 -> 40
        handleAct2CompareChoice(2); // 40 == 40 -> Reject Duplicate
        if (act2Bst.getSize() != 8) return failCheck("2.2 duplicate 40 must be rejected");
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 6) {
            return failCheck("2.2 Stash the Loot clean run failed");
        }
        std::cout << "PASS 2.2: Stash the Loot insert & duplicate rejection verified (3 stars)\n";

        // 2.3: Steal and Collapse (verify all 3 distinct cases: Leaf 80, One-Child 70, Two-Children 30)
        startLevel(7);
        showingBriefing = false;
        if (bstDeleteCase(findTreeNodeConst(act2Bst.getRoot(), 80)) != 1) return failCheck("2.3: 80 must be Leaf");
        handleAct2DeleteCase(3); // wrong case penalty
        if (scoring.alarm() != 1) return failCheck("2.3 wrong delete case must add +1 alarm");

        startLevel(7);
        showingBriefing = false;
        handleAct2DeleteCase(1); // Delete 80 (Leaf)
        if (act2Bst.search(80) || bstDeleteCase(findTreeNodeConst(act2Bst.getRoot(), 70)) != 2) {
            return failCheck("2.3: after deleting 80, 70 must have exactly One-Child (60)");
        }
        handleAct2DeleteCase(2); // Delete 70 (One-Child)
        if (act2Bst.search(70) || bstDeleteCase(findTreeNodeConst(act2Bst.getRoot(), 30)) != 3) {
            return failCheck("2.3: after deleting 70, 30 must still have Two-Children (20 & 40)");
        }
        handleAct2DeleteCase(3); // Delete 30 (Two-Children) -> Phase 1 (Swap Value)
        if (!act2TwoChildPhase2Pending) return failCheck("2.3 two-child delete must split into Phase 1 swap & Phase 2 unlink");
        // Verify Phase 1 mid-layout visibly shows successor value 40 copied into target slot while both exist
        if (act2MidLayout.nodeCount != 5 || act2MidLayout.nodes[act2MidLayout.findNodeIndexById(2)].key != 40) {
            return failCheck("2.3 Phase 1 mid-layout must show swapped successor value");
        }
        animCtrl.finishInstant();
        startAct2TwoChildPhase2(); // Phase 2 (Unlink Successor)
        animCtrl.finishInstant();
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || act2Bst.search(30) || !act2Bst.isValidBST() || scoring.stars() != 3) {
            return failCheck("2.3 Steal and Collapse two-phase delete failed");
        }
        std::cout << "PASS 2.3: Steal & Collapse (Leaf + 1-Child + 2-Child 2-step animation) verified (3 stars)\n";

        // 2.4: Range Raid (Report + Prune Left/Right + grey-out + +50 bonus per prune)
        startLevel(8);
        showingBriefing = false;
        handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT); // wrong at root 50
        if (scoring.alarm() != 1) return failCheck("2.4 wrong range decision must add +1 alarm");

        startLevel(8);
        showingBriefing = false;
        handleAct2RangeDecision(RQ_ACT_REPORT);      // 50
        handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT);  // prune 10 (+50 bonus)
        handleAct2RangeDecision(RQ_ACT_REPORT);      // 35
        handleAct2RangeDecision(RQ_ACT_PRUNE_RIGHT); // prune 90 (+50 bonus)
        handleAct2RangeDecision(RQ_ACT_REPORT);      // 65
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            scoring.bonusPoints() != 100 || act2PrunedCount != 2) {
            return failCheck("2.4 Range Raid failed");
        }
        std::cout << "PASS 2.4: Range Raid pruning & bonuses (+100 bonus) verified (3 stars)\n";

        // 2.5: Boss: The Lazy Architect (sorted chain + shrinking alarm + O(n) search + Act 3 cutscene)
        startLevel(9);
        showingBriefing = false;
        int opt25 = computeOptimalSteps(levelData[9]);
        if (scoring.capacity() >= ScoringEngine::capacityFor(opt25)) {
            return failCheck("2.5 boss initial alarm capacity must be tighter than normal formula");
        }
        for (int s = 0; s < 4; ++s) {
            for (int r = 0; r <= s; ++r) handleAct2CompareChoice(3); // Go Right down chain
        }
        if (act2Bst.getHeight() != 5 || act2Bst.getSize() != 5 || act2BossStage != 1 ||
            scoring.capacity() != 17) {
            return failCheck("2.5 degenerate chain build or shrinking capacity mismatch");
        }
        // Phase 2: Search for 50 down the height-5 chain (10 -> 20 -> 30 -> 40 -> 50 -> Found)
        for (int r = 0; r < 4; ++r) handleAct2CompareChoice(3);
        handleAct2CompareChoice(2); // Found 50
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 15 ||
            status.find("AVL") == std::string::npos) {
            return failCheck("2.5 Boss completion or Act 3 balancing note missing");
        }
        std::cout << "PASS 2.5: Lazy Architect Boss (Height=5, Cap shrank 21->17, 3 stars) verified\n";

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 3 levels (--verify-act3)
    if (verifyAct3) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };

        // 3.1: Read the Badges (+1 traversal penalty on balanced node, no -50 rot penalty; 3 stars clean)
        startLevel(10);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int idx50 = avlLayout.findNodeIndexByKey(50);
            if (idx50 < 0) return failCheck("3.1 missing balanced node 50");
            handleLevelNodeClick(avlLayout.nodes[idx50].id, 50);
            if (scoring.alarm() != 1 || scoring.score() != (SCORE_BASE - SCORE_PER_ALARM_STEP) ||
                scoring.penaltyEvents() != 1) {
                return failCheck("3.1 wrong badge click must use +1 alarm (no -50 rot penalty)");
            }
        }
        startLevel(10);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int idx30 = avlLayout.findNodeIndexByKey(30);
            int idx20 = avlLayout.findNodeIndexByKey(20);
            if (idx30 < 0 || idx20 < 0) return failCheck("3.1 missing violation nodes 30/20");
            handleLevelNodeClick(avlLayout.nodes[idx30].id, 30);
            handleLevelNodeClick(avlLayout.nodes[idx20].id, 20);
            if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 2) {
                return failCheck("3.1 Read the Badges clean run failed");
            }
        }
        std::cout << "PASS 3.1: Read the Badges (real AVL BF & +1 traversal penalty) verified (3 stars)\n";

        // 3.2: Manual Repair LL/RR (pivot click + LL/RR choice + real AVLTree rotations + +2/-50 penalty)
        startLevel(11);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            const LevelPuzzleSpec& p0 = levelData[11].puzzles[0];
            int idxChild = avlLayout.findNodeIndexByKey(p0.childKey);
            if (idxChild < 0) return failCheck("3.2 missing puzzle 0 child");
            handleLevelNodeClick(avlLayout.nodes[idxChild].id, p0.childKey); // balanced -> PENALTY_WRONG_ROTATION
            if (scoring.alarm() != 2 ||
                scoring.score() != (SCORE_BASE - 2 * SCORE_PER_ALARM_STEP - SCORE_ROTATION_PENALTY)) {
                return failCheck("3.2 balanced pivot click must apply wrong rotation penalty (+2/-50)");
            }
            int idxRoot = avlLayout.findNodeIndexByKey(p0.rootKey);
            handleLevelNodeClick(avlLayout.nodes[idxRoot].id, p0.rootKey);
            handleAct3SingleRotation(p0.caseName == "LL" ? 2 : 1); // wrong LL/RR button
            if (scoring.alarm() != 4 ||
                scoring.score() != (SCORE_BASE - 4 * SCORE_PER_ALARM_STEP - 2 * SCORE_ROTATION_PENALTY)) {
                return failCheck("3.2 wrong LL/RR choice must apply +2/-50 penalty");
            }
        }
        startLevel(11);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        for (int pIdx = 0; pIdx < levelData[11].puzzleCount; ++pIdx) {
            const LevelPuzzleSpec& p = levelData[11].puzzles[pIdx];
            int rIdx = avlLayout.findNodeIndexByKey(p.rootKey);
            if (rIdx < 0) return failCheck("3.2 missing puzzle root");
            handleLevelNodeClick(avlLayout.nodes[rIdx].id, p.rootKey);
            if (!act3PivotPicked || act3PivotKey != p.rootKey) return failCheck("3.2 failed to select pivot");
            handleAct3SingleRotation(p.caseName == "LL" ? 1 : 2);
            int badKey = -1;
            if (!avl.isValidAVL() || !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.2 real AVL single rotation left invalid tree or subtree size");
            }
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("3.2 clean LL/RR solve failed");
        }
        std::cout << "PASS 3.2: Manual Repair LL & RR via real AVLTree rotations verified (3 stars)\n";

        // 3.3: Double Rotations LR/RL (identify case -> child pivot -> root pivot; two real single rotations)
        startLevel(12);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        handleAct3DoubleCase(2); // LR puzzle picked as RL -> PENALTY_WRONG_ROTATION
        if (scoring.alarm() != 2 ||
            scoring.score() != (SCORE_BASE - 2 * SCORE_PER_ALARM_STEP - SCORE_ROTATION_PENALTY)) {
            return failCheck("3.3 wrong zig-zag case identification must apply +2/-50 penalty");
        }
        startLevel(12);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        for (int pIdx = 0; pIdx < levelData[12].puzzleCount; ++pIdx) {
            const LevelPuzzleSpec& p = levelData[12].puzzles[pIdx];
            handleAct3DoubleCase(p.caseName == "LR" ? 1 : 2);
            int cIdx = avlLayout.findNodeIndexByKey(p.childKey);
            if (cIdx < 0) return failCheck("3.3 missing child pivot");
            handleLevelNodeClick(avlLayout.nodes[cIdx].id, p.childKey);
            animCtrl.finishInstant();
            int rIdx = avlLayout.findNodeIndexByKey(p.rootKey);
            if (rIdx < 0) return failCheck("3.3 missing root pivot");
            handleLevelNodeClick(avlLayout.nodes[rIdx].id, p.rootKey);
            int badKey = -1;
            if (!avl.isValidAVL() || !avl.getRoot() || avl.getRoot()->key != p.innerKey ||
                !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.3 real AVL double rotation produced invalid tree or wrong root");
            }
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("3.3 clean LR/RL solve failed");
        }
        std::cout << "PASS 3.3: Double Rotations LR & RL via case ID + two real rotations verified (3 stars)\n";

        // 3.4: Auto Mode & Prediction (predict LL/RR/LR/RL/None before real avl.insert)
        startLevel(13);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            const std::string& exp0 = levelData[13].inserts[0].expectedRotation;
            handleAct3Prediction(exp0 == "LL" ? "RR" : "LL");
            if (scoring.alarm() != 2 ||
                scoring.score() != (SCORE_BASE - 2 * SCORE_PER_ALARM_STEP - SCORE_ROTATION_PENALTY)) {
                return failCheck("3.4 wrong prediction must apply +2/-50 penalty");
            }
        }
        startLevel(13);
        showingBriefing = false;
        for (int i = 0; i < levelData[13].insertCount; ++i) {
            handleAct3Prediction(levelData[13].inserts[i].expectedRotation.c_str());
            int badKey = -1;
            if (!avl.isValidAVL() || !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.4 real AVL insert produced invalid tree or broken subtree size");
            }
            animCtrl.finishInstant();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("3.4 clean rotation prediction solve failed");
        }
        std::cout << "PASS 3.4: Rotation Prediction (LL/RR/LR/RL/None) + real AVL insert verified (3 stars)\n";

        // 3.5: Order-Statistic Boss (k-th MOST VALUABLE / LARGEST + live rotation subtree size validation + O(log n))
        startLevel(14);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int idx30 = avlLayout.findNodeIndexByKey(30);
            if (idx30 < 0) return failCheck("3.5 missing node 30");
            handleLevelNodeClick(avlLayout.nodes[idx30].id, 30);
            if (scoring.alarm() != 1) return failCheck("3.5 wrong click must add +1 alarm");
        }
        startLevel(14);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int c1 = 0;
            int k1 = kthLargestAvl(avl, levelData[14].kQueries[0], c1);
            if (k1 != 50 || c1 > avl.getHeight()) return failCheck("3.5 challenge 1 k-th largest mismatch (expected 50)");
            handleLevelNodeClick(avlLayout.nodes[avlLayout.findNodeIndexByKey(k1)].id, k1);
            animCtrl.finishInstant();
            int badKey = -1;
            if (!avl.isValidAVL() || avl.getSize() != 9 || !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.5 live key-22 rotation failed subtree size validation");
            }
            int c2 = 0;
            int k2 = kthLargestAvl(avl, levelData[14].kQueries[1], c2);
            if (k2 != 25 || c2 > avl.getHeight()) return failCheck("3.5 challenge 2 k-th largest mismatch (expected 25)");
            handleLevelNodeClick(avlLayout.nodes[avlLayout.findNodeIndexByKey(k2)].id, k2);
            animCtrl.finishInstant();
            completePendingWin();
            if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || !act3SizeOk ||
                act3LastComps <= 0 || act3LastComps > act3LastHeight) {
                return failCheck("3.5 Order-Statistic Boss verification failed");
            }
        }
        std::cout << "PASS 3.5: Order-Statistic Boss (k-th largest {50,25}, O(log n) comps, live rotation sz check) verified (3 stars)\n";

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 4 levels (--verify-act4)
    if (verifyAct4) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };
        auto checkArrayMaxHeap = [&](const int* arr, int n) -> bool {
            for (int i = 1; i < n; ++i) {
                if (arr[(i - 1) / 2] < arr[i]) return false;
            }
            return true;
        };

        // 4.1: Activate the Vault (bottom-up build + invalid penalty + 3 stars)
        startLevel(15);
        showingBriefing = false;
        handleLevelNodeClick(0, heapArr[0]); // wrong order click (expected 2 or 6) -> PENALTY_ACT4_INVALID
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("4.1 wrong click must add +1 alarm");

        startLevel(15);
        showingBriefing = false;
        while (buildStepIdx >= 0) {
            handleLevelNodeClick(buildStepIdx, heapArr[buildStepIdx]);
            animCtrl.finishInstant();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || !checkArrayMaxHeap(heapArr, activeHeapSize)) {
            return failCheck("4.1 clean bottom-up build failed");
        }
        std::cout << "PASS 4.1: Bottom-up Max Heap construction & +1 penalty verified (3 stars)\n";

        // 4.2: Repair Security Locks (sift-down root + invalid penalty + 3 stars)
        startLevel(16);
        showingBriefing = false;
        handleLevelNodeClick(4, heapArr[4]); // wrong node -> PENALTY_ACT4_INVALID
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("4.2 wrong click must add +1 alarm");

        startLevel(16);
        showingBriefing = false;
        handleLevelNodeClick(buildStepIdx, heapArr[buildStepIdx]);
        animCtrl.finishInstant();
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || !checkArrayMaxHeap(heapArr, activeHeapSize)) {
            return failCheck("4.2 root sift-down repair failed");
        }
        std::cout << "PASS 4.2: Sabotaged root sift-down repair verified (3 stars)\n";

        // 4.3: Extract the Greatest Treasure (reject deliberately wrong sorted order, accept ascending tail)
        startLevel(17);
        showingBriefing = false;
        for (int e = 0; e < 6; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        sortedArr[0] = 999; // tamper sorted region before final extraction
        handleLevelNodeClick(0, act4Heap.getMax());
        animCtrl.finishInstant();
        completePendingWin();
        if (pendingWin || currentScreen == SCREEN_RESULT || scoring.penaltyEvents() < 1) {
            return failCheck("4.3 must reject mismatched sortedArr instead of winning");
        }

        startLevel(17);
        showingBriefing = false;
        while (!act4Heap.isEmpty()) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (!pendingWin || sortedCount != levelData[17].expectedSortedResultCount ||
            !validateAct4SortedResult(levelData[17], sortedArr, sortedCount)) {
            return failCheck("4.3 extracted sorted region is not strictly ascending expected_sorted_result");
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("4.3 clean optimal run must earn 3 stars");
        }
        std::cout << "PASS 4.3: Ascending tail [40..95] & expected_sorted_result mismatch guard verified (3 stars)\n";

        // 4.4: Race Against the Guards (complexity/step HUD readout + ascending extraction + 3 stars)
        startLevel(18);
        showingBriefing = false;
        if (status.find("0/7") == std::string::npos || status.find("O(log n)") == std::string::npos) {
            return failCheck("4.4 initial status missing step/O(log n) complexity readout");
        }
        handleLevelNodeClick(2, act4Heap.getElementAt(2)); // wrong click != root 0
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("4.4 non-root click must add +1 alarm");

        startLevel(18);
        showingBriefing = false;
        for (int e = 0; e < 3; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (status.find("3/7") == std::string::npos || status.find("O(log n)") == std::string::npos) {
            return failCheck("4.4 live status missing step/complexity progress readout");
        }
        while (!act4Heap.isEmpty()) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (!validateAct4SortedResult(levelData[18], sortedArr, sortedCount)) {
            return failCheck("4.4 ascending sorted result mismatch");
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            status.find("7/7") == std::string::npos || status.find("O(log n)") == std::string::npos) {
            return failCheck("4.4 clean optimal race failed or missing final complexity summary");
        }
        std::cout << "PASS 4.4: Race Against the Guards live Theta(n log n) step readout & ascending tail verified (3 stars)\n";

        // 4.5: Final Vault Boss (wrong sortedArr guard + Build Max Heap + 8 extractions = 9 optimal steps -> 3 stars)
        startLevel(19);
        showingBriefing = false;
        triggerStructuralAction(1);
        animCtrl.finishInstant();
        for (int e = 0; e < 7; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        sortedArr[0] = 1; // deliberately break ascending order before last extraction
        handleLevelNodeClick(0, act4Heap.getMax());
        animCtrl.finishInstant();
        completePendingWin();
        if (pendingWin || currentScreen == SCREEN_RESULT || scoring.penaltyEvents() < 1) {
            return failCheck("4.5 boss must reject wrong sorted array against expected_sorted_result");
        }

        startLevel(19);
        showingBriefing = false;
        triggerStructuralAction(1); // Phase 1: Build Max Heap (1 step)
        animCtrl.finishInstant();
        while (!act4Heap.isEmpty()) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (!validateAct4SortedResult(levelData[19], sortedArr, sortedCount)) {
            return failCheck("4.5 boss sorted tail did not match ascending expected_sorted_result");
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.steps() != 9) {
            return failCheck("4.5 boss clean optimal run failed to earn 3 stars in 9 steps");
        }
        std::cout << "PASS 4.5: Final Vault Boss (Build Max Heap + ascending tail + mismatch guard) verified (3 stars)\n";

        return 0;
    }

    // End-to-end automated sanity check for Endless Mode (--verify-endless)
    if (verifyEndless) {
        const char* tmpSave = "verify_endless_save_tmp.txt";
        const char* tmpLb   = "verify_endless_lb_tmp.txt";
        std::remove(tmpSave);
        std::remove(tmpLb);
        scoreLog.load("");
        endlessLb.load(tmpLb);

        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            std::remove(tmpSave);
            std::remove(tmpLb);
            return 1;
        };

        startEndlessMode(424242u);
        if (currentScreen != SCREEN_ENDLESS || !endlessSession.isActive() || !EndlessSession::validateTreeFull(avl)) {
            return failCheck("Endless startNew failed to enter SCREEN_ENDLESS with valid AVL");
        }

        float w0 = endlessSession.getOpWindowSec();
        int prevSurvMs = 0;
        for (int step = 0; step < 12; ++step) {
            const EndlessOp op = endlessSession.getPendingOp();
            if (op.type == ENDLESS_OP_DELETE && !avl.search(op.key)) {
                return failCheck("Endless DELETE op targeted absent key");
            }
            if (op.type == ENDLESS_OP_INSERT && avl.search(op.key)) {
                return failCheck("Endless INSERT op targeted duplicate key");
            }
            if (endlessSession.tick(0.55f)) {
                return failCheck("Endless expired unexpectedly inside active window");
            }
            if (endlessSession.getSurvivalMs() <= prevSurvMs) {
                return failCheck("Endless survival time failed to increment");
            }
            prevSurvMs = endlessSession.getSurvivalMs();
            triggerEndlessExecute();
            animCtrl.finishInstant();
            if (!EndlessSession::validateTreeFull(avl)) {
                return failCheck("AVLTree validator failed after Endless operation");
            }
        }
        if (endlessSession.getOpsCompleted() != 12 || endlessSession.getOpWindowSec() >= w0) {
            return failCheck("Endless difficulty scaling (shrinking window) or op count mismatch");
        }

        int savedSize = avl.getSize();
        int savedRoot = avl.getRoot() ? avl.getRoot()->key : -1;
        int savedSurvMs = endlessSession.getSurvivalMs();
        EndlessOp savedNextOp = endlessSession.getPendingOp();
        saveEndlessRun(tmpSave);

        avl.clear();
        if (!loadEndlessRun(tmpSave)) return failCheck("Endless loadFromFile failed");
        if (!EndlessSession::validateTreeFull(avl) || avl.getSize() != savedSize ||
            !avl.getRoot() || avl.getRoot()->key != savedRoot ||
            endlessSession.getInitialSeed() != 424242u ||
            endlessSession.getOpsCompleted() != 12 ||
            endlessSession.getSurvivalMs() != savedSurvMs ||
            endlessSession.getPendingOp().type != savedNextOp.type ||
            endlessSession.getPendingOp().key != savedNextOp.key) {
            return failCheck("Endless save/load did not faithfully restore AVL tree, seed, or state");
        }

        for (int step = 0; step < 3; ++step) {
            endlessSession.tick(0.30f);
            triggerEndlessExecute();
            animCtrl.finishInstant();
            if (!EndlessSession::validateTreeFull(avl)) return failCheck("Resumed Endless run broke AVL invariants");
        }

        if (!endlessSession.tick(endlessSession.getOpTimeRemaining() + 0.1f)) {
            return failCheck("Endless deadline expiry failed to trigger game-over");
        }
        finishEndlessGameOver();
        if (currentScreen != SCREEN_ENDLESS_LEADERBOARD || !endlessSession.isGameOver()) {
            return failCheck("Game over did not transition to SCREEN_ENDLESS_LEADERBOARD");
        }

        EndlessLeaderboardEntry highRun;
        highRun.nameDate = "VerifyTop";
        highRun.survivalMs = 99000;
        highRun.opsCompleted = 40;
        highRun.seed = 777u;
        endlessLb.recordRun(highRun);

        EndlessLeaderboard checkLb;
        checkLb.load(tmpLb);
        if (checkLb.getCount() != 2 || checkLb.get(0).survivalMs != 99000 ||
            checkLb.get(1).seed != 424242u) {
            return failCheck("Endless leaderboard write/read/descending sort failed");
        }

        std::remove(tmpSave);
        std::remove(tmpLb);
        std::cout << "PASS Endless: Seeded stream, AVL validator, difficulty ramp, preorder+# save/load & leaderboard verified\n";
        return 0;
    }

    // Optional uncapped FPS profiler (--profile-fps)
    if (profileFps) {
        window.setFramerateLimit(0);
        window.setVerticalSyncEnabled(false);
        auto benchScene = [&](const char* label, const UiInput& in) -> double {
            const int WARMUP = 5;
            const int MAX_FRAMES = 300;
            for (int f = 0; f < WARMUP; ++f) {
                while (const auto ev = window.pollEvent()) { (void)ev; }
                renderFrame(in);
            }
            double sumCpu = 0.0, sumDisp = 0.0;
            int actualFrames = 0;
            sf::Clock clk;
            for (int f = 0; f < MAX_FRAMES; ++f) {
                while (const auto ev = window.pollEvent()) { (void)ev; }
                double cpuMs = 0.0, dispMs = 0.0;
                renderFrame(in, "", &cpuMs, &dispMs);
                sumCpu += cpuMs;
                sumDisp += dispMs;
                actualFrames++;
                if (actualFrames >= 15 && clk.getElapsedTime().asSeconds() >= 1.5f) break;
            }
            double secs = static_cast<double>(clk.getElapsedTime().asSeconds());
            double fps = (secs > 0.0) ? (static_cast<double>(actualFrames) / secs) : 0.0;
            double ms = (secs > 0.0) ? (secs * 1000.0 / static_cast<double>(actualFrames)) : 0.0;
            std::cout << label << ": " << fps << " FPS (" << ms << " ms/frame | cpu="
                      << (sumCpu / actualFrames) << " ms, display=" << (sumDisp / actualFrames)
                      << " ms, frames=" << actualFrames << ")\n";
            return fps;
        };

        UiInput in;
        double sumFps = 0.0;

        // 1. Level 2.2 (BST + EventLog + Toast + Tooltip)
        startLevel(6);
        showingBriefing = false;
        {
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.search(80, batch);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, false, false);
        }
        in.mx = (bstLayout.nodeCount > 0) ? bstLayout.nodes[bstLayout.nodeCount / 2].x : 600.f;
        in.my = (bstLayout.nodeCount > 0) ? bstLayout.nodes[bstLayout.nodeCount / 2].y : 300.f;
        pushToast("Clicked Leaf 80 cleanly!", TOAST_KIND_SUCCESS);
        toastQueue.update(0.25f);
        sumFps += benchScene("Level 2.2 (BST + EventLog + Toast + Tooltip)", in);

        // 2. Level 4.1 (MaxHeap + Synchronised Array View + Tooltip)
        toastQueue.clear();
        startLevel(15);
        showingBriefing = false;
        layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        in.mx = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].x : 600.f;
        in.my = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].y : 300.f;
        sumFps += benchScene("Level 4.1 (MaxHeap + Array + Tooltip)", in);

        // 3. Sandbox (Dual AVL + MaxHeap + EventLog)
        enterSandbox();
        {
            LayoutResult before;
            LayoutEngine::layoutAVL(avl, 30.f, 95.f, 560.f, 540.f, before);
            EventBatch batch;
            avl.insert(15, batch);
            layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
            animCtrl.loadOperation(batch, before, avlLayout, true, false);
        }
        in.mx = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].x : 300.f;
        in.my = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].y : 300.f;
        sumFps += benchScene("Sandbox (Dual AVL + MaxHeap + EventLog)", in);

        // 4. Campaign Map (20 level buttons + 4 wing panels)
        toastQueue.clear();
        currentScreen = SCREEN_CAMPAIGN_MAP;
        in.mx = 800.f; in.my = 450.f;
        sumFps += benchScene("Campaign Map (20 buttons + 4 panels)", in);

        std::cout << "Average FPS: " << (sumFps / 4.0) << "\n";
        return 0;
    }
