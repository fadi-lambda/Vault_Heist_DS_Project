#include "game/LevelLoader.h"
#include "game/Json.h"
#include <fstream>

namespace VaultHeist {

// ---------------------------------------------------------------------------
// JSON -> LevelData
// ---------------------------------------------------------------------------
namespace {

int readIntArray(const JsonValue* root, const char* key, int* out, int cap) {
    const JsonValue* arr = root->get(key);
    if (!arr || !arr->isArray()) return 0;
    int n = 0;
    for (int i = 0; i < arr->size() && n < cap; ++i) {
        const JsonValue* v = arr->at(i);
        if (v && v->isInt()) out[n++] = v->asInt();
    }
    return n;
}

// null / missing child => -1
int childKey(const JsonValue* obj, const char* key) {
    const JsonValue* v = obj->get(key);
    return (v && v->isInt()) ? v->asInt() : -1;
}

int firstPresent(const JsonValue* obj, const char* a, const char* b, const char* c) {
    const char* names[3] = { a, b, c };
    for (int i = 0; i < 3; ++i) {
        if (!names[i]) continue;
        const JsonValue* v = obj->get(names[i]);
        if (v && v->isInt()) return v->asInt();
    }
    return 0;
}

} // namespace

bool loadLevelFromJson(const std::string& path, LevelData& out, std::string& err) {
    JsonValue* root = parseJsonFile(path, err);
    if (!root) return false;
    if (!root->isObject()) {
        err = path + ": top-level JSON value must be an object";
        delete root;
        return false;
    }

    out = LevelData();
    out.id          = root->getInt("level_id", 0);
    out.act         = root->getInt("act", 0);
    out.title       = root->getString("title", "");
    out.conceptText = root->getString("concept", "");
    out.objective   = root->getString("objective", "");
    out.controls    = root->getString("controls", "");
    out.startStatus = root->getString("start_status", "");

    if (out.id == 0 || out.title.empty()) {
        err = path + ": missing level_id or title";
        delete root;
        return false;
    }

    // Explicit tree
    const JsonValue* nodes = root->get("nodes");
    if (nodes && nodes->isArray()) {
        for (int i = 0; i < nodes->size() && out.nodeCount < LEVEL_MAX_NODES; ++i) {
            const JsonValue* n = nodes->at(i);
            if (!n || !n->isObject()) continue;
            LevelNodeSpec& spec = out.nodes[out.nodeCount++];
            spec.key   = n->getInt("key", 0);
            spec.left  = childKey(n, "left");
            spec.right = childKey(n, "right");
        }
    }

    out.initialKeyCount     = readIntArray(root, "initial_keys",   out.initialKeys,    LEVEL_MAX_INTS);
    out.initialArrayCount   = readIntArray(root, "initial_array",  out.initialArray,   LEVEL_MAX_INTS);
    out.preorderCount       = readIntArray(root, "preorder",       out.preorder,       LEVEL_MAX_INTS);
    out.inorderCount        = readIntArray(root, "inorder",        out.inorder,        LEVEL_MAX_INTS);
    out.targetKeyCount      = readIntArray(root, "target_keys",    out.targetKeys,     LEVEL_MAX_INTS);
    out.deleteSequenceCount = readIntArray(root, "delete_sequence",out.deleteSequence, LEVEL_MAX_INTS);
    out.kQueryCount         = readIntArray(root, "k_queries",      out.kQueries,       LEVEL_MAX_INTS);
    out.rangeLo = root->getInt("range_lo", 0);
    out.rangeHi = root->getInt("range_hi", 0);

    const JsonValue* ins = root->get("insertion_sequence");
    if (ins && ins->isArray()) {
        for (int i = 0; i < ins->size() && out.insertCount < LEVEL_MAX_INSERTS; ++i) {
            const JsonValue* e = ins->at(i);
            if (!e || !e->isObject()) continue;
            LevelInsertSpec& s = out.inserts[out.insertCount++];
            s.key = e->getInt("key", 0);
            s.pivotKey = e->getInt("pivot_key", -1);
            s.expectedRotation = e->getString("expected_rotation", "None");
        }
    }

    const JsonValue* puz = root->get("puzzles");
    if (puz && puz->isArray()) {
        for (int i = 0; i < puz->size() && out.puzzleCount < LEVEL_MAX_PUZZLES; ++i) {
            const JsonValue* p = puz->at(i);
            if (!p || !p->isObject()) continue;
            LevelPuzzleSpec& s = out.puzzles[out.puzzleCount++];
            s.caseName = p->getString("case", "");
            s.rootKey  = p->getInt("root_key", 0);
            s.childKey = firstPresent(p, "left_child", "right_child", nullptr);
            s.innerKey = firstPresent(p, "left_grandchild", "right_grandchild", "inner_child");
        }
    }

    const JsonValue* hints = root->get("hints");
    if (hints && hints->isArray()) {
        for (int i = 0; i < hints->size() && out.hintCount < LEVEL_MAX_HINTS; ++i) {
            const JsonValue* h = hints->at(i);
            if (h && h->isString()) out.hints[out.hintCount++] = h->asString();
        }
    }

    delete root;
    return true;
}

std::string levelJsonRelativePath(int levelId) {
    int act = levelId / 10;
    int idx = levelId % 10;
    return "levels/act" + std::to_string(act) + "/level_" + std::to_string(act) + "_" +
           std::to_string(idx) + ".json";
}

bool loadLevelById(int levelId, LevelData& out, std::string& err) {
    static const char* const prefixes[6] = { "", "../", "../../", "../../../", "../../../../", "../../../../../" };
    std::string rel = levelJsonRelativePath(levelId);
    for (int i = 0; i < 6; ++i) {
        std::string path = std::string(prefixes[i]) + rel;
        std::ifstream probe(path);
        if (probe.good()) {
            probe.close();
            return loadLevelFromJson(path, out, err);
        }
    }
    err = "level file not found: " + rel + " (searched working dir and up to 5 parents)";
    return false;
}

// ---------------------------------------------------------------------------
// Derived answers
// ---------------------------------------------------------------------------
namespace {

int findSpecIndex(const LevelData& d, int key) {
    for (int i = 0; i < d.nodeCount; ++i) if (d.nodes[i].key == key) return i;
    return -1;
}

int heightOfKey(const LevelData& d, int key) {
    int idx = findSpecIndex(d, key);
    if (key < 0 || idx < 0) return 0;
    int lh = heightOfKey(d, d.nodes[idx].left);
    int rh = heightOfKey(d, d.nodes[idx].right);
    return 1 + (lh > rh ? lh : rh);
}

struct SimBstNode {
    int key;
    int left;
    int right;
    SimBstNode() : key(0), left(-1), right(-1) {}
};

int simInsertSteps(SimBstNode* pool, int& count, int cap, int key) {
    if (count == 0) {
        if (count < cap) {
            pool[count].key = key;
            pool[count].left = -1;
            pool[count].right = -1;
            ++count;
        }
        return 1;
    }
    int steps = 0;
    int cur = 0;
    while (cur >= 0 && cur < count) {
        ++steps;
        if (key < pool[cur].key) {
            if (pool[cur].left < 0) {
                if (count < cap) {
                    int ni = count++;
                    pool[ni].key = key;
                    pool[ni].left = -1;
                    pool[ni].right = -1;
                    pool[cur].left = ni;
                }
                break;
            }
            cur = pool[cur].left;
        } else if (key > pool[cur].key) {
            if (pool[cur].right < 0) {
                if (count < cap) {
                    int ni = count++;
                    pool[ni].key = key;
                    pool[ni].left = -1;
                    pool[ni].right = -1;
                    pool[cur].right = ni;
                }
                break;
            }
            cur = pool[cur].right;
        } else {
            // Duplicate found -> 1 step to reject
            break;
        }
    }
    return steps;
}

int simSearchSteps(const SimBstNode* pool, int count, int key) {
    int steps = 0;
    int cur = (count > 0) ? 0 : -1;
    while (cur >= 0 && cur < count) {
        ++steps;
        if (key == pool[cur].key) break;
        cur = (key < pool[cur].key) ? pool[cur].left : pool[cur].right;
    }
    return steps;
}

int simRangeSteps(const SimBstNode* pool, int count, int idx, int lo, int hi) {
    if (idx < 0 || idx >= count) return 0;
    int steps = 0;
    if (pool[idx].key >= lo && pool[idx].key <= hi) ++steps;
    if (pool[idx].key <= lo && pool[idx].left >= 0) ++steps;
    if (pool[idx].key >= hi && pool[idx].right >= 0) ++steps;
    if (pool[idx].key > lo) steps += simRangeSteps(pool, count, pool[idx].left, lo, hi);
    if (pool[idx].key < hi) steps += simRangeSteps(pool, count, pool[idx].right, lo, hi);
    return steps;
}

void buildSimBst(const LevelData& d, SimBstNode* pool, int& count) {
    count = 0;
    for (int i = 0; i < d.initialKeyCount && count < LEVEL_MAX_INTS; ++i) {
        simInsertSteps(pool, count, LEVEL_MAX_INTS, d.initialKeys[i]);
    }
}

} // namespace

int findUnbalancedKeys(const LevelData& data, int* outKeys, int cap) {
    int n = 0;
    for (int i = 0; i < data.nodeCount && n < cap; ++i) {
        int bf = heightOfKey(data, data.nodes[i].left) - heightOfKey(data, data.nodes[i].right);
        if (bf > 1 || bf < -1) outKeys[n++] = data.nodes[i].key;
    }
    return n;
}

int countBottomUpSiftDowns(const int* arr, int n) {
    if (n <= 1) return 0;
    int work[LEVEL_MAX_INTS];
    int len = (n < LEVEL_MAX_INTS) ? n : LEVEL_MAX_INTS;
    for (int i = 0; i < len; ++i) work[i] = arr[i];
    int needed = 0;
    for (int start = len / 2 - 1; start >= 0; --start) {
        bool moved = false;
        int i = start;
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2, big = i;
            if (l < len && work[l] > work[big]) big = l;
            if (r < len && work[r] > work[big]) big = r;
            if (big == i) break;
            int t = work[i]; work[i] = work[big]; work[big] = t;
            moved = true;
            i = big;
        }
        if (moved) ++needed;
    }
    return needed;
}

