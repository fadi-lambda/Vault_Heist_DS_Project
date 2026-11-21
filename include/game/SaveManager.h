#pragma once

#include "structures/BinaryTree.h"
#include "structures/BST.h"
#include "structures/AVLTree.h"
#include "structures/MaxHeap.h"
#include "structures/Validator.h"
#include <string>

namespace VaultHeist {

struct SaveData {
    static const int NUM_LEVELS = 20;
    static const int MAX_HEAP_SAVE = 64;

    bool unlocked[NUM_LEVELS];
    int stars[NUM_LEVELS];

    int endlessWave;
    int endlessScore;
    unsigned int seed;

    std::string binaryTreePreorder; // e.g. "50 30 # # 70 60 # # #"
    std::string bstPreorder;
    std::string avlPreorder;

    int heapArray[MAX_HEAP_SAVE];
    int activeHeapSize;
    int sortedArray[MAX_HEAP_SAVE];
    int sortedCount;

    SaveData()
        : endlessWave(1), endlessScore(0), seed(1337u),
          activeHeapSize(0), sortedCount(0) {
        for (int i = 0; i < NUM_LEVELS; ++i) {
            unlocked[i] = (i == 0);
            stars[i] = 0;
        }
        for (int i = 0; i < MAX_HEAP_SAVE; ++i) {
            heapArray[i] = 0;
            sortedArray[i] = 0;
        }
    }
};

/**
 * @brief Plain-text save/load manager (Spec Section 8 / J).
 *
 * Saves campaign progress, stars, endless state, seed, preorder-with-#
 * tree representations, and heap array + active size.
 * Validates all restored structures before returning success.
 */
class SaveManager {
private:
    static void serializePreorderWithNulls(const TreeNode* node, std::string& out);
    static void serializeAVLPreorder(const AVLNode* node, std::string& out);
    static TreeNode* deserializeBinaryTreeTokens(BinaryTree& tree, TreeNode* parent, bool asLeft,
                                                 const std::string* tokens, int tokenCount, int& idx);
    static int tokenize(const std::string& line, std::string* outTokens, int maxTokens);

public:
    static std::string exportBinaryTreePreorder(const BinaryTree& tree);
    static bool importBinaryTreePreorder(const std::string& serialized, BinaryTree& outTree);

    static bool saveToFile(const std::string& filePath, const SaveData& data,
                           const BinaryTree& bt, const BST& bst, const AVLTree& avl,
                           const int* heapArr, int activeHeapSize,
                           const int* sortedArr, int sortedCount);

    static bool loadFromFile(const std::string& filePath, SaveData& outData,
                             BinaryTree& outBt, BST& outBst, AVLTree& outAvl);
};

} // namespace VaultHeist
