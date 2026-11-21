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

// ========================== Event helpers (file-local functions) =============

static inline void emitOp(EventBatch* events, int type, const char* label, int key) {
    if (!events) return;
    Event e(type, label);
    e.newValue = key;
    events->append(e);
}

// VISIT + COMPARE for one node checked against `key`.
// COMPARE: nodeId = node, oldValue = node value, newValue = key being compared.
static inline void emitVisitCompare(EventBatch* events, const TreeNode* node, int key) {
    if (!events) return;
    Event v(EVENT_VISIT, "visit");
    v.nodeId = node->id;
    v.newValue = node->value;
    events->append(v);

    Event c(EVENT_COMPARE, "compare");
    c.nodeId = node->id;
    c.oldValue = node->value;
    c.newValue = key;
    events->append(c);
}

// ========================== Construction / Destruction ========================

BST::BST() : root(nullptr), nextId(1), nodeCount(0) {}

BST::~BST() {
    destroyRecursive(root);
}

void BST::clear() {
    destroyRecursive(root);
    root = nullptr;
    nodeCount = 0;
    nextId = 1;
}

void BST::destroyRecursive(TreeNode* node) {
    if (!node) return;
    destroyRecursive(node->left);
    destroyRecursive(node->right);
    delete node;
}

// ========================== Insert ===========================================

TreeNode* BST::insertHelper(TreeNode* node, TreeNode* parent, int key, bool& inserted,
                            EventBatch* events) {
    if (!node) {
        TreeNode* n = new TreeNode(nextId++, key);
        n->parent = parent;
        inserted = true;
        nodeCount++;
        if (events) {
            Event e(EVENT_INSERT, "insert");
            e.nodeId = n->id;
            e.otherNodeId = parent ? parent->id : -1;
            e.newValue = key;
            events->append(e);
        }
        return n;
    }
    emitVisitCompare(events, node, key);
    if (key < node->value) {
        node->left = insertHelper(node->left, node, key, inserted, events);
    } else if (key > node->value) {
        node->right = insertHelper(node->right, node, key, inserted, events);
    }
    // key == node->value -> duplicate, inserted stays false
    return node;
}

bool BST::insert(int key) {
    bool inserted = false;
    root = insertHelper(root, nullptr, key, inserted, nullptr);
    if (!inserted) {
        std::cout << "Duplicate key " << key << " rejected." << std::endl;
    }
    return inserted;
}

