#include <gtest/gtest.h>
#include "structures/MaxHeap.h"

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

// ---------------- Phase 6: event emission (additions only) ----------------

// 5. buildMaxHeap emits HEAP_BUILD + one HEAP_SWAP per real swap
TEST(MaxHeapTest, EventsBuildMaxHeap) {
    MaxHeap heap(10);
    int arr[] = { 1, 2, 3 };
    EventBatch events;
    heap.buildMaxHeap(arr, 3, events);

    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(events.countOfType(EVENT_HEAP_BUILD), 1);
    EXPECT_EQ(events.countOfType(EVENT_HEAP_SWAP), 1);      // 1 <-> 3
    EXPECT_EQ(events.countOfType(EVENT_HEAPIFY_START), 1);  // only index 0 is non-leaf
    EXPECT_EQ(events.countOfType(EVENT_HEAPIFY_DONE), 1);
    for (int i = 0; i < events.getCount(); ++i) {
        Event e = events.getEventAt(i);
        if (e.type == EVENT_HEAP_SWAP) {
            EXPECT_EQ(e.indexA, 0);
            EXPECT_EQ(e.indexB, 2);
            EXPECT_EQ(e.oldValue, 1);
            EXPECT_EQ(e.newValue, 3);
        }
    }
}

// 6. extractMax emits root/last swap, EXTRACT_MAX, HEAP_SIZE_CHANGE
TEST(MaxHeapTest, EventsExtractMax) {
    MaxHeap heap(10);
    heap.insert(10);
    heap.insert(40);
    heap.insert(20);

    EventBatch events;
    EXPECT_EQ(heap.extractMax(events), 40);
    EXPECT_TRUE(heap.isMaxHeap());
    EXPECT_EQ(events.countOfType(EVENT_EXTRACT_MAX), 1);
    EXPECT_GE(events.countOfType(EVENT_HEAP_SWAP), 1);
    bool sawSize = false;
    for (int i = 0; i < events.getCount(); ++i) {
        Event e = events.getEventAt(i);
        if (e.type == EVENT_HEAP_SIZE_CHANGE) {
            EXPECT_EQ(e.oldValue, 3);
            EXPECT_EQ(e.newValue, 2);
            sawSize = true;
        }
        if (e.type == EVENT_EXTRACT_MAX) EXPECT_EQ(e.oldValue, 40);
    }
    EXPECT_TRUE(sawSize);

    MaxHeap empty(5);
    EventBatch fail;
    EXPECT_EQ(empty.extractMax(fail), -1);
    EXPECT_EQ(fail.getEventAt(fail.getCount() - 1).type, EVENT_OPERATION_FAILED);
}

// 7. heapSort marks every index sorted exactly once and ends with SORT_COMPLETE
TEST(MaxHeapTest, EventsHeapSort) {
    MaxHeap heap(10);
    int arr[] = { 5, 1, 4, 2, 3 };
    EventBatch events;
    heap.heapSort(arr, 5, events);

    for (int i = 0; i < 5; ++i) EXPECT_EQ(arr[i], i + 1);
    EXPECT_EQ(events.countOfType(EVENT_MARK_SORTED), 5);
    EXPECT_EQ(events.countOfType(EVENT_EXTRACT_MAX), 4);
    EXPECT_EQ(events.countOfType(EVENT_HEAP_SORT_COMPLETE), 1);
    int marked[5] = { 0, 0, 0, 0, 0 };
    for (int i = 0; i < events.getCount(); ++i) {
        Event e = events.getEventAt(i);
        if (e.type == EVENT_MARK_SORTED) marked[e.indexA]++;
    }
    for (int i = 0; i < 5; ++i) EXPECT_EQ(marked[i], 1);
    EXPECT_EQ(events.getEventAt(events.getCount() - 2).type, EVENT_HEAP_SORT_COMPLETE);
    EXPECT_EQ(events.getEventAt(events.getCount() - 1).type, EVENT_OPERATION_COMPLETED);
}
