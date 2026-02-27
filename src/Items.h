#pragma once
#include <algorithm>
#include <array>
#include <vector>
#include "Pin.h"

enum class BallType {
    Normal,
    BlackHole,
    Midas,
    Upgrade,
    Heavy,
    Fastball,
    OddBall,
    EightBall,
    Icy,
    Retrigger,
};

enum class ShoeType {
    None,
    Clown,
    Running,
    Moon,
    Slippers,
    HighHeels,
    SteelCap,
};

enum class PowerType {
    Greedy,
    RandomUpgrade,
    ExtraPins,
    ExtraBall,
    Duplicate,
    Bumpers,
    SwapPins,
    HomeBase,
    Confusion,
    Earthquake,
    Skip,
    UpgradesForEveryone,
    Sales,
    PassedGo,
    MoMoney,
    SevenEightNine,
    ExtraPowerSlot,
};

inline int getBallTokenCost(BallType t) {
    switch (t) {
        case BallType::BlackHole:  return 4;
        case BallType::Midas:      return 5;
        case BallType::Upgrade:    return 3;
        case BallType::Heavy:      return 2;
        case BallType::Fastball:   return 2;
        case BallType::OddBall:    return 3;
        case BallType::EightBall:  return 4;
        case BallType::Icy:        return 4;
        case BallType::Retrigger:  return 4;
        default:                   return 1;
    }
}

inline int getPinTokenCost(PinType t) {
    switch (t) {
        case PinType::Gold:         return 2;
        case PinType::Mischievous:  return 2;
        case PinType::Exploding:    return 5;
        case PinType::Light:        return 2;
        case PinType::Big:          return 3;
        case PinType::Ice:          return 2;
        case PinType::CopyCat:      return 3;
        case PinType::LuckyDucky:   return 4;
        case PinType::LevelUp:      return 1;
        case PinType::Lover:        return 3;
        case PinType::ChangeIsGood: return 3;
        case PinType::ThirdTime:    return 3;
        default:                    return 1;
    }
}

inline int getShoeTokenCost(ShoeType t) {
    switch (t) {
        case ShoeType::Clown:     return 0;
        case ShoeType::Running:   return 3;
        case ShoeType::Moon:      return 4;
        case ShoeType::Slippers:  return 3;
        case ShoeType::HighHeels: return 3;
        case ShoeType::SteelCap:  return 4;
        default:                  return 0;
    }
}

inline int getPowerTokenCost(PowerType t) {
    switch (t) {
        case PowerType::Greedy:              return 5;
        case PowerType::RandomUpgrade:       return 3;
        case PowerType::ExtraPins:           return 5;
        case PowerType::ExtraBall:           return 7;
        case PowerType::Duplicate:           return 2;
        case PowerType::Bumpers:             return 5;
        case PowerType::SwapPins:            return 2;
        case PowerType::HomeBase:            return 5;
        case PowerType::Confusion:           return 3;
        case PowerType::Earthquake:          return 6;
        case PowerType::Skip:                return 1;
        case PowerType::UpgradesForEveryone: return 4;
        case PowerType::Sales:               return 4;
        case PowerType::PassedGo:            return 4;
        case PowerType::MoMoney:             return 4;
        case PowerType::SevenEightNine:      return 3;
        case PowerType::ExtraPowerSlot:      return 4;
        default:                             return 0;
    }
}

struct ActiveItems {
    struct PinSlotAssignment {
        int slot = 1; // 1-based spawn-order slot
        PinType type = PinType::Normal;
    };

    // Ball currently active for this shot (set by Game::resetBall)
    BallType ballType = BallType::Normal;
    // Persistent loadout: shot 1 uses slot 1, shot 2 uses slot 2,
    // and shot 3 uses slot 3 when Extra Ball is active.
    BallType ballSlot1 = BallType::Normal;
    BallType ballSlot2 = BallType::Normal;
    BallType ballSlot3 = BallType::Normal;
    ShoeType shoeType  = ShoeType::None;
    std::vector<int> purchasedPowers; // cast from PowerType

