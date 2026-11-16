// =============================================================================
// BinaryTree.cpp — General Binary Tree (Act 1)
// No ordering invariant. Traversals, reconstruction, structural queries.
// Height convention: leaf = 1, nullptr = 0.
// =============================================================================

#include "structures/BinaryTree.h"

namespace VaultHeist {

// ========================== Helpers (local, not global vars) ==================

static inline int maxInt(int a, int b) { return (a > b) ? a : b; }

// ========================== Construction / Destruction ========================

BinaryTree::BinaryTree() : root(nullptr), nextId(1), nodeCount(0) {}

BinaryTree::~BinaryTree() {
    destroyRecursive(root);
}

void BinaryTree::destroyRecursive(TreeNode* node) {
    if (!node) return;
    destroyRecursive(node->left);
    destroyRecursive(node->right);
    delete node;
}

TreeNode* BinaryTree::addNode(int value, TreeNode* parent, bool asLeft) {
    TreeNode* n = new TreeNode(nextId++, value);
    n->parent = parent;
    if (!parent) {
        // Setting root (destroys old tree first for safety)
        destroyRecursive(root);
        root = n;
        nodeCount = 1;
    } else {
        if (asLeft) parent->left = n;
        else        parent->right = n;
        nodeCount++;
    }
    return n;
}

// ========================== Recursive Traversals =============================

void BinaryTree::preorderHelper(TreeNode* node, int* out, int& idx) const {
    if (!node) return;
    out[idx++] = node->value;
    preorderHelper(node->left, out, idx);
    preorderHelper(node->right, out, idx);
}

void BinaryTree::inorderHelper(TreeNode* node, int* out, int& idx) const {
    if (!node) return;
    inorderHelper(node->left, out, idx);
    out[idx++] = node->value;
    inorderHelper(node->right, out, idx);
}

void BinaryTree::postorderHelper(TreeNode* node, int* out, int& idx) const {
    if (!node) return;
    postorderHelper(node->left, out, idx);
    postorderHelper(node->right, out, idx);
    out[idx++] = node->value;
}

int* BinaryTree::preorderRecursive(int& count) const {
    count = nodeCount;
    if (count == 0) return nullptr;
    int* result = new int[count];
    int idx = 0;
    preorderHelper(root, result, idx);
    return result;
}

int* BinaryTree::inorderRecursive(int& count) const {
    count = nodeCount;
    if (count == 0) return nullptr;
    int* result = new int[count];
    int idx = 0;
    inorderHelper(root, result, idx);
    return result;
}

int* BinaryTree::postorderRecursive(int& count) const {
    count = nodeCount;
    if (count == 0) return nullptr;
    int* result = new int[count];
    int idx = 0;
    postorderHelper(root, result, idx);
    return result;
}

// ========================== Iterative Traversals =============================
// Using explicit stack (raw array) — no std::stack / std::vector.

int* BinaryTree::preorderIterative(int& count) const {
    count = nodeCount;
    if (count == 0) return nullptr;

    int* result = new int[count];
    int idx = 0;

    // Manual stack of TreeNode*
    TreeNode** stack = new TreeNode*[count];
    int top = -1;

    stack[++top] = root;
    while (top >= 0) {
        TreeNode* curr = stack[top--];
        result[idx++] = curr->value;
        // Push right first so left is processed first
        if (curr->right) stack[++top] = curr->right;
        if (curr->left)  stack[++top] = curr->left;
    }

    delete[] stack;
    return result;
}

int* BinaryTree::inorderIterative(int& count) const {
    count = nodeCount;
    if (count == 0) return nullptr;

    int* result = new int[count];
    int idx = 0;

    TreeNode** stack = new TreeNode*[count];
    int top = -1;

    TreeNode* curr = root;
    while (curr || top >= 0) {
        while (curr) {
            stack[++top] = curr;
            curr = curr->left;
        }
        curr = stack[top--];
        result[idx++] = curr->value;
        curr = curr->right;
    }

    delete[] stack;
    return result;
}

int* BinaryTree::postorderIterative(int& count) const {
    count = nodeCount;
    if (count == 0) return nullptr;

    int* result = new int[count];

    // Two-stack approach
    TreeNode** stack1 = new TreeNode*[count];
    TreeNode** stack2 = new TreeNode*[count];
    int top1 = -1, top2 = -1;

    stack1[++top1] = root;
    while (top1 >= 0) {
        TreeNode* curr = stack1[top1--];
        stack2[++top2] = curr;
        if (curr->left)  stack1[++top1] = curr->left;
        if (curr->right) stack1[++top1] = curr->right;
    }

    int idx = 0;
    while (top2 >= 0) {
        result[idx++] = stack2[top2--]->value;
    }

    delete[] stack1;
    delete[] stack2;
    return result;
}

// Level-order (BFS) with manual array-based queue — no std::queue.
int* BinaryTree::levelOrder(int& count) const {
    count = nodeCount;
    if (count == 0) return nullptr;

    int* result = new int[count];
    int idx = 0;

    // Array-based circular queue
    TreeNode** queue = new TreeNode*[count];
    int front = 0, back = 0;

    queue[back++] = root;
    while (front < back) {
        TreeNode* curr = queue[front++];
        result[idx++] = curr->value;
        if (curr->left)  queue[back++] = curr->left;
        if (curr->right) queue[back++] = curr->right;
    }

    delete[] queue;
    return result;
}

// ========================== Reconstruction ===================================

int BinaryTree::findIndex(const int* arr, int lo, int hi, int value) {
    for (int i = lo; i <= hi; ++i) {
        if (arr[i] == value) return i;
    }
    return -1;
}

TreeNode* BinaryTree::buildFromPreIn(const int* pre, int preStart, int preEnd,
                                      const int* in, int inStart, int inEnd,
                                      TreeNode* parent) {
    if (preStart > preEnd || inStart > inEnd) return nullptr;

    int rootVal = pre[preStart];
    TreeNode* node = new TreeNode(nextId++, rootVal);
    node->parent = parent;
    nodeCount++;

    int inIdx = findIndex(in, inStart, inEnd, rootVal);
    int leftSize = inIdx - inStart;

    node->left  = buildFromPreIn(pre, preStart + 1, preStart + leftSize,
                                 in, inStart, inIdx - 1, node);
    node->right = buildFromPreIn(pre, preStart + leftSize + 1, preEnd,
                                 in, inIdx + 1, inEnd, node);
    return node;
}

TreeNode* BinaryTree::buildFromPostIn(const int* post, int postStart, int postEnd,
                                       const int* in, int inStart, int inEnd,
                                       TreeNode* parent) {
    if (postStart > postEnd || inStart > inEnd) return nullptr;

    int rootVal = post[postEnd];
    TreeNode* node = new TreeNode(nextId++, rootVal);
    node->parent = parent;
    nodeCount++;

    int inIdx = findIndex(in, inStart, inEnd, rootVal);
    int leftSize = inIdx - inStart;

    node->left  = buildFromPostIn(post, postStart, postStart + leftSize - 1,
                                  in, inStart, inIdx - 1, node);
    node->right = buildFromPostIn(post, postStart + leftSize, postEnd - 1,
                                  in, inIdx + 1, inEnd, node);
    return node;
}

void BinaryTree::buildFromPreorderInorder(const int* preorder, const int* inorder, int n) {
    destroyRecursive(root);
    root = nullptr;
    nodeCount = 0;
    if (n <= 0) return;
    root = buildFromPreIn(preorder, 0, n - 1, inorder, 0, n - 1, nullptr);
}

void BinaryTree::buildFromPostorderInorder(const int* postorder, const int* inorder, int n) {
    destroyRecursive(root);
    root = nullptr;
    nodeCount = 0;
    if (n <= 0) return;
    root = buildFromPostIn(postorder, 0, n - 1, inorder, 0, n - 1, nullptr);
}

// ========================== Structural Queries ===============================

int BinaryTree::heightHelper(TreeNode* node) const {
    if (!node) return 0;  // nullptr = 0
    return 1 + maxInt(heightHelper(node->left), heightHelper(node->right));
}

int BinaryTree::height() const {
    return heightHelper(root);
}

// Returns height of node while updating maxDiam as side effect.
int BinaryTree::diameterHelper(TreeNode* node, int& maxDiam) const {
    if (!node) return 0;
    int lh = diameterHelper(node->left, maxDiam);
    int rh = diameterHelper(node->right, maxDiam);
    // Diameter through this node = leftHeight + rightHeight + 1 (counting nodes)
    int throughHere = lh + rh + 1;
    if (throughHere > maxDiam) maxDiam = throughHere;
    return 1 + maxInt(lh, rh);
}

int BinaryTree::diameter() const {
    if (!root) return 0;
    int maxDiam = 0;
    diameterHelper(root, maxDiam);
    return maxDiam;
}

bool BinaryTree::findNode(TreeNode* node, int val) const {
    if (!node) return false;
    if (node->value == val) return true;
    return findNode(node->left, val) || findNode(node->right, val);
}

// General-tree LCA — no ordering to exploit, must search both subtrees.
TreeNode* BinaryTree::lcaHelper(TreeNode* node, int a, int b) const {
    if (!node) return nullptr;
    if (node->value == a || node->value == b) return node;

    TreeNode* leftLca  = lcaHelper(node->left, a, b);
    TreeNode* rightLca = lcaHelper(node->right, a, b);

    if (leftLca && rightLca) return node;  // a and b in different subtrees
    return leftLca ? leftLca : rightLca;
}

int BinaryTree::lca(int a, int b) const {
    if (!findNode(root, a) || !findNode(root, b)) return -1;
    TreeNode* result = lcaHelper(root, a, b);
    return result ? result->value : -1;
}

bool BinaryTree::pathToHelper(TreeNode* node, int target, int* path, int& len) const {
    if (!node) return false;
    path[len++] = node->value;
    if (node->value == target) return true;
    if (pathToHelper(node->left, target, path, len)) return true;
    if (pathToHelper(node->right, target, path, len)) return true;
    len--;  // backtrack
    return false;
}

int* BinaryTree::pathTo(int target, int& pathLen) const {
    if (!root) { pathLen = 0; return nullptr; }
    // Worst case path length = nodeCount
    int* path = new int[nodeCount];
    pathLen = 0;
    if (pathToHelper(root, target, path, pathLen)) {
        return path;
    }
    delete[] path;
    pathLen = 0;
    return nullptr;
}

// ========================== Transforms =======================================

void BinaryTree::mirrorHelper(TreeNode* node) {
    if (!node) return;
    // Swap children
    TreeNode* tmp = node->left;
    node->left = node->right;
    node->right = tmp;
    mirrorHelper(node->left);
    mirrorHelper(node->right);
}

void BinaryTree::mirror() {
    mirrorHelper(root);
}

// ========================== Comparison =======================================

bool BinaryTree::isSameTreeHelper(TreeNode* a, TreeNode* b) const {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a->value != b->value) return false;
    return isSameTreeHelper(a->left, b->left)
        && isSameTreeHelper(a->right, b->right);
}

bool BinaryTree::isSameTree(const BinaryTree& other) const {
    return isSameTreeHelper(root, other.root);
}

bool BinaryTree::isSymmetricHelper(TreeNode* left, TreeNode* right) const {
    if (!left && !right) return true;
    if (!left || !right) return false;
    if (left->value != right->value) return false;
    return isSymmetricHelper(left->left, right->right)
        && isSymmetricHelper(left->right, right->left);
}

bool BinaryTree::isSymmetric() const {
    if (!root) return true;
    return isSymmetricHelper(root->left, root->right);
}

// ========================== Debug ============================================

void BinaryTree::printInorderHelper(TreeNode* node) const {
    if (!node) return;
    printInorderHelper(node->left);
    std::cout << node->value << " ";
    printInorderHelper(node->right);
}

void BinaryTree::printInorder() const {
    printInorderHelper(root);
    std::cout << std::endl;
}

} // namespace VaultHeist
