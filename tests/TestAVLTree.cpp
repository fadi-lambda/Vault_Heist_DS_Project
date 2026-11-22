#include <gtest/gtest.h>
#include "structures/AVLTree.h"

using namespace VaultHeist;

// 1. Test LL Rotation (Single Right Rotation)
TEST(AVLTreeTest, LeftLeftRotation) {
    AVLTree tree;
    // Inserting 30, 20, 10 produces an LL imbalance at 30
    tree.insert(30);
    tree.insert(20);
    tree.insert(10);

    EXPECT_TRUE(tree.isBalanced());
    EXPECT_EQ(tree.getRoot()->key, 20); // 20 becomes the new root
    EXPECT_EQ(tree.getRoot()->left->key, 10);
    EXPECT_EQ(tree.getRoot()->right->key, 30);
    EXPECT_EQ(tree.getHeight(), 2);
    EXPECT_EQ(tree.getSize(), 3);
}

// 2. Test RR Rotation (Single Left Rotation)
TEST(AVLTreeTest, RightRightRotation) {
    AVLTree tree;
    // Inserting 10, 20, 30 produces an RR imbalance at 10
    tree.insert(10);
    tree.insert(20);
    tree.insert(30);

    EXPECT_TRUE(tree.isBalanced());
    EXPECT_EQ(tree.getRoot()->key, 20); // 20 becomes the new root
    EXPECT_EQ(tree.getRoot()->left->key, 10);
    EXPECT_EQ(tree.getRoot()->right->key, 30);
    EXPECT_EQ(tree.getHeight(), 2);
    EXPECT_EQ(tree.getSize(), 3);
}

// 3. Test LR Rotation (Double: Left then Right)
TEST(AVLTreeTest, LeftRightRotation) {
    AVLTree tree;
    // Inserting 30, 10, 20 produces an LR imbalance at 30
    tree.insert(30);
    tree.insert(10);
    tree.insert(20);

    EXPECT_TRUE(tree.isBalanced());
    EXPECT_EQ(tree.getRoot()->key, 20); // 20 becomes the new root
    EXPECT_EQ(tree.getRoot()->left->key, 10);
    EXPECT_EQ(tree.getRoot()->right->key, 30);
    EXPECT_EQ(tree.getHeight(), 2);
    EXPECT_EQ(tree.getSize(), 3);
}

// 4. Test RL Rotation (Double: Right then Left)
TEST(AVLTreeTest, RightLeftRotation) {
    AVLTree tree;
    // Inserting 10, 30, 20 produces an RL imbalance at 10
    tree.insert(10);
    tree.insert(30);
    tree.insert(20);

    EXPECT_TRUE(tree.isBalanced());
    EXPECT_EQ(tree.getRoot()->key, 20); // 20 becomes the new root
    EXPECT_EQ(tree.getRoot()->left->key, 10);
    EXPECT_EQ(tree.getRoot()->right->key, 30);
    EXPECT_EQ(tree.getHeight(), 2);
    EXPECT_EQ(tree.getSize(), 3);
}

// 5. Test Duplicate Rejection (Section 5.3 & 5.4)
TEST(AVLTreeTest, DuplicateRejection) {
    AVLTree tree;
    EXPECT_TRUE(tree.insert(50));
    EXPECT_FALSE(tree.insert(50)); // Duplicate must be rejected
    EXPECT_EQ(tree.getSize(), 1);
}

// 6. Test Deletion with rebalancing (Leaf, One Child, Two Children)
TEST(AVLTreeTest, DeleteWithRebalancing) {
    AVLTree tree;
    // Insert: 50, 25, 75, 10, 30, 60, 80, 5, 15, 27, 35
    int keys[] = { 50, 25, 75, 10, 30, 60, 80, 5, 15, 27, 35 };
    for (int k : keys) {
        tree.insert(k);
    }
    EXPECT_TRUE(tree.isBalanced());
    EXPECT_EQ(tree.getSize(), 11);

    // Delete a leaf (5)
    EXPECT_TRUE(tree.deleteKey(5));
    EXPECT_TRUE(tree.isBalanced());
    EXPECT_FALSE(tree.search(5));

    // Delete node with one child (10 now has only 15)
    EXPECT_TRUE(tree.deleteKey(10));
    EXPECT_TRUE(tree.isBalanced());
    EXPECT_FALSE(tree.search(10));

    // Delete node with two children (50 - root)
    EXPECT_TRUE(tree.deleteKey(50));
    EXPECT_TRUE(tree.isBalanced());
    EXPECT_FALSE(tree.search(50));

    // Attempt to delete non-existent key
    EXPECT_FALSE(tree.deleteKey(999));
}

// 7. Test Order Statistics in O(log n) (Boss Level 3.5: kth, rank, countInRange)
TEST(AVLTreeTest, OrderStatistics) {
    AVLTree tree;
    // Sorted keys: 10, 20, 30, 40, 50, 60, 70
    int keys[] = { 40, 20, 60, 10, 30, 50, 70 };
    for (int k : keys) {
        tree.insert(k);
    }

    // kth smallest (1-based index)
    EXPECT_EQ(tree.kth(1), 10);
    EXPECT_EQ(tree.kth(2), 20);
    EXPECT_EQ(tree.kth(3), 30);
    EXPECT_EQ(tree.kth(4), 40);
    EXPECT_EQ(tree.kth(5), 50);
    EXPECT_EQ(tree.kth(6), 60);
    EXPECT_EQ(tree.kth(7), 70);

    // rank (number of elements <= key)
    EXPECT_EQ(tree.rank(5), 0);
    EXPECT_EQ(tree.rank(10), 1);
    EXPECT_EQ(tree.rank(25), 2);
    EXPECT_EQ(tree.rank(40), 4);
    EXPECT_EQ(tree.rank(70), 7);
    EXPECT_EQ(tree.rank(100), 7);

    // countInRange [lo, hi]
    EXPECT_EQ(tree.countInRange(20, 50), 4); // 20, 30, 40, 50
    EXPECT_EQ(tree.countInRange(10, 70), 7);
    EXPECT_EQ(tree.countInRange(25, 35), 1); // only 30
}

