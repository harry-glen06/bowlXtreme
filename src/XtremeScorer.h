#pragma once

// Simple "Xtreme" scoring + progression.
// - Each round has 2 frames (4 or 6 shots total with Extra Ball)
// - Each frame has 2 shots (or 3 with Extra Ball power)
// - Shot score: (baseImpact + sum(pin values hit)) * (pinsHit + baseCombo)

class XtremeScorer {
public:
    void reset();

    // Pass in how many pins were knocked this shot and the sum of their values.
    // comboMultiplier scales the combo term ((pinsHit + baseCombo + comboAdd) * comboMultiplier).
    // If strike is true, this shot gains +40% bonus points.
    void recordShot(int pinsHit, int pinValueSum, bool strike = false, float comboMultiplier = 1.0f, int comboAdd = 0);

    // Progress
    int getRound() const { return round; }
    int getFrameInRound() const { return frameInRound; } // 1-2
    int getShotInFrame() const { return shotInFrame; }   // 1..shotsPerFrame
    int getShotsPerFrame() const { return shotsPerFrame; }
    int getTotalShots() const { return totalShots; }

    // Tokens
    int getTokens() const { return tokenCounter; }
    void addTokens(int amount) { tokenCounter += amount; }

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
    void setBaseCombo(float v) { baseCombo = v; }
    void setTargetStart(int v) { targetStart = v; }
    void setTargetIncrease(int v) { targetIncreasePercent = v; } // percent
    void setTargetIncreasePercent(int v) { targetIncreasePercent = v; }
    void setExtraBallEnabled(bool enabled) { shotsPerFrame = enabled ? 3 : 2; }
    void setPowerPassedGo(bool enabled) { powerPassedGo = enabled; }
    void setPowerMoMoney(bool enabled) { powerMoMoney = enabled; }

    // Shop
    bool shopReady = false;
    bool isShopReady() const { return shopReady; }
    void consumeShopReady() { shopReady = false; }

private:
    int round = 1;
    int frameInRound = 1;
    int shotInFrame = 1;
    int shotsPerFrame = 2;
    int totalShots = 0;

    int roundScore = 0;

    bool lost = false;

    int baseImpact = 10;
    float baseCombo = 1.0f;

    int targetStart = 400;
    int targetIncreasePercent = 25;
    int targetScore = 400;

    // last shot
    int lastImpact = 10;
    int lastCombo = 1;
    int lastShotScore = 10;
    int lastPinsHit = 0;
    int lastPinValueSum = 0;

    int tokenCounter = 0;

    bool roundPassed = false;
    bool powerPassedGo = false;
    bool powerMoMoney = false;
};
