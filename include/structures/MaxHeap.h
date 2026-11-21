#pragma once

#include <string>
#include "events/Event.h"

namespace VaultHeist {

class MaxHeap {
private:
    int* data;         // Dynamic array (NO std::vector allowed)
    int capacity;      // Maximum allocated capacity
    int size;          // Current number of elements in the active heap

    // Benchmark-only instrumentation counters (reset before each measured run)
    static long long s_comparisons;
    static long long s_swaps;

    // Helper functions for parent and children indices
    int parentIndex(int i) const { return (i - 1) / 2; }
    int leftChildIndex(int i) const { return 2 * i + 1; }
    int rightChildIndex(int i) const { return 2 * i + 2; }

    // Resize array if it runs out of capacity
    void resize(int newCapacity);

    // Swap two elements in the array (emits EVENT_HEAP_SWAP when events != nullptr)
    void swap(int i, int j, EventBatch* events);

    // Internal implementations (nullptr skips event recording)
    void siftUpImpl(int index, EventBatch* events);
    void siftDownImpl(int index, int heapSize, EventBatch* events);
    void insertImpl(int value, EventBatch* events);
    int extractMaxImpl(EventBatch* events);
    void buildMaxHeapImpl(const int* arr, int n, EventBatch* events);
    void heapSortImpl(int* arr, int n, EventBatch* events);

public:
    // Constructor: creates an empty heap with initial capacity
    MaxHeap(int initialCapacity = 20);

    // Destructor: frees allocated memory
    ~MaxHeap();

    // Benchmark-only Counter API
    static void resetCounters();
    static long long getComparisons();
    static long long getSwaps();

    // Core Heap Operations
    void siftUp(int index);
    void siftDown(int index, int heapSize);
    void insert(int value);
    int extractMax();
    int getMax() const;

    // Bulk Operations (Section 5.5)
    void buildMaxHeap(const int* arr, int n);
    void heapSort(int* arr, int n);

    // Event-emitting overloads (PLAN.md §6.4/§6.5) — identical behaviour,
    // additionally append to `events`. Heap events use indexA/indexB for
    // array positions and oldValue/newValue for the values involved.
    //   siftUp/siftDown -> EVENT_HEAP_SWAP per actual swap
    //   siftDown        -> bracketed by EVENT_HEAPIFY_START / EVENT_HEAPIFY_DONE
    //   extractMax      -> HEAP_SWAP(root,last), EXTRACT_MAX, HEAP_SIZE_CHANGE
    //   buildMaxHeap    -> EVENT_HEAP_BUILD + heapify/swap events
    //   heapSort        -> swap/extract events, MARK_SORTED per index,
    //                      HEAP_SORT_COMPLETE at the end
    void siftUp(int index, EventBatch& events);
    void siftDown(int index, int heapSize, EventBatch& events);
    void insert(int value, EventBatch& events);
    int extractMax(EventBatch& events);
    void buildMaxHeap(const int* arr, int n, EventBatch& events);
    void heapSort(int* arr, int n, EventBatch& events);

    // Invariant Check (Section 5.6)
    bool isMaxHeap() const;

    // Getters
    int getSize() const { return size; }
    int getCapacity() const { return capacity; }
    bool isEmpty() const { return size == 0; }
    int getElementAt(int index) const;

    // Debug print
    void printHeap() const;
};

} // namespace VaultHeist
