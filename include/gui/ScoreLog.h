#pragma once

#include <string>

namespace VaultHeist {

static const int MAX_SCORE_ENTRIES = 64;

struct ScoreEntry {
    std::string levelTitle;
    int score;
    int stars;
    int alarm;
    ScoreEntry() : score(0), stars(0), alarm(0) {}
};

/**
 * @brief Tiny plain-text score log (NOT a save/load system).
 *
 * One line per completed level: `score|stars|alarm|level title`.
 * Fixed-size array (no STL container); oldest entries are dropped past 64.
 */
class ScoreLog {
private:
    ScoreEntry entries[MAX_SCORE_ENTRIES];
    int count;
    std::string path;

    bool writeFile() const;

public:
    ScoreLog() : count(0) {}

    void load(const std::string& filePath);
    bool append(const ScoreEntry& e);
    int getCount() const { return count; }
    const ScoreEntry& get(int i) const { return entries[i]; }
    // Index of the rank-th best score (0 = best), or -1.
    int topIndex(int rank) const;
};

} // namespace VaultHeist
