#include "structures/MaxHeap.h"
#include <iostream>

namespace VaultHeist {

// Benchmark-only instrumentation counters
long long MaxHeap::s_comparisons = 0;
long long MaxHeap::s_swaps = 0;

void MaxHeap::resetCounters() {
    s_comparisons = 0;
    s_swaps = 0;
}

long long MaxHeap::getComparisons() {
    return s_comparisons;
}

long long MaxHeap::getSwaps() {
    return s_swaps;
}

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

// ---- Event helpers (file-local functions, not global variables) ----
static inline void emitHeap(EventBatch& events, int type, const char* label,
                            int indexA, int indexB, int oldValue, int newValue) {
    Event e(type, label);
    e.indexA = indexA;
    e.indexB = indexB;
    e.oldValue = oldValue;
    e.newValue = newValue;
    events.append(e);
}

static inline void emitOp(EventBatch& events, int type, const char* label) {
    events.append(Event(type, label));
}

// Swap two values at indices i and j
void MaxHeap::swap(int i, int j, EventBatch* events) {
    s_swaps++;
    if (events) {
        // HEAP_SWAP: indexA/indexB = positions, oldValue/newValue = values at i/j before swap
        emitHeap(*events, EVENT_HEAP_SWAP, "heap swap", i, j, data[i], data[j]);
    }
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
    siftUpImpl(index, nullptr);
}

void MaxHeap::siftUp(int index, EventBatch& events) {
    siftUpImpl(index, &events);
}

void MaxHeap::siftUpImpl(int index, EventBatch* events) {
    while (index > 0) {
        int parent = parentIndex(index);
        // If current element is greater than its parent, swap!
        s_comparisons++;
        if (data[index] > data[parent]) {
            swap(index, parent, events);
            index = parent; // Move up
        } else {
            break; // Valid position found
        }
    }
}

// Sift-Down: Moves an element downward until parent >= both children
void MaxHeap::siftDown(int index, int heapSize) {
    siftDownImpl(index, heapSize, nullptr);
}

void MaxHeap::siftDown(int index, int heapSize, EventBatch& events) {
    siftDownImpl(index, heapSize, &events);
}

void MaxHeap::siftDownImpl(int index, int heapSize, EventBatch* events) {
    if (events) emitHeap(*events, EVENT_HEAPIFY_START, "heapify start", index, -1, -1, heapSize);
    while (true) {
        int left = leftChildIndex(index);
        int right = rightChildIndex(index);
        int largest = index;

        // Check if left child is larger
        if (left < heapSize) {
            s_comparisons++;
            if (data[left] > data[largest]) {
                largest = left;
            }
        }

        // Check if right child is larger
        if (right < heapSize) {
            s_comparisons++;
            if (data[right] > data[largest]) {
                largest = right;
            }
        }

        // If largest is not the current element, swap and continue sinking
        if (largest != index) {
            swap(index, largest, events);
            index = largest; // Move down
        } else {
            break; // Valid position found
        }
    }
    // indexA = final resting index of the sifted element
    if (events) emitHeap(*events, EVENT_HEAPIFY_DONE, "heapify done", index, -1, -1, heapSize);
}

// Insert: Add to the end of the array, then siftUp
void MaxHeap::insert(int value) {
    insertImpl(value, nullptr);
}

void MaxHeap::insert(int value, EventBatch& events) {
    insertImpl(value, &events);
}

void MaxHeap::insertImpl(int value, EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "heap insert");
    if (size >= capacity) {
        resize(capacity * 2);
    }
    data[size] = value;
    if (events) emitHeap(*events, EVENT_INSERT, "heap insert", size, -1, -1, value);
    siftUpImpl(size, events);
    size++;
    if (events) {
        emitHeap(*events, EVENT_HEAP_SIZE_CHANGE, "heap size", -1, -1, size - 1, size);
        emitOp(*events, EVENT_OPERATION_COMPLETED, "heap insert");
    }
}

// Extract Max: Remove and return the maximum element (data[0])
int MaxHeap::extractMax() {
    return extractMaxImpl(nullptr);
}

int MaxHeap::extractMax(EventBatch& events) {
    return extractMaxImpl(&events);
}

int MaxHeap::extractMaxImpl(EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "extractMax");
    if (isEmpty()) {
        std::cerr << "Error: Heap is empty!\n";
        if (events) emitOp(*events, EVENT_OPERATION_FAILED, "extractMax: empty");
        return -1;
    }

    int maxValue = data[0];

    if (events) {
        // Event only: visualised as a root/last swap. (The array itself copies the
        // last element into the root; the old root leaves the active heap.)
        emitHeap(*events, EVENT_HEAP_SWAP, "root/last swap", 0, size - 1, data[0], data[size - 1]);
        emitHeap(*events, EVENT_EXTRACT_MAX, "extract max", 0, size - 1, maxValue, data[size - 1]);
    }

    // Swap root with the last element
    if (size > 1) s_swaps++;
    data[0] = data[size - 1];
    size--;
    if (events) emitHeap(*events, EVENT_HEAP_SIZE_CHANGE, "heap size", -1, -1, size + 1, size);

    // Sift down the new root to restore max heap property
    if (size > 0) {
        siftDownImpl(0, size, events);
    }

    if (events) emitOp(*events, EVENT_OPERATION_COMPLETED, "extractMax");
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
    buildMaxHeapImpl(arr, n, nullptr);
}

void MaxHeap::buildMaxHeap(const int* arr, int n, EventBatch& events) {
    buildMaxHeapImpl(arr, n, &events);
}

