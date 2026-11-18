#pragma once

namespace VaultHeist {

// Penalty kinds (static const int instead of enum, per course policy)
static const int PENALTY_WRONG_TRAVERSAL = 0;  // wrong traversal / selection click: +1 alarm
static const int PENALTY_WRONG_ROTATION  = 1;  // wrong rotation (or pivot) choice:   +2 alarm, -50 points
static const int PENALTY_ACT4_INVALID    = 2;  // bad parent-child compare / invalid swap in Act 4: +1 alarm

static const int SCORE_BASE              = 1000;
static const int SCORE_PER_ALARM_STEP    = 10;
static const int SCORE_ROTATION_PENALTY  = 50;
static const int STARS_HINT_CAP          = 2;

// Single reusable scoring / alarm engine (Project.pdf §2.2 / §2.4).
// One instance tracks the level currently being played; every level uses it.
class ScoringEngine {
public:
    ScoringEngine();

    // Alarm capacity = ceil(1.5 * optimalSteps) + 2
    static int capacityFor(int optimalSteps);

    // Pure star computation, exposed so it can be unit-tested and reused.
    //  steps     : valid progress steps the player has taken
    //  penalties : number of penalty events (hints are NOT penalties; they only cap stars)
    static int starsFor(int steps, int optimalSteps, int penaltyCount,
                        bool hintUsed, int alarmSteps, int alarmCapacity);
    // Pure score: SCORE_BASE - 10*alarmSteps - penaltyPoints + bonus, floored at 0.
    static int scoreFor(int alarmSteps, int penaltyPoints, int bonus);

    // Start a new level attempt. optimalSteps must be computed from the level data.
    void begin(int optimalSteps);

    // A valid progress step: +n steps and +n alarm.
    void addStep(int n = 1);
    // Apply one of the PENALTY_* kinds. Returns the alarm added.
    int applyPenalty(int kind);
    // Hint used: caps the level at STARS_HINT_CAP stars. Returns false if already used.
    bool applyHint();
    void addBonus(int points);

    // Restore mutable counters from an undo snapshot.
    void restoreState(int alarmSteps, int stepCount, int penaltyCount,
                      int penaltyPts, int bonus, bool hintUsed);

    bool isAlarmFull() const { return alarmSteps >= alarmCapacity; }
    int  stars() const;
    int  score() const;

    int optimal() const        { return optimalSteps; }
    int capacity() const       { return alarmCapacity; }
    int alarm() const          { return alarmSteps; }
    int steps() const          { return stepCount; }
    int penaltyEvents() const  { return penaltyCount; }
    int penaltyPoints() const  { return penaltyPts; }
    int bonusPoints() const    { return bonus; }
    bool hintWasUsed() const   { return hintUsed; }

private:
    int optimalSteps;
    int alarmCapacity;
    int alarmSteps;
    int stepCount;
    int penaltyCount;
    int penaltyPts;
    int bonus;
    bool hintUsed;
};

} // namespace VaultHeist
