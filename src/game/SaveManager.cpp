#include "game/SaveManager.h"
#include <fstream>
#include <sstream>

namespace VaultHeist {

void SaveManager::serializePreorderWithNulls(const TreeNode* node, std::string& out) {
    if (!node) {
        if (!out.empty()) out += ' ';
        out += '#';
        return;
    }
    if (!out.empty()) out += ' ';
    out += std::to_string(node->value);
    serializePreorderWithNulls(node->left, out);
    serializePreorderWithNulls(node->right, out);
}

void SaveManager::serializeAVLPreorder(const AVLNode* node, std::string& out) {
    if (!node) return;
    if (!out.empty()) out += ' ';
    out += std::to_string(node->key);
    serializeAVLPreorder(node->left, out);
    serializeAVLPreorder(node->right, out);
}

int SaveManager::tokenize(const std::string& line, std::string* outTokens, int maxTokens) {
    std::istringstream iss(line);
    std::string tok;
    int count = 0;
    while (count < maxTokens && (iss >> tok)) {
        outTokens[count++] = tok;
    }
    return count;
}

TreeNode* SaveManager::deserializeBinaryTreeTokens(BinaryTree& tree, TreeNode* parent, bool asLeft,
                                                   const std::string* tokens, int tokenCount, int& idx) {
    if (idx >= tokenCount) return nullptr;
    const std::string& tok = tokens[idx++];
    if (tok == "#") return nullptr;
    int val = std::stoi(tok);
    TreeNode* created = tree.addNode(val, parent, asLeft);
    deserializeBinaryTreeTokens(tree, created, true, tokens, tokenCount, idx);
    deserializeBinaryTreeTokens(tree, created, false, tokens, tokenCount, idx);
    return created;
}

std::string SaveManager::exportBinaryTreePreorder(const BinaryTree& tree) {
    std::string out;
    serializePreorderWithNulls(tree.getRoot(), out);
    return out;
}

bool SaveManager::importBinaryTreePreorder(const std::string& serialized, BinaryTree& outTree) {
    outTree.clear();
    if (serialized.empty()) return true;
    static const int MAX_TOKENS = 512;
    std::string tokens[MAX_TOKENS];
    int n = tokenize(serialized, tokens, MAX_TOKENS);
    int idx = 0;
    deserializeBinaryTreeTokens(outTree, nullptr, true, tokens, n, idx);
    return Validator::validateBinaryTree(outTree);
}

bool SaveManager::saveToFile(const std::string& filePath, const SaveData& data,
                             const BinaryTree& bt, const BST& bst, const AVLTree& avl,
                             const int* heapArr, int activeHeapSize,
                             const int* sortedArr, int sortedCount) {
    std::ofstream ofs(filePath);
    if (!ofs.is_open()) return false;

    ofs << "VAULT_HEIST_SAVE_V1\n";
    ofs << "SEED " << data.seed << "\n";
    ofs << "ENDLESS " << data.endlessWave << " " << data.endlessScore << "\n";

    ofs << "UNLOCKED";
    for (int i = 0; i < SaveData::NUM_LEVELS; ++i) ofs << " " << (data.unlocked[i] ? 1 : 0);
    ofs << "\n";

    ofs << "STARS";
    for (int i = 0; i < SaveData::NUM_LEVELS; ++i) ofs << " " << data.stars[i];
    ofs << "\n";

    std::string btStr = exportBinaryTreePreorder(bt);
    ofs << "BT " << btStr << "\n";

    std::string bstStr;
    serializePreorderWithNulls(bst.getRoot(), bstStr);
    ofs << "BST " << bstStr << "\n";

    std::string avlStr;
    serializeAVLPreorder(avl.getRoot(), avlStr);
    ofs << "AVL " << avlStr << "\n";

    int hCap = activeHeapSize;
    if (hCap > SaveData::MAX_HEAP_SAVE) hCap = SaveData::MAX_HEAP_SAVE;
    ofs << "HEAP " << hCap;
    for (int i = 0; i < hCap; ++i) ofs << " " << (heapArr ? heapArr[i] : 0);
    ofs << "\n";

    int sCap = sortedCount;
    if (sCap > SaveData::MAX_HEAP_SAVE) sCap = SaveData::MAX_HEAP_SAVE;
    ofs << "SORTED " << sCap;
    for (int i = 0; i < sCap; ++i) ofs << " " << (sortedArr ? sortedArr[i] : 0);
    ofs << "\n";

    return true;
}

bool SaveManager::loadFromFile(const std::string& filePath, SaveData& outData,
                               BinaryTree& outBt, BST& outBst, AVLTree& outAvl) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) return false;

    std::string header;
    std::getline(ifs, header);
    if (header != "VAULT_HEIST_SAVE_V1") return false;

    outBt.clear();
    outBst.clear();
    outAvl.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "SEED") {
            iss >> outData.seed;
        } else if (tag == "ENDLESS") {
            iss >> outData.endlessWave >> outData.endlessScore;
        } else if (tag == "UNLOCKED") {
            for (int i = 0; i < SaveData::NUM_LEVELS; ++i) {
                int v = 0;
                if (iss >> v) outData.unlocked[i] = (v != 0);
            }
        } else if (tag == "STARS") {
            for (int i = 0; i < SaveData::NUM_LEVELS; ++i) {
                int s = 0;
                if (iss >> s) outData.stars[i] = s;
            }
        } else if (tag == "BT") {
            std::string rest;
            std::getline(iss, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            outData.binaryTreePreorder = rest;
            if (!importBinaryTreePreorder(rest, outBt)) return false;
        } else if (tag == "BST") {
            std::string rest;
            std::getline(iss, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            outData.bstPreorder = rest;
            std::istringstream bss(rest);
            std::string tok;
            while (bss >> tok) {
                if (tok != "#") outBst.insert(std::stoi(tok));
            }
        } else if (tag == "AVL") {
            std::string rest;
            std::getline(iss, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            outData.avlPreorder = rest;
            std::istringstream ass(rest);
            std::string tok;
            while (ass >> tok) {
                if (tok != "#") outAvl.insert(std::stoi(tok));
            }
        } else if (tag == "HEAP") {
            int n = 0;
            iss >> n;
            if (n < 0) n = 0;
            if (n > SaveData::MAX_HEAP_SAVE) n = SaveData::MAX_HEAP_SAVE;
            outData.activeHeapSize = n;
            for (int i = 0; i < n; ++i) {
                iss >> outData.heapArray[i];
            }
        } else if (tag == "SORTED") {
            int n = 0;
            iss >> n;
            if (n < 0) n = 0;
            if (n > SaveData::MAX_HEAP_SAVE) n = SaveData::MAX_HEAP_SAVE;
            outData.sortedCount = n;
            for (int i = 0; i < n; ++i) {
                iss >> outData.sortedArray[i];
            }
        }
    }

    // Validate all restored structures before resuming game
    if (!Validator::validateBinaryTree(outBt)) return false;
    if (!Validator::validateBST(outBst)) return false;
    if (!Validator::validateAVL(outAvl)) return false;
    if (!Validator::validateHeapSortPartition(outData.heapArray, outData.activeHeapSize,
                                              outData.sortedArray, outData.sortedCount)) {
        return false;
    }
    return true;
}

} // namespace VaultHeist
