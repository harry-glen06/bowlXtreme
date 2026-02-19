#pragma once
#include <vector>

enum class BallType {
    Normal,
    BlackHole,
    Midas,
    Upgrade,
    Heavy,
    Fastball,
    OddBall,
    EightBall,
    Retrigger,
};

struct ActiveItems {
    BallType ballType = BallType::Normal;

    float radiusMultiplier  = 1.0f;
    float speedMultiplier   = 1.0f;
    float massMultiplier    = 1.0f;

    // Per-shot state
    int   pinsHitThisShot  = 0;
    bool  retriggered      = false;
    int   retriggeredValue = 0;
    int   firstBallHitPinIndex = -1;
    std::vector<int> goldPinIndices;
    int   thirdTimeComboBonus = 0;

    // Purchased pin types — one pin per type gets assigned each new frame
    // e.g. if player bought Gold and Exploding, one pin will be Gold and one Exploding each frame
    std::vector<int> purchasedPinTypes;  // stored as int (cast from PinType)

    void resetForNewShot() {
        pinsHitThisShot        = 0;
        retriggered            = false;
        retriggeredValue       = 0;
        firstBallHitPinIndex   = -1;
        thirdTimeComboBonus    = 0;
    }

    void resetAll() {
        resetForNewShot();
        goldPinIndices.clear();
        purchasedPinTypes.clear();
        ballType        = BallType::Normal;
        radiusMultiplier = 1.0f;
        speedMultiplier  = 1.0f;
        massMultiplier   = 1.0f;
    }

    void applyBallType(BallType type) {
        ballType = type;
        radiusMultiplier = 1.0f;
        speedMultiplier  = 1.0f;
        massMultiplier   = 1.0f;

        switch (type) {
            case BallType::BlackHole:
                radiusMultiplier = 0.92f;
                break;
            case BallType::Upgrade:
                massMultiplier  = 0.90f;
                speedMultiplier = 1.05f;
                break;
            case BallType::Heavy:
                massMultiplier  = 1.15f;
                break;
            case BallType::Fastball:
                massMultiplier  = 0.95f;
                speedMultiplier = 1.15f;
                break;
            default:
                break;
        }
    }
};
