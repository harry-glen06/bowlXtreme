#pragma once

// Simple "Xtreme" scoring + progression.
// - Each round has 2 frames (4 shots total)
// - Each frame has 2 shots
// - Shot score: (baseImpact + sum(pin values hit)) * (pinsHit + baseCombo)

class XtremeScorer {
public:
    void reset();

    // Pass in how many pins were knocked this shot and the sum of their values
    void recordShot(int pinsHit, int pinValueSum);

    // Progress
    int getRound() const { return round; }
    int getFrameInRound() const { return frameInRound; } // 1-2
    int getShotInFrame() const { return shotInFrame; }   // 1-2

    // Scores
    int getRoundScore() const { return roundScore; }
    int getTargetScore() const { return targetScore; }

    bool isGameOver() const { return lost; }

    // Last shot breakdown (for HUD)
    int getLastImpact() const { return lastImpact; }
    int getLastCombo() const { return lastCombo; }
    int getLastShotScore() const { return lastShotScore; }
    int getLastPinsHit() const { return lastPinsHit; }
    int getLastPinValueSum() const { return lastPinValueSum; }

    // Tuning
    void setBaseImpact(int v) { baseImpact = v; }
    void setBaseCombo(int v) { baseCombo = v; }
    void setTargetStart(int v) { targetStart = v; }
    void setTargetIncrease(int v) { targetIncrease = v; }

private:
    int round = 1;
    int frameInRound = 1;
    int shotInFrame = 1;

    int roundScore = 0;

    bool lost = false;

    int baseImpact = 10;
    int baseCombo = 1;

    int targetStart = 600;
    int targetIncrease = 200;
    int targetScore = 600;

    // last shot
    int lastImpact = 10;
    int lastCombo = 1;
    int lastShotScore = 10;
    int lastPinsHit = 0;
    int lastPinValueSum = 0;
};