    float radiusMultiplier  = 1.0f;
    float speedMultiplier   = 1.0f;
    float massMultiplier    = 1.0f;
    float launchSpeedMultiplier = 1.0f; // shoes
    float pinMassMultiplier     = 1.0f; // shoes
    float slideMultiplier       = 1.0f; // shoes
    bool  lockPinChangesMidRound = false; // shoes

    // Powers
    bool powerGreedy = false;
    bool powerRandomUpgrade = false;
    bool powerExtraPins = false;
    bool powerExtraBall = false;
    bool powerBumpers = false;
    bool powerHomeBase = false;
    bool powerConfusion = false;
    bool powerEarthquake = false;
    bool powerSkip = false;
    bool powerUpgradesForEveryone = false;
    bool powerSales = false;
    bool powerPassedGo = false;
    bool powerMoMoney = false;
    bool powerExtraPowerSlot = false;
    bool clownBonusClaimed = false;
    bool clownShoesPurchased = false;
    int clownRoundsRemaining = 0;

    int duplicateCharges = 0;
    int swapCharges = 0;
    int sevenEightNineCharges = 0;
    bool sevenEightNinePin9Destroyed = false;
    int skipCharges = 0;
    int earthquakeShotCounter = 0;
    int pendingRandomPinUpgrades = 0;
    float homeBaseComboBonus = 0.0f;
    int homeBasePinsTowardNextCombo = 0;
    int thirdTimeGlobalKnocks = 0;
    int pinsStandingAtShotStart = 10;

    // Per-shot state
    int   pinsHitThisShot  = 0;
    bool  retriggered      = false;
    int   retriggeredValue = 0;
    int   firstBallHitPinIndex = -1;
    std::vector<int> goldPinIndices;
    int   thirdTimeComboBonus = 0;
    int   pinChangeEventsThisShot = 0;
    std::array<int, 12> pinChangeHitCountsThisShot{};
    std::array<bool, 12> pinHitByBallThisShot{};

    // Purchased/assigned pin types by explicit rack slot (spawn order).
    std::vector<PinSlotAssignment> pinSlotAssignments;
    std::array<int, 12> pinSlotCurrentValues{}; // slot 1 -> index 0; 0 means unavailable
    int activePinSlotCount = 10;

    void resetForNewShot() {
        pinsHitThisShot        = 0;
        retriggered            = false;
        retriggeredValue       = 0;
        firstBallHitPinIndex   = -1;
        thirdTimeComboBonus    = 0;
        pinChangeEventsThisShot = 0;
        pinChangeHitCountsThisShot.fill(0);
        pinHitByBallThisShot.fill(false);
    }

    void resetAll() {
        resetForNewShot();
        goldPinIndices.clear();
        pinSlotAssignments.clear();
        pinSlotCurrentValues.fill(0);
        activePinSlotCount = 10;
        purchasedPowers.clear();
        ballType         = BallType::Normal;
        ballSlot1        = BallType::Normal;
        ballSlot2        = BallType::Normal;
        ballSlot3        = BallType::Normal;
        shoeType         = ShoeType::None;
        radiusMultiplier = 1.0f;
        speedMultiplier  = 1.0f;
        massMultiplier   = 1.0f;
        launchSpeedMultiplier = 1.0f;
        pinMassMultiplier     = 1.0f;
        slideMultiplier       = 1.0f;
        lockPinChangesMidRound = false;

        powerGreedy = false;
        powerRandomUpgrade = false;
        powerExtraPins = false;
        powerExtraBall = false;
        powerBumpers = false;
        powerHomeBase = false;
        powerConfusion = false;
        powerEarthquake = false;
        powerSkip = false;
        powerUpgradesForEveryone = false;
        powerSales = false;
        powerPassedGo = false;
        powerMoMoney = false;
        powerExtraPowerSlot = false;
        clownBonusClaimed = false;
        clownShoesPurchased = false;
        clownRoundsRemaining = 0;

        duplicateCharges = 0;
        swapCharges = 0;
        sevenEightNineCharges = 0;
        sevenEightNinePin9Destroyed = false;
        skipCharges = 0;
        earthquakeShotCounter = 0;
        pendingRandomPinUpgrades = 0;
        homeBaseComboBonus = 0.0f;
        homeBasePinsTowardNextCombo = 0;
        thirdTimeGlobalKnocks = 0;
        pinsStandingAtShotStart = 10;
    }

