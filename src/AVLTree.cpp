#include "AVLTree.h"
#include <iostream>
#include <climits>

namespace VaultHeist {

// Constructor
AVLTree::AVLTree() : root(nullptr) {}

// Destructor: recursively frees all nodes to prevent memory leaks
AVLTree::~AVLTree() {
    destroyRecursive(root);
    root = nullptr;
}

void AVLTree::destroyRecursive(AVLNode* node) {
    if (node) {
        destroyRecursive(node->left);
        destroyRecursive(node->right);
        delete node;
    }
}

// Balance factor = height(left) - height(right)
int AVLTree::getBalanceFactor(AVLNode* node) const {
    if (!node) return 0;
    return getNodeHeight(node->left) - getNodeHeight(node->right);
}

// Updates height and subtreeSize based on children
void AVLTree::updateHeightAndSize(AVLNode* node) {
    if (!node) return;

    int leftH = getNodeHeight(node->left);
    int rightH = getNodeHeight(node->right);
    node->height = 1 + (leftH > rightH ? leftH : rightH);

    int leftS = getNodeSize(node->left);
    int rightS = getNodeSize(node->right);
    node->size = 1 + leftS + rightS;
}

// LL Case: Single Right Rotation
//
//        y                              x
//       / \                            / \
//      x   T3     rotateRight(y)     T1   y
//     / \        -------------->         / \
//    T1  T2                             T2  T3
AVLNode* AVLTree::rotateRight(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update parent pointers
    x->parent = y->parent;
    y->parent = x;
    if (T2) {
        T2->parent = y;
    }

    // Update heights and subtree sizes (child y first, then new parent x)
    updateHeightAndSize(y);
    updateHeightAndSize(x);

    return x; // New root of this subtree
}

// RR Case: Single Left Rotation
//
//       x                              y
//      / \                            / \
//     T1  y       rotateLeft(x)      x   T3
//        / \     ------------->     / \
//       T2  T3                     T1  T2
AVLNode* AVLTree::rotateLeft(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    // Perform rotation
    y->left = x;
    x->right = T2;

    // Update parent pointers
    y->parent = x->parent;
    x->parent = y;
    if (T2) {
        T2->parent = x;
    }

    // Update heights and subtree sizes (child x first, then new parent y)
    updateHeightAndSize(x);
    updateHeightAndSize(y);

    return y; // New root of this subtree
}

// Rebalances a node by checking its balance factor and selecting LL, RR, LR, or RL
AVLNode* AVLTree::rebalance(AVLNode* node) {
    if (!node) return nullptr;

    updateHeightAndSize(node);
    int bf = getBalanceFactor(node);

    // Left Heavy (BF > 1)
    if (bf > 1) {
        // LR Case: Left child is right-heavy -> rotate left on left child first
        if (getBalanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
        }
        // LL Case: Single right rotation
        return rotateRight(node);
    }

    // Right Heavy (BF < -1)
    if (bf < -1) {
        // RL Case: Right child is left-heavy -> rotate right on right child first
        if (getBalanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
        }
        // RR Case: Single left rotation
        return rotateLeft(node);
    }

    return node; // Node is already balanced (BF is -1, 0, or 1)
}

// Insert Helper: standard BST insert with duplicate rejection, then rebalances up
AVLNode* AVLTree::insertHelper(AVLNode* node, AVLNode* parent, int key, bool& inserted) {
    if (!node) {
        inserted = true;
        AVLNode* newNode = new AVLNode(key);
        newNode->parent = parent;
        return newNode;
    }

    if (key < node->key) {
        node->left = insertHelper(node->left, node, key, inserted);
    } else if (key > node->key) {
        node->right = insertHelper(node->right, node, key, inserted);
    } else {
        // Duplicate key: rejected according to Section 5.3 & 5.4
        std::cout << "Warning: Duplicate key " << key << " rejected in Vault!\n";
        inserted = false;
        return node;
    }

    return rebalance(node);
}

bool AVLTree::insert(int key) {
    bool inserted = false;
    root = insertHelper(root, nullptr, key, inserted);
    if (root) {
        root->parent = nullptr;
    }
    return inserted;
}

// Find minimum node in a subtree (leftmost node)
AVLNode* AVLTree::findMinNode(AVLNode* node) const {
    AVLNode* curr = node;
    while (curr && curr->left) {
        curr = curr->left;
    }
    return curr;
}

// Find maximum node in a subtree (rightmost node)
AVLNode* AVLTree::findMaxNode(AVLNode* node) const {
    AVLNode* curr = node;
    while (curr && curr->right) {
        curr = curr->right;
    }
    return curr;
}

int AVLTree::findMin() const {
    AVLNode* minNode = findMinNode(root);
    return minNode ? minNode->key : -1;
}

int AVLTree::findMax() const {
    AVLNode* maxNode = findMaxNode(root);
    return maxNode ? maxNode->key : -1;
}

// Delete Helper: Handles all 3 BST delete cases, then rebalances returning up
// Case 1: Leaf node (delete node)
// Case 2: One child (bypass node)
// Case 3: Two children (replace with inorder successor, delete successor)
AVLNode* AVLTree::deleteHelper(AVLNode* node, int key, bool& deleted) {
    if (!node) {
        deleted = false;
        return nullptr;
    }

    if (key < node->key) {
        node->left = deleteHelper(node->left, key, deleted);
    } else if (key > node->key) {
        node->right = deleteHelper(node->right, key, deleted);
    } else {
        // Key found!
        deleted = true;

        // Case 1: Leaf node (no children)
        if (!node->left && !node->right) {
            delete node;
            return nullptr;
        }

        // Case 2: One child
        if (!node->left || !node->right) {
            AVLNode* child = node->left ? node->left : node->right;
            child->parent = node->parent;
            delete node;
            return child;
        }

        // Case 3: Two children
        // Find inorder successor (smallest node in right subtree)
        AVLNode* successor = findMinNode(node->right);
        node->key = successor->key;
        // Delete successor from right subtree
        node->right = deleteHelper(node->right, successor->key, deleted);
    }

    return rebalance(node);
}

bool AVLTree::deleteKey(int key) {
    bool deleted = false;
    root = deleteHelper(root, key, deleted);
    if (root) {
        root->parent = nullptr;
    }
    return deleted;
}

// Search
AVLNode* AVLTree::searchHelper(AVLNode* node, int key) const {
    if (!node || node->key == key) return node;
    if (key < node->key) return searchHelper(node->left, key);
    return searchHelper(node->right, key);
}

bool AVLTree::search(int key) const {
    return searchHelper(root, key) != nullptr;
}

// Order Statistics in O(log n) using subtreeSize (Boss Level 3.5)
// k-th smallest element (1-based: k=1 returns minimum)
int AVLTree::kthHelper(AVLNode* node, int k) const {
    if (!node) return -1;

    int leftSize = getNodeSize(node->left);

    if (k == leftSize + 1) {
        return node->key; // Found k-th element
    } else if (k <= leftSize) {
        return kthHelper(node->left, k); // Look in left subtree
    } else {
        return kthHelper(node->right, k - leftSize - 1); // Look in right subtree
    }
}

int AVLTree::kth(int k) const {
    if (k < 1 || k > getSize()) {
        std::cerr << "Error: k is out of range [1, " << getSize() << "]\n";
        return -1;
    }
    return kthHelper(root, k);
}

// Rank: count of keys <= key in O(log n)
int AVLTree::rankHelper(AVLNode* node, int key) const {
    if (!node) return 0;

    int leftSize = getNodeSize(node->left);

    if (key == node->key) {
        return leftSize + 1;
    } else if (key < node->key) {
        return rankHelper(node->left, key);
    } else {
        return leftSize + 1 + rankHelper(node->right, key);
    }
}

int AVLTree::rank(int key) const {
    return rankHelper(root, key);
}

// Count keys in range [lo, hi]: rank(hi) - rank(lo - 1)
int AVLTree::countInRange(int lo, int hi) const {
    if (lo > hi) return 0;
    return rank(hi) - rank(lo - 1);
}

// Invariant Validation (Section 5.6)
bool AVLTree::validateInvariants(AVLNode* node, int minVal, int maxVal) const {
    if (!node) return true;

    // 1. Check BST ordering
    if (node->key <= minVal || node->key >= maxVal) return false;

    // 2. Check balance factor constraint: |BF| <= 1
    int bf = getBalanceFactor(node);
    if (bf < -1 || bf > 1) return false;

    // 3. Check height invariant: stored height == 1 + max(leftH, rightH)
    int leftH = getNodeHeight(node->left);
    int rightH = getNodeHeight(node->right);
    int expectedH = 1 + (leftH > rightH ? leftH : rightH);
    if (node->height != expectedH) return false;

    // 4. Check size invariant: stored size == 1 + leftS + rightS
    int leftS = getNodeSize(node->left);
    int rightS = getNodeSize(node->right);
    if (node->size != 1 + leftS + rightS) return false;

    // 5. Check parent pointer consistency
    if (node->left && node->left->parent != node) return false;
    if (node->right && node->right->parent != node) return false;

    return validateInvariants(node->left, minVal, node->key) &&
           validateInvariants(node->right, node->key, maxVal);
}

bool AVLTree::isBalanced() const {
    return isValidAVL();
}

bool AVLTree::isValidAVL() const {
    return validateInvariants(root, INT_MIN, INT_MAX);
}

// Inorder traversal display
void AVLTree::inorderPrint(AVLNode* node) const {
    if (node) {
        inorderPrint(node->left);
        std::cout << node->key << " ";
        inorderPrint(node->right);
    }
}

void AVLTree::printInorder() const {
    std::cout << "AVL Inorder: ";
    inorderPrint(root);
    std::cout << "\n";
}

} // namespace VaultHeist
