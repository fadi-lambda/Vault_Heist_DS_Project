#pragma once

#include "structures/BinaryTree.h"
#include "structures/BST.h"
#include "structures/AVLTree.h"
#include "structures/MaxHeap.h"

namespace VaultHeist {

/**
 * @brief Unified structural invariant validator (Spec Section 5.6).
 *
 * Pure C++20, no STL containers, no enums, no global variables, zero SFML.
 */
class Validator {
private:
    static bool checkBinaryTreeNode(const TreeNode* node, const TreeNode* expectedParent,
                                    int& visited, int maxNodes) {
        if (!node) return true;
        if (node->parent != expectedParent) return false;
        visited++;
        if (visited > maxNodes) return false; // cycle or shared-node guard
        return checkBinaryTreeNode(node->left, node, visited, maxNodes) &&
               checkBinaryTreeNode(node->right, node, visited, maxNodes);
    }

public:
    static bool validateBinaryTree(const BinaryTree& tree) {
        int visited = 0;
        if (!checkBinaryTreeNode(tree.getRoot(), nullptr, visited, tree.getSize())) {
            return false;
        }
        return visited == tree.getSize();
    }

    static bool validateBST(const BST& bst) {
        return bst.isValidBST();
    }

    static bool validateAVL(const AVLTree& avl) {
        return avl.isValidAVL();
    }

    static bool validateMaxHeap(const MaxHeap& heap) {
        return heap.isMaxHeap();
    }

    static bool validateHeapSortPartition(const int* activeHeap, int activeSize,
                                          const int* sortedArr, int sortedCount) {
        for (int i = 0; i < activeSize; ++i) {
            int l = 2 * i + 1;
            int r = 2 * i + 2;
            if (l < activeSize && activeHeap[i] < activeHeap[l]) return false;
            if (r < activeSize && activeHeap[i] < activeHeap[r]) return false;
        }
        for (int s = 1; s < sortedCount; ++s) {
            if (sortedArr[s - 1] > sortedArr[s]) return false;
        }
        if (activeSize > 0 && sortedCount > 0) {
            int maxActive = activeHeap[0];
            for (int s = 0; s < sortedCount; ++s) {
                if (maxActive > sortedArr[s]) return false;
            }
        }
        return true;
    }
};

} // namespace VaultHeist
