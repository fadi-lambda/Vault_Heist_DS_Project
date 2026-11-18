#include <gtest/gtest.h>
#include "game/Json.h"
#include "game/LevelData.h"
#include "game/LevelLoader.h"
#include "game/Scoring.h"
#include "game/HintSystem.h"
#include "gui/DragDrop.h"

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
        7, 2, 2, 2, 1,
        3, 3, 3, 2, 6,
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
    ASSERT_TRUE(loadLevelById(21, d, err));
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