// 8. Test Balance on Large Sequential Insert (Boss Level 2.5 vs Act 3 comparison)
TEST(AVLTreeTest, LargeSequentialInsertionMaintainsLogNHeight) {
    AVLTree tree;
    // Inserting 1 to 63 in sorted order would create a BST of height 63
    // But in AVL, height must be at most ceil(1.44 * log2(64)) = ~8!
    for (int i = 1; i <= 63; ++i) {
        tree.insert(i);
        EXPECT_TRUE(tree.isBalanced());
    }

    EXPECT_EQ(tree.getSize(), 63);
    EXPECT_LE(tree.getHeight(), 7); // Max height for 63 nodes in AVL is 6 or 7
}

// ---------------- Phase 6: event emission (additions only) ----------------

static int countRotations(const EventBatch& events, int rotationCase) {
    int n = 0;
    for (int i = 0; i < events.getCount(); ++i) {
        Event e = events.getEventAt(i);
        if (e.type == EVENT_ROTATE && e.rotationCase == rotationCase) n++;
    }
    return n;
}

// 9. LL insert emits exactly one ROTATE tagged ROTATION_LL
TEST(AVLTreeTest, EventsLLRotation) {
    AVLTree tree;
    tree.insert(30);
    tree.insert(20);
    EventBatch events;
    EXPECT_TRUE(tree.insert(10, events));

    EXPECT_EQ(events.countOfType(EVENT_ROTATE), 1);
    EXPECT_EQ(countRotations(events, ROTATION_LL), 1);
    EXPECT_EQ(events.countOfType(EVENT_BALANCE_ALERT), 1);
    EXPECT_EQ(events.countOfType(EVENT_INSERT), 1);
    EXPECT_GE(events.countOfType(EVENT_UPDATE_HEIGHT), 1);
    EXPECT_EQ(events.getEventAt(0).type, EVENT_OPERATION_STARTED);
    EXPECT_EQ(events.getEventAt(events.getCount() - 1).type, EVENT_OPERATION_COMPLETED);
    // INSERT happens before the ROTATE
    int insertIdx = -1, rotateIdx = -1;
    for (int i = 0; i < events.getCount(); ++i) {
        if (events.getEventAt(i).type == EVENT_INSERT) insertIdx = i;
        if (events.getEventAt(i).type == EVENT_ROTATE) rotateIdx = i;
    }
    EXPECT_LT(insertIdx, rotateIdx);
    EXPECT_EQ(events.getEventAt(rotateIdx).nodeId, 30);      // pivot
    EXPECT_EQ(events.getEventAt(rotateIdx).otherNodeId, 20); // new subtree root
    EXPECT_TRUE(tree.isValidAVL());
}

// 10. RR / LR / RL cases are tagged correctly (double rotations emit two ROTATEs)
TEST(AVLTreeTest, EventsRotationCases) {
    {
        AVLTree t; t.insert(10); t.insert(20);
        EventBatch e; t.insert(30, e);
        EXPECT_EQ(countRotations(e, ROTATION_RR), 1);
        EXPECT_EQ(e.countOfType(EVENT_ROTATE), 1);
        EXPECT_TRUE(t.isValidAVL());
    }
    {
        AVLTree t; t.insert(30); t.insert(10);
        EventBatch e; t.insert(20, e);
        EXPECT_EQ(countRotations(e, ROTATION_LR), 2);
        EXPECT_EQ(e.countOfType(EVENT_ROTATE), 2);
        EXPECT_TRUE(t.isValidAVL());
    }
    {
        AVLTree t; t.insert(10); t.insert(30);
        EventBatch e; t.insert(20, e);
        EXPECT_EQ(countRotations(e, ROTATION_RL), 2);
        EXPECT_EQ(e.countOfType(EVENT_ROTATE), 2);
        EXPECT_TRUE(t.isValidAVL());
    }
}

// 11. Duplicate insert / search / delete events
TEST(AVLTreeTest, EventsSearchDeleteDuplicate) {
    AVLTree tree;
    int keys[] = { 40, 20, 60, 10, 30, 50, 70 };
    for (int k : keys) tree.insert(k);

    EventBatch dup;
    EXPECT_FALSE(tree.insert(30, dup));
    EXPECT_EQ(dup.countOfType(EVENT_INSERT), 0);
    EXPECT_EQ(dup.getEventAt(dup.getCount() - 1).type, EVENT_OPERATION_FAILED);

    EventBatch s;
    EXPECT_TRUE(tree.search(30, s));
    EXPECT_EQ(s.countOfType(EVENT_COMPARE), 3); // 40, 20, 30

    EventBatch del;
    EXPECT_TRUE(tree.deleteKey(40, del));       // two children -> successor 50
    EXPECT_EQ(del.countOfType(EVENT_SWAP_VALUE), 1);
    EXPECT_EQ(del.countOfType(EVENT_UNLINK), 1);
    EXPECT_GE(del.countOfType(EVENT_UPDATE_SIZE), 1);
    EXPECT_TRUE(tree.isValidAVL());
    EXPECT_EQ(tree.getSize(), 6);
}

