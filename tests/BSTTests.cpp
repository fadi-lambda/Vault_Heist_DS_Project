#include <gtest/gtest.h>
#include "structures/BST.h"

using namespace VaultHeist;

// 1. Basic Insertion, Search, and Duplicate Rejection
TEST(BSTTest, InsertSearchAndDuplicateRejection) {
    BST bst;
    EXPECT_TRUE(bst.isEmpty());
    EXPECT_EQ(bst.getSize(), 0);
    EXPECT_EQ(bst.getHeight(), 0);

    EXPECT_TRUE(bst.insert(50));
    EXPECT_TRUE(bst.insert(30));
    EXPECT_TRUE(bst.insert(70));
    EXPECT_TRUE(bst.insert(20));
    EXPECT_TRUE(bst.insert(40));
    EXPECT_EQ(bst.getSize(), 5);

    // Duplicates must be rejected
    EXPECT_FALSE(bst.insert(50));
    EXPECT_FALSE(bst.insert(30));
    EXPECT_FALSE(bst.insert(20));
    EXPECT_EQ(bst.getSize(), 5);

    // Search tests
    EXPECT_TRUE(bst.search(50));
    EXPECT_TRUE(bst.search(30));
    EXPECT_TRUE(bst.search(70));
    EXPECT_TRUE(bst.search(20));
    EXPECT_TRUE(bst.search(40));
    EXPECT_FALSE(bst.search(99));
    EXPECT_FALSE(bst.search(10));

    EXPECT_TRUE(bst.isValidBST());
}

// 2. Min and Max
TEST(BSTTest, FindMinAndFindMax) {
    BST bst;
    EXPECT_EQ(bst.findMin(), -1);
    EXPECT_EQ(bst.findMax(), -1);

    bst.insert(50);
    EXPECT_EQ(bst.findMin(), 50);
    EXPECT_EQ(bst.findMax(), 50);

    bst.insert(20);
    bst.insert(80);
    bst.insert(10);
    bst.insert(90);

    EXPECT_EQ(bst.findMin(), 10);
    EXPECT_EQ(bst.findMax(), 90);
    EXPECT_TRUE(bst.isValidBST());
}

// 3. Deletion Cases (Leaf, One Child Left, One Child Right, Two Children, Non-existent, Root)
TEST(BSTTest, DeleteAllCases) {
    // Construct tree:
    //          50
    //        /    \
    //       30     70
    //      /  \      \
    //     20  40      80
    //    /
    //   10
    BST bst;
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    bst.insert(20);
    bst.insert(40);
    bst.insert(80);
    bst.insert(10);
    EXPECT_EQ(bst.getSize(), 7);

    // Case 0: Delete key that does not exist -> returns false, size unchanged
    EXPECT_FALSE(bst.deleteKey(999));
    EXPECT_EQ(bst.getSize(), 7);
    EXPECT_TRUE(bst.isValidBST());

    // Case 1: Delete a leaf node (40)
    EXPECT_TRUE(bst.deleteKey(40));
    EXPECT_FALSE(bst.search(40));
    EXPECT_EQ(bst.getSize(), 6);
    EXPECT_TRUE(bst.isValidBST());

    // Case 2a: Delete a node with only left child (20, has left child 10)
    EXPECT_TRUE(bst.deleteKey(20));
    EXPECT_FALSE(bst.search(20));
    EXPECT_TRUE(bst.search(10));
    EXPECT_EQ(bst.getSize(), 5);
    EXPECT_TRUE(bst.isValidBST());

    // Case 2b: Delete a node with only right child (70, has right child 80)
    EXPECT_TRUE(bst.deleteKey(70));
    EXPECT_FALSE(bst.search(70));
    EXPECT_TRUE(bst.search(80));
    EXPECT_EQ(bst.getSize(), 4);
    EXPECT_TRUE(bst.isValidBST());

    // Re-insert some nodes to test 2-children deletion
    bst.insert(25);
    bst.insert(35);
    // Tree now has 30 with left (10, 25) and right (35)
    // Case 3: Delete a node with two children (30)
    EXPECT_TRUE(bst.deleteKey(30));
    EXPECT_FALSE(bst.search(30));
    EXPECT_TRUE(bst.search(25));
    EXPECT_TRUE(bst.search(35));
    EXPECT_TRUE(bst.isValidBST());

    // Case 4: Delete the root node
    int currentRootVal = bst.getRoot()->value;
    EXPECT_TRUE(bst.deleteKey(currentRootVal));
    EXPECT_FALSE(bst.search(currentRootVal));
    EXPECT_TRUE(bst.isValidBST());

    // Delete remaining nodes down to empty tree
    while (!bst.isEmpty()) {
        int m = bst.findMin();
        EXPECT_TRUE(bst.deleteKey(m));
        EXPECT_TRUE(bst.isValidBST());
    }
    EXPECT_TRUE(bst.isEmpty());
    EXPECT_EQ(bst.getSize(), 0);
    EXPECT_EQ(bst.getHeight(), 0);
}

