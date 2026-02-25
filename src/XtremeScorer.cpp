#include "XtremeScorer.h"
#include <algorithm>
#include <cmath>

void XtremeScorer::refreshLastPreviewFromBase() {
    lastImpact = baseImpact;
    lastCombo = static_cast<int>(std::lround(baseCombo));
    lastShotScore = static_cast<int>(std::lround(static_cast<float>(lastImpact) * baseCombo));
}

void XtremeScorer::setBaseImpact(int v) {
    if (v < 0) v = 0;
    baseImpact = v;
    refreshLastPreviewFromBase();
}

void XtremeScorer::setBaseCombo(float v) {
    if (v < 0.0f) v = 0.0f;
    baseCombo = v;
    refreshLastPreviewFromBase();
}

void XtremeScorer::setRoundTargetMultiplier(float multiplier) {
    if (multiplier < 1.0f) multiplier = 1.0f;
    if (std::abs(multiplier - roundTargetMultiplier) < 0.0001f) return;

    float baseTarget = static_cast<float>(targetScore) / std::max(1.0f, roundTargetMultiplier);
    int normalizedBase = static_cast<int>(std::round(baseTarget / 25.0f)) * 25;
    if (normalizedBase < 25) normalizedBase = 25;

    int adjusted = static_cast<int>(std::round(static_cast<float>(normalizedBase) * multiplier / 25.0f)) * 25;
    if (adjusted < 25) adjusted = 25;

    roundTargetMultiplier = multiplier;
    targetScore = adjusted;
    roundPassed = (roundScore >= targetScore);
}

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

    refreshLastPreviewFromBase();
    lastPinsHit = 0;
    lastPinValueSum = 0;
    roundPassed = false;
    powerPassedGo = false;
    powerMoMoney = false;
    roundTargetMultiplier = 1.0f;
    roundClearRewardBonus = 0;
}

void XtremeScorer::recordShot(int pinsHit, int pinValueSum, bool strike, bool spare,
                              float comboMultiplier, int comboAdd) {
    if (lost) return;

    if (pinsHit < 0) pinsHit = 0;
    if (pinValueSum < 0) pinValueSum = 0;
    if (comboMultiplier < 1.0f) comboMultiplier = 1.0f;
    if (comboAdd < 0) comboAdd = 0;
    totalShots++;

    lastPinsHit = pinsHit;
    lastPinValueSum = pinValueSum;

    lastImpact = baseImpact + pinValueSum;
    float comboValue = (static_cast<float>(pinsHit + comboAdd) + baseCombo) * comboMultiplier;
    lastCombo = static_cast<int>(std::lround(comboValue));
    lastShotScore = static_cast<int>(std::lround(static_cast<float>(lastImpact) * comboValue));
    if (strike) {
        int strikeBonus = (lastShotScore * 40) / 100;
        lastShotScore += strikeBonus;
    } else if (spare) {
        int spareBonus = (lastShotScore * spareBonusPercent) / 100;
        lastShotScore += spareBonus;
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

    auto normalizeRoundTarget = [&]() {
        if (roundTargetMultiplier <= 1.0f) return;
        float baseTarget = static_cast<float>(targetScore) / roundTargetMultiplier;
        int normalizedBase = static_cast<int>(std::round(baseTarget / 25.0f)) * 25;
        if (normalizedBase < 25) normalizedBase = 25;
        targetScore = normalizedBase;
        roundTargetMultiplier = 1.0f;
    };

    auto nextTargetFromPercent = [&]() {
        int pct = targetIncreasePercent;
        if (round >= 15) {
            pct = targetIncreasePercentRound15Plus;
        } else if (round >= 10) {
            pct = targetIncreasePercentRound10Plus;
        }
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
            if (interest > interestCap) interest = interestCap;
            int reward = 6;     // 3 base + 3 bonus for early clear
            if (powerPassedGo) reward += 3;
            reward += roundClearRewardBonus;
            tokenCounter += interest + reward;

            shopReady = true; 

            normalizeRoundTarget();
            roundClearRewardBonus = 0;
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
    if (interest > interestCap) interest = interestCap;
    int reward = 3;     // normal clear reward
    if (powerPassedGo) reward += 3;
    reward += roundClearRewardBonus;
    tokenCounter += interest + reward;

    shopReady = true; 

    normalizeRoundTarget();
    roundClearRewardBonus = 0;
    round++;
    nextTargetFromPercent();
    roundScore = 0;
    roundPassed = false;
}

void XtremeScorer::debugAdvanceRounds(int roundsToAdvance) {
    if (lost) return;
    if (roundsToAdvance <= 0) return;

    auto normalizeRoundTarget = [&]() {
        if (roundTargetMultiplier <= 1.0f) return;
        float baseTarget = static_cast<float>(targetScore) / roundTargetMultiplier;
        int normalizedBase = static_cast<int>(std::round(baseTarget / 25.0f)) * 25;
        if (normalizedBase < 25) normalizedBase = 25;
        targetScore = normalizedBase;
        roundTargetMultiplier = 1.0f;
    };

    auto nextTargetFromPercent = [&]() {
        int pct = targetIncreasePercent;
        if (round >= 15) {
            pct = targetIncreasePercentRound15Plus;
        } else if (round >= 10) {
            pct = targetIncreasePercentRound10Plus;
        }
        if (pct < 1) pct = 1;
        float scaled = static_cast<float>(targetScore) * (100.0f + static_cast<float>(pct)) / 100.0f;
        int snapped = static_cast<int>(std::round(scaled / 25.0f)) * 25;
        if (snapped <= targetScore) snapped = targetScore + 25;
        targetScore = snapped;
    };

    for (int i = 0; i < roundsToAdvance; i++) {
        normalizeRoundTarget();
        roundClearRewardBonus = 0;
        shopReady = false;
        roundPassed = false;
        roundScore = 0;
        frameInRound = 1;
        shotInFrame = 1;
        round++;
        nextTargetFromPercent();
    }

    lastPinsHit = 0;
    lastPinValueSum = 0;
    refreshLastPreviewFromBase();
}
