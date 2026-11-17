#pragma once

namespace VaultHeist {

/**
 * @brief Event vocabulary (Project.pdf §4.3, PLAN.md §6.3).
 *
 * Course rules: no enum / enum class, so event types are plain
 * `static const int` constants (same pattern as SCREEN_* in main.cpp).
 */
static const int EVENT_OPERATION_STARTED   = 0;
static const int EVENT_VISIT               = 1;
static const int EVENT_COMPARE             = 2;
static const int EVENT_INSERT              = 3;
static const int EVENT_UNLINK              = 4;
static const int EVENT_SWAP_VALUE          = 5;
static const int EVENT_ROTATE              = 6;
static const int EVENT_UPDATE_HEIGHT       = 7;
static const int EVENT_UPDATE_SIZE         = 8;
static const int EVENT_BALANCE_ALERT       = 9;
static const int EVENT_HIGHLIGHT_SUBTREE   = 10;
static const int EVENT_HEAP_BUILD          = 11;
static const int EVENT_HEAP_SWAP           = 12;
static const int EVENT_HEAPIFY_START       = 13;
static const int EVENT_HEAPIFY_DONE        = 14;
static const int EVENT_EXTRACT_MAX         = 15;
static const int EVENT_MARK_SORTED         = 16;
static const int EVENT_HEAP_SIZE_CHANGE    = 17;
static const int EVENT_HEAP_SORT_COMPLETE  = 18;
static const int EVENT_OPERATION_COMPLETED = 19;
static const int EVENT_OPERATION_FAILED    = 20;

// AVL rotation cases (Event::rotationCase). -1 = not a rotation event.
static const int ROTATION_NONE = -1;
static const int ROTATION_LL   = 0;
static const int ROTATION_RR   = 1;
static const int ROTATION_LR   = 2;
static const int ROTATION_RL   = 3;

/**
 * @brief One flat event record (PLAN.md §6.1 / §6.3).
 *
 * Every field is an int; unused fields stay at -1.
 *  - nodeId / otherNodeId : TreeNode::id for BinaryTree/BST; the node's key
 *                           for AVLTree (AVLNode has no id, keys are unique).
 *  - oldValue / newValue  : before/after value (height, size, key, heap value…)
 *  - indexA / indexB      : array indices (MaxHeap / heap sort)
 *  - rotationCase         : ROTATION_* constant for EVENT_ROTATE
 *  - label                : static string literal, human-readable (debug/GUI log)
 */
struct Event {
    int type;
    int operationId;
    int sequence;
    int nodeId;
    int otherNodeId;
    int oldValue;
    int newValue;
    int indexA;
    int indexB;
    int rotationCase;
    const char* label;

    Event()
        : type(-1), operationId(-1), sequence(-1), nodeId(-1), otherNodeId(-1),
          oldValue(-1), newValue(-1), indexA(-1), indexB(-1),
          rotationCase(ROTATION_NONE), label("") {}

    Event(int t, const char* lbl)
        : type(t), operationId(-1), sequence(-1), nodeId(-1), otherNodeId(-1),
          oldValue(-1), newValue(-1), indexA(-1), indexB(-1),
          rotationCase(ROTATION_NONE), label(lbl) {}
};

/**
 * @brief Ordered batch of events produced by ONE operation.
 *
 * Manual dynamic array (data/capacity/count + resize), same pattern as
 * MaxHeap — no std::vector allowed. Header-only so every structure can use
 * it without extra link dependencies.
 */
class EventBatch {
private:
    Event* data;
    int capacity;
    int count;
    int operationId;

    void resize(int newCapacity) {
        Event* newData = new Event[newCapacity];
        for (int i = 0; i < count; ++i) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

    void copyFrom(const EventBatch& other) {
        capacity = (other.capacity > 0) ? other.capacity : 8;
        count = other.count;
        operationId = other.operationId;
        data = new Event[capacity];
        for (int i = 0; i < count; ++i) {
            data[i] = other.data[i];
        }
    }

public:
    EventBatch() : data(new Event[8]), capacity(8), count(0), operationId(-1) {}

    ~EventBatch() {
        delete[] data;
        data = nullptr;
    }

    EventBatch(const EventBatch& other) : data(nullptr), capacity(0), count(0), operationId(-1) {
        copyFrom(other);
    }

    EventBatch& operator=(const EventBatch& other) {
        if (this != &other) {
            delete[] data;
            copyFrom(other);
        }
        return *this;
    }

    // Appends a copy of `e`, stamping its sequence number and operation id.
    void append(const Event& e) {
        if (count >= capacity) {
            resize(capacity * 2);
        }
        data[count] = e;
        data[count].sequence = count;
        data[count].operationId = operationId;
        count++;
    }

    int getCount() const { return count; }
    bool isEmpty() const { return count == 0; }

    // Returns a copy of the i-th event; out of range -> default Event (type -1).
    Event getEventAt(int i) const {
        if (i < 0 || i >= count) return Event();
        return data[i];
    }

    // Number of events of a given EVENT_* type (convenience for tests / GUI).
    int countOfType(int type) const {
        int c = 0;
        for (int i = 0; i < count; ++i) {
            if (data[i].type == type) c++;
        }
        return c;
    }

    int getOperationId() const { return operationId; }

    // Sets the operation id on the batch and every event already in it.
    void setOperationId(int id) {
        operationId = id;
        for (int i = 0; i < count; ++i) {
            data[i].operationId = id;
        }
    }

    void clear() { count = 0; }
};

} // namespace VaultHeist
