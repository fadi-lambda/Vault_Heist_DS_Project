#include "gui/LayoutEngine.h"

namespace VaultHeist {

static inline int maxIntLayout(int a, int b) { return (a > b) ? a : b; }
static inline float minFloatLayout(float a, float b) { return (a < b) ? a : b; }
static inline float maxFloatLayout(float a, float b) { return (a > b) ? a : b; }
static inline float clampFloatLayout(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int LayoutEngine::computeTreeStats(const TreeNode* node, int depth, int& maxDepth) {
    if (!node) return 0;
    if (depth > maxDepth) maxDepth = depth;
    return 1 + computeTreeStats(node->left, depth + 1, maxDepth)
             + computeTreeStats(node->right, depth + 1, maxDepth);
}

void LayoutEngine::assignTreeNodePositions(const TreeNode* node, int parentId, int depth,
                                           int& inorderIdx, int totalNodes, int maxDepth,
                                           float areaX, float areaY, float areaW, float areaH,
                                           LayoutResult& out, int& outNodeHeight, int& outNodeSize) {
    if (!node) {
        outNodeHeight = 0;
        outNodeSize = 0;
        return;
    }

    int leftH = 0, leftSz = 0;
    int rightH = 0, rightSz = 0;

    assignTreeNodePositions(node->left, node->id, depth + 1, inorderIdx, totalNodes, maxDepth,
                            areaX, areaY, areaW, areaH, out, leftH, leftSz);

    int slot = inorderIdx++;

    assignTreeNodePositions(node->right, node->id, depth + 1, inorderIdx, totalNodes, maxDepth,
                            areaX, float(areaY), areaW, areaH, out, rightH, rightSz);

    outNodeHeight = 1 + maxIntLayout(leftH, rightH);
    outNodeSize = 1 + leftSz + rightSz;

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
        float fitStepY = usableH / static_cast<float>(maxDepth);
        stepY = minFloatLayout(76.f, fitStepY);
        if (stepY < 38.f) stepY = 38.f;
    }
    float y = areaY + marginTop + static_cast<float>(depth) * stepY;

    float pad = radius + 12.f;
    x = clampFloatLayout(x, areaX + pad, areaX + areaW - pad);
    y = clampFloatLayout(y, areaY + pad, areaY + areaH - pad);

    NodeLayout nl;
    nl.id = node->id;
    nl.key = node->value;
    nl.x = x;
    nl.y = y;
    nl.radius = radius;
    nl.scale = 1.0f;
    nl.alpha = 1.0f;
    nl.height = outNodeHeight;
    nl.bf = leftH - rightH;
    nl.size = outNodeSize;
    nl.depth = depth;
    nl.parentId = parentId;
    nl.isViolation = false;
    nl.isRoot = (parentId == -1);

    out.nodes[out.nodeCount++] = nl;
}

void LayoutEngine::layoutBST(const BST& bst, float areaX, float areaY, float areaW, float areaH,
                             LayoutResult& out) {
    out.clear();
    out.structureType = STRUCT_BST;

    const TreeNode* root = bst.getRoot();
    if (!root) return;

    int maxDepth = 0;
    int totalNodes = computeTreeStats(root, 0, maxDepth);
    if (totalNodes > MAX_LAYOUT_NODES) totalNodes = MAX_LAYOUT_NODES;

    int inorderIdx = 0;
    int rootH = 0, rootSz = 0;
    assignTreeNodePositions(root, -1, 0, inorderIdx, totalNodes, maxDepth,
                            areaX, areaY, areaW, areaH, out, rootH, rootSz);

    // Build edges by looking up each node's parent in `out.nodes`
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
                el.isViolation = false;
                out.edges[out.edgeCount++] = el;
            }
        }
    }
}

void LayoutEngine::layoutBinaryTree(const BinaryTree& tree, float areaX, float areaY, float areaW, float areaH,
                                    LayoutResult& out) {
    out.clear();
    out.structureType = STRUCT_BINARY_TREE;

    const TreeNode* root = tree.getRoot();
    if (!root) return;

    int maxDepth = 0;
    int totalNodes = computeTreeStats(root, 0, maxDepth);
    if (totalNodes > MAX_LAYOUT_NODES) totalNodes = MAX_LAYOUT_NODES;

    int inorderIdx = 0;
    int rootH = 0, rootSz = 0;
    assignTreeNodePositions(root, -1, 0, inorderIdx, totalNodes, maxDepth,
                            areaX, areaY, areaW, areaH, out, rootH, rootSz);

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
                el.isViolation = false;
                out.edges[out.edgeCount++] = el;
            }
        }
    }
}