void MaxHeap::buildMaxHeapImpl(const int* arr, int n, EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "buildMaxHeap");
    if (n <= 0 || arr == nullptr) {
        if (events) emitOp(*events, EVENT_OPERATION_FAILED, "buildMaxHeap: empty input");
        return;
    }

    if (n > capacity) {
        resize(n * 2);
    }

    // Copy array elements
    for (int i = 0; i < n; ++i) {
        data[i] = arr[i];
    }
    int oldSize = size;  // event bookkeeping only
    size = n;
    if (events) {
        emitHeap(*events, EVENT_HEAP_BUILD, "build heap", -1, -1, -1, n);
        emitHeap(*events, EVENT_HEAP_SIZE_CHANGE, "heap size", -1, -1, oldSize, size);
    }

    // Start siftDown from the last non-leaf node down to root 0
    for (int i = (size / 2) - 1; i >= 0; --i) {
        siftDownImpl(i, size, events);
    }
    if (events) emitOp(*events, EVENT_OPERATION_COMPLETED, "buildMaxHeap");
}

// Heap Sort: sorts an array in ascending order
void MaxHeap::heapSort(int* arr, int n) {
    heapSortImpl(arr, n, nullptr);
}

void MaxHeap::heapSort(int* arr, int n, EventBatch& events) {
    heapSortImpl(arr, n, &events);
}

void MaxHeap::heapSortImpl(int* arr, int n, EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "heapSort");
    if (n <= 1 || arr == nullptr) {
        // Trivially sorted (0/1 element) or no array: nothing to do.
        if (events) {
            if (arr != nullptr && n == 1) {
                emitHeap(*events, EVENT_MARK_SORTED, "sorted", 0, -1, -1, arr[0]);
            }
            emitOp(*events, EVENT_HEAP_SORT_COMPLETE, "heap sort complete");
            emitOp(*events, EVENT_OPERATION_COMPLETED, "heapSort");
        }
        return;
    }

    // Step 1: Build max heap on the input array in-place
    if (events) emitHeap(*events, EVENT_HEAP_BUILD, "build heap", -1, -1, -1, n);
    for (int i = (n / 2) - 1; i >= 0; --i) {
        int curr = i;
        if (events) emitHeap(*events, EVENT_HEAPIFY_START, "heapify start", curr, -1, -1, n);
        while (true) {
            int left = 2 * curr + 1;
            int right = 2 * curr + 2;
            int largest = curr;

            if (left < n) {
                s_comparisons++;
                if (arr[left] > arr[largest]) largest = left;
            }
            if (right < n) {
                s_comparisons++;
                if (arr[right] > arr[largest]) largest = right;
            }

            if (largest != curr) {
                s_swaps++;
                if (events) emitHeap(*events, EVENT_HEAP_SWAP, "heap swap", curr, largest, arr[curr], arr[largest]);
                int temp = arr[curr];
                arr[curr] = arr[largest];
                arr[largest] = temp;
                curr = largest;
            } else {
                break;
            }
        }
        if (events) emitHeap(*events, EVENT_HEAPIFY_DONE, "heapify done", curr, -1, -1, n);
    }

    // Step 2: Repeatedly swap root with last active element and shrink heap
    for (int activeSize = n - 1; activeSize > 0; --activeSize) {
        s_swaps++;
        if (events) {
            emitHeap(*events, EVENT_HEAP_SWAP, "root/last swap", 0, activeSize, arr[0], arr[activeSize]);
            emitHeap(*events, EVENT_EXTRACT_MAX, "extract max", 0, activeSize, arr[0], arr[activeSize]);
        }
        // Swap root (max) with last element of active heap
        int temp = arr[0];
        arr[0] = arr[activeSize];
        arr[activeSize] = temp;
        if (events) {
            emitHeap(*events, EVENT_MARK_SORTED, "sorted", activeSize, -1, -1, arr[activeSize]);
            emitHeap(*events, EVENT_HEAP_SIZE_CHANGE, "heap size", -1, -1, activeSize + 1, activeSize);
        }

        // Restore heap for the remaining activeSize elements
        int curr = 0;
        if (events) emitHeap(*events, EVENT_HEAPIFY_START, "heapify start", curr, -1, -1, activeSize);
        while (true) {
            int left = 2 * curr + 1;
            int right = 2 * curr + 2;
            int largest = curr;

            if (left < activeSize) {
                s_comparisons++;
                if (arr[left] > arr[largest]) largest = left;
            }
            if (right < activeSize) {
                s_comparisons++;
                if (arr[right] > arr[largest]) largest = right;
            }

            if (largest != curr) {
                s_swaps++;
                if (events) emitHeap(*events, EVENT_HEAP_SWAP, "heap swap", curr, largest, arr[curr], arr[largest]);
                int t = arr[curr];
                arr[curr] = arr[largest];
                arr[largest] = t;
                curr = largest;
            } else {
                break;
            }
        }
        if (events) emitHeap(*events, EVENT_HEAPIFY_DONE, "heapify done", curr, -1, -1, activeSize);
    }

    // Index 0 holds the minimum once the loop ends — it is sorted too.
    if (events) {
        emitHeap(*events, EVENT_MARK_SORTED, "sorted", 0, -1, -1, arr[0]);
        emitOp(*events, EVENT_HEAP_SORT_COMPLETE, "heap sort complete");
        emitOp(*events, EVENT_OPERATION_COMPLETED, "heapSort");
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
