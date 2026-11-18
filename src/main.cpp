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
#include <optional>
#include <iostream>
#include <string>
#include <cstring>
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
static const int SCREEN_SPLASH        = 0;
static const int SCREEN_MAIN_MENU     = 1;
static const int SCREEN_CAMPAIGN_MAP  = 2;
static const int SCREEN_LEVEL_PLAY    = 3;
static const int SCREEN_RESULT        = 4;
static const int SCREEN_SANDBOX       = 5;
static const int SCREEN_ACT1_DEMO     = 6;
static const int SCREEN_ACT2_DEMO     = 7;
static const int SCREEN_LEADERBOARD   = 8;
static const int SCREEN_SETTINGS      = 9;

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

// Structs for Custom / Manual Tree Puzzles (Levels 3.1 - 3.3)
struct ManualNode {
    int key;
    int height;
    int size;
    ManualNode* left;
    ManualNode* right;

    ManualNode(int k, int h = 1, int s = 1)
        : key(k), height(h), size(s), left(nullptr), right(nullptr) {}
};

static void deleteManualTree(ManualNode* node) {
    if (!node) return;
    deleteManualTree(node->left);
    deleteManualTree(node->right);
    delete node;
}

static void updateManualMetadata(ManualNode* node) {
    if (!node) return;
    updateManualMetadata(node->left);
    updateManualMetadata(node->right);
    int lh = node->left ? node->left->height : 0;
    int rh = node->right ? node->right->height : 0;
    node->height = 1 + maxInt(lh, rh);
    int ls = node->left ? node->left->size : 0;
    int rs = node->right ? node->right->size : 0;
    node->size = 1 + ls + rs;
}

static ManualNode* manualRotateRight(ManualNode* y) {
    if (!y || !y->left) return y;
    ManualNode* x = y->left;
    ManualNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    updateManualMetadata(y);
    updateManualMetadata(x);
    return x;
}

static ManualNode* manualRotateLeft(ManualNode* x) {
    if (!x || !x->right) return x;
    ManualNode* y = x->right;
    ManualNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    updateManualMetadata(x);
    updateManualMetadata(y);
    return y;
}

static int countManualTree(const ManualNode* node, int depth, int& maxDepth) {
    if (!node) return 0;
    if (depth > maxDepth) maxDepth = depth;
    return 1 + countManualTree(node->left, depth + 1, maxDepth)
             + countManualTree(node->right, depth + 1, maxDepth);
}

static void assignManualLayout(const ManualNode* node, int parentId, int depth,
                               int& inorderIdx, int totalNodes, int maxDepth,
                               float areaX, float areaY, float areaW, float areaH,
                               LayoutResult& out) {
    if (!node) return;
    assignManualLayout(node->left, node->key, depth + 1, inorderIdx, totalNodes, maxDepth,
                       areaX, areaY, areaW, areaH, out);
    int slot = inorderIdx++;
    assignManualLayout(node->right, node->key, depth + 1, inorderIdx, totalNodes, maxDepth,
                       areaX, areaY, areaW, areaH, out);

    if (out.nodeCount >= MAX_LAYOUT_NODES) return;

    float radius = (totalNodes > 18) ? 18.f : 23.f;
    float marginX = 52.f;
    float usableW = areaW - 2.f * marginX;
    if (usableW < 80.f) usableW = 80.f;

    float x = areaX + areaW * 0.5f;
    if (totalNodes > 1) {
        float stepX = usableW / static_cast<float>(totalNodes - 1);
        float maxStepX = 110.f;
        if (stepX > maxStepX) stepX = maxStepX;
        float spanW = stepX * static_cast<float>(totalNodes - 1);
        float startX = areaX + (areaW - spanW) * 0.5f;
        x = startX + static_cast<float>(slot) * stepX;
    }

    float marginTop = 38.f;
    float marginBot = 42.f;
    float usableH = areaH - marginTop - marginBot;
    if (usableH < 60.f) usableH = 60.f;
    float stepY = 76.f;
    if (maxDepth > 0) {
        stepY = minFloat(76.f, usableH / static_cast<float>(maxDepth));
        if (stepY < 38.f) stepY = 38.f;
    }
    float y = areaY + marginTop + static_cast<float>(depth) * stepY;

    float pad = radius + 12.f;
    x = clampFloat(x, areaX + pad, areaX + areaW - pad);
    y = clampFloat(y, areaY + pad, areaY + areaH - pad);

    int lh = node->left ? node->left->height : 0;
    int rh = node->right ? node->right->height : 0;
    int bf = lh - rh;

    NodeLayout nl;
    nl.id = node->key;
    nl.key = node->key;
    nl.x = x;
    nl.y = y;
    nl.radius = radius;
    nl.height = node->height;
    nl.bf = bf;
    nl.size = node->size;
    nl.depth = depth;
    nl.parentId = parentId;
    nl.isViolation = (bf < -1 || bf > 1);
    nl.isRoot = (parentId == -1);
    out.nodes[out.nodeCount++] = nl;
}

static void layoutManualTree(const ManualNode* root, float areaX, float areaY, float areaW, float areaH,
                             LayoutResult& out) {
    out.clear();
    out.structureType = STRUCT_AVL;
    if (!root) return;
    int maxDepth = 0;
    int total = countManualTree(root, 0, maxDepth);
    if (total > MAX_LAYOUT_NODES) total = MAX_LAYOUT_NODES;
    int inorderIdx = 0;
    assignManualLayout(root, -1, 0, inorderIdx, total, maxDepth, areaX, areaY, areaW, areaH, out);
    for (int i = 0; i < out.nodeCount; ++i) {
        int pid = out.nodes[i].parentId;
        if (pid >= 0) {
            int pIdx = out.findNodeIndexById(pid);
            if (pIdx >= 0 && out.edgeCount < MAX_LAYOUT_NODES) {
                EdgeLayout el;
                el.parentId = pid;
                el.childId = out.nodes[i].id;
                el.x1 = out.nodes[pIdx].x;
                el.y1 = out.nodes[pIdx].y;
                el.x2 = out.nodes[i].x;
                el.y2 = out.nodes[i].y;
                el.isViolation = out.nodes[i].isViolation;
                out.edges[out.edgeCount++] = el;
            }
        }
    }
}

