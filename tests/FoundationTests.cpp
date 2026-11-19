#include <gtest/gtest.h>
#include "game/Json.h"
#include "game/LevelData.h"
#include "game/LevelLoader.h"
#include "game/Scoring.h"
#include "game/HintSystem.h"
#include "game/EndlessMode.h"
#include "gui/DragDrop.h"
#include "structures/BST.h"
#include "structures/AVLTree.h"
#include "structures/MaxHeap.h"
#include <cstdio>

using namespace VaultHeist;

// =============================================================================
// 1. Scoring / Alarm Engine
// =============================================================================

TEST(ScoringEngineTest, AlarmCapacityMatchesFormula) {
    // ceil(1.5 * optimalSteps) + 2
    EXPECT_EQ(ScoringEngine::capacityFor(1), 4);
    EXPECT_EQ(ScoringEngine::capacityFor(2), 5);
    EXPECT_EQ(ScoringEngine::capacityFor(3), 7);
    EXPECT_EQ(ScoringEngine::capacityFor(4), 8);
    EXPECT_EQ(ScoringEngine::capacityFor(7), 13);
    EXPECT_EQ(ScoringEngine::capacityFor(9), 16);
}

TEST(ScoringEngineTest, StarCalculationAndHintCap) {
    ScoringEngine sc;
    sc.begin(4); // cap = 8
    EXPECT_EQ(sc.capacity(), 8);

    // Perfect run: 4 steps, 0 penalties, no hint -> 3 stars
    sc.addStep(4);
    EXPECT_EQ(sc.alarm(), 4);
    EXPECT_EQ(sc.stars(), 3);
    EXPECT_EQ(sc.score(), 960);

    // Hint caps max stars at 2 even on a perfect run
    EXPECT_TRUE(sc.applyHint());
    EXPECT_FALSE(sc.applyHint());
    EXPECT_EQ(sc.stars(), 2);

    // <= 1.25 * optimal (5 <= 5) and <= 1 penalty -> 2 stars
    EXPECT_EQ(ScoringEngine::starsFor(5, 4, 1, false, 6, 8), 2);
    // > 1.25 * optimal (6 > 5) within capacity -> 1 star
    EXPECT_EQ(ScoringEngine::starsFor(6, 4, 0, false, 6, 8), 1);
    // 2 penalties within capacity -> 1 star
    EXPECT_EQ(ScoringEngine::starsFor(4, 4, 2, false, 6, 8), 1);
    // Alarm full -> 0 stars
    EXPECT_EQ(ScoringEngine::starsFor(4, 4, 0, false, 8, 8), 0);
}

TEST(ScoringEngineTest, PenaltiesAndScoreFloor) {
    ScoringEngine sc;
    sc.begin(5); // cap = 10

    EXPECT_EQ(sc.applyPenalty(PENALTY_WRONG_TRAVERSAL), 1);
    EXPECT_EQ(sc.alarm(), 1);
    EXPECT_EQ(sc.penaltyPoints(), 0);
    EXPECT_EQ(sc.score(), 990);

    EXPECT_EQ(sc.applyPenalty(PENALTY_ACT4_INVALID), 1);
    EXPECT_EQ(sc.alarm(), 2);
    EXPECT_EQ(sc.penaltyPoints(), 0);
    EXPECT_EQ(sc.score(), 980);

    EXPECT_EQ(sc.applyPenalty(PENALTY_WRONG_ROTATION), 2);
    EXPECT_EQ(sc.alarm(), 4);
    EXPECT_EQ(sc.penaltyPoints(), 50);
    EXPECT_EQ(sc.score(), 910);

    sc.addBonus(40);
    EXPECT_EQ(sc.score(), 950);

    // Score floors at 0
    EXPECT_EQ(ScoringEngine::scoreFor(120, 100, 0), 0);

    // Shrinking capacity (used in Act 2 Boss) triggers full-alarm cleanly
    sc.setCapacity(4);
    EXPECT_EQ(sc.capacity(), 4);
    EXPECT_TRUE(sc.isAlarmFull());
    EXPECT_EQ(sc.stars(), 0);
    sc.setCapacity(0);
    EXPECT_EQ(sc.capacity(), 1);
}

