#include "XtremeScorer.h"

void XtremeScorer::reset() {
    round = 1;
    frameInRound = 1;
    shotInFrame = 1;

    roundScore = 0;
    
    tokenCounter = 0;

    lost = false;

    targetScore = targetStart;

    lastImpact = baseImpact;
    lastCombo = baseCombo;
    lastShotScore = baseImpact * baseCombo;
    lastPinsHit = 0;
    lastPinValueSum = 0;
    roundPassed = false;
}

void XtremeScorer::recordShot(int pinsHit, int pinValueSum) {
    if (lost) return;

    if (pinsHit < 0) pinsHit = 0;
    if (pinValueSum < 0) pinValueSum = 0;

    lastPinsHit = pinsHit;
    lastPinValueSum = pinValueSum;

    lastImpact = baseImpact + pinValueSum;
    lastCombo = pinsHit + baseCombo;
    lastShotScore = lastImpact * lastCombo;

    roundScore += lastShotScore;

    // mark round as passed if target reached
    if (roundScore >= targetScore) {
        roundPassed = true;
    }

    // advance shot
    if (shotInFrame == 1) {
        shotInFrame = 2;
        return;
    }

    // finished shot 2
    shotInFrame = 1;

    // =====================
    // FRAME 1 COMPLETE
    // =====================
    if (frameInRound == 1) {

        // passed early -> skip frame 2
        if (roundPassed) {

            // token reward
            int interest = tokenCounter / 3;
            int reward = 6;     // 3 base + 3 bonus for early clear
            tokenCounter += interest + reward;

            shopReady = true; 

            round++;
            targetScore += targetIncrease;
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
    int interest = tokenCounter / 3;
    int reward = 3;     // normal clear reward
    tokenCounter += interest + reward;

    shopReady = true; 

    round++;
    targetScore += targetIncrease;
    roundScore = 0;
    roundPassed = false;
}
