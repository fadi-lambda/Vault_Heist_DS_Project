#include <gtest/gtest.h>
#include "MaxHeap.h"

using namespace VaultHeist;

// 1. Test basic insertion and max extraction
TEST(MaxHeapTest, InsertAndExtractMax) {
    MaxHeap heap(10);
    EXPECT_TRUE(heap.isEmpty());
    EXPECT_EQ(heap.getSize(), 0);

    // Insert numbers in random order
    heap.insert(10);
    heap.insert(40);
    heap.insert(20);
    heap.insert(50);
    heap.insert(30);

    EXPECT_EQ(heap.getSize(), 5);
    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(heap.getMax(), 50);

    // Extracts should return numbers in descending order
    EXPECT_EQ(heap.extractMax(), 50);
    EXPECT_TRUE(heap.isMaxHeap());

    EXPECT_EQ(heap.extractMax(), 40);
    EXPECT_TRUE(heap.isMaxHeap());

    EXPECT_EQ(heap.extractMax(), 30);
    EXPECT_TRUE(heap.isMaxHeap());

    EXPECT_EQ(heap.extractMax(), 20);
    EXPECT_TRUE(heap.isMaxHeap());

    EXPECT_EQ(heap.extractMax(), 10);
    EXPECT_TRUE(heap.isEmpty());
}

// 2. Test dynamic resizing when inserting more elements than initial capacity
TEST(MaxHeapTest, DynamicResizing) {
    MaxHeap heap(5); // Small initial capacity

    for (int i = 1; i <= 20; ++i) {
        heap.insert(i);
        EXPECT_TRUE(heap.isMaxHeap());
    }

    EXPECT_EQ(heap.getSize(), 20);
    EXPECT_EQ(heap.getMax(), 20);
}

// 3. Test BuildMaxHeap with different array types (Section 9.1 requirements)
TEST(MaxHeapTest, BuildMaxHeapVariousArrays) {
    MaxHeap heap;

    // A. Single element
    int singleArr[] = { 42 };
    heap.buildMaxHeap(singleArr, 1);
    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(heap.getMax(), 42);

    // B. Reverse-sorted array (worst case for normal, easy for max heap)
    int revArr[] = { 1, 2, 3, 4, 5, 6, 7 };
    heap.buildMaxHeap(revArr, 7);
    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(heap.getMax(), 7);

    // C. Already a max heap
    int alreadyHeap[] = { 100, 80, 70, 50, 40, 60, 30 };
    heap.buildMaxHeap(alreadyHeap, 7);
    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(heap.getMax(), 100);

    // D. Array with duplicate values
    int dupArr[] = { 15, 10, 20, 10, 20, 15, 5 };
    heap.buildMaxHeap(dupArr, 7);
    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(heap.getMax(), 20);

    // E. Random unsorted array
    int randomArr[] = { 35, 12, 89, 44, 2, 77, 60, 19, 51 };
    heap.buildMaxHeap(randomArr, 9);
    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(heap.getMax(), 89);
}

// 4. Test Heap Sort (Section 9.1 requirements)
TEST(MaxHeapTest, HeapSortCorrectness) {
    MaxHeap heap;

    // A. Empty array
    int emptyArr[] = { 0 };
    heap.heapSort(emptyArr, 0); // Should not crash

    // B. Single element
    int singleArr[] = { 99 };
    heap.heapSort(singleArr, 1);
    EXPECT_EQ(singleArr[0], 99);

    // C. Random array including duplicates and negatives
    int arr[] = { 15, -3, 50, 22, -10, 0, 50, 8, 12 };
    int n = sizeof(arr) / sizeof(arr[0]);

    heap.heapSort(arr, n);

    // Verify array is sorted in ascending order
    for (int i = 0; i < n - 1; ++i) {
        EXPECT_LE(arr[i], arr[i + 1]);
    }

    // Verify exact expected order
    int expected[] = { -10, -3, 0, 8, 12, 15, 22, 50, 50 };
    for (int i = 0; i < n; ++i) {
        EXPECT_EQ(arr[i], expected[i]);
    }
}
