// =============================================================================
// Vault Heist: The Living Vault — Main Game Source (1600x900)
// =============================================================================

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
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
    tree.clear();
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
    outTree.clear();
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
#include "run/run_setup.inc"
#include "run/run_handlers.inc"
#include "run/run_render.inc"
#include "run/run_verify.inc"
#include "run/run_loop.inc"
}
