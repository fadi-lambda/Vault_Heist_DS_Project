#pragma once

#include <string>
#include <iostream>

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

    // Helper utilities for height, size, and balance factor
    int getNodeHeight(AVLNode* node) const { return node ? node->height : 0; }
    int getNodeSize(AVLNode* node) const { return node ? node->size : 0; }
    int getBalanceFactor(AVLNode* node) const;

    void updateHeightAndSize(AVLNode* node);

    // Rotations (LL, RR, LR, RL)
    AVLNode* rotateRight(AVLNode* y); // LL Case (Single Right Rotation)
    AVLNode* rotateLeft(AVLNode* x);  // RR Case (Single Left Rotation)
    AVLNode* rebalance(AVLNode* node);

    // Recursive helpers
    AVLNode* insertHelper(AVLNode* node, AVLNode* parent, int key, bool& inserted);
    AVLNode* deleteHelper(AVLNode* node, int key, bool& deleted);
    AVLNode* findMinNode(AVLNode* node) const;
    AVLNode* findMaxNode(AVLNode* node) const;
    AVLNode* searchHelper(AVLNode* node, int key) const;

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

    // Core Operations (Section 5.4)
    bool insert(int key);
    bool deleteKey(int key);
    bool search(int key) const;

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
