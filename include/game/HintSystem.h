#pragma once
#include "game/LevelData.h"
#include "game/Scoring.h"
#include <string>

namespace VaultHeist {

// One reusable hint button's backing logic: available once per level.
// Using it shows one short nudge (never the answer) and caps the level at 2 stars
// through ScoringEngine::applyHint().
class HintSystem {
public:
    HintSystem() : used(false) {}

    void reset() { used = false; shown.clear(); }
    bool isAvailable() const { return !used; }
    bool wasUsed() const { return used; }
    const std::string& shownText() const { return shown; }

    // Picks the nudge for the level's current stage (clamped to the last hint).
    static std::string pick(const LevelData& data, int stage);

    // Consumes the hint. Returns false (and changes nothing) if already used.
    bool use(ScoringEngine& scoring, const LevelData& data, int stage);

private:
    bool used;
    std::string shown;
};

} // namespace VaultHeist
