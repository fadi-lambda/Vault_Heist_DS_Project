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

static inline void emitOp(EventBatch& events, int type, const char* label, int key) {
    Event e(type, label);
    e.newValue = key;
    events.append(e);
}

// VISIT + COMPARE for one node checked against `key`.
// COMPARE: nodeId = node, oldValue = node value, newValue = key being compared.
static inline void emitVisitCompare(EventBatch& events, const TreeNode* node, int key) {
    Event v(EVENT_VISIT, "visit");
    v.nodeId = node->id;
    v.newValue = node->value;
    events.append(v);

    Event c(EVENT_COMPARE, "compare");
    c.nodeId = node->id;
    c.oldValue = node->value;
    c.newValue = key;
    events.append(c);
}

// ========================== Benchmark Counters ===============================

long long BST::s_comparisons = 0;

void BST::resetCounters() {
    s_comparisons = 0;
}

long long BST::getComparisons() {
    return s_comparisons;
}

// ========================== Construction / Destruction ========================

BST::BST() : root(nullptr), nextId(1), nodeCount(0) {}

BST::~BST() {
    destroyRecursive(root);
    root = nullptr;
}

void BST::clear() {
    destroyRecursive(root);
    root = nullptr;
    nodeCount = 0;
    nextId = 1;
}

void BST::destroyRecursive(TreeNode* node) {
    // Iterative vine-flatten deletion: O(n) time, O(1) auxiliary/stack space
    // so degenerate chains of 10,000+ nodes never overflow the call stack.
    TreeNode* curr = node;
    while (curr) {
        if (curr->left) {
            TreeNode* left = curr->left;
            curr->left = left->right;
            left->right = curr;
            curr = left;
        } else {
            TreeNode* next = curr->right;
            delete curr;
            curr = next;
        }
    }
}

// ========================== Insert ===========================================

bool BST::insertImpl(int key, EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "insert", key);

    if (!root) {
        TreeNode* n = new TreeNode(nextId++, key);
        root = n;
        nodeCount++;
        if (events) {
            Event e(EVENT_INSERT, "insert");
            e.nodeId = n->id;
            e.otherNodeId = -1;
            e.newValue = key;
            events->append(e);
            emitOp(*events, EVENT_OPERATION_COMPLETED, "insert", key);
        }
        return true;
    }

    TreeNode* curr = root;
    TreeNode* parent = nullptr;
    bool goLeft = false;
    while (curr) {
        if (events) emitVisitCompare(*events, curr, key);
        s_comparisons++;
        parent = curr;
        if (key < curr->value) {
            goLeft = true;
            curr = curr->left;
        } else if (key > curr->value) {
            goLeft = false;
            curr = curr->right;
        } else {
            // key == curr->value -> duplicate rejected
            std::cout << "Duplicate key " << key << " rejected." << std::endl;
            if (events) emitOp(*events, EVENT_OPERATION_FAILED, "insert: duplicate", key);
            return false;
        }
    }

    TreeNode* n = new TreeNode(nextId++, key);
    n->parent = parent;
    if (goLeft) parent->left = n;
    else        parent->right = n;
    nodeCount++;

    if (events) {
        Event e(EVENT_INSERT, "insert");
        e.nodeId = n->id;
        e.otherNodeId = parent->id;
        e.newValue = key;
        events->append(e);
        emitOp(*events, EVENT_OPERATION_COMPLETED, "insert", key);
    }
    return true;
}

bool BST::insert(int key) {
    return insertImpl(key, nullptr);
}

bool BST::insert(int key, EventBatch& events) {
    return insertImpl(key, &events);
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

    if (events) emitVisitCompare(*events, node, key);
    s_comparisons++;
    if (key < node->value) {
        node->left = deleteHelper(node->left, key, deleted, events);
        if (node->left) node->left->parent = node;
    } else if (key > node->value) {
        node->right = deleteHelper(node->right, key, deleted, events);
        if (node->right) node->right->parent = node;
    } else {
        // Found the node to delete
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
        // Case 3: Two children — find inorder successor (min of right subtree),
        // copy its value, delete the successor node.
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
        node->id = succ->id;  // preserve the successor's id for tracking
        node->right = deleteHelper(node->right, succ->value, deleted, events);
        if (node->right) node->right->parent = node;
        deleted = true;
    }
    return node;
}

bool BST::deleteImpl(int key, EventBatch* events) {
    if (events) emitOp(*events, EVENT_OPERATION_STARTED, "delete", key);
    bool deleted = false;
    root = deleteHelper(root, key, deleted, events);
    if (root) root->parent = nullptr;
    if (events) {
        if (deleted) emitOp(*events, EVENT_OPERATION_COMPLETED, "delete", key);
        else         emitOp(*events, EVENT_OPERATION_FAILED, "delete: not found", key);
    }
    return deleted;
}

bool BST::deleteKey(int key) {
    return deleteImpl(key, nullptr);
}

bool BST::deleteKey(int key, EventBatch& events) {
    return deleteImpl(key, &events);
}

// ========================== Search ===========================================

TreeNode* BST::searchHelper(TreeNode* node, int key) const {
    return searchHelper(node, key, nullptr);
}

TreeNode* BST::searchHelper(TreeNode* node, int key, EventBatch* events) const {
    TreeNode* curr = node;
    while (curr) {
        if (events) emitVisitCompare(*events, curr, key);
        s_comparisons++;
        if (key == curr->value) return curr;
        if (key < curr->value) curr = curr->left;
        else                   curr = curr->right;
    }
    return nullptr;
}

