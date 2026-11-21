#include "game/EndlessMode.h"
#include <fstream>
#include <cstdlib>

namespace VaultHeist {

namespace {

static inline float maxFloatLocal(float a, float b) { return (a > b) ? a : b; }
static inline float clampFloatLocal(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool keyExistsInArray(const int* arr, int count, int key) {
    for (int i = 0; i < count; ++i) {
        if (arr[i] == key) return true;
    }
    return false;
}

static void collectPreorderKeys(const AVLNode* node, int* out, int& count, int cap) {
    if (!node || count >= cap) return;
    out[count++] = node->key;
    collectPreorderKeys(node->left, out, count, cap);
    collectPreorderKeys(node->right, out, count, cap);
}

static int validateSubtreeFull(const AVLNode* node, bool& ok) {
    if (!node || !ok) return 0;
    int ls = validateSubtreeFull(node->left, ok);
    int rs = validateSubtreeFull(node->right, ok);
    int realSz = 1 + ls + rs;
    int lh = node->left ? node->left->height : 0;
    int rh = node->right ? node->right->height : 0;
    int bf = lh - rh;
    if (node->size != realSz || bf < -1 || bf > 1) ok = false;
    return realSz;
}

static void serializeAvlPreorderWithNulls(const AVLNode* node, std::string& out) {
    if (!node) {
        if (!out.empty()) out += ' ';
        out += '#';
        return;
    }
    if (!out.empty()) out += ' ';
    out += std::to_string(node->key);
    serializeAvlPreorderWithNulls(node->left, out);
    serializeAvlPreorderWithNulls(node->right, out);
}

static AVLNode* rebuildAvlFromTokens(const std::string* tokens, int tokenCount,
                                     int& idx, AVLNode* parent, bool& parseOk) {
    if (idx >= tokenCount) {
        parseOk = false;
        return nullptr;
    }
    const std::string& tok = tokens[idx++];
    if (tok == "#") return nullptr;

    char* endPtr = nullptr;
    long val = std::strtol(tok.c_str(), &endPtr, 10);
    if (endPtr == tok.c_str() || *endPtr != '\0') {
        parseOk = false;
        return nullptr;
    }

    AVLNode* node = new AVLNode(static_cast<int>(val));
    node->parent = parent;
    node->left = rebuildAvlFromTokens(tokens, tokenCount, idx, node, parseOk);
    node->right = rebuildAvlFromTokens(tokens, tokenCount, idx, node, parseOk);
    int lh = node->left ? node->left->height : 0;
    int rh = node->right ? node->right->height : 0;
    node->height = 1 + ((lh > rh) ? lh : rh);
    int ls = node->left ? node->left->size : 0;
    int rs = node->right ? node->right->size : 0;
    node->size = 1 + ls + rs;
    return node;
}

static std::string sanitizeField(const std::string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        if (out[i] == '|' || out[i] == '\r' || out[i] == '\n') out[i] = '_';
    }
    return out;
}

} // namespace

// =============================================================================
// EndlessLeaderboard
// =============================================================================

void EndlessLeaderboard::sortDescending() {
    for (int i = 1; i < count; ++i) {
        EndlessLeaderboardEntry keyEntry = entries[i];
        int j = i - 1;
        while (j >= 0 && (entries[j].survivalMs < keyEntry.survivalMs ||
                          (entries[j].survivalMs == keyEntry.survivalMs &&
                           entries[j].opsCompleted < keyEntry.opsCompleted))) {
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = keyEntry;
    }
}

void EndlessLeaderboard::load(const std::string& filePath) {
    path = filePath;
    count = 0;
    if (path.empty()) return;

    std::ifstream in(path);
    if (!in.is_open()) return;

    std::string line;
    while (count < ENDLESS_MAX_LB_ENTRIES && std::getline(in, line)) {
        size_t a = line.find('|');
        if (a == std::string::npos) continue;
        size_t b = line.find('|', a + 1);
        if (b == std::string::npos) continue;
        size_t c = line.find('|', b + 1);
        if (c == std::string::npos) continue;

        EndlessLeaderboardEntry e;
        e.survivalMs   = std::atoi(line.substr(0, a).c_str());
        e.opsCompleted = std::atoi(line.substr(a + 1, b - a - 1).c_str());
        e.seed         = static_cast<unsigned int>(std::strtoul(line.substr(b + 1, c - b - 1).c_str(), nullptr, 10));
        e.nameDate     = line.substr(c + 1);
        while (!e.nameDate.empty() && (e.nameDate.back() == '\r' || e.nameDate.back() == '\n')) {
            e.nameDate.pop_back();
        }
        entries[count++] = e;
    }
    sortDescending();
}

bool EndlessLeaderboard::writeFile() const {
    if (path.empty()) return false;
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;
    for (int i = 0; i < count; ++i) {
        out << entries[i].survivalMs << '|'
            << entries[i].opsCompleted << '|'
            << entries[i].seed << '|'
            << sanitizeField(entries[i].nameDate) << '\n';
    }
    return out.good();
}

bool EndlessLeaderboard::recordRun(const EndlessLeaderboardEntry& entry) {
    EndlessLeaderboardEntry clean = entry;
    clean.nameDate = sanitizeField(clean.nameDate);
    if (clean.nameDate.empty()) clean.nameDate = "Operator";

    if (count < ENDLESS_MAX_LB_ENTRIES) {
        entries[count++] = clean;
        sortDescending();
    } else {
        // Already sorted descending; only replace worst entry if this run beats or ties it.
        if (clean.survivalMs > entries[count - 1].survivalMs ||
            (clean.survivalMs == entries[count - 1].survivalMs &&
             clean.opsCompleted >= entries[count - 1].opsCompleted)) {
            entries[count - 1] = clean;
            sortDescending();
        }
    }
    return path.empty() ? true : writeFile();
}

std::string EndlessLeaderboard::formatTimeMs(int ms) {
    if (ms < 0) ms = 0;
    int tenthsTotal = (ms + 50) / 100;
    int secs = tenthsTotal / 10;
    int tenths = tenthsTotal % 10;
    return std::to_string(secs) + "." + std::to_string(tenths) + "s";
}

// =============================================================================
// EndlessSession
// =============================================================================

EndlessSession::EndlessSession()
    : initialSeed(1), rngState(1), survivalTimeSec(0.f), opWindowSec(8.0f),
      opTimeRemaining(8.0f), opsCompleted(0), pendingOp(),
      active(false), gameOver(false), gameOverReason("") {}

float EndlessSession::computeWindowFor(float survivalSec, int opsDone) {
    if (survivalSec < 0.f) survivalSec = 0.f;
    if (opsDone < 0) opsDone = 0;
    float denom = 1.0f + 0.035f * survivalSec + 0.08f * static_cast<float>(opsDone);
    return maxFloatLocal(2.2f, 8.0f / denom);
}

float EndlessSession::computeOpsPerMin(float windowSec) {
    if (windowSec <= 0.1f) return 600.0f;
    return 60.0f / windowSec;
}

bool EndlessSession::validateTreeFull(const AVLTree& tree) {
    if (!tree.isValidAVL()) return false;
    bool ok = true;
    validateSubtreeFull(tree.getRoot(), ok);
    return ok;
}

unsigned int EndlessSession::nextRand() {
    unsigned int x = rngState;
    if (x == 0u) x = 0x9E3779B9u;
    x ^= (x << 13);
    x ^= (x >> 17);
    x ^= (x << 5);
    rngState = x;
    return x;
}

EndlessOp EndlessSession::generateNextOp(const AVLTree& tree) {
    int keys[ENDLESS_MAX_TREE_KEYS];
    int keyCount = 0;
    collectPreorderKeys(tree.getRoot(), keys, keyCount, ENDLESS_MAX_TREE_KEYS);

    int opType = ENDLESS_OP_INSERT;
    if (keyCount <= 3) {
        opType = ENDLESS_OP_INSERT;
    } else if (keyCount >= 16) {
        opType = ENDLESS_OP_DELETE;
    } else {
        opType = ((nextRand() % 100u) < 58u) ? ENDLESS_OP_INSERT : ENDLESS_OP_DELETE;
    }

    if (opType == ENDLESS_OP_DELETE && keyCount > 0) {
        int idx = static_cast<int>(nextRand() % static_cast<unsigned int>(keyCount));
        return EndlessOp(ENDLESS_OP_DELETE, keys[idx]);
    }

    // Generate a unique insert candidate not currently in tree
    int cand = 5 + static_cast<int>(nextRand() % 95u);
    for (int probe = 0; probe < 250; ++probe) {
        if (!keyExistsInArray(keys, keyCount, cand)) break;
        cand = 5 + ((cand + 7) % 190);
    }
    return EndlessOp(ENDLESS_OP_INSERT, cand);
}

void EndlessSession::startNew(unsigned int seed, AVLTree& tree) {
    initialSeed = (seed == 0u) ? 1u : seed;
    rngState = initialSeed;
    survivalTimeSec = 0.f;
    opsCompleted = 0;
    active = true;
    gameOver = false;
    gameOverReason = "";

    tree.clear();
    const int initKeys[7] = { 40, 20, 60, 10, 30, 50, 70 };
    for (int i = 0; i < 7; ++i) {
        tree.insert(initKeys[i]);
    }

    opWindowSec = computeWindowFor(0.f, 0);
    opTimeRemaining = opWindowSec;
    pendingOp = generateNextOp(tree);
}

bool EndlessSession::tick(float dt) {
    if (!active || gameOver) return false;
    if (dt <= 0.f) return false;

    survivalTimeSec += dt;
    opTimeRemaining -= dt;
    if (opTimeRemaining <= 0.f) {
        opTimeRemaining = 0.f;
        active = false;
        gameOver = true;
        gameOverReason = "Deadline missed! Rising operation rate overwhelmed the vault.";
        return true;
    }
    return false;
}

bool EndlessSession::executePending(AVLTree& tree, EventBatch& outBatch) {
    if (!active || gameOver) return false;

    if (pendingOp.type == ENDLESS_OP_INSERT) {
        tree.insert(pendingOp.key, outBatch);
    } else {
        tree.deleteKey(pendingOp.key, outBatch);
    }

    if (!validateTreeFull(tree)) {
        active = false;
        gameOver = true;
        gameOverReason = "AVL invariant validator failed after operation!";
        return false;
    }

    opsCompleted++;
    opWindowSec = computeWindowFor(survivalTimeSec, opsCompleted);
    opTimeRemaining = opWindowSec;
    pendingOp = generateNextOp(tree);
    return true;
}

bool EndlessSession::saveToFile(const std::string& filePath, const AVLTree& tree) const {
    if (filePath.empty()) return false;
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;

    std::string preorder;
    serializeAvlPreorderWithNulls(tree.getRoot(), preorder);

    int survMs = getSurvivalMs();
    int winMs = static_cast<int>(opWindowSec * 1000.0f + 0.5f);
    int remMs = static_cast<int>(opTimeRemaining * 1000.0f + 0.5f);

    out << "VAULT_HEIST_ENDLESS_SAVE_V1\n";
    out << "initial_seed " << initialSeed << '\n';
    out << "rng_state " << rngState << '\n';
    out << "survival_ms " << survMs << '\n';
    out << "ops_completed " << opsCompleted << '\n';
    out << "pending_type " << pendingOp.type << '\n';
    out << "pending_key " << pendingOp.key << '\n';
    out << "window_ms " << winMs << '\n';
    out << "remaining_ms " << remMs << '\n';
    out << "avl_preorder " << preorder << '\n';
    return out.good();
}

bool EndlessSession::loadFromFile(const std::string& filePath, AVLTree& tree) {
    if (filePath.empty()) return false;
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    std::string header;
    if (!std::getline(in, header)) return false;
    while (!header.empty() && (header.back() == '\r' || header.back() == '\n')) header.pop_back();
    if (header != "VAULT_HEIST_ENDLESS_SAVE_V1") return false;

    unsigned int sInit = 1u;
    unsigned int sRng = 1u;
    int sSurvMs = 0;
    int sOps = 0;
    int sType = ENDLESS_OP_INSERT;
    int sKey = 0;
    int sWinMs = 8000;
    int sRemMs = 8000;
    std::string preorderLine;

    bool hasSeed = false, hasRng = false, hasTree = false;
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty()) continue;

        size_t sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string tag = line.substr(0, sp);
        std::string val = line.substr(sp + 1);

        if (tag == "initial_seed")  { sInit   = static_cast<unsigned int>(std::strtoul(val.c_str(), nullptr, 10)); hasSeed = true; }
        else if (tag == "rng_state")    { sRng    = static_cast<unsigned int>(std::strtoul(val.c_str(), nullptr, 10)); hasRng = true; }
        else if (tag == "survival_ms")  { sSurvMs = std::atoi(val.c_str()); }
        else if (tag == "ops_completed"){ sOps    = std::atoi(val.c_str()); }
        else if (tag == "pending_type") { sType   = std::atoi(val.c_str()); }
        else if (tag == "pending_key")  { sKey    = std::atoi(val.c_str()); }
        else if (tag == "window_ms")    { sWinMs  = std::atoi(val.c_str()); }
        else if (tag == "remaining_ms") { sRemMs  = std::atoi(val.c_str()); }
        else if (tag == "avl_preorder") { preorderLine = val; hasTree = true; }
    }

