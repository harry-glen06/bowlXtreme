#include "XtremeScorer.h"
#include <cmath>

void XtremeScorer::reset() {
    round = 1;
    frameInRound = 1;
    shotInFrame = 1;
    shotsPerFrame = 2;
    totalShots = 0;

    roundScore = 0;
    
    tokenCounter = 0;

    lost = false;

    targetScore = targetStart;

    lastImpact = baseImpact;
    lastCombo = static_cast<int>(std::lround(baseCombo));
    lastShotScore = static_cast<int>(std::lround(baseImpact * baseCombo));
    lastPinsHit = 0;
    lastPinValueSum = 0;
    roundPassed = false;
    powerPassedGo = false;
    powerMoMoney = false;
}

void XtremeScorer::recordShot(int pinsHit, int pinValueSum, bool strike) {
    if (lost) return;

    if (pinsHit < 0) pinsHit = 0;
    if (pinValueSum < 0) pinValueSum = 0;
    totalShots++;

    lastPinsHit = pinsHit;
    lastPinValueSum = pinValueSum;

    lastImpact = baseImpact + pinValueSum;
    float comboValue = static_cast<float>(pinsHit) + baseCombo;
    lastCombo = static_cast<int>(std::lround(comboValue));
    lastShotScore = static_cast<int>(std::lround(static_cast<float>(lastImpact) * comboValue));
    if (strike) {
        int strikeBonus = (lastShotScore * 40) / 100;
        lastShotScore += strikeBonus;
    }

    roundScore += lastShotScore;

    // mark round as passed if target reached
    if (roundScore >= targetScore) {
        roundPassed = true;
    }

    // advance shot
    if (shotInFrame < shotsPerFrame) {
        shotInFrame++;
        return;
    }

    auto nextTargetFromPercent = [&]() {
        int pct = targetIncreasePercent;
        if (pct < 1) pct = 1;
        float scaled = static_cast<float>(targetScore) * (100.0f + static_cast<float>(pct)) / 100.0f;
        int snapped = static_cast<int>(std::round(scaled / 25.0f)) * 25;
        if (snapped <= targetScore) snapped = targetScore + 25;
        targetScore = snapped;
    };

    // finished final shot in frame
    shotInFrame = 1;

    // =====================
    // FRAME 1 COMPLETE
    // =====================
    if (frameInRound == 1) {

        // passed early -> skip frame 2
        if (roundPassed) {

            // token reward
            int interestDivisor = powerMoMoney ? 2 : 3;
            int interest = tokenCounter / interestDivisor;
            int reward = 6;     // 3 base + 3 bonus for early clear
            if (powerPassedGo) reward += 3;
            tokenCounter += interest + reward;

            shopReady = true; 

            round++;
            nextTargetFromPercent();
            roundScore = 0;

            frameInRound = 1;
            roundPassed = false;
            return;
        }

        // otherwise go to frame 2
        frameInRound = 2;
        return;
    }

    // =====================
    // FRAME 2 COMPLETE
    // =====================

    frameInRound = 1;

    // failed round
    if (!roundPassed) {
        lost = true;
        return;
    }

    // passed round normally
    int interestDivisor = powerMoMoney ? 2 : 3;
    int interest = tokenCounter / interestDivisor;
    int reward = 3;     // normal clear reward
    if (powerPassedGo) reward += 3;
    tokenCounter += interest + reward;

    shopReady = true; 

    round++;
    nextTargetFromPercent();
    roundScore = 0;
    roundPassed = false;
}