bool BST::insert(int key, EventBatch& events) {
    emitOp(&events, EVENT_OPERATION_STARTED, "insert", key);
    bool inserted = false;
    root = insertHelper(root, nullptr, key, inserted, &events);
    if (!inserted) {
        std::cout << "Duplicate key " << key << " rejected." << std::endl;
        emitOp(&events, EVENT_OPERATION_FAILED, "insert: duplicate", key);
    } else {
        emitOp(&events, EVENT_OPERATION_COMPLETED, "insert", key);
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

TreeNode* BST::deleteHelper(TreeNode* node, int key, bool& deleted, EventBatch* events) {
    if (!node) return nullptr;

    emitVisitCompare(events, node, key);
    if (key < node->value) {
        node->left = deleteHelper(node->left, key, deleted, events);
        if (node->left) node->left->parent = node;
    } else if (key > node->value) {
        node->right = deleteHelper(node->right, key, deleted, events);
        if (node->right) node->right->parent = node;
    } else {
        deleted = true;

        // Case 1: Leaf node
        if (!node->left && !node->right) {
            if (events) {
                Event e(EVENT_UNLINK, "unlink leaf");
                e.nodeId = node->id;
                e.oldValue = node->value;
                events->append(e);
            }
            delete node;
            nodeCount--;
            return nullptr;
        }
        // Case 2a: Only right child
        if (!node->left) {
            TreeNode* rightChild = node->right;
            if (events) {
                Event e(EVENT_UNLINK, "unlink one-child");
                e.nodeId = node->id;
                e.otherNodeId = rightChild->id;
                e.oldValue = node->value;
                events->append(e);
            }
            rightChild->parent = node->parent;
            delete node;
            nodeCount--;
            return rightChild;
        }
        // Case 2b: Only left child
        if (!node->right) {
            TreeNode* leftChild = node->left;
            if (events) {
                Event e(EVENT_UNLINK, "unlink one-child");
                e.nodeId = node->id;
                e.otherNodeId = leftChild->id;
                e.oldValue = node->value;
                events->append(e);
            }
            leftChild->parent = node->parent;
            delete node;
            nodeCount--;
            return leftChild;
        }
        // Case 3: Two children — copy inorder successor
        TreeNode* succ = findMinNode(node->right);
        if (events) {
            Event e(EVENT_SWAP_VALUE, "copy successor");
            e.nodeId = node->id;
            e.otherNodeId = succ->id;
            e.oldValue = node->value;
            e.newValue = succ->value;
            events->append(e);
        }
        node->value = succ->value;
        node->id = succ->id;
        node->right = deleteHelper(node->right, succ->value, deleted, events);
        if (node->right) node->right->parent = node;
        deleted = true;
    }
    return node;
}

bool BST::deleteKey(int key) {
    bool deleted = false;
    root = deleteHelper(root, key, deleted, nullptr);
    if (root) root->parent = nullptr;
    return deleted;
}

bool BST::deleteKey(int key, EventBatch& events) {
    emitOp(&events, EVENT_OPERATION_STARTED, "delete", key);
    bool deleted = false;
    root = deleteHelper(root, key, deleted, &events);
    if (root) root->parent = nullptr;
    if (deleted) emitOp(&events, EVENT_OPERATION_COMPLETED, "delete", key);
    else         emitOp(&events, EVENT_OPERATION_FAILED, "delete: not found", key);
    return deleted;
}

// ========================== Search ===========================================

TreeNode* BST::searchHelper(TreeNode* node, int key) const {
    if (!node) return nullptr;
    if (key == node->value) return node;
    if (key < node->value) return searchHelper(node->left, key);
    return searchHelper(node->right, key);
}

TreeNode* BST::searchHelper(TreeNode* node, int key, EventBatch* events) const {
    if (!node) return nullptr;
    emitVisitCompare(events, node, key);
    if (key == node->value) return node;
    if (key < node->value) return searchHelper(node->left, key, events);
    return searchHelper(node->right, key, events);
}

bool BST::search(int key) const {
    return searchHelper(root, key) != nullptr;
}

bool BST::search(int key, EventBatch& events) const {
    emitOp(&events, EVENT_OPERATION_STARTED, "search", key);
    bool found = searchHelper(root, key, &events) != nullptr;
    if (found) emitOp(&events, EVENT_OPERATION_COMPLETED, "search", key);
    else       emitOp(&events, EVENT_OPERATION_FAILED, "search: not found", key);
    return found;
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

void BST::rangeQueryHelper(TreeNode* node, int lo, int hi, int* out, int& idx,
                           EventBatch* events) const {
    if (!node) return;
    if (events) {
        Event v(EVENT_VISIT, "visit");
        v.nodeId = node->id;
        v.newValue = node->value;
        events->append(v);
    }
    // Prune: if node->value > lo, left subtree may have keys in range
    if (node->value > lo) {
        rangeQueryHelper(node->left, lo, hi, out, idx, events);
    } else if (node->left && events) {
        Event h(EVENT_HIGHLIGHT_SUBTREE, "pruned subtree");
        h.nodeId = node->left->id;
        h.otherNodeId = node->id;
        events->append(h);
    }
    // Include this node if in range
    if (node->value >= lo && node->value <= hi)
        out[idx++] = node->value;
    // Prune: if node->value < hi, right subtree may have keys in range
    if (node->value < hi) {
        rangeQueryHelper(node->right, lo, hi, out, idx, events);
    } else if (node->right && events) {
        Event h(EVENT_HIGHLIGHT_SUBTREE, "pruned subtree");
        h.nodeId = node->right->id;
        h.otherNodeId = node->id;
        events->append(h);
    }
}

int BST::countNodes(TreeNode* node) const {
    if (!node) return 0;
    return 1 + countNodes(node->left) + countNodes(node->right);
}

int* BST::rangeQuery(int lo, int hi, int& count) const {
    count = 0;
    if (!root) return nullptr;
    int* result = new int[nodeCount];
    rangeQueryHelper(root, lo, hi, result, count, nullptr);
    if (count == 0) {
        delete[] result;
        return nullptr;
    }
    return result;
}

int* BST::rangeQuery(int lo, int hi, int& count, EventBatch& events) const {
    {
        Event s(EVENT_OPERATION_STARTED, "rangeQuery");
        s.oldValue = lo;
        s.newValue = hi;
        events.append(s);
    }
    count = 0;
    if (!root) {
        events.append(Event(EVENT_OPERATION_COMPLETED, "rangeQuery"));
        return nullptr;
    }
    // Allocate worst-case (all nodes in range); caller deletes.
    int maxPossible = nodeCount;
    int* result = new int[maxPossible];
    rangeQueryHelper(root, lo, hi, result, count, &events);
    events.append(Event(EVENT_OPERATION_COMPLETED, "rangeQuery"));
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

bool BST::validateBST(TreeNode* node, const TreeNode* minNode, const TreeNode* maxNode,
                      int& visitedCount) const {
    if (!node) return true;
    visitedCount++;
    if (visitedCount > nodeCount) return false; // cycle/shared-node guard
    if (minNode && node->value <= minNode->value) return false;
    if (maxNode && node->value >= maxNode->value) return false;
    if (node->left && node->left->parent != node) return false;
    if (node->right && node->right->parent != node) return false;
    return validateBST(node->left, minNode, node, visitedCount)
        && validateBST(node->right, node, maxNode, visitedCount);
}

bool BST::isValidBST() const {
    if (root && root->parent != nullptr) return false;
    int visitedCount = 0;
    if (!validateBST(root, nullptr, nullptr, visitedCount)) return false;
    return visitedCount == nodeCount;
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