    if (!hasSeed || !hasRng || !hasTree) return false;
    if (sType != ENDLESS_OP_INSERT && sType != ENDLESS_OP_DELETE) return false;

    std::string tokens[ENDLESS_MAX_SAVE_TOKENS];
    int tokCount = 0;
    size_t i = 0;
    while (i < preorderLine.size() && tokCount < ENDLESS_MAX_SAVE_TOKENS) {
        while (i < preorderLine.size() && preorderLine[i] == ' ') ++i;
        if (i >= preorderLine.size()) break;
        size_t j = i;
        while (j < preorderLine.size() && preorderLine[j] != ' ') ++j;
        tokens[tokCount++] = preorderLine.substr(i, j - i);
        i = j;
    }
    if (tokCount <= 0) return false;

    int idx = 0;
    bool parseOk = true;
    AVLNode* rebuiltRoot = rebuildAvlFromTokens(tokens, tokCount, idx, nullptr, parseOk);
    if (!parseOk || idx != tokCount) {
        AVLTree tmp;
        tmp.setRoot(rebuiltRoot);
        tmp.clear();
        return false;
    }

    AVLTree candidate;
    candidate.setRoot(rebuiltRoot);
    if (!validateTreeFull(candidate)) {
        candidate.clear();
        return false;
    }

    // Move ownership into tree safely
    tree.clear();
    tree.setRoot(candidate.getRoot());
    candidate.setRoot(nullptr);

    initialSeed = (sInit == 0u) ? 1u : sInit;
    rngState = (sRng == 0u) ? 1u : sRng;
    survivalTimeSec = (sSurvMs < 0) ? 0.f : (static_cast<float>(sSurvMs) / 1000.0f);
    opsCompleted = (sOps < 0) ? 0 : sOps;
    opWindowSec = computeWindowFor(survivalTimeSec, opsCompleted);
    if (sWinMs > 0) {
        float loadedWin = static_cast<float>(sWinMs) / 1000.0f;
        opWindowSec = clampFloatLocal(loadedWin, 2.2f, 8.0f);
    }
    float loadedRem = (sRemMs <= 0) ? opWindowSec : (static_cast<float>(sRemMs) / 1000.0f);
    opTimeRemaining = clampFloatLocal(loadedRem, 0.25f, opWindowSec);
    pendingOp = EndlessOp(sType, sKey);
    active = true;
    gameOver = false;
    gameOverReason = "";
    return true;
}

} // namespace VaultHeist
