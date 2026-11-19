#include "structures/AVLTree.h"
#include <iostream>
#include <climits>

namespace VaultHeist {

// Benchmark-only instrumentation counters
long long AVLTree::s_comparisons = 0;
long long AVLTree::s_rotations = 0;

void AVLTree::resetCounters() {
    s_comparisons = 0;
    s_rotations = 0;
}

long long AVLTree::getComparisons() {
    return s_comparisons;
}

long long AVLTree::getRotations() {
    return s_rotations;
}

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
void AVLTree::updateHeightAndSizeImpl(AVLNode* node, EventBatch* events) {
    if (!node) return;

    int oldH = node->height;   // event bookkeeping only
    int oldS = node->size;     // event bookkeeping only

    int leftH = getNodeHeight(node->left);
    int rightH = getNodeHeight(node->right);
    node->height = 1 + (leftH > rightH ? leftH : rightH);

    int leftS = getNodeSize(node->left);
    int rightS = getNodeSize(node->right);
    node->size = 1 + leftS + rightS;

    if (events) {
        if (node->height != oldH) {
            Event e(EVENT_UPDATE_HEIGHT, "update height");
            e.nodeId = node->key;
            e.oldValue = oldH;
            e.newValue = node->height;
            events->append(e);
        }
        if (node->size != oldS) {
            Event e(EVENT_UPDATE_SIZE, "update size");
            e.nodeId = node->key;
            e.oldValue = oldS;
            e.newValue = node->size;
            events->append(e);
        }
    }
}

void AVLTree::updateHeightAndSize(AVLNode* node, EventBatch& events) {
    updateHeightAndSizeImpl(node, &events);
}

// LL Case: Single Right Rotation
//
//        y                              x
//       / \                            / \
//      x   T3     rotateRight(y)     T1   y
//     / \        -------------->         / \
//    T1  T2                             T2  T3
AVLNode* AVLTree::rotateRightImpl(AVLNode* y, EventBatch* events, int rotationCase) {
    s_rotations++;
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

    if (events) {
        Event e(EVENT_ROTATE, "rotate right");
        e.nodeId = y->key;       // pivot (old subtree root)
        e.otherNodeId = x->key;  // new subtree root
        e.rotationCase = rotationCase;
        events->append(e);
    }

    // Update heights and subtree sizes (child y first, then new parent x)
    updateHeightAndSizeImpl(y, events);
    updateHeightAndSizeImpl(x, events);

    return x; // New root of this subtree
}

AVLNode* AVLTree::rotateRight(AVLNode* y, EventBatch& events, int rotationCase) {
    return rotateRightImpl(y, &events, rotationCase);
}

// RR Case: Single Left Rotation
//
//       x                              y
//      / \                            / \
//     T1  y       rotateLeft(x)      x   T3
//        / \     ------------->     / \
//       T2  T3                     T1  T2
AVLNode* AVLTree::rotateLeftImpl(AVLNode* x, EventBatch* events, int rotationCase) {
    s_rotations++;
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

    if (events) {
        Event e(EVENT_ROTATE, "rotate left");
        e.nodeId = x->key;       // pivot (old subtree root)
        e.otherNodeId = y->key;  // new subtree root
        e.rotationCase = rotationCase;
        events->append(e);
    }

    // Update heights and subtree sizes (child x first, then new parent y)
    updateHeightAndSizeImpl(x, events);
    updateHeightAndSizeImpl(y, events);

    return y; // New root of this subtree
}

AVLNode* AVLTree::rotateLeft(AVLNode* x, EventBatch& events, int rotationCase) {
    return rotateLeftImpl(x, &events, rotationCase);
}

// Rebalances a node by checking its balance factor and selecting LL, RR, LR, or RL
AVLNode* AVLTree::rebalanceImpl(AVLNode* node, EventBatch* events) {
    if (!node) return nullptr;

    updateHeightAndSizeImpl(node, events);
    int bf = getBalanceFactor(node);

    if (events && (bf > 1 || bf < -1)) {
        Event e(EVENT_BALANCE_ALERT, "balance alert");
        e.nodeId = node->key;
        e.newValue = bf;
        events->append(e);
    }

    // Left Heavy (BF > 1)
    if (bf > 1) {
        // Event tag only: LR if the left child is right-heavy, else LL
        int rotationCase = (getBalanceFactor(node->left) < 0) ? ROTATION_LR : ROTATION_LL;
        // LR Case: Left child is right-heavy -> rotate left on left child first
        if (getBalanceFactor(node->left) < 0) {
            node->left = rotateLeftImpl(node->left, events, rotationCase);
        }
        // LL Case: Single right rotation
        return rotateRightImpl(node, events, rotationCase);
    }

    // Right Heavy (BF < -1)
    if (bf < -1) {
        // Event tag only: RL if the right child is left-heavy, else RR
        int rotationCase = (getBalanceFactor(node->right) > 0) ? ROTATION_RL : ROTATION_RR;
        // RL Case: Right child is left-heavy -> rotate right on right child first
        if (getBalanceFactor(node->right) > 0) {
            node->right = rotateRightImpl(node->right, events, rotationCase);
        }
        // RR Case: Single left rotation
        return rotateLeftImpl(node, events, rotationCase);
    }

    return node; // Node is already balanced (BF is -1, 0, or 1)
}

AVLNode* AVLTree::rebalance(AVLNode* node, EventBatch& events) {
    return rebalanceImpl(node, &events);
}

// VISIT + COMPARE for one node checked against `key` (file-local helper).
static inline void emitVisitCompare(EventBatch& events, const AVLNode* node, int key) {
    Event v(EVENT_VISIT, "visit");
    v.nodeId = node->key;
    v.newValue = node->key;
    events.append(v);

    Event c(EVENT_COMPARE, "compare");
    c.nodeId = node->key;
    c.oldValue = node->key;
    c.newValue = key;
    events.append(c);
}

static inline void emitOp(EventBatch& events, int type, const char* label, int key) {
    Event e(type, label);
    e.newValue = key;
    events.append(e);
}

// Insert Helper: standard BST insert with duplicate rejection, then rebalances up
AVLNode* AVLTree::insertHelper(AVLNode* node, AVLNode* parent, int key, bool& inserted,
                               EventBatch* events) {
    if (!node) {
        inserted = true;
        AVLNode* newNode = new AVLNode(key);
        newNode->parent = parent;
        if (events) {
            Event e(EVENT_INSERT, "insert");
            e.nodeId = key;
            e.otherNodeId = parent ? parent->key : -1;
            e.newValue = key;
            events->append(e);
        }
        return newNode;
    }

    if (events) emitVisitCompare(*events, node, key);
    s_comparisons++;
    if (key < node->key) {
        node->left = insertHelper(node->left, node, key, inserted, events);
    } else if (key > node->key) {
        node->right = insertHelper(node->right, node, key, inserted, events);
    } else {
        // Duplicate key: rejected according to Section 5.3 & 5.4
        std::cout << "Warning: Duplicate key " << key << " rejected in Vault!\n";
        inserted = false;
        return node;
    }

    return rebalanceImpl(node, events);
}

bool AVLTree::insertImpl(int key, EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "insert", key);
    bool inserted = false;
    root = insertHelper(root, nullptr, key, inserted, events);
    if (root) {
        root->parent = nullptr;
    }
    if (events) {
        if (inserted) emitOp(*events, EVENT_OPERATION_COMPLETED, "insert", key);
        else          emitOp(*events, EVENT_OPERATION_FAILED, "insert: duplicate", key);
    }
    return inserted;
}

