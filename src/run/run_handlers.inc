
    // Node-click handler for ALL 20 campaign levels
    auto handleLevelNodeClick = [&](int clickedId, int clickedKey) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        layoutDirty = true;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        selectedNodeId = clickedId;

        if (lid == LEVEL_1_1) {
            int cnt = 0;
            int* expSeq = getAct1Traversal(act1Tree, act1TravMode, cnt);
            if (expSeq && act1SeqIdx < cnt) {
                if (clickedKey == expSeq[act1SeqIdx]) {
                    addMarkedId(clickedId);
                    act1SeqIdx++;
                    scoring.addStep(1);
                    syncScoringHud();
                    pushToast(std::string(traversalModeShort(act1TravMode)) + " [" +
                                    std::to_string(act1SeqIdx) + "/" + std::to_string(cnt) +
                                    "]: Visited " + std::to_string(clickedKey), TOAST_KIND_SUCCESS);
                    if (act1SeqIdx == cnt) {
                        LayoutResult before;
                        LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch batch;
                        int animCnt = 0;
                        int* res = getAct1Traversal(act1Tree, act1TravMode, animCnt, &batch);
                        delete[] res;
                        layoutBtForScreen(false);
                        animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
                        pendingWin = true;
                        status = "All " + std::to_string(cnt) + " rooms visited in " +
                                 traversalModeShort(act1TravMode) + "! Playing traversal events...";
                    } else {
                        status = "Visited " + std::to_string(clickedKey) + " (" + std::to_string(act1SeqIdx) +
                                 "/" + std::to_string(cnt) + "). Click next room in " +
                                 traversalModeShort(act1TravMode) + "!";
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Expected " + std::to_string(expSeq[act1SeqIdx]) + " next in " +
                                 traversalModeName(act1TravMode) + ", got " + std::to_string(clickedKey) + "!");
                }
            }
            delete[] expSeq;
        }
        else if (lid == LEVEL_1_2) {
            // Handled via drag-and-drop onto blueprint slots
        }
        else if (lid == LEVEL_1_3) {
            if (act1Hl1 != -9999 && clickedKey != act1Hl1) {
                triggerWrongFlash(clickedId);
                raisePenalty(PENALTY_WRONG_TRAVERSAL,
                             "Postorder processes leaves/children first! Current room is " +
                             std::to_string(act1Hl1) + ".");
            } else {
                status = "Room " + std::to_string(clickedKey) + " selected. Adjust stepper [-]/[+] and press Submit [Enter]!";
            }
        }
        else if (lid == LEVEL_1_4) {
            if (act1LcaStage == 0) {
                act1LcaA = clickedKey;
                act1Hl1 = clickedKey;
                act1LcaStage = 1;
                scoring.addStep(1);
                syncScoringHud();
                pushToast("Selected Room A: " + std::to_string(act1LcaA), TOAST_KIND_SUCCESS);
                status = "Room A = " + std::to_string(act1LcaA) + " selected. Now click a second room (Room B)!";
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
            } else if (act1LcaStage == 1) {
                if (clickedKey == act1LcaA) {
                    pushToast("Pick a different second room!", TOAST_KIND_WARNING);
                } else {
                    act1LcaB = clickedKey;
                    act1Hl2 = clickedKey;
                    buildRouteBetween(act1Tree, act1LcaA, act1LcaB, act1RoutePath, act1RouteLen);
                    act1LcaStage = 2;
                    scoring.addStep(1);
                    syncScoringHud();
                    pushToast("Selected Room B: " + std::to_string(act1LcaB), TOAST_KIND_SUCCESS);
                    status = "Rooms " + std::to_string(act1LcaA) + " and " + std::to_string(act1LcaB) +
                             " selected! Step 2/3: Click their Lowest Common Ancestor (LCA)!";
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                }
            } else if (act1LcaStage == 2) {
                int expectedLca = act1Tree.lca(act1LcaA, act1LcaB);
                if (clickedKey == expectedLca) {
                    act1LcaStage = 3;
                    act1RouteIdx = 0;
                    clearMarked();
                    scoring.addStep(1);
                    syncScoringHud();
                    pushToast("LCA(" + std::to_string(act1LcaA) + ", " + std::to_string(act1LcaB) +
                                    ") = " + std::to_string(expectedLca) + "!", TOAST_KIND_SUCCESS);
                    status = "LCA = " + std::to_string(expectedLca) + " verified! Step 3/3: Trace the path from " +
                             std::to_string(act1LcaA) + " to " + std::to_string(act1LcaB) +
                             " by clicking each room along it in order!";
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Room " + std::to_string(clickedKey) + " is not the LCA of " +
                                 std::to_string(act1LcaA) + " and " + std::to_string(act1LcaB) + "!");
                }
            } else if (act1LcaStage == 3 && act1RouteIdx < act1RouteLen) {
                if (act1RouteIdx == 0 && act1RouteLen > 1 && clickedKey == act1RoutePath[act1RouteLen - 1]) {
                    for (int l = 0, r = act1RouteLen - 1; l < r; ++l, --r) {
                        swapInt(act1RoutePath[l], act1RoutePath[r]);
                    }
                }
                if (clickedKey == act1RoutePath[act1RouteIdx]) {
                    addMarkedId(clickedId);
                    act1RouteIdx++;
                    scoring.addStep(1);
                    syncScoringHud();
                    if (scoring.isAlarmFull()) {
                        triggerAlarmFullFail();
                    } else if (act1RouteIdx == act1RouteLen) {
                        finishLevelWin(scoring.stars(),
                                       "Route traced between " + std::to_string(act1LcaA) + " and " +
                                       std::to_string(act1LcaB) + " through LCA " +
                                       std::to_string(act1Tree.lca(act1LcaA, act1LcaB)) + "!");
                    } else {
                        pushToast("Path [" + std::to_string(act1RouteIdx) + "/" +
                                        std::to_string(act1RouteLen) + "]: " + std::to_string(clickedKey),
                                        TOAST_KIND_SUCCESS);
                        status = "Path step " + std::to_string(act1RouteIdx) + "/" + std::to_string(act1RouteLen) +
                                 ": clicked " + std::to_string(clickedKey) + ". Click next room along the route!";
                    }
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Wrong path room " + std::to_string(clickedKey) + "! Expected " +
                                 std::to_string(act1RoutePath[act1RouteIdx]) + " next along the route.");
                }
            }
        }
        else if (lid == LEVEL_1_5) {
            if (!act1Mirrored && !pendingWin) {
                TreeNode* u = findTreeNodeMutable(act1Tree.getRoot(), clickedKey);
                if (!u) return;
                if (!u->left && !u->right) {
                    if (containsInt(markedIds, markedCount, clickedId)) {
                        pushToast("Leaf " + std::to_string(clickedKey) + " already checked", TOAST_KIND_WARNING);
                    } else {
                        addMarkedId(clickedId);
                        scoring.addStep(1);
                        syncScoringHud();
                        pushToast("Leaf " + std::to_string(clickedKey) + " checked (no children to swap)",
                                        TOAST_KIND_SUCCESS);
                        status = "Leaf " + std::to_string(clickedKey) + " checked. Click internal nodes bottom-up to swap children!";
                        if (scoring.isAlarmFull()) triggerAlarmFullFail();
                    }
                } else if (containsInt(act1SwappedKeys, act1SwappedCount, clickedKey)) {
                    pushToast("Room " + std::to_string(clickedKey) + " already swapped!", TOAST_KIND_WARNING);
                } else if (!allInternalDescendantsSwapped(u, act1SwappedKeys, act1SwappedCount)) {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Bottom-up order required! Swap lower subtrees before clicking " +
                                 std::to_string(clickedKey) + "!");
                } else {
                    pushUndoSnapshot(false);
                    LayoutResult before;
                    LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    TreeNode* tmp = u->left;
                    u->left = u->right;
                    u->right = tmp;
                    if (act1SwappedCount < 32) act1SwappedKeys[act1SwappedCount++] = clickedKey;
                    addMarkedId(clickedId);
                    EventBatch batch;
                    batch.append(Event(EVENT_OPERATION_STARTED, "mirror node"));
                    Event ev(EVENT_VISIT, "swap children");
                    ev.nodeId = u->id;
                    ev.newValue = u->value;
                    batch.append(ev);
                    batch.append(Event(EVENT_OPERATION_COMPLETED, "node inverted"));
                    layoutBtForScreen(false);
                    animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    if (allInternalNodesSwapped(act1Tree.getRoot(), act1SwappedKeys, act1SwappedCount)) {
                        act1Mirrored = true;
                        pendingWin = true;
                        pushToast("All subtrees inverted bottom-up!", TOAST_KIND_SUCCESS);
                        status = "All subtrees inverted bottom-up! Vault mirror complete...";
                    } else {
                        pushToast("Swapped children of " + std::to_string(clickedKey) + "!", TOAST_KIND_SUCCESS);
                        status = "Inverted subtree at " + std::to_string(clickedKey) +
                                 "! Continue swapping upward toward the root (or press Auto Mirror).";
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                }
            }
        }
        else if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) {
            bool isSearchMode = (lid == LEVEL_2_1) || (lid == LEVEL_2_5 && act2BossStage == 1);
            const char* midAct = isSearchMode ? "= Found" : "= Reject Dup";
            pushToast(std::string("At room ") + std::to_string(act2CurKey) + ": use [< Go Left], [" +
                            midAct + "], or [Go Right >]!", TOAST_KIND_WARNING);
            status = "Current room is " + std::to_string(act2CurKey) +
                     " - use bottom comparison buttons ([< Go Left], [" + midAct + "], [Go Right >])!";
        }
        else if (lid == LEVEL_2_3) {
            int delK = (act2DelStage < d.deleteSequenceCount) ? d.deleteSequence[act2DelStage] : -1;
            pushToast("Classify room " + std::to_string(delK) +
                            " via [Leaf], [One-Child], or [Two-Children]!", TOAST_KIND_WARNING);
        }
        else if (lid == LEVEL_2_4) {
            pushToast("Decide at room " + std::to_string(act2Hl1) +
                            ": [Report Room], [Prune Left], or [Prune Right]!", TOAST_KIND_WARNING);
        }
        else if (lid == LEVEL_3_1) {
            int violKeys[LEVEL_MAX_NODES];
            int violCount = findUnbalancedKeys(d, violKeys, LEVEL_MAX_NODES);
            int matchIdx = -1;
            for (int i = 0; i < violCount; ++i) {
                if (violKeys[i] == clickedKey) { matchIdx = i; break; }
            }
            int fullMask = (violCount > 0 && violCount < 31) ? ((1 << violCount) - 1) : 0;
            if (matchIdx >= 0) {
                int bit = (1 << matchIdx);
                if (clicked31Mask & bit) {
                    pushToast("Node " + std::to_string(clickedKey) + " already flagged!", TOAST_KIND_WARNING);
                } else {
                    addMarkedId(clickedId);
                    clicked31Mask |= bit;
                    scoring.addStep(1);
                    syncScoringHud();
                    pushToast("Flagged unbalanced node " + std::to_string(clickedKey), TOAST_KIND_SUCCESS);
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                    else if (clicked31Mask == fullMask) {
                        finishLevelWin(scoring.stars(), "All |BF| > 1 violations identified!");
                    } else {
                        status = "Flagged " + std::to_string(clickedKey) + "! Click the remaining RED violation node.";
                    }
                }
            } else {
                triggerWrongFlash(clickedId);
                raisePenalty(PENALTY_WRONG_TRAVERSAL, "Node " + std::to_string(clickedKey) + " is balanced (|BF| <= 1)!");
            }
        }
        else if (lid == LEVEL_3_2) {
            int pIdx = manualSubPuzzle - 1;
            if (pIdx >= 0 && pIdx < d.puzzleCount) {
                AVLNode* clickedNode = findAvlNodeMutable(avl.getRoot(), clickedKey);
                if (clickedNode) {
                    int bf = avl.balanceFactor(clickedNode);
                    if (bf > 1 || bf < -1) {
                        act3PivotPicked = true;
                        act3PivotKey = clickedKey;
                        selectedNodeId = clickedId;
                        pushToast("Selected unbalanced pivot " + std::to_string(clickedKey) +
                                        " (BF=" + std::to_string(bf) + ")! Pick [LL] or [RR].",
                                        TOAST_KIND_SUCCESS);
                        status = "Pivot " + std::to_string(clickedKey) + " (BF=" + std::to_string(bf) +
                                 ") selected! Click [LL: Rotate Right (1)] or [RR: Rotate Left (2)].";
                    } else {
                        triggerWrongFlash(clickedId);
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     "Node " + std::to_string(clickedKey) + " is balanced (BF=" +
                                     std::to_string(bf) + ")! Click an unbalanced pivot (|BF| > 1).");
                    }
                }
            }
        }
        else if (lid == LEVEL_3_3) {
            int pIdx = manualSubPuzzle - 1;
            if (pIdx >= 0 && pIdx < d.puzzleCount) {
                if (manualDoubleStep == 0) {
                    pushToast("First identify the zig-zag case: click [Case: LR] or [Case: RL]!",
                                    TOAST_KIND_WARNING);
                    status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(d.puzzleCount) +
                             ": Pick [Case: LR (1)] or [Case: RL (2)] before rotating pivots!";
                } else if (manualDoubleStep == 1) {
                    AVLNode* rootNode = avl.getRoot();
                    int rbf = avl.balanceFactor(rootNode);
                    bool isLR = (rbf > 1 && rootNode && rootNode->left && avl.balanceFactor(rootNode->left) < 0);
                    bool isRL = (rbf < -1 && rootNode && rootNode->right && avl.balanceFactor(rootNode->right) > 0);
                    AVLNode* childPivot = isLR ? rootNode->left : (isRL ? rootNode->right : nullptr);
                    if (childPivot && clickedKey == childPivot->key) {
                        pushUndoSnapshot(false);
                        LayoutResult before;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch batch;
                        batch.append(Event(EVENT_OPERATION_STARTED,
                                           isLR ? "LR step 1: rotate left child" : "RL step 1: rotate right child"));
                        {
                            Event alert(EVENT_BALANCE_ALERT, "balance alert");
                            alert.nodeId = childPivot->key;
                            alert.newValue = avl.balanceFactor(childPivot);
                            batch.append(alert);
                        }
                        AVLNode* newChild = isLR ? avl.rotateLeft(childPivot, batch, ROTATION_LR)
                                                 : avl.rotateRight(childPivot, batch, ROTATION_RL);
                        if (isLR) rootNode->left = newChild;
                        else      rootNode->right = newChild;
                        if (newChild) newChild->parent = rootNode;
                        avl.updateHeightAndSize(rootNode, batch);
                        batch.append(Event(EVENT_OPERATION_COMPLETED, "child pivot rotated"));

                        int badKey = -1;
                        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                            raisePenalty(PENALTY_WRONG_ROTATION,
                                         "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
                            return;
                        }

                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
                        manualDoubleStep = 2;
                        selectedNodeId = -1;
                        scoring.addStep(1);
                        syncScoringHud();
                        pushToast("Step 1/2 rotated child " + std::to_string(clickedKey) +
                                        "! Now click root " + std::to_string(rootNode->key),
                                        TOAST_KIND_SUCCESS);
                        status = std::string(isLR ? "LR" : "RL") + " Step 3/3: Now click root pivot " +
                                 std::to_string(rootNode->key) + "!";
                        if (scoring.isAlarmFull()) triggerAlarmFullFail();
                    } else {
                        triggerWrongFlash(clickedId);
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     "Click heavy child pivot " +
                                     std::to_string(childPivot ? childPivot->key : d.puzzles[pIdx].childKey) +
                                     " first to straighten the zig-zag!");
                    }
                } else {
                    AVLNode* rootNode = avl.getRoot();
                    if (rootNode && clickedKey == rootNode->key) {
                        int rbf = avl.balanceFactor(rootNode);
                        bool isLR = (rbf > 1);
                        pushUndoSnapshot(false);
                        LayoutResult before;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch batch;
                        batch.append(Event(EVENT_OPERATION_STARTED,
                                           isLR ? "LR step 2: rotate right root" : "RL step 2: rotate left root"));
                        AVLNode* newRoot = isLR ? avl.rotateRight(rootNode, batch, ROTATION_LR)
                                                : avl.rotateLeft(rootNode, batch, ROTATION_RL);
                        avl.setRoot(newRoot);
                        batch.append(Event(EVENT_OPERATION_COMPLETED, "double rotation complete"));

                        int badKey = -1;
                        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                            raisePenalty(PENALTY_WRONG_ROTATION,
                                         "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
                            return;
                        }

                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
                        scoring.addStep(1);
                        syncScoringHud();
                        selectedNodeId = -1;
                        if (scoring.isAlarmFull()) {
                            triggerAlarmFullFail();
                        } else if (manualSubPuzzle < d.puzzleCount) {
                            act3NextPuzzlePending = true;
                            pushToast("Double Rotation complete! Advancing to Puzzle " +
                                            std::to_string(manualSubPuzzle + 1), TOAST_KIND_SUCCESS);
                            status = "Double rotation animated! Loading next zig-zag puzzle...";
                        } else {
                            pendingWin = true;
                            status = "Final double rotation complete! Verifying balanced AVL tree...";
                        }
                    } else {
                        triggerWrongFlash(clickedId);
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     "Step 3/3: Click root pivot " +
                                     std::to_string(rootNode ? rootNode->key : d.puzzles[pIdx].rootKey) +
                                     " to complete double rotation!");
                    }
                }
            }
        }
        else if (lid == LEVEL_3_4) {
            if (avlQueuePos < d.insertCount) {
                int incoming = d.inserts[avlQueuePos].key;
                pushToast("Predict the rotation for key " + std::to_string(incoming) +
                                " using [LL], [RR], [LR], [RL], or [None]!", TOAST_KIND_WARNING);
                status = "Use the bottom prediction buttons [LL] [RR] [LR] [RL] [None] for incoming key " +
                         std::to_string(incoming) + "!";
            }
        }
        else if (lid == LEVEL_3_5) {
            if (bossQueryIdx < d.kQueryCount) {
                int badKey = -1;
                if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                    act3SizeOk = false;
                    raisePenalty(PENALTY_WRONG_ROTATION,
                                 "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
                    return;
                }
                int k = d.kQueries[bossQueryIdx];
                int comps = 0;
                int expectedKey = kthLargestAvl(avl, k, comps);
                if (clickedKey == expectedKey) {
                    pushUndoSnapshot(false);
                    int hBefore = avl.getHeight();
                    act3LastComps = comps;
                    act3LastHeight = hBefore;
                    scoring.addStep(1);
                    scoring.addBonus(50);
                    syncScoringHud();
                    bossQueryIdx++;
                    if (scoring.isAlarmFull()) {
                        triggerAlarmFullFail();
                    } else if (bossQueryIdx >= d.kQueryCount) {
                        LayoutResult sameLayout;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, sameLayout);
                        EventBatch qBatch;
                        int dummyC = 0;
                        kthLargestAvl(avl, k, dummyC, &qBatch);
                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(qBatch, sameLayout, avlLayout, true, autoPlayEvents);
                        pushToast("Stole #" + std::to_string(k) + " largest=" + std::to_string(expectedKey) +
                                        " in " + std::to_string(comps) + " comps <= H=" +
                                        std::to_string(hBefore) + " (sz OK)!", TOAST_KIND_SUCCESS);
                        pendingWin = true;
                        status = "Final #" + std::to_string(k) + " largest=" + std::to_string(expectedKey) +
                                 " verified in " + std::to_string(comps) + " comps <= H=" +
                                 std::to_string(hBefore) + "! Completing Boss...";
                    } else {
                        LayoutResult before;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch rotBatch;
                        if (!avl.search(22)) {
                            avl.insert(22, rotBatch);
                        } else {
                            int dummyC = 0;
                            kthLargestAvl(avl, k, dummyC, &rotBatch);
                        }
                        int postBad = -1;
                        act3SizeOk = validateAvlSubtreeSizes(avl.getRoot(), postBad);
                        if (!act3SizeOk) {
                            raisePenalty(PENALTY_WRONG_ROTATION,
                                         "Subtree size mismatch after security rotation at " +
                                         std::to_string(postBad) + "!");
                            return;
                        }
                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(rotBatch, before, avlLayout, true, autoPlayEvents);
                        pushToast("Stole #" + std::to_string(k) + " largest=" + std::to_string(expectedKey) +
                                        " (" + std::to_string(comps) + " comps <= H=" +
                                        std::to_string(hBefore) + ")! Security inserted 22 + rotated (sz OK)!",
                                        TOAST_KIND_SUCCESS);
                        status = "Last query: " + std::to_string(comps) + " comps <= H=" +
                                 std::to_string(hBefore) + " (sz OK). Security added 22 & rotated -> Now click #" +
                                 std::to_string(d.kQueries[bossQueryIdx]) + " MOST VALUABLE!";
                    }
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Node " + std::to_string(clickedKey) + " is not the #" + std::to_string(k) +
                                 " MOST VALUABLE (largest) key! Compare right subtree sz.");
                }
            }
        }
        else if (lid == LEVEL_4_1) {
            if (buildStepIdx >= 0) {
                int partner = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
                if (clickedId == buildStepIdx || clickedId == partner) {
                    pushUndoSnapshot(false);
                    LayoutResult before;
                    LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount,
                                                  CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    int doneIdx = buildStepIdx;
                    siftDownHeapArrayWithEvents(heapArr, activeHeapSize, doneIdx, batch);
                    buildStepIdx = findNextHeapViolation(heapArr, activeHeapSize, doneIdx - 1);
                    hlIndex1 = buildStepIdx;
                    hlIndex2 = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
                    scoring.addStep(1);
                    syncScoringHud();
                    if (buildStepIdx >= 0) {
                        pushToast("Sifted down [" + std::to_string(doneIdx) + "]!", TOAST_KIND_SUCCESS);
                        status = "Next: Click node [" + std::to_string(buildStepIdx) + "] (val " +
                                 std::to_string(heapArr[buildStepIdx]) + ") or [" + std::to_string(hlIndex2) +
                                 "] (val " + std::to_string(heapArr[hlIndex2]) + ") to sift down!";
                    } else {
                        pendingWin = true;
                        pushToast("Root sifted down! Valid Max Heap built", TOAST_KIND_SUCCESS);
                        status = "Bottom-up build complete! Playing final swaps...";
                    }
                    layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_ACT4_INVALID,
                                 "Sift down node [" + std::to_string(buildStepIdx) + "] next (bottom-up order)!");
                }
            }
        }
        else if (lid == LEVEL_4_2) {
            if (!pendingWin && buildStepIdx >= 0) {
                int partner = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
                if (clickedId == buildStepIdx || clickedId == partner) {
                    pushUndoSnapshot(false);
                    LayoutResult before;
                    LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount,
                                                  CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    siftDownHeapArrayWithEvents(heapArr, activeHeapSize, buildStepIdx, batch);
                    layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    pendingWin = true;
                    buildStepIdx = -1;
                    hlIndex1 = -1; hlIndex2 = -1;
                    pushToast("Sabotaged node sifted down! Heap valid", TOAST_KIND_SUCCESS);
                    status = "Sabotaged node sifted down! Playing repair animation...";
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_ACT4_INVALID,
                                 "Click sabotaged node [" + std::to_string(buildStepIdx) + "] (or child [" +
                                 std::to_string(partner) + "]) to repair!");
                }
            }
        }
        else if (lid == LEVEL_4_3 || lid == LEVEL_4_4) {
            if (!act4Heap.isEmpty()) {
                if (clickedId == 0) {
                    pushUndoSnapshot(true);
                    LayoutResult before;
                    LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount,
                                                CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    int maxVal = act4Heap.extractMax(batch);
                    insertExtractedMaxIntoSortedTail(sortedArr, sortedCount, maxVal);
                    layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    int optS = computeOptimalSteps(d);
                    if (act4Heap.isEmpty()) {
                        if (!validateAct4SortedResult(d, sortedArr, sortedCount)) {
                            pendingWin = false;
                            raisePenalty(PENALTY_ACT4_INVALID, "Sorted region does not match expected ascending order!");
                            status = "ERROR: Sorted array mismatch against expected_sorted_result!";
                        } else {
                            pushToast("Extracted max " + std::to_string(maxVal) + " (ascending order verified)!",
                                            TOAST_KIND_SUCCESS);
                            pendingWin = true;
                            if (lid == LEVEL_4_4) {
                                status = "All " + std::to_string(optS) + " extracted! Steps: " +
                                         std::to_string(scoring.steps()) + "/" + std::to_string(optS) +
                                         " (" + std::to_string(optS) + " extractions, O(log n) each = Theta(n log n))!";
                            } else {
                                status = "Extracted final treasure " + std::to_string(maxVal) + "! Ascending sorted array verified.";
                            }
                        }
                    } else {
                        pushToast("Extracted max " + std::to_string(maxVal) + " to sorted tail!", TOAST_KIND_SUCCESS);
                        if (lid == LEVEL_4_4) {
                            status = "Extracted " + std::to_string(maxVal) + " | Steps: " +
                                     std::to_string(scoring.steps()) + "/" + std::to_string(optS) +
                                     " (" + std::to_string(optS) + " extractions, O(log n) each = Theta(n log n)). Next root [0]=" +
                                     std::to_string(act4Heap.getMax()) + "!";
                        } else {
                            status = "Extracted " + std::to_string(maxVal) + "! Click root [0] (" +
                                     std::to_string(act4Heap.getMax()) + ") to extract next.";
                        }
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_ACT4_INVALID, "Max Heap extraction always removes root [0]!");
                }
            }
        }
        else if (lid == LEVEL_4_5) {
            int buildStart = (activeHeapSize >= 2) ? (activeHeapSize / 2 - 1) : 0;
            if (!act4Built) {
                if (clickedId == buildStart) triggerStructuralAction(1);
                else pushToast("Click [Build Max Heap [B]] or node [" + std::to_string(buildStart) + "] first",
                                     TOAST_KIND_WARNING);
            } else if (!act4Heap.isEmpty()) {
                if (clickedId == 0) {
                    pushUndoSnapshot(true);
                    LayoutResult before;
                    LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount,
                                                CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    int maxVal = act4Heap.extractMax(batch);
                    insertExtractedMaxIntoSortedTail(sortedArr, sortedCount, maxVal);
                    layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    if (act4Heap.isEmpty()) {
                        if (!validateAct4SortedResult(d, sortedArr, sortedCount)) {
                            pendingWin = false;
                            raisePenalty(PENALTY_ACT4_INVALID, "Sorted region does not match expected ascending order!");
                            status = "ERROR: Sorted array mismatch against expected_sorted_result!";
                        } else {
                            pushToast("In-place ascending Heap Sort verified!", TOAST_KIND_SUCCESS);
                            pendingWin = true;
                            status = "All treasures heap-sorted ascending! Unlocking final vault...";
                        }
                    } else {
                        pushToast("Extracted " + std::to_string(maxVal) + " to ascending tail", TOAST_KIND_SUCCESS);
                        status = "Extracted " + std::to_string(maxVal) + ". Click root [0] (" +
                                 std::to_string(act4Heap.getMax()) + ") next!";
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    triggerWrongFlash(clickedId);
                    raisePenalty(PENALTY_ACT4_INVALID, "Click root [0] to extract the next maximum!");
                }
            }
        }
    };

    // Explorer Demos (Act 1 & Act 2)
    auto triggerAct1Action = [&](int actionCode) {
        layoutDirty = true;
        if (actionCode == 1) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            int cPre = 0, cIn = 0, cPost = 0, cLvl = 0;
            int* pre = act1Tree.preorderIterative(cPre);
            int* in  = act1Tree.inorderIterative(cIn, batch);
            int* post = act1Tree.postorderIterative(cPost);
            int* lvl = act1Tree.levelOrder(cLvl);
            layoutBtForScreen(true);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);

            std::string preStr = "Pre: ", inStr = "In: ", postStr = "Post: ", lvlStr = "Level: ";
            for (int i = 0; i < cPre; ++i) preStr += std::to_string(pre[i]) + " ";
            for (int i = 0; i < cIn; ++i) inStr += std::to_string(in[i]) + " ";
            for (int i = 0; i < cPost; ++i) postStr += std::to_string(post[i]) + " ";
            for (int i = 0; i < cLvl; ++i) lvlStr += std::to_string(lvl[i]) + " ";
            act1TraversalStr = preStr + " | " + inStr + "\n" + postStr + " | " + lvlStr;
            status = "Traversals computed! Watch Inorder events in the log.";
            delete[] pre; delete[] in; delete[] post; delete[] lvl;
        } else if (actionCode == 2) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            act1Tree.mirror(batch);
            layoutBtForScreen(true);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            act1Hl1 = -9999; act1Hl2 = -9999;
            status = "Tree mirrored! Symmetric: " + std::string(act1Tree.isSymmetric() ? "YES" : "NO");
        } else if (actionCode == 3) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            int preArr[] = { 10, 5, 2, 7, 15, 12, 20 };
            int inArr[]  = { 2, 5, 7, 10, 12, 15, 20 };
            act1Tree.buildFromPreorderInorder(preArr, inArr, 7, batch);
            layoutBtForScreen(true);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            act1Hl1 = 10; act1Hl2 = -9999;
            status = "Reconstructed tree from Preorder + Inorder!";
        } else if (actionCode == 4) {
            status = "Height = " + std::to_string(act1Tree.height()) + " (leaf=1)  |  Diameter = " + std::to_string(act1Tree.diameter()) + " nodes";
        } else if (actionCode == 5) {
            if (!act1Tree.getRoot()) { status = "Tree is empty!"; return; }
            int a = 20, b = 40;
            int lcaVal = act1Tree.lca(a, b);
            if (lcaVal == -1) {
                a = act1Tree.getRoot()->value;
                b = act1Tree.getRoot()->left ? act1Tree.getRoot()->left->value : a;
                lcaVal = act1Tree.lca(a, b);
            }
            act1Hl1 = a; act1Hl2 = b;
            status = "LCA(" + std::to_string(a) + ", " + std::to_string(b) + ") = " + std::to_string(lcaVal);
        } else if (actionCode == 6) {
            act1Tree = BinaryTree();
            auto r1 = act1Tree.addNode(1, nullptr, true);
            auto r2L = act1Tree.addNode(2, r1, true);
            auto r2R = act1Tree.addNode(2, r1, false);
            act1Tree.addNode(3, r2L, true);
            act1Tree.addNode(4, r2L, false);
            act1Tree.addNode(4, r2R, true);
            act1Tree.addNode(3, r2R, false);
            animCtrl.clearCurrent();
            act1Hl1 = -9999; act1Hl2 = -9999;
            status = "Loaded Symmetric Binary Tree! isSymmetric() = " + std::string(act1Tree.isSymmetric() ? "TRUE" : "FALSE");
        } else if (actionCode == 7) {
            resetAct1();
            animCtrl.clearCurrent();
            status = "Reset Act 1 General Binary Tree.";
        }
    };

    auto triggerAct2Action = [&](int actionCode) {
        layoutDirty = true;
        if (actionCode == 1) {
            int candidates[] = { 35, 65, 85, 25, 45, 50, 15 };
            static int insStep = 0;
            int key = candidates[insStep++ % 7];
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            bool ok = act2Bst.insert(key, batch);
            layoutBstForScreen(true);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            act2Hl1 = key; act2Hl2 = -9999;
            status = ok ? ("Inserted key " + std::to_string(key) + " | ValidBST=" + (act2Bst.isValidBST() ? "YES" : "NO"))
                        : ("Duplicate key " + std::to_string(key) + " rejected!");
        } else if (actionCode == 2) {
            static int delStep = 0;
            int delKey = -1;
            if (delStep == 0 && act2Bst.search(80)) delKey = 80;
            else if (delStep == 1 && act2Bst.search(20)) delKey = 20;
            else if (delStep == 2 && act2Bst.search(30)) delKey = 30;
            else delKey = act2Bst.findMin();
            delStep = (delStep + 1) % 3;
            if (delKey != -1) {
                LayoutResult before;
                LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
                EventBatch batch;
                act2Bst.deleteKey(delKey, batch);
                layoutBstForScreen(true);
                animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
                act2Hl1 = -9999; act2Hl2 = -9999;
                status = "Deleted key " + std::to_string(delKey) + " | Size=" + std::to_string(act2Bst.getSize());
            }
        } else if (actionCode == 3) {
            int searchKeys[] = { 40, 99, 70, 25, 88 };
            static int sIdx = 0;
            int sk = searchKeys[sIdx++ % 5];
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            bool found = act2Bst.search(sk, batch);
            layoutBstForScreen(true);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            act2Hl1 = found ? sk : -9999;
            status = "search(" + std::to_string(sk) + ") = " + (found ? "FOUND" : "NOT FOUND");
        } else if (actionCode == 4) {
            act2Hl1 = act2Bst.findMin();
            act2Hl2 = act2Bst.findMax();
            status = "Min = " + std::to_string(act2Hl1) + "  |  Max = " + std::to_string(act2Hl2);
        } else if (actionCode == 5) {
            int target = 40;
            if (!act2Bst.search(target) && act2Bst.getRoot()) target = act2Bst.getRoot()->value;
            if (target != -1) {
                int sc = act2Bst.successor(target);
                int pd = act2Bst.predecessor(target);
                act2Hl1 = target;
                status = "Node " + std::to_string(target) + " -> Pred=" + (pd != -1 ? std::to_string(pd) : "None") +
                         " | Succ=" + (sc != -1 ? std::to_string(sc) : "None");
            }
        } else if (actionCode == 6) {
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            int count = 0;
            int* rq = act2Bst.rangeQuery(25, 75, count, batch);
            layoutBstForScreen(true);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            std::string rqStr = "Range [25..75]: ";
            if (rq) {
                for (int i = 0; i < count; ++i) rqStr += std::to_string(rq[i]) + (i + 1 < count ? ", " : "");
                delete[] rq;
            }
            status = rqStr;
            act2ExtraInfo = rqStr;
        } else if (actionCode == 7) {
            static int k = 1;
            if (k > act2Bst.getSize()) k = 1;
            int ans = act2Bst.kthSmallest(k);
            act2Hl1 = ans;
            status = "kthSmallest(k=" + std::to_string(k) + ") = " + std::to_string(ans);
            k++;
        } else if (actionCode == 8) {
            act2Bst.clear();
            for (int i = 1; i <= 7; ++i) act2Bst.insert(i * 10);
            act2Hl1 = -9999; act2Hl2 = -9999;
            animCtrl.clearCurrent();
            layoutBstForScreen(true);
            status = "Degenerate Chain (10..70): Height=" + std::to_string(act2Bst.getHeight()) + " == Size=" + std::to_string(act2Bst.getSize());
        } else if (actionCode == 9) {
            resetAct2();
            layoutBstForScreen(true);
            status = "Reset Act 2 BST.";
        }
    };

    // Playback Slider / Scrubber helpers
    bool draggingSpeedSlider = false;
    bool draggingScrubber = false;
    float sliderPanelX = LOG_X, sliderPanelW = LOG_W;
    auto updateSpeedFromMouse = [&](float mxPos, float panelX, float panelW) {
        float trackX = panelX + 94.f;
        float trackW = panelW - 108.f;
        if (trackW > 240.f) trackW = 240.f;
        float norm = clampFloat((mxPos - trackX) / trackW, 0.f, 1.f);
        animCtrl.setSpeed(0.25f + norm * 2.75f);
    };
    auto updateScrubberFromMouse = [&](float mxPos, float panelX, float panelW) {
        int total = animCtrl.getEventCount();
        if (total <= 0) return;
        float scrubX = panelX + 90.f;
        float scrubW = panelW - 104.f;
        if (scrubW < 40.f) scrubW = 40.f;
        float norm = clampFloat((mxPos - scrubX) / scrubW, 0.f, 1.f);
        int targetIdx = (total == 1) ? 0 : static_cast<int>(norm * static_cast<float>(total - 1) + 0.5f);
        animCtrl.seekToEvent(targetIdx);
        status = "Scrubbed: " + AnimationController::formatEvent(animCtrl.getCurrentEvent());
    };
    auto handlePlaybackControlsClick = [&](float mxPos, float myPos, float panelX, float panelY, float panelW, float panelH) -> bool {
        float row0Y = panelY + panelH - 96.f;
        float row1Y = panelY + panelH - 64.f;
        float row2Y = panelY + panelH - 30.f;
        float btnW = (panelW - 32.f) / 3.f;
        if (btnW > 116.f) btnW = 116.f;

        if (Ui::inside(mxPos, myPos, panelX + 8.f, row0Y, 72.f, 24.f)) {
            animCtrl.replayLastBatch();
            status = "Replaying operation events...";
            return true;
        }
        float scrubX = panelX + 90.f;
        float scrubW = panelW - 104.f;
        if (scrubW < 40.f) scrubW = 40.f;
        if (Ui::inside(mxPos, myPos, scrubX - 6.f, row0Y, scrubW + 12.f, 24.f)) {
            draggingScrubber = true;
            sliderPanelX = panelX;
            sliderPanelW = panelW;
            updateScrubberFromMouse(mxPos, panelX, panelW);
            return true;
        }
        if (Ui::inside(mxPos, myPos, panelX + 8.f, row1Y, btnW, 25.f)) {
            animCtrl.togglePlayPause();
            return true;
        }
        if (Ui::inside(mxPos, myPos, panelX + 16.f + btnW, row1Y, btnW, 25.f)) {
            animCtrl.setPlaying(false);
            stepAnimForward();
            return true;
        }
        if (Ui::inside(mxPos, myPos, panelX + 24.f + 2.f * btnW, row1Y, btnW, 25.f)) {
            animCtrl.finishInstant();
            if (act2TwoChildPhase2Pending) {
                startAct2TwoChildPhase2();
                animCtrl.finishInstant();
            }
            if (act3NextPuzzlePending) {
                loadNextAct3Puzzle();
                animCtrl.finishInstant();
            }
            if (pendingWin) completePendingWin();
            return true;
        }
        float trackX = panelX + 94.f;
        float trackW = panelW - 108.f;
        if (trackW > 240.f) trackW = 240.f;
        if (Ui::inside(mxPos, myPos, trackX - 8.f, row2Y, trackW + 16.f, 24.f)) {
            draggingSpeedSlider = true;
            sliderPanelX = panelX;
            sliderPanelW = panelW;
            updateSpeedFromMouse(mxPos, panelX, panelW);
            return true;
        }
        return false;
    };

    // Sandbox actions
    auto enterSandbox = [&]() {
        currentScreen = SCREEN_SANDBOX;
        sbAvlCam.reset();
        sbHeapCam.reset();
        layoutDirty = true;
        avl.clear();
        int init[] = { 40, 20, 60, 10, 30, 50, 70 };
        for (int v : init) avl.insert(v);
        int hInit[] = { 90, 75, 80, 60, 50, 40, 70 };
        act4Heap.buildMaxHeap(hInit, 7);
        sortedCount = 0;
        animCtrl.clearCurrent();
        undoStack.clear();
        status = "Sandbox: Real AVLTree + MaxHeap with live EventLog & Undo.";
    };
    auto triggerSandboxAction = [&](int code) {
        layoutDirty = true;
        if (code == 1) {
            static int sbInsIdx = 0;
            int vals[] = { 15, 25, 35, 55, 65, 75, 5, 85 };
            int key = vals[sbInsIdx++ % 8];
            pushUndoSnapshot(true);
            LayoutResult before;
            LayoutEngine::layoutAVL(avl, 30.f, 95.f, 560.f, 540.f, before);
            EventBatch batch;
            bool ok = avl.insert(key, batch);
            layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
            animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
            status = ok ? ("Sandbox AVL Insert(" + std::to_string(key) + ")! Height=" + std::to_string(avl.getHeight()))
                        : ("Sandbox AVL Duplicate(" + std::to_string(key) + ") rejected!");
            pushToast(status, ok ? TOAST_KIND_SUCCESS : TOAST_KIND_WARNING);
        } else if (code == 2 && !avl.isEmpty()) {
            int key = avl.findMin();
            pushUndoSnapshot(true);
            LayoutResult before;
            LayoutEngine::layoutAVL(avl, 30.f, 95.f, 560.f, 540.f, before);
            EventBatch batch;
            avl.deleteKey(key, batch);
            layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
            animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
            status = "Sandbox AVL Deleted min key " + std::to_string(key) + "!";
            pushToast(status, TOAST_KIND_SUCCESS);
        } else if (code == 3 && !avl.isEmpty()) {
            static int sbK = 1;
            if (sbK > avl.getSize()) sbK = 1;
            int val = avl.kth(sbK);
            status = "Sandbox AVL kth(" + std::to_string(sbK) + ") = " + std::to_string(val);
            pushToast(status, TOAST_KIND_SUCCESS);
            sbK++;
        } else if (code == 4 && !act4Heap.isEmpty()) {
            pushUndoSnapshot(true);
            LayoutResult before;
            LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount, 605.f, 95.f, 565.f, 540.f, before);
            EventBatch batch;
            int mxVal = act4Heap.extractMax(batch);
            insertExtractedMaxIntoSortedTail(sortedArr, sortedCount, mxVal);
            layoutHeapForScreen(true, 605.f, 95.f, 565.f, 540.f);
            animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
            status = "Sandbox Heap Extracted max=" + std::to_string(mxVal) + "!";
            pushToast(status, TOAST_KIND_SUCCESS);
        } else if (code == 5) {
            enterSandbox();
            pushToast("Sandbox reset!", TOAST_KIND_WARNING);
        }
    };

    // Endless Mode actions (Step F)
    auto startEndlessMode = [&](unsigned int seed) {
        currentScreen = SCREEN_ENDLESS;
        endlessCam.reset();
        endlessPaused = false;
        endlessShowLastResult = false;
        endlessLastReason = "";
        if (seed == 0u) seed = static_cast<unsigned int>(std::time(nullptr)) ^ 0xA5A55A5Au;
        if (seed == 0u) seed = 1u;
        endlessLastSeed = seed;
        endlessSession.startNew(seed, avl);
        animCtrl.clearAll();
        undoStack.clear();
        selectedNodeId = -1;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        layoutDirty = false;
        cachedEndlessOps = -1;
        cachedEndlessOpType = -1;
        const EndlessOp& op = endlessSession.getPendingOp();
        status = "Seed #" + std::to_string(endlessSession.getInitialSeed()) +
                 " started! Click [Execute] to apply " +
                 std::string(op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                 std::to_string(op.key) + " before the deadline expires!";
        pushToast("Endless Mode active! Execute ops before deadline shrinks", TOAST_KIND_SUCCESS);
    };
    auto finishEndlessGameOver = [&]() {
        endlessSession.stop();
        endlessPaused = false;
        endlessShowLastResult = true;
        EndlessLeaderboardEntry rec;
        rec.nameDate = currentDateTag();
        rec.survivalMs = endlessSession.getSurvivalMs();
        rec.opsCompleted = endlessSession.getOpsCompleted();
        rec.seed = endlessSession.getInitialSeed();
        endlessLastEntry = rec;
        endlessLastReason = endlessSession.getGameOverReason();
        endlessLb.recordRun(rec);
        doSaveGame();
        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
        status = endlessLastReason + " Survived " + EndlessLeaderboard::formatTimeMs(rec.survivalMs) +
                 " (" + std::to_string(rec.opsCompleted) + " ops).";
        pushToast("Game Over! Survived " + EndlessLeaderboard::formatTimeMs(rec.survivalMs),
                        TOAST_KIND_WARNING);
    };
    auto triggerEndlessExecute = [&]() {
        if (currentScreen != SCREEN_ENDLESS || !endlessSession.isActive() || endlessPaused) return;
        const EndlessOp applied = endlessSession.getPendingOp();
        LayoutResult before;
        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
        EventBatch batch;
        bool ok = endlessSession.executePending(avl, batch);
        int badKey = -1;
        bool szOk = validateAvlSubtreeSizes(avl.getRoot(), badKey);
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        layoutDirty = false;
        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
        if (!ok || !szOk) {
            pushToast("AVL invariant check failed!", TOAST_KIND_PENALTY);
            finishEndlessGameOver();
            return;
        }
        const EndlessOp& next = endlessSession.getPendingOp();
        std::string doneWord = (applied.type == ENDLESS_OP_INSERT) ? "INSERT " : "DELETE ";
        std::string nextWord = (next.type == ENDLESS_OP_INSERT) ? "INSERT " : "DELETE ";
        status = "Executed " + doneWord + std::to_string(applied.key) +
                 " (AVL valid, H=" + std::to_string(avl.getHeight()) +
                 ")! Next: " + nextWord + std::to_string(next.key);
        pushToast(doneWord + std::to_string(applied.key) + " -> AVL valid!", TOAST_KIND_SUCCESS);
    };
    auto saveEndlessRun = [&](const std::string& pathOverride = "") {
        std::string target = pathOverride.empty() ? endlessSavePath : pathOverride;
        if (endlessSession.saveToFile(target, avl)) {
            status = "Saved Endless run (Seed=" + std::to_string(endlessSession.getInitialSeed()) +
                     ", Time=" + EndlessLeaderboard::formatTimeMs(endlessSession.getSurvivalMs()) +
                     ", Preorder+null AVL persisted).";
            pushToast("Endless session saved!", TOAST_KIND_SUCCESS);
        } else {
            pushToast("Failed to save Endless session", TOAST_KIND_PENALTY);
        }
    };
    auto loadEndlessRun = [&](const std::string& pathOverride = "") -> bool {
        std::string target = pathOverride.empty() ? endlessSavePath : pathOverride;
        if (!endlessSession.loadFromFile(target, avl)) {
            pushToast("No valid saved Endless run found", TOAST_KIND_WARNING);
            return false;
        }
        currentScreen = SCREEN_ENDLESS;
        endlessCam.reset();
        endlessPaused = false;
        endlessShowLastResult = false;
        endlessLastSeed = endlessSession.getInitialSeed();
        animCtrl.clearAll();
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        layoutDirty = false;
        cachedEndlessOps = -1;
        cachedEndlessOpType = -1;
        const EndlessOp& op = endlessSession.getPendingOp();
        status = "Resumed Endless Save (Seed=" + std::to_string(endlessSession.getInitialSeed()) +
                 ", Survived=" + EndlessLeaderboard::formatTimeMs(endlessSession.getSurvivalMs()) +
                 ")! Next: " + std::string(op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                 std::to_string(op.key);
        pushToast("Resumed saved Endless run!", TOAST_KIND_SUCCESS);
        return true;
    };
