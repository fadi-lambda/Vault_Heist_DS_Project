#pragma once

#include "TreeNode.h"
#include "events/Event.h"
#include <iostream>

namespace VaultHeist {

/**
 * @brief General Binary Tree (Act 1) — no ordering invariant.
 *
 * Provides traversals (recursive + iterative), reconstruction from
 * traversal pairs, structural queries (height, diameter, LCA, path),
 * and structural transforms (mirror, comparison, symmetry check).
 *
 * Height convention: leaf = 1, nullptr = 0.
 * No STL containers — traversals return caller-owned int* arrays.
 * No enum — plain int constants where needed.
 */
class BinaryTree {
private:
    TreeNode* root;
    int nextId;   // monotonic id counter (class-scoped, not global)
    int nodeCount;

    // --- Recursive traversal helpers (fill into caller's array) ---
    void preorderHelper(TreeNode* node, int* out, int& idx, EventBatch* events) const;
    void inorderHelper(TreeNode* node, int* out, int& idx, EventBatch* events) const;
    void postorderHelper(TreeNode* node, int* out, int& idx, EventBatch* events) const;

    // --- Reconstruction helpers ---
    // Returns index of `value` in arr[lo..hi], or -1
    static int findIndex(const int* arr, int lo, int hi, int value);

    TreeNode* buildFromPreIn(const int* pre, int preStart, int preEnd,
                             const int* in, int inStart, int inEnd,
                             TreeNode* parent, EventBatch* events);

    TreeNode* buildFromPostIn(const int* post, int postStart, int postEnd,
                              const int* in, int inStart, int inEnd,
                              TreeNode* parent, EventBatch* events);

    // --- Structural helpers ---
    int heightHelper(TreeNode* node) const;
    int diameterHelper(TreeNode* node, int& maxDiam) const;

    TreeNode* lcaHelper(TreeNode* node, int a, int b) const;
    bool findNode(TreeNode* node, int val) const;
    bool pathToHelper(TreeNode* node, int target, int* path, int& len) const;

    void mirrorHelper(TreeNode* node, EventBatch* events);

    bool isSameTreeHelper(TreeNode* a, TreeNode* b) const;
    bool isSymmetricHelper(TreeNode* left, TreeNode* right) const;

    // --- Cleanup ---
    void destroyRecursive(TreeNode* node);

    // --- Debug ---
    void printInorderHelper(TreeNode* node) const;

public:
    BinaryTree();
    ~BinaryTree();
    BinaryTree(const BinaryTree&) = delete;
    BinaryTree& operator=(const BinaryTree&) = delete;

    void clear();

    // --- Manual construction (for testing) ---
    // Insert `value` as a new node; returns pointer to the created node.
    // Attaches as left or right child of `parent`; if parent == nullptr, sets root.
    // `asLeft` controls which child pointer to use.
    TreeNode* addNode(int value, TreeNode* parent, bool asLeft);

    // --- Traversals (recursive) ---
    // Each returns a newly-allocated int* of node values; caller must delete[].
    // `count` is set to the number of elements written.
    // Event overloads append one EVENT_VISIT per node, in actual visit order.
    int* preorderRecursive(int& count) const;
    int* inorderRecursive(int& count) const;
    int* postorderRecursive(int& count) const;
    int* preorderRecursive(int& count, EventBatch& events) const;
    int* inorderRecursive(int& count, EventBatch& events) const;
    int* postorderRecursive(int& count, EventBatch& events) const;

    // --- Traversals (iterative, explicit stack / queue) ---
    int* preorderIterative(int& count) const;
    int* inorderIterative(int& count) const;
    int* postorderIterative(int& count) const;
    int* levelOrder(int& count) const;   // iterative BFS with manual array-queue
    int* preorderIterative(int& count, EventBatch& events) const;
    int* inorderIterative(int& count, EventBatch& events) const;
    int* postorderIterative(int& count, EventBatch& events) const;
    int* levelOrder(int& count, EventBatch& events) const;

    // --- Reconstruction ---
    // Replaces the current tree with a tree reconstructed from traversal pairs.
    // Event overloads append one EVENT_INSERT per created node (creation order).
    void buildFromPreorderInorder(const int* preorder, const int* inorder, int n);
    void buildFromPostorderInorder(const int* postorder, const int* inorder, int n);
    void buildFromPreorderInorder(const int* preorder, const int* inorder, int n, EventBatch& events);
    void buildFromPostorderInorder(const int* postorder, const int* inorder, int n, EventBatch& events);

    // --- Structural queries ---
    int height() const;
    int diameter() const;

    // General-tree LCA (no ordering assumption — searches both subtrees).
    // Returns the value of the LCA node, or -1 if either value is absent.
    int lca(int a, int b) const;

    // Root-to-node path. Returns new int[] of values from root to target.
    // `pathLen` set to length. Returns nullptr if target not found.
    int* pathTo(int target, int& pathLen) const;

    // --- Transforms ---
    // Event overload appends one EVENT_VISIT per node whose children are flipped.
    void mirror();
    void mirror(EventBatch& events);

    // --- Comparison ---
    bool isSameTree(const BinaryTree& other) const;
    bool isSymmetric() const;

    // --- Accessors ---
    TreeNode* getRoot() const { return root; }
    int getSize() const { return nodeCount; }
    bool isEmpty() const { return root == nullptr; }

    // --- Debug ---
    void printInorder() const;
};

} // namespace VaultHeist