    BallType getBallForShot(int shot) const {
        if (shot <= 1) return ballSlot1;
        if (shot == 2) return ballSlot2;
        return ballSlot3;
    }

    BallType getBallForSlot(int slot) const {
        if (slot == 2) return ballSlot2;
        if (slot == 3) return ballSlot3;
        return ballSlot1;
    }

    void setBallForSlot(int slot, BallType type) {
        if (slot == 2) ballSlot2 = type;
        else if (slot == 3) ballSlot3 = type;
        else           ballSlot1 = type;
    }

    void setActivePinSlotCount(int count) {
        activePinSlotCount = std::clamp(count, 1, 12);
    }

    int getActivePinSlotCount() const {
        return activePinSlotCount;
    }

    int getMaxPermanentPowerSlots() const {
        bool hasBonus = powerExtraPowerSlot || hasPurchasedPower(PowerType::ExtraPowerSlot);
        return hasBonus ? 5 : 4;
    }

    int findPinAssignmentIndexBySlot(int slot) const {
        for (int i = 0; i < (int)pinSlotAssignments.size(); i++) {
            if (pinSlotAssignments[i].slot == slot) return i;
        }
        return -1;
    }

    PinType getPinTypeForSlot(int slot) const {
        int idx = findPinAssignmentIndexBySlot(slot);
        if (idx < 0) return PinType::Normal;
        return pinSlotAssignments[idx].type;
    }

    bool hasPinAssignmentAtSlot(int slot) const {
        return findPinAssignmentIndexBySlot(slot) >= 0;
    }

    int getPinAssignmentCount() const {
        return (int)pinSlotAssignments.size();
    }

    bool removePinAssignmentAtSlot(int slot, PinType* removedType = nullptr) {
        int idx = findPinAssignmentIndexBySlot(slot);
        if (idx < 0) return false;
        if (removedType) *removedType = pinSlotAssignments[idx].type;
        pinSlotAssignments.erase(pinSlotAssignments.begin() + idx);
        return true;
    }

    void setPinAssignment(int slot, PinType type) {
        if (slot < 1 || slot > 12) return;
        if (type == PinType::Normal) {
            removePinAssignmentAtSlot(slot);
            return;
        }
        int idx = findPinAssignmentIndexBySlot(slot);
        if (idx >= 0) {
            pinSlotAssignments[idx].type = type;
        } else {
            pinSlotAssignments.push_back({slot, type});
        }
        std::sort(pinSlotAssignments.begin(), pinSlotAssignments.end(),
                  [](const PinSlotAssignment& a, const PinSlotAssignment& b) {
                      return a.slot < b.slot;
                  });
    }