bool BST::search(int key) const {
    return searchHelper(root, key, nullptr) != nullptr;
}

bool BST::search(int key, EventBatch& events) const {
    emitOp(events, EVENT_OPERATION_STARTED, "search", key);
    bool found = searchHelper(root, key, &events) != nullptr;
    if (found) emitOp(events, EVENT_OPERATION_COMPLETED, "search", key);
    else       emitOp(events, EVENT_OPERATION_FAILED, "search: not found", key);
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
                           EventBatch& events) const {
    if (!node) return;
    {
        Event v(EVENT_VISIT, "visit");
        v.nodeId = node->id;
        v.newValue = node->value;
        events.append(v);
    }
    // Prune: if node->value > lo, left subtree may have keys in range
    if (node->value > lo) {
        rangeQueryHelper(node->left, lo, hi, out, idx, events);
    } else if (node->left) {
        Event h(EVENT_HIGHLIGHT_SUBTREE, "pruned subtree");
        h.nodeId = node->left->id;
        h.otherNodeId = node->id;
        events.append(h);
    }
    // Include this node if in range
    if (node->value >= lo && node->value <= hi)
        out[idx++] = node->value;
    // Prune: if node->value < hi, right subtree may have keys in range
    if (node->value < hi) {
        rangeQueryHelper(node->right, lo, hi, out, idx, events);
    } else if (node->right) {
        Event h(EVENT_HIGHLIGHT_SUBTREE, "pruned subtree");
        h.nodeId = node->right->id;
        h.otherNodeId = node->id;
        events.append(h);
    }
}

int BST::countNodes(TreeNode* node) const {
    if (!node) return 0;
    return 1 + countNodes(node->left) + countNodes(node->right);
}

int* BST::rangeQuery(int lo, int hi, int& count) const {
    EventBatch tmp;
    return rangeQuery(lo, hi, count, tmp);
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
    rangeQueryHelper(root, lo, hi, result, count, events);
    events.append(Event(EVENT_OPERATION_COMPLETED, "rangeQuery"));
    if (count == 0) {
        delete[] result;
        return nullptr;
    }
    return result;
}

// ========================== k-th Smallest ====================================
// O(n) inorder walk — iterative manual stack avoids call-stack overflow on
// deep degenerate BSTs (e.g. n=10000 sorted chains).

void BST::kthHelper(TreeNode* node, int& count, int k, int& result) const {
    if (!node || count >= k) return;
    int cap = 64;
    TreeNode** st = new TreeNode*[cap];
    int top = 0;
    TreeNode* curr = node;
    while (curr || top > 0) {
        while (curr) {
            if (top >= cap) {
                int newCap = cap * 2;
                TreeNode** nst = new TreeNode*[newCap];
                for (int i = 0; i < top; ++i) nst[i] = st[i];
                delete[] st;
                st = nst;
                cap = newCap;
            }
            st[top++] = curr;
            curr = curr->left;
        }
        curr = st[--top];
        count++;
        s_comparisons++;
        if (count == k) {
            result = curr->value;
            break;
        }
        curr = curr->right;
    }
    delete[] st;
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

bool BST::validateBST(TreeNode* node, const TreeNode* /*minNode*/, const TreeNode* /*maxNode*/) const {
    if (!node) return true;
    int cap = 64;
    TreeNode** st = new TreeNode*[cap];
    int top = 0;
    TreeNode* curr = node;
    bool hasPrev = false;
    int prevVal = 0;
    bool valid = true;
    while ((curr || top > 0) && valid) {
        while (curr) {
            if (top >= cap) {
                int newCap = cap * 2;
                TreeNode** nst = new TreeNode*[newCap];
                for (int i = 0; i < top; ++i) nst[i] = st[i];
                delete[] st;
                st = nst;
                cap = newCap;
            }
            st[top++] = curr;
            curr = curr->left;
        }
        curr = st[--top];
        if (hasPrev && curr->value <= prevVal) {
            valid = false;
            break;
        }
        hasPrev = true;
        prevVal = curr->value;
        curr = curr->right;
    }
    delete[] st;
    return valid;
}

bool BST::isValidBST() const {
    return validateBST(root, nullptr, nullptr);
}

// ========================== Height ===========================================

int BST::heightHelper(TreeNode* node) const {
    if (!node) return 0;
    int cap = 64;
    TreeNode** q = new TreeNode*[cap];
    int head = 0, tail = 0;
    q[tail++] = node;
    int h = 0;
    while (head < tail) {
        int levelSize = tail - head;
        h++;
        for (int i = 0; i < levelSize; ++i) {
            TreeNode* cur = q[head++];
            if (cur->left) {
                if (tail >= cap) {
                    int active = tail - head;
                    int newCap = (active + 2 > cap) ? cap * 2 : cap;
                    TreeNode** nq = new TreeNode*[newCap];
                    for (int j = 0; j < active; ++j) nq[j] = q[head + j];
                    delete[] q;
                    q = nq;
                    cap = newCap;
                    tail = active;
                    head = 0;
                }
                q[tail++] = cur->left;
            }
            if (cur->right) {
                if (tail >= cap) {
                    int active = tail - head;
                    int newCap = (active + 2 > cap) ? cap * 2 : cap;
                    TreeNode** nq = new TreeNode*[newCap];
                    for (int j = 0; j < active; ++j) nq[j] = q[head + j];
                    delete[] q;
                    q = nq;
                    cap = newCap;
                    tail = active;
                    head = 0;
                }
                q[tail++] = cur->right;
            }
        }
        if (head == tail) {
            head = 0;
            tail = 0;
        }
    }
    delete[] q;
    return h;
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