bool AVLTree::insert(int key) {
    return insertImpl(key, nullptr);
}

bool AVLTree::insert(int key, EventBatch& events) {
    return insertImpl(key, &events);
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
AVLNode* AVLTree::deleteHelper(AVLNode* node, int key, bool& deleted, EventBatch* events) {
    if (!node) {
        deleted = false;
        return nullptr;
    }

    if (events) emitVisitCompare(*events, node, key);
    s_comparisons++;
    if (key < node->key) {
        node->left = deleteHelper(node->left, key, deleted, events);
    } else if (key > node->key) {
        node->right = deleteHelper(node->right, key, deleted, events);
    } else {
        // Key found!
        deleted = true;

        // Case 1: Leaf node (no children)
        if (!node->left && !node->right) {
            if (events) {
                Event e(EVENT_UNLINK, "unlink leaf");
                e.nodeId = node->key;
                e.oldValue = node->key;
                events->append(e);
            }
            delete node;
            return nullptr;
        }

        // Case 2: One child
        if (!node->left || !node->right) {
            AVLNode* child = node->left ? node->left : node->right;
            if (events) {
                Event e(EVENT_UNLINK, "unlink one-child");
                e.nodeId = node->key;
                e.otherNodeId = child->key;
                e.oldValue = node->key;
                events->append(e);
            }
            child->parent = node->parent;
            delete node;
            return child;
        }

        // Case 3: Two children
        // Find inorder successor (smallest node in right subtree)
        AVLNode* successor = findMinNode(node->right);
        if (events) {
            Event e(EVENT_SWAP_VALUE, "copy successor");
            e.nodeId = node->key;
            e.otherNodeId = successor->key;
            e.oldValue = node->key;
            e.newValue = successor->key;
            events->append(e);
        }
        node->key = successor->key;
        // Delete successor from right subtree
        node->right = deleteHelper(node->right, successor->key, deleted, events);
    }

    return rebalanceImpl(node, events);
}

bool AVLTree::deleteImpl(int key, EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "delete", key);
    bool deleted = false;
    root = deleteHelper(root, key, deleted, events);
    if (root) {
        root->parent = nullptr;
    }
    if (events) {
        if (deleted) emitOp(*events, EVENT_OPERATION_COMPLETED, "delete", key);
        else         emitOp(*events, EVENT_OPERATION_FAILED, "delete: not found", key);
    }
    return deleted;
}

