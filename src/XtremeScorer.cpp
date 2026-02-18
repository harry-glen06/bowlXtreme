#include "XtremeScorer.h"

void XtremeScorer::reset() {
    round = 1;
    frameInRound = 1;
    shotInFrame = 1;

    roundScore = 0;

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

    // If we ever hit the target, lock it in for this round
    if (roundScore >= targetScore) {
        roundPassed = true;
    }

    // Advance shot
    if (shotInFrame == 1) {
        shotInFrame = 2;
        return;
    }

    // Shot 2 completed, move to next frame or round
    shotInFrame = 1;

    // Finished frame 1
    if (frameInRound == 1) {
        // If we already passed, skip frame 2 and start next round
        if (roundPassed) {
            round++;
            targetScore += targetIncrease;
            roundScore = 0;
            frameInRound = 1;
            shotInFrame = 1;
            roundPassed = false;
            return;
        }

        // Otherwise go to frame 2
        frameInRound = 2;
        return;
    }

    // Finished frame 2 => round complete
    frameInRound = 1;

    // Only lose if we never passed
    if (!roundPassed) {
        lost = true;
        return;
    }

    // Passed the round
    round++;
    targetScore += targetIncrease;
    roundScore = 0;
    roundPassed = false;
}

