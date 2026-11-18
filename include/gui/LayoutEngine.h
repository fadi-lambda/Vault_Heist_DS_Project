#pragma once

#include "structures/BST.h"
#include "structures/BinaryTree.h"
#include "structures/AVLTree.h"
#include "structures/MaxHeap.h"

namespace VaultHeist {

static const int MAX_LAYOUT_NODES = 128;

// Structure type constants (no enum allowed)
static const int STRUCT_NONE        = 0;
static const int STRUCT_BINARY_TREE = 1;
static const int STRUCT_BST         = 2;
static const int STRUCT_AVL         = 3;
static const int STRUCT_MAX_HEAP    = 4;

struct NodeLayout {
    int id;
    int key;
    float x;
    float y;
    float radius;
    float scale;
    float alpha;
    int height;
    int bf;
    int size;
    int depth;
    int parentId;
    bool isViolation;
    bool isRoot;

    NodeLayout()
        : id(-1), key(0), x(0.f), y(0.f), radius(21.f), scale(1.0f), alpha(1.0f),
          height(1), bf(0), size(1), depth(0), parentId(-1),
          isViolation(false), isRoot(false) {}
};

struct EdgeLayout {
    int parentId;
    int childId;
    float x1;
    float y1;
    float x2;
    float y2;
    bool isViolation;

    EdgeLayout()
        : parentId(-1), childId(-1), x1(0.f), y1(0.f), x2(0.f), y2(0.f),
          isViolation(false) {}
};

struct ArrayCellLayout {
    int index;
    int value;
    float x;
    float y;
    float w;
    float h;
    bool isSorted;
    bool isRoot;

    ArrayCellLayout()
        : index(-1), value(0), x(0.f), y(0.f), w(50.f), h(40.f),
          isSorted(false), isRoot(false) {}
};

struct LayoutResult {
    int structureType;
    NodeLayout nodes[MAX_LAYOUT_NODES];
    int nodeCount;
    EdgeLayout edges[MAX_LAYOUT_NODES];
    int edgeCount;
    ArrayCellLayout cells[MAX_LAYOUT_NODES];
    int cellCount;

    LayoutResult() : structureType(STRUCT_NONE), nodeCount(0), edgeCount(0), cellCount(0) {}

    void clear() {
        structureType = STRUCT_NONE;
        nodeCount = 0;
        edgeCount = 0;
        cellCount = 0;
    }

    int findNodeIndexById(int id) const {
        if (id < 0) return -1;
        for (int i = 0; i < nodeCount; ++i) {
            if (nodes[i].id == id) return i;
        }
        return -1;
    }

    int findNodeIndexByKey(int key) const {
        for (int i = 0; i < nodeCount; ++i) {
            if (nodes[i].key == key) return i;
        }
        return -1;
    }

    int hitTestNode(float mx, float my) const {
        for (int i = 0; i < nodeCount; ++i) {
            float dx = mx - nodes[i].x;
            float dy = my - nodes[i].y;
            float r = nodes[i].radius * nodes[i].scale + 4.f;
            if (dx * dx + dy * dy <= r * r) {
                return i;
            }
        }
        return -1;
    }

    int hitTestCell(float mx, float my) const {
        for (int i = 0; i < cellCount; ++i) {
            if (!cells[i].isSorted &&
                mx >= cells[i].x && mx <= cells[i].x + cells[i].w &&
                my >= cells[i].y && my <= cells[i].y + cells[i].h) {
                return cells[i].index;
            }
        }
        return -1;
    }
};

class LayoutEngine {
private:
    static int computeTreeStats(const TreeNode* node, int depth, int& maxDepth);
    static void assignTreeNodePositions(const TreeNode* node, int parentId, int depth,
                                        int& inorderIdx, int totalNodes, int maxDepth,
                                        float areaX, float areaY, float areaW, float areaH,
                                        LayoutResult& out, int& outNodeHeight, int& outNodeSize);

    static int computeAVLStats(const AVLNode* node, int depth, int& maxDepth);
    static void assignAVLNodePositions(const AVLTree& avl, AVLNode* node,
                                       int parentId, int depth, int& inorderIdx,
                                       int totalNodes, int maxDepth,
                                       float areaX, float areaY, float areaW, float areaH,
                                       LayoutResult& out);

public:
    static void layoutBST(const BST& bst, float areaX, float areaY, float areaW, float areaH,
                          LayoutResult& out);
    static void layoutBinaryTree(const BinaryTree& tree, float areaX, float areaY, float areaW, float areaH,
                                 LayoutResult& out);
    static void layoutAVL(const AVLTree& avl, float areaX, float areaY, float areaW, float areaH,
                          LayoutResult& out);
    static void layoutMaxHeap(const MaxHeap& heap, const int* sortedArr, int sortedCount,
                              float areaX, float areaY, float areaW, float areaH,
                              LayoutResult& out);
    static void layoutHeapArray(const int* arr, int activeSize, const int* sortedArr, int sortedCount,
                                float areaX, float areaY, float areaW, float areaH,
                                LayoutResult& out);
};

} // namespace VaultHeist