int LayoutEngine::computeAVLStats(const AVLNode* node, int depth, int& maxDepth) {
    if (!node) return 0;
    if (depth > maxDepth) maxDepth = depth;
    return 1 + computeAVLStats(node->left, depth + 1, maxDepth)
             + computeAVLStats(node->right, depth + 1, maxDepth);
}

void LayoutEngine::assignAVLNodePositions(const AVLTree& avl, AVLNode* node,
                                          int parentId, int depth, int& inorderIdx,
                                          int totalNodes, int maxDepth,
                                          float areaX, float areaY, float areaW, float areaH,
                                          LayoutResult& out) {
    if (!node) return;

    assignAVLNodePositions(avl, node->left, node->key, depth + 1, inorderIdx, totalNodes, maxDepth,
                           areaX, areaY, areaW, areaH, out);

    int slot = inorderIdx++;

    assignAVLNodePositions(avl, node->right, node->key, depth + 1, inorderIdx, totalNodes, maxDepth,
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
        float fitStepY = usableH / static_cast<float>(maxDepth);
        stepY = minFloatLayout(76.f, fitStepY);
        if (stepY < 38.f) stepY = 38.f;
    }
    float y = areaY + marginTop + static_cast<float>(depth) * stepY;

    float pad = radius + 12.f;
    x = clampFloatLayout(x, areaX + pad, areaX + areaW - pad);
    y = clampFloatLayout(y, areaY + pad, areaY + areaH - pad);

    int bf = avl.balanceFactor(node);
    NodeLayout nl;
    nl.id = node->key; // AVLTree events identify nodes by unique key
    nl.key = node->key;
    nl.x = x;
    nl.y = y;
    nl.radius = radius;
    nl.scale = 1.0f;
    nl.alpha = 1.0f;
    nl.height = node->height;
    nl.bf = bf;
    nl.size = node->size;
    nl.depth = depth;
    nl.parentId = parentId;
    nl.isViolation = (bf < -1 || bf > 1);
    nl.isRoot = (parentId == -1);

    out.nodes[out.nodeCount++] = nl;
}