// =============================================================================
// 2. JSON Reader & Level Loader (all 20 levels)
// =============================================================================

TEST(LevelLoaderTest, LoadsAllTwentyLevelsAndComputesOptimalSteps) {
    const int ids[20] = {
        11, 12, 13, 14, 15,
        21, 22, 23, 24, 25,
        31, 32, 33, 34, 35,
        41, 42, 43, 44, 45
    };
    const int expectedOpt[20] = {
        7, 7, 8, 8, 7,
        3, 6, 3, 5, 15,
        2, 2, 4, 4, 2,
        3, 1, 7, 7, 9
    };

    for (int i = 0; i < 20; ++i) {
        LevelData d;
        std::string err;
        ASSERT_TRUE(loadLevelById(ids[i], d, err)) << "Failed id " << ids[i] << ": " << err;
        EXPECT_EQ(d.id, ids[i]);
        EXPECT_EQ(d.act, ids[i] / 10);
        EXPECT_FALSE(d.title.empty());
        EXPECT_FALSE(d.objective.empty());
        EXPECT_FALSE(d.controls.empty());
        EXPECT_FALSE(d.startStatus.empty());
        EXPECT_GE(d.hintCount, 1);
        EXPECT_EQ(computeOptimalSteps(d), expectedOpt[i]) << "Mismatch for level " << ids[i];
    }

    // Act 1 corrected level sequence & data checks
    LevelData d11, d12, d13, d14, d15;
    std::string aerr;
    ASSERT_TRUE(loadLevelById(11, d11, aerr));
    EXPECT_EQ(d11.hintCount, 4);
    EXPECT_EQ(d11.nodeCount, 7);

    ASSERT_TRUE(loadLevelById(12, d12, aerr));
    EXPECT_EQ(d12.title, "1.2 Decode the Blueprint");
    EXPECT_EQ(d12.preorderCount, 7);
    EXPECT_EQ(d12.inorderCount, 7);

    ASSERT_TRUE(loadLevelById(13, d13, aerr));
    EXPECT_EQ(d13.title, "1.3 The Alarm Spreads");
    EXPECT_EQ(d13.nodeCount, 7);
    EXPECT_EQ(computeOptimalSteps(d13), 8);

    ASSERT_TRUE(loadLevelById(14, d14, aerr));
    EXPECT_EQ(d14.title, "1.4 Find Where Two Routes Meet");
    EXPECT_EQ(d14.title.find("Boss"), std::string::npos);
    EXPECT_EQ(computeOptimalSteps(d14), 8);

    ASSERT_TRUE(loadLevelById(15, d15, aerr));
    EXPECT_EQ(d15.title, "1.5 Mirror the Vault");
    EXPECT_EQ(d15.title.find("Boss"), std::string::npos);
    EXPECT_EQ(computeOptimalSteps(d15), 7);

    // Act 2 corrected level sequence & data checks
    LevelData d21, d22, d23, d24, d25;
    ASSERT_TRUE(loadLevelById(21, d21, aerr));
    EXPECT_EQ(d21.title, "2.1 Search Heist");
    EXPECT_EQ(computeOptimalSteps(d21), 3);

    ASSERT_TRUE(loadLevelById(22, d22, aerr));
    EXPECT_EQ(d22.title, "2.2 Stash the Loot");
    EXPECT_EQ(computeOptimalSteps(d22), 6);

    ASSERT_TRUE(loadLevelById(23, d23, aerr));
    EXPECT_EQ(d23.title, "2.3 Steal and Collapse");
    ASSERT_EQ(d23.deleteSequenceCount, 3);
    EXPECT_EQ(d23.deleteSequence[0], 80);
    EXPECT_EQ(d23.deleteSequence[1], 70);
    EXPECT_EQ(d23.deleteSequence[2], 30);
    {
        BST bst23;
        for (int k = 0; k < d23.initialKeyCount; ++k) bst23.insert(d23.initialKeys[k]);
        auto findKey = [](const TreeNode* root, int target) -> const TreeNode* {
            const TreeNode* c = root;
            while (c) {
                if (target == c->value) return c;
                c = (target < c->value) ? c->left : c->right;
            }
            return nullptr;
        };
        const TreeNode* n0 = findKey(bst23.getRoot(), d23.deleteSequence[0]);
        ASSERT_NE(n0, nullptr);
        EXPECT_TRUE(!n0->left && !n0->right); // Case 1: Leaf (80)
        bst23.deleteKey(d23.deleteSequence[0]);

        const TreeNode* n1 = findKey(bst23.getRoot(), d23.deleteSequence[1]);
        ASSERT_NE(n1, nullptr);
        EXPECT_TRUE((n1->left != nullptr) != (n1->right != nullptr)); // Case 2: One-Child (70 -> 60)
        bst23.deleteKey(d23.deleteSequence[1]);

        const TreeNode* n2 = findKey(bst23.getRoot(), d23.deleteSequence[2]);
        ASSERT_NE(n2, nullptr);
        EXPECT_TRUE(n2->left != nullptr && n2->right != nullptr); // Case 3: Two-Children (30 -> 20 & 40)
        bst23.deleteKey(d23.deleteSequence[2]);
        EXPECT_TRUE(bst23.isValidBST());
    }

    ASSERT_TRUE(loadLevelById(24, d24, aerr));
    EXPECT_EQ(d24.title, "2.4 Range Raid");
    EXPECT_EQ(d24.rangeLo, 30);
    EXPECT_EQ(d24.rangeHi, 70);
    EXPECT_EQ(computeOptimalSteps(d24), 5);

    ASSERT_TRUE(loadLevelById(25, d25, aerr));
    EXPECT_EQ(d25.title, "2.5 Boss: The Lazy Architect");
    EXPECT_EQ(computeOptimalSteps(d25), 15);

    // Specific structural assertions on loaded data
    LevelData d31;
    std::string err;
    ASSERT_TRUE(loadLevelById(31, d31, err));
    int viols[LEVEL_MAX_NODES];
    int nViols = findUnbalancedKeys(d31, viols, LEVEL_MAX_NODES);
    ASSERT_EQ(nViols, 2);
    EXPECT_EQ(viols[0], 30);
    EXPECT_EQ(viols[1], 20);

    LevelData d34;
    ASSERT_TRUE(loadLevelById(34, d34, err));
    EXPECT_EQ(d34.initialKeyCount, 2);
    EXPECT_EQ(d34.insertCount, 4);
    EXPECT_EQ(d34.inserts[0].key, 10);
    EXPECT_EQ(d34.inserts[0].pivotKey, 40);
    EXPECT_EQ(d34.inserts[3].key, 50);
    EXPECT_EQ(d34.inserts[3].pivotKey, 20);
}

