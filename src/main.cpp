// =============================================================================
// Vault Heist: The Living Vault — Main Game Source (1600x900)
// =============================================================================

#include <SFML/Graphics.hpp>
#include "structures/MaxHeap.h"
#include "structures/AVLTree.h"
#include "structures/BinaryTree.h"
#include "structures/BST.h"
#include "structures/TreeNode.h"
#include "gui/LayoutEngine.h"
#include "gui/Renderer.h"
#include "gui/AnimationController.h"
#include "gui/UndoStack.h"
#include "gui/ToastQueue.h"
#include "gui/ScoreLog.h"
#include "gui/Ui.h"
#include "gui/DragDrop.h"
#include "gui/DragDropView.h"
#include "game/LevelData.h"
#include "game/LevelLoader.h"
#include "game/Scoring.h"
#include "game/HintSystem.h"
#include "game/EndlessMode.h"
#include "benchmark/Benchmark.h"
#include <optional>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Hand-written replacements for <algorithm> — functions, not global variables.
static inline void swapInt(int& a, int& b) { int t = a; a = b; b = t; }
static inline float minFloat(float a, float b) { return (a < b) ? a : b; }
static inline int minInt(int a, int b) { return (a < b) ? a : b; }
static inline int maxInt(int a, int b) { return (a > b) ? a : b; }
static inline float clampFloat(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

using namespace VaultHeist;

// Screen State Constants (no enum allowed)
static const int SCREEN_SPLASH              = 0;
static const int SCREEN_MAIN_MENU           = 1;
static const int SCREEN_CAMPAIGN_MAP        = 2;
static const int SCREEN_LEVEL_PLAY          = 3;
static const int SCREEN_RESULT              = 4;
static const int SCREEN_SANDBOX             = 5;
static const int SCREEN_ACT1_DEMO           = 6;
static const int SCREEN_ACT2_DEMO           = 7;
static const int SCREEN_LEADERBOARD         = 8;
static const int SCREEN_SETTINGS            = 9;
static const int SCREEN_ENDLESS             = 10;
static const int SCREEN_ENDLESS_LEADERBOARD = 11;

// Traversal Mode Constants for Level 1.1 (no enum allowed)
static const int TRAV_PREORDER    = 0;
static const int TRAV_INORDER     = 1;
static const int TRAV_POSTORDER   = 2;
static const int TRAV_LEVEL_ORDER = 3;

// Level ID Constants (no enum allowed)
static const int LEVEL_1_1 = 11;  // Traversals Protocol (Pre / In / Post / Level)
static const int LEVEL_1_2 = 12;  // Decode the Blueprint (Pre + In Drag-Drop Reconstruction)
static const int LEVEL_1_3 = 13;  // The Alarm Spreads (Postorder Heights + Diameter)
static const int LEVEL_1_4 = 14;  // Find Where Two Routes Meet (LCA + Path Trace)
static const int LEVEL_1_5 = 15;  // Mirror the Vault (Bottom-up Swap or Auto Mirror)

static const int LEVEL_2_1 = 21;  // Search Heist (BST root-to-target comparisons)
static const int LEVEL_2_2 = 22;  // Stash the Loot (BST Insert & Duplicate Rejection)
static const int LEVEL_2_3 = 23;  // Steal and Collapse (BST Delete: Leaf / 1-Child / 2-Child)
static const int LEVEL_2_4 = 24;  // Range Raid ([lo, hi] + Subtree Pruning)
static const int LEVEL_2_5 = 25;  // Boss: The Lazy Architect (Degeneration & Search)

static const int LEVEL_3_1 = 31;  // Read the Badges (Identification)
static const int LEVEL_3_2 = 32;  // Manual Repair — Single (LL / RR)
static const int LEVEL_3_3 = 33;  // Manual Repair — Double (LR / RL)
static const int LEVEL_3_4 = 34;  // Auto Mode & Prediction
static const int LEVEL_3_5 = 35;  // Boss: Order-Statistic Heist

static const int LEVEL_4_1 = 41;  // Activate the Vault (Build Heap bottom-up)
static const int LEVEL_4_2 = 42;  // Repair Security Locks (Sift-Down)
static const int LEVEL_4_3 = 43;  // Extract the Greatest Treasure
static const int LEVEL_4_4 = 44;  // Race Against the Guards
static const int LEVEL_4_5 = 45;  // Final Vault Boss (Heap Sort)

struct CanvasCamera {
    sf::View view;
    float rectX, rectY, rectW, rectH;
    float zoom;
    bool panning;
    sf::Vector2i lastPanPixel;
    sf::Clock dblClickClock;
    sf::Vector2i lastClickPixel;
    bool hasLastClick;
    mutable int cachedPct;
    mutable std::string cachedLabel;

    CanvasCamera()
        : rectX(0.f), rectY(0.f), rectW(100.f), rectH(100.f),
          zoom(1.0f), panning(false), lastPanPixel(0, 0),
          lastClickPixel(0, 0), hasLastClick(false), cachedPct(-1) {}

    void init(float rx, float ry, float rw, float rh, float winW, float winH) {
        rectX = rx; rectY = ry; rectW = rw; rectH = rh;
        zoom = 1.0f;
        panning = false;
        hasLastClick = false;
        cachedPct = -1;
        view.setCenter({rx + rw * 0.5f, ry + rh * 0.5f});
        view.setSize({rw, rh});
        view.setViewport(sf::FloatRect({rx / winW, ry / winH}, {rw / winW, rh / winH}));
    }

    void reset() {
        zoom = 1.0f;
        panning = false;
        cachedPct = -1;
        view.setCenter({rectX + rectW * 0.5f, rectY + rectH * 0.5f});
        view.setSize({rectW, rectH});
    }

    void clampCenter() {
        sf::Vector2f c = view.getCenter();
        c.x = clampFloat(c.x, rectX - rectW, rectX + 2.f * rectW);
        c.y = clampFloat(c.y, rectY - rectH, rectY + 2.f * rectH);
        view.setCenter(c);
    }

    bool containsPixel(float px, float py) const {
        return px >= rectX && px <= rectX + rectW && py >= rectY && py <= rectY + rectH;
    }

    bool handleZoom(const sf::RenderWindow& window, sf::Vector2i pixelPos, float wheelDelta) {
        if (!containsPixel(static_cast<float>(pixelPos.x), static_cast<float>(pixelPos.y))) return false;
        sf::Vector2f beforeWorld = window.mapPixelToCoords(pixelPos, view);
        float factor = (wheelDelta > 0.f) ? 1.15f : (1.f / 1.15f);
        zoom = clampFloat(zoom * factor, 0.35f, 3.0f);
        view.setSize({rectW / zoom, rectH / zoom});
        sf::Vector2f afterWorld = window.mapPixelToCoords(pixelPos, view);
        view.move(beforeWorld - afterWorld);
        clampCenter();
        cachedPct = -1;
        return true;
    }

    bool checkDoubleClick(sf::Vector2i pixelPos) {
        float elapsed = dblClickClock.getElapsedTime().asSeconds();
        int dx = pixelPos.x - lastClickPixel.x;
        int dy = pixelPos.y - lastClickPixel.y;
        bool dbl = hasLastClick && (elapsed <= 0.35f) && ((dx * dx + dy * dy) <= 144);
        lastClickPixel = pixelPos;
        dblClickClock.restart();
        hasLastClick = !dbl;
        return dbl;
    }

    const std::string& zoomText() const {
        int pct = static_cast<int>(zoom * 100.f + 0.5f);
        if (cachedPct != pct) {
            cachedPct = pct;
            cachedLabel = "Zoom: " + std::to_string(pct) + "%  |  Drag Empty / R-Drag: Pan  |  Dbl-Click: Reset";
        }
        return cachedLabel;
    }
};

// Real AVLTree helpers for Act 3 (Levels 3.1 - 3.5)
static void updateAvlSubtreeMetadata(AVLTree& tree, AVLNode* node) {
    if (!node) return;
    updateAvlSubtreeMetadata(tree, node->left);
    updateAvlSubtreeMetadata(tree, node->right);
    if (node->left) node->left->parent = node;
    if (node->right) node->right->parent = node;
    EventBatch tmp;
    tree.updateHeightAndSize(node, tmp);
}

static AVLNode* findAvlNodeMutable(AVLNode* node, int key) {
    if (!node) return nullptr;
    if (node->key == key) return node;
    AVLNode* l = findAvlNodeMutable(node->left, key);
    return l ? l : findAvlNodeMutable(node->right, key);
}

static int findLevelNodeIndex(const LevelData& d, int key) {
    for (int i = 0; i < d.nodeCount; ++i) {
        if (d.nodes[i].key == key) return i;
    }
    return -1;
}

static AVLNode* buildUnbalancedAvlNodeRec(const LevelData& d, int key, AVLNode* parent) {
    int idx = findLevelNodeIndex(d, key);
    if (idx < 0) return nullptr;
    AVLNode* node = new AVLNode(d.nodes[idx].key);
    node->parent = parent;
    if (d.nodes[idx].left != -1) node->left = buildUnbalancedAvlNodeRec(d, d.nodes[idx].left, node);
    if (d.nodes[idx].right != -1) node->right = buildUnbalancedAvlNodeRec(d, d.nodes[idx].right, node);
    return node;
}

static void buildUnbalancedAvlFromData(AVLTree& tree, const LevelData& d) {
    tree.clear();
    if (d.nodeCount <= 0) return;
    AVLNode* r = buildUnbalancedAvlNodeRec(d, d.nodes[0].key, nullptr);
    tree.setRoot(r);
    updateAvlSubtreeMetadata(tree, tree.getRoot());
}

static void buildPuzzleAvl(AVLTree& tree, const LevelPuzzleSpec& p) {
    tree.clear();
    AVLNode* root = new AVLNode(p.rootKey);
    AVLNode* child = new AVLNode(p.childKey);
    AVLNode* inner = new AVLNode(p.innerKey);
    if (p.caseName == "LL") {
        root->left = child; child->parent = root;
        child->left = inner; inner->parent = child;
    } else if (p.caseName == "RR") {
        root->right = child; child->parent = root;
        child->right = inner; inner->parent = child;
    } else if (p.caseName == "LR") {
        root->left = child; child->parent = root;
        child->right = inner; inner->parent = child;
    } else if (p.caseName == "RL") {
        root->right = child; child->parent = root;
        child->left = inner; inner->parent = child;
    }
    tree.setRoot(root);
    updateAvlSubtreeMetadata(tree, tree.getRoot());
}

static int countAndValidateAvlSizesRec(const AVLNode* node, bool& ok, int& outBadKey) {
    if (!node) return 0;
    int ls = countAndValidateAvlSizesRec(node->left, ok, outBadKey);
    int rs = countAndValidateAvlSizesRec(node->right, ok, outBadKey);
    int realSz = 1 + ls + rs;
    if (ok && node->size != realSz) {
        ok = false;
        outBadKey = node->key;
    }
    return realSz;
}

static bool validateAvlSubtreeSizes(const AVLNode* root, int& outBadKey) {
    bool ok = true;
    outBadKey = -1;
    countAndValidateAvlSizesRec(root, ok, outBadKey);
    return ok;
}

static int kthLargestAvl(const AVLTree& tree, int k, int& outComparisons, EventBatch* events = nullptr) {
    outComparisons = 0;
    if (events) {
        Event s(EVENT_OPERATION_STARTED, "kth largest");
        s.newValue = k;
        events->append(s);
    }
    const AVLNode* cur = tree.getRoot();
    if (!cur || k < 1 || k > cur->size) {
        if (events) events->append(Event(EVENT_OPERATION_FAILED, "kth largest: out of range"));
        return -1;
    }
    int rem = k;
    while (cur) {
        int rightSize = cur->right ? cur->right->size : 0;
        outComparisons++;
        if (events) {
            Event v(EVENT_VISIT, "visit");
            v.nodeId = cur->key;
            v.newValue = cur->key;
            events->append(v);

            Event c(EVENT_COMPARE, "compare right size");
            c.nodeId = cur->key;
            c.oldValue = rightSize + 1;
            c.newValue = rem;
            events->append(c);
        }
        if (rem == rightSize + 1) {
            if (events) {
                Event done(EVENT_OPERATION_COMPLETED, "kth largest");
                done.nodeId = cur->key;
                done.newValue = cur->key;
                events->append(done);
            }
            return cur->key;
        }
        if (rem <= rightSize) {
            cur = cur->right;
        } else {
            rem -= (rightSize + 1);
            cur = cur->left;
        }
    }
    return -1;
}

static void attachBinaryTreeFromSpec(BinaryTree& tree, const LevelData& d, int key,
                                     TreeNode* parent, bool asLeft) {
    int idx = findLevelNodeIndex(d, key);
    if (idx < 0) return;
    TreeNode* created = tree.addNode(d.nodes[idx].key, parent, asLeft);
    if (d.nodes[idx].left != -1) attachBinaryTreeFromSpec(tree, d, d.nodes[idx].left, created, true);
    if (d.nodes[idx].right != -1) attachBinaryTreeFromSpec(tree, d, d.nodes[idx].right, created, false);
}

static void buildBinaryTreeFromData(BinaryTree& tree, const LevelData& d) {
    tree = BinaryTree();
    if (d.nodeCount > 0) attachBinaryTreeFromSpec(tree, d, d.nodes[0].key, nullptr, true);
}

static const char* traversalModeName(int mode) {
    switch (mode) {
        case TRAV_PREORDER:    return "PREORDER (Root -> Left -> Right)";
        case TRAV_INORDER:     return "INORDER (Left -> Root -> Right)";
        case TRAV_POSTORDER:   return "POSTORDER (Left -> Right -> Root)";
        case TRAV_LEVEL_ORDER: return "LEVEL-ORDER (Breadth-First)";
        default:               return "INORDER (Left -> Root -> Right)";
    }
}

static const char* traversalModeShort(int mode) {
    switch (mode) {
        case TRAV_PREORDER:    return "Preorder";
        case TRAV_INORDER:     return "Inorder";
        case TRAV_POSTORDER:   return "Postorder";
        case TRAV_LEVEL_ORDER: return "Level-Order";
        default:               return "Inorder";
    }
}

static int* getAct1Traversal(const BinaryTree& tree, int mode, int& cnt, EventBatch* events = nullptr) {
    cnt = 0;
    if (events) {
        if (mode == TRAV_PREORDER)    return tree.preorderIterative(cnt, *events);
        if (mode == TRAV_POSTORDER)   return tree.postorderIterative(cnt, *events);
        if (mode == TRAV_LEVEL_ORDER) return tree.levelOrder(cnt, *events);
        return tree.inorderIterative(cnt, *events);
    }
    if (mode == TRAV_PREORDER)    return tree.preorderRecursive(cnt);
    if (mode == TRAV_POSTORDER)   return tree.postorderRecursive(cnt);
    if (mode == TRAV_LEVEL_ORDER) return tree.levelOrder(cnt);
    return tree.inorderRecursive(cnt);
}

static const TreeNode* findTreeNodeConst(const TreeNode* node, int key) {
    if (!node) return nullptr;
    if (node->value == key) return node;
    const TreeNode* l = findTreeNodeConst(node->left, key);
    return l ? l : findTreeNodeConst(node->right, key);
}

static TreeNode* findTreeNodeMutable(TreeNode* node, int key) {
    if (!node) return nullptr;
    if (node->value == key) return node;
    TreeNode* l = findTreeNodeMutable(node->left, key);
    return l ? l : findTreeNodeMutable(node->right, key);
}

static void buildTreeFromSlotsRec(const LayoutResult& slotLayout, const DragDropController& dd,
                                  int slotIdx, TreeNode* parentOut, bool asLeft, BinaryTree& outTree) {
    if (slotIdx < 0 || slotIdx >= slotLayout.nodeCount) return;
    int tokId = dd.tokenInSlot(slotIdx);
    const DragToken* tok = dd.token(tokId);
    if (!tok) return;
    TreeNode* created = outTree.addNode(tok->value, parentOut, asLeft);
    int myId = slotLayout.nodes[slotIdx].id;
    float myX = slotLayout.nodes[slotIdx].x;
    int leftSlot = -1, rightSlot = -1;
    for (int i = 0; i < slotLayout.nodeCount; ++i) {
        if (slotLayout.nodes[i].parentId == myId) {
            if (slotLayout.nodes[i].x < myX) leftSlot = i;
            else rightSlot = i;
        }
    }
    if (leftSlot >= 0)  buildTreeFromSlotsRec(slotLayout, dd, leftSlot,  created, true,  outTree);
    if (rightSlot >= 0) buildTreeFromSlotsRec(slotLayout, dd, rightSlot, created, false, outTree);
}

static void buildTreeFromSlots(const LayoutResult& slotLayout, const DragDropController& dd, BinaryTree& outTree) {
    outTree = BinaryTree();
    int rootSlot = -1;
    for (int i = 0; i < slotLayout.nodeCount; ++i) {
        if (slotLayout.nodes[i].parentId == -1) { rootSlot = i; break; }
    }
    if (rootSlot >= 0) buildTreeFromSlotsRec(slotLayout, dd, rootSlot, nullptr, true, outTree);
}

static void buildRouteBetween(const BinaryTree& tree, int keyA, int keyB, int* outPath, int& outLen) {
    outLen = 0;
    int lenA = 0, lenB = 0;
    int* pA = tree.pathTo(keyA, lenA);
    int* pB = tree.pathTo(keyB, lenB);
    if (pA && pB && lenA > 0 && lenB > 0) {
        int lcaIdx = 0;
        int minLen = minInt(lenA, lenB);
        while (lcaIdx + 1 < minLen && pA[lcaIdx + 1] == pB[lcaIdx + 1]) {
            ++lcaIdx;
        }
        for (int i = lenA - 1; i >= lcaIdx && outLen < 16; --i) {
            outPath[outLen++] = pA[i];
        }
        for (int i = lcaIdx + 1; i < lenB && outLen < 16; ++i) {
            outPath[outLen++] = pB[i];
        }
    }
    delete[] pA;
    delete[] pB;
}

static bool containsInt(const int* arr, int count, int val) {
    for (int i = 0; i < count; ++i) {
        if (arr[i] == val) return true;
    }
    return false;
}

static bool allSubtreeInternalsSwapped(const TreeNode* sub, const int* swappedKeys, int swappedCount) {
    if (!sub) return true;
    if (sub->left || sub->right) {
        if (!containsInt(swappedKeys, swappedCount, sub->value)) return false;
    }
    return allSubtreeInternalsSwapped(sub->left, swappedKeys, swappedCount) &&
           allSubtreeInternalsSwapped(sub->right, swappedKeys, swappedCount);
}

static bool allInternalDescendantsSwapped(const TreeNode* node, const int* swappedKeys, int swappedCount) {
    if (!node) return true;
    return allSubtreeInternalsSwapped(node->left, swappedKeys, swappedCount) &&
           allSubtreeInternalsSwapped(node->right, swappedKeys, swappedCount);
}

static bool allInternalNodesSwapped(const TreeNode* root, const int* swappedKeys, int swappedCount) {
    return allSubtreeInternalsSwapped(root, swappedKeys, swappedCount);
}

static void buildBstFromData(BST& bst, const LevelData& d) {
    bst = BST();
    for (int i = 0; i < d.initialKeyCount; ++i) bst.insert(d.initialKeys[i]);
}

static int treeHeightNodes(const TreeNode* node) {
    if (!node) return 0;
    int lh = treeHeightNodes(node->left);
    int rh = treeHeightNodes(node->right);
    return 1 + maxInt(lh, rh);
}

static int findBstInsertParent(const TreeNode* root, int newKey) {
    const TreeNode* cur = root;
    const TreeNode* parent = nullptr;
    while (cur) {
        parent = cur;
        if (newKey < cur->value) cur = cur->left;
        else if (newKey > cur->value) cur = cur->right;
        else return cur->value;
    }
    return parent ? parent->value : -1;
}

static int bstDeleteCase(const TreeNode* node) {
    if (!node) return 0;
    if (!node->left && !node->right) return 1;
    if (!node->left || !node->right) return 2;
    return 3;
}

static const TreeNode* findBstMinNodeConst(const TreeNode* node) {
    if (!node) return nullptr;
    while (node->left) node = node->left;
    return node;
}

// Level 2.4 Range Raid action constants (no enum allowed)
static const int RQ_ACT_REPORT      = 1;
static const int RQ_ACT_PRUNE_LEFT  = 2;
static const int RQ_ACT_PRUNE_RIGHT = 3;

struct Act2RangeStep {
    int nodeId;
    int nodeKey;
    int expectedAction;
    int subtreeRootId;
    int subtreeRootKey;

    Act2RangeStep()
        : nodeId(-1), nodeKey(0), expectedAction(RQ_ACT_REPORT),
          subtreeRootId(-1), subtreeRootKey(0) {}
};

static void collectSubtreeKeys(const TreeNode* node, int* outKeys, int& count, int maxCap) {
    if (!node || count >= maxCap) return;
    outKeys[count++] = node->value;
    collectSubtreeKeys(node->left, outKeys, count, maxCap);
    collectSubtreeKeys(node->right, outKeys, count, maxCap);
}

static void buildAct2RangeDecisionsRec(const TreeNode* node, int lo, int hi,
                                        Act2RangeStep* out, int& count, int maxCap) {
    if (!node || count >= maxCap) return;
    if (node->value >= lo && node->value <= hi && count < maxCap) {
        Act2RangeStep s;
        s.nodeId = node->id;
        s.nodeKey = node->value;
        s.expectedAction = RQ_ACT_REPORT;
        out[count++] = s;
    }
    if (node->value <= lo && node->left && count < maxCap) {
        Act2RangeStep s;
        s.nodeId = node->id;
        s.nodeKey = node->value;
        s.expectedAction = RQ_ACT_PRUNE_LEFT;
        s.subtreeRootId = node->left->id;
        s.subtreeRootKey = node->left->value;
        out[count++] = s;
    }
    if (node->value >= hi && node->right && count < maxCap) {
        Act2RangeStep s;
        s.nodeId = node->id;
        s.nodeKey = node->value;
        s.expectedAction = RQ_ACT_PRUNE_RIGHT;
        s.subtreeRootId = node->right->id;
        s.subtreeRootKey = node->right->value;
        out[count++] = s;
    }
    if (node->value > lo) buildAct2RangeDecisionsRec(node->left, lo, hi, out, count, maxCap);
    if (node->value < hi) buildAct2RangeDecisionsRec(node->right, lo, hi, out, count, maxCap);
}

static void buildAct2RangeDecisions(const TreeNode* root, int lo, int hi,
                                     Act2RangeStep* out, int& count, int maxCap) {
    count = 0;
    buildAct2RangeDecisionsRec(root, lo, hi, out, count, maxCap);
}

static int act2BossCapacityFor(int optSteps, int chainHeight) {
    int base = optSteps + 6;
    int shrunk = base - maxInt(0, chainHeight - 1);
    return maxInt(optSteps + 2, shrunk);
}

static int heapLargerChildIdx(const int* arr, int n, int idx) {
    if (idx < 0 || idx >= n) return -1;
    int l = 2 * idx + 1;
    int r = 2 * idx + 2;
    int big = idx;
    if (l < n && arr[l] > arr[big]) big = l;
    if (r < n && arr[r] > arr[big]) big = r;
    return (big != idx) ? big : -1;
}

static int findNextHeapViolation(const int* arr, int n, int startIdx) {
    int maxStart = n / 2 - 1;
    if (startIdx > maxStart) startIdx = maxStart;
    for (int i = startIdx; i >= 0; --i) {
        if (heapLargerChildIdx(arr, n, i) >= 0) return i;
    }
    return -1;
}

static void siftDownHeapArrayWithEvents(int* arr, int n, int startIdx, EventBatch& batch) {
    Event st(EVENT_HEAPIFY_START, "heapify");
    st.indexA = startIdx;
    batch.append(st);
    int i = startIdx;
    while (true) {
        int l = 2 * i + 1;
        int r = 2 * i + 2;
        int big = i;
        if (l < n && arr[l] > arr[big]) big = l;
        if (r < n && arr[r] > arr[big]) big = r;
        if (big == i) break;
        Event sw(EVENT_HEAP_SWAP, "heap swap");
        sw.indexA = i;
        sw.indexB = big;
        sw.oldValue = arr[i];
        sw.newValue = arr[big];
        batch.append(sw);
        swapInt(arr[i], arr[big]);
        i = big;
    }
    Event dn(EVENT_HEAPIFY_DONE, "heapify done");
    dn.indexA = i;
    batch.append(dn);
}

static void insertExtractedMaxIntoSortedTail(int* sortedArr, int& sortedCount, int maxVal) {
    if (sortedCount < 0) sortedCount = 0;
    if (sortedCount >= 30) sortedCount = 29;
    for (int i = sortedCount; i > 0; --i) {
        sortedArr[i] = sortedArr[i - 1];
    }
    sortedArr[0] = maxVal;
    sortedCount++;
}

static bool validateAct4SortedResult(const LevelData& d, const int* sortedArr, int sortedCount) {
    if (d.expectedSortedResultCount <= 0) return false;
    if (sortedCount != d.expectedSortedResultCount) return false;
    for (int i = 0; i < sortedCount; ++i) {
        if (sortedArr[i] != d.expectedSortedResult[i]) return false;
    }
    return true;
}

struct LevelInfo {
    int id;
    int act;
    std::string title;
    std::string objective;
    std::string controls;
    int alarmCapacity;
    bool unlocked;
    int stars;

    LevelInfo() : id(0), act(0), alarmCapacity(20), unlocked(false), stars(0) {}
    LevelInfo(int i, int a, std::string t, std::string o, std::string c, int cap, bool u, int s)
        : id(i), act(a), title(t), objective(o), controls(c), alarmCapacity(cap), unlocked(u), stars(s) {}
};

static void drawTreeWatermark(sf::RenderWindow& window, float rootX, float rootY) {
    sf::Color branchColor(24, 40, 64, 130);
    sf::Color nodeFill(13, 22, 38, 160);
    sf::Color nodeOutline(26, 46, 74, 170);

    float lx = rootX - 68.f, ly = rootY + 86.f;
    float rx = rootX + 68.f, ry = rootY + 86.f;

    sf::Vertex lBranch[] = { sf::Vertex({rootX, rootY}, branchColor), sf::Vertex({lx, ly}, branchColor) };
    sf::Vertex rBranch[] = { sf::Vertex({rootX, rootY}, branchColor), sf::Vertex({rx, ry}, branchColor) };
    window.draw(lBranch, 2, sf::PrimitiveType::Lines);
    window.draw(rBranch, 2, sf::PrimitiveType::Lines);

    float r = 32.f;
    auto drawWMNode = [&](float x, float y) {
        sf::CircleShape c(r);
        c.setOrigin({r, r});
        c.setPosition({x, y});
        c.setFillColor(nodeFill);
        c.setOutlineThickness(2.2f);
        c.setOutlineColor(nodeOutline);
        window.draw(c);
    };

    drawWMNode(rootX, rootY);
    drawWMNode(lx, ly);
    drawWMNode(rx, ry);
}

static std::string currentDateTag() {
    std::time_t now = std::time(nullptr);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Operator (%04d-%02d-%02d %02d:%02d)",
                  tmBuf.tm_year + 1900, tmBuf.tm_mon + 1, tmBuf.tm_mday,
                  tmBuf.tm_hour, tmBuf.tm_min);
    return std::string(buf);
}

// =============================================================================
// MAIN FUNCTION
// =============================================================================
int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--benchmark") == 0) {
            std::string reportPath = "benchmark_report.txt";
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                reportPath = argv[i + 1];
            }
            return Benchmark::runAll(reportPath) ? 0 : 1;
        }
    }

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::string verifyDir;
    bool profileFps = false;
    bool verifyAct1 = false;
    bool verifyAct2 = false;
    bool verifyAct3 = false;
    bool verifyAct4 = false;
    bool verifyEndless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verify-screenshots") == 0 && i + 1 < argc) {
            verifyDir = argv[++i];
        } else if (std::strcmp(argv[i], "--profile-fps") == 0) {
            profileFps = true;
        } else if (std::strcmp(argv[i], "--verify-act1") == 0) {
            verifyAct1 = true;
        } else if (std::strcmp(argv[i], "--verify-act2") == 0) {
            verifyAct2 = true;
        } else if (std::strcmp(argv[i], "--verify-act3") == 0) {
            verifyAct3 = true;
        } else if (std::strcmp(argv[i], "--verify-act4") == 0) {
            verifyAct4 = true;
        } else if (std::strcmp(argv[i], "--verify-endless") == 0) {
            verifyEndless = true;
        }
    }

    constexpr unsigned int W = SCREEN_WIDTH;
    constexpr unsigned int H = SCREEN_HEIGHT;
    sf::RenderWindow window(sf::VideoMode({W, H}), "Vault Heist: The Living Vault");
    int targetFps = 60;
    window.setFramerateLimit(static_cast<unsigned int>(targetFps));

    sf::Font font;
    bool fontLoaded = font.openFromFile("assets/fonts/arial.ttf") ||
                      font.openFromFile("../../assets/fonts/arial.ttf") ||
                      font.openFromFile("out/build/x64-debug/assets/fonts/arial.ttf") ||
                      font.openFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/fonts/arial.ttf") ||
                      font.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (!fontLoaded) std::cerr << "Warning: Arial Font could not be loaded!\n";

    sf::Font menuMonoFont;
    bool menuMonoLoaded = menuMonoFont.openFromFile("assets/fonts/Orbitron-Medium.ttf") ||
                          menuMonoFont.openFromFile("../../assets/fonts/Orbitron-Medium.ttf") ||
                          menuMonoFont.openFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/fonts/Orbitron-Medium.ttf") ||
                          menuMonoFont.openFromFile("C:/Windows/Fonts/consola.ttf") ||
                          menuMonoFont.openFromFile("C:/Windows/Fonts/cour.ttf");
    const sf::Font& menuFont = menuMonoLoaded ? menuMonoFont : font;

    sf::Texture vaultBgTex;
    bool vaultBgLoaded = vaultBgTex.loadFromFile("assets/images/vault_door_bg.png") ||
                         vaultBgTex.loadFromFile("../../assets/images/vault_door_bg.png") ||
                         vaultBgTex.loadFromFile("out/build/x64-debug/assets/images/vault_door_bg.png") ||
                         vaultBgTex.loadFromFile("E:/Vault_Heist_DS/out/build/x64-debug/assets/images/vault_door_bg.png");
    if (vaultBgLoaded) vaultBgTex.setSmooth(true);
    std::optional<sf::Sprite> vaultBgSprite;
    if (vaultBgLoaded) {
        vaultBgSprite.emplace(vaultBgTex);
        sf::Vector2u texSz = vaultBgTex.getSize();
        if (texSz.x > 0 && texSz.y > 0) {
            vaultBgSprite->setScale({
                static_cast<float>(W) / static_cast<float>(texSz.x),
                static_cast<float>(H) / static_cast<float>(texSz.y)
            });
        }
    }

    // Persistent score logs (Campaign scores.txt & Endless endless_leaderboard.txt)
    ScoreLog scoreLog;
    scoreLog.load("scores.txt");
    EndlessLeaderboard endlessLb;
    endlessLb.load("endless_leaderboard.txt");
    const std::string endlessSavePath = "endless_save.txt";

    // 20 Campaign Levels across 4 Wings (5 levels per Wing) — loaded from levels/actN/level_N_M.json
    const int TOTAL_LEVELS = 20;
    const int LEVEL_IDS[TOTAL_LEVELS] = {
        LEVEL_1_1, LEVEL_1_2, LEVEL_1_3, LEVEL_1_4, LEVEL_1_5,
        LEVEL_2_1, LEVEL_2_2, LEVEL_2_3, LEVEL_2_4, LEVEL_2_5,
        LEVEL_3_1, LEVEL_3_2, LEVEL_3_3, LEVEL_3_4, LEVEL_3_5,
        LEVEL_4_1, LEVEL_4_2, LEVEL_4_3, LEVEL_4_4, LEVEL_4_5
    };
    LevelData levelData[TOTAL_LEVELS];
    LevelInfo levels[TOTAL_LEVELS];
    for (int i = 0; i < TOTAL_LEVELS; ++i) {
        std::string loadErr;
        if (!loadLevelById(LEVEL_IDS[i], levelData[i], loadErr)) {
            std::cerr << "Error loading level " << LEVEL_IDS[i] << ": " << loadErr << "\n";
            return 1;
        }
        int opt = computeOptimalSteps(levelData[i]);
        int cap = (levelData[i].id == LEVEL_2_5) ? act2BossCapacityFor(opt, 1) : ScoringEngine::capacityFor(opt);
        levels[i] = LevelInfo(levelData[i].id, levelData[i].act, levelData[i].title,
                              levelData[i].objective, levelData[i].controls,
                              cap, (i == 0), 0);
    }

    // Real configurable settings (Item 4)
    bool soundEnabled   = true;
    bool showNodeBadges = true;
    bool showBgGrid     = true;
    bool autoPlayEvents = true;

    // Puzzle tracking state
    bool act1Reconstructed = false;
    bool act1Mirrored = false;
    int act1SeqIdx = 0;
    int act1TravMode = TRAV_PREORDER;
    int act1TravCounter = 0;
    int act1PostIdx = 0;
    int act1StepperVal = 1;
    int act1ConfirmedHeight[32];
    int act1LcaStage = 0;
    int act1LcaA = -1;
    int act1LcaB = -1;
    int act1RoutePath[16];
    int act1RouteLen = 0;
    int act1RouteIdx = 0;
    int act1SwappedKeys[32];
    int act1SwappedCount = 0;

    int act2TargetIdx = 0;
    int act2CurKey = -9999;
    int act2DelStage = 0;
    bool act2TwoChildPhase2Pending = false;
    LayoutResult act2MidLayout;
    EventBatch act2Phase2Batch;
    Act2RangeStep act2RqSteps[32];
    int act2RqCount = 0;
    int act2RqIdx = 0;
    int act2PrunedKeys[32];
    int act2PrunedCount = 0;
    int act2LastPrunedId = -1;
    std::string act2ReportedStr = "";
    int act2BossStage = 0;

    // Multi-selection markers for rendering
    int markedIds[16];
    int markedCount = 0;
    auto clearMarked = [&]() {
        markedCount = 0;
        for (int i = 0; i < 16; ++i) markedIds[i] = -1;
    };
    auto addMarkedId = [&](int id) {
        if (id < 0 || markedCount >= 16) return;
        for (int i = 0; i < markedCount; ++i) {
            if (markedIds[i] == id) return;
        }
        markedIds[markedCount++] = id;
    };

    // Game state & shared Phase 7 foundation systems
    int currentScreen = SCREEN_SPLASH;
    int menuSelectedIdx = 0;
    int selectedActIdx = 0;
    int selectedLevelInAct = 0;
    int currentLevelIdx = -1;
    std::string status = "Welcome to Vault Heist!";
    ScoringEngine scoring;
    HintSystem hintSys;
    DragDropController dragDrop;
    int alarmSteps = 0, score = SCORE_BASE;
    float splashTimer = 0.0f;
    float uiAnimTimer = 0.0f;
    float dialAngle = 0.0f;
    sf::Clock deltaClock;

    // Act 3 State (real AVLTree + live rotation/size validation)
    int manualSubPuzzle = 1;
    int manualDoubleStep = 0;
    int clicked31Mask = 0;
    bool act3PivotPicked = false;
    int act3PivotKey = -1;
    bool act3NextPuzzlePending = false;
    int act3LastComps = 0;
    int act3LastHeight = 0;
    bool act3SizeOk = true;

    AVLTree avl;
    int avlQueuePos = 0;

    int bossQueryIdx = 0;

    // Act 4 State
    MaxHeap act4Heap;
    int heapArr[30];
    int activeHeapSize = 0;
    int sortedArr[30];
    int sortedCount = 0;
    int buildStepIdx = -1;
    int hlIndex1 = -1, hlIndex2 = -1;
    bool act4Built = false;

    // Act 1 (General Binary Tree) State
    bool layoutDirty = true;
    BinaryTree act1Tree;
    BinaryTree act1RefTree;
    int act1Hl1 = -9999, act1Hl2 = -9999;
    std::string act1TraversalStr = "";
    auto resetAct1 = [&]() {
        buildBinaryTreeFromData(act1Tree, levelData[0]);
        act1Hl1 = -9999;
        act1Hl2 = -9999;
        act1TraversalStr = "";
        layoutDirty = true;
    };
    resetAct1();

    // Act 2 (BST) State & Shared GUI Controllers
    BST act2Bst;
    int act2Hl1 = -9999, act2Hl2 = -9999;
    std::string act2ExtraInfo = "";
    LayoutResult bstLayout, btLayout, avlLayout, heapLayout;
    Renderer guiRenderer;
    AnimationController animCtrl;
    UndoStack undoStack;
    ToastQueue toastQueue;
    UiCache uiCache;
    UiCache menuUiCache;
    int cachedLayoutScreen = -1;
    int cachedLayoutLid = -1;

    // Live FPS readout state (Part A)
    sf::Clock fpsReportClock;
    int fpsFrameAccum = 0;
    std::string liveFpsStr = "FPS: --";

    // Cached render strings (Part A: avoid per-frame std::to_string & concatenation)
    bool campCachedUnl[20];
    int campCachedStars[20];
    std::string campCachedLabels[20];
    for (int i = 0; i < 20; ++i) { campCachedUnl[i] = false; campCachedStars[i] = -1; }

    // Demo unlock-all state (non-global, password-protected)
    bool demoAllUnlocked = false;
    bool demoPromptOpen = false;
    char demoPasswordBuf[33];
    demoPasswordBuf[0] = '\0';
    int demoPasswordLen = 0;
    auto submitDemoPassword = [&]() {
        if (std::strcmp(demoPasswordBuf, "lambda@fadi") == 0) {
            if (!demoAllUnlocked) {
                for (int i = 0; i < TOTAL_LEVELS; ++i) {
                    levels[i].unlocked = true;
                    campCachedUnl[i] = false;
                    campCachedLabels[i].clear();
                }
                demoAllUnlocked = true;
                toastQueue.push("[DEMO] All levels unlocked", TOAST_KIND_SUCCESS);
            } else {
                levels[0].unlocked = true;
                campCachedUnl[0] = false;
                campCachedLabels[0].clear();
                for (int i = 1; i < TOTAL_LEVELS; ++i) {
                    levels[i].unlocked = false;
                    campCachedUnl[i] = false;
                    campCachedLabels[i].clear();
                }
                demoAllUnlocked = false;
                toastQueue.push("[DEMO] Levels locked", TOAST_KIND_WARNING);
            }
        } else {
            toastQueue.push("Wrong password", TOAST_KIND_WARNING);
        }
        demoPromptOpen = false;
        demoPasswordLen = 0;
        demoPasswordBuf[0] = '\0';
    };

    int cachedHudLid = -1;
    std::string cachedHudObj;
    std::string cachedHudCtrlLine;
    int cachedHudAlarm = -1, cachedHudCap = -1, cachedHudScore = -1;
    std::string cachedHudAlarmScore;
    int cachedPromptLid = -1, cachedPromptSeq = -1, cachedPromptAvl = -1;
    int cachedPromptMode = -1, cachedPromptPost = -1, cachedPromptLca = -1;
    int cachedPromptAct2A = -9999, cachedPromptAct2B = -9999;
    int cachedPromptAct3A = -9999, cachedPromptAct3B = -9999;
    std::string cachedPromptHead;
    int cachedBriefLid = -1, cachedBriefCap = -1;
    std::string cachedBriefAct, cachedBriefObj, cachedBriefInt;

    int cachedResIdx = -1, cachedResScore = -1, cachedResAlarm = -1, cachedResBatches = -1, cachedResEvs = -1, cachedResStars = -1;
    std::string cachedResStats;

    std::string cachedAct1Stats;
    std::string cachedAct2Stats;

    int cachedLbCount = -1;
    std::string cachedLbRank[10];
    std::string cachedLbScore[10];
    std::string cachedLbAlarm[10];

    int cachedSetFpsVal = -1;
    std::string cachedSetResStr = "Display Resolution: " + std::to_string(W) + " x " + std::to_string(H);
    std::string cachedSetFpsLimStr;
    std::string cachedSetFpsBtnStr;

    int selectedNodeId = -1;
    bool pendingWin = false;
    bool showingBriefing = false;

    // Shared 1600x900 Canvas & Event Log Rectangles + Camera Views (Part B)
    const float CANVAS_X = 30.f,   CANVAS_Y = 86.f, CANVAS_W = 1140.f, CANVAS_H = 560.f;
    const float LOG_X    = 1185.f, LOG_Y    = 86.f, LOG_W    = 385.f,  LOG_H    = 560.f;

    CanvasCamera levelCam, demoCam, sbAvlCam, sbHeapCam, endlessCam;
    levelCam.init(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, static_cast<float>(W), static_cast<float>(H));
    demoCam.init(30.f, 110.f, 1140.f, 470.f, static_cast<float>(W), static_cast<float>(H));
    sbAvlCam.init(30.f, 95.f, 560.f, 540.f, static_cast<float>(W), static_cast<float>(H));
    sbHeapCam.init(605.f, 95.f, 565.f, 540.f, static_cast<float>(W), static_cast<float>(H));
    endlessCam.init(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, static_cast<float>(W), static_cast<float>(H));

    // Endless Mode state (Step F)
    EndlessSession endlessSession;
    bool endlessPaused = false;
    bool endlessShowLastResult = false;
    EndlessLeaderboardEntry endlessLastEntry;
    std::string endlessLastReason = "";
    unsigned int endlessLastSeed = 1337u;
    int cachedEndlessOps = -1, cachedEndlessNodes = -1, cachedEndlessRateTenths = -1;
    std::string cachedEndlessSub = "";
    int cachedEndlessOpType = -1, cachedEndlessOpKey = -999999;
    bool cachedEndlessPaused = false;
    std::string cachedEndlessPrompt = "";

    auto layoutBstForScreen = [&](bool isDemo) {
        if (isDemo) LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, bstLayout);
        else        LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, bstLayout);
    };
    auto layoutBtForScreen = [&](bool isDemo) {
        if (isDemo) LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, btLayout);
        else        LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, btLayout);
    };
    auto layoutAvlForScreen = [&](float ax, float ay, float aw, float ah) {
        LayoutEngine::layoutAVL(avl, ax, ay, aw, ah, avlLayout);
    };
    auto layoutHeapForScreen = [&](bool useRealHeap, float ax, float ay, float aw, float ah) {
        if (useRealHeap) {
            LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount, ax, ay, aw, ah, heapLayout);
        } else {
            LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount, ax, ay, aw, ah, heapLayout);
        }
    };

    auto resetAct2 = [&]() {
        buildBstFromData(act2Bst, levelData[5]);
        act2Hl1 = -9999;
        act2Hl2 = -9999;
        act2ExtraInfo = "";
        selectedNodeId = -1;
        pendingWin = false;
        clearMarked();
        animCtrl.clearCurrent();
        layoutBstForScreen(false);
        layoutDirty = true;
    };
    resetAct2();

    auto syncScoringHud = [&]() {
        alarmSteps = scoring.alarm();
        score = scoring.score();
    };

    auto triggerAlarmFullFail = [&]() {
        if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            levels[currentLevelIdx].stars = 0;
            pendingWin = false;
            currentScreen = SCREEN_RESULT;
            status = "ALARM FULL! Guards arrived. Level failed! Click Retry.";
            toastQueue.push("ALARM FULL! Guards arrived!", TOAST_KIND_PENALTY);
        }
    };

    auto raisePenalty = [&](int penaltyKind, const std::string& msg) {
        int added = scoring.applyPenalty(penaltyKind);
        syncScoringHud();
        status = msg;
        std::string toastMsg = "+" + std::to_string(added) + " Alarm";
        if (penaltyKind == PENALTY_WRONG_ROTATION) {
            toastMsg += ", -" + std::to_string(SCORE_ROTATION_PENALTY) + " pts";
        }
        toastMsg += ": " + msg;
        toastQueue.push(toastMsg, TOAST_KIND_PENALTY);
        if (scoring.isAlarmFull()) triggerAlarmFullFail();
    };

    auto finishLevelWin = [&](int starsAwarded, const std::string& msg) {
        if (currentLevelIdx < 0) return;
        pendingWin = false;
        levels[currentLevelIdx].stars = maxInt(levels[currentLevelIdx].stars, starsAwarded);
        if (currentLevelIdx + 1 < TOTAL_LEVELS) levels[currentLevelIdx + 1].unlocked = true;
        status = msg;

        ScoreEntry se;
        se.levelTitle = levels[currentLevelIdx].title;
        se.score = score;
        se.stars = starsAwarded;
        se.alarm = alarmSteps;
        scoreLog.append(se);

        currentScreen = SCREEN_RESULT;
        toastQueue.push("Level Complete! +" + std::to_string(score) + " pts", TOAST_KIND_SUCCESS);
    };

    auto currentHintStage = [&]() -> int {
        if (currentLevelIdx < 0) return 0;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        if (lid == LEVEL_1_1) return act1TravMode;
        if (lid == LEVEL_1_2) {
            int rootSlot = -1;
            for (int i = 0; i < btLayout.nodeCount; ++i) {
                if (btLayout.nodes[i].isRoot) { rootSlot = i; break; }
            }
            return (rootSlot >= 0 && dragDrop.tokenInSlot(rootSlot) >= 0) ? 1 : 0;
        }
        if (lid == LEVEL_1_3) return (act1PostIdx >= act1Tree.getSize()) ? 1 : 0;
        if (lid == LEVEL_1_4) return (act1LcaStage < 2) ? 0 : (act1LcaStage == 2 ? 1 : 2);
        if (lid == LEVEL_1_5) return 0;
        if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_3 || lid == LEVEL_2_4) return 0;
        if (lid == LEVEL_2_5) return act2BossStage;
        if (lid == LEVEL_3_2) return manualSubPuzzle - 1;
        if (lid == LEVEL_3_3) return (manualDoubleStep == 0) ? 0 : 1;
        if (lid == LEVEL_3_4) return avlQueuePos;
        if (lid == LEVEL_3_5) return bossQueryIdx;
        if (lid == LEVEL_4_1) {
            int firstIdx = (d.initialArrayCount / 2) - 1;
            return maxInt(0, firstIdx - buildStepIdx);
        }
        if (lid == LEVEL_4_5) return act4Built ? 1 : 0;
        return 0;
    };

    auto useHint = [&]() {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || showingBriefing) return;
        if (!hintSys.isAvailable()) {
            toastQueue.push("Hint already used on this level!", TOAST_KIND_WARNING);
            return;
        }
        int stage = currentHintStage();
        if (hintSys.use(scoring, levelData[currentLevelIdx], stage)) {
            status = "HINT (-1 star cap): " + hintSys.shownText();
            toastQueue.push("Hint used (max 2 stars this level)", TOAST_KIND_WARNING);
        }
    };

    auto startLevel = [&](int idx) {
        currentLevelIdx = idx;
        currentScreen = SCREEN_LEVEL_PLAY;
        const LevelData& d = levelData[idx];
        int opt = computeOptimalSteps(d);
        scoring.begin(opt);
        levels[idx].alarmCapacity = scoring.capacity();
        hintSys.reset();
        dragDrop.clear();
        syncScoringHud();

        sortedCount = 0;
        hlIndex1 = -1;
        hlIndex2 = -1;

        int lid = d.id;
        act1Reconstructed = false;
        act1Mirrored = false;
        act1SeqIdx = 0;
        act1PostIdx = 0;
        act1StepperVal = 1;
        for (int i = 0; i < 32; ++i) act1ConfirmedHeight[i] = 0;
        act1LcaStage = 0;
        act1LcaA = -1;
        act1LcaB = -1;
        act1RouteLen = 0;
        act1RouteIdx = 0;
        act1SwappedCount = 0;
        for (int i = 0; i < 32; ++i) act1SwappedKeys[i] = 0;
        act2TargetIdx = 0;
        act2CurKey = -9999;
        act2DelStage = 0;
        act2TwoChildPhase2Pending = false;
        act2Phase2Batch.clear();
        act2RqCount = 0;
        act2RqIdx = 0;
        act2PrunedCount = 0;
        act2LastPrunedId = -1;
        act2ReportedStr = "";
        act2BossStage = 0;
        manualSubPuzzle = 1;
        manualDoubleStep = 0;
        clicked31Mask = 0;
        act3PivotPicked = false;
        act3PivotKey = -1;
        act3NextPuzzlePending = false;
        act3LastComps = 0;
        act3LastHeight = 0;
        act3SizeOk = true;
        avlQueuePos = 0;
        bossQueryIdx = 0;
        selectedNodeId = -1;
        pendingWin = false;
        showingBriefing = true;
        clearMarked();
        toastQueue.clear();
        animCtrl.clearAll();
        undoStack.clear();
        levelCam.reset();
        layoutDirty = true;

        if (lid == LEVEL_1_1) {
            buildBinaryTreeFromData(act1Tree, d);
            act1TravMode = (act1TravCounter++) % 4;
            act1Hl1 = -9999; act1Hl2 = -9999; act1TraversalStr = "";
            status = std::string(traversalModeName(act1TravMode)) + " - click all rooms in exact order!";
        } else if (lid == LEVEL_1_2) {
            act1Tree = BinaryTree();
            act1RefTree = BinaryTree();
            act1RefTree.buildFromPreorderInorder(d.preorder, d.inorder, d.preorderCount);
            LayoutEngine::layoutBinaryTree(act1RefTree, CANVAS_X, CANVAS_Y + 75.f, CANVAS_W, CANVAS_H - 95.f, btLayout);
            layoutDirty = false;
            int nTok = btLayout.nodeCount;
            for (int i = 0; i < nTok; ++i) {
                int sId = dragDrop.addSlot(btLayout.nodes[i].x, btLayout.nodes[i].y, 26.f, DRAG_ANY_VALUE);
                dragDrop.setSlotLabel(sId, "?");
            }
            float traySpan = (nTok > 1) ? 700.f : 0.f;
            float trayStartX = CANVAS_X + CANVAS_W * 0.5f - traySpan * 0.5f;
            float trayStepX = (nTok > 1) ? (traySpan / static_cast<float>(nTok - 1)) : 0.f;
            for (int i = 0; i < d.preorderCount && i < nTok; ++i) {
                float tx = trayStartX + static_cast<float>(i) * trayStepX;
                dragDrop.addToken(d.preorder[i], tx, CANVAS_Y + 46.f, 22.f);
            }
            act1Hl1 = -9999; act1Hl2 = -9999; act1TraversalStr = "";
            status = d.startStatus;
        } else if (lid == LEVEL_1_3) {
            buildBinaryTreeFromData(act1Tree, d);
            int postCnt = 0;
            int* postSeq = act1Tree.postorderRecursive(postCnt);
            act1Hl1 = (postSeq && postCnt > 0) ? postSeq[0] : -9999;
            act1Hl2 = -9999;
            delete[] postSeq;
            act1TraversalStr = "";
            status = (act1Hl1 != -9999)
                ? ("Postorder height check [1/" + std::to_string(postCnt) + "]: enter height of highlighted room " +
                   std::to_string(act1Hl1) + " (leaf = 1) and press Submit!")
                : d.startStatus;
        } else if (lid == LEVEL_1_4 || lid == LEVEL_1_5) {
            buildBinaryTreeFromData(act1Tree, d);
            act1Hl1 = -9999; act1Hl2 = -9999; act1TraversalStr = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_1 || lid == LEVEL_2_2) {
            buildBstFromData(act2Bst, d);
            act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
            act2Hl1 = act2CurKey;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_3) {
            buildBstFromData(act2Bst, d);
            act2Hl1 = (d.deleteSequenceCount > 0) ? d.deleteSequence[0] : -9999;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_4) {
            buildBstFromData(act2Bst, d);
            buildAct2RangeDecisions(act2Bst.getRoot(), d.rangeLo, d.rangeHi, act2RqSteps, act2RqCount, 32);
            act2RqIdx = 0;
            act2Hl1 = (act2RqCount > 0) ? act2RqSteps[0].nodeKey : -9999;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_2_5) {
            buildBstFromData(act2Bst, d);
            int startCap = act2BossCapacityFor(opt, act2Bst.getHeight());
            scoring.setCapacity(startCap);
            levels[idx].alarmCapacity = scoring.capacity();
            act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
            act2Hl1 = act2CurKey;
            act2Hl2 = -9999;
            act2ExtraInfo = "";
            status = d.startStatus;
        } else if (lid == LEVEL_3_1) {
            buildUnbalancedAvlFromData(avl, d);
            status = d.startStatus;
        } else if (lid == LEVEL_3_2 || lid == LEVEL_3_3) {
            avl.clear();
            if (d.puzzleCount > 0) buildPuzzleAvl(avl, d.puzzles[0]);
            status = d.startStatus;
        } else if (lid == LEVEL_3_4) {
            avl.clear();
            for (int i = 0; i < d.initialKeyCount; ++i) avl.insert(d.initialKeys[i]);
            avlQueuePos = 0;
            if (d.insertCount > 0) {
                status = "Incoming key " + std::to_string(d.inserts[0].key) + "! " + d.startStatus;
            } else {
                status = d.startStatus;
            }
        } else if (lid == LEVEL_3_5) {
            avl.clear();
            for (int i = 0; i < d.initialKeyCount; ++i) avl.insert(d.initialKeys[i]);
            int badKey = -1;
            act3SizeOk = validateAvlSubtreeSizes(avl.getRoot(), badKey);
            act3LastHeight = avl.getHeight();
            bossQueryIdx = 0;
            status = d.startStatus;
        } else if (lid == LEVEL_4_1 || lid == LEVEL_4_2) {
            activeHeapSize = minInt(30, d.initialArrayCount);
            for (int i = 0; i < activeHeapSize; ++i) heapArr[i] = d.initialArray[i];
            buildStepIdx = findNextHeapViolation(heapArr, activeHeapSize, activeHeapSize / 2 - 1);
            hlIndex1 = buildStepIdx;
            hlIndex2 = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
            status = d.startStatus;
        } else if (lid == LEVEL_4_3 || lid == LEVEL_4_4) {
            act4Heap.buildMaxHeap(d.initialArray, d.initialArrayCount);
            act4Built = true;
            hlIndex1 = 0;
            if (lid == LEVEL_4_4) {
                status = "Steps: 0/" + std::to_string(opt) + " (" + std::to_string(opt) +
                         " extractions, O(log n) sift-down each = Theta(n log n)). Click root [0] (" +
                         std::to_string(act4Heap.getMax()) + ")!";
            } else {
                status = d.startStatus;
            }
        } else if (lid == LEVEL_4_5) {
            activeHeapSize = minInt(30, d.initialArrayCount);
            for (int i = 0; i < activeHeapSize; ++i) heapArr[i] = d.initialArray[i];
            act4Built = false;
            hlIndex1 = (activeHeapSize >= 2) ? (activeHeapSize / 2 - 1) : -1;
            status = d.startStatus;
        }
    };

    auto pushUndoSnapshot = [&](bool useRealHeap) {
        GameSnapshot snap;
        UndoStack::captureStructures(act1Tree, act2Bst, avl, act4Heap,
                                     heapArr, activeHeapSize, sortedArr, sortedCount,
                                     useRealHeap, snap);
        snap.alarmSteps = scoring.alarm();
        snap.score = scoring.score();
        snap.stepCount = scoring.steps();
        snap.penaltyCount = scoring.penaltyEvents();
        snap.penaltyPts = scoring.penaltyPoints();
        snap.bonus = scoring.bonusPoints();
        snap.hintUsed = scoring.hintWasUsed();
        snap.act1Reconstructed = act1Reconstructed;
        snap.act2Step21 = act2TargetIdx;
        snap.act2DelStage = act2DelStage;
        snap.act2BossStage = act2BossStage;
        snap.act2Clicked24Mask = act2RqIdx;
        snap.avlQueuePos = avlQueuePos;
        snap.bossQueryIdx = (currentLevelIdx >= 0 && (levelData[currentLevelIdx].id == LEVEL_3_2 || levelData[currentLevelIdx].id == LEVEL_3_3))
                                ? (manualSubPuzzle * 10 + manualDoubleStep)
                                : bossQueryIdx;
        snap.buildStepIdx = buildStepIdx;
        snap.act4Built = act4Built;
        snap.act1Hl1 = act1Hl1;
        snap.act1Hl2 = act1Hl2;
        snap.act2Hl1 = act2Hl1;
        snap.act2Hl2 = act2Hl2;
        snap.hlIndex1 = hlIndex1;
        snap.hlIndex2 = hlIndex2;
        snap.status = status;
        undoStack.push(snap);
    };

    auto performUndo = [&]() {
        GameSnapshot snap;
        if (!undoStack.pop(snap)) {
            status = "Nothing to undo!";
            toastQueue.push("Nothing to undo", TOAST_KIND_WARNING);
            return;
        }
        UndoStack::restoreStructures(snap, act1Tree, act2Bst, avl, act4Heap,
                                     heapArr, activeHeapSize, sortedArr, sortedCount);
        bool keepHint = scoring.hintWasUsed() || snap.hintUsed;
        scoring.restoreState(snap.alarmSteps, snap.stepCount, snap.penaltyCount,
                             snap.penaltyPts, snap.bonus, keepHint);
        syncScoringHud();
        act1Reconstructed = snap.act1Reconstructed;
        act2TargetIdx = snap.act2Step21;
        act2DelStage = snap.act2DelStage;
        act2BossStage = snap.act2BossStage;
        act2RqIdx = snap.act2Clicked24Mask;
        act2TwoChildPhase2Pending = false;
        act2Phase2Batch.clear();
        act3PivotPicked = false;
        act3PivotKey = -1;
        act3NextPuzzlePending = false;
        avlQueuePos = snap.avlQueuePos;
        bossQueryIdx = snap.bossQueryIdx;
        buildStepIdx = snap.buildStepIdx;
        act4Built = snap.act4Built;
        act1Hl1 = snap.act1Hl1;
        act1Hl2 = snap.act1Hl2;
        act2Hl1 = snap.act2Hl1;
        act2Hl2 = snap.act2Hl2;
        act2CurKey = (act2Hl1 != -9999) ? act2Hl1 : (act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999);
        hlIndex1 = snap.hlIndex1;
        hlIndex2 = snap.hlIndex2;
        pendingWin = false;
        selectedNodeId = -1;
        clearMarked();
        if (currentLevelIdx >= 0 && currentLevelIdx < TOTAL_LEVELS) {
            const LevelData& curD = levelData[currentLevelIdx];
            if (curD.id == LEVEL_2_4) {
                buildAct2RangeDecisions(act2Bst.getRoot(), curD.rangeLo, curD.rangeHi, act2RqSteps, act2RqCount, 32);
                act2PrunedCount = 0;
                act2LastPrunedId = -1;
                act2ReportedStr = "";
                for (int ri = 0; ri < act2RqIdx && ri < act2RqCount; ++ri) {
                    if (act2RqSteps[ri].expectedAction == RQ_ACT_REPORT) {
                        addMarkedId(act2RqSteps[ri].nodeId);
                        if (!act2ReportedStr.empty()) act2ReportedStr += ", ";
                        act2ReportedStr += std::to_string(act2RqSteps[ri].nodeKey);
                    } else {
                        const TreeNode* subRoot = findTreeNodeConst(act2Bst.getRoot(), act2RqSteps[ri].subtreeRootKey);
                        collectSubtreeKeys(subRoot, act2PrunedKeys, act2PrunedCount, 32);
                        act2LastPrunedId = act2RqSteps[ri].subtreeRootId;
                    }
                }
            } else if (curD.id == LEVEL_2_5) {
                scoring.setCapacity(act2BossCapacityFor(scoring.optimal(), act2Bst.getHeight()));
                levels[currentLevelIdx].alarmCapacity = scoring.capacity();
            } else if (curD.id == LEVEL_3_1) {
                buildUnbalancedAvlFromData(avl, curD);
            } else if (curD.id == LEVEL_3_2 || curD.id == LEVEL_3_3) {
                manualSubPuzzle = maxInt(1, snap.bossQueryIdx / 10);
                manualDoubleStep = snap.bossQueryIdx % 10;
                if (manualSubPuzzle - 1 < curD.puzzleCount) {
                    buildPuzzleAvl(avl, curD.puzzles[manualSubPuzzle - 1]);
                    if (curD.id == LEVEL_3_3 && manualDoubleStep == 2 && avl.getRoot()) {
                        AVLNode* r = avl.getRoot();
                        EventBatch dummy;
                        if (avl.balanceFactor(r) > 1 && r->left) r->left = avl.rotateLeft(r->left, dummy, ROTATION_LR);
                        else if (avl.balanceFactor(r) < -1 && r->right) r->right = avl.rotateRight(r->right, dummy, ROTATION_RL);
                        avl.updateHeightAndSize(r, dummy);
                    }
                }
            }
        }
        animCtrl.clearCurrent();
        layoutDirty = true;
        status = "Restored previous state (" + std::to_string(undoStack.getCount()) + " snapshots left).";
        toastQueue.push("Undo restored previous state", TOAST_KIND_WARNING);
    };

    auto startAct2TwoChildPhase2 = [&]() {
        if (!act2TwoChildPhase2Pending) return;
        act2TwoChildPhase2Pending = false;
        animCtrl.loadOperation(act2Phase2Batch, act2MidLayout, bstLayout, true, autoPlayEvents);
        toastQueue.push("Phase 2: Unlinking inorder successor node!", TOAST_KIND_SUCCESS);
        if (currentLevelIdx >= 0 && act2DelStage >= levelData[currentLevelIdx].deleteSequenceCount) {
            pendingWin = true;
            status = "Unlinking successor node! Completing Steal & Collapse...";
        } else if (currentLevelIdx >= 0) {
            int nextKey = levelData[currentLevelIdx].deleteSequence[act2DelStage];
            status = "Deleted 2-child node! Target " + std::to_string(act2DelStage + 1) + "/" +
                     std::to_string(levelData[currentLevelIdx].deleteSequenceCount) +
                     ": classify room " + std::to_string(nextKey) + "!";
        }
    };

    auto loadNextAct3Puzzle = [&]() {
        if (!act3NextPuzzlePending || currentLevelIdx < 0) return;
        act3NextPuzzlePending = false;
        const LevelData& curD = levelData[currentLevelIdx];
        if (manualSubPuzzle < curD.puzzleCount) {
            buildPuzzleAvl(avl, curD.puzzles[manualSubPuzzle]);
            manualSubPuzzle++;
            manualDoubleStep = 0;
            act3PivotPicked = false;
            act3PivotKey = -1;
            selectedNodeId = -1;
            animCtrl.clearCurrent();
            layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
            layoutDirty = false;
            if (curD.id == LEVEL_3_2) {
                status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(curD.puzzleCount) +
                         ": Click the unbalanced pivot node (|BF| > 1), then pick [LL] or [RR]!";
            } else {
                status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(curD.puzzleCount) +
                         ": Identify the zig-zag case ([Case: LR] or [Case: RL])!";
            }
            toastQueue.push("Loaded Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(curD.puzzleCount),
                            TOAST_KIND_SUCCESS);
        }
    };

    auto completePendingWin = [&]() {
        if (!pendingWin || currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        if (scoring.isAlarmFull()) {
            triggerAlarmFullFail();
            return;
        }
        int lid = levels[currentLevelIdx].id;
        int stars = scoring.stars();
        if (lid == LEVEL_1_1) finishLevelWin(stars, std::string(traversalModeShort(act1TravMode)) + " sequence decoded the blueprint protocol!");
        else if (lid == LEVEL_1_5) finishLevelWin(stars, "Vault mirrored! Inverted security tree verified.");
        else if (lid == LEVEL_2_1) finishLevelWin(stars, "Target key located via root-to-target BST comparisons!");
        else if (lid == LEVEL_2_2) finishLevelWin(stars, "Key 25 inserted into BST and duplicate 40 rejected!");
        else if (lid == LEVEL_2_3) finishLevelWin(stars, "Steal & Collapse complete: Leaf, One-Child, and Two-Children cases mastered!");
        else if (lid == LEVEL_2_4) finishLevelWin(stars, "Range Raid complete! Reported {" + act2ReportedStr + "} (+" + std::to_string(scoring.bonusPoints()) + " prune bonus)!");
        else if (lid == LEVEL_2_5) {
            int h = act2Bst.getHeight();
            toastQueue.push("ACT 2 CLEARED: Sorted keys caused O(n) degeneration (Height=" +
                            std::to_string(h) + ")!", TOAST_KIND_WARNING);
            toastQueue.push("Wing 3 Unlocked: AVL rotations prevent Lazy Architect O(n) chains!", TOAST_KIND_SUCCESS);
            finishLevelWin(stars,
                           "LAZY ARCHITECT EXPOSED! Sorted keys built a Height=" + std::to_string(h) +
                           " degenerate chain, turning searches into O(n) walks under a shrinking alarm. "
                           "Wing 3 (AVL Trees) is now unlocked to enforce balance and O(log n) speed!");
        }
        else if (lid == LEVEL_3_2) finishLevelWin(stars, "Single rotations (LL & RR) repaired using real AVLTree rotations!");
        else if (lid == LEVEL_3_3) finishLevelWin(stars, "Double rotations (LR & RL) repaired using real AVLTree rotations!");
        else if (lid == LEVEL_3_4) finishLevelWin(stars, "All AVL rotation types predicted & auto-balanced cleanly!");
        else if (lid == LEVEL_3_5) {
            finishLevelWin(stars,
                           "Act 3 Boss defeated! Stole k-th most valuable treasures in " +
                           std::to_string(act3LastComps) + " comparisons <= Height " +
                           std::to_string(act3LastHeight) + " (O(log n), Size Check: " +
                           std::string(act3SizeOk ? "PASS" : "FAIL") + ")! Wing 4 unlocked!");
        }
        else if (lid == LEVEL_4_1) finishLevelWin(stars, "Bottom-up Max Heap activated in O(n)!");
        else if (lid == LEVEL_4_2) finishLevelWin(stars, "Sabotaged root sifted down; Max Heap restored!");
        else if (lid == LEVEL_4_3 || lid == LEVEL_4_4 || lid == LEVEL_4_5) {
            if (!validateAct4SortedResult(levelData[currentLevelIdx], sortedArr, sortedCount)) {
                pendingWin = false;
                raisePenalty(PENALTY_ACT4_INVALID, "Sorted array mismatch against expected_sorted_result!");
                return;
            }
            if (lid == LEVEL_4_3) {
                finishLevelWin(stars, "All treasures extracted in ascending order!");
            } else if (lid == LEVEL_4_4) {
                int optS = computeOptimalSteps(levelData[currentLevelIdx]);
                finishLevelWin(stars, "Race won! Steps: " + std::to_string(scoring.steps()) + "/" +
                                      std::to_string(optS) + " (" + std::to_string(optS) +
                                      " extractions, O(log n) each = Theta(n log n))!");
            } else {
                finishLevelWin(stars, "Final Vault unlocked! In-place ascending Heap Sort complete!");
            }
        }
        else finishLevelWin(stars, "Level Complete!");
    };

    auto stepAnimForward = [&]() {
        if (animCtrl.stepForward()) {
            Event cur = animCtrl.getCurrentEvent();
            status = "Step: " + AnimationController::formatEvent(cur);
        } else if (act2TwoChildPhase2Pending && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            startAct2TwoChildPhase2();
        } else if (act3NextPuzzlePending && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            loadNextAct3Puzzle();
        } else if (pendingWin && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            completePendingWin();
        } else {
            status = "Event batch complete!";
        }
    };

    auto handleAct3SingleRotation = [&](int choice) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_3_2) return;
        if (!act3PivotPicked || act3PivotKey < 0) {
            toastQueue.push("Click the unbalanced pivot node (|BF| > 1) first!", TOAST_KIND_WARNING);
            status = "Step 1/2: Click the unbalanced pivot node (RED |BF| > 1) on the tree first!";
            return;
        }
        AVLNode* pivot = findAvlNodeMutable(avl.getRoot(), act3PivotKey);
        if (!pivot) return;
        int bf = avl.balanceFactor(pivot);
        int expectedChoice = (bf > 1 && pivot->left && avl.balanceFactor(pivot->left) >= 0) ? 1
                           : ((bf < -1 && pivot->right && avl.balanceFactor(pivot->right) <= 0) ? 2 : 0);
        if (choice != expectedChoice) {
            raisePenalty(PENALTY_WRONG_ROTATION,
                         "Wrong rotation choice! Pivot " + std::to_string(act3PivotKey) +
                         " has BF=" + std::to_string(bf) + " (pick " +
                         std::string(expectedChoice == 1 ? "[LL: Rotate Right]" : "[RR: Rotate Left]") + ").");
            return;
        }

        pushUndoSnapshot(false);
        LayoutResult before;
        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
        EventBatch batch;
        batch.append(Event(EVENT_OPERATION_STARTED, choice == 1 ? "single LL rotate right" : "single RR rotate left"));
        {
            Event alert(EVENT_BALANCE_ALERT, "balance alert");
            alert.nodeId = pivot->key;
            alert.newValue = bf;
            batch.append(alert);
        }
        AVLNode* oldParent = pivot->parent;
        bool wasLeft = (oldParent && oldParent->left == pivot);
        AVLNode* newSub = (choice == 1) ? avl.rotateRight(pivot, batch, ROTATION_LL)
                                        : avl.rotateLeft(pivot, batch, ROTATION_RR);
        if (!oldParent) {
            avl.setRoot(newSub);
        } else {
            if (wasLeft) oldParent->left = newSub;
            else oldParent->right = newSub;
            newSub->parent = oldParent;
            for (AVLNode* anc = oldParent; anc; anc = anc->parent) {
                avl.updateHeightAndSize(anc, batch);
            }
        }
        batch.append(Event(EVENT_OPERATION_COMPLETED, choice == 1 ? "LL rotation complete" : "RR rotation complete"));

        int badKey = -1;
        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
            raisePenalty(PENALTY_WRONG_ROTATION, "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
            return;
        }

        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
        scoring.addStep(1);
        syncScoringHud();
        act3PivotPicked = false;
        act3PivotKey = -1;
        selectedNodeId = -1;

        toastQueue.push(std::string(choice == 1 ? "LL (Rotate Right)" : "RR (Rotate Left)") +
                        " repaired puzzle " + std::to_string(manualSubPuzzle) + "!", TOAST_KIND_SUCCESS);
        if (scoring.isAlarmFull()) {
            triggerAlarmFullFail();
        } else if (manualSubPuzzle < d.puzzleCount) {
            act3NextPuzzlePending = true;
            status = "Rotation animated! Advancing to Puzzle " + std::to_string(manualSubPuzzle + 1) + "/" +
                     std::to_string(d.puzzleCount) + "...";
        } else {
            pendingWin = true;
            status = "Final single rotation complete! Verifying AVL balance...";
        }
    };

    auto handleAct3DoubleCase = [&](int choice) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_3_3) return;
        if (manualDoubleStep != 0) {
            toastQueue.push("Case already identified! Click the pivot node on the tree.", TOAST_KIND_WARNING);
            return;
        }
        AVLNode* r = avl.getRoot();
        int rbf = avl.balanceFactor(r);
        int expectedCase = (rbf > 1 && r && r->left && avl.balanceFactor(r->left) < 0) ? 1
                         : ((rbf < -1 && r && r->right && avl.balanceFactor(r->right) > 0) ? 2 : 0);
        if (choice != expectedCase) {
            raisePenalty(PENALTY_WRONG_ROTATION,
                         "Wrong zig-zag case! Inspect BF signs on root and heavy child (+2/-1 = LR, -2/+1 = RL).");
            return;
        }
        manualDoubleStep = 1;
        int childKey = (expectedCase == 1 && r->left) ? r->left->key
                     : ((expectedCase == 2 && r->right) ? r->right->key : -1);
        toastQueue.push(std::string("Identified ") + (expectedCase == 1 ? "LR" : "RL") +
                        " case! Now click child pivot " + std::to_string(childKey), TOAST_KIND_SUCCESS);
        status = std::string(expectedCase == 1 ? "LR" : "RL") +
                 " case confirmed! Step 2/3: Click child pivot node " + std::to_string(childKey) + " first!";
    };

    auto handleAct3Prediction = [&](const char* pred) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_3_4 || avlQueuePos >= d.insertCount) return;

        int incoming = d.inserts[avlQueuePos].key;
        const std::string& expectedRot = d.inserts[avlQueuePos].expectedRotation;
        if (expectedRot != pred) {
            raisePenalty(PENALTY_WRONG_ROTATION,
                         "Wrong prediction '" + std::string(pred) + "' for incoming key " +
                         std::to_string(incoming) + "! Check which ancestor hits |BF| = 2.");
            return;
        }

        pushUndoSnapshot(false);
        LayoutResult before;
        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
        EventBatch batch;
        avl.insert(incoming, batch);

        int badKey = -1;
        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
            raisePenalty(PENALTY_WRONG_ROTATION, "Subtree size mismatch at node " + std::to_string(badKey) + "!");
            return;
        }

        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
        scoring.addStep(1);
        scoring.addBonus(50);
        syncScoringHud();
        toastQueue.push("Correct (" + expectedRot + ")! Inserted & balanced key " + std::to_string(incoming),
                        TOAST_KIND_SUCCESS);

        avlQueuePos++;
        if (avlQueuePos >= d.insertCount) {
            pendingWin = true;
            status = "All incoming keys balanced! Playing final AVL events...";
        } else {
            status = "Inserted " + std::to_string(incoming) + "! Predict rotation for next key [" +
                     std::to_string(d.inserts[avlQueuePos].key) + "]: LL, RR, LR, RL, or None!";
        }
        if (scoring.isAlarmFull()) triggerAlarmFullFail();
    };

    auto handleAct2CompareChoice = [&](int choice) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        if (lid != LEVEL_2_1 && lid != LEVEL_2_2 && lid != LEVEL_2_5) return;

        layoutDirty = true;
        const TreeNode* curNode = findTreeNodeConst(act2Bst.getRoot(), act2CurKey);
        if (!curNode) return;

        bool isSearchMode = (lid == LEVEL_2_1) || (lid == LEVEL_2_5 && act2BossStage == 1);
        int key = isSearchMode
                      ? ((lid == LEVEL_2_1)
                             ? ((act2TargetIdx < d.targetKeyCount) ? d.targetKeys[act2TargetIdx] : -1)
                             : ((d.kQueryCount > 0) ? d.kQueries[0]
                                : (d.targetKeyCount > 0 ? d.targetKeys[d.targetKeyCount - 1] : -1)))
                      : ((act2TargetIdx < d.targetKeyCount) ? d.targetKeys[act2TargetIdx] : -1);
        if (key < 0) return;

        int expectedChoice = (key < curNode->value) ? 1 : ((key > curNode->value) ? 3 : 2);
        if (choice != expectedChoice) {
            std::string hintDir = (expectedChoice == 1)
                                      ? ("[< Go Left] (" + std::to_string(key) + " < " + std::to_string(curNode->value) + ")")
                                      : (expectedChoice == 3
                                             ? ("[Go Right >] (" + std::to_string(key) + " > " + std::to_string(curNode->value) + ")")
                                             : (isSearchMode ? "[= Found]" : "[= Reject Dup]"));
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Wrong choice at room " + std::to_string(curNode->value) + " for key " +
                         std::to_string(key) + "! Expected " + hintDir + ".");
            return;
        }

        pushUndoSnapshot(false);

        if (isSearchMode) {
            if (choice == 1 || choice == 3) {
                const TreeNode* nextNode = (choice == 1) ? curNode->left : curNode->right;
                if (!nextNode) {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL, "Target key " + std::to_string(key) + " is not in that branch!");
                    return;
                }
                addMarkedId(curNode->id);
                scoring.addStep(1);
                syncScoringHud();

                EventBatch batch;
                Event v(EVENT_VISIT, "visit");
                v.nodeId = curNode->id;
                v.newValue = curNode->value;
                batch.append(v);
                Event c(EVENT_COMPARE, (choice == 1) ? "go left" : "go right");
                c.nodeId = curNode->id;
                c.oldValue = curNode->value;
                c.newValue = key;
                batch.append(c);
                layoutBstForScreen(false);
                animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);

                act2CurKey = nextNode->value;
                act2Hl1 = act2CurKey;
                toastQueue.push("Compared " + std::to_string(key) + (choice == 1 ? " < " : " > ") +
                                std::to_string(curNode->value) + " -> room " + std::to_string(act2CurKey),
                                TOAST_KIND_SUCCESS);
                status = "At room " + std::to_string(act2CurKey) + ": compare target " + std::to_string(key) +
                         " vs " + std::to_string(act2CurKey) + " ([< Go Left], [= Found], [Go Right >])!";
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
            } else {
                addMarkedId(curNode->id);
                scoring.addStep(1);
                syncScoringHud();

                LayoutResult before;
                LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                EventBatch batch;
                act2Bst.search(key, batch);
                layoutBstForScreen(false);
                animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);

                if (lid == LEVEL_2_1) {
                    act2TargetIdx++;
                    if (act2TargetIdx < d.targetKeyCount) {
                        clearMarked();
                        act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                        act2Hl1 = act2CurKey;
                        toastQueue.push("Found target " + std::to_string(key) + "!", TOAST_KIND_SUCCESS);
                        status = "Found " + std::to_string(key) + "! Next target: " +
                                 std::to_string(d.targetKeys[act2TargetIdx]) + ". Start at root " +
                                 std::to_string(act2CurKey) + "!";
                    } else {
                        pendingWin = true;
                        toastQueue.push("Found target key " + std::to_string(key) + "!", TOAST_KIND_SUCCESS);
                        status = "Target " + std::to_string(key) + " located! Playing BST search events...";
                    }
                } else {
                    pendingWin = true;
                    toastQueue.push("Located " + std::to_string(key) + " on degenerate chain!", TOAST_KIND_SUCCESS);
                    status = "Found key " + std::to_string(key) + " on height-" +
                             std::to_string(act2Bst.getHeight()) + " degenerate chain! Finishing boss...";
                }
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
            }
            return;
        }

        // Insert mode (2.2 or 2.5 Stage 0)
        if (choice == 2) {
            addMarkedId(curNode->id);
            scoring.addStep(1);
            syncScoringHud();

            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.insert(key, batch); // rejects duplicate and emits EVENT_OPERATION_FAILED
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            toastQueue.push("Duplicate key " + std::to_string(key) + " REJECTED (no duplicates in BST)!",
                            TOAST_KIND_WARNING);

            act2TargetIdx++;
            if (act2TargetIdx >= d.targetKeyCount) {
                pendingWin = true;
                status = "Duplicate key " + std::to_string(key) + " rejected! BST invariant preserved.";
            } else {
                clearMarked();
                act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                act2Hl1 = act2CurKey;
                status = "Duplicate " + std::to_string(key) + " rejected! Next incoming key: " +
                         std::to_string(d.targetKeys[act2TargetIdx]) + " - start at root " +
                         std::to_string(act2CurKey) + "!";
            }
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
            return;
        }

        const TreeNode* nextNode = (choice == 1) ? curNode->left : curNode->right;
        if (nextNode) {
            addMarkedId(curNode->id);
            scoring.addStep(1);
            syncScoringHud();

            EventBatch batch;
            Event v(EVENT_VISIT, "visit");
            v.nodeId = curNode->id;
            v.newValue = curNode->value;
            batch.append(v);
            Event c(EVENT_COMPARE, (choice == 1) ? "go left" : "go right");
            c.nodeId = curNode->id;
            c.oldValue = curNode->value;
            c.newValue = key;
            batch.append(c);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);

            act2CurKey = nextNode->value;
            act2Hl1 = act2CurKey;
            toastQueue.push("Compared " + std::to_string(key) + (choice == 1 ? " < " : " > ") +
                            std::to_string(curNode->value) + " -> room " + std::to_string(act2CurKey),
                            TOAST_KIND_SUCCESS);
            status = "At room " + std::to_string(act2CurKey) + ": guide incoming key " + std::to_string(key) + "!";
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        } else {
            scoring.addStep(1);
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.insert(key, batch);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);

            if (lid == LEVEL_2_5) {
                int h = act2Bst.getHeight();
                int newCap = act2BossCapacityFor(scoring.optimal(), h);
                scoring.setCapacity(newCap);
                levels[currentLevelIdx].alarmCapacity = scoring.capacity();
                toastQueue.push("Inserted " + std::to_string(key) + " (Height=" + std::to_string(h) +
                                ")! Alarm cap shrank to " + std::to_string(newCap) + "!", TOAST_KIND_WARNING);
            } else {
                toastQueue.push("Inserted key " + std::to_string(key) + " under room " +
                                std::to_string(curNode->value) + "!", TOAST_KIND_SUCCESS);
            }
            syncScoringHud();

            act2TargetIdx++;
            clearMarked();
            if (act2TargetIdx >= d.targetKeyCount) {
                if (lid == LEVEL_2_5) {
                    act2BossStage = 1;
                    act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                    act2Hl1 = act2CurKey;
                    int searchKey = (d.kQueryCount > 0) ? d.kQueries[0] : key;
                    status = "Degenerate Chain built (Height=" + std::to_string(act2Bst.getHeight()) +
                             ", Cap=" + std::to_string(scoring.capacity()) +
                             ")! Now search for target " + std::to_string(searchKey) + " from root " +
                             std::to_string(act2CurKey) + "!";
                } else {
                    act2Hl1 = key;
                    pendingWin = true;
                    status = "Inserted key " + std::to_string(key) + "! Playing final BST insert events...";
                }
            } else {
                act2CurKey = act2Bst.getRoot() ? act2Bst.getRoot()->value : -9999;
                act2Hl1 = act2CurKey;
                status = "Key " + std::to_string(key) + " inserted! Incoming key " +
                         std::to_string(d.targetKeys[act2TargetIdx]) + " (" +
                         std::to_string(act2TargetIdx + 1) + "/" + std::to_string(d.targetKeyCount) +
                         ") - start at root " + std::to_string(act2CurKey) + "!";
            }
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        }
    };

    auto handleAct2DeleteCase = [&](int pickedCase) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_2_3 || act2DelStage >= d.deleteSequenceCount) return;

        layoutDirty = true;
        if (act2TwoChildPhase2Pending) startAct2TwoChildPhase2();

        int targetKey = d.deleteSequence[act2DelStage];
        const TreeNode* u = findTreeNodeConst(act2Bst.getRoot(), targetKey);
        if (!u) return;
        int trueCase = bstDeleteCase(u);
        if (pickedCase != trueCase) {
            const char* pickedName = (pickedCase == 1) ? "Leaf (0 children)"
                                   : (pickedCase == 2) ? "One-Child (1 child)"
                                                       : "Two-Children (2 children)";
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Wrong case! Room " + std::to_string(targetKey) + " is not " + pickedName + " right now.");
            return;
        }

        pushUndoSnapshot(false);
        scoring.addStep(1);
        syncScoringHud();

        if (trueCase == 1 || trueCase == 2) {
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.deleteKey(targetKey, batch);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            act2DelStage++;
            if (act2DelStage < d.deleteSequenceCount) {
                act2Hl1 = d.deleteSequence[act2DelStage];
                toastQueue.push("Deleted " + std::to_string(targetKey) + " (" +
                                (trueCase == 1 ? "Leaf" : "One-Child") + " case)!", TOAST_KIND_SUCCESS);
                status = "Deleted " + std::to_string(targetKey) + "! Target " +
                         std::to_string(act2DelStage + 1) + "/" + std::to_string(d.deleteSequenceCount) +
                         ": Room " + std::to_string(act2Hl1) + " - classify [Leaf], [One-Child], or [Two-Children]!";
            } else {
                act2Hl1 = -9999;
                pendingWin = true;
                toastQueue.push("Deleted final room " + std::to_string(targetKey) + "!", TOAST_KIND_SUCCESS);
                status = "Deleted final room " + std::to_string(targetKey) + "! Playing deletion animation...";
            }
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        } else {
            // Two-Children case: Phase 1 highlights successor + copies value; Phase 2 unlinks successor.
            const TreeNode* succ = findBstMinNodeConst(u->right);
            int uId = u->id;
            int succId = succ ? succ->id : -1;
            int succVal = succ ? succ->value : -1;
            int succChildId = (succ && succ->right) ? succ->right->id : -1;

            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);

            act2MidLayout = before;
            int uLayoutIdx = act2MidLayout.findNodeIndexById(uId);
            if (uLayoutIdx >= 0 && succ) act2MidLayout.nodes[uLayoutIdx].key = succVal;

            EventBatch batchPhase1;
            batchPhase1.append(Event(EVENT_OPERATION_STARTED, "delete 2-child"));
            {
                Event v1(EVENT_VISIT, "target node");
                v1.nodeId = uId;
                v1.newValue = targetKey;
                batchPhase1.append(v1);
            }
            if (succ) {
                Event v2(EVENT_VISIT, "inorder successor");
                v2.nodeId = succId;
                v2.newValue = succVal;
                batchPhase1.append(v2);

                Event sw(EVENT_SWAP_VALUE, "copy successor");
                sw.nodeId = uId;
                sw.otherNodeId = succId;
                sw.oldValue = targetKey;
                sw.newValue = succVal;
                batchPhase1.append(sw);
            }
            batchPhase1.append(Event(EVENT_OPERATION_COMPLETED, "successor value copied"));

            act2Phase2Batch.clear();
            act2Phase2Batch.append(Event(EVENT_OPERATION_STARTED, "unlink successor"));
            if (succ) {
                Event v3(EVENT_VISIT, "locate successor");
                v3.nodeId = succId;
                v3.newValue = succVal;
                act2Phase2Batch.append(v3);

                Event ul(EVENT_UNLINK, "unlink successor");
                ul.nodeId = succId;
                ul.otherNodeId = succChildId;
                ul.oldValue = succVal;
                act2Phase2Batch.append(ul);
            }
            act2Phase2Batch.append(Event(EVENT_OPERATION_COMPLETED, "delete 2-child complete"));

            EventBatch tmpReal;
            act2Bst.deleteKey(targetKey, tmpReal);
            layoutBstForScreen(false);

            act2DelStage++;
            act2Hl1 = (act2DelStage < d.deleteSequenceCount) ? d.deleteSequence[act2DelStage] : -9999;
            act2TwoChildPhase2Pending = true;
            animCtrl.loadOperation(batchPhase1, before, act2MidLayout, true, autoPlayEvents);
            toastQueue.push("Phase 1: Highlighting inorder successor " + std::to_string(succVal) +
                            " and swapping value into " + std::to_string(targetKey) + "!", TOAST_KIND_SUCCESS);
            status = "Two-Children Case: Phase 1 highlights successor " + std::to_string(succVal) +
                     " & swaps value; Phase 2 will unlink the successor node!";
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        }
    };

    auto handleAct2RangeDecision = [&](int actionCode) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0 || pendingWin || showingBriefing) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_2_4 || act2RqIdx >= act2RqCount) return;

        layoutDirty = true;
        const Act2RangeStep& step = act2RqSteps[act2RqIdx];
        if (actionCode != step.expectedAction) {
            std::string expStr = (step.expectedAction == RQ_ACT_REPORT)
                                     ? "Report Room"
                                     : ((step.expectedAction == RQ_ACT_PRUNE_LEFT) ? "Prune Left" : "Prune Right");
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Wrong Range decision at room " + std::to_string(step.nodeKey) +
                         " for [" + std::to_string(d.rangeLo) + ".." + std::to_string(d.rangeHi) +
                         "]! Expected [" + expStr + "].");
            return;
        }

        pushUndoSnapshot(false);
        scoring.addStep(1);

        if (actionCode == RQ_ACT_REPORT) {
            addMarkedId(step.nodeId);
            if (!act2ReportedStr.empty()) act2ReportedStr += ", ";
            act2ReportedStr += std::to_string(step.nodeKey);

            EventBatch batch;
            Event v(EVENT_VISIT, "report in-range");
            v.nodeId = step.nodeId;
            v.newValue = step.nodeKey;
            batch.append(v);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);
            toastQueue.push("Reported in-range key " + std::to_string(step.nodeKey) + "!", TOAST_KIND_SUCCESS);
        } else {
            const TreeNode* subRoot = findTreeNodeConst(act2Bst.getRoot(), step.subtreeRootKey);
            collectSubtreeKeys(subRoot, act2PrunedKeys, act2PrunedCount, 32);
            act2LastPrunedId = step.subtreeRootId;
            scoring.addBonus(50);

            EventBatch batch;
            Event p(EVENT_HIGHLIGHT_SUBTREE, "prune subtree");
            p.nodeId = step.subtreeRootId;
            p.otherNodeId = step.nodeId;
            p.newValue = step.subtreeRootKey;
            batch.append(p);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, bstLayout, bstLayout, true, autoPlayEvents);
            toastQueue.push("Pruned subtree " + std::to_string(step.subtreeRootKey) +
                            " outside [" + std::to_string(d.rangeLo) + ".." + std::to_string(d.rangeHi) +
                            "]! +50 bonus", TOAST_KIND_SUCCESS);
        }

        syncScoringHud();
        act2RqIdx++;
        if (act2RqIdx >= act2RqCount) {
            act2Hl1 = -9999;
            pendingWin = true;
            status = "Range Raid [" + std::to_string(d.rangeLo) + ".." + std::to_string(d.rangeHi) +
                     "] complete! Reported {" + act2ReportedStr + "} (+" +
                     std::to_string(scoring.bonusPoints()) + " prune bonus)!";
        } else {
            act2Hl1 = act2RqSteps[act2RqIdx].nodeKey;
            status = "At room " + std::to_string(act2Hl1) + " [" + std::to_string(act2RqIdx + 1) + "/" +
                     std::to_string(act2RqCount) + "]: reported {" +
                     (act2ReportedStr.empty() ? std::string("none") : act2ReportedStr) +
                     "} - choose [Report Room], [Prune Left], or [Prune Right]!";
        }
        if (scoring.isAlarmFull()) triggerAlarmFullFail();
    };

    auto cycleAct1TravMode = [&]() {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        if (levelData[currentLevelIdx].id != LEVEL_1_1) return;
        if (act1SeqIdx > 0 || pendingWin) {
            toastQueue.push("Cannot switch traversal mid-sequence", TOAST_KIND_WARNING);
            return;
        }
        act1TravMode = (act1TravMode + 1) % 4;
        status = std::string(traversalModeName(act1TravMode)) + " - click all rooms in exact order!";
        toastQueue.push(std::string("Protocol: ") + traversalModeShort(act1TravMode), TOAST_KIND_SUCCESS);
    };

    auto submitAct1HeightOrDiameter = [&]() {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_1_3) return;
        layoutDirty = true;

        int postCnt = 0;
        int* postSeq = act1Tree.postorderRecursive(postCnt);
        if (act1PostIdx < postCnt && postSeq) {
            int targetKey = postSeq[act1PostIdx];
            const TreeNode* node = findTreeNodeConst(act1Tree.getRoot(), targetKey);
            int trueH = treeHeightNodes(node);
            if (act1StepperVal == trueH) {
                act1ConfirmedHeight[act1PostIdx] = trueH;
                if (node) addMarkedId(node->id);
                act1PostIdx++;
                scoring.addStep(1);
                syncScoringHud();
                if (scoring.isAlarmFull()) {
                    triggerAlarmFullFail();
                } else if (act1PostIdx < postCnt) {
                    act1Hl1 = postSeq[act1PostIdx];
                    act1StepperVal = 1;
                    toastQueue.push("Height(" + std::to_string(targetKey) + ") = " + std::to_string(trueH) + " verified!",
                                    TOAST_KIND_SUCCESS);
                    status = "Room " + std::to_string(targetKey) + " height = " + std::to_string(trueH) +
                             ". Now enter height of room " + std::to_string(act1Hl1) + " [" +
                             std::to_string(act1PostIdx + 1) + "/" + std::to_string(postCnt) + "]!";
                } else {
                    act1Hl1 = act1Tree.getRoot() ? act1Tree.getRoot()->value : -9999;
                    act1StepperVal = 1;
                    toastQueue.push("All room heights verified! Now enter Diameter.", TOAST_KIND_SUCCESS);
                    status = "All room heights verified! Final step: enter the tree's Diameter (longest path in nodes) and press Submit!";
                }
            } else {
                raisePenalty(PENALTY_WRONG_TRAVERSAL,
                             "Wrong height " + std::to_string(act1StepperVal) + " for room " +
                             std::to_string(targetKey) + "! Leaf = 1, parent = 1 + max(left, right).");
            }
        } else {
            int trueDiam = act1Tree.diameter();
            if (act1StepperVal == trueDiam) {
                scoring.addStep(1);
                syncScoringHud();
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
                else finishLevelWin(scoring.stars(),
                                    "Postorder heights & tree diameter (" + std::to_string(trueDiam) + " nodes) verified!");
            } else {
                raisePenalty(PENALTY_WRONG_TRAVERSAL,
                             "Wrong diameter " + std::to_string(act1StepperVal) +
                             "! Longest path (in nodes) through the root is leftHeight + rightHeight + 1.");
            }
        }
        delete[] postSeq;
    };

    auto handleAct1BlueprintDrop = [&](const DropResult& res) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        const LevelData& d = levelData[currentLevelIdx];
        if (d.id != LEVEL_1_2 || act1Reconstructed) return;
        if (res.outcome != DROP_SNAPPED) return;

        scoring.addStep(1);
        syncScoringHud();

        int nSlots = dragDrop.slotCount();
        int filledCount = 0;
        for (int i = 0; i < nSlots; ++i) {
            if (dragDrop.tokenInSlot(i) >= 0) ++filledCount;
        }
        if (filledCount < nSlots) {
            status = "Placed token " + std::to_string(res.tokenValue) + " (" + std::to_string(filledCount) +
                     "/" + std::to_string(nSlots) + " slots filled). Keep filling all blueprint slots!";
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
            return;
        }

        BinaryTree playerTree;
        buildTreeFromSlots(btLayout, dragDrop, playerTree);
        if (playerTree.isSameTree(act1RefTree)) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act1Tree = BinaryTree();
            act1Tree.buildFromPreorderInorder(d.preorder, d.inorder, d.preorderCount, batch);
            act1Reconstructed = true;
            dragDrop.clear();
            layoutBtForScreen(false);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
            else finishLevelWin(scoring.stars(), "Blueprint decoded! Reconstructed tree matches Preorder + Inorder.");
        } else {
            for (int i = 0; i < nSlots && i < btLayout.nodeCount; ++i) {
                int tid = dragDrop.tokenInSlot(i);
                const DragToken* tok = dragDrop.token(tid);
                if (tok && tok->value != btLayout.nodes[i].key) {
                    dragDrop.returnTokenHome(tid);
                }
            }
            raisePenalty(PENALTY_WRONG_TRAVERSAL,
                         "Blueprint mismatch! Misplaced tokens returned to tray - check how Inorder splits left and right subtrees.");
        }
    };

    // Minimal non-choice structural triggers (e.g., 1.5 Auto Mirror, 4.5 Build Heap)
    auto triggerStructuralAction = [&](int code) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        layoutDirty = true;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        bool useRealHeapSnap = (lid == LEVEL_4_3 || lid == LEVEL_4_4 || (lid == LEVEL_4_5 && act4Built));
        pushUndoSnapshot(useRealHeapSnap);

        if (lid == LEVEL_1_5 && code == 1 && !act1Mirrored && !pendingWin) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            if (act1SwappedCount > 0) {
                buildBinaryTreeFromData(act1Tree, d);
            }
            EventBatch batch;
            act1Tree.mirror(batch);
            layoutBtForScreen(false);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            act1Mirrored = true;
            int rem = maxInt(1, 3 - act1SwappedCount);
            scoring.addStep(rem);
            syncScoringHud();
            pendingWin = true;
            status = "Auto Mirror triggered! Playing vault inversion events...";
            toastQueue.push("Auto Mirror running!", TOAST_KIND_SUCCESS);
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        } else if (lid == LEVEL_4_5 && code == 1 && !act4Built) {
            LayoutResult before;
            LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount,
                                          CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act4Heap.buildMaxHeap(heapArr, activeHeapSize, batch);
            act4Built = true;
            hlIndex1 = 0;
            hlIndex2 = -1;
            layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
            animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
            scoring.addStep(1);
            syncScoringHud();
            status = "Max Heap built! Phase 2: Click root [0] repeatedly to extract all treasures!";
            toastQueue.push("Max Heap built! Now click root [0] to extract", TOAST_KIND_SUCCESS);
            if (scoring.isAlarmFull()) triggerAlarmFullFail();
        }
    };

    // Node-click handler for ALL 20 campaign levels
    auto handleLevelNodeClick = [&](int clickedId, int clickedKey) {
        if (currentScreen != SCREEN_LEVEL_PLAY || currentLevelIdx < 0) return;
        layoutDirty = true;
        const LevelData& d = levelData[currentLevelIdx];
        int lid = d.id;
        selectedNodeId = clickedId;

        if (lid == LEVEL_1_1) {
            int cnt = 0;
            int* expSeq = getAct1Traversal(act1Tree, act1TravMode, cnt);
            if (expSeq && act1SeqIdx < cnt) {
                if (clickedKey == expSeq[act1SeqIdx]) {
                    addMarkedId(clickedId);
                    act1SeqIdx++;
                    scoring.addStep(1);
                    syncScoringHud();
                    toastQueue.push(std::string(traversalModeShort(act1TravMode)) + " [" +
                                    std::to_string(act1SeqIdx) + "/" + std::to_string(cnt) +
                                    "]: Visited " + std::to_string(clickedKey), TOAST_KIND_SUCCESS);
                    if (act1SeqIdx == cnt) {
                        LayoutResult before;
                        LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch batch;
                        int animCnt = 0;
                        int* res = getAct1Traversal(act1Tree, act1TravMode, animCnt, &batch);
                        delete[] res;
                        layoutBtForScreen(false);
                        animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
                        pendingWin = true;
                        status = "All " + std::to_string(cnt) + " rooms visited in " +
                                 traversalModeShort(act1TravMode) + "! Playing traversal events...";
                    } else {
                        status = "Visited " + std::to_string(clickedKey) + " (" + std::to_string(act1SeqIdx) +
                                 "/" + std::to_string(cnt) + "). Click next room in " +
                                 traversalModeShort(act1TravMode) + "!";
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Expected " + std::to_string(expSeq[act1SeqIdx]) + " next in " +
                                 traversalModeName(act1TravMode) + ", got " + std::to_string(clickedKey) + "!");
                }
            }
            delete[] expSeq;
        }
        else if (lid == LEVEL_1_2) {
            // Handled via drag-and-drop onto blueprint slots
        }
        else if (lid == LEVEL_1_3) {
            if (act1Hl1 != -9999 && clickedKey != act1Hl1) {
                raisePenalty(PENALTY_WRONG_TRAVERSAL,
                             "Postorder processes leaves/children first! Current room is " +
                             std::to_string(act1Hl1) + ".");
            } else {
                status = "Room " + std::to_string(clickedKey) + " selected. Adjust stepper [-]/[+] and press Submit [Enter]!";
            }
        }
        else if (lid == LEVEL_1_4) {
            if (act1LcaStage == 0) {
                act1LcaA = clickedKey;
                act1Hl1 = clickedKey;
                act1LcaStage = 1;
                scoring.addStep(1);
                syncScoringHud();
                toastQueue.push("Selected Room A: " + std::to_string(act1LcaA), TOAST_KIND_SUCCESS);
                status = "Room A = " + std::to_string(act1LcaA) + " selected. Now click a second room (Room B)!";
                if (scoring.isAlarmFull()) triggerAlarmFullFail();
            } else if (act1LcaStage == 1) {
                if (clickedKey == act1LcaA) {
                    toastQueue.push("Pick a different second room!", TOAST_KIND_WARNING);
                } else {
                    act1LcaB = clickedKey;
                    act1Hl2 = clickedKey;
                    buildRouteBetween(act1Tree, act1LcaA, act1LcaB, act1RoutePath, act1RouteLen);
                    act1LcaStage = 2;
                    scoring.addStep(1);
                    syncScoringHud();
                    toastQueue.push("Selected Room B: " + std::to_string(act1LcaB), TOAST_KIND_SUCCESS);
                    status = "Rooms " + std::to_string(act1LcaA) + " and " + std::to_string(act1LcaB) +
                             " selected! Step 2/3: Click their Lowest Common Ancestor (LCA)!";
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                }
            } else if (act1LcaStage == 2) {
                int expectedLca = act1Tree.lca(act1LcaA, act1LcaB);
                if (clickedKey == expectedLca) {
                    act1LcaStage = 3;
                    act1RouteIdx = 0;
                    clearMarked();
                    scoring.addStep(1);
                    syncScoringHud();
                    toastQueue.push("LCA(" + std::to_string(act1LcaA) + ", " + std::to_string(act1LcaB) +
                                    ") = " + std::to_string(expectedLca) + "!", TOAST_KIND_SUCCESS);
                    status = "LCA = " + std::to_string(expectedLca) + " verified! Step 3/3: Trace the path from " +
                             std::to_string(act1LcaA) + " to " + std::to_string(act1LcaB) +
                             " by clicking each room along it in order!";
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Room " + std::to_string(clickedKey) + " is not the LCA of " +
                                 std::to_string(act1LcaA) + " and " + std::to_string(act1LcaB) + "!");
                }
            } else if (act1LcaStage == 3 && act1RouteIdx < act1RouteLen) {
                if (act1RouteIdx == 0 && act1RouteLen > 1 && clickedKey == act1RoutePath[act1RouteLen - 1]) {
                    for (int l = 0, r = act1RouteLen - 1; l < r; ++l, --r) {
                        swapInt(act1RoutePath[l], act1RoutePath[r]);
                    }
                }
                if (clickedKey == act1RoutePath[act1RouteIdx]) {
                    addMarkedId(clickedId);
                    act1RouteIdx++;
                    scoring.addStep(1);
                    syncScoringHud();
                    if (scoring.isAlarmFull()) {
                        triggerAlarmFullFail();
                    } else if (act1RouteIdx == act1RouteLen) {
                        finishLevelWin(scoring.stars(),
                                       "Route traced between " + std::to_string(act1LcaA) + " and " +
                                       std::to_string(act1LcaB) + " through LCA " +
                                       std::to_string(act1Tree.lca(act1LcaA, act1LcaB)) + "!");
                    } else {
                        toastQueue.push("Path [" + std::to_string(act1RouteIdx) + "/" +
                                        std::to_string(act1RouteLen) + "]: " + std::to_string(clickedKey),
                                        TOAST_KIND_SUCCESS);
                        status = "Path step " + std::to_string(act1RouteIdx) + "/" + std::to_string(act1RouteLen) +
                                 ": clicked " + std::to_string(clickedKey) + ". Click next room along the route!";
                    }
                } else {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Wrong path room " + std::to_string(clickedKey) + "! Expected " +
                                 std::to_string(act1RoutePath[act1RouteIdx]) + " next along the route.");
                }
            }
        }
        else if (lid == LEVEL_1_5) {
            if (!act1Mirrored && !pendingWin) {
                TreeNode* u = findTreeNodeMutable(act1Tree.getRoot(), clickedKey);
                if (!u) return;
                if (!u->left && !u->right) {
                    if (containsInt(markedIds, markedCount, clickedId)) {
                        toastQueue.push("Leaf " + std::to_string(clickedKey) + " already checked", TOAST_KIND_WARNING);
                    } else {
                        addMarkedId(clickedId);
                        scoring.addStep(1);
                        syncScoringHud();
                        toastQueue.push("Leaf " + std::to_string(clickedKey) + " checked (no children to swap)",
                                        TOAST_KIND_SUCCESS);
                        status = "Leaf " + std::to_string(clickedKey) + " checked. Click internal nodes bottom-up to swap children!";
                        if (scoring.isAlarmFull()) triggerAlarmFullFail();
                    }
                } else if (containsInt(act1SwappedKeys, act1SwappedCount, clickedKey)) {
                    toastQueue.push("Room " + std::to_string(clickedKey) + " already swapped!", TOAST_KIND_WARNING);
                } else if (!allInternalDescendantsSwapped(u, act1SwappedKeys, act1SwappedCount)) {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Bottom-up order required! Swap lower subtrees before clicking " +
                                 std::to_string(clickedKey) + "!");
                } else {
                    pushUndoSnapshot(false);
                    LayoutResult before;
                    LayoutEngine::layoutBinaryTree(act1Tree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    TreeNode* tmp = u->left;
                    u->left = u->right;
                    u->right = tmp;
                    if (act1SwappedCount < 32) act1SwappedKeys[act1SwappedCount++] = clickedKey;
                    addMarkedId(clickedId);
                    EventBatch batch;
                    batch.append(Event(EVENT_OPERATION_STARTED, "mirror node"));
                    Event ev(EVENT_VISIT, "swap children");
                    ev.nodeId = u->id;
                    ev.newValue = u->value;
                    batch.append(ev);
                    batch.append(Event(EVENT_OPERATION_COMPLETED, "node inverted"));
                    layoutBtForScreen(false);
                    animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    if (allInternalNodesSwapped(act1Tree.getRoot(), act1SwappedKeys, act1SwappedCount)) {
                        act1Mirrored = true;
                        pendingWin = true;
                        toastQueue.push("All subtrees inverted bottom-up!", TOAST_KIND_SUCCESS);
                        status = "All subtrees inverted bottom-up! Vault mirror complete...";
                    } else {
                        toastQueue.push("Swapped children of " + std::to_string(clickedKey) + "!", TOAST_KIND_SUCCESS);
                        status = "Inverted subtree at " + std::to_string(clickedKey) +
                                 "! Continue swapping upward toward the root (or press Auto Mirror).";
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                }
            }
        }
        else if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) {
            bool isSearchMode = (lid == LEVEL_2_1) || (lid == LEVEL_2_5 && act2BossStage == 1);
            const char* midAct = isSearchMode ? "= Found" : "= Reject Dup";
            toastQueue.push(std::string("At room ") + std::to_string(act2CurKey) + ": use [< Go Left], [" +
                            midAct + "], or [Go Right >]!", TOAST_KIND_WARNING);
            status = "Current room is " + std::to_string(act2CurKey) +
                     " - use bottom comparison buttons ([< Go Left], [" + midAct + "], [Go Right >])!";
        }
        else if (lid == LEVEL_2_3) {
            int delK = (act2DelStage < d.deleteSequenceCount) ? d.deleteSequence[act2DelStage] : -1;
            toastQueue.push("Classify room " + std::to_string(delK) +
                            " via [Leaf], [One-Child], or [Two-Children]!", TOAST_KIND_WARNING);
        }
        else if (lid == LEVEL_2_4) {
            toastQueue.push("Decide at room " + std::to_string(act2Hl1) +
                            ": [Report Room], [Prune Left], or [Prune Right]!", TOAST_KIND_WARNING);
        }
        else if (lid == LEVEL_3_1) {
            int violKeys[LEVEL_MAX_NODES];
            int violCount = findUnbalancedKeys(d, violKeys, LEVEL_MAX_NODES);
            int matchIdx = -1;
            for (int i = 0; i < violCount; ++i) {
                if (violKeys[i] == clickedKey) { matchIdx = i; break; }
            }
            int fullMask = (violCount > 0 && violCount < 31) ? ((1 << violCount) - 1) : 0;
            if (matchIdx >= 0) {
                int bit = (1 << matchIdx);
                if (clicked31Mask & bit) {
                    toastQueue.push("Node " + std::to_string(clickedKey) + " already flagged!", TOAST_KIND_WARNING);
                } else {
                    addMarkedId(clickedId);
                    clicked31Mask |= bit;
                    scoring.addStep(1);
                    syncScoringHud();
                    toastQueue.push("Flagged unbalanced node " + std::to_string(clickedKey), TOAST_KIND_SUCCESS);
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                    else if (clicked31Mask == fullMask) {
                        finishLevelWin(scoring.stars(), "All |BF| > 1 violations identified!");
                    } else {
                        status = "Flagged " + std::to_string(clickedKey) + "! Click the remaining RED violation node.";
                    }
                }
            } else {
                raisePenalty(PENALTY_WRONG_TRAVERSAL, "Node " + std::to_string(clickedKey) + " is balanced (|BF| <= 1)!");
            }
        }
        else if (lid == LEVEL_3_2) {
            int pIdx = manualSubPuzzle - 1;
            if (pIdx >= 0 && pIdx < d.puzzleCount) {
                AVLNode* clickedNode = findAvlNodeMutable(avl.getRoot(), clickedKey);
                if (clickedNode) {
                    int bf = avl.balanceFactor(clickedNode);
                    if (bf > 1 || bf < -1) {
                        act3PivotPicked = true;
                        act3PivotKey = clickedKey;
                        selectedNodeId = clickedId;
                        toastQueue.push("Selected unbalanced pivot " + std::to_string(clickedKey) +
                                        " (BF=" + std::to_string(bf) + ")! Pick [LL] or [RR].",
                                        TOAST_KIND_SUCCESS);
                        status = "Pivot " + std::to_string(clickedKey) + " (BF=" + std::to_string(bf) +
                                 ") selected! Click [LL: Rotate Right (1)] or [RR: Rotate Left (2)].";
                    } else {
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     "Node " + std::to_string(clickedKey) + " is balanced (BF=" +
                                     std::to_string(bf) + ")! Click an unbalanced pivot (|BF| > 1).");
                    }
                }
            }
        }
        else if (lid == LEVEL_3_3) {
            int pIdx = manualSubPuzzle - 1;
            if (pIdx >= 0 && pIdx < d.puzzleCount) {
                if (manualDoubleStep == 0) {
                    toastQueue.push("First identify the zig-zag case: click [Case: LR] or [Case: RL]!",
                                    TOAST_KIND_WARNING);
                    status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(d.puzzleCount) +
                             ": Pick [Case: LR (1)] or [Case: RL (2)] before rotating pivots!";
                } else if (manualDoubleStep == 1) {
                    AVLNode* rootNode = avl.getRoot();
                    int rbf = avl.balanceFactor(rootNode);
                    bool isLR = (rbf > 1 && rootNode && rootNode->left && avl.balanceFactor(rootNode->left) < 0);
                    bool isRL = (rbf < -1 && rootNode && rootNode->right && avl.balanceFactor(rootNode->right) > 0);
                    AVLNode* childPivot = isLR ? rootNode->left : (isRL ? rootNode->right : nullptr);
                    if (childPivot && clickedKey == childPivot->key) {
                        pushUndoSnapshot(false);
                        LayoutResult before;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch batch;
                        batch.append(Event(EVENT_OPERATION_STARTED,
                                           isLR ? "LR step 1: rotate left child" : "RL step 1: rotate right child"));
                        {
                            Event alert(EVENT_BALANCE_ALERT, "balance alert");
                            alert.nodeId = childPivot->key;
                            alert.newValue = avl.balanceFactor(childPivot);
                            batch.append(alert);
                        }
                        AVLNode* newChild = isLR ? avl.rotateLeft(childPivot, batch, ROTATION_LR)
                                                 : avl.rotateRight(childPivot, batch, ROTATION_RL);
                        if (isLR) rootNode->left = newChild;
                        else      rootNode->right = newChild;
                        if (newChild) newChild->parent = rootNode;
                        avl.updateHeightAndSize(rootNode, batch);
                        batch.append(Event(EVENT_OPERATION_COMPLETED, "child pivot rotated"));

                        int badKey = -1;
                        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                            raisePenalty(PENALTY_WRONG_ROTATION,
                                         "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
                            return;
                        }

                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
                        manualDoubleStep = 2;
                        selectedNodeId = -1;
                        scoring.addStep(1);
                        syncScoringHud();
                        toastQueue.push("Step 1/2 rotated child " + std::to_string(clickedKey) +
                                        "! Now click root " + std::to_string(rootNode->key),
                                        TOAST_KIND_SUCCESS);
                        status = std::string(isLR ? "LR" : "RL") + " Step 3/3: Now click root pivot " +
                                 std::to_string(rootNode->key) + "!";
                        if (scoring.isAlarmFull()) triggerAlarmFullFail();
                    } else {
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     "Click heavy child pivot " +
                                     std::to_string(childPivot ? childPivot->key : d.puzzles[pIdx].childKey) +
                                     " first to straighten the zig-zag!");
                    }
                } else {
                    AVLNode* rootNode = avl.getRoot();
                    if (rootNode && clickedKey == rootNode->key) {
                        int rbf = avl.balanceFactor(rootNode);
                        bool isLR = (rbf > 1);
                        pushUndoSnapshot(false);
                        LayoutResult before;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch batch;
                        batch.append(Event(EVENT_OPERATION_STARTED,
                                           isLR ? "LR step 2: rotate right root" : "RL step 2: rotate left root"));
                        AVLNode* newRoot = isLR ? avl.rotateRight(rootNode, batch, ROTATION_LR)
                                                : avl.rotateLeft(rootNode, batch, ROTATION_RL);
                        avl.setRoot(newRoot);
                        batch.append(Event(EVENT_OPERATION_COMPLETED, "double rotation complete"));

                        int badKey = -1;
                        if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                            raisePenalty(PENALTY_WRONG_ROTATION,
                                         "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
                            return;
                        }

                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
                        scoring.addStep(1);
                        syncScoringHud();
                        selectedNodeId = -1;
                        if (scoring.isAlarmFull()) {
                            triggerAlarmFullFail();
                        } else if (manualSubPuzzle < d.puzzleCount) {
                            act3NextPuzzlePending = true;
                            toastQueue.push("Double Rotation complete! Advancing to Puzzle " +
                                            std::to_string(manualSubPuzzle + 1), TOAST_KIND_SUCCESS);
                            status = "Double rotation animated! Loading next zig-zag puzzle...";
                        } else {
                            pendingWin = true;
                            status = "Final double rotation complete! Verifying balanced AVL tree...";
                        }
                    } else {
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     "Step 3/3: Click root pivot " +
                                     std::to_string(rootNode ? rootNode->key : d.puzzles[pIdx].rootKey) +
                                     " to complete double rotation!");
                    }
                }
            }
        }
        else if (lid == LEVEL_3_4) {
            if (avlQueuePos < d.insertCount) {
                int incoming = d.inserts[avlQueuePos].key;
                toastQueue.push("Predict the rotation for key " + std::to_string(incoming) +
                                " using [LL], [RR], [LR], [RL], or [None]!", TOAST_KIND_WARNING);
                status = "Use the bottom prediction buttons [LL] [RR] [LR] [RL] [None] for incoming key " +
                         std::to_string(incoming) + "!";
            }
        }
        else if (lid == LEVEL_3_5) {
            if (bossQueryIdx < d.kQueryCount) {
                int badKey = -1;
                if (!validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                    act3SizeOk = false;
                    raisePenalty(PENALTY_WRONG_ROTATION,
                                 "Subtree size invariant broken at node " + std::to_string(badKey) + "!");
                    return;
                }
                int k = d.kQueries[bossQueryIdx];
                int comps = 0;
                int expectedKey = kthLargestAvl(avl, k, comps);
                if (clickedKey == expectedKey) {
                    pushUndoSnapshot(false);
                    int hBefore = avl.getHeight();
                    act3LastComps = comps;
                    act3LastHeight = hBefore;
                    scoring.addStep(1);
                    scoring.addBonus(50);
                    syncScoringHud();
                    bossQueryIdx++;
                    if (scoring.isAlarmFull()) {
                        triggerAlarmFullFail();
                    } else if (bossQueryIdx >= d.kQueryCount) {
                        LayoutResult sameLayout;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, sameLayout);
                        EventBatch qBatch;
                        int dummyC = 0;
                        kthLargestAvl(avl, k, dummyC, &qBatch);
                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(qBatch, sameLayout, avlLayout, true, autoPlayEvents);
                        toastQueue.push("Stole #" + std::to_string(k) + " largest=" + std::to_string(expectedKey) +
                                        " in " + std::to_string(comps) + " comps <= H=" +
                                        std::to_string(hBefore) + " (sz OK)!", TOAST_KIND_SUCCESS);
                        pendingWin = true;
                        status = "Final #" + std::to_string(k) + " largest=" + std::to_string(expectedKey) +
                                 " verified in " + std::to_string(comps) + " comps <= H=" +
                                 std::to_string(hBefore) + "! Completing Boss...";
                    } else {
                        LayoutResult before;
                        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                        EventBatch rotBatch;
                        if (!avl.search(22)) {
                            avl.insert(22, rotBatch);
                        } else {
                            int dummyC = 0;
                            kthLargestAvl(avl, k, dummyC, &rotBatch);
                        }
                        int postBad = -1;
                        act3SizeOk = validateAvlSubtreeSizes(avl.getRoot(), postBad);
                        if (!act3SizeOk) {
                            raisePenalty(PENALTY_WRONG_ROTATION,
                                         "Subtree size mismatch after security rotation at " +
                                         std::to_string(postBad) + "!");
                            return;
                        }
                        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                        animCtrl.loadOperation(rotBatch, before, avlLayout, true, autoPlayEvents);
                        toastQueue.push("Stole #" + std::to_string(k) + " largest=" + std::to_string(expectedKey) +
                                        " (" + std::to_string(comps) + " comps <= H=" +
                                        std::to_string(hBefore) + ")! Security inserted 22 + rotated (sz OK)!",
                                        TOAST_KIND_SUCCESS);
                        status = "Last query: " + std::to_string(comps) + " comps <= H=" +
                                 std::to_string(hBefore) + " (sz OK). Security added 22 & rotated -> Now click #" +
                                 std::to_string(d.kQueries[bossQueryIdx]) + " MOST VALUABLE!";
                    }
                } else {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Node " + std::to_string(clickedKey) + " is not the #" + std::to_string(k) +
                                 " MOST VALUABLE (largest) key! Compare right subtree sz.");
                }
            }
        }
        else if (lid == LEVEL_4_1) {
            if (buildStepIdx >= 0) {
                int partner = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
                if (clickedId == buildStepIdx || clickedId == partner) {
                    pushUndoSnapshot(false);
                    LayoutResult before;
                    LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount,
                                                  CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    int doneIdx = buildStepIdx;
                    siftDownHeapArrayWithEvents(heapArr, activeHeapSize, doneIdx, batch);
                    buildStepIdx = findNextHeapViolation(heapArr, activeHeapSize, doneIdx - 1);
                    hlIndex1 = buildStepIdx;
                    hlIndex2 = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
                    scoring.addStep(1);
                    syncScoringHud();
                    if (buildStepIdx >= 0) {
                        toastQueue.push("Sifted down [" + std::to_string(doneIdx) + "]!", TOAST_KIND_SUCCESS);
                        status = "Next: Click node [" + std::to_string(buildStepIdx) + "] (val " +
                                 std::to_string(heapArr[buildStepIdx]) + ") or [" + std::to_string(hlIndex2) +
                                 "] (val " + std::to_string(heapArr[hlIndex2]) + ") to sift down!";
                    } else {
                        pendingWin = true;
                        toastQueue.push("Root sifted down! Valid Max Heap built", TOAST_KIND_SUCCESS);
                        status = "Bottom-up build complete! Playing final swaps...";
                    }
                    layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    raisePenalty(PENALTY_ACT4_INVALID,
                                 "Sift down node [" + std::to_string(buildStepIdx) + "] next (bottom-up order)!");
                }
            }
        }
        else if (lid == LEVEL_4_2) {
            if (!pendingWin && buildStepIdx >= 0) {
                int partner = heapLargerChildIdx(heapArr, activeHeapSize, buildStepIdx);
                if (clickedId == buildStepIdx || clickedId == partner) {
                    pushUndoSnapshot(false);
                    LayoutResult before;
                    LayoutEngine::layoutHeapArray(heapArr, activeHeapSize, sortedArr, sortedCount,
                                                  CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    siftDownHeapArrayWithEvents(heapArr, activeHeapSize, buildStepIdx, batch);
                    layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    pendingWin = true;
                    buildStepIdx = -1;
                    hlIndex1 = -1; hlIndex2 = -1;
                    toastQueue.push("Sabotaged node sifted down! Heap valid", TOAST_KIND_SUCCESS);
                    status = "Sabotaged node sifted down! Playing repair animation...";
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    raisePenalty(PENALTY_ACT4_INVALID,
                                 "Click sabotaged node [" + std::to_string(buildStepIdx) + "] (or child [" +
                                 std::to_string(partner) + "]) to repair!");
                }
            }
        }
        else if (lid == LEVEL_4_3 || lid == LEVEL_4_4) {
            if (!act4Heap.isEmpty()) {
                if (clickedId == 0) {
                    pushUndoSnapshot(true);
                    LayoutResult before;
                    LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount,
                                                CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    int maxVal = act4Heap.extractMax(batch);
                    insertExtractedMaxIntoSortedTail(sortedArr, sortedCount, maxVal);
                    layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    int optS = computeOptimalSteps(d);
                    if (act4Heap.isEmpty()) {
                        if (!validateAct4SortedResult(d, sortedArr, sortedCount)) {
                            pendingWin = false;
                            raisePenalty(PENALTY_ACT4_INVALID, "Sorted region does not match expected ascending order!");
                            status = "ERROR: Sorted array mismatch against expected_sorted_result!";
                        } else {
                            toastQueue.push("Extracted max " + std::to_string(maxVal) + " (ascending order verified)!",
                                            TOAST_KIND_SUCCESS);
                            pendingWin = true;
                            if (lid == LEVEL_4_4) {
                                status = "All " + std::to_string(optS) + " extracted! Steps: " +
                                         std::to_string(scoring.steps()) + "/" + std::to_string(optS) +
                                         " (" + std::to_string(optS) + " extractions, O(log n) each = Theta(n log n))!";
                            } else {
                                status = "Extracted final treasure " + std::to_string(maxVal) + "! Ascending sorted array verified.";
                            }
                        }
                    } else {
                        toastQueue.push("Extracted max " + std::to_string(maxVal) + " to sorted tail!", TOAST_KIND_SUCCESS);
                        if (lid == LEVEL_4_4) {
                            status = "Extracted " + std::to_string(maxVal) + " | Steps: " +
                                     std::to_string(scoring.steps()) + "/" + std::to_string(optS) +
                                     " (" + std::to_string(optS) + " extractions, O(log n) each = Theta(n log n)). Next root [0]=" +
                                     std::to_string(act4Heap.getMax()) + "!";
                        } else {
                            status = "Extracted " + std::to_string(maxVal) + "! Click root [0] (" +
                                     std::to_string(act4Heap.getMax()) + ") to extract next.";
                        }
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    raisePenalty(PENALTY_ACT4_INVALID, "Max Heap extraction always removes root [0]!");
                }
            }
        }
        else if (lid == LEVEL_4_5) {
            int buildStart = (activeHeapSize >= 2) ? (activeHeapSize / 2 - 1) : 0;
            if (!act4Built) {
                if (clickedId == buildStart) triggerStructuralAction(1);
                else toastQueue.push("Click [Build Max Heap [B]] or node [" + std::to_string(buildStart) + "] first",
                                     TOAST_KIND_WARNING);
            } else if (!act4Heap.isEmpty()) {
                if (clickedId == 0) {
                    pushUndoSnapshot(true);
                    LayoutResult before;
                    LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount,
                                                CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    int maxVal = act4Heap.extractMax(batch);
                    insertExtractedMaxIntoSortedTail(sortedArr, sortedCount, maxVal);
                    layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    if (act4Heap.isEmpty()) {
                        if (!validateAct4SortedResult(d, sortedArr, sortedCount)) {
                            pendingWin = false;
                            raisePenalty(PENALTY_ACT4_INVALID, "Sorted region does not match expected ascending order!");
                            status = "ERROR: Sorted array mismatch against expected_sorted_result!";
                        } else {
                            toastQueue.push("In-place ascending Heap Sort verified!", TOAST_KIND_SUCCESS);
                            pendingWin = true;
                            status = "All treasures heap-sorted ascending! Unlocking final vault...";
                        }
                    } else {
                        toastQueue.push("Extracted " + std::to_string(maxVal) + " to ascending tail", TOAST_KIND_SUCCESS);
                        status = "Extracted " + std::to_string(maxVal) + ". Click root [0] (" +
                                 std::to_string(act4Heap.getMax()) + ") next!";
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    raisePenalty(PENALTY_ACT4_INVALID, "Click root [0] to extract the next maximum!");
                }
            }
        }
    };

    // Explorer Demos (Act 1 & Act 2)
    auto triggerAct1Action = [&](int actionCode) {
        layoutDirty = true;
        if (actionCode == 1) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            int cPre = 0, cIn = 0, cPost = 0, cLvl = 0;
            int* pre = act1Tree.preorderIterative(cPre);
            int* in  = act1Tree.inorderIterative(cIn, batch);
            int* post = act1Tree.postorderIterative(cPost);
            int* lvl = act1Tree.levelOrder(cLvl);
            layoutBtForScreen(true);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);

            std::string preStr = "Pre: ", inStr = "In: ", postStr = "Post: ", lvlStr = "Level: ";
            for (int i = 0; i < cPre; ++i) preStr += std::to_string(pre[i]) + " ";
            for (int i = 0; i < cIn; ++i) inStr += std::to_string(in[i]) + " ";
            for (int i = 0; i < cPost; ++i) postStr += std::to_string(post[i]) + " ";
            for (int i = 0; i < cLvl; ++i) lvlStr += std::to_string(lvl[i]) + " ";
            act1TraversalStr = preStr + " | " + inStr + "\n" + postStr + " | " + lvlStr;
            status = "Traversals computed! Watch Inorder events in the log.";
            delete[] pre; delete[] in; delete[] post; delete[] lvl;
        } else if (actionCode == 2) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            act1Tree.mirror(batch);
            layoutBtForScreen(true);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            act1Hl1 = -9999; act1Hl2 = -9999;
            status = "Tree mirrored! Symmetric: " + std::string(act1Tree.isSymmetric() ? "YES" : "NO");
        } else if (actionCode == 3) {
            LayoutResult before;
            LayoutEngine::layoutBinaryTree(act1Tree, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            int preArr[] = { 10, 5, 2, 7, 15, 12, 20 };
            int inArr[]  = { 2, 5, 7, 10, 12, 15, 20 };
            act1Tree.buildFromPreorderInorder(preArr, inArr, 7, batch);
            layoutBtForScreen(true);
            animCtrl.loadOperation(batch, before, btLayout, true, autoPlayEvents);
            act1Hl1 = 10; act1Hl2 = -9999;
            status = "Reconstructed tree from Preorder + Inorder!";
        } else if (actionCode == 4) {
            status = "Height = " + std::to_string(act1Tree.height()) + " (leaf=1)  |  Diameter = " + std::to_string(act1Tree.diameter()) + " nodes";
        } else if (actionCode == 5) {
            if (!act1Tree.getRoot()) { status = "Tree is empty!"; return; }
            int a = 20, b = 40;
            int lcaVal = act1Tree.lca(a, b);
            if (lcaVal == -1) {
                a = act1Tree.getRoot()->value;
                b = act1Tree.getRoot()->left ? act1Tree.getRoot()->left->value : a;
                lcaVal = act1Tree.lca(a, b);
            }
            act1Hl1 = a; act1Hl2 = b;
            status = "LCA(" + std::to_string(a) + ", " + std::to_string(b) + ") = " + std::to_string(lcaVal);
        } else if (actionCode == 6) {
            act1Tree = BinaryTree();
            auto r1 = act1Tree.addNode(1, nullptr, true);
            auto r2L = act1Tree.addNode(2, r1, true);
            auto r2R = act1Tree.addNode(2, r1, false);
            act1Tree.addNode(3, r2L, true);
            act1Tree.addNode(4, r2L, false);
            act1Tree.addNode(4, r2R, true);
            act1Tree.addNode(3, r2R, false);
            animCtrl.clearCurrent();
            act1Hl1 = -9999; act1Hl2 = -9999;
            status = "Loaded Symmetric Binary Tree! isSymmetric() = " + std::string(act1Tree.isSymmetric() ? "TRUE" : "FALSE");
        } else if (actionCode == 7) {
            resetAct1();
            animCtrl.clearCurrent();
            status = "Reset Act 1 General Binary Tree.";
        }
    };

    auto triggerAct2Action = [&](int actionCode) {
        layoutDirty = true;
        if (actionCode == 1) {
            int candidates[] = { 35, 65, 85, 25, 45, 50, 15 };
            static int insStep = 0;
            int key = candidates[insStep++ % 7];
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            bool ok = act2Bst.insert(key, batch);
            layoutBstForScreen(true);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            act2Hl1 = key; act2Hl2 = -9999;
            status = ok ? ("Inserted key " + std::to_string(key) + " | ValidBST=" + (act2Bst.isValidBST() ? "YES" : "NO"))
                        : ("Duplicate key " + std::to_string(key) + " rejected!");
        } else if (actionCode == 2) {
            static int delStep = 0;
            int delKey = -1;
            if (delStep == 0 && act2Bst.search(80)) delKey = 80;
            else if (delStep == 1 && act2Bst.search(20)) delKey = 20;
            else if (delStep == 2 && act2Bst.search(30)) delKey = 30;
            else delKey = act2Bst.findMin();
            delStep = (delStep + 1) % 3;
            if (delKey != -1) {
                LayoutResult before;
                LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
                EventBatch batch;
                act2Bst.deleteKey(delKey, batch);
                layoutBstForScreen(true);
                animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
                act2Hl1 = -9999; act2Hl2 = -9999;
                status = "Deleted key " + std::to_string(delKey) + " | Size=" + std::to_string(act2Bst.getSize());
            }
        } else if (actionCode == 3) {
            int searchKeys[] = { 40, 99, 70, 25, 88 };
            static int sIdx = 0;
            int sk = searchKeys[sIdx++ % 5];
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            bool found = act2Bst.search(sk, batch);
            layoutBstForScreen(true);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            act2Hl1 = found ? sk : -9999;
            status = "search(" + std::to_string(sk) + ") = " + (found ? "FOUND" : "NOT FOUND");
        } else if (actionCode == 4) {
            act2Hl1 = act2Bst.findMin();
            act2Hl2 = act2Bst.findMax();
            status = "Min = " + std::to_string(act2Hl1) + "  |  Max = " + std::to_string(act2Hl2);
        } else if (actionCode == 5) {
            int target = 40;
            if (!act2Bst.search(target) && act2Bst.getRoot()) target = act2Bst.getRoot()->value;
            if (target != -1) {
                int sc = act2Bst.successor(target);
                int pd = act2Bst.predecessor(target);
                act2Hl1 = target;
                status = "Node " + std::to_string(target) + " -> Pred=" + (pd != -1 ? std::to_string(pd) : "None") +
                         " | Succ=" + (sc != -1 ? std::to_string(sc) : "None");
            }
        } else if (actionCode == 6) {
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
            EventBatch batch;
            int count = 0;
            int* rq = act2Bst.rangeQuery(25, 75, count, batch);
            layoutBstForScreen(true);
            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
            std::string rqStr = "Range [25..75]: ";
            if (rq) {
                for (int i = 0; i < count; ++i) rqStr += std::to_string(rq[i]) + (i + 1 < count ? ", " : "");
                delete[] rq;
            }
            status = rqStr;
            act2ExtraInfo = rqStr;
        } else if (actionCode == 7) {
            static int k = 1;
            if (k > act2Bst.getSize()) k = 1;
            int ans = act2Bst.kthSmallest(k);
            act2Hl1 = ans;
            status = "kthSmallest(k=" + std::to_string(k) + ") = " + std::to_string(ans);
            k++;
        } else if (actionCode == 8) {
            act2Bst = BST();
            for (int i = 1; i <= 7; ++i) act2Bst.insert(i * 10);
            act2Hl1 = -9999; act2Hl2 = -9999;
            animCtrl.clearCurrent();
            layoutBstForScreen(true);
            status = "Degenerate Chain (10..70): Height=" + std::to_string(act2Bst.getHeight()) + " == Size=" + std::to_string(act2Bst.getSize());
        } else if (actionCode == 9) {
            resetAct2();
            layoutBstForScreen(true);
            status = "Reset Act 2 BST.";
        }
    };

    // Playback Slider / Scrubber helpers
    bool draggingSpeedSlider = false;
    bool draggingScrubber = false;
    float sliderPanelX = LOG_X, sliderPanelW = LOG_W;
    auto updateSpeedFromMouse = [&](float mxPos, float panelX, float panelW) {
        float trackX = panelX + 94.f;
        float trackW = panelW - 108.f;
        if (trackW > 240.f) trackW = 240.f;
        float norm = clampFloat((mxPos - trackX) / trackW, 0.f, 1.f);
        animCtrl.setSpeed(0.25f + norm * 2.75f);
    };
    auto updateScrubberFromMouse = [&](float mxPos, float panelX, float panelW) {
        int total = animCtrl.getEventCount();
        if (total <= 0) return;
        float scrubX = panelX + 90.f;
        float scrubW = panelW - 104.f;
        if (scrubW < 40.f) scrubW = 40.f;
        float norm = clampFloat((mxPos - scrubX) / scrubW, 0.f, 1.f);
        int targetIdx = (total == 1) ? 0 : static_cast<int>(norm * static_cast<float>(total - 1) + 0.5f);
        animCtrl.seekToEvent(targetIdx);
        status = "Scrubbed: " + AnimationController::formatEvent(animCtrl.getCurrentEvent());
    };
    auto handlePlaybackControlsClick = [&](float mxPos, float myPos, float panelX, float panelY, float panelW, float panelH) -> bool {
        float row0Y = panelY + panelH - 96.f;
        float row1Y = panelY + panelH - 64.f;
        float row2Y = panelY + panelH - 30.f;
        float btnW = (panelW - 32.f) / 3.f;
        if (btnW > 116.f) btnW = 116.f;

        if (Ui::inside(mxPos, myPos, panelX + 8.f, row0Y, 72.f, 24.f)) {
            animCtrl.replayLastBatch();
            status = "Replaying operation events...";
            return true;
        }
        float scrubX = panelX + 90.f;
        float scrubW = panelW - 104.f;
        if (scrubW < 40.f) scrubW = 40.f;
        if (Ui::inside(mxPos, myPos, scrubX - 6.f, row0Y, scrubW + 12.f, 24.f)) {
            draggingScrubber = true;
            sliderPanelX = panelX;
            sliderPanelW = panelW;
            updateScrubberFromMouse(mxPos, panelX, panelW);
            return true;
        }
        if (Ui::inside(mxPos, myPos, panelX + 8.f, row1Y, btnW, 25.f)) {
            animCtrl.togglePlayPause();
            return true;
        }
        if (Ui::inside(mxPos, myPos, panelX + 16.f + btnW, row1Y, btnW, 25.f)) {
            animCtrl.setPlaying(false);
            stepAnimForward();
            return true;
        }
        if (Ui::inside(mxPos, myPos, panelX + 24.f + 2.f * btnW, row1Y, btnW, 25.f)) {
            animCtrl.finishInstant();
            if (act2TwoChildPhase2Pending) {
                startAct2TwoChildPhase2();
                animCtrl.finishInstant();
            }
            if (act3NextPuzzlePending) {
                loadNextAct3Puzzle();
                animCtrl.finishInstant();
            }
            if (pendingWin) completePendingWin();
            return true;
        }
        float trackX = panelX + 94.f;
        float trackW = panelW - 108.f;
        if (trackW > 240.f) trackW = 240.f;
        if (Ui::inside(mxPos, myPos, trackX - 8.f, row2Y, trackW + 16.f, 24.f)) {
            draggingSpeedSlider = true;
            sliderPanelX = panelX;
            sliderPanelW = panelW;
            updateSpeedFromMouse(mxPos, panelX, panelW);
            return true;
        }
        return false;
    };

    // Sandbox actions
    auto enterSandbox = [&]() {
        currentScreen = SCREEN_SANDBOX;
        sbAvlCam.reset();
        sbHeapCam.reset();
        layoutDirty = true;
        avl = AVLTree();
        int init[] = { 40, 20, 60, 10, 30, 50, 70 };
        for (int v : init) avl.insert(v);
        int hInit[] = { 90, 75, 80, 60, 50, 40, 70 };
        act4Heap.buildMaxHeap(hInit, 7);
        sortedCount = 0;
        animCtrl.clearCurrent();
        undoStack.clear();
        status = "Sandbox: Real AVLTree + MaxHeap with live EventLog & Undo.";
    };
    auto triggerSandboxAction = [&](int code) {
        layoutDirty = true;
        if (code == 1) {
            static int sbInsIdx = 0;
            int vals[] = { 15, 25, 35, 55, 65, 75, 5, 85 };
            int key = vals[sbInsIdx++ % 8];
            pushUndoSnapshot(true);
            LayoutResult before;
            LayoutEngine::layoutAVL(avl, 30.f, 95.f, 560.f, 540.f, before);
            EventBatch batch;
            bool ok = avl.insert(key, batch);
            layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
            animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
            status = ok ? ("Sandbox AVL Insert(" + std::to_string(key) + ")! Height=" + std::to_string(avl.getHeight()))
                        : ("Sandbox AVL Duplicate(" + std::to_string(key) + ") rejected!");
            toastQueue.push(status, ok ? TOAST_KIND_SUCCESS : TOAST_KIND_WARNING);
        } else if (code == 2 && !avl.isEmpty()) {
            int key = avl.findMin();
            pushUndoSnapshot(true);
            LayoutResult before;
            LayoutEngine::layoutAVL(avl, 30.f, 95.f, 560.f, 540.f, before);
            EventBatch batch;
            avl.deleteKey(key, batch);
            layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
            animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
            status = "Sandbox AVL Deleted min key " + std::to_string(key) + "!";
            toastQueue.push(status, TOAST_KIND_SUCCESS);
        } else if (code == 3 && !avl.isEmpty()) {
            static int sbK = 1;
            if (sbK > avl.getSize()) sbK = 1;
            int val = avl.kth(sbK);
            status = "Sandbox AVL kth(" + std::to_string(sbK) + ") = " + std::to_string(val);
            toastQueue.push(status, TOAST_KIND_SUCCESS);
            sbK++;
        } else if (code == 4 && !act4Heap.isEmpty()) {
            pushUndoSnapshot(true);
            LayoutResult before;
            LayoutEngine::layoutMaxHeap(act4Heap, sortedArr, sortedCount, 605.f, 95.f, 565.f, 540.f, before);
            EventBatch batch;
            int mxVal = act4Heap.extractMax(batch);
            insertExtractedMaxIntoSortedTail(sortedArr, sortedCount, mxVal);
            layoutHeapForScreen(true, 605.f, 95.f, 565.f, 540.f);
            animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
            status = "Sandbox Heap Extracted max=" + std::to_string(mxVal) + "!";
            toastQueue.push(status, TOAST_KIND_SUCCESS);
        } else if (code == 5) {
            enterSandbox();
            toastQueue.push("Sandbox reset!", TOAST_KIND_WARNING);
        }
    };

    // Endless Mode actions (Step F)
    auto startEndlessMode = [&](unsigned int seed) {
        currentScreen = SCREEN_ENDLESS;
        endlessCam.reset();
        endlessPaused = false;
        endlessShowLastResult = false;
        endlessLastReason = "";
        if (seed == 0u) seed = static_cast<unsigned int>(std::time(nullptr)) ^ 0xA5A55A5Au;
        if (seed == 0u) seed = 1u;
        endlessLastSeed = seed;
        endlessSession.startNew(seed, avl);
        animCtrl.clearAll();
        undoStack.clear();
        selectedNodeId = -1;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        layoutDirty = false;
        cachedEndlessOps = -1;
        cachedEndlessOpType = -1;
        const EndlessOp& op = endlessSession.getPendingOp();
        status = "Seed #" + std::to_string(endlessSession.getInitialSeed()) +
                 " started! Click [Execute] to apply " +
                 std::string(op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                 std::to_string(op.key) + " before the deadline expires!";
        toastQueue.push("Endless Mode active! Execute ops before deadline shrinks", TOAST_KIND_SUCCESS);
    };
    auto finishEndlessGameOver = [&]() {
        endlessSession.stop();
        endlessPaused = false;
        endlessShowLastResult = true;
        EndlessLeaderboardEntry rec;
        rec.nameDate = currentDateTag();
        rec.survivalMs = endlessSession.getSurvivalMs();
        rec.opsCompleted = endlessSession.getOpsCompleted();
        rec.seed = endlessSession.getInitialSeed();
        endlessLastEntry = rec;
        endlessLastReason = endlessSession.getGameOverReason();
        endlessLb.recordRun(rec);
        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
        status = endlessLastReason + " Survived " + EndlessLeaderboard::formatTimeMs(rec.survivalMs) +
                 " (" + std::to_string(rec.opsCompleted) + " ops).";
        toastQueue.push("Game Over! Survived " + EndlessLeaderboard::formatTimeMs(rec.survivalMs),
                        TOAST_KIND_WARNING);
    };
    auto triggerEndlessExecute = [&]() {
        if (currentScreen != SCREEN_ENDLESS || !endlessSession.isActive() || endlessPaused) return;
        const EndlessOp applied = endlessSession.getPendingOp();
        LayoutResult before;
        LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
        EventBatch batch;
        bool ok = endlessSession.executePending(avl, batch);
        int badKey = -1;
        bool szOk = validateAvlSubtreeSizes(avl.getRoot(), badKey);
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        layoutDirty = false;
        animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
        if (!ok || !szOk) {
            toastQueue.push("AVL invariant check failed!", TOAST_KIND_PENALTY);
            finishEndlessGameOver();
            return;
        }
        const EndlessOp& next = endlessSession.getPendingOp();
        std::string doneWord = (applied.type == ENDLESS_OP_INSERT) ? "INSERT " : "DELETE ";
        std::string nextWord = (next.type == ENDLESS_OP_INSERT) ? "INSERT " : "DELETE ";
        status = "Executed " + doneWord + std::to_string(applied.key) +
                 " (AVL valid, H=" + std::to_string(avl.getHeight()) +
                 ")! Next: " + nextWord + std::to_string(next.key);
        toastQueue.push(doneWord + std::to_string(applied.key) + " -> AVL valid!", TOAST_KIND_SUCCESS);
    };
    auto saveEndlessRun = [&](const std::string& pathOverride = "") {
        std::string target = pathOverride.empty() ? endlessSavePath : pathOverride;
        if (endlessSession.saveToFile(target, avl)) {
            status = "Saved Endless run (Seed=" + std::to_string(endlessSession.getInitialSeed()) +
                     ", Time=" + EndlessLeaderboard::formatTimeMs(endlessSession.getSurvivalMs()) +
                     ", Preorder+null AVL persisted).";
            toastQueue.push("Endless session saved!", TOAST_KIND_SUCCESS);
        } else {
            toastQueue.push("Failed to save Endless session", TOAST_KIND_PENALTY);
        }
    };
    auto loadEndlessRun = [&](const std::string& pathOverride = "") -> bool {
        std::string target = pathOverride.empty() ? endlessSavePath : pathOverride;
        if (!endlessSession.loadFromFile(target, avl)) {
            toastQueue.push("No valid saved Endless run found", TOAST_KIND_WARNING);
            return false;
        }
        currentScreen = SCREEN_ENDLESS;
        endlessCam.reset();
        endlessPaused = false;
        endlessShowLastResult = false;
        endlessLastSeed = endlessSession.getInitialSeed();
        animCtrl.clearAll();
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        layoutDirty = false;
        cachedEndlessOps = -1;
        cachedEndlessOpType = -1;
        const EndlessOp& op = endlessSession.getPendingOp();
        status = "Resumed Endless Save (Seed=" + std::to_string(endlessSession.getInitialSeed()) +
                 ", Survived=" + EndlessLeaderboard::formatTimeMs(endlessSession.getSurvivalMs()) +
                 ")! Next: " + std::string(op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                 std::to_string(op.key);
        toastQueue.push("Resumed saved Endless run!", TOAST_KIND_SUCCESS);
        return true;
    };

    // =========================================================================
    // SHARED FRAME RENDERER (used by both main loop and --verify-screenshots)
    // =========================================================================
    auto renderFrame = [&](const UiInput& in, const std::string& capturePath = "",
                           double* outCpuMs = nullptr, double* outDispMs = nullptr) {
        sf::Clock frameCpuClk;
        int curLid = (currentLevelIdx >= 0 && currentLevelIdx < TOTAL_LEVELS) ? levels[currentLevelIdx].id : -1;
        bool needLayout = layoutDirty || (cachedLayoutScreen != currentScreen) ||
                          (currentScreen == SCREEN_LEVEL_PLAY && cachedLayoutLid != curLid);

        auto mapHoverToCam = [&](const CanvasCamera& cam, float sx, float sy, sf::Vector2f& outW) -> bool {
            if (!cam.containsPixel(sx, sy)) return false;
            outW = window.mapPixelToCoords({static_cast<int>(sx), static_cast<int>(sy)}, cam.view);
            return true;
        };
        auto drawCamTooltip = [&](const CanvasCamera& cam, const LayoutResult& layout, int hoveredNodeId) {
            if (hoveredNodeId < 0) return;
            int hIdx = layout.findNodeIndexById(hoveredNodeId);
            if (hIdx < 0) return;
            sf::Vector2i sp = window.mapCoordsToPixel({layout.nodes[hIdx].x, layout.nodes[hIdx].y}, cam.view);
            if (cam.containsPixel(static_cast<float>(sp.x), static_cast<float>(sp.y))) {
                guiRenderer.drawNodeTooltip(window, font, layout, hoveredNodeId,
                                            static_cast<float>(W), static_cast<float>(H),
                                            static_cast<float>(sp.x), static_cast<float>(sp.y));
            }
        };

        window.clear(Ui::bgNavy());
        if (showBgGrid && currentScreen != SCREEN_MAIN_MENU &&
            currentScreen != SCREEN_SPLASH && currentScreen != SCREEN_CAMPAIGN_MAP &&
            currentScreen != SCREEN_SETTINGS && currentScreen != SCREEN_LEADERBOARD &&
            currentScreen != SCREEN_ENDLESS_LEADERBOARD && currentScreen != SCREEN_RESULT) {
            Ui::grid(window, static_cast<float>(W), static_cast<float>(H), &uiCache);
        }

        // 0. SPLASH SCREEN
        if (currentScreen == SCREEN_SPLASH) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color splashBright(0, 255, 65);
            const sf::Color splashDim(74, 122, 74);
            float cx = W * 0.5f;

            Ui::textCenter(window, menuFont, "VAULT HEIST", 56, splashBright, cx, 326.f, true, &uiCache);
            Ui::textCenter(window, menuFont, "THE LIVING VAULT", 18, splashDim, cx, 396.f, false, &uiCache);

            float barW = 400.f, barH = 10.f;
            float barX = cx - barW * 0.5f, barY = 442.f;
            sf::RectangleShape pTrack({barW, barH});
            pTrack.setPosition({barX, barY});
            pTrack.setFillColor(sf::Color(12, 26, 15));
            pTrack.setOutlineColor(sf::Color(0, 255, 65, 100));
            pTrack.setOutlineThickness(1.f);
            window.draw(pTrack);

            float prog = minFloat(1.0f, splashTimer / 3.2f);
            if (prog > 0.0f) {
                sf::RectangleShape pFill({barW * prog, barH});
                pFill.setPosition({barX, barY});
                pFill.setFillColor(splashBright);
                window.draw(pFill);
            }

            const char* splashMsgs[4] = {
                "Cracking security protocols...",
                "Decrypting vault blueprints...",
                "Bypassing biometric locks...",
                "Initializing heist terminal..."
            };
            int msgIdx = static_cast<int>(splashTimer / 0.8f) % 4;
            if (msgIdx < 0) msgIdx = 0;
            Ui::textCenter(window, menuFont, splashMsgs[msgIdx], 15, sf::Color(110, 195, 110), cx, 474.f, false, &uiCache);
            Ui::textCenter(window, menuFont, "[Press any key or click to skip]", 14, sf::Color(95, 165, 95), cx, 780.f, false, &uiCache);
        }

        // 1. MAIN MENU
        if (currentScreen == SCREEN_MAIN_MENU) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }

            const sf::Color menuBright(0, 255, 65);
            const sf::Color menuDim(74, 122, 74);

            // Thin green border rectangle around entire screen
            sf::RectangleShape screenBorder({static_cast<float>(W) - 32.f, static_cast<float>(H) - 32.f});
            screenBorder.setPosition({16.f, 16.f});
            screenBorder.setFillColor(sf::Color::Transparent);
            screenBorder.setOutlineColor(sf::Color(0, 255, 65, 110));
            screenBorder.setOutlineThickness(1.f);
            window.draw(screenBorder);

            // Top-left title: "VAULT" (#00FF41) + "HEIST" (#FFFFFF)
            float titleX = 100.f;
            float titleY = 96.f;
            float vaultAdv = Ui::textWidth(menuFont, "VAULT ", 64, true);
            Ui::text(window, menuFont, "VAULT", 64, menuBright, titleX, titleY, true, &uiCache);
            Ui::text(window, menuFont, "HEIST", 64, sf::Color::White, titleX + vaultAdv, titleY, true, &uiCache);

            // Subtle accent line under title
            sf::RectangleShape titleLine({540.f, 1.5f});
            titleLine.setPosition({titleX, titleY + 86.f});
            titleLine.setFillColor(sf::Color(0, 255, 65, 160));
            window.draw(titleLine);

            // Subtitle below title
            Ui::text(window, menuFont, "BREAK IN . TAKE CONTROL . GET OUT", 15, menuDim, titleX, titleY + 100.f, false, &uiCache);

            // Top-right corner: "SECURE SYSTEM ▪"
            float ssW = Ui::textWidth(menuFont, "SECURE SYSTEM", 14, false);
            Ui::text(window, menuFont, "SECURE SYSTEM", 14, sf::Color(0, 255, 65, 200),
                     static_cast<float>(W) - 62.f - ssW, 34.f, false, &uiCache);
            sf::RectangleShape topSq({10.f, 10.f});
            topSq.setPosition({static_cast<float>(W) - 46.f, 37.f});
            topSq.setFillColor(menuBright);
            window.draw(topSq);

            // Left-aligned menu list (x ~ 120px, starting y ~ 300px, spacing 70px)
            const char* menuItems[6] = {
                "1.  CAMPAIGN",
                "2.  ENDLESS",
                "3.  SANDBOX",
                "4.  LEADERBOARD",
                "5.  SETTINGS",
                "6.  QUIT"
            };
            for (int i = 0; i < 6; ++i) {
                float itemY = 300.f + static_cast<float>(i) * 70.f;
                bool sel = (i == menuSelectedIdx);
                if (sel) {
                    Ui::text(window, menuFont, ">", 24, menuBright, 96.f, itemY, true, &uiCache);
                    Ui::text(window, menuFont, menuItems[i], 24, menuBright, 128.f, itemY, true, &uiCache);
                    sf::RectangleShape itemUnderline({360.f, 2.f});
                    itemUnderline.setPosition({128.f, itemY + 36.f});
                    itemUnderline.setFillColor(menuBright);
                    window.draw(itemUnderline);
                } else {
                    Ui::text(window, menuFont, menuItems[i], 24, menuDim, 128.f, itemY, false, &uiCache);
                }
            }

            // Bottom full-width thin green line (height 1px, #00FF41)
            sf::RectangleShape bottomLine({static_cast<float>(W) - 32.f, 1.f});
            bottomLine.setPosition({16.f, static_cast<float>(H) - 64.f});
            bottomLine.setFillColor(menuBright);
            window.draw(bottomLine);

            // Bottom-left and bottom-right small green squares
            sf::RectangleShape blSq({10.f, 10.f});
            blSq.setPosition({32.f, static_cast<float>(H) - 44.f});
            blSq.setFillColor(menuBright);
            window.draw(blSq);

            sf::RectangleShape brSq({10.f, 10.f});
            brSq.setPosition({static_cast<float>(W) - 42.f, static_cast<float>(H) - 44.f});
            brSq.setFillColor(menuBright);
            window.draw(brSq);

            // Bottom-left version & prompt text
            Ui::text(window, menuFont, "v1.0.0 | PRESS ANY KEY TO CONTINUE", 14, sf::Color(110, 195, 110),
                     58.f, static_cast<float>(H) - 48.f, false, &uiCache);
        }

        // 2. CAMPAIGN MAP
        if (currentScreen == SCREEN_CAMPAIGN_MAP) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }

            const sf::Color mapBright(0, 255, 65);
            const sf::Color mapDim(74, 122, 74);
            const sf::Color mapSubtle(110, 185, 110);

            UiInput mapIn = in;
            if (demoPromptOpen) {
                mapIn.mx = -1.f;
                mapIn.my = -1.f;
                mapIn.down = false;
            }

            static const char* actNames[4] = {
                "ACT 1 : BLUEPRINT WING",
                "ACT 2 : SORTED WING",
                "ACT 3 : ARCHITECT'S WING",
                "ACT 4 : HEAP FORTRESS"
            };
            static const char* actShort[4] = {
                "ACT 1", "ACT 2", "ACT 3", "ACT 4"
            };
            static const char* lvlNames[4][5] = {
                { "The Blueprint", "Decode the Blueprint", "The Alarm Spreads", "Two Routes Meet", "Mirror the Vault" },
                { "Search Heist", "Stash the Loot", "Steal and Collapse", "Range Raid", "The Lazy Architect" },
                { "Read the Badges", "Single Rotations", "Double Rotations", "Auto Mode", "Order-Statistic Heist" },
                { "Activate the Vault", "Repair Security Locks", "Extract the Greatest", "Race the Guards", "Final Vault Boss" }
            };
            static const char* lvlDescs[4][5] = {
                {
                    "Visit all rooms in traversal order to map the vault layout.",
                    "Reconstruct the tree from Pre-order and In-order sequences.",
                    "Calculate room heights and tree diameter before alarms trigger.",
                    "Find the common ancestor of two rooms and trace the path.",
                    "Invert the vault blueprint to expose the hidden exit."
                },
                {
                    "Navigate the BST to locate the target key before time runs out.",
                    "Insert new keys into the BST without breaking sort order.",
                    "Delete targets using correct BST deletion cases.",
                    "Report all rooms within the target range, prune the rest.",
                    "Insert sorted keys into a degenerating BST chain. Boss level."
                },
                {
                    "Identify all unbalanced nodes with violation badges.",
                    "Apply LL or RR rotations to restore AVL balance.",
                    "Execute LR and RL double rotations on zig-zag imbalances.",
                    "Predict the rotation type before each insertion.",
                    "Steal the k-th largest treasure in O(log n). Boss level."
                },
                {
                    "Build a Max Heap bottom-up from an unsorted array.",
                    "Sift down the sabotaged root to restore heap order.",
                    "Extract treasures one by one from the heap root.",
                    "Extract all treasures before the alarm capacity fills.",
                    "Build heap then heap-sort all treasures. Final boss."
                }
            };

            // Outer screen border
            sf::RectangleShape mapBorder({static_cast<float>(W) - 32.f, static_cast<float>(H) - 32.f});
            mapBorder.setPosition({16.f, 16.f});
            mapBorder.setFillColor(sf::Color::Transparent);
            mapBorder.setOutlineColor(sf::Color(0, 255, 65, 90));
            mapBorder.setOutlineThickness(1.f);
            window.draw(mapBorder);

            // Top bar: "VAULT HEIST //" left, active act center, "BACK TO MENU [ESC]" right
            Ui::text(window, menuFont, "VAULT HEIST //", 28, mapBright, 44.f, 24.f, true, &uiCache);

            const char* curActTitle = actNames[selectedActIdx];
            float actTitleW = Ui::textWidth(menuFont, curActTitle, 20, true);
            float actTitleX = static_cast<float>(W) * 0.5f - actTitleW * 0.5f;
            Ui::text(window, menuFont, curActTitle, 20, mapBright, actTitleX, 28.f, true, &uiCache);

            float topLineLW = actTitleX - 380.f;
            if (topLineLW < 20.f) topLineLW = 20.f;
            sf::RectangleShape topLineL({topLineLW, 1.f});
            topLineL.setPosition({350.f, 39.f});
            topLineL.setFillColor(sf::Color(0, 255, 65, 130));
            window.draw(topLineL);

            float topLineRW = 1320.f - (actTitleX + actTitleW + 30.f);
            if (topLineRW < 20.f) topLineRW = 20.f;
            sf::RectangleShape topLineR({topLineRW, 1.f});
            topLineR.setPosition({actTitleX + actTitleW + 30.f, 39.f});
            topLineR.setFillColor(sf::Color(0, 255, 65, 130));
            window.draw(topLineR);

            bool backHov = Ui::inside(mapIn.mx, mapIn.my, 1350.f, 20.f, 220.f, 38.f);
            Ui::text(window, menuFont, "BACK TO MENU [ESC]", 15, backHov ? sf::Color::White : mapBright,
                     1370.f, 30.f, backHov, &uiCache);

            // Left panel (~280px wide): 4 act buttons connected by dots/lines
            sf::RectangleShape leftDiv({1.f, 670.f});
            leftDiv.setPosition({300.f, 110.f});
            leftDiv.setFillColor(sf::Color(0, 255, 65, 90));
            window.draw(leftDiv);

            for (int a = 0; a < 4; ++a) {
                float bx = 44.f;
                float by = 190.f + static_cast<float>(a) * 124.f;
                float bw = 220.f, bh = 66.f;
                bool active = (a == selectedActIdx);
                bool hov = Ui::inside(mapIn.mx, mapIn.my, bx, by, bw, bh);

                sf::RectangleShape actBox({bw, bh});
                actBox.setPosition({bx, by});
                actBox.setFillColor(active ? sf::Color(8, 28, 12, 225) : sf::Color(6, 14, 8, 200));
                actBox.setOutlineColor((active || hov) ? mapBright : mapDim);
                actBox.setOutlineThickness(active ? 2.f : 1.f);
                window.draw(actBox);

                std::string actLbl = active ? (std::string("> ") + actShort[a]) : std::string(actShort[a]);
                Ui::textCenter(window, menuFont, actLbl, 20, (active || hov) ? mapBright : mapDim,
                               bx + bw * 0.5f, by + 20.f, active, &uiCache);

                if (a < 3) {
                    float cx = bx + bw * 0.5f;
                    sf::RectangleShape conn({2.f, 58.f});
                    conn.setPosition({cx - 1.f, by + bh});
                    conn.setFillColor(sf::Color(0, 255, 65, 130));
                    window.draw(conn);

                    sf::CircleShape dot(5.f);
                    dot.setOrigin({5.f, 5.f});
                    dot.setPosition({cx, by + bh + 29.f});
                    dot.setFillColor(mapBright);
                    window.draw(dot);
                }
            }

            // Center area: 5 level nodes for active act as circles connected by lines
            const float nodeX[5] = { 440.f, 610.f, 780.f, 950.f, 1120.f };
            const float nodeY[5] = { 360.f, 460.f, 295.f, 450.f, 305.f };
            const float nodeR = 44.f;

            for (int i = 0; i < 4; ++i) {
                int liNext = selectedActIdx * 5 + (i + 1);
                sf::Color lineCol = levels[liNext].unlocked ? mapBright : mapDim;
                sf::Vertex seg[] = {
                    sf::Vertex({nodeX[i], nodeY[i]}, lineCol),
                    sf::Vertex({nodeX[i + 1], nodeY[i + 1]}, lineCol)
                };
                window.draw(seg, 2, sf::PrimitiveType::Lines);
            }

            for (int i = 0; i < 5; ++i) {
                int li = selectedActIdx * 5 + i;
                bool unl = levels[li].unlocked;
                bool isSel = (i == selectedLevelInAct);
                float dx = mapIn.mx - nodeX[i];
                float dy = mapIn.my - nodeY[i];
                bool hov = (dx * dx + dy * dy) <= (nodeR * nodeR);

                sf::CircleShape nodeCirc(nodeR);
                nodeCirc.setOrigin({nodeR, nodeR});
                nodeCirc.setPosition({nodeX[i], nodeY[i]});
                nodeCirc.setFillColor(unl ? sf::Color(8, 28, 12, 235) : sf::Color(6, 14, 8, 225));
                nodeCirc.setOutlineColor((unl || isSel || hov) ? mapBright : mapDim);
                nodeCirc.setOutlineThickness(isSel ? 3.f : (unl ? 2.f : 1.5f));
                window.draw(nodeCirc);

                if (unl) {
                    char numBuf[4];
                    numBuf[0] = static_cast<char>('1' + i);
                    numBuf[1] = '\0';
                    Ui::textCenter(window, menuFont, numBuf, 28, mapBright,
                                   nodeX[i], nodeY[i] - 18.f, true, &uiCache);
                    if (levels[li].stars > 0) {
                        Ui::stars(window, nodeX[i], nodeY[i] + nodeR + 14.f, levels[li].stars, 3, 6.f, &uiCache);
                    }
                } else {
                    sf::Color lockCol = (isSel || hov) ? mapBright : mapDim;
                    // Padlock shackle
                    sf::RectangleShape shackle({16.f, 13.f});
                    shackle.setOrigin({8.f, 13.f});
                    shackle.setPosition({nodeX[i], nodeY[i] - 2.f});
                    shackle.setFillColor(sf::Color::Transparent);
                    shackle.setOutlineColor(lockCol);
                    shackle.setOutlineThickness(2.5f);
                    window.draw(shackle);
                    // Padlock body
                    sf::RectangleShape lockBody({24.f, 18.f});
                    lockBody.setOrigin({12.f, 0.f});
                    lockBody.setPosition({nodeX[i], nodeY[i] - 2.f});
                    lockBody.setFillColor(lockCol);
                    window.draw(lockBody);
                    // Keyhole
                    sf::RectangleShape keyhole({4.f, 7.f});
                    keyhole.setOrigin({2.f, 0.f});
                    keyhole.setPosition({nodeX[i], nodeY[i] + 4.f});
                    keyhole.setFillColor(sf::Color(6, 14, 8));
                    window.draw(keyhole);
                }
            }

            // Bottom info panel (~180px tall)
            float infoX = 340.f, infoY = 600.f, infoW = 840.f, infoH = 180.f;
            sf::RectangleShape infoPanel({infoW, infoH});
            infoPanel.setPosition({infoX, infoY});
            infoPanel.setFillColor(sf::Color(6, 14, 10, 225));
            infoPanel.setOutlineColor(sf::Color(0, 255, 65, 140));
            infoPanel.setOutlineThickness(1.f);
            window.draw(infoPanel);

            // Top-left green bar accent on info panel
            sf::RectangleShape infoAccent({52.f, 4.f});
            infoAccent.setPosition({infoX, infoY});
            infoAccent.setFillColor(mapBright);
            window.draw(infoAccent);

            std::string missionTitle = "Mission " + std::to_string(selectedActIdx + 1) + "." +
                                       std::to_string(selectedLevelInAct + 1) + " : " +
                                       lvlNames[selectedActIdx][selectedLevelInAct];
            Ui::text(window, menuFont, missionTitle, 22, mapBright, infoX + 28.f, infoY + 26.f, true, &uiCache);
            Ui::textWrap(window, menuFont, lvlDescs[selectedActIdx][selectedLevelInAct], 15, mapSubtle,
                         infoX + 28.f, infoY + 72.f, infoW - 56.f, false, &uiCache);

            int selLi = selectedActIdx * 5 + selectedLevelInAct;
            if (levels[selLi].unlocked) {
                Ui::text(window, menuFont, "[Click node or press Enter to launch]", 13, mapBright,
                         infoX + 28.f, infoY + infoH - 34.f, false, &uiCache);
            } else {
                Ui::text(window, menuFont, "[LOCKED - Clear previous level to unlock]", 13, mapDim,
                         infoX + 28.f, infoY + infoH - 34.f, false, &uiCache);
            }

            // Bottom status bar
            sf::RectangleShape bottomLine({static_cast<float>(W) - 32.f, 1.f});
            bottomLine.setPosition({16.f, static_cast<float>(H) - 64.f});
            bottomLine.setFillColor(mapBright);
            window.draw(bottomLine);

            sf::RectangleShape blSq({10.f, 10.f});
            blSq.setPosition({32.f, static_cast<float>(H) - 44.f});
            blSq.setFillColor(mapBright);
            window.draw(blSq);

            Ui::text(window, menuFont, "SELECT A ACT | NAVIGATE WITH ARROWS", 14, mapSubtle,
                     58.f, static_cast<float>(H) - 48.f, false, &uiCache);

            // Demo unlock button centered in bottom status bar
            float demoBtnX = static_cast<float>(W) * 0.5f - 135.f;
            float demoBtnY = static_cast<float>(H) - 54.f;
            float demoBtnW = 270.f, demoBtnH = 34.f;
            bool demoHov = Ui::inside(mapIn.mx, mapIn.my, demoBtnX, demoBtnY, demoBtnW, demoBtnH);
            sf::RectangleShape demoBox({demoBtnW, demoBtnH});
            demoBox.setPosition({demoBtnX, demoBtnY});
            demoBox.setFillColor(demoHov ? sf::Color(12, 36, 18, 230) : sf::Color(6, 16, 10, 200));
            demoBox.setOutlineColor(demoHov ? mapBright : sf::Color(0, 255, 65, 110));
            demoBox.setOutlineThickness(1.f);
            window.draw(demoBox);
            Ui::textCenter(window, menuFont,
                           demoAllUnlocked ? "Demo - Lock All Levels" : "Demo - Unlock All Levels",
                           14, demoHov ? sf::Color::White : mapBright,
                           static_cast<float>(W) * 0.5f, demoBtnY + 8.f, false, &uiCache);

            // Right progress counter
            int actCleared = 0;
            for (int i = 0; i < 5; ++i) {
                if (levels[selectedActIdx * 5 + i].stars > 0) ++actCleared;
            }
            std::string progStr = "PROGRESS: " + std::to_string(actCleared) + " / 5";
            float progW = Ui::textWidth(menuFont, progStr, 14, false);
            Ui::text(window, menuFont, progStr, 14, mapSubtle,
                     static_cast<float>(W) - 36.f - progW, static_cast<float>(H) - 48.f, false, &uiCache);

            if (demoPromptOpen) {
                sf::RectangleShape dim({static_cast<float>(W), static_cast<float>(H)});
                dim.setFillColor(sf::Color(4, 10, 6, 195));
                window.draw(dim);

                float mw = 460.f, mh = 210.f;
                float mx0 = (static_cast<float>(W) - mw) * 0.5f;
                float my0 = (static_cast<float>(H) - mh) * 0.5f;

                sf::RectangleShape modalBox({mw, mh});
                modalBox.setPosition({mx0, my0});
                modalBox.setFillColor(sf::Color(5, 14, 8, 244));
                modalBox.setOutlineColor(mapBright);
                modalBox.setOutlineThickness(2.f);
                window.draw(modalBox);

                // Corner accents
                const float cLen = 14.f, cThk = 3.f;
                sf::RectangleShape c1({cLen, cThk}); c1.setPosition({mx0 - 1.f, my0 - 1.f}); c1.setFillColor(mapBright); window.draw(c1);
                sf::RectangleShape c2({cThk, cLen}); c2.setPosition({mx0 - 1.f, my0 - 1.f}); c2.setFillColor(mapBright); window.draw(c2);
                sf::RectangleShape c3({cLen, cThk}); c3.setPosition({mx0 + mw - cLen + 1.f, my0 - 1.f}); c3.setFillColor(mapBright); window.draw(c3);
                sf::RectangleShape c4({cThk, cLen}); c4.setPosition({mx0 + mw - cThk + 1.f, my0 - 1.f}); c4.setFillColor(mapBright); window.draw(c4);
                sf::RectangleShape c5({cLen, cThk}); c5.setPosition({mx0 - 1.f, my0 + mh - cThk + 1.f}); c5.setFillColor(mapBright); window.draw(c5);
                sf::RectangleShape c6({cThk, cLen}); c6.setPosition({mx0 - 1.f, my0 + mh - cLen + 1.f}); c6.setFillColor(mapBright); window.draw(c6);
                sf::RectangleShape c7({cLen, cThk}); c7.setPosition({mx0 + mw - cLen + 1.f, my0 + mh - cThk + 1.f}); c7.setFillColor(mapBright); window.draw(c7);
                sf::RectangleShape c8({cThk, cLen}); c8.setPosition({mx0 + mw - cThk + 1.f, my0 + mh - cLen + 1.f}); c8.setFillColor(mapBright); window.draw(c8);

                Ui::textCenter(window, menuFont,
                               demoAllUnlocked ? "LOCK ALL LEVELS (DEMO)" : "UNLOCK ALL LEVELS (DEMO)",
                               20, mapBright, mx0 + mw * 0.5f, my0 + 18.f, true, &uiCache);
                Ui::textCenter(window, menuFont, "Enter demo password.", 14, mapSubtle,
                               mx0 + mw * 0.5f, my0 + 52.f, false, &uiCache);

                float ix = mx0 + 36.f, iy = my0 + 86.f, iw = mw - 72.f, ih = 42.f;
                sf::RectangleShape inputBox({iw, ih});
                inputBox.setPosition({ix, iy});
                inputBox.setFillColor(sf::Color(3, 10, 5));
                inputBox.setOutlineColor(mapBright);
                inputBox.setOutlineThickness(1.5f);
                window.draw(inputBox);

                char maskBuf[34];
                for (int k = 0; k < demoPasswordLen; ++k) maskBuf[k] = '*';
                bool showCursor = (static_cast<int>(uiAnimTimer * 2.5f) % 2 == 0);
                if (demoPasswordLen < 32 && showCursor) {
                    maskBuf[demoPasswordLen] = '_';
                    maskBuf[demoPasswordLen + 1] = '\0';
                } else {
                    maskBuf[demoPasswordLen] = '\0';
                }
                Ui::text(window, menuFont, maskBuf, 18, mapBright, ix + 14.f, iy + 9.f, false, &uiCache);

                bool subHov = Ui::inside(in.mx, in.my, mx0 + 36.f, my0 + 148.f, 180.f, 38.f);
                sf::RectangleShape subBox({180.f, 38.f});
                subBox.setPosition({mx0 + 36.f, my0 + 148.f});
                subBox.setFillColor(subHov ? sf::Color(0, 255, 65) : sf::Color(10, 72, 26));
                subBox.setOutlineColor(mapBright);
                subBox.setOutlineThickness(2.f);
                window.draw(subBox);
                Ui::textCenter(window, menuFont, "Submit [Enter]", 14,
                               subHov ? sf::Color(4, 14, 6) : sf::Color::White,
                               mx0 + 36.f + 90.f, my0 + 158.f, true, &uiCache);

                bool canHov = Ui::inside(in.mx, in.my, mx0 + mw - 216.f, my0 + 148.f, 180.f, 38.f);
                sf::RectangleShape canBox({180.f, 38.f});
                canBox.setPosition({mx0 + mw - 216.f, my0 + 148.f});
                canBox.setFillColor(canHov ? sf::Color(14, 38, 20) : sf::Color(5, 16, 8));
                canBox.setOutlineColor(canHov ? mapBright : mapDim);
                canBox.setOutlineThickness(1.5f);
                window.draw(canBox);
                Ui::textCenter(window, menuFont, "Cancel [Esc]", 14,
                               canHov ? sf::Color::White : mapSubtle,
                               mx0 + mw - 216.f + 90.f, my0 + 158.f, false, &uiCache);
            }
        }

        // 3. LEVEL PLAY SCREEN
        if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            int lid = levels[currentLevelIdx].id;
            int alarmCap = levels[currentLevelIdx].alarmCapacity;

            if (cachedHudLid != lid) {
                cachedHudLid = lid;
                cachedHudObj = "Objective: " + levels[currentLevelIdx].objective;
                cachedHudCtrlLine = levels[currentLevelIdx].controls +
                                    "  |  [H] Hint  |  [Z] Undo  |  [Space] Play/Pause  |  [Right] Step  |  [Esc] Map";
            }
            if (cachedHudAlarm != alarmSteps || cachedHudCap != alarmCap || cachedHudScore != score) {
                cachedHudAlarm = alarmSteps;
                cachedHudCap = alarmCap;
                cachedHudScore = score;
                cachedHudAlarmScore = "Alarm: " + std::to_string(alarmSteps) + " / " +
                                      std::to_string(alarmCap) + "    Score: " + std::to_string(score);
            }

            // Top HUD bar (x=30..1570)
            Ui::textFit(window, menuFont, levels[currentLevelIdx].title, 24, sf::Color(0, 255, 65), 30.f, 10.f, 675.f, true, &menuUiCache);
            Ui::textWrap(window, font, cachedHudObj, 12, sf::Color(215, 240, 220), 30.f, 42.f, 675.f, false, &uiCache);

            Ui::button(window, font, hintSys.isAvailable() ? "Hint [H]" : "Hint Used", 722.f, 18.f, 115.f, 34.f,
                       hintSys.isAvailable() ? BTN_PRIMARY : BTN_DISABLED, in, 13, &uiCache);
            Ui::button(window, font, "Undo [Z]", 850.f, 18.f, 115.f, 34.f,
                       undoStack.isEmpty() ? BTN_DISABLED : BTN_GHOST, in, 13, &uiCache);
            Ui::button(window, font, "Map [Esc]", 978.f, 18.f, 115.f, 34.f, BTN_GHOST, in, 13, &uiCache);

            float meterX = 1120.f, meterW = 450.f;
            sf::RectangleShape& meterRect = uiCache.rect;
            meterRect.setSize({meterW, 20.f});
            meterRect.setOrigin({0.f, 0.f});
            meterRect.setPosition({meterX, 16.f});
            meterRect.setFillColor(sf::Color(6, 18, 10));
            meterRect.setOutlineThickness(1.5f);
            meterRect.setOutlineColor(sf::Color(0, 255, 65, 160));
            window.draw(meterRect);

            float ratio = minFloat(1.0f, static_cast<float>(alarmSteps) / static_cast<float>(alarmCap));
            meterRect.setSize({meterW * ratio, 20.f});
            meterRect.setPosition({meterX, 16.f});
            meterRect.setOutlineThickness(0.f);
            meterRect.setFillColor(ratio > 0.8f ? Ui::red() : sf::Color(0, 255, 65));
            window.draw(meterRect);

            Ui::textFit(window, menuFont, cachedHudAlarmScore, 14, sf::Color(0, 255, 65), meterX, 44.f, meterW, true, &menuUiCache);

            // Structure Canvas Frame (opaque interior so only canvas world grid shows)
            Ui::panel(window, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, sf::Color(55, 85, 130), 248, &uiCache);

            // Populate Highlight State
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.showBadges = showNodeBadges;
            for (int m = 0; m < markedCount; ++m) hl.addMarked(markedIds[m]);

            sf::Vector2f canvasHoverWorld(0.f, 0.f);
            bool inCanvasHover = !showingBriefing && mapHoverToCam(levelCam, in.mx, in.my, canvasHoverWorld);

            const LayoutResult* activeDrawLayout = nullptr;
            LayoutResult act2DimLayout;
            bool showBf = false, showId = false;

            if (lid >= LEVEL_1_1 && lid <= LEVEL_1_5) {
                if (lid == LEVEL_1_2 && !act1Reconstructed) {
                    layoutDirty = false;
                } else if (!animCtrl.hasEvents() && needLayout) {
                    layoutBtForScreen(false);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : btLayout;
                activeDrawLayout = &drawL;
                if (inCanvasHover && !(lid == LEVEL_1_2 && !act1Reconstructed)) {
                    int hovIdx = drawL.hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
                }
                if (animCtrl.hasEvents()) {
                    animCtrl.fillHighlightState(hl);
                } else {
                    int hlIdx1 = drawL.findNodeIndexByKey(act1Hl1);
                    int hlIdx2 = drawL.findNodeIndexByKey(act1Hl2);
                    if (hlIdx1 >= 0) { hl.primaryNodeId = drawL.nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                    if (hlIdx2 >= 0) hl.secondaryNodeId = drawL.nodes[hlIdx2].id;
                }
                if (lid == LEVEL_1_3) hl.showBadges = false;
                showId = true;
            }
            else if (lid >= LEVEL_2_1 && lid <= LEVEL_2_5) {
                if (!animCtrl.hasEvents() && needLayout) {
                    layoutBstForScreen(false);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : bstLayout;
                activeDrawLayout = &drawL;
                if (lid == LEVEL_2_4 && act2PrunedCount > 0) {
                    act2DimLayout = drawL;
                    for (int di = 0; di < act2DimLayout.nodeCount; ++di) {
                        if (containsInt(act2PrunedKeys, act2PrunedCount, act2DimLayout.nodes[di].key)) {
                            act2DimLayout.nodes[di].alpha = 0.24f;
                        }
                    }
                    activeDrawLayout = &act2DimLayout;
                }
                if (inCanvasHover) {
                    int hovIdx = activeDrawLayout->hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = activeDrawLayout->nodes[hovIdx].id;
                }
                if (animCtrl.hasEvents() && (!animCtrl.isAtEnd() || animCtrl.isPlaying())) {
                    animCtrl.fillHighlightState(hl);
                } else {
                    int hlIdx1 = activeDrawLayout->findNodeIndexByKey(act2Hl1);
                    int hlIdx2 = activeDrawLayout->findNodeIndexByKey(act2Hl2);
                    if (hlIdx1 >= 0) { hl.primaryNodeId = activeDrawLayout->nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                    if (hlIdx2 >= 0) hl.secondaryNodeId = activeDrawLayout->nodes[hlIdx2].id;
                }
                showId = true;
            }
            else if (lid >= LEVEL_3_1 && lid <= LEVEL_3_5) {
                if (!animCtrl.hasEvents() && needLayout) {
                    layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : avlLayout;
                activeDrawLayout = &drawL;
                if (inCanvasHover) {
                    int hovIdx = drawL.hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
                }
                if (animCtrl.hasEvents()) animCtrl.fillHighlightState(hl);
                showBf = true;
                if (lid == LEVEL_3_5) hl.showBadges = true;
            }
            else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                bool useRealHeap = (lid == LEVEL_4_3 || lid == LEVEL_4_4 || (lid == LEVEL_4_5 && act4Built));
                if (!animCtrl.hasEvents() && needLayout) {
                    layoutHeapForScreen(useRealHeap, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    layoutDirty = false;
                }
                const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : heapLayout;
                activeDrawLayout = &drawL;
                if (inCanvasHover) {
                    int hovIdx = drawL.hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
                    else {
                        int cIdx = drawL.hitTestCell(canvasHoverWorld.x, canvasHoverWorld.y);
                        if (cIdx >= 0) hl.hoveredNodeId = cIdx;
                    }
                }
                if (animCtrl.hasEvents()) {
                    animCtrl.fillHighlightState(hl);
                } else {
                    hl.primaryNodeId = hlIndex1;
                    hl.secondaryNodeId = hlIndex2;
                    hl.arrayHl1 = hlIndex1;
                    hl.arrayHl2 = hlIndex2;
                    if (hlIndex1 >= 0) hl.primaryStyle = HL_VISIT;
                }
            }

            if (activeDrawLayout) {
                window.setView(levelCam.view);
                if (showBgGrid) guiRenderer.drawCanvasGrid(window, levelCam.view);
                if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                    guiRenderer.drawHeapLayout(window, font, *activeDrawLayout, hl, CANVAS_X + 14.f, CANVAS_Y + CANVAS_H - 80.f);
                } else if (lid == LEVEL_1_2 && !act1Reconstructed) {
                    for (int ei = 0; ei < activeDrawLayout->edgeCount; ++ei) {
                        const EdgeLayout& e = activeDrawLayout->edges[ei];
                        sf::Vertex line[] = {
                            sf::Vertex({e.x1, e.y1}, sf::Color(85, 130, 190, 185)),
                            sf::Vertex({e.x2, e.y2}, sf::Color(85, 130, 190, 185))
                        };
                        window.draw(line, 2, sf::PrimitiveType::Lines);
                    }
                    DragDropView::draw(window, font, dragDrop, &uiCache);
                } else {
                    guiRenderer.drawTreeLayout(window, font, *activeDrawLayout, hl, showBf, showId);
                    if (lid == LEVEL_1_3 && act1PostIdx > 0) {
                        int postCnt = 0;
                        int* postSeq = act1Tree.postorderRecursive(postCnt);
                        if (postSeq) {
                            for (int pi = 0; pi < act1PostIdx && pi < postCnt; ++pi) {
                                int nIdx = activeDrawLayout->findNodeIndexByKey(postSeq[pi]);
                                if (nIdx >= 0) {
                                    const NodeLayout& nd = activeDrawLayout->nodes[nIdx];
                                    float bx = nd.x + nd.radius - 6.f;
                                    float by = nd.y - nd.radius - 10.f;
                                    sf::RectangleShape& pill = uiCache.rect;
                                    pill.setSize({32.f, 18.f});
                                    pill.setOrigin({0.f, 0.f});
                                    pill.setPosition({bx, by});
                                    pill.setFillColor(sf::Color(20, 75, 50, 235));
                                    pill.setOutlineThickness(1.2f);
                                    pill.setOutlineColor(Ui::green());
                                    window.draw(pill);
                                    Ui::textCenter(window, font, "h=" + std::to_string(act1ConfirmedHeight[pi]),
                                                   11, Ui::green(), bx + 16.f, by + 2.f, true, &uiCache);
                                }
                            }
                            delete[] postSeq;
                        }
                    }
                }
                if ((lid != LEVEL_1_2 || act1Reconstructed) && (dragDrop.tokenCount() > 0 || dragDrop.slotCount() > 0)) {
                    DragDropView::draw(window, font, dragDrop, &uiCache);
                }
                window.setView(window.getDefaultView());

                Ui::text(window, font, levelCam.zoomText(), 11, Ui::textMuted(),
                         CANVAS_X + 12.f, CANVAS_Y + 6.f, false, &uiCache);
                if (lid != LEVEL_1_3 && !(lid == LEVEL_1_2 && !act1Reconstructed)) {
                    drawCamTooltip(levelCam, *activeDrawLayout, hl.hoveredNodeId);
                }
            }

            // Event Log + Playback Controls Panel (right column)
            guiRenderer.drawEventLogPanel(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            // Bottom Guidance & Minimal Structural Controls Panel
            Ui::panel(window, 30.f, 660.f, 1540.f, 210.f, sf::Color(60, 85, 125), 235, &uiCache);

            int act2PromptA = -9999, act2PromptB = -9999;
            int act3PromptA = -9999, act3PromptB = -9999;
            if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) {
                act2PromptA = act2CurKey;
                act2PromptB = act2TargetIdx * 10 + act2BossStage;
            } else if (lid == LEVEL_2_3) {
                act2PromptA = act2DelStage;
                act2PromptB = act2Bst.getSize();
            } else if (lid == LEVEL_2_4) {
                act2PromptA = act2RqIdx;
                act2PromptB = act2PrunedCount;
            } else if (lid == LEVEL_3_1) {
                act3PromptA = clicked31Mask;
            } else if (lid == LEVEL_3_2) {
                act3PromptA = manualSubPuzzle;
                act3PromptB = act3PivotPicked ? act3PivotKey : -1;
            } else if (lid == LEVEL_3_3) {
                act3PromptA = manualSubPuzzle;
                act3PromptB = manualDoubleStep;
            } else if (lid == LEVEL_3_5) {
                act3PromptA = bossQueryIdx;
                act3PromptB = avl.getSize();
            } else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                act3PromptA = scoring.steps();
                act3PromptB = sortedCount * 10 + (act4Built ? 1 : 0);
            }

            if (cachedPromptLid != lid || cachedPromptSeq != act1SeqIdx || cachedPromptAvl != avlQueuePos ||
                cachedPromptMode != act1TravMode || cachedPromptPost != act1PostIdx || cachedPromptLca != act1LcaStage ||
                cachedPromptAct2A != act2PromptA || cachedPromptAct2B != act2PromptB ||
                cachedPromptAct3A != act3PromptA || cachedPromptAct3B != act3PromptB) {
                cachedPromptLid = lid;
                cachedPromptSeq = act1SeqIdx;
                cachedPromptAvl = avlQueuePos;
                cachedPromptMode = act1TravMode;
                cachedPromptPost = act1PostIdx;
                cachedPromptLca = act1LcaStage;
                cachedPromptAct2A = act2PromptA;
                cachedPromptAct2B = act2PromptB;
                cachedPromptAct3A = act3PromptA;
                cachedPromptAct3B = act3PromptB;
                cachedPromptHead = "NODE INTERACTION GUIDE";
                const LevelData& curD = levelData[currentLevelIdx];
                if (lid == LEVEL_3_4 && avlQueuePos < curD.insertCount) {
                    cachedPromptHead = "INCOMING KEY [" + std::to_string(curD.inserts[avlQueuePos].key) +
                                       "] - Predict rotation (LL / RR / LR / RL / None):";
                } else if (lid == LEVEL_3_1) {
                    cachedPromptHead = "READ THE BADGES - Click every room with |BF| > 1 (height(L) - height(R)):";
                } else if (lid == LEVEL_3_2) {
                    if (!act3PivotPicked) {
                        cachedPromptHead = "SINGLE ROTATION [" + std::to_string(manualSubPuzzle) + "/" +
                                           std::to_string(curD.puzzleCount) + "] - Step 1/2: Click unbalanced pivot (|BF| > 1):";
                    } else {
                        cachedPromptHead = "PIVOT [" + std::to_string(act3PivotKey) +
                                           "] SELECTED - Step 2/2: Pick [LL: Rotate Right] or [RR: Rotate Left]:";
                    }
                } else if (lid == LEVEL_3_3) {
                    if (manualDoubleStep == 0) {
                        cachedPromptHead = "DOUBLE ROTATION [" + std::to_string(manualSubPuzzle) + "/" +
                                           std::to_string(curD.puzzleCount) + "] - Step 1/3: Pick [Case: LR] or [Case: RL]:";
                    } else if (manualDoubleStep == 1) {
                        cachedPromptHead = "DOUBLE ROTATION - Step 2/3: Click heavy CHILD pivot node first:";
                    } else {
                        cachedPromptHead = "DOUBLE ROTATION - Step 3/3: Click ROOT pivot node second:";
                    }
                } else if (lid == LEVEL_3_5 && bossQueryIdx < curD.kQueryCount) {
                    cachedPromptHead = "ORDER-STATISTIC BOSS [" + std::to_string(bossQueryIdx + 1) + "/" +
                                       std::to_string(curD.kQueryCount) + "] - Click #" +
                                       std::to_string(curD.kQueries[bossQueryIdx]) + " MOST VALUABLE (LARGEST) via sz";
                    if (act3LastComps > 0) {
                        cachedPromptHead += " | Last: " + std::to_string(act3LastComps) + " comps <= H=" +
                                            std::to_string(act3LastHeight) + " (sz " +
                                            std::string(act3SizeOk ? "OK" : "ERR") + ")";
                    }
                    cachedPromptHead += ":";
                } else if (lid == LEVEL_1_1) {
                    cachedPromptHead = std::string(traversalModeName(act1TravMode)) + " [" +
                                       std::to_string(act1SeqIdx) + "/" + std::to_string(act1Tree.getSize()) +
                                       "] - Click rooms in exact order:";
                } else if (lid == LEVEL_1_2) {
                    std::string preS = "PRE: ", inS = "IN: ";
                    for (int i = 0; i < curD.preorderCount; ++i) preS += std::to_string(curD.preorder[i]) + (i + 1 < curD.preorderCount ? " " : "");
                    for (int i = 0; i < curD.inorderCount; ++i)  inS += std::to_string(curD.inorder[i])  + (i + 1 < curD.inorderCount ? " " : "");
                    cachedPromptHead = preS + "   |   " + inS;
                } else if (lid == LEVEL_1_3) {
                    if (act1PostIdx < act1Tree.getSize()) {
                        cachedPromptHead = "POSTORDER HEIGHTS [" + std::to_string(act1PostIdx) + "/" +
                                           std::to_string(act1Tree.getSize()) + "] - Enter height of room " +
                                           std::to_string(act1Hl1) + " (leaf = 1):";
                    } else {
                        cachedPromptHead = "FINAL CHECK - Enter the tree's Diameter (longest path in nodes):";
                    }
                } else if (lid == LEVEL_1_4) {
                    if (act1LcaStage == 0) cachedPromptHead = "STEP 1/3 - Click Room A (any start room on the tree):";
                    else if (act1LcaStage == 1) cachedPromptHead = "STEP 1/3 - Room A=" + std::to_string(act1LcaA) + " picked; click Room B:";
                    else if (act1LcaStage == 2) cachedPromptHead = "STEP 2/3 - Click the Lowest Common Ancestor (LCA) of " + std::to_string(act1LcaA) + " & " + std::to_string(act1LcaB) + ":";
                    else cachedPromptHead = "STEP 3/3 - Trace the route between " + std::to_string(act1LcaA) + " & " + std::to_string(act1LcaB) + " in order:";
                } else if (lid == LEVEL_1_5) {
                    cachedPromptHead = "MIRROR THE VAULT - Click internal rooms bottom-up to swap children, or press Auto:";
                } else if (lid == LEVEL_2_1) {
                    int tgt = (act2TargetIdx < curD.targetKeyCount) ? curD.targetKeys[act2TargetIdx] : -1;
                    cachedPromptHead = "SEARCH HEIST - Target [" + std::to_string(tgt) + "]  |  Room [" +
                                       std::to_string(act2CurKey) + "] - Compare & pick Left / Found / Right:";
                } else if (lid == LEVEL_2_2) {
                    int insK = (act2TargetIdx < curD.targetKeyCount) ? curD.targetKeys[act2TargetIdx] : -1;
                    cachedPromptHead = "STASH THE LOOT [" + std::to_string(minInt(act2TargetIdx + 1, curD.targetKeyCount)) +
                                       "/" + std::to_string(curD.targetKeyCount) + "] - Key [" + std::to_string(insK) +
                                       "]  |  Room [" + std::to_string(act2CurKey) + "]:";
                } else if (lid == LEVEL_2_3) {
                    int delK = (act2DelStage < curD.deleteSequenceCount) ? curD.deleteSequence[act2DelStage] : -1;
                    cachedPromptHead = "STEAL & COLLAPSE [" + std::to_string(minInt(act2DelStage + 1, curD.deleteSequenceCount)) +
                                       "/" + std::to_string(curD.deleteSequenceCount) + "] - Room [" +
                                       (delK >= 0 ? std::to_string(delK) : std::string("Done")) + "] - Pick Case:";
                } else if (lid == LEVEL_2_4) {
                    int curR = (act2RqIdx < act2RqCount) ? act2RqSteps[act2RqIdx].nodeKey : -1;
                    cachedPromptHead = "RANGE RAID [" + std::to_string(curD.rangeLo) + ".." + std::to_string(curD.rangeHi) +
                                       "] (" + std::to_string(act2RqIdx) + "/" + std::to_string(act2RqCount) +
                                       ") - Room [" + (curR >= 0 ? std::to_string(curR) : std::string("Done")) +
                                       "]  |  Pruned: " + std::to_string(act2PrunedCount);
                } else if (lid == LEVEL_2_5) {
                    if (act2BossStage == 0) {
                        int insK = (act2TargetIdx < curD.targetKeyCount) ? curD.targetKeys[act2TargetIdx] : -1;
                        cachedPromptHead = "BOSS: LAZY ARCHITECT - Insert [" + std::to_string(minInt(act2TargetIdx + 1, curD.targetKeyCount)) +
                                           "/" + std::to_string(curD.targetKeyCount) + "] Key [" + std::to_string(insK) +
                                           "]  |  Room [" + std::to_string(act2CurKey) + "]  |  Height=" + std::to_string(act2Bst.getHeight());
                    } else {
                        int sk = (curD.kQueryCount > 0) ? curD.kQueries[0] : -1;
                        cachedPromptHead = "BOSS STAGE 2 - Search Key [" + std::to_string(sk) +
                                           "] down Height-" + std::to_string(act2Bst.getHeight()) +
                                           " Chain  |  Room [" + std::to_string(act2CurKey) + "]:";
                    }
                } else if (lid == LEVEL_4_1) {
                    cachedPromptHead = "ACTIVATE THE VAULT - Click bottom-up violation [" +
                                       std::to_string(maxInt(0, buildStepIdx)) + "] (or its larger child) to sift down:";
                } else if (lid == LEVEL_4_2) {
                    cachedPromptHead = "REPAIR SECURITY LOCKS - Click sabotaged root [0] (or larger child [1]):";
                } else if (lid == LEVEL_4_3) {
                    cachedPromptHead = "EXTRACT GREATEST [" + std::to_string(sortedCount) + "/" +
                                       std::to_string(curD.initialArrayCount) +
                                       "] - Click root [0] to fill ascending sorted tail:";
                } else if (lid == LEVEL_4_4) {
                    int optS = computeOptimalSteps(curD);
                    cachedPromptHead = "RACE AGAINST THE GUARDS - Steps: " + std::to_string(scoring.steps()) + "/" +
                                       std::to_string(optS) + " (" + std::to_string(optS) +
                                       " extractions, O(log n) each = Theta(n log n)):";
                } else if (lid == LEVEL_4_5) {
                    if (!act4Built) {
                        cachedPromptHead = "FINAL VAULT BOSS - Phase 1/2: Click [Build Max Heap [B]] to heapify:";
                    } else {
                        cachedPromptHead = "FINAL VAULT BOSS - Phase 2/2 [" + std::to_string(sortedCount) + "/" +
                                           std::to_string(curD.initialArrayCount) +
                                           "] - Click root [0] to fill ascending tail:";
                    }
                }
            }
            float headMaxW = (lid == LEVEL_3_4) ? 960.f : 1080.f;
            float bodyMaxW = (lid == LEVEL_3_4) ? 960.f : 1050.f;
            Ui::textFit(window, menuFont, cachedPromptHead, TXT_H2, sf::Color(0, 255, 65), 54.f, 678.f, headMaxW, true, &menuUiCache);
            Ui::textWrap(window, font, status, TXT_BODY, sf::Color(225, 245, 230), 54.f, 718.f, bodyMaxW, false, &uiCache);
            if (hintSys.wasUsed()) {
                Ui::textWrap(window, font, "HINT (-1 star cap): " + hintSys.shownText(),
                             TXT_HINT, sf::Color(0, 255, 65), 54.f, 774.f, bodyMaxW, false, &uiCache);
            }

            if (lid == LEVEL_1_1 && act1SeqIdx == 0 && !pendingWin) {
                Ui::button(window, font, "Cycle Mode [C]",
                           1220.f, 705.f, 310.f, 52.f, BTN_SECONDARY, in, 15, &uiCache);
            } else if (lid == LEVEL_1_3) {
                bool askingDiam = (act1PostIdx >= act1Tree.getSize());
                std::string valLabel = (askingDiam ? "Diam: " : "Height: ") + std::to_string(act1StepperVal);
                Ui::button(window, font, "-", 1130.f, 705.f, 60.f, 52.f, BTN_SECONDARY, in, 18, &uiCache);
                Ui::panel(window, 1200.f, 705.f, 150.f, 52.f, sf::Color(0, 255, 65), 240, &uiCache);
                Ui::textCenter(window, font, valLabel, 17, sf::Color(0, 255, 65), 1275.f, 719.f, true, &uiCache);
                Ui::button(window, font, "+", 1360.f, 705.f, 60.f, 52.f, BTN_SECONDARY, in, 18, &uiCache);
                Ui::button(window, font, "Submit", 1430.f, 705.f, 120.f, 52.f, BTN_PRIMARY, in, 15, &uiCache);
            } else if (lid == LEVEL_1_5 && !act1Mirrored && !pendingWin) {
                Ui::button(window, font, "Auto Mirror [A]",
                           1220.f, 705.f, 310.f, 52.f, BTN_SECONDARY, in, 15, &uiCache);
            } else if ((lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) && !pendingWin) {
                bool isSearchMode = (lid == LEVEL_2_1) || (lid == LEVEL_2_5 && act2BossStage == 1);
                const char* midLabel = isSearchMode ? "= Found" : "= Reject Dup";
                Ui::button(window, font, "< Go Left",  1122.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, midLabel,     1268.f, 705.f, 136.f, 52.f, BTN_PRIMARY,   in, 14, &uiCache);
                Ui::button(window, font, "Go Right >", 1414.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
            } else if (lid == LEVEL_2_3 && !pendingWin) {
                Ui::button(window, font, "Leaf",         1122.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "One-Child",    1268.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "Two-Children", 1414.f, 705.f, 136.f, 52.f, BTN_PRIMARY,   in, 13, &uiCache);
            } else if (lid == LEVEL_2_4 && !pendingWin) {
                Ui::button(window, font, "Report Room", 1122.f, 705.f, 136.f, 52.f, BTN_PRIMARY,   in, 14, &uiCache);
                Ui::button(window, font, "Prune Left",  1268.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "Prune Right", 1414.f, 705.f, 136.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
            } else if (lid == LEVEL_3_2 && !pendingWin) {
                Ui::button(window, font, "LL: Rotate Right (1)", 1122.f, 705.f, 206.f, 52.f,
                           act3PivotPicked ? BTN_PRIMARY : BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "RR: Rotate Left (2)",  1344.f, 705.f, 206.f, 52.f,
                           act3PivotPicked ? BTN_PRIMARY : BTN_SECONDARY, in, 14, &uiCache);
            } else if (lid == LEVEL_3_3 && manualDoubleStep == 0 && !pendingWin) {
                Ui::button(window, font, "Case: LR (1)", 1122.f, 705.f, 206.f, 52.f, BTN_PRIMARY,   in, 15, &uiCache);
                Ui::button(window, font, "Case: RL (2)", 1344.f, 705.f, 206.f, 52.f, BTN_SECONDARY, in, 15, &uiCache);
            } else if (lid == LEVEL_3_4 && !pendingWin) {
                Ui::button(window, font, "LL (1)",   1038.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "RR (2)",   1142.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "LR (3)",   1246.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "RL (4)",   1350.f, 705.f, 96.f, 52.f, BTN_SECONDARY, in, 14, &uiCache);
                Ui::button(window, font, "None (N)", 1454.f, 705.f, 96.f, 52.f, BTN_PRIMARY,   in, 14, &uiCache);
            } else if (lid == LEVEL_4_5 && !act4Built) {
                Ui::button(window, font, "Build Max Heap [B]",
                           1220.f, 705.f, 310.f, 52.f, BTN_PRIMARY, in, 15, &uiCache);
            }

            Ui::textFit(window, font, cachedHudCtrlLine, TXT_HINT, Ui::textMuted(), 54.f, 832.f, 1480.f, false, &uiCache);

            // Mission Briefing Modal (Part B)
            if (showingBriefing) {
                if (vaultBgSprite.has_value()) {
                    window.draw(*vaultBgSprite);
                }
                sf::RectangleShape& bd = uiCache.rect;
                bd.setSize({static_cast<float>(W), static_cast<float>(H)});
                bd.setOrigin({0.f, 0.f});
                bd.setPosition({0.f, 0.f});
                bd.setOutlineThickness(0.f);
                bd.setFillColor(sf::Color(4, 12, 7, 175));
                window.draw(bd);

                if (cachedBriefLid != lid || cachedBriefCap != alarmCap) {
                    cachedBriefLid = lid;
                    cachedBriefCap = alarmCap;
                    cachedBriefAct = "MISSION BRIEFING // ACT " + std::to_string(levels[currentLevelIdx].act);
                    cachedBriefObj = levels[currentLevelIdx].objective;
                    cachedBriefInt = levels[currentLevelIdx].controls;
                }

                const sf::Color brBright(0, 255, 65);
                float bw = 820.f, bh = 420.f;
                float bx = static_cast<float>(W) * 0.5f - bw * 0.5f;
                float by = static_cast<float>(H) * 0.5f - bh * 0.5f;

                // Layered glow + dark terminal panel
                Ui::panel(window, bx, by, bw, bh, brBright, 236, &uiCache);

                // Cut/notched corner accents
                const float cLen = 18.f, cThk = 3.5f;
                sf::RectangleShape bc1({cLen, cThk}); bc1.setPosition({bx - 1.f, by - 1.f}); bc1.setFillColor(brBright); window.draw(bc1);
                sf::RectangleShape bc2({cThk, cLen}); bc2.setPosition({bx - 1.f, by - 1.f}); bc2.setFillColor(brBright); window.draw(bc2);
                sf::RectangleShape bc3({cLen, cThk}); bc3.setPosition({bx + bw - cLen + 1.f, by - 1.f}); bc3.setFillColor(brBright); window.draw(bc3);
                sf::RectangleShape bc4({cThk, cLen}); bc4.setPosition({bx + bw - cThk + 1.f, by - 1.f}); bc4.setFillColor(brBright); window.draw(bc4);
                sf::RectangleShape bc5({cLen, cThk}); bc5.setPosition({bx - 1.f, by + bh - cThk + 1.f}); bc5.setFillColor(brBright); window.draw(bc5);
                sf::RectangleShape bc6({cThk, cLen}); bc6.setPosition({bx - 1.f, by + bh - cLen + 1.f}); bc6.setFillColor(brBright); window.draw(bc6);
                sf::RectangleShape bc7({cLen, cThk}); bc7.setPosition({bx + bw - cLen + 1.f, by + bh - cThk + 1.f}); bc7.setFillColor(brBright); window.draw(bc7);
                sf::RectangleShape bc8({cThk, cLen}); bc8.setPosition({bx + bw - cThk + 1.f, by + bh - cLen + 1.f}); bc8.setFillColor(brBright); window.draw(bc8);

                // Header + extending thin green line
                Ui::text(window, menuFont, cachedBriefAct, 14, brBright, bx + 40.f, by + 26.f, true, &uiCache);
                float hdrW = Ui::textWidth(menuFont, cachedBriefAct, 14, true);
                float lineX = bx + 54.f + hdrW;
                if (lineX < bx + bw - 40.f) {
                    sf::RectangleShape hdrLine({(bx + bw - 40.f) - lineX, 1.f});
                    hdrLine.setPosition({lineX, by + 35.f});
                    hdrLine.setFillColor(sf::Color(0, 255, 65, 160));
                    window.draw(hdrLine);
                }

                // Level title
                Ui::textFit(window, menuFont, levels[currentLevelIdx].title, 32, brBright,
                            bx + 40.f, by + 58.f, bw - 80.f, true, &uiCache);

                // Row 1: Target icon + OBJECTIVE: + objective text
                float r1Y = by + 132.f;
                float iconX = bx + 50.f;
                sf::CircleShape tgtOuter(10.f);
                tgtOuter.setOrigin({10.f, 10.f});
                tgtOuter.setPosition({iconX, r1Y + 10.f});
                tgtOuter.setFillColor(sf::Color::Transparent);
                tgtOuter.setOutlineColor(brBright);
                tgtOuter.setOutlineThickness(2.f);
                window.draw(tgtOuter);
                sf::CircleShape tgtDot(3.5f);
                tgtDot.setOrigin({3.5f, 3.5f});
                tgtDot.setPosition({iconX, r1Y + 10.f});
                tgtDot.setFillColor(brBright);
                window.draw(tgtDot);

                Ui::text(window, menuFont, "OBJECTIVE:", 15, brBright, bx + 76.f, r1Y, true, &uiCache);
                float objH = Ui::textWrap(window, font, cachedBriefObj, 15, sf::Color::White,
                                          bx + 216.f, r1Y, bw - 256.f, false, &uiCache);

                // Divider line
                float divY = r1Y + ((objH > 26.f) ? objH + 14.f : 38.f);
                sf::RectangleShape divLine({bw - 80.f, 1.f});
                divLine.setPosition({bx + 40.f, divY});
                divLine.setFillColor(sf::Color(0, 255, 65, 90));
                window.draw(divLine);

                // Row 2: Info icon + INTERACTION: + controls text
                float r2Y = divY + 20.f;
                sf::CircleShape infoCirc(10.f);
                infoCirc.setOrigin({10.f, 10.f});
                infoCirc.setPosition({iconX, r2Y + 10.f});
                infoCirc.setFillColor(brBright);
                window.draw(infoCirc);
                Ui::textCenter(window, menuFont, "i", 14, sf::Color(4, 14, 6), iconX, r2Y + 1.f, true, &uiCache);
                Ui::text(window, menuFont, "INTERACTION:", 15, brBright, bx + 76.f, r2Y, true, &uiCache);
                float intH = Ui::textWrap(window, font, cachedBriefInt, 15, sf::Color::White,
                                          bx + 216.f, r2Y, bw - 256.f, false, &uiCache);

                // Row 3: Warning icon + ALARM CAPACITY: + capacity strikes
                float r3Y = r2Y + ((intH > 26.f) ? intH + 16.f : 42.f);
                sf::ConvexShape warnTri(3);
                warnTri.setPoint(0, {iconX, r3Y});
                warnTri.setPoint(1, {iconX - 10.f, r3Y + 19.f});
                warnTri.setPoint(2, {iconX + 10.f, r3Y + 19.f});
                warnTri.setFillColor(brBright);
                window.draw(warnTri);
                Ui::textCenter(window, menuFont, "!", 13, sf::Color(4, 14, 6), iconX, r3Y + 3.f, true, &uiCache);
                Ui::text(window, menuFont, "ALARM CAPACITY:", 15, brBright, bx + 76.f, r3Y, true, &uiCache);
                float alarmLblW = Ui::textWidth(menuFont, "ALARM CAPACITY:", 15, true);
                Ui::text(window, font, std::to_string(alarmCap) + " strikes", 15, sf::Color::White,
                         bx + 90.f + alarmLblW, r3Y, false, &uiCache);

                Ui::button(window, menuFont, "START MISSION [Enter]", static_cast<float>(W) * 0.5f - 150.f,
                           by + bh - 74.f, 300.f, 48.f, BTN_PRIMARY, in, 16, &uiCache);
            }
        }

        // 4. RESULT SCREEN
        if (currentScreen == SCREEN_RESULT) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            bool failed = (currentLevelIdx >= 0 && levels[currentLevelIdx].stars == 0);
            float cardW = 760.f, cardH = 520.f;
            float cardX = W * 0.5f - cardW * 0.5f, cardY = 170.f;
            Ui::panel(window, cardX, cardY, cardW, cardH, failed ? Ui::red() : sf::Color(0, 255, 65), 240, &uiCache);

            Ui::textCenter(window, font,
                           failed ? "ALARM TRIGGERED - LEVEL FAILED" : "LEVEL COMPLETE",
                           36, failed ? Ui::red() : sf::Color(0, 255, 65), W * 0.5f, cardY + 34.f, true, &uiCache);

            if (currentLevelIdx >= 0) {
                int bc = animCtrl.getLog().getBatchCount();
                int ec = animCtrl.getLog().getTotalEventCount();
                int st = levels[currentLevelIdx].stars;
                if (cachedResIdx != currentLevelIdx || cachedResScore != score || cachedResAlarm != alarmSteps ||
                    cachedResBatches != bc || cachedResEvs != ec || cachedResStars != st) {
                    cachedResIdx = currentLevelIdx;
                    cachedResScore = score;
                    cachedResAlarm = alarmSteps;
                    cachedResBatches = bc;
                    cachedResEvs = ec;
                    cachedResStars = st;
                    cachedResStats = "Score: " + std::to_string(score) +
                                     "   |   Alarm Steps: " + std::to_string(alarmSteps) +
                                     "   |   Ops Logged: " + std::to_string(bc) +
                                     " (" + std::to_string(ec) + " events)";
                }
                Ui::textCenter(window, font, levels[currentLevelIdx].title, TXT_H2, sf::Color::White,
                               W * 0.5f, cardY + 96.f, true, &uiCache);
                Ui::stars(window, W * 0.5f, cardY + 166.f, st, 3, 22.f, &uiCache);
                Ui::textCenter(window, font, cachedResStats, TXT_BODY, Ui::textBody(), W * 0.5f, cardY + 230.f, false, &uiCache);
            }

            Ui::textWrap(window, font, status, TXT_BODY,
                         failed ? sf::Color(255, 175, 175) : Ui::green(),
                         cardX + 60.f, cardY + 280.f, cardW - 120.f, false, &uiCache);

            Ui::button(window, font, "Retry Level [R]", W * 0.5f - 230.f, cardY + 365.f, 210.f, 52.f, BTN_SECONDARY, in, 16, &uiCache);
            Ui::button(window, font, "Next Level [N]",  W * 0.5f + 20.f,  cardY + 365.f, 210.f, 52.f, BTN_PRIMARY, in, 16, &uiCache);
            Ui::button(window, font, "Campaign Map [Esc]", W * 0.5f - 120.f, cardY + 436.f, 240.f, 42.f, BTN_GHOST, in, 14, &uiCache);
        }

        // 5. SANDBOX MODE
        if (currentScreen == SCREEN_SANDBOX) {
            Ui::screenHeader(window, font, "SANDBOX SIMULATOR // AVL TREE + MAX HEAP",
                             "Interactive free-play over real AVLTree & MaxHeap instances with EventLog & Undo.", &uiCache);
            Ui::button(window, font, "Back to Menu [Esc]", 1370.f, 26.f, 190.f, 38.f, BTN_GHOST, in, 13, &uiCache);

            Ui::panel(window, 30.f,  95.f, 560.f, 540.f, sf::Color(0, 255, 65), 248, &uiCache);
            Ui::panel(window, 605.f, 95.f, 565.f, 540.f, sf::Color(0, 255, 65), 248, &uiCache);

            if (!animCtrl.hasEvents() && needLayout) {
                layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
                layoutHeapForScreen(true, 605.f, 95.f, 565.f, 540.f);
                layoutDirty = false;
            } else if (needLayout) {
                if (animCtrl.getRenderLayout().structureType != STRUCT_AVL) layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
                if (animCtrl.getRenderLayout().structureType != STRUCT_MAX_HEAP) layoutHeapForScreen(true, 605.f, 95.f, 565.f, 540.f);
                layoutDirty = false;
            }
            bool animAvl  = (animCtrl.hasEvents() && animCtrl.getRenderLayout().structureType == STRUCT_AVL);
            bool animHeap = (animCtrl.hasEvents() && animCtrl.getRenderLayout().structureType == STRUCT_MAX_HEAP);
            const LayoutResult& sbAvlL  = animAvl  ? animCtrl.getRenderLayout() : avlLayout;
            const LayoutResult& sbHeapL = animHeap ? animCtrl.getRenderLayout() : heapLayout;

            RenderHighlightState sbAvlHl, sbHeapHl;
            sbAvlHl.showBadges  = showNodeBadges;
            sbHeapHl.showBadges = showNodeBadges;
            if (animAvl)  animCtrl.fillHighlightState(sbAvlHl);
            if (animHeap) animCtrl.fillHighlightState(sbHeapHl);
            sf::Vector2f wPosAvl(0.f, 0.f), wPosHeap(0.f, 0.f);
            if (mapHoverToCam(sbAvlCam, in.mx, in.my, wPosAvl)) {
                int sbAvlHov = sbAvlL.hitTestNode(wPosAvl.x, wPosAvl.y);
                if (sbAvlHov >= 0) sbAvlHl.hoveredNodeId = sbAvlL.nodes[sbAvlHov].id;
            }
            if (mapHoverToCam(sbHeapCam, in.mx, in.my, wPosHeap)) {
                int sbHeapHov = sbHeapL.hitTestNode(wPosHeap.x, wPosHeap.y);
                if (sbHeapHov >= 0) sbHeapHl.hoveredNodeId = sbHeapL.nodes[sbHeapHov].id;
                else {
                    int cIdx = sbHeapL.hitTestCell(wPosHeap.x, wPosHeap.y);
                    if (cIdx >= 0) sbHeapHl.hoveredNodeId = cIdx;
                }
            }

            window.setView(sbAvlCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, sbAvlCam.view);
            guiRenderer.drawTreeLayout(window, font, sbAvlL, sbAvlHl, true, false);

            window.setView(sbHeapCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, sbHeapCam.view);
            guiRenderer.drawHeapLayout(window, font, sbHeapL, sbHeapHl, 619.f, 555.f);

            window.setView(window.getDefaultView());
            Ui::text(window, font, sbAvlCam.zoomText(), 10, Ui::textMuted(), 40.f, 100.f, false, &uiCache);
            Ui::text(window, font, sbHeapCam.zoomText(), 10, Ui::textMuted(), 615.f, 100.f, false, &uiCache);
            drawCamTooltip(sbAvlCam, sbAvlL, sbAvlHl.hoveredNodeId);
            drawCamTooltip(sbHeapCam, sbHeapL, sbHeapHl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, 95.f, LOG_W, 540.f,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, 95.f, LOG_W, 540.f,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            Ui::panel(window, 30.f, 650.f, 1540.f, 220.f, sf::Color(60, 85, 125), 235, &uiCache);
            Ui::button(window, font, "[I] AVL Insert",   54.f,   674.f, 190.f, 46.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[D] AVL Delete",   264.f,  674.f, 190.f, 46.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[K] AVL kth",      474.f,  674.f, 190.f, 46.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[E] Heap Extract", 684.f,  674.f, 200.f, 46.f, BTN_PRIMARY, in, 14, &uiCache);
            Ui::button(window, font, "[Z] Undo",         904.f,  674.f, 170.f, 46.f, undoStack.isEmpty() ? BTN_DISABLED : BTN_GHOST, in, 14, &uiCache);
            Ui::button(window, font, "[R] Reset",        1094.f, 674.f, 170.f, 46.f, BTN_DANGER, in, 14, &uiCache);

            Ui::textFit(window, font, status, TXT_BODY, sf::Color(0, 255, 65), 54.f, 746.f, 1480.f, false, &uiCache);
            Ui::textFit(window, font, "Shortcuts: [I] Insert AVL  |  [D] Delete AVL  |  [K] kth  |  [E] Extract Heap  |  [Z] Undo  |  [Space] Play/Pause  |  [Right] Step",
                        TXT_HINT, Ui::textMuted(), 54.f, 820.f, 1480.f, false, &uiCache);
        }

        // 6. ACT 1 EXPLORER DEMO
        if (currentScreen == SCREEN_ACT1_DEMO) {
            if (needLayout || cachedAct1Stats.empty()) {
                cachedAct1Stats = "Nodes: " + std::to_string(act1Tree.getSize()) +
                                  "  |  Height: " + std::to_string(act1Tree.height()) +
                                  "  |  Diameter: " + std::to_string(act1Tree.diameter()) +
                                  "  |  Symmetric: " + (act1Tree.isSymmetric() ? "YES" : "NO");
            }
            Ui::screenHeader(window, font, "ACT 1 EXPLORER // GENERAL BINARY TREE", cachedAct1Stats, &uiCache);
            Ui::button(window, font, "Campaign Map [Esc]", 1350.f, 26.f, 210.f, 38.f, BTN_GHOST, in, 13, &uiCache);

            Ui::panel(window, 30.f, 110.f, 1140.f, 470.f, sf::Color(0, 255, 65), 248, &uiCache);
            if (!animCtrl.hasEvents() && needLayout) {
                layoutBtForScreen(true);
                layoutDirty = false;
            }
            const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : btLayout;
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.showBadges = showNodeBadges;
            sf::Vector2f wPos(0.f, 0.f);
            if (mapHoverToCam(demoCam, in.mx, in.my, wPos)) {
                int hovIdx = drawL.hitTestNode(wPos.x, wPos.y);
                if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
            }
            if (animCtrl.hasEvents()) {
                animCtrl.fillHighlightState(hl);
            } else {
                int hlIdx1 = drawL.findNodeIndexByKey(act1Hl1);
                int hlIdx2 = drawL.findNodeIndexByKey(act1Hl2);
                if (hlIdx1 >= 0) { hl.primaryNodeId = drawL.nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                if (hlIdx2 >= 0) hl.secondaryNodeId = drawL.nodes[hlIdx2].id;
            }
            window.setView(demoCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, demoCam.view);
            guiRenderer.drawTreeLayout(window, font, drawL, hl, false, true);
            window.setView(window.getDefaultView());
            Ui::text(window, font, demoCam.zoomText(), 11, Ui::textMuted(), 42.f, 116.f, false, &uiCache);
            drawCamTooltip(demoCam, drawL, hl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            Ui::panel(window, 30.f, 600.f, 1540.f, 270.f, sf::Color(60, 85, 125), 235, &uiCache);
            Ui::button(window, font, "[T] Traversals",    54.f,   620.f, 200.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[M] Mirror Tree",   274.f,  620.f, 200.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[R] Rebuild Pre+In",494.f,  620.f, 210.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[H] Height & Diam", 724.f,  620.f, 210.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[L] LCA & Path",    954.f,  620.f, 190.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[S] Symmetry",      1164.f, 620.f, 180.f, 44.f, BTN_GHOST, in, 14, &uiCache);
            Ui::button(window, font, "[Z] Reset",         1364.f, 620.f, 170.f, 44.f, BTN_DANGER, in, 14, &uiCache);

            Ui::textFit(window, font, status, TXT_BODY, sf::Color(0, 255, 65), 54.f, 690.f, 1480.f, false, &uiCache);
            if (!act1TraversalStr.empty()) {
                Ui::textWrap(window, font, act1TraversalStr, TXT_HINT, sf::Color(120, 235, 145), 54.f, 728.f, 1480.f, false, &uiCache);
            }
            Ui::textFit(window, font, "Click any node to compute root-to-node path  |  [Space] Play/Pause  |  [Right] Step",
                        TXT_HINT, Ui::textMuted(), 54.f, 830.f, 1480.f, false, &uiCache);
        }

        // 7. ACT 2 EXPLORER DEMO
        if (currentScreen == SCREEN_ACT2_DEMO) {
            if (needLayout || cachedAct2Stats.empty()) {
                cachedAct2Stats = "Nodes: " + std::to_string(act2Bst.getSize()) +
                                  "  |  Height: " + std::to_string(act2Bst.getHeight()) +
                                  "  |  Valid BST: " + (act2Bst.isValidBST() ? "YES" : "NO");
            }
            Ui::screenHeader(window, font, "ACT 2 EXPLORER // BINARY SEARCH TREE", cachedAct2Stats, &uiCache);
            Ui::button(window, font, "Campaign Map [Esc]", 1350.f, 26.f, 210.f, 38.f, BTN_GHOST, in, 13, &uiCache);

            Ui::panel(window, 30.f, 110.f, 1140.f, 470.f, sf::Color(0, 255, 65), 248, &uiCache);
            if (!animCtrl.hasEvents() && needLayout) {
                layoutBstForScreen(true);
                layoutDirty = false;
            }
            const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : bstLayout;
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.showBadges = showNodeBadges;
            sf::Vector2f wPos(0.f, 0.f);
            if (mapHoverToCam(demoCam, in.mx, in.my, wPos)) {
                int hovIdx = drawL.hitTestNode(wPos.x, wPos.y);
                if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
            }
            if (animCtrl.hasEvents()) {
                animCtrl.fillHighlightState(hl);
            } else {
                int hlIdx1 = drawL.findNodeIndexByKey(act2Hl1);
                int hlIdx2 = drawL.findNodeIndexByKey(act2Hl2);
                if (hlIdx1 >= 0) { hl.primaryNodeId = drawL.nodes[hlIdx1].id; hl.primaryStyle = HL_VISIT; }
                if (hlIdx2 >= 0) hl.secondaryNodeId = drawL.nodes[hlIdx2].id;
            }
            window.setView(demoCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, demoCam.view);
            guiRenderer.drawTreeLayout(window, font, drawL, hl, false, true);
            window.setView(window.getDefaultView());
            Ui::text(window, font, demoCam.zoomText(), 11, Ui::textMuted(), 42.f, 116.f, false, &uiCache);
            drawCamTooltip(demoCam, drawL, hl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, 110.f, LOG_W, 470.f,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            Ui::panel(window, 30.f, 600.f, 1540.f, 270.f, sf::Color(60, 85, 125), 235, &uiCache);
            Ui::button(window, font, "[I] Insert",      54.f,   620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[D] Delete",      222.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[S] Search",      390.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[M] Min/Max",     558.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[P] Succ/Pred",   726.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[R] Range",       894.f,  620.f, 155.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[K] kth",         1062.f, 620.f, 145.f, 44.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "[C] Chain",       1220.f, 620.f, 145.f, 44.f, BTN_GHOST, in, 14, &uiCache);
            Ui::button(window, font, "[Z] Reset",       1378.f, 620.f, 156.f, 44.f, BTN_DANGER, in, 14, &uiCache);

            Ui::textFit(window, font, status, TXT_BODY, sf::Color(0, 255, 65), 54.f, 690.f, 1480.f, false, &uiCache);
            if (!act2ExtraInfo.empty()) {
                Ui::textFit(window, font, act2ExtraInfo, TXT_HINT, sf::Color(120, 235, 145), 54.f, 730.f, 1480.f, false, &uiCache);
            }
            Ui::textFit(window, font, "Click any BST node to search & inspect Pred/Succ  |  [Space] Play/Pause  |  [Right] Step",
                        TXT_HINT, Ui::textMuted(), 54.f, 830.f, 1480.f, false, &uiCache);
        }

        // 8. REAL PERSISTENT LEADERBOARD (Item 3)
        if (currentScreen == SCREEN_LEADERBOARD) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color lbBright(0, 255, 65);
            const sf::Color lbDim(74, 122, 74);
            const sf::Color lbSubtle(120, 195, 120);

            Ui::text(window, menuFont, "OPERATOR LEADERBOARD", 34, lbBright, 50.f, 24.f, true, &uiCache);
            Ui::text(window, menuFont, "Recorded campaign heist scores loaded from scores.txt", 14, lbSubtle, 50.f, 70.f, false, &uiCache);
            sf::RectangleShape hdrLine({1150.f, 1.f});
            hdrLine.setPosition({50.f, 102.f});
            hdrLine.setFillColor(sf::Color(0, 255, 65, 140));
            window.draw(hdrLine);

            float mw = 920.f, mh = 600.f;
            float mx0 = static_cast<float>(W) * 0.5f - mw * 0.5f, my0 = 130.f;
            sf::RectangleShape lbPanel({mw, mh});
            lbPanel.setPosition({mx0, my0});
            lbPanel.setFillColor(sf::Color(6, 14, 10, 228));
            lbPanel.setOutlineColor(lbBright);
            lbPanel.setOutlineThickness(1.5f);
            window.draw(lbPanel);

            // Corner accents
            const float cLen = 14.f, cThk = 3.f;
            sf::RectangleShape lc1({cLen, cThk}); lc1.setPosition({mx0 - 1.f, my0 - 1.f}); lc1.setFillColor(lbBright); window.draw(lc1);
            sf::RectangleShape lc2({cThk, cLen}); lc2.setPosition({mx0 - 1.f, my0 - 1.f}); lc2.setFillColor(lbBright); window.draw(lc2);
            sf::RectangleShape lc3({cLen, cThk}); lc3.setPosition({mx0 + mw - cLen + 1.f, my0 - 1.f}); lc3.setFillColor(lbBright); window.draw(lc3);
            sf::RectangleShape lc4({cThk, cLen}); lc4.setPosition({mx0 + mw - cThk + 1.f, my0 - 1.f}); lc4.setFillColor(lbBright); window.draw(lc4);
            sf::RectangleShape lc5({cLen, cThk}); lc5.setPosition({mx0 - 1.f, my0 + mh - cThk + 1.f}); lc5.setFillColor(lbBright); window.draw(lc5);
            sf::RectangleShape lc6({cThk, cLen}); lc6.setPosition({mx0 - 1.f, my0 + mh - cLen + 1.f}); lc6.setFillColor(lbBright); window.draw(lc6);
            sf::RectangleShape lc7({cLen, cThk}); lc7.setPosition({mx0 + mw - cLen + 1.f, my0 + mh - cThk + 1.f}); lc7.setFillColor(lbBright); window.draw(lc7);
            sf::RectangleShape lc8({cThk, cLen}); lc8.setPosition({mx0 + mw - cThk + 1.f, my0 + mh - cLen + 1.f}); lc8.setFillColor(lbBright); window.draw(lc8);

            if (scoreLog.getCount() == 0) {
                Ui::textCenter(window, menuFont, "No heists recorded yet", 24, lbBright,
                               static_cast<float>(W) * 0.5f, my0 + 200.f, true, &uiCache);
                Ui::textCenter(window, menuFont, "Complete any campaign level to record your score here.",
                               15, lbSubtle, static_cast<float>(W) * 0.5f, my0 + 255.f, false, &uiCache);
            } else {
                Ui::text(window, menuFont, "RANK",  14, lbBright, mx0 + 36.f,  my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "LEVEL", 14, lbBright, mx0 + 120.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "SCORE", 14, lbBright, mx0 + 520.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "ALARM", 14, lbBright, mx0 + 660.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, menuFont, "STARS", 14, lbBright, mx0 + 785.f, my0 + 24.f, true, &uiCache);

                sf::RectangleShape thLine({mw - 48.f, 1.f});
                thLine.setPosition({mx0 + 24.f, my0 + 52.f});
                thLine.setFillColor(sf::Color(0, 255, 65, 90));
                window.draw(thLine);

                int rows = minInt(10, scoreLog.getCount());
                if (cachedLbCount != scoreLog.getCount()) {
                    cachedLbCount = scoreLog.getCount();
                    for (int r = 0; r < rows; ++r) {
                        int idx = scoreLog.topIndex(r);
                        if (idx < 0) break;
                        const ScoreEntry& e = scoreLog.get(idx);
                        cachedLbRank[r]  = "#" + std::to_string(r + 1);
                        cachedLbScore[r] = std::to_string(e.score) + " pts";
                        cachedLbAlarm[r] = std::to_string(e.alarm);
                    }
                }
                for (int r = 0; r < rows; ++r) {
                    int idx = scoreLog.topIndex(r);
                    if (idx < 0) break;
                    const ScoreEntry& e = scoreLog.get(idx);
                    float ry = my0 + 68.f + static_cast<float>(r) * 46.f;
                    Ui::text(window, menuFont, cachedLbRank[r], 15, lbBright, mx0 + 36.f, ry, true, &uiCache);
                    Ui::textFit(window, menuFont, e.levelTitle, 15, sf::Color(220, 240, 225), mx0 + 120.f, ry, 380.f, false, &uiCache);
                    Ui::text(window, menuFont, cachedLbScore[r], 15, lbBright, mx0 + 520.f, ry, false, &uiCache);
                    Ui::text(window, menuFont, cachedLbAlarm[r], 15, sf::Color(220, 240, 225), mx0 + 675.f, ry, false, &uiCache);
                    Ui::stars(window, mx0 + 820.f, ry + 9.f, e.stars, 3, 7.f, &uiCache);

                    if (r < rows - 1) {
                        sf::RectangleShape rowLine({mw - 48.f, 1.f});
                        rowLine.setPosition({mx0 + 24.f, ry + 34.f});
                        rowLine.setFillColor(sf::Color(0, 255, 65, 35));
                        window.draw(rowLine);
                    }
                }
            }

            bool endHov = Ui::inside(in.mx, in.my, static_cast<float>(W) * 0.5f - 310.f, 770.f, 290.f, 48.f);
            sf::RectangleShape endBtn({290.f, 48.f});
            endBtn.setPosition({static_cast<float>(W) * 0.5f - 310.f, 770.f});
            endBtn.setFillColor(endHov ? sf::Color(10, 40, 18, 235) : sf::Color(6, 16, 10, 225));
            endBtn.setOutlineColor(endHov ? lbBright : lbDim);
            endBtn.setOutlineThickness(endHov ? 2.f : 1.5f);
            window.draw(endBtn);
            Ui::textCenter(window, menuFont, "Endless Leaderboard [E]", 15,
                           endHov ? sf::Color::White : lbBright,
                           static_cast<float>(W) * 0.5f - 165.f, 784.f, endHov, &uiCache);

            bool retHov = Ui::inside(in.mx, in.my, static_cast<float>(W) * 0.5f + 20.f, 770.f, 290.f, 48.f);
            sf::RectangleShape retBtn({290.f, 48.f});
            retBtn.setPosition({static_cast<float>(W) * 0.5f + 20.f, 770.f});
            retBtn.setFillColor(retHov ? sf::Color(12, 54, 22, 240) : sf::Color(8, 28, 14, 230));
            retBtn.setOutlineColor(lbBright);
            retBtn.setOutlineThickness(2.f);
            window.draw(retBtn);
            Ui::textCenter(window, menuFont, "Return to Main Menu [ESC]", 15,
                           retHov ? lbBright : sf::Color::White,
                           static_cast<float>(W) * 0.5f + 165.f, 784.f, true, &uiCache);
        }

        // 9. REAL WORKING SETTINGS (Item 4)
        if (currentScreen == SCREEN_SETTINGS) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color setBright(0, 255, 65);
            const sf::Color setDim(74, 122, 74);
            const sf::Color setSubtle(120, 195, 120);

            Ui::text(window, menuFont, "SYSTEM SETTINGS", 34, setBright, 50.f, 24.f, true, &uiCache);
            Ui::text(window, menuFont, "Every setting below takes effect immediately across the interface.", 14, setSubtle, 50.f, 70.f, false, &uiCache);
            sf::RectangleShape hdrLine({1150.f, 1.f});
            hdrLine.setPosition({50.f, 102.f});
            hdrLine.setFillColor(sf::Color(0, 255, 65, 140));
            window.draw(hdrLine);

            float mw = 760.f, mh = 560.f;
            float sx0 = static_cast<float>(W) * 0.5f - mw * 0.5f, sy0 = 140.f;
            sf::RectangleShape setPanel({mw, mh});
            setPanel.setPosition({sx0, sy0});
            setPanel.setFillColor(sf::Color(6, 14, 10, 228));
            setPanel.setOutlineColor(setBright);
            setPanel.setOutlineThickness(1.5f);
            window.draw(setPanel);

            // Corner accents
            const float cLen = 14.f, cThk = 3.f;
            sf::RectangleShape sc1({cLen, cThk}); sc1.setPosition({sx0 - 1.f, sy0 - 1.f}); sc1.setFillColor(setBright); window.draw(sc1);
            sf::RectangleShape sc2({cThk, cLen}); sc2.setPosition({sx0 - 1.f, sy0 - 1.f}); sc2.setFillColor(setBright); window.draw(sc2);
            sf::RectangleShape sc3({cLen, cThk}); sc3.setPosition({sx0 + mw - cLen + 1.f, sy0 - 1.f}); sc3.setFillColor(setBright); window.draw(sc3);
            sf::RectangleShape sc4({cThk, cLen}); sc4.setPosition({sx0 + mw - cThk + 1.f, sy0 - 1.f}); sc4.setFillColor(setBright); window.draw(sc4);
            sf::RectangleShape sc5({cLen, cThk}); sc5.setPosition({sx0 - 1.f, sy0 + mh - cThk + 1.f}); sc5.setFillColor(setBright); window.draw(sc5);
            sf::RectangleShape sc6({cThk, cLen}); sc6.setPosition({sx0 - 1.f, sy0 + mh - cLen + 1.f}); sc6.setFillColor(setBright); window.draw(sc6);
            sf::RectangleShape sc7({cLen, cThk}); sc7.setPosition({sx0 + mw - cLen + 1.f, sy0 + mh - cThk + 1.f}); sc7.setFillColor(setBright); window.draw(sc7);
            sf::RectangleShape sc8({cThk, cLen}); sc8.setPosition({sx0 + mw - cThk + 1.f, sy0 + mh - cLen + 1.f}); sc8.setFillColor(setBright); window.draw(sc8);

            if (cachedSetFpsVal != targetFps) {
                cachedSetFpsVal = targetFps;
                cachedSetFpsLimStr = "Frame Rate Limit: " + std::to_string(targetFps) + " FPS";
                cachedSetFpsBtnStr = "Frame Rate Cap: " + std::to_string(targetFps) + " FPS (Click to toggle 60/120)";
            }

            float bw = 560.f, bh = 52.f, bx = static_cast<float>(W) * 0.5f - bw * 0.5f;
            Ui::text(window, menuFont, cachedSetResStr, 15, setSubtle, bx, sy0 + 36.f, false, &uiCache);
            Ui::text(window, menuFont, cachedSetFpsLimStr, 15, setSubtle, bx, sy0 + 68.f, false, &uiCache);

            auto drawSettingToggle = [&](const std::string& label, float yOff, bool active) {
                float by = sy0 + yOff;
                bool hov = Ui::inside(in.mx, in.my, bx, by, bw, bh);
                sf::RectangleShape box({bw, bh});
                box.setPosition({bx, by});
                box.setFillColor(hov ? sf::Color(10, 38, 18, 235) : sf::Color(5, 16, 9, 220));
                box.setOutlineColor(hov ? setBright : (active ? sf::Color(0, 255, 65, 160) : setDim));
                box.setOutlineThickness(hov ? 2.f : 1.f);
                window.draw(box);
                Ui::textCenter(window, menuFont, label, 15,
                               hov ? setBright : (active ? sf::Color(220, 245, 225) : setSubtle),
                               bx + bw * 0.5f, by + 16.f, hov, &uiCache);
            };

            drawSettingToggle(soundEnabled ? "Audio FX Feedback: ENABLED" : "Audio FX Feedback: MUTED", 125.f, soundEnabled);
            drawSettingToggle(showNodeBadges ? "Node Badges (h / BF / sz): VISIBLE" : "Node Badges (h / BF / sz): HIDDEN", 195.f, showNodeBadges);
            drawSettingToggle(showBgGrid ? "Blueprint Grid Overlay: VISIBLE" : "Blueprint Grid Overlay: HIDDEN", 265.f, showBgGrid);
            drawSettingToggle(autoPlayEvents ? "Operation Animation Mode: AUTO-PLAY" : "Operation Animation Mode: STEP-BY-STEP", 335.f, autoPlayEvents);
            drawSettingToggle(cachedSetFpsBtnStr, 405.f, true);

            bool retHov = Ui::inside(in.mx, in.my, static_cast<float>(W) * 0.5f - 150.f, 740.f, 300.f, 48.f);
            sf::RectangleShape retBtn({300.f, 48.f});
            retBtn.setPosition({static_cast<float>(W) * 0.5f - 150.f, 740.f});
            retBtn.setFillColor(retHov ? sf::Color(12, 46, 20, 235) : sf::Color(6, 18, 10, 225));
            retBtn.setOutlineColor(setBright);
            retBtn.setOutlineThickness(retHov ? 2.f : 1.5f);
            window.draw(retBtn);
            Ui::textCenter(window, menuFont, "Return to Main Menu [ESC]", 15,
                           retHov ? sf::Color::White : setBright,
                           static_cast<float>(W) * 0.5f, 754.f, retHov, &uiCache);
        }

        // 10. ENDLESS SURVIVAL MODE (Step F)
        if (currentScreen == SCREEN_ENDLESS) {
            int opsNow = endlessSession.getOpsCompleted();
            int nodesNow = avl.getSize();
            float winSec = endlessSession.getOpWindowSec();
            float remSec = endlessSession.getOpTimeRemaining();
            float opsMin = EndlessSession::computeOpsPerMin(winSec);
            int rateTenths = static_cast<int>(opsMin * 10.0f + 0.5f);

            if (cachedEndlessOps != opsNow || cachedEndlessNodes != nodesNow || cachedEndlessRateTenths != rateTenths) {
                cachedEndlessOps = opsNow;
                cachedEndlessNodes = nodesNow;
                cachedEndlessRateTenths = rateTenths;
                cachedEndlessSub = "Seed: " + std::to_string(endlessSession.getInitialSeed()) +
                                   "  |  Ops: " + std::to_string(opsNow) +
                                   "  |  Nodes: " + std::to_string(nodesNow) + " (H=" + std::to_string(avl.getHeight()) +
                                   ")  |  Rate: " + std::to_string(rateTenths / 10) + "." +
                                   std::to_string(rateTenths % 10) + " ops/min";
            }

            Ui::textFit(window, font, "ENDLESS SURVIVAL // LIVE AVL STREAM", 24, sf::Color(0, 255, 65),
                        30.f, 12.f, 620.f, true, &uiCache);
            Ui::textFit(window, font, cachedEndlessSub, 14, Ui::textBody(),
                        30.f, 46.f, 620.f, false, &uiCache);

            Ui::button(window, font, endlessPaused ? "Resume [P]" : "Pause [P]",
                       670.f, 18.f, 125.f, 34.f, endlessPaused ? BTN_PRIMARY : BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Restart [R]", 805.f, 18.f, 125.f, 34.f, BTN_GHOST, in, 13, &uiCache);
            Ui::button(window, font, "Menu [Esc]",  940.f, 18.f, 125.f, 34.f, BTN_GHOST, in, 13, &uiCache);

            float meterX = 1090.f, meterW = 480.f;
            float ratio = clampFloat((winSec > 0.f) ? (remSec / winSec) : 0.f, 0.f, 1.f);
            sf::RectangleShape& mRect = uiCache.rect;
            mRect.setSize({meterW, 20.f});
            mRect.setOrigin({0.f, 0.f});
            mRect.setPosition({meterX, 16.f});
            mRect.setFillColor(sf::Color(35, 42, 58));
            mRect.setOutlineThickness(1.5f);
            mRect.setOutlineColor(sf::Color(100, 115, 140));
            window.draw(mRect);

            mRect.setSize({meterW * ratio, 20.f});
            mRect.setPosition({meterX, 16.f});
            mRect.setOutlineThickness(0.f);
            mRect.setFillColor(ratio < 0.25f ? Ui::red() : (ratio < 0.55f ? Ui::amber() : Ui::green()));
            window.draw(mRect);

            int remMs = static_cast<int>(remSec * 1000.0f + 0.5f);
            int winMs = static_cast<int>(winSec * 1000.0f + 0.5f);
            std::string hudTimerStr = "Survival: " + EndlessLeaderboard::formatTimeMs(endlessSession.getSurvivalMs()) +
                                      "   |   Deadline: " + EndlessLeaderboard::formatTimeMs(remMs) +
                                      " / " + EndlessLeaderboard::formatTimeMs(winMs);
            Ui::textFit(window, font, hudTimerStr, 14, sf::Color::White, meterX, 44.f, meterW, true, &uiCache);

            // Canvas + AVL render
            Ui::panel(window, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, sf::Color(0, 255, 65), 248, &uiCache);
            if (!animCtrl.hasEvents() && needLayout) {
                layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                layoutDirty = false;
            }
            const LayoutResult& drawL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : avlLayout;
            RenderHighlightState hl;
            hl.selectedNodeId = selectedNodeId;
            hl.showBadges = true;
            sf::Vector2f wPos(0.f, 0.f);
            if (mapHoverToCam(endlessCam, in.mx, in.my, wPos)) {
                int hovIdx = drawL.hitTestNode(wPos.x, wPos.y);
                if (hovIdx >= 0) hl.hoveredNodeId = drawL.nodes[hovIdx].id;
            }
            if (animCtrl.hasEvents()) animCtrl.fillHighlightState(hl);

            window.setView(endlessCam.view);
            if (showBgGrid) guiRenderer.drawCanvasGrid(window, endlessCam.view);
            guiRenderer.drawTreeLayout(window, font, drawL, hl, true, false);
            window.setView(window.getDefaultView());
            Ui::text(window, font, endlessCam.zoomText(), 11, Ui::textMuted(),
                     CANVAS_X + 12.f, CANVAS_Y + 6.f, false, &uiCache);
            drawCamTooltip(endlessCam, drawL, hl.hoveredNodeId);

            guiRenderer.drawEventLogPanel(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                          animCtrl.getBatch(), animCtrl.getCursor(),
                                          animCtrl.getLog().getBatchCount());
            guiRenderer.drawPlaybackControls(window, font, LOG_X, LOG_Y, LOG_W, LOG_H,
                                             animCtrl.isPlaying(), animCtrl.getSpeed(),
                                             animCtrl.getCursor(), animCtrl.getEventCount(), in);

            // Bottom Pending Operation & Persistence Panel
            Ui::panel(window, 30.f, 660.f, 1540.f, 210.f, sf::Color(60, 85, 125), 235, &uiCache);
            const EndlessOp& op = endlessSession.getPendingOp();
            if (cachedEndlessOpType != op.type || cachedEndlessOpKey != op.key || cachedEndlessPaused != endlessPaused) {
                cachedEndlessOpType = op.type;
                cachedEndlessOpKey = op.key;
                cachedEndlessPaused = endlessPaused;
                if (endlessPaused) {
                    cachedEndlessPrompt = "PAUSED - Press [P] to resume live AVL stream timer";
                } else {
                    cachedEndlessPrompt = std::string("INCOMING OP: ") +
                                          (op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                                          std::to_string(op.key) +
                                          "  - Click [Execute] to apply real AVL operation & check invariants!";
                }
            }
            Ui::textFit(window, font, cachedEndlessPrompt, TXT_H2, sf::Color(0, 255, 65), 54.f, 678.f, 1040.f, true, &uiCache);
            Ui::textWrap(window, font, status, TXT_BODY, Ui::textBody(), 54.f, 720.f, 1040.f, false, &uiCache);
            Ui::textFit(window, font, "[Enter / X] Execute Pending Op  |  [P] Pause  |  [F5] Save Run  |  [F9] Load Run  |  [L] Leaderboard  |  [Esc] Main Menu",
                        TXT_HINT, Ui::textMuted(), 54.f, 832.f, 1480.f, false, &uiCache);

            std::string execBtnLbl = std::string("EXECUTE: ") +
                                     (op.type == ENDLESS_OP_INSERT ? "INSERT " : "DELETE ") +
                                     std::to_string(op.key) + " [Enter]";
            int execStyle = endlessPaused ? BTN_DISABLED : (ratio < 0.25f ? BTN_DANGER : BTN_PRIMARY);
            Ui::button(window, font, execBtnLbl, 1120.f, 678.f, 428.f, 54.f, execStyle, in, 15, &uiCache);
            Ui::button(window, font, "Save Run [F5]", 1120.f, 748.f, 136.f, 46.f, BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Load Run [F9]", 1266.f, 748.f, 136.f, 46.f, BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Scores [L]",    1412.f, 748.f, 136.f, 46.f, BTN_GHOST,     in, 13, &uiCache);
        }

        // 11. ENDLESS MODE LEADERBOARD (endless_leaderboard.txt)
        if (currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
            if (vaultBgSprite.has_value()) {
                window.draw(*vaultBgSprite);
            }
            const sf::Color elbBright(0, 255, 65);
            const sf::Color elbSubtle(120, 195, 120);

            Ui::text(window, menuFont, "ENDLESS SURVIVAL LEADERBOARD", 34, elbBright, 50.f, 20.f, true, &menuUiCache);
            Ui::text(window, menuFont, "Survival times ranked descending from endless_leaderboard.txt", 14, elbSubtle, 50.f, 64.f, false, &menuUiCache);
            sf::RectangleShape hdrLine({1150.f, 1.f});
            hdrLine.setPosition({50.f, 92.f});
            hdrLine.setFillColor(sf::Color(0, 255, 65, 140));
            window.draw(hdrLine);

            float mw = 920.f;
            float mx0 = static_cast<float>(W) * 0.5f - mw * 0.5f;
            if (endlessShowLastResult) {
                Ui::panel(window, mx0, 104.f, mw, 86.f, Ui::red(), 235, &uiCache);
                std::string runSum = "ENDLESS RUN ENDED: " + EndlessLeaderboard::formatTimeMs(endlessLastEntry.survivalMs) +
                                     " survived  |  " + std::to_string(endlessLastEntry.opsCompleted) +
                                     " ops  |  Seed #" + std::to_string(endlessLastEntry.seed);
                Ui::textFit(window, menuFont, runSum, TXT_H2, sf::Color(255, 110, 110), mx0 + 28.f, 116.f, mw - 56.f, true, &menuUiCache);
                Ui::textFit(window, menuFont, endlessLastReason, TXT_HINT, sf::Color(255, 200, 200),
                            mx0 + 28.f, 154.f, mw - 56.f, false, &menuUiCache);
            }

            float my0 = endlessShowLastResult ? 204.f : 130.f;
            float mh  = endlessShowLastResult ? 540.f : 600.f;
            Ui::panel(window, mx0, my0, mw, mh, elbBright, 232, &uiCache);

            if (endlessLb.getCount() == 0) {
                Ui::textCenter(window, menuFont, "No endless survival runs recorded yet", 24, elbBright,
                               static_cast<float>(W) * 0.5f, my0 + 180.f, true, &menuUiCache);
                Ui::textCenter(window, menuFont, "Start Endless Survival Mode from the Main Menu to set a survival record!",
                               15, elbSubtle, static_cast<float>(W) * 0.5f, my0 + 235.f, false, &menuUiCache);
            } else {
                Ui::text(window, menuFont, "RANK",            14, elbBright, mx0 + 36.f,  my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "OPERATOR / DATE", 14, elbBright, mx0 + 120.f, my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "SURVIVAL TIME",   14, elbBright, mx0 + 500.f, my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "OPS",             14, elbBright, mx0 + 670.f, my0 + 22.f, true, &menuUiCache);
                Ui::text(window, menuFont, "SEED",            14, elbBright, mx0 + 760.f, my0 + 22.f, true, &menuUiCache);

                sf::RectangleShape thLine({mw - 48.f, 1.f});
                thLine.setPosition({mx0 + 24.f, my0 + 48.f});
                thLine.setFillColor(sf::Color(0, 255, 65, 90));
                window.draw(thLine);

                int rows = minInt(10, endlessLb.getCount());
                for (int r = 0; r < rows; ++r) {
                    const EndlessLeaderboardEntry& e = endlessLb.get(r);
                    float ry = my0 + 58.f + static_cast<float>(r) * 44.f;
                    Ui::text(window, menuFont, "#" + std::to_string(r + 1), 15, elbBright,
                             mx0 + 36.f, ry, true, &menuUiCache);
                    Ui::textFit(window, menuFont, e.nameDate, 15, sf::Color(220, 240, 225),
                                mx0 + 120.f, ry, 360.f, false, &menuUiCache);
                    Ui::text(window, menuFont, EndlessLeaderboard::formatTimeMs(e.survivalMs), 15, elbBright,
                             mx0 + 515.f, ry, false, &menuUiCache);
                    Ui::text(window, menuFont, std::to_string(e.opsCompleted), 15, sf::Color(220, 240, 225),
                             mx0 + 675.f, ry, false, &menuUiCache);
                    Ui::text(window, menuFont, std::to_string(e.seed), 15, elbSubtle,
                             mx0 + 760.f, ry, false, &menuUiCache);
                }
            }

            Ui::button(window, font, "New Endless Run [R]", 270.f,  770.f, 250.f, 48.f, BTN_PRIMARY,   in, 14, &uiCache);
            Ui::button(window, font, "Replay Seed [S]",     540.f,  770.f, 250.f, 48.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "Campaign Scores [C]", 810.f,  770.f, 250.f, 48.f, BTN_SECONDARY, in, 14, &uiCache);
            Ui::button(window, font, "Main Menu [Esc]",     1080.f, 770.f, 250.f, 48.f, BTN_GHOST,     in, 14, &uiCache);
        }

        cachedLayoutScreen = currentScreen;
        cachedLayoutLid = curLid;

        if (!profileFps && verifyDir.empty()) {
            Ui::text(window, font, liveFpsStr, 11, Ui::textMuted(), 1530.f, 2.f, false, &uiCache);
        }

        // Toasts rendered on top of every screen (Item 2)
        toastQueue.draw(window, font, static_cast<float>(W));
        if (outCpuMs) *outCpuMs = static_cast<double>(frameCpuClk.getElapsedTime().asMicroseconds()) / 1000.0;
        if (!capturePath.empty()) {
            sf::Texture tex(sf::Vector2u(static_cast<unsigned>(W), static_cast<unsigned>(H)));
            tex.update(window);
            (void)tex.copyToImage().saveToFile(capturePath);
        }
        sf::Clock dispClk;
        window.display();
        if (outDispMs) *outDispMs = static_cast<double>(dispClk.getElapsedTime().asMicroseconds()) / 1000.0;
    };

    // Optional automated screenshot verification mode (--verify-screenshots <dir>)
    if (!verifyDir.empty()) {
        auto saveShot = [&](const std::string& name, const UiInput& in) {
            while (const auto ev = window.pollEvent()) {
                (void)ev;
            }
            renderFrame(in, verifyDir + "/" + name + ".png");
        };

        UiInput idle;
        idle.mx = 800.f; idle.my = 450.f;

        currentScreen = SCREEN_MAIN_MENU;
        saveShot("01_main_menu", idle);

        currentScreen = SCREEN_CAMPAIGN_MAP;
        saveShot("02_campaign_map", idle);

        // Unlock & inspect Level 2.2 briefing
        startLevel(6);
        saveShot("03_level_briefing", idle);

        // Dismiss briefing, hover root 50 & trigger a toast
        showingBriefing = false;
        layoutBstForScreen(false);
        if (bstLayout.nodeCount > 0) {
            idle.mx = bstLayout.nodes[bstLayout.nodeCount / 2].x;
            idle.my = bstLayout.nodes[bstLayout.nodeCount / 2].y;
        }
        toastQueue.push("Clicked Leaf 80 cleanly!", TOAST_KIND_SUCCESS);
        toastQueue.update(0.25f);
        saveShot("04_bst_node_play_toast_tooltip", idle);

        // Level 3.4 AVL with BF badges & incoming prompt
        toastQueue.clear();
        startLevel(13);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        idle.mx = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].x : 600.f;
        idle.my = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].y : 300.f;
        saveShot("05_avl_badges", idle);

        // Level 4.1 Heap + synchronised array view
        startLevel(15);
        showingBriefing = false;
        layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        idle.mx = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].x : 600.f;
        idle.my = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].y : 300.f;
        saveShot("06_heap_array_sync", idle);

        currentScreen = SCREEN_LEADERBOARD;
        saveShot("07_leaderboard", idle);

        currentScreen = SCREEN_SETTINGS;
        saveShot("08_settings", idle);

        // Zoomed & panned canvas verification (Part B + Part C Figma grid + screen-fixed HUD/tooltip)
        toastQueue.clear();
        startLevel(6);
        showingBriefing = false;
        layoutBstForScreen(false);
        layoutDirty = false;
        if (bstLayout.nodeCount > 0) {
            sf::Vector2i zoomAnchor = window.mapCoordsToPixel({bstLayout.nodes[0].x, bstLayout.nodes[0].y}, levelCam.view);
            levelCam.handleZoom(window, zoomAnchor, 1.f);
            levelCam.handleZoom(window, zoomAnchor, 1.f);
            levelCam.handleZoom(window, zoomAnchor, 1.f); // ~1.52x zoom toward root 50
            levelCam.view.move({45.f, -22.f});            // pan slightly
            levelCam.clampCenter();
            sf::Vector2i hovPx = window.mapCoordsToPixel({bstLayout.nodes[0].x, bstLayout.nodes[0].y}, levelCam.view);
            idle.mx = static_cast<float>(hovPx.x);
            idle.my = static_cast<float>(hovPx.y);
        }
        saveShot("09_canvas_pan_zoom", idle);

        // Act 1 Levels (1.1 through 1.5) visual verification
        toastQueue.clear();
        startLevel(0); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        saveShot("10_act1_1_traversals", idle);

        startLevel(1); showingBriefing = false;
        saveShot("11_act1_2_blueprint_dragdrop", idle);

        startLevel(2); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        act1StepperVal = 1; submitAct1HeightOrDiameter();
        act1StepperVal = 1; submitAct1HeightOrDiameter();
        saveShot("12_act1_3_heights_diameter", idle);

        startLevel(3); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        if (btLayout.findNodeIndexByKey(20) >= 0) handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(20)].id, 20);
        if (btLayout.findNodeIndexByKey(40) >= 0) handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(40)].id, 40);
        saveShot("13_act1_4_lca_path", idle);

        startLevel(4); showingBriefing = false; layoutBtForScreen(false); layoutDirty = false;
        if (btLayout.findNodeIndexByKey(30) >= 0) handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(30)].id, 30);
        saveShot("14_act1_5_mirror_vault", idle);

        // Act 2 Levels (2.1 through 2.5) visual verification
        toastQueue.clear();
        startLevel(5); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2CompareChoice(1); // 40 < 50 -> move to 30
        saveShot("15_act2_1_search_heist", idle);

        startLevel(6); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2CompareChoice(1); // 25 < 50 -> 30
        handleAct2CompareChoice(1); // 25 < 30 -> 20
        handleAct2CompareChoice(3); // 25 > 20 -> insert 25
        saveShot("16_act2_2_stash_the_loot", idle);

        startLevel(7); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2DeleteCase(1); // Leaf 80
        animCtrl.finishInstant();
        handleAct2DeleteCase(2); // One-Child 70
        animCtrl.finishInstant();
        handleAct2DeleteCase(3); // Two-Children 30 -> Phase 1 (successor swap)
        animCtrl.finishInstant();
        saveShot("17_act2_3_steal_and_collapse", idle);

        startLevel(8); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        handleAct2RangeDecision(RQ_ACT_REPORT);     // Report 50
        handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT); // Prune 10 (greys out 10)
        handleAct2RangeDecision(RQ_ACT_REPORT);     // Report 35
        animCtrl.finishInstant();
        saveShot("18_act2_4_range_raid", idle);

        startLevel(9); showingBriefing = false; layoutBstForScreen(false); layoutDirty = false;
        for (int s = 0; s < 4; ++s) {
            for (int r = 0; r <= s; ++r) handleAct2CompareChoice(3); // walk down right spine + insert
        }
        animCtrl.finishInstant();
        saveShot("19_act2_5_lazy_architect_boss", idle);

        // Act 3 Levels (3.1 through 3.5) visual verification
        toastQueue.clear();
        startLevel(10); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (avlLayout.findNodeIndexByKey(30) >= 0) handleLevelNodeClick(avlLayout.nodes[avlLayout.findNodeIndexByKey(30)].id, 30);
        saveShot("20_act3_1_read_badges", idle);

        startLevel(11); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[11].puzzleCount > 0) {
            int pk = levelData[11].puzzles[0].rootKey;
            int pi = avlLayout.findNodeIndexByKey(pk);
            if (pi >= 0) handleLevelNodeClick(avlLayout.nodes[pi].id, pk);
        }
        saveShot("21_act3_2_single_rotations", idle);

        startLevel(12); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        handleAct3DoubleCase(1); // Identify LR case -> highlights heavy child pivot step
        saveShot("22_act3_3_double_rotations", idle);

        // Report: force the pure RR (3.2 puzzle 2) and RL (3.3 puzzle 2) cases so all four rotation types are captured
        startLevel(11); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[11].puzzleCount > 1) {
            const LevelPuzzleSpec& sp0 = levelData[11].puzzles[0];
            int si0 = avlLayout.findNodeIndexByKey(sp0.rootKey);
            if (si0 >= 0) handleLevelNodeClick(avlLayout.nodes[si0].id, sp0.rootKey);
            handleAct3SingleRotation(sp0.caseName == "LL" ? 1 : 2);
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
            const LevelPuzzleSpec& sp1 = levelData[11].puzzles[1];
            int si1 = avlLayout.findNodeIndexByKey(sp1.rootKey);
            if (si1 >= 0) handleLevelNodeClick(avlLayout.nodes[si1].id, sp1.rootKey);
        }
        saveShot("22b_act3_2_rr_rotation", idle);

        startLevel(12); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[12].puzzleCount > 1) {
            const LevelPuzzleSpec& dp0 = levelData[12].puzzles[0];
            handleAct3DoubleCase(dp0.caseName == "LR" ? 1 : 2);
            int dc = avlLayout.findNodeIndexByKey(dp0.childKey);
            if (dc >= 0) handleLevelNodeClick(avlLayout.nodes[dc].id, dp0.childKey);
            animCtrl.finishInstant();
            int dr = avlLayout.findNodeIndexByKey(dp0.rootKey);
            if (dr >= 0) handleLevelNodeClick(avlLayout.nodes[dr].id, dp0.rootKey);
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
            handleAct3DoubleCase(levelData[12].puzzles[1].caseName == "LR" ? 1 : 2);
        }
        saveShot("22c_act3_3_rl_rotation", idle);

        startLevel(13); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[13].insertCount >= 2) {
            handleAct3Prediction(levelData[13].inserts[0].expectedRotation.c_str()); animCtrl.finishInstant();
            handleAct3Prediction(levelData[13].inserts[1].expectedRotation.c_str()); animCtrl.finishInstant();
        }
        saveShot("23_act3_4_predict_insert", idle);

        startLevel(14); showingBriefing = false; layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        if (levelData[14].kQueryCount > 0) {
            int dummyC = 0;
            int ans1 = kthLargestAvl(avl, levelData[14].kQueries[0], dummyC);
            int idx1 = avlLayout.findNodeIndexByKey(ans1);
            if (idx1 >= 0) handleLevelNodeClick(avlLayout.nodes[idx1].id, ans1);
            animCtrl.finishInstant();
        }
        saveShot("24_act3_5_order_stat_boss", idle);

        // Act 4 Levels (4.3..4.5 ascending sorted tail & 4.4 step/complexity readout)
        toastQueue.clear();
        startLevel(17); showingBriefing = false; layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        for (int e = 0; e < 4; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("25_act4_3_extract_ascending", idle);

        startLevel(18); showingBriefing = false; layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        for (int e = 0; e < 3; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("26_act4_4_race_complexity", idle);

        startLevel(19); showingBriefing = false; layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H); layoutDirty = false;
        triggerStructuralAction(1);
        animCtrl.finishInstant();
        saveShot("26b_act4_5_build_max_heap", idle); // Report: bottom-up buildMaxHeap result before any extraction
        for (int e = 0; e < 4; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("27_act4_5_boss_heapsort", idle);
        // Report: finish every extraction so the array view shows the fully ascending sorted result
        for (int guard = 0; guard < 16 && act4Heap.getSize() > 0 && currentScreen == SCREEN_LEVEL_PLAY; ++guard) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        saveShot("27b_act4_5_sorted_result", idle);

        // Endless Mode & Endless Leaderboard visual verification (in-memory only)
        endlessLb.load("");
        toastQueue.clear();
        startEndlessMode(424242u);
        for (int s = 0; s < 5; ++s) {
            endlessSession.tick(0.75f);
            triggerEndlessExecute();
            animCtrl.finishInstant();
        }
        saveShot("28_endless_mode_play", idle);

        endlessSession.tick(endlessSession.getOpTimeRemaining() + 0.1f);
        finishEndlessGameOver();
        saveShot("29_endless_leaderboard", idle);

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 1 levels (--verify-act1)
    if (verifyAct1) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };

        // 1.1: All 4 traversal modes + wrong-click +1 alarm penalty
        for (int m = 0; m < 4; ++m) {
            startLevel(0);
            showingBriefing = false;
            act1TravMode = m;
            layoutBtForScreen(false);
            int cnt = 0;
            int* seq = getAct1Traversal(act1Tree, m, cnt);
            if (!seq || cnt != 7) return failCheck("1.1 sequence length != 7");
            if (m == 0) {
                int wrongKey = seq[1];
                int wIdx = btLayout.findNodeIndexByKey(wrongKey);
                handleLevelNodeClick(btLayout.nodes[wIdx].id, wrongKey);
                if (scoring.alarm() != 1) return failCheck("1.1 wrong traversal click must add +1 alarm");
            }
            for (int i = 0; i < cnt; ++i) {
                int nIdx = btLayout.findNodeIndexByKey(seq[i]);
                handleLevelNodeClick(btLayout.nodes[nIdx].id, seq[i]);
            }
            delete[] seq;
            completePendingWin();
            if (currentScreen != SCREEN_RESULT) return failCheck("1.1 did not reach result");
            if (m > 0 && scoring.stars() != 3) return failCheck("1.1 clean traversal should earn 3 stars");
            if (m == 0 && scoring.stars() != 2) return failCheck("1.1 single-penalty run should earn 2 stars");
        }
        std::cout << "PASS 1.1: All 4 traversal modes & +1 penalty verified\n";

        // 1.2: Drag-and-drop reconstruction (wrong placement penalty + clean solve)
        startLevel(1);
        showingBriefing = false;
        for (int s = 0; s < dragDrop.slotCount(); ++s) {
            int tokIdx = (s + 1) % dragDrop.tokenCount();
            const DragToken* tk = dragDrop.token(tokIdx);
            const DropSlot* sl = dragDrop.slot(s);
            dragDrop.press(tk->x, tk->y);
            dragDrop.move(sl->x, sl->y);
            DropResult r = dragDrop.release(sl->x, sl->y);
            handleAct1BlueprintDrop(r);
        }
        if (scoring.penaltyEvents() != 1 || scoring.alarm() != 8) return failCheck("1.2 wrong placement penalty mismatch");

        startLevel(1);
        showingBriefing = false;
        for (int s = 0; s < btLayout.nodeCount; ++s) {
            int wantedKey = btLayout.nodes[s].key;
            int tokIdx = -1;
            for (int t = 0; t < dragDrop.tokenCount(); ++t) {
                if (dragDrop.token(t)->value == wantedKey) { tokIdx = t; break; }
            }
            if (tokIdx < 0) return failCheck("1.2 missing token for key");
            const DragToken* tk = dragDrop.token(tokIdx);
            const DropSlot* sl = dragDrop.slot(s);
            dragDrop.press(tk->x, tk->y);
            dragDrop.move(sl->x, sl->y);
            DropResult r = dragDrop.release(sl->x, sl->y);
            handleAct1BlueprintDrop(r);
        }
        if (currentScreen != SCREEN_RESULT || !act1Reconstructed || scoring.stars() != 3) {
            return failCheck("1.2 clean drag-drop reconstruction failed");
        }
        std::cout << "PASS 1.2: Drag-drop Pre+In reconstruction verified (3 stars)\n";

        // 1.3: Postorder heights + diameter stepper
        startLevel(2);
        showingBriefing = false;
        layoutBtForScreen(false);
        act1StepperVal = 4; // wrong height for leaf
        submitAct1HeightOrDiameter();
        if (scoring.alarm() != 1) return failCheck("1.3 wrong height must add +1 alarm");

        startLevel(2);
        showingBriefing = false;
        layoutBtForScreen(false);
        {
            int postCnt = 0;
            int* postSeq = act1Tree.postorderRecursive(postCnt);
            for (int i = 0; i < postCnt; ++i) {
                act1StepperVal = treeHeightNodes(findTreeNodeConst(act1Tree.getRoot(), postSeq[i]));
                submitAct1HeightOrDiameter();
            }
            delete[] postSeq;
            act1StepperVal = act1Tree.diameter();
            submitAct1HeightOrDiameter();
        }
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) return failCheck("1.3 clean postorder height+diameter failed");
        std::cout << "PASS 1.3: Postorder heights + diameter verified (3 stars)\n";

        // 1.4: Pick 2 rooms -> click LCA -> trace path (no Boss label)
        if (levels[3].title.find("Boss") != std::string::npos) return failCheck("1.4 must not have Boss label");
        startLevel(3);
        showingBriefing = false;
        layoutBtForScreen(false);
        {
            int seq[] = { 20, 80, 50, 20, 30, 50, 70, 80 };
            for (int k : seq) {
                int idx = btLayout.findNodeIndexByKey(k);
                handleLevelNodeClick(btLayout.nodes[idx].id, k);
            }
        }
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) return failCheck("1.4 route trace failed");
        std::cout << "PASS 1.4: 2-room pick + LCA + path trace verified (3 stars)\n";

        // 1.5: Bottom-up swap + penalty on top-down click + Auto Mirror alternative
        startLevel(4);
        showingBriefing = false;
        layoutBtForScreen(false);
        handleLevelNodeClick(btLayout.nodes[btLayout.findNodeIndexByKey(50)].id, 50); // top-down rejected
        if (scoring.alarm() != 1) return failCheck("1.5 top-down click must trigger +1 alarm");

        startLevel(4);
        showingBriefing = false;
        layoutBtForScreen(false);
        {
            int swaps[] = { 30, 70, 50 };
            for (int k : swaps) {
                int idx = btLayout.findNodeIndexByKey(k);
                handleLevelNodeClick(btLayout.nodes[idx].id, k);
            }
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            !act1Tree.getRoot() || act1Tree.getRoot()->left->value != 70) {
            return failCheck("1.5 bottom-up mirror failed");
        }

        startLevel(4);
        showingBriefing = false;
        layoutBtForScreen(false);
        triggerStructuralAction(1);
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            !act1Tree.getRoot() || act1Tree.getRoot()->left->value != 70) {
            return failCheck("1.5 Auto Mirror option failed");
        }
        std::cout << "PASS 1.5: Bottom-up click-swap and Auto Mirror both verified (3 stars)\n";

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 2 levels (--verify-act2)
    if (verifyAct2) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };

        // 2.1: Search Heist (root-to-target L/R/Found + wrong choice +1 penalty)
        startLevel(5);
        showingBriefing = false;
        layoutBstForScreen(false);
        handleAct2CompareChoice(3); // wrong: 40 < 50
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("2.1 wrong direction must add +1 alarm");

        startLevel(5);
        showingBriefing = false;
        handleAct2CompareChoice(1); // at 50 -> Left to 30
        handleAct2CompareChoice(3); // at 30 -> Right to 40
        handleAct2CompareChoice(2); // at 40 -> Found
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 3) {
            return failCheck("2.1 Search Heist clean run failed");
        }
        std::cout << "PASS 2.1: Search Heist root-to-target comparisons verified (3 stars)\n";

        // 2.2: Stash the Loot (walk-to-slot insert 25 + duplicate 40 rejected)
        startLevel(6);
        showingBriefing = false;
        handleAct2CompareChoice(2); // wrong at root 50 when inserting 25
        if (scoring.alarm() != 1) return failCheck("2.2 wrong choice must add +1 alarm");

        startLevel(6);
        showingBriefing = false;
        handleAct2CompareChoice(1); // 25 < 50 -> 30
        handleAct2CompareChoice(1); // 25 < 30 -> 20
        handleAct2CompareChoice(3); // 25 > 20 -> empty right slot -> insert 25
        if (!act2Bst.search(25) || !act2Bst.isValidBST() || act2Bst.getSize() != 8) {
            return failCheck("2.2 insert 25 failed");
        }
        handleAct2CompareChoice(1); // 40 < 50 -> 30
        handleAct2CompareChoice(3); // 40 > 30 -> 40
        handleAct2CompareChoice(2); // 40 == 40 -> Reject Duplicate
        if (act2Bst.getSize() != 8) return failCheck("2.2 duplicate 40 must be rejected");
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 6) {
            return failCheck("2.2 Stash the Loot clean run failed");
        }
        std::cout << "PASS 2.2: Stash the Loot insert & duplicate rejection verified (3 stars)\n";

        // 2.3: Steal and Collapse (verify all 3 distinct cases: Leaf 80, One-Child 70, Two-Children 30)
        startLevel(7);
        showingBriefing = false;
        if (bstDeleteCase(findTreeNodeConst(act2Bst.getRoot(), 80)) != 1) return failCheck("2.3: 80 must be Leaf");
        handleAct2DeleteCase(3); // wrong case penalty
        if (scoring.alarm() != 1) return failCheck("2.3 wrong delete case must add +1 alarm");

        startLevel(7);
        showingBriefing = false;
        handleAct2DeleteCase(1); // Delete 80 (Leaf)
        if (act2Bst.search(80) || bstDeleteCase(findTreeNodeConst(act2Bst.getRoot(), 70)) != 2) {
            return failCheck("2.3: after deleting 80, 70 must have exactly One-Child (60)");
        }
        handleAct2DeleteCase(2); // Delete 70 (One-Child)
        if (act2Bst.search(70) || bstDeleteCase(findTreeNodeConst(act2Bst.getRoot(), 30)) != 3) {
            return failCheck("2.3: after deleting 70, 30 must still have Two-Children (20 & 40)");
        }
        handleAct2DeleteCase(3); // Delete 30 (Two-Children) -> Phase 1 (Swap Value)
        if (!act2TwoChildPhase2Pending) return failCheck("2.3 two-child delete must split into Phase 1 swap & Phase 2 unlink");
        // Verify Phase 1 mid-layout visibly shows successor value 40 copied into target slot while both exist
        if (act2MidLayout.nodeCount != 5 || act2MidLayout.nodes[act2MidLayout.findNodeIndexById(2)].key != 40) {
            return failCheck("2.3 Phase 1 mid-layout must show swapped successor value");
        }
        animCtrl.finishInstant();
        startAct2TwoChildPhase2(); // Phase 2 (Unlink Successor)
        animCtrl.finishInstant();
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || act2Bst.search(30) || !act2Bst.isValidBST() || scoring.stars() != 3) {
            return failCheck("2.3 Steal and Collapse two-phase delete failed");
        }
        std::cout << "PASS 2.3: Steal & Collapse (Leaf + 1-Child + 2-Child 2-step animation) verified (3 stars)\n";

        // 2.4: Range Raid (Report + Prune Left/Right + grey-out + +50 bonus per prune)
        startLevel(8);
        showingBriefing = false;
        handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT); // wrong at root 50
        if (scoring.alarm() != 1) return failCheck("2.4 wrong range decision must add +1 alarm");

        startLevel(8);
        showingBriefing = false;
        handleAct2RangeDecision(RQ_ACT_REPORT);      // 50
        handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT);  // prune 10 (+50 bonus)
        handleAct2RangeDecision(RQ_ACT_REPORT);      // 35
        handleAct2RangeDecision(RQ_ACT_PRUNE_RIGHT); // prune 90 (+50 bonus)
        handleAct2RangeDecision(RQ_ACT_REPORT);      // 65
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            scoring.bonusPoints() != 100 || act2PrunedCount != 2) {
            return failCheck("2.4 Range Raid failed");
        }
        std::cout << "PASS 2.4: Range Raid pruning & bonuses (+100 bonus) verified (3 stars)\n";

        // 2.5: Boss: The Lazy Architect (sorted chain + shrinking alarm + O(n) search + Act 3 cutscene)
        startLevel(9);
        showingBriefing = false;
        int opt25 = computeOptimalSteps(levelData[9]);
        if (scoring.capacity() >= ScoringEngine::capacityFor(opt25)) {
            return failCheck("2.5 boss initial alarm capacity must be tighter than normal formula");
        }
        for (int s = 0; s < 4; ++s) {
            for (int r = 0; r <= s; ++r) handleAct2CompareChoice(3); // Go Right down chain
        }
        if (act2Bst.getHeight() != 5 || act2Bst.getSize() != 5 || act2BossStage != 1 ||
            scoring.capacity() != 17) {
            return failCheck("2.5 degenerate chain build or shrinking capacity mismatch");
        }
        // Phase 2: Search for 50 down the height-5 chain (10 -> 20 -> 30 -> 40 -> 50 -> Found)
        for (int r = 0; r < 4; ++r) handleAct2CompareChoice(3);
        handleAct2CompareChoice(2); // Found 50
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 15 ||
            status.find("AVL") == std::string::npos) {
            return failCheck("2.5 Boss completion or Act 3 balancing note missing");
        }
        std::cout << "PASS 2.5: Lazy Architect Boss (Height=5, Cap shrank 21->17, 3 stars) verified\n";

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 3 levels (--verify-act3)
    if (verifyAct3) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };

        // 3.1: Read the Badges (+1 traversal penalty on balanced node, no -50 rot penalty; 3 stars clean)
        startLevel(10);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int idx50 = avlLayout.findNodeIndexByKey(50);
            if (idx50 < 0) return failCheck("3.1 missing balanced node 50");
            handleLevelNodeClick(avlLayout.nodes[idx50].id, 50);
            if (scoring.alarm() != 1 || scoring.score() != (SCORE_BASE - SCORE_PER_ALARM_STEP) ||
                scoring.penaltyEvents() != 1) {
                return failCheck("3.1 wrong badge click must use +1 alarm (no -50 rot penalty)");
            }
        }
        startLevel(10);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int idx30 = avlLayout.findNodeIndexByKey(30);
            int idx20 = avlLayout.findNodeIndexByKey(20);
            if (idx30 < 0 || idx20 < 0) return failCheck("3.1 missing violation nodes 30/20");
            handleLevelNodeClick(avlLayout.nodes[idx30].id, 30);
            handleLevelNodeClick(avlLayout.nodes[idx20].id, 20);
            if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.alarm() != 2) {
                return failCheck("3.1 Read the Badges clean run failed");
            }
        }
        std::cout << "PASS 3.1: Read the Badges (real AVL BF & +1 traversal penalty) verified (3 stars)\n";

        // 3.2: Manual Repair LL/RR (pivot click + LL/RR choice + real AVLTree rotations + +2/-50 penalty)
        startLevel(11);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            const LevelPuzzleSpec& p0 = levelData[11].puzzles[0];
            int idxChild = avlLayout.findNodeIndexByKey(p0.childKey);
            if (idxChild < 0) return failCheck("3.2 missing puzzle 0 child");
            handleLevelNodeClick(avlLayout.nodes[idxChild].id, p0.childKey); // balanced -> PENALTY_WRONG_ROTATION
            if (scoring.alarm() != 2 ||
                scoring.score() != (SCORE_BASE - 2 * SCORE_PER_ALARM_STEP - SCORE_ROTATION_PENALTY)) {
                return failCheck("3.2 balanced pivot click must apply wrong rotation penalty (+2/-50)");
            }
            int idxRoot = avlLayout.findNodeIndexByKey(p0.rootKey);
            handleLevelNodeClick(avlLayout.nodes[idxRoot].id, p0.rootKey);
            handleAct3SingleRotation(p0.caseName == "LL" ? 2 : 1); // wrong LL/RR button
            if (scoring.alarm() != 4 ||
                scoring.score() != (SCORE_BASE - 4 * SCORE_PER_ALARM_STEP - 2 * SCORE_ROTATION_PENALTY)) {
                return failCheck("3.2 wrong LL/RR choice must apply +2/-50 penalty");
            }
        }
        startLevel(11);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        for (int pIdx = 0; pIdx < levelData[11].puzzleCount; ++pIdx) {
            const LevelPuzzleSpec& p = levelData[11].puzzles[pIdx];
            int rIdx = avlLayout.findNodeIndexByKey(p.rootKey);
            if (rIdx < 0) return failCheck("3.2 missing puzzle root");
            handleLevelNodeClick(avlLayout.nodes[rIdx].id, p.rootKey);
            if (!act3PivotPicked || act3PivotKey != p.rootKey) return failCheck("3.2 failed to select pivot");
            handleAct3SingleRotation(p.caseName == "LL" ? 1 : 2);
            int badKey = -1;
            if (!avl.isValidAVL() || !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.2 real AVL single rotation left invalid tree or subtree size");
            }
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("3.2 clean LL/RR solve failed");
        }
        std::cout << "PASS 3.2: Manual Repair LL & RR via real AVLTree rotations verified (3 stars)\n";

        // 3.3: Double Rotations LR/RL (identify case -> child pivot -> root pivot; two real single rotations)
        startLevel(12);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        handleAct3DoubleCase(2); // LR puzzle picked as RL -> PENALTY_WRONG_ROTATION
        if (scoring.alarm() != 2 ||
            scoring.score() != (SCORE_BASE - 2 * SCORE_PER_ALARM_STEP - SCORE_ROTATION_PENALTY)) {
            return failCheck("3.3 wrong zig-zag case identification must apply +2/-50 penalty");
        }
        startLevel(12);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        for (int pIdx = 0; pIdx < levelData[12].puzzleCount; ++pIdx) {
            const LevelPuzzleSpec& p = levelData[12].puzzles[pIdx];
            handleAct3DoubleCase(p.caseName == "LR" ? 1 : 2);
            int cIdx = avlLayout.findNodeIndexByKey(p.childKey);
            if (cIdx < 0) return failCheck("3.3 missing child pivot");
            handleLevelNodeClick(avlLayout.nodes[cIdx].id, p.childKey);
            animCtrl.finishInstant();
            int rIdx = avlLayout.findNodeIndexByKey(p.rootKey);
            if (rIdx < 0) return failCheck("3.3 missing root pivot");
            handleLevelNodeClick(avlLayout.nodes[rIdx].id, p.rootKey);
            int badKey = -1;
            if (!avl.isValidAVL() || !avl.getRoot() || avl.getRoot()->key != p.innerKey ||
                !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.3 real AVL double rotation produced invalid tree or wrong root");
            }
            animCtrl.finishInstant();
            if (act3NextPuzzlePending) loadNextAct3Puzzle();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("3.3 clean LR/RL solve failed");
        }
        std::cout << "PASS 3.3: Double Rotations LR & RL via case ID + two real rotations verified (3 stars)\n";

        // 3.4: Auto Mode & Prediction (predict LL/RR/LR/RL/None before real avl.insert)
        startLevel(13);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            const std::string& exp0 = levelData[13].inserts[0].expectedRotation;
            handleAct3Prediction(exp0 == "LL" ? "RR" : "LL");
            if (scoring.alarm() != 2 ||
                scoring.score() != (SCORE_BASE - 2 * SCORE_PER_ALARM_STEP - SCORE_ROTATION_PENALTY)) {
                return failCheck("3.4 wrong prediction must apply +2/-50 penalty");
            }
        }
        startLevel(13);
        showingBriefing = false;
        for (int i = 0; i < levelData[13].insertCount; ++i) {
            handleAct3Prediction(levelData[13].inserts[i].expectedRotation.c_str());
            int badKey = -1;
            if (!avl.isValidAVL() || !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.4 real AVL insert produced invalid tree or broken subtree size");
            }
            animCtrl.finishInstant();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("3.4 clean rotation prediction solve failed");
        }
        std::cout << "PASS 3.4: Rotation Prediction (LL/RR/LR/RL/None) + real AVL insert verified (3 stars)\n";

        // 3.5: Order-Statistic Boss (k-th MOST VALUABLE / LARGEST + live rotation subtree size validation + O(log n))
        startLevel(14);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int idx30 = avlLayout.findNodeIndexByKey(30);
            if (idx30 < 0) return failCheck("3.5 missing node 30");
            handleLevelNodeClick(avlLayout.nodes[idx30].id, 30);
            if (scoring.alarm() != 1) return failCheck("3.5 wrong click must add +1 alarm");
        }
        startLevel(14);
        showingBriefing = false;
        layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        {
            int c1 = 0;
            int k1 = kthLargestAvl(avl, levelData[14].kQueries[0], c1);
            if (k1 != 50 || c1 > avl.getHeight()) return failCheck("3.5 challenge 1 k-th largest mismatch (expected 50)");
            handleLevelNodeClick(avlLayout.nodes[avlLayout.findNodeIndexByKey(k1)].id, k1);
            animCtrl.finishInstant();
            int badKey = -1;
            if (!avl.isValidAVL() || avl.getSize() != 9 || !validateAvlSubtreeSizes(avl.getRoot(), badKey)) {
                return failCheck("3.5 live key-22 rotation failed subtree size validation");
            }
            int c2 = 0;
            int k2 = kthLargestAvl(avl, levelData[14].kQueries[1], c2);
            if (k2 != 25 || c2 > avl.getHeight()) return failCheck("3.5 challenge 2 k-th largest mismatch (expected 25)");
            handleLevelNodeClick(avlLayout.nodes[avlLayout.findNodeIndexByKey(k2)].id, k2);
            animCtrl.finishInstant();
            completePendingWin();
            if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || !act3SizeOk ||
                act3LastComps <= 0 || act3LastComps > act3LastHeight) {
                return failCheck("3.5 Order-Statistic Boss verification failed");
            }
        }
        std::cout << "PASS 3.5: Order-Statistic Boss (k-th largest {50,25}, O(log n) comps, live rotation sz check) verified (3 stars)\n";

        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 4 levels (--verify-act4)
    if (verifyAct4) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            return 1;
        };
        auto checkArrayMaxHeap = [&](const int* arr, int n) -> bool {
            for (int i = 1; i < n; ++i) {
                if (arr[(i - 1) / 2] < arr[i]) return false;
            }
            return true;
        };

        // 4.1: Activate the Vault (bottom-up build + invalid penalty + 3 stars)
        startLevel(15);
        showingBriefing = false;
        handleLevelNodeClick(0, heapArr[0]); // wrong order click (expected 2 or 6) -> PENALTY_ACT4_INVALID
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("4.1 wrong click must add +1 alarm");

        startLevel(15);
        showingBriefing = false;
        while (buildStepIdx >= 0) {
            handleLevelNodeClick(buildStepIdx, heapArr[buildStepIdx]);
            animCtrl.finishInstant();
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || !checkArrayMaxHeap(heapArr, activeHeapSize)) {
            return failCheck("4.1 clean bottom-up build failed");
        }
        std::cout << "PASS 4.1: Bottom-up Max Heap construction & +1 penalty verified (3 stars)\n";

        // 4.2: Repair Security Locks (sift-down root + invalid penalty + 3 stars)
        startLevel(16);
        showingBriefing = false;
        handleLevelNodeClick(4, heapArr[4]); // wrong node -> PENALTY_ACT4_INVALID
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("4.2 wrong click must add +1 alarm");

        startLevel(16);
        showingBriefing = false;
        handleLevelNodeClick(buildStepIdx, heapArr[buildStepIdx]);
        animCtrl.finishInstant();
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || !checkArrayMaxHeap(heapArr, activeHeapSize)) {
            return failCheck("4.2 root sift-down repair failed");
        }
        std::cout << "PASS 4.2: Sabotaged root sift-down repair verified (3 stars)\n";

        // 4.3: Extract the Greatest Treasure (reject deliberately wrong sorted order, accept ascending tail)
        startLevel(17);
        showingBriefing = false;
        for (int e = 0; e < 6; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        sortedArr[0] = 999; // tamper sorted region before final extraction
        handleLevelNodeClick(0, act4Heap.getMax());
        animCtrl.finishInstant();
        completePendingWin();
        if (pendingWin || currentScreen == SCREEN_RESULT || scoring.penaltyEvents() < 1) {
            return failCheck("4.3 must reject mismatched sortedArr instead of winning");
        }

        startLevel(17);
        showingBriefing = false;
        while (!act4Heap.isEmpty()) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (!pendingWin || sortedCount != levelData[17].expectedSortedResultCount ||
            !validateAct4SortedResult(levelData[17], sortedArr, sortedCount)) {
            return failCheck("4.3 extracted sorted region is not strictly ascending expected_sorted_result");
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3) {
            return failCheck("4.3 clean optimal run must earn 3 stars");
        }
        std::cout << "PASS 4.3: Ascending tail [40..95] & expected_sorted_result mismatch guard verified (3 stars)\n";

        // 4.4: Race Against the Guards (complexity/step HUD readout + ascending extraction + 3 stars)
        startLevel(18);
        showingBriefing = false;
        if (status.find("0/7") == std::string::npos || status.find("O(log n)") == std::string::npos) {
            return failCheck("4.4 initial status missing step/O(log n) complexity readout");
        }
        handleLevelNodeClick(2, act4Heap.getElementAt(2)); // wrong click != root 0
        if (scoring.alarm() != 1 || scoring.penaltyEvents() != 1) return failCheck("4.4 non-root click must add +1 alarm");

        startLevel(18);
        showingBriefing = false;
        for (int e = 0; e < 3; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (status.find("3/7") == std::string::npos || status.find("O(log n)") == std::string::npos) {
            return failCheck("4.4 live status missing step/complexity progress readout");
        }
        while (!act4Heap.isEmpty()) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (!validateAct4SortedResult(levelData[18], sortedArr, sortedCount)) {
            return failCheck("4.4 ascending sorted result mismatch");
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 ||
            status.find("7/7") == std::string::npos || status.find("O(log n)") == std::string::npos) {
            return failCheck("4.4 clean optimal race failed or missing final complexity summary");
        }
        std::cout << "PASS 4.4: Race Against the Guards live Theta(n log n) step readout & ascending tail verified (3 stars)\n";

        // 4.5: Final Vault Boss (wrong sortedArr guard + Build Max Heap + 8 extractions = 9 optimal steps -> 3 stars)
        startLevel(19);
        showingBriefing = false;
        triggerStructuralAction(1);
        animCtrl.finishInstant();
        for (int e = 0; e < 7; ++e) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        sortedArr[0] = 1; // deliberately break ascending order before last extraction
        handleLevelNodeClick(0, act4Heap.getMax());
        animCtrl.finishInstant();
        completePendingWin();
        if (pendingWin || currentScreen == SCREEN_RESULT || scoring.penaltyEvents() < 1) {
            return failCheck("4.5 boss must reject wrong sorted array against expected_sorted_result");
        }

        startLevel(19);
        showingBriefing = false;
        triggerStructuralAction(1); // Phase 1: Build Max Heap (1 step)
        animCtrl.finishInstant();
        while (!act4Heap.isEmpty()) {
            handleLevelNodeClick(0, act4Heap.getMax());
            animCtrl.finishInstant();
        }
        if (!validateAct4SortedResult(levelData[19], sortedArr, sortedCount)) {
            return failCheck("4.5 boss sorted tail did not match ascending expected_sorted_result");
        }
        completePendingWin();
        if (currentScreen != SCREEN_RESULT || scoring.stars() != 3 || scoring.steps() != 9) {
            return failCheck("4.5 boss clean optimal run failed to earn 3 stars in 9 steps");
        }
        std::cout << "PASS 4.5: Final Vault Boss (Build Max Heap + ascending tail + mismatch guard) verified (3 stars)\n";

        return 0;
    }

    // End-to-end automated sanity check for Endless Mode (--verify-endless)
    if (verifyEndless) {
        const char* tmpSave = "verify_endless_save_tmp.txt";
        const char* tmpLb   = "verify_endless_lb_tmp.txt";
        std::remove(tmpSave);
        std::remove(tmpLb);
        scoreLog.load("");
        endlessLb.load(tmpLb);

        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            std::remove(tmpSave);
            std::remove(tmpLb);
            return 1;
        };

        startEndlessMode(424242u);
        if (currentScreen != SCREEN_ENDLESS || !endlessSession.isActive() || !EndlessSession::validateTreeFull(avl)) {
            return failCheck("Endless startNew failed to enter SCREEN_ENDLESS with valid AVL");
        }

        float w0 = endlessSession.getOpWindowSec();
        int prevSurvMs = 0;
        for (int step = 0; step < 12; ++step) {
            const EndlessOp op = endlessSession.getPendingOp();
            if (op.type == ENDLESS_OP_DELETE && !avl.search(op.key)) {
                return failCheck("Endless DELETE op targeted absent key");
            }
            if (op.type == ENDLESS_OP_INSERT && avl.search(op.key)) {
                return failCheck("Endless INSERT op targeted duplicate key");
            }
            if (endlessSession.tick(0.55f)) {
                return failCheck("Endless expired unexpectedly inside active window");
            }
            if (endlessSession.getSurvivalMs() <= prevSurvMs) {
                return failCheck("Endless survival time failed to increment");
            }
            prevSurvMs = endlessSession.getSurvivalMs();
            triggerEndlessExecute();
            animCtrl.finishInstant();
            if (!EndlessSession::validateTreeFull(avl)) {
                return failCheck("AVLTree validator failed after Endless operation");
            }
        }
        if (endlessSession.getOpsCompleted() != 12 || endlessSession.getOpWindowSec() >= w0) {
            return failCheck("Endless difficulty scaling (shrinking window) or op count mismatch");
        }

        int savedSize = avl.getSize();
        int savedRoot = avl.getRoot() ? avl.getRoot()->key : -1;
        int savedSurvMs = endlessSession.getSurvivalMs();
        EndlessOp savedNextOp = endlessSession.getPendingOp();
        saveEndlessRun(tmpSave);

        avl.clear();
        if (!loadEndlessRun(tmpSave)) return failCheck("Endless loadFromFile failed");
        if (!EndlessSession::validateTreeFull(avl) || avl.getSize() != savedSize ||
            !avl.getRoot() || avl.getRoot()->key != savedRoot ||
            endlessSession.getInitialSeed() != 424242u ||
            endlessSession.getOpsCompleted() != 12 ||
            endlessSession.getSurvivalMs() != savedSurvMs ||
            endlessSession.getPendingOp().type != savedNextOp.type ||
            endlessSession.getPendingOp().key != savedNextOp.key) {
            return failCheck("Endless save/load did not faithfully restore AVL tree, seed, or state");
        }

        for (int step = 0; step < 3; ++step) {
            endlessSession.tick(0.30f);
            triggerEndlessExecute();
            animCtrl.finishInstant();
            if (!EndlessSession::validateTreeFull(avl)) return failCheck("Resumed Endless run broke AVL invariants");
        }

        if (!endlessSession.tick(endlessSession.getOpTimeRemaining() + 0.1f)) {
            return failCheck("Endless deadline expiry failed to trigger game-over");
        }
        finishEndlessGameOver();
        if (currentScreen != SCREEN_ENDLESS_LEADERBOARD || !endlessSession.isGameOver()) {
            return failCheck("Game over did not transition to SCREEN_ENDLESS_LEADERBOARD");
        }

        EndlessLeaderboardEntry highRun;
        highRun.nameDate = "VerifyTop";
        highRun.survivalMs = 99000;
        highRun.opsCompleted = 40;
        highRun.seed = 777u;
        endlessLb.recordRun(highRun);

        EndlessLeaderboard checkLb;
        checkLb.load(tmpLb);
        if (checkLb.getCount() != 2 || checkLb.get(0).survivalMs != 99000 ||
            checkLb.get(1).seed != 424242u) {
            return failCheck("Endless leaderboard write/read/descending sort failed");
        }

        std::remove(tmpSave);
        std::remove(tmpLb);
        std::cout << "PASS Endless: Seeded stream, AVL validator, difficulty ramp, preorder+# save/load & leaderboard verified\n";
        return 0;
    }

    // Optional uncapped FPS profiler (--profile-fps)
    if (profileFps) {
        window.setFramerateLimit(0);
        window.setVerticalSyncEnabled(false);
        auto benchScene = [&](const char* label, const UiInput& in) -> double {
            const int WARMUP = 5;
            const int MAX_FRAMES = 300;
            for (int f = 0; f < WARMUP; ++f) {
                while (const auto ev = window.pollEvent()) { (void)ev; }
                renderFrame(in);
            }
            double sumCpu = 0.0, sumDisp = 0.0;
            int actualFrames = 0;
            sf::Clock clk;
            for (int f = 0; f < MAX_FRAMES; ++f) {
                while (const auto ev = window.pollEvent()) { (void)ev; }
                double cpuMs = 0.0, dispMs = 0.0;
                renderFrame(in, "", &cpuMs, &dispMs);
                sumCpu += cpuMs;
                sumDisp += dispMs;
                actualFrames++;
                if (actualFrames >= 15 && clk.getElapsedTime().asSeconds() >= 1.5f) break;
            }
            double secs = static_cast<double>(clk.getElapsedTime().asSeconds());
            double fps = (secs > 0.0) ? (static_cast<double>(actualFrames) / secs) : 0.0;
            double ms = (secs > 0.0) ? (secs * 1000.0 / static_cast<double>(actualFrames)) : 0.0;
            std::cout << label << ": " << fps << " FPS (" << ms << " ms/frame | cpu="
                      << (sumCpu / actualFrames) << " ms, display=" << (sumDisp / actualFrames)
                      << " ms, frames=" << actualFrames << ")\n";
            return fps;
        };

        UiInput in;
        double sumFps = 0.0;

        // 1. Level 2.2 (BST + EventLog + Toast + Tooltip)
        startLevel(6);
        showingBriefing = false;
        {
            LayoutResult before;
            LayoutEngine::layoutBST(act2Bst, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
            EventBatch batch;
            act2Bst.search(80, batch);
            layoutBstForScreen(false);
            animCtrl.loadOperation(batch, before, bstLayout, false, false);
        }
        in.mx = (bstLayout.nodeCount > 0) ? bstLayout.nodes[bstLayout.nodeCount / 2].x : 600.f;
        in.my = (bstLayout.nodeCount > 0) ? bstLayout.nodes[bstLayout.nodeCount / 2].y : 300.f;
        toastQueue.push("Clicked Leaf 80 cleanly!", TOAST_KIND_SUCCESS);
        toastQueue.update(0.25f);
        sumFps += benchScene("Level 2.2 (BST + EventLog + Toast + Tooltip)", in);

        // 2. Level 4.1 (MaxHeap + Synchronised Array View + Tooltip)
        toastQueue.clear();
        startLevel(15);
        showingBriefing = false;
        layoutHeapForScreen(false, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
        in.mx = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].x : 600.f;
        in.my = (heapLayout.nodeCount > 0) ? heapLayout.nodes[0].y : 300.f;
        sumFps += benchScene("Level 4.1 (MaxHeap + Array + Tooltip)", in);

        // 3. Sandbox (Dual AVL + MaxHeap + EventLog)
        enterSandbox();
        {
            LayoutResult before;
            LayoutEngine::layoutAVL(avl, 30.f, 95.f, 560.f, 540.f, before);
            EventBatch batch;
            avl.insert(15, batch);
            layoutAvlForScreen(30.f, 95.f, 560.f, 540.f);
            animCtrl.loadOperation(batch, before, avlLayout, true, false);
        }
        in.mx = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].x : 300.f;
        in.my = (avlLayout.nodeCount > 0) ? avlLayout.nodes[0].y : 300.f;
        sumFps += benchScene("Sandbox (Dual AVL + MaxHeap + EventLog)", in);

        // 4. Campaign Map (20 level buttons + 4 wing panels)
        toastQueue.clear();
        currentScreen = SCREEN_CAMPAIGN_MAP;
        in.mx = 800.f; in.my = 450.f;
        sumFps += benchScene("Campaign Map (20 buttons + 4 panels)", in);

        std::cout << "Average FPS: " << (sumFps / 4.0) << "\n";
        return 0;
    }

    // =========================================================================
    // INTERACTIVE MAIN LOOP
    // =========================================================================
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        fpsFrameAccum++;
        float fpsSec = fpsReportClock.getElapsedTime().asSeconds();
        if (fpsSec >= 0.25f) {
            int instFps = static_cast<int>(static_cast<float>(fpsFrameAccum) / fpsSec + 0.5f);
            liveFpsStr = "FPS: " + std::to_string(instFps);
            fpsFrameAccum = 0;
            fpsReportClock.restart();
        }
        if (dt > 0.05f) dt = 0.05f;
        dialAngle += 50.f * dt;
        animCtrl.update(dt);
        toastQueue.update(dt);
        if (act2TwoChildPhase2Pending && animCtrl.isAtEnd() && !animCtrl.isPlaying()) {
            startAct2TwoChildPhase2();
        }
        if (act3NextPuzzlePending && animCtrl.isAtEnd() && !animCtrl.isPlaying()) {
            loadNextAct3Puzzle();
        }
        if (pendingWin && !act2TwoChildPhase2Pending && !act3NextPuzzlePending &&
            animCtrl.isAtEnd() && !animCtrl.isPlaying()) {
            completePendingWin();
        }
        if (currentScreen == SCREEN_ENDLESS && endlessSession.isActive() && !endlessPaused) {
            if (endlessSession.tick(dt)) {
                finishEndlessGameOver();
            }
        }

        uiAnimTimer += dt;
        if (currentScreen == SCREEN_SPLASH) {
            splashTimer += dt;
            if (splashTimer >= 3.8f) {
                currentScreen = SCREEN_MAIN_MENU;
            }
        }

        sf::Vector2i mPos = sf::Mouse::getPosition(window);
        UiInput in;
        in.mx = static_cast<float>(mPos.x);
        in.my = static_cast<float>(mPos.y);
        in.down = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        if (draggingSpeedSlider) updateSpeedFromMouse(in.mx, sliderPanelX, sliderPanelW);
        if (draggingScrubber)    updateScrubberFromMouse(in.mx, sliderPanelX, sliderPanelW);

        auto updateCamPan = [&](CanvasCamera& cam) {
            if (!cam.panning) return;
            bool anyBtn = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ||
                          sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) ||
                          sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle);
            if (!anyBtn) { cam.panning = false; return; }
            if (mPos != cam.lastPanPixel) {
                sf::Vector2f w0 = window.mapPixelToCoords(cam.lastPanPixel, cam.view);
                sf::Vector2f w1 = window.mapPixelToCoords(mPos, cam.view);
                cam.view.move(w0 - w1);
                cam.clampCenter();
                cam.lastPanPixel = mPos;
            }
        };
        if (currentScreen == SCREEN_LEVEL_PLAY && !showingBriefing) {
            if (dragDrop.isDragging()) {
                sf::Vector2f wDrag = window.mapPixelToCoords(mPos, levelCam.view);
                dragDrop.move(wDrag.x, wDrag.y);
            }
            updateCamPan(levelCam);
        }
        else if (currentScreen == SCREEN_ENDLESS) updateCamPan(endlessCam);
        else if (currentScreen == SCREEN_SANDBOX) { updateCamPan(sbAvlCam); updateCamPan(sbHeapCam); }
        else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) updateCamPan(demoCam);

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                if (currentScreen == SCREEN_MAIN_MENU) {
                    sf::Vector2f mp = window.mapPixelToCoords(mm->position);
                    for (int i = 0; i < 6; ++i) {
                        float itemY = 300.f + static_cast<float>(i) * 70.f;
                        if (Ui::inside(mp.x, mp.y, 90.f, itemY - 10.f, 430.f, 54.f)) {
                            menuSelectedIdx = i;
                            break;
                        }
                    }
                }
            }
            if (const auto* mr = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mr->button == sf::Mouse::Button::Left && dragDrop.isDragging()) {
                    sf::Vector2f wRel = window.mapPixelToCoords(mr->position, levelCam.view);
                    DropResult dropRes = dragDrop.release(wRel.x, wRel.y);
                    if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0 &&
                        levels[currentLevelIdx].id == LEVEL_1_2) {
                        handleAct1BlueprintDrop(dropRes);
                    }
                }
                draggingSpeedSlider = false;
                draggingScrubber = false;
                levelCam.panning = false;
                endlessCam.panning = false;
                demoCam.panning = false;
                sbAvlCam.panning = false;
                sbHeapCam.panning = false;
            }

            if (const auto* mw = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mw->wheel == sf::Mouse::Wheel::Vertical) {
                    if (currentScreen == SCREEN_LEVEL_PLAY && !showingBriefing) {
                        levelCam.handleZoom(window, mw->position, mw->delta);
                    } else if (currentScreen == SCREEN_ENDLESS) {
                        endlessCam.handleZoom(window, mw->position, mw->delta);
                    } else if (currentScreen == SCREEN_SANDBOX) {
                        if (!sbAvlCam.handleZoom(window, mw->position, mw->delta)) {
                            sbHeapCam.handleZoom(window, mw->position, mw->delta);
                        }
                    } else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) {
                        demoCam.handleZoom(window, mw->position, mw->delta);
                    }
                }
            }

            if (const auto* te = event->getIf<sf::Event::TextEntered>()) {
                if (currentScreen == SCREEN_CAMPAIGN_MAP && demoPromptOpen) {
                    std::uint32_t uc = te->unicode;
                    if (uc == 8) {
                        if (demoPasswordLen > 0) {
                            --demoPasswordLen;
                            demoPasswordBuf[demoPasswordLen] = '\0';
                        }
                    } else if (uc >= 32 && uc < 127) {
                        if (demoPasswordLen < 32) {
                            demoPasswordBuf[demoPasswordLen++] = static_cast<char>(uc);
                            demoPasswordBuf[demoPasswordLen] = '\0';
                        }
                    }
                    continue;
                }
            }

            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (currentScreen == SCREEN_SPLASH) {
                    currentScreen = SCREEN_MAIN_MENU;
                    continue;
                }
                if (kp->code == sf::Keyboard::Key::Escape) {
                    if (currentScreen == SCREEN_CAMPAIGN_MAP && demoPromptOpen) {
                        demoPromptOpen = false;
                        demoPasswordLen = 0;
                        demoPasswordBuf[0] = '\0';
                        continue;
                    }
                    if (currentScreen == SCREEN_LEVEL_PLAY || currentScreen == SCREEN_RESULT) {
                        dragDrop.cancel();
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Returned to Campaign Map";
                    } else if (currentScreen == SCREEN_ENDLESS) {
                        endlessSession.stop();
                        endlessPaused = false;
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    } else if (currentScreen == SCREEN_CAMPAIGN_MAP || currentScreen == SCREEN_SANDBOX ||
                               currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO ||
                               currentScreen == SCREEN_LEADERBOARD || currentScreen == SCREEN_SETTINGS ||
                               currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    } else {
                        window.close();
                    }
                    continue;
                }
                if (currentScreen == SCREEN_CAMPAIGN_MAP && demoPromptOpen) {
                    if (kp->code == sf::Keyboard::Key::Enter) {
                        submitDemoPassword();
                    }
                    continue;
                }

                if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
                    if (showingBriefing) {
                        if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space) {
                            showingBriefing = false;
                        }
                        continue;
                    }
                    int lid = levels[currentLevelIdx].id;
                    if (kp->code == sf::Keyboard::Key::H)     useHint();
                    if (kp->code == sf::Keyboard::Key::Z)     performUndo();
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (lid == LEVEL_1_1 && kp->code == sf::Keyboard::Key::C) {
                        cycleAct1TravMode();
                    } else if (lid == LEVEL_1_3) {
                        if (kp->code == sf::Keyboard::Key::Up || kp->code == sf::Keyboard::Key::Equal ||
                            kp->code == sf::Keyboard::Key::Add) {
                            act1StepperVal = minInt(20, act1StepperVal + 1);
                        } else if (kp->code == sf::Keyboard::Key::Down || kp->code == sf::Keyboard::Key::Hyphen ||
                                   kp->code == sf::Keyboard::Key::Subtract) {
                            act1StepperVal = maxInt(1, act1StepperVal - 1);
                        } else if (kp->code == sf::Keyboard::Key::Enter) {
                            submitAct1HeightOrDiameter();
                        } else {
                            int numCode = static_cast<int>(kp->code);
                            int n1 = static_cast<int>(sf::Keyboard::Key::Num1);
                            int n9 = static_cast<int>(sf::Keyboard::Key::Num9);
                            if (numCode >= n1 && numCode <= n9) {
                                act1StepperVal = (numCode - n1) + 1;
                            }
                        }
                    } else if (lid == LEVEL_1_5 && (kp->code == sf::Keyboard::Key::A || kp->code == sf::Keyboard::Key::M)) {
                        triggerStructuralAction(1);
                    } else if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) {
                        if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::L) {
                            handleAct2CompareChoice(1);
                        } else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::F ||
                                   kp->code == sf::Keyboard::Key::Enter) {
                            handleAct2CompareChoice(2);
                        } else if (kp->code == sf::Keyboard::Key::Num3 || kp->code == sf::Keyboard::Key::D) {
                            handleAct2CompareChoice(3);
                        }
                    } else if (lid == LEVEL_2_3) {
                        if (kp->code == sf::Keyboard::Key::Num1) handleAct2DeleteCase(1);
                        else if (kp->code == sf::Keyboard::Key::Num2) handleAct2DeleteCase(2);
                        else if (kp->code == sf::Keyboard::Key::Num3) handleAct2DeleteCase(3);
                    } else if (lid == LEVEL_2_4) {
                        if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::R) {
                            handleAct2RangeDecision(RQ_ACT_REPORT);
                        } else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::L) {
                            handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT);
                        } else if (kp->code == sf::Keyboard::Key::Num3 || kp->code == sf::Keyboard::Key::P) {
                            handleAct2RangeDecision(RQ_ACT_PRUNE_RIGHT);
                        }
                    } else if (lid == LEVEL_3_2 && !pendingWin) {
                        if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::L) {
                            handleAct3SingleRotation(1);
                        } else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::R) {
                            handleAct3SingleRotation(2);
                        }
                    } else if (lid == LEVEL_3_3 && manualDoubleStep == 0 && !pendingWin) {
                        if (kp->code == sf::Keyboard::Key::Num1) handleAct3DoubleCase(1);
                        else if (kp->code == sf::Keyboard::Key::Num2) handleAct3DoubleCase(2);
                    } else if (lid == LEVEL_3_4 && !pendingWin) {
                        if (kp->code == sf::Keyboard::Key::Num1) handleAct3Prediction("LL");
                        else if (kp->code == sf::Keyboard::Key::Num2) handleAct3Prediction("RR");
                        else if (kp->code == sf::Keyboard::Key::Num3) handleAct3Prediction("LR");
                        else if (kp->code == sf::Keyboard::Key::Num4) handleAct3Prediction("RL");
                        else if (kp->code == sf::Keyboard::Key::Num5 || kp->code == sf::Keyboard::Key::N) {
                            handleAct3Prediction("None");
                        }
                    } else if (lid == LEVEL_4_5 && kp->code == sf::Keyboard::Key::B) {
                        triggerStructuralAction(1);
                    }
                } else if (currentScreen == SCREEN_RESULT) {
                    if (kp->code == sf::Keyboard::Key::R) startLevel(currentLevelIdx);
                    if (kp->code == sf::Keyboard::Key::N) {
                        if (currentLevelIdx + 1 < TOTAL_LEVELS && levels[currentLevelIdx + 1].unlocked) {
                            startLevel(currentLevelIdx + 1);
                        } else {
                            currentScreen = SCREEN_CAMPAIGN_MAP;
                        }
                    }
                } else if (currentScreen == SCREEN_MAIN_MENU) {
                    auto activateMenuIdx = [&](int idx) {
                        if (idx == 0) { currentScreen = SCREEN_CAMPAIGN_MAP; status = "Heist Wings Overview"; }
                        else if (idx == 1) startEndlessMode(0u);
                        else if (idx == 2) enterSandbox();
                        else if (idx == 3) currentScreen = SCREEN_LEADERBOARD;
                        else if (idx == 4) currentScreen = SCREEN_SETTINGS;
                        else if (idx == 5) window.close();
                    };
                    if (kp->code == sf::Keyboard::Key::Up || kp->code == sf::Keyboard::Key::W) {
                        menuSelectedIdx = (menuSelectedIdx + 5) % 6;
                    } else if (kp->code == sf::Keyboard::Key::Down || kp->code == sf::Keyboard::Key::S) {
                        menuSelectedIdx = (menuSelectedIdx + 1) % 6;
                    } else if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space) {
                        activateMenuIdx(menuSelectedIdx);
                    } else if (kp->code == sf::Keyboard::Key::Num1) { menuSelectedIdx = 0; activateMenuIdx(0); }
                    else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::E) { menuSelectedIdx = 1; activateMenuIdx(1); }
                    else if (kp->code == sf::Keyboard::Key::Num3) { menuSelectedIdx = 2; activateMenuIdx(2); }
                    else if (kp->code == sf::Keyboard::Key::Num4) { menuSelectedIdx = 3; activateMenuIdx(3); }
                    else if (kp->code == sf::Keyboard::Key::Num5) { menuSelectedIdx = 4; activateMenuIdx(4); }
                    else if (kp->code == sf::Keyboard::Key::Num6) { menuSelectedIdx = 5; activateMenuIdx(5); }
                } else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    if (kp->code == sf::Keyboard::Key::Up || kp->code == sf::Keyboard::Key::W) {
                        selectedActIdx = (selectedActIdx + 3) % 4;
                    } else if (kp->code == sf::Keyboard::Key::Down || kp->code == sf::Keyboard::Key::S) {
                        selectedActIdx = (selectedActIdx + 1) % 4;
                    } else if (kp->code == sf::Keyboard::Key::Left || kp->code == sf::Keyboard::Key::A) {
                        selectedLevelInAct = (selectedLevelInAct + 4) % 5;
                    } else if (kp->code == sf::Keyboard::Key::Right || kp->code == sf::Keyboard::Key::D) {
                        selectedLevelInAct = (selectedLevelInAct + 1) % 5;
                    } else if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::Space) {
                        int li = selectedActIdx * 5 + selectedLevelInAct;
                        if (levels[li].unlocked) startLevel(li);
                        else toastQueue.push("Locked! Clear previous level first", TOAST_KIND_WARNING);
                    }
                } else if (currentScreen == SCREEN_LEADERBOARD) {
                    if (kp->code == sf::Keyboard::Key::E) {
                        endlessShowLastResult = false;
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                    }
                } else if (currentScreen == SCREEN_ENDLESS) {
                    if (kp->code == sf::Keyboard::Key::Enter || kp->code == sf::Keyboard::Key::X) {
                        triggerEndlessExecute();
                    } else if (kp->code == sf::Keyboard::Key::P) {
                        endlessPaused = !endlessPaused;
                        toastQueue.push(endlessPaused ? "Endless paused" : "Endless resumed!", TOAST_KIND_WARNING);
                    } else if (kp->code == sf::Keyboard::Key::F5) {
                        saveEndlessRun();
                    } else if (kp->code == sf::Keyboard::Key::F9) {
                        loadEndlessRun();
                    } else if (kp->code == sf::Keyboard::Key::L) {
                        endlessSession.stop();
                        endlessPaused = false;
                        endlessShowLastResult = false;
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                    } else if (kp->code == sf::Keyboard::Key::R) {
                        startEndlessMode(0u);
                    } else if (kp->code == sf::Keyboard::Key::Right) {
                        animCtrl.setPlaying(false);
                        stepAnimForward();
                    } else if (kp->code == sf::Keyboard::Key::Space) {
                        animCtrl.togglePlayPause();
                    }
                } else if (currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
                    if (kp->code == sf::Keyboard::Key::R) startEndlessMode(0u);
                    else if (kp->code == sf::Keyboard::Key::S) startEndlessMode(endlessLastSeed);
                    else if (kp->code == sf::Keyboard::Key::C) currentScreen = SCREEN_LEADERBOARD;
                } else if (currentScreen == SCREEN_SANDBOX) {
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (kp->code == sf::Keyboard::Key::Z) performUndo();
                    if (kp->code == sf::Keyboard::Key::I) triggerSandboxAction(1);
                    if (kp->code == sf::Keyboard::Key::D) triggerSandboxAction(2);
                    if (kp->code == sf::Keyboard::Key::K) triggerSandboxAction(3);
                    if (kp->code == sf::Keyboard::Key::E) triggerSandboxAction(4);
                    if (kp->code == sf::Keyboard::Key::R) triggerSandboxAction(5);
                } else if (currentScreen == SCREEN_ACT1_DEMO) {
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (kp->code == sf::Keyboard::Key::T) triggerAct1Action(1);
                    if (kp->code == sf::Keyboard::Key::M) triggerAct1Action(2);
                    if (kp->code == sf::Keyboard::Key::R) triggerAct1Action(3);
                    if (kp->code == sf::Keyboard::Key::H) triggerAct1Action(4);
                    if (kp->code == sf::Keyboard::Key::L) triggerAct1Action(5);
                    if (kp->code == sf::Keyboard::Key::S) triggerAct1Action(6);
                    if (kp->code == sf::Keyboard::Key::Z) triggerAct1Action(7);
                } else if (currentScreen == SCREEN_ACT2_DEMO) {
                    if (kp->code == sf::Keyboard::Key::Right) { animCtrl.setPlaying(false); stepAnimForward(); }
                    if (kp->code == sf::Keyboard::Key::Space) animCtrl.togglePlayPause();
                    if (kp->code == sf::Keyboard::Key::I) triggerAct2Action(1);
                    if (kp->code == sf::Keyboard::Key::D) triggerAct2Action(2);
                    if (kp->code == sf::Keyboard::Key::S) triggerAct2Action(3);
                    if (kp->code == sf::Keyboard::Key::M) triggerAct2Action(4);
                    if (kp->code == sf::Keyboard::Key::P) triggerAct2Action(5);
                    if (kp->code == sf::Keyboard::Key::R) triggerAct2Action(6);
                    if (kp->code == sf::Keyboard::Key::K) triggerAct2Action(7);
                    if (kp->code == sf::Keyboard::Key::C) triggerAct2Action(8);
                    if (kp->code == sf::Keyboard::Key::Z) triggerAct2Action(9);
                }
            }

            if (const auto* mc = event->getIf<sf::Event::MouseButtonPressed>()) {
                float mx = static_cast<float>(mc->position.x);
                float my = static_cast<float>(mc->position.y);
                bool isLeft   = (mc->button == sf::Mouse::Button::Left);
                bool isPanBtn = (mc->button == sf::Mouse::Button::Right || mc->button == sf::Mouse::Button::Middle);

                // Right/Middle button inside canvas: double-click resets view, drag pans (Part B)
                if (isPanBtn) {
                    auto handlePanPress = [&](CanvasCamera& cam) -> bool {
                        if (!cam.containsPixel(mx, my)) return false;
                        if (cam.checkDoubleClick(mc->position)) {
                            cam.reset();
                            toastQueue.push("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            cam.panning = true;
                            cam.lastPanPixel = mc->position;
                        }
                        return true;
                    };
                    if (currentScreen == SCREEN_LEVEL_PLAY && !showingBriefing) handlePanPress(levelCam);
                    else if (currentScreen == SCREEN_ENDLESS) handlePanPress(endlessCam);
                    else if (currentScreen == SCREEN_SANDBOX) {
                        if (!handlePanPress(sbAvlCam)) handlePanPress(sbHeapCam);
                    } else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) {
                        handlePanPress(demoCam);
                    }
                    continue;
                }
                if (!isLeft) continue;

                if (currentScreen == SCREEN_SPLASH) {
                    currentScreen = SCREEN_MAIN_MENU;
                }
                else if (currentScreen == SCREEN_MAIN_MENU) {
                    for (int i = 0; i < 6; ++i) {
                        float itemY = 300.f + static_cast<float>(i) * 70.f;
                        if (Ui::inside(mx, my, 90.f, itemY - 10.f, 430.f, 54.f)) {
                            menuSelectedIdx = i;
                            if (i == 0) {
                                currentScreen = SCREEN_CAMPAIGN_MAP;
                                status = "Heist Wings Overview";
                            } else if (i == 1) {
                                startEndlessMode(0u);
                            } else if (i == 2) {
                                enterSandbox();
                            } else if (i == 3) {
                                currentScreen = SCREEN_LEADERBOARD;
                            } else if (i == 4) {
                                currentScreen = SCREEN_SETTINGS;
                            } else if (i == 5) {
                                window.close();
                            }
                            break;
                        }
                    }
                }
                else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    if (demoPromptOpen) {
                        float mw = 460.f, mh = 210.f;
                        float mx0 = (static_cast<float>(W) - mw) * 0.5f;
                        float my0 = (static_cast<float>(H) - mh) * 0.5f;
                        if (Ui::inside(mx, my, mx0 + 36.f, my0 + 148.f, 180.f, 38.f)) {
                            submitDemoPassword();
                        } else if (Ui::inside(mx, my, mx0 + mw - 216.f, my0 + 148.f, 180.f, 38.f)) {
                            demoPromptOpen = false;
                            demoPasswordLen = 0;
                            demoPasswordBuf[0] = '\0';
                        }
                        continue;
                    }
                    if (Ui::inside(mx, my, static_cast<float>(W) * 0.5f - 135.f, static_cast<float>(H) - 54.f, 270.f, 34.f)) {
                        demoPromptOpen = true;
                        demoPasswordLen = 0;
                        demoPasswordBuf[0] = '\0';
                        continue;
                    }
                    if (Ui::inside(mx, my, 1350.f, 20.f, 220.f, 38.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                        continue;
                    }
                    for (int a = 0; a < 4; ++a) {
                        float bx = 44.f;
                        float by = 190.f + static_cast<float>(a) * 124.f;
                        if (Ui::inside(mx, my, bx, by, 220.f, 66.f)) {
                            selectedActIdx = a;
                            break;
                        }
                    }
                    const float nodeX[5] = { 440.f, 610.f, 780.f, 950.f, 1120.f };
                    const float nodeY[5] = { 360.f, 460.f, 295.f, 450.f, 305.f };
                    const float nodeR = 44.f;
                    for (int i = 0; i < 5; ++i) {
                        float dx = mx - nodeX[i];
                        float dy = my - nodeY[i];
                        if (dx * dx + dy * dy <= nodeR * nodeR) {
                            selectedLevelInAct = i;
                            int li = selectedActIdx * 5 + i;
                            if (levels[li].unlocked) startLevel(li);
                            else toastQueue.push("Locked! Clear previous level first", TOAST_KIND_WARNING);
                            break;
                        }
                    }
                }
                else if (currentScreen == SCREEN_LEADERBOARD) {
                    if (Ui::inside(mx, my, W * 0.5f - 310.f, 770.f, 290.f, 48.f)) {
                        endlessShowLastResult = false;
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                    } else if (Ui::inside(mx, my, W * 0.5f + 20.f, 770.f, 290.f, 48.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                    }
                }
                else if (currentScreen == SCREEN_ENDLESS) {
                    if (Ui::inside(mx, my, 670.f, 18.f, 125.f, 34.f)) {
                        endlessPaused = !endlessPaused;
                        toastQueue.push(endlessPaused ? "Endless paused" : "Endless resumed!", TOAST_KIND_WARNING);
                        continue;
                    }
                    if (Ui::inside(mx, my, 805.f, 18.f, 125.f, 34.f)) { startEndlessMode(0u); continue; }
                    if (Ui::inside(mx, my, 940.f, 18.f, 125.f, 34.f)) {
                        endlessSession.stop();
                        endlessPaused = false;
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                        continue;
                    }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, LOG_Y, LOG_W, LOG_H)) continue;
                    if (Ui::inside(mx, my, 1120.f, 678.f, 428.f, 54.f)) { triggerEndlessExecute(); continue; }
                    if (Ui::inside(mx, my, 1120.f, 748.f, 136.f, 46.f)) { saveEndlessRun(); continue; }
                    if (Ui::inside(mx, my, 1266.f, 748.f, 136.f, 46.f)) { loadEndlessRun(); continue; }
                    if (Ui::inside(mx, my, 1412.f, 748.f, 136.f, 46.f)) {
                        endlessSession.stop();
                        endlessPaused = false;
                        endlessShowLastResult = false;
                        currentScreen = SCREEN_ENDLESS_LEADERBOARD;
                        continue;
                    }
                    if (endlessCam.containsPixel(mx, my)) {
                        if (endlessCam.checkDoubleClick(mc->position)) {
                            endlessCam.reset();
                            toastQueue.push("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            endlessCam.panning = true;
                            endlessCam.lastPanPixel = mc->position;
                        }
                    }
                }
                else if (currentScreen == SCREEN_ENDLESS_LEADERBOARD) {
                    if (Ui::inside(mx, my, 270.f, 770.f, 250.f, 48.f)) startEndlessMode(0u);
                    else if (Ui::inside(mx, my, 540.f, 770.f, 250.f, 48.f)) startEndlessMode(endlessLastSeed);
                    else if (Ui::inside(mx, my, 810.f, 770.f, 250.f, 48.f)) currentScreen = SCREEN_LEADERBOARD;
                    else if (Ui::inside(mx, my, 1080.f, 770.f, 250.f, 48.f)) currentScreen = SCREEN_MAIN_MENU;
                }
                else if (currentScreen == SCREEN_SETTINGS) {
                    float sy0 = 140.f, bw = 560.f, bh = 52.f, bx = W * 0.5f - bw * 0.5f;
                    if (Ui::inside(mx, my, bx, sy0 + 125.f, bw, bh)) soundEnabled   = !soundEnabled;
                    if (Ui::inside(mx, my, bx, sy0 + 195.f, bw, bh)) showNodeBadges = !showNodeBadges;
                    if (Ui::inside(mx, my, bx, sy0 + 265.f, bw, bh)) showBgGrid     = !showBgGrid;
                    if (Ui::inside(mx, my, bx, sy0 + 335.f, bw, bh)) autoPlayEvents = !autoPlayEvents;
                    if (Ui::inside(mx, my, bx, sy0 + 405.f, bw, bh)) {
                        targetFps = (targetFps == 60) ? 120 : 60;
                        window.setFramerateLimit(static_cast<unsigned int>(targetFps));
                    }
                    if (Ui::inside(mx, my, W * 0.5f - 150.f, 740.f, 300.f, 48.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                    }
                }
                else if (currentScreen == SCREEN_RESULT) {
                    float cardY = 170.f;
                    if (Ui::inside(mx, my, W * 0.5f - 230.f, cardY + 365.f, 210.f, 52.f)) startLevel(currentLevelIdx);
                    if (Ui::inside(mx, my, W * 0.5f + 20.f, cardY + 365.f, 210.f, 52.f)) {
                        if (currentLevelIdx + 1 < TOTAL_LEVELS && levels[currentLevelIdx + 1].unlocked) {
                            startLevel(currentLevelIdx + 1);
                        } else {
                            currentScreen = SCREEN_CAMPAIGN_MAP;
                        }
                    }
                    if (Ui::inside(mx, my, W * 0.5f - 120.f, cardY + 436.f, 240.f, 42.f)) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Heist Wings Overview";
                    }
                }
                else if (currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
                    if (showingBriefing) {
                        float bh = 420.f, by = H * 0.5f - bh * 0.5f;
                        if (Ui::inside(mx, my, W * 0.5f - 150.f, by + bh - 74.f, 300.f, 48.f)) {
                            showingBriefing = false;
                        }
                        continue;
                    }
                    if (Ui::inside(mx, my, 722.f, 18.f, 115.f, 34.f)) { useHint(); continue; }
                    if (Ui::inside(mx, my, 850.f, 18.f, 115.f, 34.f)) { performUndo(); continue; }
                    if (Ui::inside(mx, my, 978.f, 18.f, 115.f, 34.f)) {
                        dragDrop.cancel();
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Returned to Campaign Map";
                        continue;
                    }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, LOG_Y, LOG_W, LOG_H)) continue;

                    int lid = levels[currentLevelIdx].id;

                    // Bottom-panel controls for specific levels
                    if (lid == LEVEL_1_1 && act1SeqIdx == 0 && !pendingWin &&
                        Ui::inside(mx, my, 1220.f, 705.f, 310.f, 52.f)) {
                        cycleAct1TravMode();
                        continue;
                    } else if (lid == LEVEL_1_3) {
                        if (Ui::inside(mx, my, 1130.f, 705.f, 60.f, 52.f)) {
                            act1StepperVal = maxInt(1, act1StepperVal - 1);
                            continue;
                        }
                        if (Ui::inside(mx, my, 1360.f, 705.f, 60.f, 52.f)) {
                            act1StepperVal = minInt(20, act1StepperVal + 1);
                            continue;
                        }
                        if (Ui::inside(mx, my, 1430.f, 705.f, 120.f, 52.f)) {
                            submitAct1HeightOrDiameter();
                            continue;
                        }
                    } else if (lid == LEVEL_1_5 && !act1Mirrored && !pendingWin &&
                               Ui::inside(mx, my, 1220.f, 705.f, 310.f, 52.f)) {
                        triggerStructuralAction(1);
                        continue;
                    } else if ((lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 136.f, 52.f)) { handleAct2CompareChoice(1); continue; }
                        if (Ui::inside(mx, my, 1268.f, 705.f, 136.f, 52.f)) { handleAct2CompareChoice(2); continue; }
                        if (Ui::inside(mx, my, 1414.f, 705.f, 136.f, 52.f)) { handleAct2CompareChoice(3); continue; }
                    } else if (lid == LEVEL_2_3 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 136.f, 52.f)) { handleAct2DeleteCase(1); continue; }
                        if (Ui::inside(mx, my, 1268.f, 705.f, 136.f, 52.f)) { handleAct2DeleteCase(2); continue; }
                        if (Ui::inside(mx, my, 1414.f, 705.f, 136.f, 52.f)) { handleAct2DeleteCase(3); continue; }
                    } else if (lid == LEVEL_2_4 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 136.f, 52.f)) { handleAct2RangeDecision(RQ_ACT_REPORT);      continue; }
                        if (Ui::inside(mx, my, 1268.f, 705.f, 136.f, 52.f)) { handleAct2RangeDecision(RQ_ACT_PRUNE_LEFT);  continue; }
                        if (Ui::inside(mx, my, 1414.f, 705.f, 136.f, 52.f)) { handleAct2RangeDecision(RQ_ACT_PRUNE_RIGHT); continue; }
                    } else if (lid == LEVEL_3_2 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 206.f, 52.f)) { handleAct3SingleRotation(1); continue; }
                        if (Ui::inside(mx, my, 1344.f, 705.f, 206.f, 52.f)) { handleAct3SingleRotation(2); continue; }
                    } else if (lid == LEVEL_3_3 && manualDoubleStep == 0 && !pendingWin) {
                        if (Ui::inside(mx, my, 1122.f, 705.f, 206.f, 52.f)) { handleAct3DoubleCase(1); continue; }
                        if (Ui::inside(mx, my, 1344.f, 705.f, 206.f, 52.f)) { handleAct3DoubleCase(2); continue; }
                    } else if (lid == LEVEL_3_4 && !pendingWin) {
                        if (Ui::inside(mx, my, 1038.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("LL");   continue; }
                        if (Ui::inside(mx, my, 1142.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("RR");   continue; }
                        if (Ui::inside(mx, my, 1246.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("LR");   continue; }
                        if (Ui::inside(mx, my, 1350.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("RL");   continue; }
                        if (Ui::inside(mx, my, 1454.f, 705.f, 96.f, 52.f)) { handleAct3Prediction("None"); continue; }
                    } else if (lid == LEVEL_4_5 && !act4Built && Ui::inside(mx, my, 1220.f, 705.f, 310.f, 52.f)) {
                        triggerStructuralAction(1);
                        continue;
                    }

                    // Canvas node/cell hit-testing across all 4 structures
                    const LayoutResult* activeL = nullptr;
                    if (lid >= LEVEL_1_1 && lid <= LEVEL_1_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &btLayout;
                    } else if (lid >= LEVEL_2_1 && lid <= LEVEL_2_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &bstLayout;
                    } else if (lid >= LEVEL_3_1 && lid <= LEVEL_3_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &avlLayout;
                    } else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                        activeL = animCtrl.hasEvents() ? &animCtrl.getRenderLayout() : &heapLayout;
                    }

                    if (activeL && levelCam.containsPixel(mx, my)) {
                        sf::Vector2f wClick = window.mapPixelToCoords(mc->position, levelCam.view);
                        if (dragDrop.press(wClick.x, wClick.y)) {
                            continue;
                        }
                        int hitIdx = (lid == LEVEL_1_2 && !act1Reconstructed) ? -1 : activeL->hitTestNode(wClick.x, wClick.y);
                        if (hitIdx >= 0) {
                            handleLevelNodeClick(activeL->nodes[hitIdx].id, activeL->nodes[hitIdx].key);
                        } else if (lid >= LEVEL_4_1 && lid <= LEVEL_4_5) {
                            int cellIdx = activeL->hitTestCell(wClick.x, wClick.y);
                            if (cellIdx >= 0 && cellIdx < activeL->nodeCount) {
                                handleLevelNodeClick(activeL->nodes[cellIdx].id, activeL->nodes[cellIdx].key);
                            } else {
                                if (levelCam.checkDoubleClick(mc->position)) {
                                    levelCam.reset();
                                    toastQueue.push("Canvas camera reset", TOAST_KIND_SUCCESS);
                                } else {
                                    levelCam.panning = true;
                                    levelCam.lastPanPixel = mc->position;
                                }
                            }
                        } else {
                            if (levelCam.checkDoubleClick(mc->position)) {
                                levelCam.reset();
                                toastQueue.push("Canvas camera reset", TOAST_KIND_SUCCESS);
                            } else {
                                levelCam.panning = true;
                                levelCam.lastPanPixel = mc->position;
                            }
                        }
                    }
                }
                else if (currentScreen == SCREEN_SANDBOX) {
                    if (Ui::inside(mx, my, 1370.f, 26.f, 190.f, 38.f)) { currentScreen = SCREEN_MAIN_MENU; continue; }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, 95.f, LOG_W, 540.f)) continue;
                    if (sbAvlCam.containsPixel(mx, my)) {
                        if (sbAvlCam.checkDoubleClick(mc->position)) {
                            sbAvlCam.reset();
                            toastQueue.push("AVL camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            sbAvlCam.panning = true;
                            sbAvlCam.lastPanPixel = mc->position;
                        }
                    } else if (sbHeapCam.containsPixel(mx, my)) {
                        if (sbHeapCam.checkDoubleClick(mc->position)) {
                            sbHeapCam.reset();
                            toastQueue.push("Heap camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            sbHeapCam.panning = true;
                            sbHeapCam.lastPanPixel = mc->position;
                        }
                    }
                    if (Ui::inside(mx, my, 54.f,   674.f, 190.f, 46.f)) triggerSandboxAction(1);
                    if (Ui::inside(mx, my, 264.f,  674.f, 190.f, 46.f)) triggerSandboxAction(2);
                    if (Ui::inside(mx, my, 474.f,  674.f, 190.f, 46.f)) triggerSandboxAction(3);
                    if (Ui::inside(mx, my, 684.f,  674.f, 200.f, 46.f)) triggerSandboxAction(4);
                    if (Ui::inside(mx, my, 904.f,  674.f, 170.f, 46.f)) performUndo();
                    if (Ui::inside(mx, my, 1094.f, 674.f, 170.f, 46.f)) triggerSandboxAction(5);
                }
                else if (currentScreen == SCREEN_ACT1_DEMO) {
                    if (Ui::inside(mx, my, 1350.f, 26.f, 210.f, 38.f)) { currentScreen = SCREEN_CAMPAIGN_MAP; continue; }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, 110.f, LOG_W, 470.f)) continue;
                    if (demoCam.containsPixel(mx, my)) {
                        const LayoutResult& activeDemoL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : btLayout;
                        sf::Vector2f wClick = window.mapPixelToCoords(mc->position, demoCam.view);
                        int hitIdx = activeDemoL.hitTestNode(wClick.x, wClick.y);
                        if (hitIdx >= 0) {
                            const NodeLayout& clicked = activeDemoL.nodes[hitIdx];
                            selectedNodeId = clicked.id;
                            int pLen = 0;
                            int* p = act1Tree.pathTo(clicked.key, pLen);
                            std::string pStr = "Path to " + std::to_string(clicked.key) + ": ";
                            if (p) {
                                for (int i = 0; i < pLen; ++i) pStr += std::to_string(p[i]) + (i + 1 < pLen ? "->" : "");
                                delete[] p;
                            }
                            act1Hl1 = clicked.key;
                            status = pStr;
                        } else if (demoCam.checkDoubleClick(mc->position)) {
                            demoCam.reset();
                            toastQueue.push("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            demoCam.panning = true;
                            demoCam.lastPanPixel = mc->position;
                        }
                    }
                    if (Ui::inside(mx, my, 54.f,   620.f, 200.f, 44.f)) triggerAct1Action(1);
                    if (Ui::inside(mx, my, 274.f,  620.f, 200.f, 44.f)) triggerAct1Action(2);
                    if (Ui::inside(mx, my, 494.f,  620.f, 210.f, 44.f)) triggerAct1Action(3);
                    if (Ui::inside(mx, my, 724.f,  620.f, 210.f, 44.f)) triggerAct1Action(4);
                    if (Ui::inside(mx, my, 954.f,  620.f, 190.f, 44.f)) triggerAct1Action(5);
                    if (Ui::inside(mx, my, 1164.f, 620.f, 180.f, 44.f)) triggerAct1Action(6);
                    if (Ui::inside(mx, my, 1364.f, 620.f, 170.f, 44.f)) triggerAct1Action(7);
                }
                else if (currentScreen == SCREEN_ACT2_DEMO) {
                    if (Ui::inside(mx, my, 1350.f, 26.f, 210.f, 38.f)) { currentScreen = SCREEN_CAMPAIGN_MAP; continue; }
                    if (handlePlaybackControlsClick(mx, my, LOG_X, 110.f, LOG_W, 470.f)) continue;
                    if (demoCam.containsPixel(mx, my)) {
                        const LayoutResult& activeDemoL = animCtrl.hasEvents() ? animCtrl.getRenderLayout() : bstLayout;
                        sf::Vector2f wClick = window.mapPixelToCoords(mc->position, demoCam.view);
                        int hitIdx = activeDemoL.hitTestNode(wClick.x, wClick.y);
                        if (hitIdx >= 0) {
                            const NodeLayout& clicked = activeDemoL.nodes[hitIdx];
                            selectedNodeId = clicked.id;
                            LayoutResult before;
                            LayoutEngine::layoutBST(act2Bst, 30.f, 110.f, 1140.f, 470.f, before);
                            EventBatch batch;
                            act2Bst.search(clicked.key, batch);
                            layoutBstForScreen(true);
                            layoutDirty = true;
                            animCtrl.loadOperation(batch, before, bstLayout, true, autoPlayEvents);
                            int pd = act2Bst.predecessor(clicked.key);
                            int sc = act2Bst.successor(clicked.key);
                            act2Hl1 = clicked.key;
                            status = "Node " + std::to_string(clicked.key) + " | Pred=" +
                                     (pd != -1 ? std::to_string(pd) : "None") + " | Succ=" +
                                     (sc != -1 ? std::to_string(sc) : "None");
                        } else if (demoCam.checkDoubleClick(mc->position)) {
                            demoCam.reset();
                            toastQueue.push("Canvas camera reset", TOAST_KIND_SUCCESS);
                        } else {
                            demoCam.panning = true;
                            demoCam.lastPanPixel = mc->position;
                        }
                    }
                    if (Ui::inside(mx, my, 54.f,   620.f, 155.f, 44.f)) triggerAct2Action(1);
                    if (Ui::inside(mx, my, 222.f,  620.f, 155.f, 44.f)) triggerAct2Action(2);
                    if (Ui::inside(mx, my, 390.f,  620.f, 155.f, 44.f)) triggerAct2Action(3);
                    if (Ui::inside(mx, my, 558.f,  620.f, 155.f, 44.f)) triggerAct2Action(4);
                    if (Ui::inside(mx, my, 726.f,  620.f, 155.f, 44.f)) triggerAct2Action(5);
                    if (Ui::inside(mx, my, 894.f,  620.f, 155.f, 44.f)) triggerAct2Action(6);
                    if (Ui::inside(mx, my, 1062.f, 620.f, 145.f, 44.f)) triggerAct2Action(7);
                    if (Ui::inside(mx, my, 1220.f, 620.f, 145.f, 44.f)) triggerAct2Action(8);
                    if (Ui::inside(mx, my, 1378.f, 620.f, 156.f, 44.f)) triggerAct2Action(9);
                }
            }
        }

        renderFrame(in);
    }

    return 0;
}
