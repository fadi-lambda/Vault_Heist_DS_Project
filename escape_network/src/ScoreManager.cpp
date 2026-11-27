#include "ScoreManager.h"
#include <cmath>

namespace EscapeNetwork {

int ScoreManager::computeRouteHeatCapacity(double optimalPathCost) {
    if (optimalPathCost < 0.0) optimalPathCost = 0.0;
    return static_cast<int>(std::ceil(1.5 * optimalPathCost)) + 2;
}

int ScoreManager::calculateRouteStars(double optimalCost, double playerCost,
                                      int currentHeat, int heatCapacity,
                                      bool validResult) const {
    if (!validResult || currentHeat > heatCapacity) return 0;
    if (playerCost <= optimalCost + 1e-6 && mistakeCount == 0) return 3;
    if (playerCost <= 1.25 * optimalCost + 1e-6 && mistakeCount <= 1) return 2;
    return 1;
}

int ScoreManager::calculateNonRouteStars(bool validResult, int mistakeBudget) const {
    if (!validResult || mistakeCount > mistakeBudget) return 0;
    if (mistakeCount == 0) return 3;
    if (mistakeCount <= 1) return 2;
    return 1;
}

int ScoreManager::calculateStars(int optimalMetric, int playerMetric) const {
    if (playerMetric <= 0 && optimalMetric > 0) return 0;
    if (playerMetric <= optimalMetric && mistakeCount == 0) return 3;
    if (playerMetric <= static_cast<int>(std::ceil(1.25 * optimalMetric)) && mistakeCount <= 1) return 2;
    return 1;
}

void ScoreManager::addHeat(int amount) {
    if (amount > 0) heatUsed += amount;
}

void ScoreManager::recordMistake(int heatPenalty) {
    ++mistakeCount;
    if (heatPenalty > 0) heatUsed += heatPenalty;
}

void ScoreManager::reset() {
    mistakeCount = 0;
    heatUsed = 0;
}

} // namespace EscapeNetwork
