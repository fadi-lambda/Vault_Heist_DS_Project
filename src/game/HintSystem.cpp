#include "game/HintSystem.h"

namespace VaultHeist {

std::string HintSystem::pick(const LevelData& data, int stage) {
    if (data.hintCount <= 0) return "Re-read the objective and look for the node the rule points at.";
    if (stage < 0) stage = 0;
    if (stage >= data.hintCount) stage = data.hintCount - 1;
    return data.hints[stage];
}

bool HintSystem::use(ScoringEngine& scoring, const LevelData& data, int stage) {
    if (used) return false;
    used = true;
    scoring.applyHint();
    shown = pick(data, stage);
    return true;
}

} // namespace VaultHeist