// =============================================================================
// 3. Hint System
// =============================================================================

TEST(HintSystemTest, OncePerLevelNudgeAndStarCap) {
    LevelData d;
    std::string err;
    ASSERT_TRUE(loadLevelById(14, d, err));
    ASSERT_EQ(d.hintCount, 3);

    ScoringEngine sc;
    sc.begin(computeOptimalSteps(d));
    HintSystem hints;

    EXPECT_TRUE(hints.isAvailable());
    EXPECT_TRUE(hints.use(sc, d, 1));
    EXPECT_FALSE(hints.isAvailable());
    EXPECT_TRUE(hints.wasUsed());
    EXPECT_EQ(hints.shownText(), d.hints[1]);
    EXPECT_FALSE(hints.use(sc, d, 2)); // second use blocked
    EXPECT_EQ(hints.shownText(), d.hints[1]);

    // Even with optimal steps and zero penalties, stars are capped at 2
    sc.addStep(computeOptimalSteps(d));
    EXPECT_EQ(sc.stars(), 2);

    hints.reset();
    EXPECT_TRUE(hints.isAvailable());
}

// =============================================================================
// 4. Drag-and-Drop Controller
// =============================================================================

TEST(DragDropTest, PickupHoverSnapRejectCancelAndReslot) {
    DragDropController dd;
    int t10 = dd.addToken(10, 100.f, 100.f, 20.f);
    int t20 = dd.addToken(20, 200.f, 100.f, 20.f);
    int sAny = dd.addSlot(100.f, 300.f, 30.f, DRAG_ANY_VALUE);
    int sOnly10 = dd.addSlot(300.f, 300.f, 30.f, 10);

    // Click empty space
    EXPECT_FALSE(dd.press(500.f, 500.f));
    EXPECT_FALSE(dd.isDragging());

    // Pick up t20 and hover sOnly10 -> rejected preview & DROP_REJECTED on release
    EXPECT_TRUE(dd.press(205.f, 100.f));
    EXPECT_TRUE(dd.isDragging());
    EXPECT_EQ(dd.draggedTokenId(), t20);
    dd.move(305.f, 300.f);
    EXPECT_EQ(dd.hoveredSlotId(), sOnly10);
    EXPECT_FALSE(dd.hoveredSlotAccepts());

    DropResult rBad = dd.release(305.f, 300.f);
    EXPECT_EQ(rBad.outcome, DROP_REJECTED);
    EXPECT_EQ(rBad.slotId, sOnly10);
    EXPECT_EQ(dd.tokenInSlot(sOnly10), -1);
    EXPECT_FLOAT_EQ(dd.token(t20)->x, 200.f);
    EXPECT_FLOAT_EQ(dd.token(t20)->y, 100.f);

    // Drag t10 over empty space -> DROP_CANCELLED returns home
    EXPECT_TRUE(dd.press(100.f, 100.f));
    DropResult rCancel = dd.release(450.f, 450.f);
    EXPECT_EQ(rCancel.outcome, DROP_CANCELLED);
    EXPECT_FLOAT_EQ(dd.token(t10)->x, 100.f);

    // Drag t10 into sOnly10 -> DROP_SNAPPED
    EXPECT_TRUE(dd.press(100.f, 100.f));
    dd.move(300.f, 300.f);
    EXPECT_EQ(dd.hoveredSlotId(), sOnly10);
    EXPECT_TRUE(dd.hoveredSlotAccepts());
    DropResult rSnap = dd.release(300.f, 300.f);
    EXPECT_EQ(rSnap.outcome, DROP_SNAPPED);
    EXPECT_EQ(rSnap.slotId, sOnly10);
    EXPECT_EQ(dd.tokenInSlot(sOnly10), t10);
    EXPECT_FLOAT_EQ(dd.token(t10)->x, 300.f);
    EXPECT_FLOAT_EQ(dd.token(t10)->y, 300.f);

    // Move t10 from sOnly10 to sAny -> vacates sOnly10
    EXPECT_TRUE(dd.press(300.f, 300.f));
    DropResult rMove = dd.release(100.f, 300.f);
    EXPECT_EQ(rMove.outcome, DROP_SNAPPED);
    EXPECT_EQ(rMove.slotId, sAny);
    EXPECT_EQ(dd.tokenInSlot(sOnly10), -1);
    EXPECT_EQ(dd.tokenInSlot(sAny), t10);

    // Drag t20 onto occupied sAny -> DROP_REJECTED
    EXPECT_TRUE(dd.press(200.f, 100.f));
    DropResult rOcc = dd.release(100.f, 300.f);
    EXPECT_EQ(rOcc.outcome, DROP_REJECTED);
    EXPECT_EQ(dd.tokenInSlot(sAny), t10);
}

