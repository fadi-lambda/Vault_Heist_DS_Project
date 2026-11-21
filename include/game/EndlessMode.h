#pragma once

#include <string>
#include "structures/AVLTree.h"
#include "events/Event.h"

namespace VaultHeist {

// Endless Mode operation types (no enum allowed)
static const int ENDLESS_OP_INSERT = 1;
static const int ENDLESS_OP_DELETE = 2;

static const int ENDLESS_MAX_LB_ENTRIES    = 64;
static const int ENDLESS_MAX_TREE_KEYS     = 128;
static const int ENDLESS_MAX_SAVE_TOKENS   = 512;

struct EndlessOp {
    int type;
    int key;
    EndlessOp() : type(ENDLESS_OP_INSERT), key(0) {}
    EndlessOp(int t, int k) : type(t), key(k) {}
};

struct EndlessLeaderboardEntry {
    std::string nameDate;
    int survivalMs;
    int opsCompleted;
    unsigned int seed;

    EndlessLeaderboardEntry() : survivalMs(0), opsCompleted(0), seed(0) {}
};

/**
 * @brief Persistent descending leaderboard for Endless Mode runs.
 *
 * File format (endless_leaderboard.txt):
 *   survivalMs|opsCompleted|seed|nameDate
 */
class EndlessLeaderboard {
private:
    EndlessLeaderboardEntry entries[ENDLESS_MAX_LB_ENTRIES];
    int count;
    std::string path;

    void sortDescending();
    bool writeFile() const;

public:
    EndlessLeaderboard() : count(0) {}

    void load(const std::string& filePath);
    bool recordRun(const EndlessLeaderboardEntry& entry);

    int getCount() const { return count; }
    const EndlessLeaderboardEntry& get(int i) const { return entries[i]; }

    static std::string formatTimeMs(int ms);
};

/**
 * @brief Stateful Endless Mode controller driving a seeded stream of mixed
 *        AVL insert/delete operations under a shrinking per-operation window.
 */
class EndlessSession {
private:
    unsigned int initialSeed;
    unsigned int rngState;
    float survivalTimeSec;
    float opWindowSec;
    float opTimeRemaining;
    int opsCompleted;
    EndlessOp pendingOp;
    bool active;
    bool gameOver;
    std::string gameOverReason;

    unsigned int nextRand();
    EndlessOp generateNextOp(const AVLTree& tree);

public:
    EndlessSession();

    void startNew(unsigned int seed, AVLTree& tree);

    // Advances elapsed survival timer and operation deadline; returns true when game-over triggers.
    bool tick(float dt);

    // Applies the pending INSERT/DELETE to tree using real AVLTree methods and runs the invariant check.
    bool executePending(AVLTree& tree, EventBatch& outBatch);

    // Persists seed, RNG state, survival time, deadline, pending op, and preorder+null AVL tree.
    bool saveToFile(const std::string& filePath, const AVLTree& tree) const;

    // Restores an in-progress Endless Mode session from filePath and validates tree invariants.
    bool loadFromFile(const std::string& filePath, AVLTree& tree);

    static float computeWindowFor(float survivalSec, int opsDone);
    static float computeOpsPerMin(float windowSec);
    static bool validateTreeFull(const AVLTree& tree);

    unsigned int getInitialSeed() const { return initialSeed; }
    unsigned int getRngState() const { return rngState; }
    float getSurvivalTimeSec() const { return survivalTimeSec; }
    int getSurvivalMs() const {
        return (survivalTimeSec <= 0.f) ? 0 : static_cast<int>(survivalTimeSec * 1000.0f + 0.5f);
    }
    float getOpWindowSec() const { return opWindowSec; }
    float getOpTimeRemaining() const { return opTimeRemaining; }
    int getOpsCompleted() const { return opsCompleted; }
    const EndlessOp& getPendingOp() const { return pendingOp; }
    bool isActive() const { return active; }
    bool isGameOver() const { return gameOver; }
    const std::string& getGameOverReason() const { return gameOverReason; }
    void stop() { active = false; }
};

} // namespace VaultHeist
