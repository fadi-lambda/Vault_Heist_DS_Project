#include "MaxHeap.h"
#include <iostream>

namespace VaultHeist {

// Constructor: allocate initial dynamic array
MaxHeap::MaxHeap(int initialCapacity) {
    if (initialCapacity < 5) initialCapacity = 5;
    capacity = initialCapacity;
    size = 0;
    data = new int[capacity];
}

// Destructor: free dynamic array to prevent memory leaks
MaxHeap::~MaxHeap() {
    delete[] data;
    data = nullptr;
    size = 0;
    capacity = 0;
}

// Swap two values at indices i and j
void MaxHeap::swap(int i, int j) {
    int temp = data[i];
    data[i] = data[j];
    data[j] = temp;
}

// Double array capacity when full
void MaxHeap::resize(int newCapacity) {
    int* newData = new int[newCapacity];
    for (int i = 0; i < size; ++i) {
        newData[i] = data[i];
    }
    delete[] data;
    data = newData;
    capacity = newCapacity;
}

// Sift-Up: Moves an element upward until parent >= child
void MaxHeap::siftUp(int index) {
    while (index > 0) {
        int parent = parentIndex(index);
        // If current element is greater than its parent, swap!
        if (data[index] > data[parent]) {
            swap(index, parent);
            index = parent; // Move up
        } else {
            break; // Valid position found
        }
    }
}

// Sift-Down: Moves an element downward until parent >= both children
void MaxHeap::siftDown(int index, int heapSize) {
    while (true) {
        int left = leftChildIndex(index);
        int right = rightChildIndex(index);
        int largest = index;

        // Check if left child is larger
        if (left < heapSize && data[left] > data[largest]) {
            largest = left;
        }

        // Check if right child is larger
        if (right < heapSize && data[right] > data[largest]) {
            largest = right;
        }

        // If largest is not the current element, swap and continue sinking
        if (largest != index) {
            swap(index, largest);
            index = largest; // Move down
        } else {
            break; // Valid position found
        }
    }
}

// Insert: Add to the end of the array, then siftUp
void MaxHeap::insert(int value) {
    if (size >= capacity) {
        resize(capacity * 2);
    }
    data[size] = value;
    siftUp(size);
    size++;
}

// Extract Max: Remove and return the maximum element (data[0])
int MaxHeap::extractMax() {
    if (isEmpty()) {
        std::cerr << "Error: Heap is empty!\n";
        return -1;
    }

    int maxValue = data[0];

    // Swap root with the last element
    data[0] = data[size - 1];
    size--;

    // Sift down the new root to restore max heap property
    if (size > 0) {
        siftDown(0, size);
    }

    return maxValue;
}

// Return root without removing
int MaxHeap::getMax() const {
    if (isEmpty()) {
        std::cerr << "Error: Heap is empty!\n";
        return -1;
    }
    return data[0];
}

// Build Max Heap: bottom-up O(n) construction from an unsorted array
void MaxHeap::buildMaxHeap(const int* arr, int n) {
    if (n <= 0 || arr == nullptr) return;

    if (n > capacity) {
        resize(n * 2);
    }

    // Copy array elements
    for (int i = 0; i < n; ++i) {
        data[i] = arr[i];
    }
    size = n;

    // Start siftDown from the last non-leaf node down to root 0
    for (int i = (size / 2) - 1; i >= 0; --i) {
        siftDown(i, size);
    }
}

// Heap Sort: sorts an array in ascending order
void MaxHeap::heapSort(int* arr, int n) {
    if (n <= 1 || arr == nullptr) return;

    // Step 1: Build max heap on the input array in-place
    for (int i = (n / 2) - 1; i >= 0; --i) {
        int curr = i;
        while (true) {
            int left = 2 * curr + 1;
            int right = 2 * curr + 2;
            int largest = curr;

            if (left < n && arr[left] > arr[largest]) largest = left;
            if (right < n && arr[right] > arr[largest]) largest = right;

            if (largest != curr) {
                int temp = arr[curr];
                arr[curr] = arr[largest];
                arr[largest] = temp;
                curr = largest;
            } else {
                break;
            }
        }
    }

    // Step 2: Repeatedly swap root with last active element and shrink heap
    for (int activeSize = n - 1; activeSize > 0; --activeSize) {
        // Swap root (max) with last element of active heap
        int temp = arr[0];
        arr[0] = arr[activeSize];
        arr[activeSize] = temp;

        // Restore heap for the remaining activeSize elements
        int curr = 0;
        while (true) {
            int left = 2 * curr + 1;
            int right = 2 * curr + 2;
            int largest = curr;

            if (left < activeSize && arr[left] > arr[largest]) largest = left;
            if (right < activeSize && arr[right] > arr[largest]) largest = right;

            if (largest != curr) {
                int t = arr[curr];
                arr[curr] = arr[largest];
                arr[largest] = t;
                curr = largest;
            } else {
                break;
            }
        }
    }
}

// Check if array satisfies Max Heap property: parent >= children
bool MaxHeap::isMaxHeap() const {
    for (int i = 0; i <= (size - 2) / 2; ++i) {
        int left = leftChildIndex(i);
        int right = rightChildIndex(i);

        if (left < size && data[i] < data[left]) return false;
        if (right < size && data[i] < data[right]) return false;
    }
    return true;
}

int MaxHeap::getElementAt(int index) const {
    if (index >= 0 && index < size) {
        return data[index];
    }
    return -1;
}

void MaxHeap::printHeap() const {
    std::cout << "Heap [size=" << size << "]: ";
    for (int i = 0; i < size; ++i) {
        std::cout << data[i] << " ";
    }
    std::cout << "\n";
}

} // namespace VaultHeist
