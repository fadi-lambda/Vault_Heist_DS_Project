#pragma once

#include "structures/BinaryTree.h"
#include "structures/BST.h"
#include "structures/AVLTree.h"
#include "structures/MaxHeap.h"
#include <string>

namespace VaultHeist {

static const int MAX_UNDO_SNAPSHOTS = 32;
static const int MAX_SERIALIZED_LEN = 256;
static const int SERIALIZE_NULL_SENTINEL = -999999;

struct GameSnapshot {
    // Serialized BinaryTree (preorder with null sentinel)
    int btPre[MAX_SERIALIZED_LEN];
    int btLen;

    // Serialized BST (preorder keys)
    int bstPre[MAX_SERIALIZED_LEN];
    int bstLen;

    // Serialized AVLTree (preorder keys)
    int avlPre[MAX_SERIALIZED_LEN];
    int avlLen;

    // Serialized Heap (real MaxHeap or raw heapArr + sortedArr)
    int heapData[MAX_SERIALIZED_LEN];
    int heapLen;
    int sortedData[MAX_SERIALIZED_LEN];
    int sortedLen;
    bool useRealHeap;

    // Level & HUD state
    int alarmSteps;
    int score;
    int stepCount;
    int penaltyCount;
    int penaltyPts;
    int bonus;
    bool hintUsed;
    bool act1Reconstructed;
    int act2Step21;
    int act2DelStage;
    int act2BossStage;
    int act2Clicked24Mask;
    int avlQueuePos;
    std::string playerPred;
    int bossQueryIdx;
    int buildStepIdx;
    bool act4Built;
    int act1Hl1;
    int act1Hl2;
    int act2Hl1;
    int act2Hl2;
    int hlIndex1;
    int hlIndex2;
    std::string status;

    GameSnapshot()
        : btLen(0), bstLen(0), avlLen(0), heapLen(0), sortedLen(0),
          useRealHeap(false), alarmSteps(0), score(1000),
          stepCount(0), penaltyCount(0), penaltyPts(0), bonus(0), hintUsed(false),
          act1Reconstructed(false), act2Step21(0), act2DelStage(0),
          act2BossStage(0), act2Clicked24Mask(0), avlQueuePos(0),
          bossQueryIdx(1), buildStepIdx(-1), act4Built(false),
          act1Hl1(-9999), act1Hl2(-9999), act2Hl1(-9999), act2Hl2(-9999),
          hlIndex1(-1), hlIndex2(-1) {}
};

class UndoStack {
private:
    GameSnapshot frames[MAX_UNDO_SNAPSHOTS];
    int count;

    static void serializeTreeWithNulls(const TreeNode* node, int* out, int& len);
    static void serializePreorder(const TreeNode* node, int* out, int& len);
    static void serializeAVLPreorder(const AVLNode* node, int* out, int& len);
    static void rebuildTreeFromNullPreorder(BinaryTree& tree, TreeNode* parent, bool asLeft,
                                            const int* arr, int len, int& idx);

public:
    UndoStack();

    void clear();
    bool isEmpty() const { return count == 0; }
    int getCount() const { return count; }

    void push(const GameSnapshot& snap);
    bool pop(GameSnapshot& out);

    // Public API serialization helpers (§8)
    static void captureStructures(const BinaryTree& bt, const BST& bst,
                                  const AVLTree& avl, const MaxHeap& heap,
                                  const int* rawHeapArr, int rawHeapSize,
                                  const int* sortedArr, int sortedCount,
                                  bool useRealHeap, GameSnapshot& snap);

    static void restoreStructures(const GameSnapshot& snap,
                                  BinaryTree& bt, BST& bst,
                                  AVLTree& avl, MaxHeap& heap,
                                  int* rawHeapArr, int& rawHeapSize,
                                  int* sortedArr, int& sortedCount);
};

} // namespace VaultHeist
