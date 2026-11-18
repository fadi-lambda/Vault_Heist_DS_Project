#pragma once
#include <string>

namespace VaultHeist {

// ---------------------------------------------------------------------------
// LevelData: everything a level needs that is *data* (not mechanics).
// Filled by loadLevelFromJson() from levels/actN/level_N_M.json.
//
// Unified schema (all keys optional except level_id/act/title/objective):
//   level_id, act, title, concept, objective, controls, start_status   (strings / ints)
//   nodes[]            : { key, left, right }  explicit tree (first entry = root; null = no child)
//   initial_keys[]     : insertion-order keys  (BST / AVL levels)
//   initial_array[]    : array-form data       (heap levels)
//   preorder[], inorder[]  : reconstruction input (1.3)
//   target_keys[]      : highlighted / queried keys (e.g. LCA pair, sweep target)
//   delete_sequence[]  : keys to delete in order (2.2)
//   range_lo, range_hi : BST range query bounds (2.5)
//   k_queries[]        : k values for kth-smallest challenges (2.5, 3.5)
//   insertion_sequence[] : { key, pivot_key, expected_rotation } (3.4)
//   puzzles[]          : { case, root_key, left_child|right_child,
//                          left_grandchild|right_grandchild|inner_child, ... } (3.2 / 3.3)
//   hints[]            : short nudges, indexed by the level's current stage
// ---------------------------------------------------------------------------

static const int LEVEL_MAX_NODES   = 32;
static const int LEVEL_MAX_INTS    = 32;
static const int LEVEL_MAX_HINTS   = 6;
static const int LEVEL_MAX_PUZZLES = 4;
static const int LEVEL_MAX_INSERTS = 16;

struct LevelNodeSpec {
    int key;
    int left;     // child key, or -1 for none
    int right;    // child key, or -1 for none
    LevelNodeSpec() : key(0), left(-1), right(-1) {}
};

struct LevelPuzzleSpec {
    std::string caseName;   // "LL", "RR", "LR", "RL"
    int rootKey;
    int childKey;           // left_child (LL, LR) or right_child (RR, RL)
    int innerKey;           // left_grandchild / right_grandchild / inner_child
    LevelPuzzleSpec() : rootKey(0), childKey(0), innerKey(0) {}
};

struct LevelInsertSpec {
    int key;
    int pivotKey;                 // node the player is expected to click
    std::string expectedRotation; // "LL","RR","LR","RL" or "None"
    LevelInsertSpec() : key(0), pivotKey(-1) {}
};

struct LevelData {
    int id;                 // e.g. 11 for level 1.1
    int act;
    std::string title;
    std::string conceptText;
    std::string objective;
    std::string controls;
    std::string startStatus;

    LevelNodeSpec nodes[LEVEL_MAX_NODES];
    int nodeCount;

    int initialKeys[LEVEL_MAX_INTS];   int initialKeyCount;
    int initialArray[LEVEL_MAX_INTS];  int initialArrayCount;
    int expectedSortedResult[LEVEL_MAX_INTS]; int expectedSortedResultCount;
    int preorder[LEVEL_MAX_INTS];      int preorderCount;
    int inorder[LEVEL_MAX_INTS];       int inorderCount;
    int targetKeys[LEVEL_MAX_INTS];    int targetKeyCount;
    int deleteSequence[LEVEL_MAX_INTS];int deleteSequenceCount;
    int kQueries[LEVEL_MAX_INTS];      int kQueryCount;

    int rangeLo;
    int rangeHi;

    LevelInsertSpec inserts[LEVEL_MAX_INSERTS]; int insertCount;
    LevelPuzzleSpec puzzles[LEVEL_MAX_PUZZLES]; int puzzleCount;

    std::string hints[LEVEL_MAX_HINTS];
    int hintCount;

    LevelData()
        : id(0), act(0), nodeCount(0), initialKeyCount(0), initialArrayCount(0),
          expectedSortedResultCount(0), preorderCount(0), inorderCount(0), targetKeyCount(0),
          deleteSequenceCount(0), kQueryCount(0), rangeLo(0), rangeHi(0),
          insertCount(0), puzzleCount(0), hintCount(0) {}
};

} // namespace VaultHeist
