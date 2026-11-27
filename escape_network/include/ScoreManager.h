#pragma once

namespace EscapeNetwork {

class ScoreManager {
public:
    ScoreManager() = default;

    // Placeholder star calculation — real per-mission-type formulas will be
    // implemented once route-based missions are introduced in Act 2.
    int calculateStars(int optimalMetric, int playerMetric) const;

    void recordMistake();
    void reset();

    int getMistakeCount() const;

private:
    int mistakeCount = 0;
};

} // namespace EscapeNetwork