bool AVLTree::deleteKey(int key) {
    return deleteImpl(key, nullptr);
}

bool AVLTree::deleteKey(int key, EventBatch& events) {
    return deleteImpl(key, &events);
}

// Search
AVLNode* AVLTree::searchHelper(AVLNode* node, int key) const {
    if (!node) return nullptr;
    s_comparisons++;
    if (node->key == key) return node;
    if (key < node->key) return searchHelper(node->left, key);
    return searchHelper(node->right, key);
}

AVLNode* AVLTree::searchHelper(AVLNode* node, int key, EventBatch& events) const {
    if (!node) return node;
    emitVisitCompare(events, node, key);
    s_comparisons++;
    if (node->key == key) return node;
    if (key < node->key) return searchHelper(node->left, key, events);
    return searchHelper(node->right, key, events);
}

bool AVLTree::search(int key) const {
    return searchHelper(root, key) != nullptr;
}

bool AVLTree::search(int key, EventBatch& events) const {
    emitOp(events, EVENT_OPERATION_STARTED, "search", key);
    bool found = searchHelper(root, key, events) != nullptr;
    if (found) emitOp(events, EVENT_OPERATION_COMPLETED, "search", key);
    else       emitOp(events, EVENT_OPERATION_FAILED, "search: not found", key);
    return found;
}

// Order Statistics in O(log n) using subtreeSize (Boss Level 3.5)
// k-th smallest element (1-based: k=1 returns minimum)
int AVLTree::kthHelper(AVLNode* node, int k) const {
    if (!node) return -1;

    s_comparisons++;
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

    s_comparisons++;
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
