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

struct ActiveItems {
    struct PinSlotAssignment {
        int slot = 1; // 1-based spawn-order slot
        PinType type = PinType::Normal;
    };

    // Ball currently active for this shot (set by Game::resetBall)
    BallType ballType = BallType::Normal;
    // Persistent loadout: shot 1 uses slot 1, shot 2 uses slot 2
    BallType ballSlot1 = BallType::Normal;
    BallType ballSlot2 = BallType::Normal;
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

    int duplicateCharges = 0;
    int swapCharges = 0;
    int sevenEightNineCharges = 0;
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

        duplicateCharges = 0;
        swapCharges = 0;
        sevenEightNineCharges = 0;
        skipCharges = 0;
        earthquakeShotCounter = 0;
        pendingRandomPinUpgrades = 0;
        homeBaseComboBonus = 0.0f;
        homeBasePinsTowardNextCombo = 0;
        thirdTimeGlobalKnocks = 0;
        pinsStandingAtShotStart = 10;
    }

    BallType getBallForShot(int shot) const {
        return (shot >= 2) ? ballSlot2 : ballSlot1;
    }

    BallType getBallForSlot(int slot) const {
        return (slot == 2) ? ballSlot2 : ballSlot1;
    }

    void setBallForSlot(int slot, BallType type) {
        if (slot == 2) ballSlot2 = type;
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
            case ShoeType::None:
            default:
                break;
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
