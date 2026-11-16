#pragma once

namespace VaultHeist {

/**
 * @brief Shared node type for General Binary Tree (Act 1) and BST (Act 2).
 *
 * - id:     monotonically increasing identifier so GUI code can track nodes
 *           across structural changes. Assigned automatically by the constructor
 *           from a static counter (class-scoped, not a global variable).
 * - value:  the payload / key stored in this node.
 * - left, right, parent: raw pointers (no STL containers).
 *
 * Height convention (consistent with the rest of the codebase):
 *   height(leaf) = 1, height(nullptr) = 0
 */
struct TreeNode {
    int id;
    int value;
    TreeNode* left;
    TreeNode* right;
    TreeNode* parent;

    TreeNode(int id, int value)
        : id(id), value(value), left(nullptr), right(nullptr), parent(nullptr) {}
};

} // namespace VaultHeist