    std::vector<PinSlotAssignment> getSortedPinAssignments() const {
        std::vector<PinSlotAssignment> out = pinSlotAssignments;
        std::sort(out.begin(), out.end(),
                  [](const PinSlotAssignment& a, const PinSlotAssignment& b) {
                      return a.slot < b.slot;
                  });
        return out;
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

    void applyShoeType(ShoeType type) {
        shoeType = type;
        launchSpeedMultiplier  = 1.0f;
        pinMassMultiplier      = 1.0f;
        slideMultiplier        = 1.0f;
        lockPinChangesMidRound = false;
        clownRoundsRemaining   = (type == ShoeType::Clown) ? 3 : 0;

        switch (type) {
            case ShoeType::Running:
                launchSpeedMultiplier = 1.18f;
                break;
            case ShoeType::Moon:
                pinMassMultiplier = 0.74f;
                break;
            case ShoeType::Slippers:
                slideMultiplier = 1.18f;
                break;
            case ShoeType::HighHeels:
                break;
            case ShoeType::SteelCap:
                lockPinChangesMidRound = true;
                break;
            case ShoeType::Clown:
                launchSpeedMultiplier = 0.92f;
                break;
            case ShoeType::None:
            default:
                break;
        }
    }

    void advanceRoundsAndExpireTemporaryEffects(int roundsAdvanced = 1) {
        if (roundsAdvanced <= 0) return;
        if (shoeType == ShoeType::Clown) {
            clownRoundsRemaining = std::max(0, clownRoundsRemaining - roundsAdvanced);
            if (clownRoundsRemaining <= 0) {
                applyShoeType(ShoeType::None);
            }
        }
    }

    bool hasPower(PowerType p) const {
        switch (p) {
            case PowerType::Greedy:             return powerGreedy;
            case PowerType::RandomUpgrade:      return powerRandomUpgrade;
            case PowerType::ExtraPins:          return powerExtraPins;
            case PowerType::ExtraBall:          return powerExtraBall;
            case PowerType::Duplicate:          return duplicateCharges > 0;
            case PowerType::Bumpers:            return powerBumpers;
            case PowerType::SwapPins:           return swapCharges > 0;
            case PowerType::SevenEightNine:     return sevenEightNineCharges > 0;
            case PowerType::HomeBase:           return powerHomeBase;
            case PowerType::Confusion:          return powerConfusion;
            case PowerType::Earthquake:         return powerEarthquake;
            case PowerType::Skip:               return powerSkip;
            case PowerType::UpgradesForEveryone:return powerUpgradesForEveryone;
            case PowerType::Sales:              return powerSales;
            case PowerType::PassedGo:           return powerPassedGo;
            case PowerType::MoMoney:            return powerMoMoney;
            case PowerType::ExtraPowerSlot:     return powerExtraPowerSlot;
            default:                            return false;
        }
    }

    void applyPower(PowerType p) {
        purchasedPowers.push_back(static_cast<int>(p));
        switch (p) {
            case PowerType::Greedy:             powerGreedy = true; break;
            case PowerType::RandomUpgrade:      powerRandomUpgrade = true; break;
            case PowerType::ExtraPins:          powerExtraPins = true; break;
            case PowerType::ExtraBall:          powerExtraBall = true; break;
            case PowerType::Duplicate:          duplicateCharges += 1; break;
            case PowerType::Bumpers:            powerBumpers = true; break;
            case PowerType::SwapPins:           swapCharges += 1; break;
            case PowerType::SevenEightNine:     sevenEightNineCharges += 1; break;
            case PowerType::HomeBase:           powerHomeBase = true; break;
            case PowerType::Confusion:          powerConfusion = true; break;
            case PowerType::Earthquake:         powerEarthquake = true; break;
            case PowerType::Skip:
                powerSkip = true;
                skipCharges = 2;
                break;
            case PowerType::UpgradesForEveryone:powerUpgradesForEveryone = true; break;
            case PowerType::Sales:              powerSales = true; break;
            case PowerType::PassedGo:           powerPassedGo = true; break;
            case PowerType::MoMoney:            powerMoMoney = true; break;
            case PowerType::ExtraPowerSlot:     powerExtraPowerSlot = true; break;
            default: break;
        }
    }

    bool hasPurchasedPower(PowerType p) const {
        int target = static_cast<int>(p);
        for (int raw : purchasedPowers) {
            if (raw == target) return true;
        }
        return false;
    }
};
