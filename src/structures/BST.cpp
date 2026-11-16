// =============================================================================
// BST.cpp — Binary Search Tree (Act 2)
// Ordering invariant: left < node < right. Rejects duplicates.
// Height convention: leaf = 1, nullptr = 0.
// No STL containers, no enum, no global variables, no <algorithm>.
// =============================================================================

#include "structures/BST.h"

namespace VaultHeist {

// ========================== Helpers (file-local, not global vars) =============

static inline int maxInt(int a, int b) { return (a > b) ? a : b; }

// ========================== Construction / Destruction ========================

BST::BST() : root(nullptr), nextId(1), nodeCount(0) {}

BST::~BST() {
    destroyRecursive(root);
}

void BST::destroyRecursive(TreeNode* node) {
    if (!node) return;
    destroyRecursive(node->left);
    destroyRecursive(node->right);
    delete node;
}

// ========================== Insert ===========================================

TreeNode* BST::insertHelper(TreeNode* node, TreeNode* parent, int key, bool& inserted) {
    if (!node) {
        TreeNode* n = new TreeNode(nextId++, key);
        n->parent = parent;
        inserted = true;
        nodeCount++;
        return n;
    }
    if (key < node->value) {
        node->left = insertHelper(node->left, node, key, inserted);
    } else if (key > node->value) {
        node->right = insertHelper(node->right, node, key, inserted);
    }
    // key == node->value → duplicate, inserted stays false
    return node;
}

bool BST::insert(int key) {
    bool inserted = false;
    root = insertHelper(root, nullptr, key, inserted);
    if (!inserted) {
        std::cout << "Duplicate key " << key << " rejected." << std::endl;
    }
    return inserted;
}

// ========================== Delete ===========================================

TreeNode* BST::findMinNode(TreeNode* node) const {
    if (!node) return nullptr;
    while (node->left) node = node->left;
    return node;
}

TreeNode* BST::findMaxNode(TreeNode* node) const {
    if (!node) return nullptr;
    while (node->right) node = node->right;
    return node;
}

TreeNode* BST::deleteHelper(TreeNode* node, int key, bool& deleted) {
    if (!node) return nullptr;

    if (key < node->value) {
        node->left = deleteHelper(node->left, key, deleted);
        if (node->left) node->left->parent = node;
    } else if (key > node->value) {
        node->right = deleteHelper(node->right, key, deleted);
        if (node->right) node->right->parent = node;
    } else {
        // Found the node to delete
        deleted = true;

        // Case 1: Leaf node
        if (!node->left && !node->right) {
            delete node;
            nodeCount--;
            return nullptr;
        }
        // Case 2a: Only right child
        if (!node->left) {
            TreeNode* rightChild = node->right;
            rightChild->parent = node->parent;
            delete node;
            nodeCount--;
            return rightChild;
        }
        // Case 2b: Only left child
        if (!node->right) {
            TreeNode* leftChild = node->left;
            leftChild->parent = node->parent;
            delete node;
            nodeCount--;
            return leftChild;
        }
        // Case 3: Two children — find inorder successor (min of right subtree),
        // copy its value, delete the successor node.
        TreeNode* succ = findMinNode(node->right);
        node->value = succ->value;
        node->id = succ->id;  // preserve the successor's id for tracking
        node->right = deleteHelper(node->right, succ->value, deleted);
        if (node->right) node->right->parent = node;
        // `deleted` was already set true above, the recursive call may set it
        // again but that's fine (idempotent).
        deleted = true;
    }
    return node;
}

bool BST::deleteKey(int key) {
    bool deleted = false;
    root = deleteHelper(root, key, deleted);
    if (root) root->parent = nullptr;
    return deleted;
}

// ========================== Search ===========================================

TreeNode* BST::searchHelper(TreeNode* node, int key) const {
    if (!node) return nullptr;
    if (key == node->value) return node;
    if (key < node->value) return searchHelper(node->left, key);
    return searchHelper(node->right, key);
}

bool BST::search(int key) const {
    return searchHelper(root, key) != nullptr;
}

// ========================== Min / Max ========================================

int BST::findMin() const {
    TreeNode* node = findMinNode(root);
    if (!node) {
        std::cout << "findMin called on empty BST." << std::endl;
        return -1;
    }
    return node->value;
}

int BST::findMax() const {
    TreeNode* node = findMaxNode(root);
    if (!node) {
        std::cout << "findMax called on empty BST." << std::endl;
        return -1;
    }
    return node->value;
}

// ========================== Successor / Predecessor ==========================
// Using parent pointers (as specified): O(h).

int BST::successor(int key) const {
    TreeNode* node = searchHelper(root, key);
    if (!node) return -1;

    // Case 1: right subtree exists → successor is min of right subtree
    if (node->right) {
        return findMinNode(node->right)->value;
    }
    // Case 2: go up until we find an ancestor where node is in left subtree
    TreeNode* p = node->parent;
    while (p && node == p->right) {
        node = p;
        p = p->parent;
    }
    return p ? p->value : -1;
}

int BST::predecessor(int key) const {
    TreeNode* node = searchHelper(root, key);
    if (!node) return -1;

    // Case 1: left subtree exists → predecessor is max of left subtree
    if (node->left) {
        return findMaxNode(node->left)->value;
    }
    // Case 2: go up until we find an ancestor where node is in right subtree
    TreeNode* p = node->parent;
    while (p && node == p->left) {
        node = p;
        p = p->parent;
    }
    return p ? p->value : -1;
}

// ========================== Range Query ======================================
// Inorder walk with pruning — skips subtrees entirely outside [lo, hi].

void BST::rangeQueryHelper(TreeNode* node, int lo, int hi, int* out, int& idx) const {
    if (!node) return;
    // Prune: if node->value > lo, left subtree may have keys in range
    if (node->value > lo)
        rangeQueryHelper(node->left, lo, hi, out, idx);
    // Include this node if in range
    if (node->value >= lo && node->value <= hi)
        out[idx++] = node->value;
    // Prune: if node->value < hi, right subtree may have keys in range
    if (node->value < hi)
        rangeQueryHelper(node->right, lo, hi, out, idx);
}

int BST::countNodes(TreeNode* node) const {
    if (!node) return 0;
    return 1 + countNodes(node->left) + countNodes(node->right);
}

int* BST::rangeQuery(int lo, int hi, int& count) const {
    count = 0;
    if (!root) return nullptr;
    // Allocate worst-case (all nodes in range); caller deletes.
    int maxPossible = nodeCount;
    int* result = new int[maxPossible];
    rangeQueryHelper(root, lo, hi, result, count);
    if (count == 0) {
        delete[] result;
        return nullptr;
    }
    return result;
}

// ========================== k-th Smallest ====================================
// O(n) inorder walk — acceptable per spec (AVL's O(log n) is Adnan's).

void BST::kthHelper(TreeNode* node, int& count, int k, int& result) const {
    if (!node || count >= k) return;
    kthHelper(node->left, count, k, result);
    count++;
    if (count == k) {
        result = node->value;
        return;
    }
    kthHelper(node->right, count, k, result);
}

int BST::kthSmallest(int k) const {
    if (k < 1 || k > nodeCount) {
        std::cout << "kthSmallest: k=" << k << " out of range [1, " << nodeCount << "]." << std::endl;
        return -1;
    }
    int count = 0, result = -1;
    kthHelper(root, count, k, result);
    return result;
}

// ========================== BST-specific LCA =================================
// Ordering-based: walk down from root comparing to both keys.

int BST::lcaBST(int a, int b) const {
    if (!search(a) || !search(b)) return -1;
    TreeNode* node = root;
    while (node) {
        if (a < node->value && b < node->value) {
            node = node->left;
        } else if (a > node->value && b > node->value) {
            node = node->right;
        } else {
            return node->value;
        }
    }
    return -1;
}

// ========================== Validator ========================================

bool BST::validateBST(TreeNode* node, const TreeNode* minNode, const TreeNode* maxNode) const {
    if (!node) return true;
    if (minNode && node->value <= minNode->value) return false;
    if (maxNode && node->value >= maxNode->value) return false;
    return validateBST(node->left, minNode, node)
        && validateBST(node->right, node, maxNode);
}

bool BST::isValidBST() const {
    return validateBST(root, nullptr, nullptr);
}

// ========================== Height ===========================================

int BST::heightHelper(TreeNode* node) const {
    if (!node) return 0;
    return 1 + maxInt(heightHelper(node->left), heightHelper(node->right));
}

int BST::getHeight() const {
    return heightHelper(root);
}

// ========================== Debug ============================================

void BST::inorderPrint(TreeNode* node) const {
    if (!node) return;
    inorderPrint(node->left);
    std::cout << node->value << " ";
    inorderPrint(node->right);
}

void BST::printInorder() const {
    inorderPrint(root);
    std::cout << std::endl;
}

} // namespace VaultHeist
