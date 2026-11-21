#include "game/Scoring.h"

namespace VaultHeist {

ScoringEngine::ScoringEngine()
    : optimalSteps(1), alarmCapacity(capacityFor(1)), alarmSteps(0), stepCount(0),
      penaltyCount(0), penaltyPts(0), bonus(0), hintUsed(false) {}

int ScoringEngine::capacityFor(int optimalSteps) {
    if (optimalSteps < 0) optimalSteps = 0;
    // ceil(1.5 * n) = (3n + 1) / 2 in integer arithmetic
    return (3 * optimalSteps + 1) / 2 + 2;
}

int ScoringEngine::starsFor(int steps, int optimalSteps, int penaltyCount,
                            bool hintUsed, int alarmSteps, int alarmCapacity) {
    if (alarmSteps >= alarmCapacity) return 0;          // alarm full: retry
    int stars = 1;                                      // completed within capacity
    if (steps * 4 <= optimalSteps * 5 && penaltyCount <= 1) stars = 2;  // <= 1.25 * optimal
    if (steps <= optimalSteps && penaltyCount == 0) stars = 3;
    if (hintUsed && stars > STARS_HINT_CAP) stars = STARS_HINT_CAP;
    return stars;
}

int ScoringEngine::scoreFor(int alarmSteps, int penaltyPoints, int bonus) {
    int s = SCORE_BASE - SCORE_PER_ALARM_STEP * alarmSteps - penaltyPoints + bonus;
    return s < 0 ? 0 : s;
}

void ScoringEngine::begin(int optimal) {
    optimalSteps = optimal < 1 ? 1 : optimal;
    alarmCapacity = capacityFor(optimalSteps);
    alarmSteps = 0;
    stepCount = 0;
    penaltyCount = 0;
    penaltyPts = 0;
    bonus = 0;
    hintUsed = false;
}

void ScoringEngine::setCapacity(int newCapacity) {
    alarmCapacity = newCapacity < 1 ? 1 : newCapacity;
}

void ScoringEngine::addStep(int n) {
    if (n <= 0) return;
    stepCount += n;
    alarmSteps += n;
}

int ScoringEngine::applyPenalty(int kind) {
    int alarmAdded = 1;
    if (kind == PENALTY_WRONG_ROTATION) {
        alarmAdded = 2;
        penaltyPts += SCORE_ROTATION_PENALTY;
    }
    alarmSteps += alarmAdded;
    ++penaltyCount;
    return alarmAdded;
}

bool ScoringEngine::applyHint() {
    if (hintUsed) return false;
    hintUsed = true;
    return true;
}

void ScoringEngine::addBonus(int points) {
    if (points > 0) bonus += points;
}

void ScoringEngine::restoreState(int alarm, int steps, int penalties,
                                 int penPts, int bonusPts, bool hint) {
    alarmSteps = alarm;
    stepCount = steps;
    penaltyCount = penalties;
    penaltyPts = penPts;
    bonus = bonusPts;
    hintUsed = hint;
}

int ScoringEngine::stars() const {
    return starsFor(stepCount, optimalSteps, penaltyCount, hintUsed, alarmSteps, alarmCapacity);
}

int ScoringEngine::score() const {
    return scoreFor(alarmSteps, penaltyPts, bonus);
}

} // namespace VaultHeist