static int findLevelNodeIndex(const LevelData& d, int key) {
    for (int i = 0; i < d.nodeCount; ++i) {
        if (d.nodes[i].key == key) return i;
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

static ManualNode* buildManualNodeRec(const LevelData& d, int key) {
    int idx = findLevelNodeIndex(d, key);
    if (idx < 0) return nullptr;
    ManualNode* node = new ManualNode(d.nodes[idx].key);
    if (d.nodes[idx].left != -1) node->left = buildManualNodeRec(d, d.nodes[idx].left);
    if (d.nodes[idx].right != -1) node->right = buildManualNodeRec(d, d.nodes[idx].right);
    return node;
}

static ManualNode* buildManualTreeFromData(const LevelData& d) {
    if (d.nodeCount <= 0) return nullptr;
    ManualNode* root = buildManualNodeRec(d, d.nodes[0].key);
    updateManualMetadata(root);
    return root;
}

static ManualNode* buildManualPuzzleTree(const LevelPuzzleSpec& p) {
    ManualNode* root = new ManualNode(p.rootKey);
    ManualNode* child = new ManualNode(p.childKey);
    ManualNode* inner = new ManualNode(p.innerKey);
    if (p.caseName == "LL") {
        root->left = child;
        child->left = inner;
    } else if (p.caseName == "RR") {
        root->right = child;
        child->right = inner;
    } else if (p.caseName == "LR") {
        root->left = child;
        child->right = inner;
    } else if (p.caseName == "RL") {
        root->right = child;
        child->left = inner;
    }
    updateManualMetadata(root);
    return root;
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

// =============================================================================
// MAIN FUNCTION
// =============================================================================
int main(int argc, char** argv) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::string verifyDir;
    bool profileFps = false;
    bool verifyAct1 = false;
    bool verifyAct2 = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verify-screenshots") == 0 && i + 1 < argc) {
            verifyDir = argv[++i];
        } else if (std::strcmp(argv[i], "--profile-fps") == 0) {
            profileFps = true;
        } else if (std::strcmp(argv[i], "--verify-act1") == 0) {
            verifyAct1 = true;
        } else if (std::strcmp(argv[i], "--verify-act2") == 0) {
            verifyAct2 = true;
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
                      font.openFromFile("C:/Windows/Fonts/arial.ttf");
    if (!fontLoaded) std::cerr << "Warning: Arial Font could not be loaded!\n";

    // Persistent score log for Leaderboard (Item 3)
    ScoreLog scoreLog;
    scoreLog.load("scores.txt");

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
    int currentLevelIdx = -1;
    std::string status = "Welcome to Vault Heist!";
    ScoringEngine scoring;
    HintSystem hintSys;
    DragDropController dragDrop;
    int alarmSteps = 0, score = SCORE_BASE;
    float splashTimer = 0.0f;
    float dialAngle = 0.0f;
    sf::Clock deltaClock;

    // Act 3 State
    ManualNode* manualTree = nullptr;
    int manualSubPuzzle = 1;
    int manualDoubleStep = 0;
    int clicked31Mask = 0;

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
    LayoutResult bstLayout, btLayout, avlLayout, manualLayout, heapLayout;
    Renderer guiRenderer;
    AnimationController animCtrl;
    UndoStack undoStack;
    ToastQueue toastQueue;
    UiCache uiCache;
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

    int cachedHudLid = -1;
    std::string cachedHudObj;
    std::string cachedHudCtrlLine;
    int cachedHudAlarm = -1, cachedHudCap = -1, cachedHudScore = -1;
    std::string cachedHudAlarmScore;
    int cachedPromptLid = -1, cachedPromptSeq = -1, cachedPromptAvl = -1;
    int cachedPromptMode = -1, cachedPromptPost = -1, cachedPromptLca = -1;
    int cachedPromptAct2A = -9999, cachedPromptAct2B = -9999;
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

    CanvasCamera levelCam, demoCam, sbAvlCam, sbHeapCam;
    levelCam.init(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, static_cast<float>(W), static_cast<float>(H));
    demoCam.init(30.f, 110.f, 1140.f, 470.f, static_cast<float>(W), static_cast<float>(H));
    sbAvlCam.init(30.f, 95.f, 560.f, 540.f, static_cast<float>(W), static_cast<float>(H));
    sbHeapCam.init(605.f, 95.f, 565.f, 540.f, static_cast<float>(W), static_cast<float>(H));

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
        if (lid == LEVEL_3_3) return (manualSubPuzzle - 1) * 2 + manualDoubleStep;
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
        deleteManualTree(manualTree);
        manualTree = nullptr;

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
            manualTree = buildManualTreeFromData(d);
            status = d.startStatus;
        } else if (lid == LEVEL_3_2 || lid == LEVEL_3_3) {
            if (d.puzzleCount > 0) manualTree = buildManualPuzzleTree(d.puzzles[0]);
            status = d.startStatus;
        } else if (lid == LEVEL_3_4) {
            avl = AVLTree();
            for (int i = 0; i < d.initialKeyCount; ++i) avl.insert(d.initialKeys[i]);
            avlQueuePos = 0;
            if (d.insertCount > 0) {
                status = "Incoming key " + std::to_string(d.inserts[0].key) + "! " + d.startStatus;
            } else {
                status = d.startStatus;
            }
        } else if (lid == LEVEL_3_5) {
            avl = AVLTree();
            for (int i = 0; i < d.initialKeyCount; ++i) avl.insert(d.initialKeys[i]);
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
            status = d.startStatus;
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
        snap.bossQueryIdx = bossQueryIdx;
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
        else if (lid == LEVEL_3_4) finishLevelWin(stars, "AVL Tree auto-balanced every incoming key!");
        else if (lid == LEVEL_4_1) finishLevelWin(stars, "Bottom-up Max Heap activated in O(n)!");
        else if (lid == LEVEL_4_2) finishLevelWin(stars, "Sabotaged root sifted down; Max Heap restored!");
        else if (lid == LEVEL_4_3) finishLevelWin(stars, "All treasures extracted to sorted region!");
        else if (lid == LEVEL_4_4) finishLevelWin(stars, "Race won before guards arrived!");
        else if (lid == LEVEL_4_5) finishLevelWin(stars, "Final Vault unlocked! Heap Sort complete!");
        else finishLevelWin(stars, "Level Complete!");
    };

    auto stepAnimForward = [&]() {
        if (animCtrl.stepForward()) {
            Event cur = animCtrl.getCurrentEvent();
            status = "Step: " + AnimationController::formatEvent(cur);
        } else if (act2TwoChildPhase2Pending && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            startAct2TwoChildPhase2();
        } else if (pendingWin && currentScreen == SCREEN_LEVEL_PLAY && currentLevelIdx >= 0) {
            completePendingWin();
        } else {
            status = "Event batch complete!";
        }
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
                const LevelPuzzleSpec& p = d.puzzles[pIdx];
                if (clickedKey == p.rootKey) {
                    manualTree = (p.caseName == "LL") ? manualRotateRight(manualTree) : manualRotateLeft(manualTree);
                    scoring.addStep(1);
                    syncScoringHud();
                    if (scoring.isAlarmFull()) {
                        triggerAlarmFullFail();
                    } else if (manualSubPuzzle < d.puzzleCount) {
                        deleteManualTree(manualTree);
                        manualTree = buildManualPuzzleTree(d.puzzles[manualSubPuzzle]);
                        manualSubPuzzle++;
                        selectedNodeId = -1;
                        const LevelPuzzleSpec& nextP = d.puzzles[manualSubPuzzle - 1];
                        toastQueue.push(p.caseName + " rotation on " + std::to_string(p.rootKey) + " succeeded!", TOAST_KIND_SUCCESS);
                        status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(d.puzzleCount) +
                                 " (" + nextP.caseName + "): Click unbalanced pivot node " + std::to_string(nextP.rootKey) + "!";
                    } else {
                        finishLevelWin(scoring.stars(), "Single rotations (LL & RR) mastered by clicking pivots!");
                    }
                } else {
                    raisePenalty(PENALTY_WRONG_ROTATION,
                                 "Click the unbalanced pivot node " + std::to_string(p.rootKey) +
                                 " for " + p.caseName + " rotation!");
                }
            }
        }
        else if (lid == LEVEL_3_3) {
            int pIdx = manualSubPuzzle - 1;
            if (pIdx >= 0 && pIdx < d.puzzleCount) {
                const LevelPuzzleSpec& p = d.puzzles[pIdx];
                if (manualDoubleStep == 0) {
                    if (clickedKey == p.childKey) {
                        if (p.caseName == "LR") manualTree->left = manualRotateLeft(manualTree->left);
                        else manualTree->right = manualRotateRight(manualTree->right);
                        updateManualMetadata(manualTree);
                        manualDoubleStep = 1;
                        scoring.addStep(1);
                        syncScoringHud();
                        toastQueue.push("Step 1/2: Rotated child " + std::to_string(p.childKey) +
                                        "! Now click root " + std::to_string(p.rootKey), TOAST_KIND_SUCCESS);
                        status = p.caseName + " Step 2/2: Now click root pivot " + std::to_string(p.rootKey) + "!";
                        if (scoring.isAlarmFull()) triggerAlarmFullFail();
                    } else {
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     p.caseName + " Step 1/2: Click child pivot " + std::to_string(p.childKey) + " first!");
                    }
                } else {
                    if (clickedKey == p.rootKey) {
                        manualTree = (p.caseName == "LR") ? manualRotateRight(manualTree) : manualRotateLeft(manualTree);
                        scoring.addStep(1);
                        syncScoringHud();
                        if (scoring.isAlarmFull()) {
                            triggerAlarmFullFail();
                        } else if (manualSubPuzzle < d.puzzleCount) {
                            deleteManualTree(manualTree);
                            manualTree = buildManualPuzzleTree(d.puzzles[manualSubPuzzle]);
                            manualSubPuzzle++;
                            manualDoubleStep = 0;
                            selectedNodeId = -1;
                            const LevelPuzzleSpec& nextP = d.puzzles[manualSubPuzzle - 1];
                            toastQueue.push(p.caseName + " Double Rotation complete! Next: " + nextP.caseName + " puzzle",
                                            TOAST_KIND_SUCCESS);
                            status = "Puzzle " + std::to_string(manualSubPuzzle) + "/" + std::to_string(d.puzzleCount) +
                                     " (" + nextP.caseName + "): Step 1/2 - Click child pivot " + std::to_string(nextP.childKey) + " first!";
                        } else {
                            finishLevelWin(scoring.stars(), "Double rotations (LR & RL) mastered!");
                        }
                    } else {
                        raisePenalty(PENALTY_WRONG_ROTATION,
                                     p.caseName + " Step 2/2: Click root pivot " + std::to_string(p.rootKey) +
                                     " to complete " + p.caseName + " rotation!");
                    }
                }
            }
        }
        else if (lid == LEVEL_3_4) {
            if (avlQueuePos < d.insertCount) {
                int incoming = d.inserts[avlQueuePos].key;
                int expectedPivot = d.inserts[avlQueuePos].pivotKey;
                const std::string& rotLabel = d.inserts[avlQueuePos].expectedRotation;
                if (clickedKey == expectedPivot) {
                    pushUndoSnapshot(false);
                    LayoutResult before;
                    LayoutEngine::layoutAVL(avl, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, before);
                    EventBatch batch;
                    avl.insert(incoming, batch);
                    layoutAvlForScreen(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, avlLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    scoring.addBonus(50);
                    syncScoringHud();
                    toastQueue.push("Correct (" + rotLabel + " at " + std::to_string(expectedPivot) +
                                    ")! Inserted " + std::to_string(incoming), TOAST_KIND_SUCCESS);
                    avlQueuePos++;
                    if (avlQueuePos >= d.insertCount) {
                        pendingWin = true;
                        status = "All incoming keys balanced! Playing final AVL events...";
                    } else {
                        status = "Inserted " + std::to_string(incoming) + ". Next incoming: " +
                                 std::to_string(d.inserts[avlQueuePos].key) + "! Click its rotation pivot (or attachment parent).";
                    }
                    if (scoring.isAlarmFull()) triggerAlarmFullFail();
                } else {
                    raisePenalty(PENALTY_WRONG_ROTATION,
                                 "Wrong node " + std::to_string(clickedKey) + " for incoming " +
                                 std::to_string(incoming) + " (expected " + rotLabel + " at " +
                                 std::to_string(expectedPivot) + ")!");
                }
            }
        }
        else if (lid == LEVEL_3_5) {
            if (bossQueryIdx < d.kQueryCount) {
                int k = d.kQueries[bossQueryIdx];
                int expectedKey = avl.kth(k);
                if (clickedKey == expectedKey) {
                    scoring.addStep(1);
                    scoring.addBonus(50);
                    syncScoringHud();
                    bossQueryIdx++;
                    if (scoring.isAlarmFull()) {
                        triggerAlarmFullFail();
                    } else if (bossQueryIdx >= d.kQueryCount) {
                        finishLevelWin(scoring.stars(), "Act 3 Boss defeated! Wing 4 (Max Heap Fortress) unlocked!");
                    } else {
                        toastQueue.push("Correct! k=" + std::to_string(k) + " treasure is " + std::to_string(expectedKey),
                                        TOAST_KIND_SUCCESS);
                        status = "Challenge " + std::to_string(bossQueryIdx + 1) + "/" + std::to_string(d.kQueryCount) +
                                 ": Click the k=" + std::to_string(d.kQueries[bossQueryIdx]) + " smallest treasure using subtree sizes (sz)!";
                    }
                } else {
                    raisePenalty(PENALTY_WRONG_TRAVERSAL,
                                 "Node " + std::to_string(clickedKey) + " is not the k=" + std::to_string(k) + " smallest key!");
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
                    sortedArr[sortedCount++] = maxVal;
                    layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    toastQueue.push("Extracted max " + std::to_string(maxVal) + "!", TOAST_KIND_SUCCESS);
                    if (act4Heap.isEmpty()) {
                        pendingWin = true;
                        status = "Extracted final treasure " + std::to_string(maxVal) + "! Finishing...";
                    } else {
                        status = "Extracted " + std::to_string(maxVal) + "! Click root [0] (" +
                                 std::to_string(act4Heap.getMax()) + ") to extract next.";
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
                    sortedArr[sortedCount++] = maxVal;
                    layoutHeapForScreen(true, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H);
                    animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
                    scoring.addStep(1);
                    syncScoringHud();
                    toastQueue.push("Extracted " + std::to_string(maxVal) + " to sorted region", TOAST_KIND_SUCCESS);
                    if (act4Heap.isEmpty()) {
                        pendingWin = true;
                        status = "All treasures heap-sorted! Unlocking final vault...";
                    } else {
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
            sortedArr[sortedCount++] = mxVal;
            layoutHeapForScreen(true, 605.f, 95.f, 565.f, 540.f);
            animCtrl.loadOperation(batch, before, heapLayout, true, autoPlayEvents);
            status = "Sandbox Heap Extracted max=" + std::to_string(mxVal) + "!";
            toastQueue.push(status, TOAST_KIND_SUCCESS);
        } else if (code == 5) {
            enterSandbox();
            toastQueue.push("Sandbox reset!", TOAST_KIND_WARNING);
        }
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
        if (showBgGrid) Ui::grid(window, static_cast<float>(W), static_cast<float>(H), &uiCache);

        // 0. SPLASH SCREEN
        if (currentScreen == SCREEN_SPLASH) {
            float cx = W * 0.5f, cy = 270.f;

            sf::CircleShape outerRing(72.f);
            outerRing.setOrigin({72.f, 72.f});
            outerRing.setPosition({cx, cy});
            outerRing.setFillColor(sf::Color::Transparent);
            outerRing.setOutlineColor(Ui::cyan());
            outerRing.setOutlineThickness(2.5f);
            window.draw(outerRing);

            sf::CircleShape innerRing(28.f);
            innerRing.setOrigin({28.f, 28.f});
            innerRing.setPosition({cx, cy});
            innerRing.setFillColor(sf::Color::Transparent);
            innerRing.setOutlineColor(Ui::cyan());
            innerRing.setOutlineThickness(2.2f);
            window.draw(innerRing);

            for (int i = 0; i < 8; ++i) {
                float rad = (-dialAngle * 0.8f + i * 45.f) * (3.14159265f / 180.f);
                sf::Vertex sp[] = {
                    sf::Vertex({cx + std::cos(rad) * 28.f, cy + std::sin(rad) * 28.f}, Ui::cyan()),
                    sf::Vertex({cx + std::cos(rad) * 40.f, cy + std::sin(rad) * 40.f}, Ui::cyan())
                };
                window.draw(sp, 2, sf::PrimitiveType::Lines);
            }
            for (int i = 0; i < 10; ++i) {
                float rad = (dialAngle + i * 36.f) * (3.14159265f / 180.f);
                sf::CircleShape dot(4.5f);
                dot.setOrigin({4.5f, 4.5f});
                dot.setPosition({cx + std::cos(rad) * 56.f, cy + std::sin(rad) * 56.f});
                dot.setFillColor(Ui::gold());
                window.draw(dot);
            }

            Ui::textCenter(window, font, "VAULT HEIST", 54, Ui::gold(), cx, 380.f, true, &uiCache);
            std::string targetSub = "The Living Vault";
            int charsToShow = minInt(16, static_cast<int>(splashTimer * 6.5f));
            std::string typed = targetSub.substr(0, charsToShow);
            if (charsToShow < 16 && (static_cast<int>(splashTimer * 4.0f) % 2 == 0)) typed += "_";
            Ui::textCenter(window, font, typed, 22, Ui::textBody(), cx, 452.f, false, &uiCache);

            sf::RectangleShape pTrack({340.f, 7.f});
            pTrack.setPosition({cx - 170.f, 515.f});
            pTrack.setFillColor(sf::Color(22, 35, 55));
            window.draw(pTrack);

            float prog = minFloat(1.0f, splashTimer / 3.2f);
            sf::RectangleShape pFill({340.f * prog, 7.f});
            pFill.setPosition({cx - 170.f, 515.f});
            pFill.setFillColor(Ui::cyan());
            window.draw(pFill);

            Ui::textCenter(window, font, "Cracking security protocols...", TXT_HINT, Ui::textMuted(), cx, 540.f, false, &uiCache);
            Ui::textCenter(window, font, "[Press any key or click to skip]", TXT_HINT, Ui::textMuted(), cx, 780.f, false, &uiCache);
        }

        // 1. MAIN MENU
        if (currentScreen == SCREEN_MAIN_MENU) {
            drawTreeWatermark(window, 240.f, 450.f);
            drawTreeWatermark(window, 1360.f, 450.f);

            Ui::textCenter(window, font, "SECURE OPERATIONAL INTERFACE // V 3.0", TXT_HINT, Ui::cyan(), W * 0.5f, 32.f, true, &uiCache);
            Ui::button(window, font, soundEnabled ? "SOUND: ON" : "SOUND: MUTED",
                       W - 180.f, 24.f, 140.f, 36.f, BTN_GHOST, in, 13, &uiCache);

            Ui::textCenter(window, font, "VAULT HEIST", 58, Ui::gold(), W * 0.5f, 110.f, true, &uiCache);
            Ui::textCenter(window, font, "THE LIVING VAULT", 22, Ui::textBody(), W * 0.5f, 184.f, true, &uiCache);

            sf::RectangleShape& uLine = uiCache.rect;
            uLine.setSize({280.f, 3.f});
            uLine.setOrigin({0.f, 0.f});
            uLine.setPosition({W * 0.5f - 140.f, 222.f});
            uLine.setOutlineThickness(0.f);
            uLine.setFillColor(Ui::cyan());
            window.draw(uLine);

            float btnW = 440.f, btnH = 54.f, btnX = W * 0.5f - btnW * 0.5f, startY = 270.f, gap = 68.f;
            Ui::button(window, font, "CAMPAIGN // HEIST WINGS", btnX, startY + 0 * gap, btnW, btnH, BTN_PRIMARY, in, 17, &uiCache);
            Ui::button(window, font, "SANDBOX SIMULATOR",        btnX, startY + 1 * gap, btnW, btnH, BTN_SECONDARY, in, 17, &uiCache);
            Ui::button(window, font, "OPERATOR LEADERBOARD",     btnX, startY + 2 * gap, btnW, btnH, BTN_SECONDARY, in, 17, &uiCache);
            Ui::button(window, font, "SYSTEM SETTINGS",          btnX, startY + 3 * gap, btnW, btnH, BTN_GHOST, in, 17, &uiCache);
            Ui::button(window, font, "QUIT TERMINAL",            btnX, startY + 4 * gap, btnW, btnH, BTN_DANGER, in, 17, &uiCache);

            Ui::textCenter(window, font, status, TXT_BODY, Ui::amber(), W * 0.5f, 650.f, false, &uiCache);
            Ui::textCenter(window, font, "[Click buttons or press 1-5]  |  [Esc] Quit", TXT_HINT, Ui::textMuted(), W * 0.5f, 700.f, false, &uiCache);
        }

        // 2. CAMPAIGN MAP
        if (currentScreen == SCREEN_CAMPAIGN_MAP) {
            Ui::screenHeader(window, font, "HEIST CAMPAIGN // 4 OPERATIONAL WINGS",
                             "Complete levels sequentially to decrypt subsequent vault wings.", &uiCache);

            Ui::button(window, font, "Act 1 Explorer",   1000.f, 26.f, 170.f, 38.f, BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Act 2 Explorer",   1185.f, 26.f, 170.f, 38.f, BTN_SECONDARY, in, 13, &uiCache);
            Ui::button(window, font, "Back to Menu [Esc]", 1370.f, 26.f, 190.f, 38.f, BTN_GHOST, in, 13, &uiCache);

            const char* wingTitles[4] = {
                "WING 1: BLUEPRINT WING (ACT 1 - BINARY TREE)",
                "WING 2: SORTED WING (ACT 2 - BST)",
                "WING 3: ARCHITECT'S WING (ACT 3 - AVL TREE)",
                "WING 4: HEAP FORTRESS (ACT 4 - MAX HEAP)"
            };
            sf::Color wingCols[4] = {
                Ui::cyan(), sf::Color(168, 85, 247), sf::Color(80, 160, 240), Ui::gold()
            };

            for (int wIdx = 0; wIdx < 4; ++wIdx) {
                int col = wIdx % 2, row = wIdx / 2;
                float cx = (col == 0) ? 40.f : 820.f;
                float cy = (row == 0) ? 115.f : 500.f;
                float cw = 740.f, ch = 360.f;

                Ui::panel(window, cx, cy, cw, ch, wingCols[wIdx], 230, &uiCache);
                Ui::textFit(window, font, wingTitles[wIdx], TXT_H2, wingCols[wIdx], cx + 20.f, cy + 14.f, cw - 40.f, true, &uiCache);

                for (int i = 0; i < 5; ++i) {
                    int li = wIdx * 5 + i;
                    float bx = cx + 20.f;
                    float by = cy + 54.f + static_cast<float>(i) * 58.f;
                    bool unl = levels[li].unlocked;
                    if (campCachedUnl[li] != unl || campCachedStars[li] != levels[li].stars || campCachedLabels[li].empty()) {
                        campCachedUnl[li] = unl;
                        campCachedStars[li] = levels[li].stars;
                        campCachedLabels[li] = unl ? levels[li].title : (levels[li].title + "  [LOCKED]");
                    }
                    int st = unl ? (wIdx == 3 ? BTN_PRIMARY : BTN_SECONDARY) : BTN_DISABLED;
                    Ui::button(window, font, campCachedLabels[li], bx, by, cw - 40.f, 48.f, st, in, 15, &uiCache);
                    if (unl && levels[li].stars > 0) {
                        Ui::stars(window, bx + cw - 110.f, by + 24.f, levels[li].stars, 3, 7.f, &uiCache);
                    }
                }
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
            Ui::textFit(window, font, levels[currentLevelIdx].title, 24, Ui::gold(), 30.f, 12.f, 675.f, true, &uiCache);
            Ui::textFit(window, font, cachedHudObj, 14, Ui::textBody(), 30.f, 46.f, 675.f, false, &uiCache);

            Ui::button(window, font, hintSys.isAvailable() ? "Hint [H]" : "Hint Used", 722.f, 18.f, 115.f, 34.f,
                       hintSys.isAvailable() ? BTN_SECONDARY : BTN_DISABLED, in, 13, &uiCache);
            Ui::button(window, font, "Undo [Z]", 850.f, 18.f, 115.f, 34.f,
                       undoStack.isEmpty() ? BTN_DISABLED : BTN_GHOST, in, 13, &uiCache);
            Ui::button(window, font, "Map [Esc]", 978.f, 18.f, 115.f, 34.f, BTN_GHOST, in, 13, &uiCache);

            float meterX = 1120.f, meterW = 450.f;
            sf::RectangleShape& meterRect = uiCache.rect;
            meterRect.setSize({meterW, 20.f});
            meterRect.setOrigin({0.f, 0.f});
            meterRect.setPosition({meterX, 16.f});
            meterRect.setFillColor(sf::Color(35, 42, 58));
            meterRect.setOutlineThickness(1.5f);
            meterRect.setOutlineColor(sf::Color(100, 115, 140));
            window.draw(meterRect);

            float ratio = minFloat(1.0f, static_cast<float>(alarmSteps) / static_cast<float>(alarmCap));
            meterRect.setSize({meterW * ratio, 20.f});
            meterRect.setPosition({meterX, 16.f});
            meterRect.setOutlineThickness(0.f);
            meterRect.setFillColor(ratio > 0.8f ? Ui::red() : Ui::amber());
            window.draw(meterRect);

            Ui::textFit(window, font, cachedHudAlarmScore, 14, sf::Color::White, meterX, 44.f, meterW, true, &uiCache);

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
            else if (lid >= LEVEL_3_1 && lid <= LEVEL_3_3) {
                if (needLayout) {
                    layoutManualTree(manualTree, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, manualLayout);
                    layoutDirty = false;
                }
                activeDrawLayout = &manualLayout;
                if (inCanvasHover) {
                    int hovIdx = manualLayout.hitTestNode(canvasHoverWorld.x, canvasHoverWorld.y);
                    if (hovIdx >= 0) hl.hoveredNodeId = manualLayout.nodes[hovIdx].id;
                }
                showBf = true;
            }
            else if (lid == LEVEL_3_4 || lid == LEVEL_3_5) {
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
            if (lid == LEVEL_2_1 || lid == LEVEL_2_2 || lid == LEVEL_2_5) {
                act2PromptA = act2CurKey;
                act2PromptB = act2TargetIdx * 10 + act2BossStage;
            } else if (lid == LEVEL_2_3) {
                act2PromptA = act2DelStage;
                act2PromptB = act2Bst.getSize();
            } else if (lid == LEVEL_2_4) {
                act2PromptA = act2RqIdx;
                act2PromptB = act2PrunedCount;
            }

            if (cachedPromptLid != lid || cachedPromptSeq != act1SeqIdx || cachedPromptAvl != avlQueuePos ||
                cachedPromptMode != act1TravMode || cachedPromptPost != act1PostIdx || cachedPromptLca != act1LcaStage ||
                cachedPromptAct2A != act2PromptA || cachedPromptAct2B != act2PromptB) {
                cachedPromptLid = lid;
                cachedPromptSeq = act1SeqIdx;
                cachedPromptAvl = avlQueuePos;
                cachedPromptMode = act1TravMode;
                cachedPromptPost = act1PostIdx;
                cachedPromptLca = act1LcaStage;
                cachedPromptAct2A = act2PromptA;
                cachedPromptAct2B = act2PromptB;
                cachedPromptHead = "NODE INTERACTION GUIDE";
                const LevelData& curD = levelData[currentLevelIdx];
                if (lid == LEVEL_3_4 && avlQueuePos < curD.insertCount) {
                    cachedPromptHead = "INCOMING KEY [" + std::to_string(curD.inserts[avlQueuePos].key) +
                                       "] - Click the predicted pivot/attachment node on the AVL tree:";
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
                }
            }
            Ui::textFit(window, font, cachedPromptHead, TXT_H2, Ui::gold(), 54.f, 678.f, 1080.f, true, &uiCache);
            Ui::textWrap(window, font, status, TXT_BODY, Ui::textBody(), 54.f, 718.f, 1050.f, false, &uiCache);
            if (hintSys.wasUsed()) {
                Ui::textWrap(window, font, "HINT (-1 star cap): " + hintSys.shownText(),
                             TXT_HINT, Ui::cyan(), 54.f, 774.f, 1050.f, false, &uiCache);
            }

            if (lid == LEVEL_1_1 && act1SeqIdx == 0 && !pendingWin) {
                Ui::button(window, font, "Cycle Mode [C]",
                           1220.f, 705.f, 310.f, 52.f, BTN_SECONDARY, in, 15, &uiCache);
            } else if (lid == LEVEL_1_3) {
                bool askingDiam = (act1PostIdx >= act1Tree.getSize());
                std::string valLabel = (askingDiam ? "Diam: " : "Height: ") + std::to_string(act1StepperVal);
                Ui::button(window, font, "-", 1130.f, 705.f, 60.f, 52.f, BTN_SECONDARY, in, 18, &uiCache);
                Ui::panel(window, 1200.f, 705.f, 150.f, 52.f, Ui::gold(), 240, &uiCache);
                Ui::textCenter(window, font, valLabel, 17, Ui::gold(), 1275.f, 719.f, true, &uiCache);
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
            } else if (lid == LEVEL_4_5 && !act4Built) {
                Ui::button(window, font, "Build Max Heap [B]",
                           1220.f, 705.f, 310.f, 52.f, BTN_PRIMARY, in, 15, &uiCache);
            }

            Ui::textFit(window, font, cachedHudCtrlLine, TXT_HINT, Ui::textMuted(), 54.f, 832.f, 1480.f, false, &uiCache);

            // Mission Briefing Modal (7D — wrapped strictly inside card)
            if (showingBriefing) {
                sf::RectangleShape& bd = uiCache.rect;
                bd.setSize({static_cast<float>(W), static_cast<float>(H)});
                bd.setOrigin({0.f, 0.f});
                bd.setPosition({0.f, 0.f});
                bd.setOutlineThickness(0.f);
                bd.setFillColor(sf::Color(6, 10, 20, 210));
                window.draw(bd);

                if (cachedBriefLid != lid || cachedBriefCap != alarmCap) {
                    cachedBriefLid = lid;
                    cachedBriefCap = alarmCap;
                    cachedBriefAct = "MISSION BRIEFING // ACT " + std::to_string(levels[currentLevelIdx].act);
                    cachedBriefObj = "OBJECTIVE: " + levels[currentLevelIdx].objective;
                    cachedBriefInt = "INTERACTION: " + levels[currentLevelIdx].controls +
                                     "\nALARM CAPACITY: " + std::to_string(alarmCap) + " steps";
                }

                float bw = 760.f, bh = 380.f;
                float bx = W * 0.5f - bw * 0.5f, by = H * 0.5f - bh * 0.5f;
                Ui::panel(window, bx, by, bw, bh, Ui::gold(), 250, &uiCache);

                Ui::text(window, font, cachedBriefAct, TXT_HINT, Ui::cyan(), bx + 36.f, by + 26.f, true, &uiCache);
                Ui::textFit(window, font, levels[currentLevelIdx].title, TXT_H1, Ui::gold(),
                            bx + 36.f, by + 54.f, bw - 72.f, true, &uiCache);

                float objY = by + 110.f;
                float objH = Ui::textWrap(window, font, cachedBriefObj,
                                          TXT_BODY, sf::Color::White, bx + 36.f, objY, bw - 72.f, false, &uiCache);
                float ctrlY = objY + objH + 18.f;
                Ui::textWrap(window, font, cachedBriefInt,
                             TXT_HINT, Ui::textBody(), bx + 36.f, ctrlY, bw - 72.f, false, &uiCache);

                Ui::button(window, font, "START MISSION [Enter]", W * 0.5f - 150.f, by + bh - 74.f,
                           300.f, 48.f, BTN_PRIMARY, in, 16, &uiCache);
            }
        }

        // 4. RESULT SCREEN
        if (currentScreen == SCREEN_RESULT) {
            bool failed = (currentLevelIdx >= 0 && levels[currentLevelIdx].stars == 0);
            float cardW = 760.f, cardH = 520.f;
            float cardX = W * 0.5f - cardW * 0.5f, cardY = 170.f;
            Ui::panel(window, cardX, cardY, cardW, cardH, failed ? Ui::red() : Ui::gold(), 240, &uiCache);

            Ui::textCenter(window, font,
                           failed ? "ALARM TRIGGERED - LEVEL FAILED" : "LEVEL COMPLETE",
                           36, failed ? Ui::red() : Ui::gold(), W * 0.5f, cardY + 34.f, true, &uiCache);

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

            Ui::panel(window, 30.f,  95.f, 560.f, 540.f, Ui::cyan(), 248, &uiCache);
            Ui::panel(window, 605.f, 95.f, 565.f, 540.f, Ui::gold(), 248, &uiCache);

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

            Ui::textFit(window, font, status, TXT_BODY, Ui::gold(), 54.f, 746.f, 1480.f, false, &uiCache);
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

            Ui::panel(window, 30.f, 110.f, 1140.f, 470.f, Ui::cyan(), 248, &uiCache);
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

            Ui::textFit(window, font, status, TXT_BODY, Ui::gold(), 54.f, 690.f, 1480.f, false, &uiCache);
            if (!act1TraversalStr.empty()) {
                Ui::textWrap(window, font, act1TraversalStr, TXT_HINT, Ui::cyan(), 54.f, 728.f, 1480.f, false, &uiCache);
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

            Ui::panel(window, 30.f, 110.f, 1140.f, 470.f, sf::Color(168, 85, 247), 248, &uiCache);
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

            Ui::textFit(window, font, status, TXT_BODY, Ui::gold(), 54.f, 690.f, 1480.f, false, &uiCache);
            if (!act2ExtraInfo.empty()) {
                Ui::textFit(window, font, act2ExtraInfo, TXT_HINT, Ui::cyan(), 54.f, 730.f, 1480.f, false, &uiCache);
            }
            Ui::textFit(window, font, "Click any BST node to search & inspect Pred/Succ  |  [Space] Play/Pause  |  [Right] Step",
                        TXT_HINT, Ui::textMuted(), 54.f, 830.f, 1480.f, false, &uiCache);
        }

        // 8. REAL PERSISTENT LEADERBOARD (Item 3)
        if (currentScreen == SCREEN_LEADERBOARD) {
            Ui::screenHeader(window, font, "OPERATOR LEADERBOARD",
                             "Recorded campaign heist scores loaded from scores.txt", &uiCache);
            float mw = 920.f, mh = 600.f;
            float mx0 = W * 0.5f - mw * 0.5f, my0 = 130.f;
            Ui::panel(window, mx0, my0, mw, mh, Ui::gold(), 240, &uiCache);

            if (scoreLog.getCount() == 0) {
                Ui::textCenter(window, font, "No heists recorded yet", TXT_H1, Ui::textBody(),
                               W * 0.5f, my0 + 200.f, true, &uiCache);
                Ui::textCenter(window, font, "Complete any campaign level to record your score here.",
                               TXT_BODY, Ui::textMuted(), W * 0.5f, my0 + 255.f, false, &uiCache);
            } else {
                Ui::text(window, font, "RANK",  TXT_HINT, Ui::cyan(), mx0 + 36.f,  my0 + 24.f, true, &uiCache);
                Ui::text(window, font, "LEVEL", TXT_HINT, Ui::cyan(), mx0 + 120.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, font, "SCORE", TXT_HINT, Ui::cyan(), mx0 + 520.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, font, "ALARM", TXT_HINT, Ui::cyan(), mx0 + 660.f, my0 + 24.f, true, &uiCache);
                Ui::text(window, font, "STARS", TXT_HINT, Ui::cyan(), mx0 + 785.f, my0 + 24.f, true, &uiCache);

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
                    float ry = my0 + 62.f + static_cast<float>(r) * 44.f;
                    Ui::text(window, font, cachedLbRank[r], TXT_BODY, Ui::gold(), mx0 + 36.f, ry, true, &uiCache);
                    Ui::textFit(window, font, e.levelTitle, TXT_BODY, Ui::textBody(), mx0 + 120.f, ry, 380.f, false, &uiCache);
                    Ui::text(window, font, cachedLbScore[r], TXT_BODY, Ui::green(), mx0 + 520.f, ry, false, &uiCache);
                    Ui::text(window, font, cachedLbAlarm[r], TXT_BODY, Ui::textBody(), mx0 + 675.f, ry, false, &uiCache);
                    Ui::stars(window, mx0 + 820.f, ry + 9.f, e.stars, 3, 7.f, &uiCache);
                }
            }

            Ui::button(window, font, "Return to Main Menu [Esc]", W * 0.5f - 150.f, 770.f, 300.f, 48.f, BTN_PRIMARY, in, 15, &uiCache);
        }

        // 9. REAL WORKING SETTINGS (Item 4)
        if (currentScreen == SCREEN_SETTINGS) {
            Ui::screenHeader(window, font, "SYSTEM SETTINGS",
                             "Every setting below takes effect immediately across the interface.", &uiCache);
            float mw = 760.f, mh = 560.f;
            float sx0 = W * 0.5f - mw * 0.5f, sy0 = 140.f;
            Ui::panel(window, sx0, sy0, mw, mh, Ui::cyan(), 240, &uiCache);

            if (cachedSetFpsVal != targetFps) {
                cachedSetFpsVal = targetFps;
                cachedSetFpsLimStr = "Frame Rate Limit: " + std::to_string(targetFps) + " FPS";
                cachedSetFpsBtnStr = "Frame Rate Cap: " + std::to_string(targetFps) + " FPS (Click to toggle 60/120)";
            }

            Ui::text(window, font, cachedSetResStr, TXT_BODY, Ui::textBody(), sx0 + 50.f, sy0 + 36.f, true, &uiCache);
            Ui::text(window, font, cachedSetFpsLimStr, TXT_BODY, Ui::textBody(), sx0 + 50.f, sy0 + 68.f, true, &uiCache);

            float bw = 560.f, bh = 52.f, bx = W * 0.5f - bw * 0.5f;
            Ui::button(window, font, soundEnabled ? "Audio FX Feedback: ENABLED" : "Audio FX Feedback: MUTED",
                       bx, sy0 + 125.f, bw, bh, soundEnabled ? BTN_SECONDARY : BTN_GHOST, in, 15, &uiCache);
            Ui::button(window, font, showNodeBadges ? "Node Badges (h / BF / sz): VISIBLE" : "Node Badges (h / BF / sz): HIDDEN",
                       bx, sy0 + 195.f, bw, bh, showNodeBadges ? BTN_SECONDARY : BTN_GHOST, in, 15, &uiCache);
            Ui::button(window, font, showBgGrid ? "Blueprint Grid Overlay: VISIBLE" : "Blueprint Grid Overlay: HIDDEN",
                       bx, sy0 + 265.f, bw, bh, showBgGrid ? BTN_SECONDARY : BTN_GHOST, in, 15, &uiCache);
            Ui::button(window, font, autoPlayEvents ? "Operation Animation Mode: AUTO-PLAY" : "Operation Animation Mode: STEP-BY-STEP",
                       bx, sy0 + 335.f, bw, bh, autoPlayEvents ? BTN_SECONDARY : BTN_GHOST, in, 15, &uiCache);
            Ui::button(window, font, cachedSetFpsBtnStr,
                       bx, sy0 + 405.f, bw, bh, BTN_SECONDARY, in, 15, &uiCache);

            Ui::button(window, font, "Return to Main Menu [Esc]", W * 0.5f - 150.f, 740.f, 300.f, 48.f, BTN_PRIMARY, in, 15, &uiCache);
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

        deleteManualTree(manualTree);
        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 1 levels (--verify-act1)
    if (verifyAct1) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            deleteManualTree(manualTree);
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

        deleteManualTree(manualTree);
        return 0;
    }

    // End-to-end automated sanity check for all 5 Act 2 levels (--verify-act2)
    if (verifyAct2) {
        scoreLog.load(""); // do not modify workspace scores.txt
        auto failCheck = [&](const char* msg) -> int {
            std::cerr << "FAIL: " << msg << "\n";
            deleteManualTree(manualTree);
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

        deleteManualTree(manualTree);
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
        deleteManualTree(manualTree);
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
        if (pendingWin && !act2TwoChildPhase2Pending && animCtrl.isAtEnd() && !animCtrl.isPlaying()) {
            completePendingWin();
        }

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
        else if (currentScreen == SCREEN_SANDBOX) { updateCamPan(sbAvlCam); updateCamPan(sbHeapCam); }
        else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) updateCamPan(demoCam);

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
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
                demoCam.panning = false;
                sbAvlCam.panning = false;
                sbHeapCam.panning = false;
            }

            if (const auto* mw = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mw->wheel == sf::Mouse::Wheel::Vertical) {
                    if (currentScreen == SCREEN_LEVEL_PLAY && !showingBriefing) {
                        levelCam.handleZoom(window, mw->position, mw->delta);
                    } else if (currentScreen == SCREEN_SANDBOX) {
                        if (!sbAvlCam.handleZoom(window, mw->position, mw->delta)) {
                            sbHeapCam.handleZoom(window, mw->position, mw->delta);
                        }
                    } else if (currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO) {
                        demoCam.handleZoom(window, mw->position, mw->delta);
                    }
                }
            }

            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (currentScreen == SCREEN_SPLASH) {
                    currentScreen = SCREEN_MAIN_MENU;
                    continue;
                }
                if (kp->code == sf::Keyboard::Key::Escape) {
                    if (currentScreen == SCREEN_LEVEL_PLAY || currentScreen == SCREEN_RESULT) {
                        dragDrop.cancel();
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Returned to Campaign Map";
                    } else if (currentScreen == SCREEN_CAMPAIGN_MAP || currentScreen == SCREEN_SANDBOX ||
                               currentScreen == SCREEN_ACT1_DEMO || currentScreen == SCREEN_ACT2_DEMO ||
                               currentScreen == SCREEN_LEADERBOARD || currentScreen == SCREEN_SETTINGS) {
                        currentScreen = SCREEN_MAIN_MENU;
                        status = "Main Menu";
                    } else {
                        window.close();
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
                    if (kp->code == sf::Keyboard::Key::Num1) { currentScreen = SCREEN_CAMPAIGN_MAP; status = "Heist Wings Overview"; }
                    if (kp->code == sf::Keyboard::Key::Num2) enterSandbox();
                    if (kp->code == sf::Keyboard::Key::Num3) currentScreen = SCREEN_LEADERBOARD;
                    if (kp->code == sf::Keyboard::Key::Num4) currentScreen = SCREEN_SETTINGS;
                    if (kp->code == sf::Keyboard::Key::Num5) window.close();
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
                    if (Ui::inside(mx, my, W - 180.f, 24.f, 140.f, 36.f)) soundEnabled = !soundEnabled;
                    float btnW = 440.f, btnH = 54.f, btnX = W * 0.5f - btnW * 0.5f, startY = 270.f, gap = 68.f;
                    if (Ui::inside(mx, my, btnX, startY + 0 * gap, btnW, btnH)) {
                        currentScreen = SCREEN_CAMPAIGN_MAP;
                        status = "Heist Wings Overview";
                    } else if (Ui::inside(mx, my, btnX, startY + 1 * gap, btnW, btnH)) {
                        enterSandbox();
                    } else if (Ui::inside(mx, my, btnX, startY + 2 * gap, btnW, btnH)) {
                        currentScreen = SCREEN_LEADERBOARD;
                    } else if (Ui::inside(mx, my, btnX, startY + 3 * gap, btnW, btnH)) {
                        currentScreen = SCREEN_SETTINGS;
                    } else if (Ui::inside(mx, my, btnX, startY + 4 * gap, btnW, btnH)) {
                        window.close();
                    }
                }
                else if (currentScreen == SCREEN_CAMPAIGN_MAP) {
                    if (Ui::inside(mx, my, 1000.f, 26.f, 170.f, 38.f)) { currentScreen = SCREEN_ACT1_DEMO; demoCam.reset(); animCtrl.clearCurrent(); layoutDirty = true; }
                    if (Ui::inside(mx, my, 1185.f, 26.f, 170.f, 38.f)) { currentScreen = SCREEN_ACT2_DEMO; demoCam.reset(); animCtrl.clearCurrent(); layoutDirty = true; }
                    if (Ui::inside(mx, my, 1370.f, 26.f, 190.f, 38.f)) { currentScreen = SCREEN_MAIN_MENU; status = "Main Menu"; }

                    for (int wIdx = 0; wIdx < 4; ++wIdx) {
                        int col = wIdx % 2, row = wIdx / 2;
                        float cx = (col == 0) ? 40.f : 820.f;
                        float cy = (row == 0) ? 115.f : 500.f;
                        for (int i = 0; i < 5; ++i) {
                            int li = wIdx * 5 + i;
                            float bx = cx + 20.f;
                            float by = cy + 54.f + static_cast<float>(i) * 58.f;
                            if (Ui::inside(mx, my, bx, by, 700.f, 48.f)) {
                                if (levels[li].unlocked) startLevel(li);
                                else toastQueue.push("Locked! Clear previous level first", TOAST_KIND_WARNING);
                            }
                        }
                    }
                }
                else if (currentScreen == SCREEN_LEADERBOARD) {
                    if (Ui::inside(mx, my, W * 0.5f - 150.f, 770.f, 300.f, 48.f)) {
                        currentScreen = SCREEN_MAIN_MENU;
                    }
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
                        float bh = 380.f, by = H * 0.5f - bh * 0.5f;
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
                    } else if (lid >= LEVEL_3_1 && lid <= LEVEL_3_3) {
                        activeL = &manualLayout;
                    } else if (lid == LEVEL_3_4 || lid == LEVEL_3_5) {
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

    deleteManualTree(manualTree);
    return 0;
}