// 4. Successor and Predecessor
TEST(BSTTest, SuccessorAndPredecessor) {
    BST bst;
    int keys[] = {20, 10, 30, 5, 15, 25, 35};
    for (int k : keys) bst.insert(k);

    // Sorted: 5, 10, 15, 20, 25, 30, 35
    EXPECT_EQ(bst.successor(5), 10);
    EXPECT_EQ(bst.successor(10), 15);
    EXPECT_EQ(bst.successor(15), 20);
    EXPECT_EQ(bst.successor(20), 25);
    EXPECT_EQ(bst.successor(25), 30);
    EXPECT_EQ(bst.successor(30), 35);
    EXPECT_EQ(bst.successor(35), -1); // Max node has no successor
    EXPECT_EQ(bst.successor(99), -1); // Missing node

    EXPECT_EQ(bst.predecessor(5), -1); // Min node has no predecessor
    EXPECT_EQ(bst.predecessor(10), 5);
    EXPECT_EQ(bst.predecessor(15), 10);
    EXPECT_EQ(bst.predecessor(20), 15);
    EXPECT_EQ(bst.predecessor(25), 20);
    EXPECT_EQ(bst.predecessor(30), 25);
    EXPECT_EQ(bst.predecessor(35), 30);
    EXPECT_EQ(bst.predecessor(99), -1);
}

// 5. Range Query
TEST(BSTTest, RangeQuery) {
    BST bst;
    int keys[] = {50, 20, 70, 10, 30, 60, 80};
    for (int k : keys) bst.insert(k);

    int count = 0;
    // Query [20, 60] -> {20, 30, 50, 60}
    int* res = bst.rangeQuery(20, 60, count);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(count, 4);
    int expected[] = {20, 30, 50, 60};
    for (int i = 0; i < count; ++i) EXPECT_EQ(res[i], expected[i]);
    delete[] res;

    // Range outside bounds -> empty result
    res = bst.rangeQuery(90, 100, count);
    EXPECT_EQ(res, nullptr);
    EXPECT_EQ(count, 0);

    // Full range [0, 100]
    res = bst.rangeQuery(0, 100, count);
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(count, 7);
    int allSorted[] = {10, 20, 30, 50, 60, 70, 80};
    for (int i = 0; i < count; ++i) EXPECT_EQ(res[i], allSorted[i]);
    delete[] res;
}

// 6. kthSmallest
TEST(BSTTest, KthSmallest) {
    BST bst;
    int keys[] = {40, 20, 60, 10, 30, 50, 70};
    for (int k : keys) bst.insert(k);

    // Sorted order: 10, 20, 30, 40, 50, 60, 70
    int sorted[] = {10, 20, 30, 40, 50, 60, 70};
    for (int k = 1; k <= 7; ++k) {
        EXPECT_EQ(bst.kthSmallest(k), sorted[k - 1]);
    }

    // Out of bounds
    EXPECT_EQ(bst.kthSmallest(0), -1);
    EXPECT_EQ(bst.kthSmallest(8), -1);
}

// 7. lcaBST
TEST(BSTTest, LCABST) {
    BST bst;
    int keys[] = {50, 30, 70, 20, 40, 60, 80};
    for (int k : keys) bst.insert(k);

    EXPECT_EQ(bst.lcaBST(20, 40), 30);
    EXPECT_EQ(bst.lcaBST(20, 80), 50);
    EXPECT_EQ(bst.lcaBST(60, 80), 70);
    EXPECT_EQ(bst.lcaBST(30, 40), 30);
    EXPECT_EQ(bst.lcaBST(50, 70), 50);
    EXPECT_EQ(bst.lcaBST(20, 99), -1);
}

// 8. Degenerate Case (Linear Chain)
TEST(BSTTest, DegenerateChainHeightEqualsN) {
    BST bst;
    int n = 15;
    for (int i = 1; i <= n; ++i) {
        EXPECT_TRUE(bst.insert(i));
    }
    EXPECT_EQ(bst.getSize(), n);
    EXPECT_EQ(bst.getHeight(), n);
    EXPECT_TRUE(bst.isValidBST());

    // Search and kthSmallest still work correctly on degenerate chain
    for (int i = 1; i <= n; ++i) {
        EXPECT_TRUE(bst.search(i));
        EXPECT_EQ(bst.kthSmallest(i), i);
    }
}
