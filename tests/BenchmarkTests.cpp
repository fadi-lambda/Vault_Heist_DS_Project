#include <gtest/gtest.h>
#include "structures/BST.h"
#include "structures/AVLTree.h"
#include "structures/MaxHeap.h"

using namespace VaultHeist;

TEST(BenchmarkCounterTest, BSTComparisonCountersAndDeepChain) {
    BST bst;
    BST::resetCounters();
    EXPECT_EQ(BST::getComparisons(), 0);

    bst.insert(50); // root: 0 comparisons
    EXPECT_EQ(BST::getComparisons(), 0);
    bst.insert(30); // compares 50: 1 comparison
    EXPECT_EQ(BST::getComparisons(), 1);
    bst.insert(70); // compares 50: 1 comparison
    EXPECT_EQ(BST::getComparisons(), 2);
    bst.insert(20); // compares 50, 30: 2 comparisons
    EXPECT_EQ(BST::getComparisons(), 4);

    BST::resetCounters();
    EXPECT_TRUE(bst.search(20));
    EXPECT_EQ(BST::getComparisons(), 3); // 50, 30, 20

    BST::resetCounters();
    EXPECT_EQ(bst.kthSmallest(2), 30);
    EXPECT_EQ(BST::getComparisons(), 2); // visited 20, 30

    // Verify deep sorted chain does not overflow stack
    BST chain;
    for (int i = 1; i <= 10000; ++i) {
        chain.insert(i);
    }
    EXPECT_EQ(chain.getHeight(), 10000);
    EXPECT_TRUE(chain.search(10000));
    EXPECT_EQ(chain.kthSmallest(10000), 10000);
}

TEST(BenchmarkCounterTest, AVLRotationAndComparisonCounters) {
    // LL -> 1 rotation
    {
        AVLTree t;
        AVLTree::resetCounters();
        t.insert(30);
        t.insert(20);
        t.insert(10);
        EXPECT_EQ(AVLTree::getRotations(), 1);
        EXPECT_GT(AVLTree::getComparisons(), 0);
    }
    // LR -> 2 single rotations
    {
        AVLTree t;
        AVLTree::resetCounters();
        t.insert(30);
        t.insert(10);
        t.insert(20);
        EXPECT_EQ(AVLTree::getRotations(), 2);
    }
    // RL -> 2 single rotations
    {
        AVLTree t;
        AVLTree::resetCounters();
        t.insert(10);
        t.insert(30);
        t.insert(20);
        EXPECT_EQ(AVLTree::getRotations(), 2);
    }
    // kth / rank increment comparisons
    {
        AVLTree t;
        for (int i = 1; i <= 15; ++i) t.insert(i);
        AVLTree::resetCounters();
        EXPECT_EQ(t.kth(8), 8);
        EXPECT_GT(AVLTree::getComparisons(), 0);
        AVLTree::resetCounters();
        EXPECT_EQ(t.rank(8), 8);
        EXPECT_GT(AVLTree::getComparisons(), 0);
    }
}

TEST(BenchmarkCounterTest, MaxHeapCountersAndLinearBuild) {
    const int n = 1000;
    int arr[n];
    for (int i = 0; i < n; ++i) arr[i] = i + 1; // ascending: worst case for buildMaxHeap

    MaxHeap heap(n);
    MaxHeap::resetCounters();
    heap.buildMaxHeap(arr, n);
    EXPECT_TRUE(heap.isMaxHeap());
    // Bottom-up buildMaxHeap performs at most 2*n comparisons
    EXPECT_GT(MaxHeap::getComparisons(), 0);
    EXPECT_LE(MaxHeap::getComparisons(), 2 * n);
    EXPECT_GT(MaxHeap::getSwaps(), 0);

    MaxHeap::resetCounters();
    heap.heapSort(arr, n);
    for (int i = 0; i < n - 1; ++i) {
        EXPECT_LE(arr[i], arr[i + 1]);
    }
    EXPECT_GT(MaxHeap::getComparisons(), 0);
    EXPECT_GT(MaxHeap::getSwaps(), 0);
}