int computeOptimalSteps(const LevelData& d) {
    switch (d.id) {
        case 11: return d.nodeCount;                       // visit every node once
        case 12: return d.preorderCount;                   // place one token per slot
        case 13: return d.nodeCount > 0 ? (d.nodeCount + 1) : 0; // postorder heights + diameter
        case 14: {                                         // 2 picks + LCA + path (up to tree diameter)
            if (d.nodeCount <= 0) return 0;
            int diam = 0;
            for (int i = 0; i < d.nodeCount; ++i) {
                int cand = heightOfKey(d, d.nodes[i].left) + heightOfKey(d, d.nodes[i].right) + 1;
                if (cand > diam) diam = cand;
            }
            return 3 + diam;
        }
        case 15: return d.nodeCount;                       // bottom-up swaps (up to N) or Auto Mirror
        case 21: {                                         // root-to-target comparisons + Found
            SimBstNode pool[LEVEL_MAX_INTS];
            int cnt = 0;
            buildSimBst(d, pool, cnt);
            int total = 0;
            for (int i = 0; i < d.targetKeyCount; ++i) total += simSearchSteps(pool, cnt, d.targetKeys[i]);
            return total;
        }
        case 22: {                                         // walk-to-slot inserts + duplicate rejection
            SimBstNode pool[LEVEL_MAX_INTS];
            int cnt = 0;
            buildSimBst(d, pool, cnt);
            int total = 0;
            for (int i = 0; i < d.targetKeyCount; ++i) {
                total += simInsertSteps(pool, cnt, LEVEL_MAX_INTS, d.targetKeys[i]);
            }
            return total;
        }
        case 23: return d.deleteSequenceCount;             // one case-classification click per deletion
        case 24: {                                         // range report + subtree pruning decisions
            SimBstNode pool[LEVEL_MAX_INTS];
            int cnt = 0;
            buildSimBst(d, pool, cnt);
            return simRangeSteps(pool, cnt, 0, d.rangeLo, d.rangeHi);
        }
        case 25: {                                         // sorted degenerate inserts + search on chain
            SimBstNode pool[LEVEL_MAX_INTS];
            int cnt = 0;
            buildSimBst(d, pool, cnt);
            int total = 0;
            for (int i = 0; i < d.targetKeyCount; ++i) {
                total += simInsertSteps(pool, cnt, LEVEL_MAX_INTS, d.targetKeys[i]);
            }
            int searchKey = (d.kQueryCount > 0)
                                ? d.kQueries[0]
                                : (d.targetKeyCount > 0 ? d.targetKeys[d.targetKeyCount - 1] : -1);
            if (searchKey >= 0) total += simSearchSteps(pool, cnt, searchKey);
            return total;
        }
        case 31: {
            int keys[LEVEL_MAX_NODES];
            return findUnbalancedKeys(d, keys, LEVEL_MAX_NODES);
        }
        case 32: return d.puzzleCount;                     // one rotation per puzzle
        case 33: return d.puzzleCount * 2;                 // two rotations per puzzle
        case 34: return d.insertCount;                     // one prediction per insert
        case 35: return d.kQueryCount;                     // one click per query
        case 41:
        case 42: return countBottomUpSiftDowns(d.initialArray, d.initialArrayCount);
        case 43:
        case 44: return d.initialArrayCount;               // one extraction per element
        case 45: return (d.initialArrayCount > 0 ? 1 : 0) + d.initialArrayCount; // build heap + extractions
        default: return 1;
    }
}

} // namespace VaultHeist
