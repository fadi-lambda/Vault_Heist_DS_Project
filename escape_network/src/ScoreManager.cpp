#include "ScoreManager.h"

namespace EscapeNetwork {

int ScoreManager::calculateStars(int /*optimalMetric*/, int /*playerMetric*/) const {
    return 0;
}

void ScoreManager::recordMistake() {
    ++mistakeCount;
}

void ScoreManager::reset() {
    mistakeCount = 0;
}

int ScoreManager::getMistakeCount() const {
    return mistakeCount;
}

} // namespace EscapeNetwork