void LayoutEngine::layoutAVL(const AVLTree& avl, float areaX, float areaY, float areaW, float areaH,
                             LayoutResult& out) {
    out.clear();
    out.structureType = STRUCT_AVL;

    AVLNode* root = avl.getRoot();
    if (!root) return;

    int maxDepth = 0;
    int totalNodes = computeAVLStats(root, 0, maxDepth);
    if (totalNodes > MAX_LAYOUT_NODES) totalNodes = MAX_LAYOUT_NODES;

    int inorderIdx = 0;
    assignAVLNodePositions(avl, root, -1, 0, inorderIdx, totalNodes, maxDepth,
                           areaX, areaY, areaW, areaH, out);

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

void LayoutEngine::layoutMaxHeap(const MaxHeap& heap, const int* sortedArr, int sortedCount,
                                 float areaX, float areaY, float areaW, float areaH,
                                 LayoutResult& out) {
    int buf[MAX_LAYOUT_NODES];
    int n = heap.getSize();
    if (n > MAX_LAYOUT_NODES) n = MAX_LAYOUT_NODES;
    for (int i = 0; i < n; ++i) {
        buf[i] = heap.getElementAt(i);
    }
    layoutHeapArray(buf, n, sortedArr, sortedCount, areaX, areaY, areaW, areaH, out);
}

void LayoutEngine::layoutHeapArray(const int* arr, int activeSize, const int* sortedArr, int sortedCount,
                                   float areaX, float areaY, float areaW, float areaH,
                                   LayoutResult& out) {
    out.clear();
    out.structureType = STRUCT_MAX_HEAP;

    int n = activeSize;
    if (n > MAX_LAYOUT_NODES) n = MAX_LAYOUT_NODES;

    // Reserve bottom 86px for synchronised array view & label
    float treeH = maxFloatLayout(120.f, areaH - 96.f);
    int maxLevel = 0;
    if (n > 1) {
        int tmp = n;
        while (tmp > 1) { tmp >>= 1; maxLevel++; }
    }
    float stepY = 74.f;
    if (maxLevel > 0) {
        stepY = minFloatLayout(74.f, (treeH - 58.f) / static_cast<float>(maxLevel));
        if (stepY < 42.f) stepY = 42.f;
    }
    float radius = (n > 15) ? 18.f : 22.f;
    float pad = radius + 12.f;

    // 1. Tree node positions for active heap indices [0..n-1]
    for (int i = 0; i < n; ++i) {
        int level = 0;
        int tmp = i + 1;
        while (tmp > 1) {
            tmp >>= 1;
            level++;
        }
        int nodesAtLvl = 1 << level;
        int idxAtLvl = i - (nodesAtLvl - 1);

        float x = areaX + (areaW / static_cast<float>(nodesAtLvl + 1)) * static_cast<float>(idxAtLvl + 1);
        float y = areaY + 34.f + static_cast<float>(level) * stepY;
        x = clampFloatLayout(x, areaX + pad, areaX + areaW - pad);
        y = clampFloatLayout(y, areaY + pad, areaY + treeH - pad);

        int pIdx = (i > 0) ? (i - 1) / 2 : -1;
        bool violation = (i > 0 && arr[i] > arr[pIdx]);

        NodeLayout nl;
        nl.id = i; // Heap events address nodes by array index
        nl.key = arr[i];
        nl.x = x;
        nl.y = y;
        nl.radius = radius;
        nl.scale = 1.0f;
        nl.alpha = 1.0f;
        nl.height = 1;
        nl.bf = 0;
        nl.size = 1;
        nl.depth = level;
        nl.parentId = pIdx;
        nl.isViolation = violation;
        nl.isRoot = (i == 0);
        out.nodes[out.nodeCount++] = nl;
    }

    // 2. Tree edges
    for (int i = 1; i < n; ++i) {
        int pIdx = (i - 1) / 2;
        if (out.edgeCount < MAX_LAYOUT_NODES) {
            EdgeLayout el;
            el.parentId = pIdx;
            el.childId = i;
            el.x1 = out.nodes[pIdx].x;
            el.y1 = out.nodes[pIdx].y;
            el.x2 = out.nodes[i].x;
            el.y2 = out.nodes[i].y;
            el.isViolation = (arr[pIdx] < arr[i]);
            out.edges[out.edgeCount++] = el;
        }
    }

    // 3. Synchronised Array View cells (active + sorted), strictly fit within areaW
    int sCount = (sortedArr && sortedCount > 0) ? sortedCount : 0;
    int totalCells = n + sCount;
    if (totalCells > MAX_LAYOUT_NODES) totalCells = MAX_LAYOUT_NODES;
    float cellGap = 6.f;
    float sortedSep = (n > 0 && sCount > 0) ? 18.f : 0.f;
    float availCellW = areaW - 28.f - sortedSep;
    float cellW = 54.f;
    if (totalCells > 0) {
        float fitW = (availCellW - static_cast<float>(totalCells - 1) * cellGap) / static_cast<float>(totalCells);
        cellW = clampFloatLayout(fitW, 28.f, 54.f);
    }
    float cellY = areaY + areaH - 54.f;
    float startX = areaX + 14.f;

    for (int i = 0; i < n && out.cellCount < MAX_LAYOUT_NODES; ++i) {
        ArrayCellLayout c;
        c.index = i;
        c.value = arr[i];
        c.x = startX + static_cast<float>(i) * (cellW + cellGap);
        c.y = cellY;
        c.w = cellW;
        c.h = 40.f;
        c.isSorted = false;
        c.isRoot = (i == 0);
        out.cells[out.cellCount++] = c;
    }

    if (sortedArr && sCount > 0) {
        float sortedStartX = startX + static_cast<float>(n) * (cellW + cellGap) + sortedSep;
        for (int s = 0; s < sCount && out.cellCount < MAX_LAYOUT_NODES; ++s) {
            ArrayCellLayout c;
            c.index = n + s;
            c.value = sortedArr[s];
            c.x = sortedStartX + static_cast<float>(s) * (cellW + cellGap);
            c.y = cellY;
            c.w = cellW;
            c.h = 40.f;
            c.isSorted = true;
            c.isRoot = false;
            out.cells[out.cellCount++] = c;
        }
    }
}

} // namespace VaultHeist