// =============================================================================
// 5. Act 3 Level Data, Real AVLTree Rotations & Order-Statistic Helper Checks
// =============================================================================

static int kthLargestCheck(const AVLTree& tree, int k, int& outComps) {
    outComps = 0;
    const AVLNode* cur = tree.getRoot();
    if (!cur || k < 1 || k > cur->size) return -1;
    int rem = k;
    while (cur) {
        int rs = cur->right ? cur->right->size : 0;
        outComps++;
        if (rem == rs + 1) return cur->key;
        if (rem <= rs) cur = cur->right;
        else { rem -= (rs + 1); cur = cur->left; }
    }
    return -1;
}

TEST(Act3AvlTests, Act3JsonRealRotationsSubtreeSizesAndKthLargest) {
    std::string err;
    LevelData d31, d32, d33, d34, d35;
    ASSERT_TRUE(loadLevelById(31, d31, err));
    ASSERT_TRUE(loadLevelById(32, d32, err));
    ASSERT_TRUE(loadLevelById(33, d33, err));
    ASSERT_TRUE(loadLevelById(34, d34, err));
    ASSERT_TRUE(loadLevelById(35, d35, err));

    EXPECT_EQ(d33.title, "3.3 Double Rotations (LR / RL)");
    EXPECT_EQ(computeOptimalSteps(d31), 2);
    EXPECT_EQ(computeOptimalSteps(d32), 2);
    EXPECT_EQ(computeOptimalSteps(d33), 4);
    EXPECT_EQ(computeOptimalSteps(d34), 4);
    EXPECT_EQ(computeOptimalSteps(d35), 2);

    // Real AVLTree::rotateRight (LL) & rotateLeft (RR) preserve validity and subtree sizes
    {
        AVLTree t;
        AVLNode* r = new AVLNode(30);
        r->left = new AVLNode(20);
        r->left->parent = r;
        r->left->left = new AVLNode(10);
        r->left->left->parent = r->left;
        t.setRoot(r);
        EventBatch dummy;
        t.updateHeightAndSize(r->left->left, dummy);
        t.updateHeightAndSize(r->left, dummy);
        t.updateHeightAndSize(r, dummy);
        EXPECT_EQ(t.balanceFactor(r), 2);

        AVLNode* newR = t.rotateRight(r, dummy, ROTATION_LL);
        t.setRoot(newR);
        EXPECT_TRUE(t.isValidAVL());
        ASSERT_NE(t.getRoot(), nullptr);
        EXPECT_EQ(t.getRoot()->key, 20);
        EXPECT_EQ(t.getRoot()->size, 3);
    }

    // Real AVLTree::rotateLeft + rotateRight (LR double rotation)
    {
        AVLTree t;
        AVLNode* r = new AVLNode(d33.puzzles[0].rootKey);
        r->left = new AVLNode(d33.puzzles[0].childKey);
        r->left->parent = r;
        r->left->right = new AVLNode(d33.puzzles[0].innerKey);
        r->left->right->parent = r->left;
        t.setRoot(r);
        EventBatch dummy;
        t.updateHeightAndSize(r->left->right, dummy);
        t.updateHeightAndSize(r->left, dummy);
        t.updateHeightAndSize(r, dummy);
        EXPECT_EQ(t.balanceFactor(r), 2);
        EXPECT_EQ(t.balanceFactor(r->left), -1);

        AVLNode* newC = t.rotateLeft(r->left, dummy, ROTATION_LR);
        r->left = newC;
        newC->parent = r;
        t.updateHeightAndSize(r, dummy);
        AVLNode* newR = t.rotateRight(r, dummy, ROTATION_LR);
        t.setRoot(newR);
        EXPECT_TRUE(t.isValidAVL());
        ASSERT_NE(t.getRoot(), nullptr);
        EXPECT_EQ(t.getRoot()->key, d33.puzzles[0].innerKey);
        EXPECT_EQ(t.getRoot()->size, 3);
    }

    // 3.5 Order-Statistic Boss: k-th MOST VALUABLE (largest) + live insert 22 LL-rebalance
    AVLTree boss;
    for (int i = 0; i < d35.initialKeyCount; ++i) boss.insert(d35.initialKeys[i]);
    ASSERT_TRUE(boss.isValidAVL());
    ASSERT_EQ(d35.kQueryCount, 2);

    int c1 = 0;
    int ans1 = kthLargestCheck(boss, d35.kQueries[0], c1);
    EXPECT_EQ(ans1, 50); // {10,20,25,30,40,50,60,70}: 3rd largest = 50 (not 3rd smallest 25)
    EXPECT_LE(c1, boss.getHeight());

    EventBatch rotBatch;
    boss.insert(22, rotBatch); // triggers LL rotation on 30
    EXPECT_TRUE(boss.isValidAVL());
    EXPECT_EQ(boss.getSize(), 9);
    int c2 = 0;
    int ans2 = kthLargestCheck(boss, d35.kQueries[1], c2);
    EXPECT_EQ(ans2, 25); // 6th largest of 9-node tree = 25
    EXPECT_LE(c2, boss.getHeight());
}

