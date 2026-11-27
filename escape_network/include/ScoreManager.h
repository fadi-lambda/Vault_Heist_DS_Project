#pragma once

namespace EscapeNetwork {

class ScoreManager {
public:
    ScoreManager() = default;

    // Route mission heat capacity (§2.2): ceil(1.5 * optimalCost) + 2
    // Applies ONLY to route missions with non-negative travel costs.
    static int computeRouteHeatCapacity(double optimalPathCost);

    // Route mission star award (§2.3):
    //   3 stars: valid optimal result (cost <= optimal) with 0 mistakes
    //   2 stars: valid result within 1.25 * optimalCost with <= 1 mistake
    //   1 star : valid completion before heat/mistake budget exhausted
    //   0 stars: invalid result or budget exhausted
    int calculateRouteStars(double optimalCost, double playerCost,
                            int heatUsed, int heatCapacity, bool validResult) const;

    // Non-route mission star award (§2.3):
    // Based strictly on correctness and mistakes (never compares unlike metrics).
    int calculateNonRouteStars(bool validResult, int mistakeBudget) const;

    // Compatibility wrapper for general metric comparison
    int calculateStars(int optimalMetric, int playerMetric) const;

    void addHeat(int amount);
    void recordMistake(int heatPenalty = 1);
    void reset();

    int getMistakeCount() const { return mistakeCount; }
    int getHeatUsed() const { return heatUsed; }

private:
    int mistakeCount = 0;
    int heatUsed = 0;
};

} // namespace EscapeNetwork
