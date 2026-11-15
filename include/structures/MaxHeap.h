#pragma once

#include <string>

namespace VaultHeist {

class MaxHeap {
private:
    int* data;         // Dynamic array (NO std::vector allowed)
    int capacity;      // Maximum allocated capacity
    int size;          // Current number of elements in the active heap

    // Helper functions for parent and children indices
    int parentIndex(int i) const { return (i - 1) / 2; }
    int leftChildIndex(int i) const { return 2 * i + 1; }
    int rightChildIndex(int i) const { return 2 * i + 2; }

    // Resize array if it runs out of capacity
    void resize(int newCapacity);

    // Swap two elements in the array
    void swap(int i, int j);

public:
    // Constructor: creates an empty heap with initial capacity
    MaxHeap(int initialCapacity = 20);

    // Destructor: frees allocated memory
    ~MaxHeap();

    // Core Heap Operations
    void siftUp(int index);
    void siftDown(int index, int heapSize);
    void insert(int value);
    int extractMax();
    int getMax() const;

    // Bulk Operations (Section 5.5)
    void buildMaxHeap(const int* arr, int n);
    void heapSort(int* arr, int n);

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