// =============================================================================
// 6. Act 4 Level Data (expected_sorted_result) & Ascending Heap-Sort Check
// =============================================================================

TEST(Act4HeapTests, ExpectedSortedResultAndAscendingHeapSortValidation) {
    std::string err;
    LevelData d41, d42, d43, d44, d45;
    ASSERT_TRUE(loadLevelById(41, d41, err));
    ASSERT_TRUE(loadLevelById(42, d42, err));
    ASSERT_TRUE(loadLevelById(43, d43, err));
    ASSERT_TRUE(loadLevelById(44, d44, err));
    ASSERT_TRUE(loadLevelById(45, d45, err));

    EXPECT_EQ(computeOptimalSteps(d41), 3);
    EXPECT_EQ(computeOptimalSteps(d42), 1);
    EXPECT_EQ(computeOptimalSteps(d43), 7);
    EXPECT_EQ(computeOptimalSteps(d44), 7);
    EXPECT_EQ(computeOptimalSteps(d45), 9);

    ASSERT_EQ(d43.expectedSortedResultCount, 7);
    const int exp43[] = { 40, 50, 60, 70, 75, 80, 95 };
    for (int i = 0; i < 7; ++i) EXPECT_EQ(d43.expectedSortedResult[i], exp43[i]);

    ASSERT_EQ(d44.expectedSortedResultCount, 7);
    const int exp44[] = { 30, 40, 50, 60, 70, 80, 90 };
    for (int i = 0; i < 7; ++i) EXPECT_EQ(d44.expectedSortedResult[i], exp44[i]);

    ASSERT_EQ(d45.expectedSortedResultCount, 8);
    const int exp45[] = { 15, 25, 30, 40, 60, 70, 80, 95 };
    for (int i = 0; i < 8; ++i) EXPECT_EQ(d45.expectedSortedResult[i], exp45[i]);

    const LevelData* sortLevels[3] = { &d43, &d44, &d45 };
    for (int li = 0; li < 3; ++li) {
        const LevelData& ld = *sortLevels[li];
        MaxHeap h;
        h.buildMaxHeap(ld.initialArray, ld.initialArrayCount);
        int ascTail[LEVEL_MAX_INTS];
        int descFront[LEVEL_MAX_INTS];
        int ascCount = 0;
        int descCount = 0;
        while (!h.isEmpty()) {
            int mv = h.extractMax();
            for (int k = ascCount; k > 0; --k) ascTail[k] = ascTail[k - 1];
            ascTail[0] = mv;
            ascCount++;
            descFront[descCount++] = mv;
        }
        ASSERT_EQ(ascCount, ld.expectedSortedResultCount);
        bool descMatchedAll = (descCount == ld.expectedSortedResultCount);
        for (int i = 0; i < ascCount; ++i) {
            EXPECT_EQ(ascTail[i], ld.expectedSortedResult[i]);
            if (descFront[i] != ld.expectedSortedResult[i]) descMatchedAll = false;
        }
        EXPECT_FALSE(descMatchedAll); // descending extraction order must never match ascending JSON
    }
}

