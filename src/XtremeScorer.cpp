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

    // Advance shot/frame/round
    if (shotInFrame == 1) {
        shotInFrame = 2;
        return;
    }

    // shot 2 completed
    shotInFrame = 1;

    if (frameInRound == 1) {
        frameInRound = 2;
        return;
    }

    // Round complete (2 frames)
    frameInRound = 1;

    // Lose condition: you must meet the target by the end of the round
    if (roundScore < targetScore) lost = true;

    // Passed the round
    round++;
    targetScore += targetIncrease;
    roundScore = 0;
}
