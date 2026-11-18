#pragma once
#include "game/LevelData.h"
#include <string>

namespace VaultHeist {

// Parses one level JSON file into out. Returns false and fills err on any problem.
bool loadLevelFromJson(const std::string& path, LevelData& out, std::string& err);

// Locates levels/actN/level_N_M.json by probing a few parent directories of the
// working directory (so it works from the repo root or the CMake build tree),
// then loads it. levelId uses the 11..45 numbering (act*10 + index).
bool loadLevelById(int levelId, LevelData& out, std::string& err);

// Builds "<base>levels/actN/level_N_M.json" path for a level id.
std::string levelJsonRelativePath(int levelId);

// ----- Level-derived answers (computed from the data, never hardcoded) -----

// Minimum number of player actions a perfect run needs, computed from the
// level's own data at level start (Project.pdf §2.2).
int computeOptimalSteps(const LevelData& data);

// Keys of the explicit-tree nodes (LevelData::nodes) whose |balance factor| > 1,
// in node-list order. Returns count written (<= cap).
int findUnbalancedKeys(const LevelData& data, int* outKeys, int cap);

// Count of heap-array indices that need a sift-down during bottom-up build.
int countBottomUpSiftDowns(const int* arr, int n);

} // namespace VaultHeist