// =============================================================================
// 7. Endless Mode (Operation Stream, Validator, Save/Load, Leaderboard)
// =============================================================================

TEST(EndlessModeTests, StreamGeneratorValidatorSaveLoadAndLeaderboard) {
    AVLTree tree;
    EndlessSession session;
    session.startNew(1337u, tree);

    EXPECT_TRUE(session.isActive());
    EXPECT_FALSE(session.isGameOver());
    EXPECT_EQ(session.getInitialSeed(), 1337u);
    EXPECT_EQ(tree.getSize(), 7);
    EXPECT_TRUE(EndlessSession::validateTreeFull(tree));

    float initialWindow = session.getOpWindowSec();

    // Run 40 mixed insert/delete operations and verify invariants before & after every op
    for (int step = 0; step < 40; ++step) {
        const EndlessOp& op = session.getPendingOp();
        if (op.type == ENDLESS_OP_DELETE) {
            EXPECT_TRUE(tree.search(op.key));
        } else {
            EXPECT_EQ(op.type, ENDLESS_OP_INSERT);
            EXPECT_FALSE(tree.search(op.key));
        }

        EXPECT_FALSE(session.tick(0.45f));
        EventBatch batch;
        ASSERT_TRUE(session.executePending(tree, batch));
        EXPECT_TRUE(EndlessSession::validateTreeFull(tree));
        EXPECT_GT(batch.getCount(), 0);
    }

    EXPECT_EQ(session.getOpsCompleted(), 40);
    EXPECT_GT(session.getSurvivalMs(), 17000);
    EXPECT_LT(session.getOpWindowSec(), initialWindow);

    // Save/load round-trip (preorder + null markers + seed + rng + elapsed time)
    const char* savePath = "test_endless_save_tmp.txt";
    ASSERT_TRUE(session.saveToFile(savePath, tree));

    int expectedSize = tree.getSize();
    int expectedRoot = tree.getRoot() ? tree.getRoot()->key : -1;
    unsigned int expectedRng = session.getRngState();
    int expectedSurvMs = session.getSurvivalMs();
    EndlessOp expectedPending = session.getPendingOp();

    AVLTree reloadedTree;
    EndlessSession reloadedSession;
    ASSERT_TRUE(reloadedSession.loadFromFile(savePath, reloadedTree));
    EXPECT_TRUE(EndlessSession::validateTreeFull(reloadedTree));
    EXPECT_EQ(reloadedTree.getSize(), expectedSize);
    ASSERT_NE(reloadedTree.getRoot(), nullptr);
    EXPECT_EQ(reloadedTree.getRoot()->key, expectedRoot);
    EXPECT_EQ(reloadedSession.getInitialSeed(), 1337u);
    EXPECT_EQ(reloadedSession.getRngState(), expectedRng);
    EXPECT_EQ(reloadedSession.getSurvivalMs(), expectedSurvMs);
    EXPECT_EQ(reloadedSession.getPendingOp().type, expectedPending.type);
    EXPECT_EQ(reloadedSession.getPendingOp().key, expectedPending.key);

    // Continued execution after resume matches uninterrupted run deterministically
    EventBatch bOrig, bReloaded;
    ASSERT_TRUE(session.executePending(tree, bOrig));
    ASSERT_TRUE(reloadedSession.executePending(reloadedTree, bReloaded));
    EXPECT_EQ(tree.getSize(), reloadedTree.getSize());
    EXPECT_EQ(session.getPendingOp().type, reloadedSession.getPendingOp().type);
    EXPECT_EQ(session.getPendingOp().key, reloadedSession.getPendingOp().key);

    // Deadline expiry triggers Game Over cleanly
    EXPECT_TRUE(reloadedSession.tick(reloadedSession.getOpTimeRemaining() + 0.1f));
    EXPECT_TRUE(reloadedSession.isGameOver());
    EXPECT_FALSE(reloadedSession.isActive());

    // Endless leaderboard write/read + descending sort round-trip
    const char* lbPath = "test_endless_lb_tmp.txt";
    std::remove(lbPath);
    EndlessLeaderboard lb;
    lb.load(lbPath);

    EndlessLeaderboardEntry e1, e2, e3;
    e1.nameDate = "RunA"; e1.survivalMs = 12400; e1.opsCompleted = 8;  e1.seed = 111u;
    e2.nameDate = "RunB"; e2.survivalMs = 45900; e2.opsCompleted = 22; e2.seed = 222u;
    e3.nameDate = "RunC"; e3.survivalMs = 27100; e3.opsCompleted = 14; e3.seed = 333u;
    ASSERT_TRUE(lb.recordRun(e1));
    ASSERT_TRUE(lb.recordRun(e2));
    ASSERT_TRUE(lb.recordRun(e3));

    EndlessLeaderboard lbReloaded;
    lbReloaded.load(lbPath);
    ASSERT_EQ(lbReloaded.getCount(), 3);
    EXPECT_EQ(lbReloaded.get(0).survivalMs, 45900);
    EXPECT_EQ(lbReloaded.get(0).nameDate, "RunB");
    EXPECT_EQ(lbReloaded.get(1).survivalMs, 27100);
    EXPECT_EQ(lbReloaded.get(2).survivalMs, 12400);

    std::remove(savePath);
    std::remove(lbPath);
}
