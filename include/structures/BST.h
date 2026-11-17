#pragma once

#include "TreeNode.h"
#include "events/Event.h"
#include <iostream>

namespace VaultHeist {

/**
 * @brief Binary Search Tree (Act 2) — ordering invariant enforced on insert.
 *
 * Uses the shared TreeNode (value doubles as key).
 * Rejects duplicate keys.
 * Height convention: leaf = 1, nullptr = 0.
 * No STL containers, no enum, no global variables.
 */
class BST {
private:
    TreeNode* root;
    int nextId;    // monotonic node-id counter (class-scoped, not global)
    int nodeCount;

    // --- Recursive helpers ---
    TreeNode* insertHelper(TreeNode* node, TreeNode* parent, int key, bool& inserted,
                           EventBatch& events);
    TreeNode* deleteHelper(TreeNode* node, int key, bool& deleted, EventBatch& events);
    TreeNode* searchHelper(TreeNode* node, int key) const;
    TreeNode* searchHelper(TreeNode* node, int key, EventBatch& events) const;

    TreeNode* findMinNode(TreeNode* node) const;
    TreeNode* findMaxNode(TreeNode* node) const;

    int heightHelper(TreeNode* node) const;

    // Inorder helper for rangeQuery
    void rangeQueryHelper(TreeNode* node, int lo, int hi, int* out, int& idx,
                          EventBatch& events) const;

    // Inorder counter for kthSmallest
    void kthHelper(TreeNode* node, int& count, int k, int& result) const;

    // Validator
    bool validateBST(TreeNode* node, const TreeNode* minNode, const TreeNode* maxNode) const;

    // Cleanup
    void destroyRecursive(TreeNode* node);

    // Count nodes in subtree (for rangeQuery array sizing)
    int countNodes(TreeNode* node) const;

    // Debug
    void inorderPrint(TreeNode* node) const;

public:
    BST();
    ~BST();

    // --- Core Operations (§5.3) ---
    // Event overloads (PLAN.md §6.4):
    //   search   -> EVENT_VISIT + EVENT_COMPARE per node checked
    //   insert   -> VISIT/COMPARE per node, EVENT_INSERT only on success
    //   deleteKey-> VISIT/COMPARE, EVENT_UNLINK (leaf/one child) or
    //               EVENT_SWAP_VALUE + EVENT_UNLINK (two children, successor)
    // Every batch is bracketed by OPERATION_STARTED and
    // OPERATION_COMPLETED (success) / OPERATION_FAILED (not found / duplicate).
    bool insert(int key);
    bool deleteKey(int key);
    bool search(int key) const;
    bool insert(int key, EventBatch& events);
    bool deleteKey(int key, EventBatch& events);
    bool search(int key, EventBatch& events) const;

    // --- Min / Max ---
    int findMin() const;
    int findMax() const;

    // --- Successor / Predecessor (via parent pointers) ---
    int successor(int key) const;
    int predecessor(int key) const;

    // --- Range Query ---
    // Returns a newly-allocated int* of keys in [lo, hi] in sorted order.
    // `count` set to number of results. Caller must delete[].
    int* rangeQuery(int lo, int hi, int& count) const;
    // Event overload: EVENT_VISIT per node examined, EVENT_HIGHLIGHT_SUBTREE
    // (nodeId = subtree root) for every subtree pruned without being walked.
    int* rangeQuery(int lo, int hi, int& count, EventBatch& events) const;

    // --- k-th Smallest (O(n) inorder walk) ---
    int kthSmallest(int k) const;

    // --- BST-specific LCA (ordering-based walk) ---
    int lcaBST(int a, int b) const;

    // --- Validator (§5.6) ---
    bool isValidBST() const;

    // --- Accessors ---
    TreeNode* getRoot() const { return root; }
    int getHeight() const;
    int getSize() const { return nodeCount; }
    bool isEmpty() const { return root == nullptr; }

    // --- Debug ---
    void printInorder() const;
};

} // namespace VaultHeist
