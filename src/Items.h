#pragma once
#include <vector>

enum class BallType {
    Normal,
    BlackHole,   // Pins pulled toward ball; ball 8% smaller
    Midas,       // Hit pins become gold (score gold tokens)
    Upgrade,     // Each pin hit gains +1 value; ball 10% lighter, 5% faster
    Heavy,       // Ball 15% heavier, easier to knock pins
    Fastball,    // Ball 5% lighter, 15% faster
    OddBall,     // Odd-value pins score double, even-value pins score half
    EightBall,   // All pins worth 8
    Retrigger,   // 2nd pin hit is scored 3x total
};

struct ActiveItems {
    BallType ballType = BallType::Normal;

    // Derived stat multipliers (recalculated when ball changes)
    float radiusMultiplier  = 1.0f;
    float speedMultiplier   = 1.0f;
    float massMultiplier    = 1.0f;   // applied to ball collision mass

    // Per-shot state (reset each ball launch)
    int   pinsHitThisShot  = 0;
    bool  retriggered      = false;
    int   retriggeredValue = 0;       // value added to score at shot end

    // Midas: which pin indices are gold this round
    std::vector<int> goldPinIndices;

    void resetForNewShot() {
        pinsHitThisShot  = 0;
        retriggered      = false;
        retriggeredValue = 0;
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
