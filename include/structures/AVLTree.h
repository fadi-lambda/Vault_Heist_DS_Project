#pragma once

#include <string>
#include <iostream>
#include "events/Event.h"

namespace VaultHeist {

/**
 * @brief Node for AVL Tree (Act 3)
 * - height: leaf = 1, nullptr = 0
 * - size: total number of nodes in this subtree (for O(log n) order statistics)
 * - parent: pointer to parent node (simplifies rotations and traversals)
 */
struct AVLNode {
    int key;
    int height;
    int size;
    AVLNode* left;
    AVLNode* right;
    AVLNode* parent;

    AVLNode(int k)
        : key(k), height(1), size(1), left(nullptr), right(nullptr), parent(nullptr) {}
};

class AVLTree {
private:
    AVLNode* root;

    // Benchmark-only instrumentation counters (reset before each measured run)
    static long long s_comparisons;
    static long long s_rotations;

    // Helper utilities for height, size, and balance factor
    int getNodeHeight(AVLNode* node) const { return node ? node->height : 0; }
    int getNodeSize(AVLNode* node) const { return node ? node->size : 0; }
    int getBalanceFactor(AVLNode* node) const;

    // Internal pointer-based event helpers (nullptr skips event recording)
    void updateHeightAndSizeImpl(AVLNode* node, EventBatch* events);
    AVLNode* rotateRightImpl(AVLNode* y, EventBatch* events, int rotationCase);
    AVLNode* rotateLeftImpl(AVLNode* x, EventBatch* events, int rotationCase);
    AVLNode* rebalanceImpl(AVLNode* node, EventBatch* events);

    // Recursive helpers
    AVLNode* insertHelper(AVLNode* node, AVLNode* parent, int key, bool& inserted, EventBatch* events);
    AVLNode* deleteHelper(AVLNode* node, int key, bool& deleted, EventBatch* events);
    bool insertImpl(int key, EventBatch* events);
    bool deleteImpl(int key, EventBatch* events);
    AVLNode* findMinNode(AVLNode* node) const;
    AVLNode* findMaxNode(AVLNode* node) const;
    AVLNode* searchHelper(AVLNode* node, int key) const;
    AVLNode* searchHelper(AVLNode* node, int key, EventBatch& events) const;

    // Order statistics helpers (Boss Level 3.5)
    int kthHelper(AVLNode* node, int k) const;
    int rankHelper(AVLNode* node, int key) const;

    // Validation & cleanup helpers
    void destroyRecursive(AVLNode* node);
    bool validateInvariants(AVLNode* node, int minVal, int maxVal) const;
    void inorderPrint(AVLNode* node) const;

public:
    AVLTree();
    ~AVLTree();

    // Benchmark-only Counter API
    static void resetCounters();
    static long long getComparisons();
    static long long getRotations();

    // Direct manual rotation & subtree metadata entry points (Act 3 interactive puzzles)
    void updateHeightAndSize(AVLNode* node, EventBatch& events);
    AVLNode* rotateRight(AVLNode* y, EventBatch& events, int rotationCase); // LL Case (Single Right Rotation)
    AVLNode* rotateLeft(AVLNode* x, EventBatch& events, int rotationCase);  // RR Case (Single Left Rotation)
    AVLNode* rebalance(AVLNode* node, EventBatch& events);
    void setRoot(AVLNode* newRoot) { root = newRoot; if (root) root->parent = nullptr; }
    void clear() { destroyRecursive(root); root = nullptr; }

    // Core Operations (Section 5.4)
    bool insert(int key);
    bool deleteKey(int key);
    bool search(int key) const;

    // Event-emitting overloads (PLAN.md §6.4/§6.5). Same behaviour as above,
    // additionally append to `events`. nodeId/otherNodeId carry node KEYS
    // (AVLNode has no id field; keys are unique). Batches are bracketed by
    // OPERATION_STARTED and OPERATION_COMPLETED / OPERATION_FAILED.
    bool insert(int key, EventBatch& events);
    bool deleteKey(int key, EventBatch& events);
    bool search(int key, EventBatch& events) const;

    // Min and Max
    int findMin() const;
    int findMax() const;

    // Balance & Properties
    int balanceFactor(AVLNode* node) const { return getBalanceFactor(node); }
    int getRootBalanceFactor() const { return getBalanceFactor(root); }
    int getHeight() const { return getNodeHeight(root); }
    int getSize() const { return getNodeSize(root); }
    bool isEmpty() const { return root == nullptr; }
    AVLNode* getRoot() const { return root; }

    // Order Statistics - O(log n) (Section 5.4 & Boss Level 3.5)
    int kth(int k) const;                   // k-th smallest element (1-based index)
    int rank(int key) const;                 // count of keys <= key
    int countInRange(int lo, int hi) const;  // rank(hi) - rank(lo - 1)

    // Invariant Validator (Section 5.6)
    bool isBalanced() const;
    bool isValidAVL() const;

    // Display
    void printInorder() const;
};

} // namespace VaultHeist
