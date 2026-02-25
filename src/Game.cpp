#include "Game.h"
#include "Physics.h"
#include "Items.h"
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <cstdint>

namespace {
constexpr float kHomeBaseComboBonusCap = 8.0f;

struct ResetConfirmLayout {
    sf::FloatRect panel;
    sf::FloatRect yesButton;
    sf::FloatRect noButton;
};

struct BossIntroLayout {
    sf::FloatRect panel;
    sf::FloatRect continueButton;
};

ResetConfirmLayout makeResetConfirmLayout(float windowW, float windowH) {
    ResetConfirmLayout out;
    float panelW = std::clamp(windowW * 0.34f, 360.0f, 560.0f);
    float panelH = std::clamp(windowH * 0.24f, 200.0f, 280.0f);
    float panelX = windowW * 0.5f - panelW * 0.5f;
    float panelY = windowH * 0.5f - panelH * 0.5f;
    out.panel = sf::FloatRect(sf::Vector2f(panelX, panelY), sf::Vector2f(panelW, panelH));

    float btnW = std::clamp(panelW * 0.34f, 120.0f, 190.0f);
    float btnH = 48.0f;
    float btnGap = std::clamp(panelW * 0.08f, 22.0f, 42.0f);
    float btnY = panelY + panelH - btnH - 22.0f;
    float btnStartX = panelX + panelW * 0.5f - (btnW * 2.0f + btnGap) * 0.5f;

    out.yesButton = sf::FloatRect(sf::Vector2f(btnStartX, btnY), sf::Vector2f(btnW, btnH));
    out.noButton = sf::FloatRect(sf::Vector2f(btnStartX + btnW + btnGap, btnY), sf::Vector2f(btnW, btnH));
    return out;
}

BossIntroLayout makeBossIntroLayout(float windowW, float windowH) {
    BossIntroLayout out;
    float panelW = std::clamp(windowW * 0.48f, 420.0f, 760.0f);
    float panelH = std::clamp(windowH * 0.34f, 260.0f, 430.0f);
    float panelX = windowW * 0.5f - panelW * 0.5f;
    float panelY = windowH * 0.5f - panelH * 0.5f;
    out.panel = sf::FloatRect(sf::Vector2f(panelX, panelY), sf::Vector2f(panelW, panelH));

    float btnW = std::clamp(panelW * 0.30f, 150.0f, 240.0f);
    float btnH = 54.0f;
    float btnX = panelX + panelW * 0.5f - btnW * 0.5f;
    float btnY = panelY + panelH - btnH - 20.0f;
    out.continueButton = sf::FloatRect(sf::Vector2f(btnX, btnY), sf::Vector2f(btnW, btnH));
    return out;
}

bool pointInRectWithPad(const sf::FloatRect& rect, sf::Vector2f point, float pad = 0.0f) {
    return point.x >= rect.position.x - pad &&
           point.x <= rect.position.x + rect.size.x + pad &&
           point.y >= rect.position.y - pad &&
           point.y <= rect.position.y + rect.size.y + pad;
}

float randomRange(float lo, float hi) {
    if (hi <= lo) return lo;
    float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return lo + t * (hi - lo);
}

sf::Color randomConfettiColor() {
    static const sf::Color kPalette[] = {
        sf::Color(255, 60, 80),   // red
        sf::Color(255, 150, 45),  // orange
        sf::Color(255, 235, 70),  // yellow
        sf::Color(75, 225, 95),   // green
        sf::Color(70, 190, 255),  // cyan
        sf::Color(100, 130, 255), // blue
        sf::Color(200, 110, 255), // purple
        sf::Color(255, 105, 205), // pink
    };
    return kPalette[rand() % (sizeof(kPalette) / sizeof(kPalette[0]))];
}
}

Game::Game()
: window(sf::VideoMode(sf::Vector2u((unsigned)windowW, (unsigned)windowH)), "Bowling Prototype")
, ball(25.0f)
{
    window.setFramerateLimit(60);
    
    // Seed random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    // Fixed world camera
    view = sf::View(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(windowW, windowH)));
    window.setView(view);

    // Apply letterbox once at startup
    auto s = window.getSize();
    applyLetterbox(s.x, s.y);

    lane.init(windowW);

    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    pins = createPins(lane.centerX(), 240.0f);
    
    // Audio automatically initializes itself
    // Start on menu, so no game music yet
    
    // Load high score
    loadHighScore();
    
    // Start in menu state
    ui.setState(GameState::Menu);
    audio.playMenuMusic();
}

std::vector<Pin> Game::createPins(float centerX, float startY) {
    std::vector<Pin> out;

    float spacing = 55.0f;
    if (activeItems.shoeType == ShoeType::HighHeels) {
        spacing *= 0.92f; // 8% closer rack spacing
    }
    float radius  = 9.0f;

    int pinValue = 1;
    for (int row = 0; row < 4; row++) {
        int count = row + 1;
        float y = startY - row * spacing;

        float rowWidth = (count - 1) * spacing;
        float startX = centerX - rowWidth / 2.0f;

        for (int i = 0; i < count; i++) {
            float x = startX + i * spacing;
            out.emplace_back(sf::Vector2f(x, y), radius, pinValue);
            pinValue++;
        }
    }

    const bool hasExtraPins =
        activeItems.powerExtraPins || activeItems.hasPurchasedPower(PowerType::ExtraPins);
    activeItems.setActivePinSlotCount(hasExtraPins ? 12 : 10);
    if (hasExtraPins) {
        // Spawn directly next to pin 1 (front-center), not deeper in the rack.
        float y = startY;
        out.emplace_back(sf::Vector2f(centerX - spacing * 1.02f, y), radius, pinValue++);
        out.emplace_back(sf::Vector2f(centerX + spacing * 1.02f, y), radius, pinValue++);
    }

    // Preserve previously changed values by pin slot when we rerack/rebuild.
    applySavedPinSlotValues(out);
    updatePinSlotValueSnapshot(out);
    return out;
}

int Game::countStandingPins() const {
    int standing = 0;
    for (const auto& pin : pins) {
        if (pin.isActive() && !pin.isFallen()) standing++;
    }
    return standing;
}

void Game::applyPurchasedPinTypes(std::vector<Pin>& pinSet) {
    if (!activeItems.pinSlotAssignments.empty()) {
        int maxSlot = std::min((int)pinSet.size(), activeItems.getActivePinSlotCount());
        for (const auto& assigned : activeItems.pinSlotAssignments) {
            if (assigned.slot < 1 || assigned.slot > maxSlot) continue;
            int idx = assigned.slot - 1;
            if (!pinSet[idx].isActive()) continue;
            pinSet[idx].setPinType(assigned.type);
        }
    }

    // Always restore saved slot values after type application, so value upgrades
    // persist even when pins are rebuilt or reset.
    applySavedPinSlotValues(pinSet);
    updatePinSlotValueSnapshot(pinSet);
}

void Game::applyPendingRandomPinUpgrades(std::vector<Pin>& pinSet) {
    if (activeItems.pendingRandomPinUpgrades <= 0) return;

    std::vector<int> candidates;
    candidates.reserve(pinSet.size());
    for (int i = 0; i < (int)pinSet.size(); i++) {
        if (pinSet[i].isActive()) candidates.push_back(i);
    }
    if (candidates.empty()) {
        activeItems.pendingRandomPinUpgrades = 0;
        return;
    }

    for (int n = 0; n < activeItems.pendingRandomPinUpgrades; n++) {
        int idx = candidates[rand() % candidates.size()];
        pinSet[idx].setValue(pinSet[idx].getValue() + 1);
    }
    activeItems.pendingRandomPinUpgrades = 0;
    updatePinSlotValueSnapshot(pinSet);
}

void Game::applyPowerPinLayout(std::vector<Pin>& pinSet) {
    if (ui.getState() == GameState::Xtreme) {
        refreshBossStateForCurrentRound();
    }
    // Duplicate and Swap are now manual (player-chosen), not random-on-rerack.
    applyPendingRandomPinUpgrades(pinSet);
    applyBossPinLayout(pinSet);

    // Keep 7 8 9's pin-9 destruction persistent across all rerack/layout paths.
    if (activeItems.sevenEightNinePin9Destroyed) {
        int maxSlot = std::min((int)pinSet.size(), activeItems.getActivePinSlotCount());
        constexpr int idx9 = 8;
        if (maxSlot > idx9) {
            pinSet[idx9].setFallen(true);
            pinSet[idx9].setVel({0.0f, 0.0f});
            pinSet[idx9].setAngularVel(0.0f);
            pinSet[idx9].setActive(false);
        }
    }

    updatePinSlotValueSnapshot(pinSet);
}

bool Game::isBossRound(int round) const {
    return round > 0 && (round % 5 == 0);
}

int Game::getShotBallSlot(int shotInFrame) const {
    if (shotInFrame <= 1) return 1;
    if (shotInFrame == 2) return 2;
    return 3;
}

std::string Game::getActiveBossName() const {
    switch (activeBoss) {
        case BossType::MissingPin: return "THE ENCHANTRESS";
        case BossType::DoubleTarget: return "THE OVERLORD";
        case BossType::LowBaseScore: return "THE TWINS";
        case BossType::RandomPowerDisabledEachShot: return "THE EMPEROR";
        case BossType::HeavyPins: return "THE MONSTER";
        case BossType::None:
        default: return "BOSS";
    }
}

std::string Game::getActiveBossDescription() const {
    switch (activeBoss) {
        case BossType::MissingPin:
            return "One random pin is missing every round.";
        case BossType::DoubleTarget:
            return "Double the normal amount of the target score.";
        case BossType::LowBaseScore:
            return "Base score is 5 x 0.5.";
        case BossType::RandomPowerDisabledEachShot:
            return "One random power disabled every shot.";
        case BossType::HeavyPins:
            return "All pins are 15% heavier.";
        case BossType::None:
        default:
            return "Special round rules active.";
    }
}

void Game::playBossLaughForCurrentBoss(float volume) {
    switch (activeBoss) {
        case BossType::MissingPin:
            audio.playBossLaugh(AudioManager::BossLaughType::Enchantress, volume);
            break;
        case BossType::DoubleTarget:
            audio.playBossLaugh(AudioManager::BossLaughType::Overlord, volume);
            break;
        case BossType::LowBaseScore:
            audio.playBossLaugh(AudioManager::BossLaughType::Twins, volume);
            break;
        case BossType::RandomPowerDisabledEachShot:
            audio.playBossLaugh(AudioManager::BossLaughType::Emperor, volume);
            break;
        case BossType::HeavyPins:
            audio.playBossLaugh(AudioManager::BossLaughType::Monster, volume);
            break;
        case BossType::None:
        default:
            break;
    }
}

void Game::openBossIntroIfNeeded() {
    if (ui.getState() != GameState::Xtreme) return;
    int round = xtreme.getRound();
    if (!isBossRound(round)) return;
    if (bossIntroShownRound == round) return;

    refreshBossStateForCurrentRound();
    bossIntroOpen = true;
    bossIntroShownRound = round;
    playBossLaughForCurrentBoss(70.0f);
}

bool Game::isPowerEnabledThisShot(PowerType p) const {
    bool owned = activeItems.hasPower(p) || activeItems.hasPurchasedPower(p);
    if (!owned) return false;

    if (ui.getState() == GameState::Xtreme &&
        activeBoss == BossType::RandomPowerDisabledEachShot &&
        activeBossRound == xtreme.getRound() &&
        bossDisabledPowerThisShot.has_value() &&
        bossDisabledPowerThisShot.value() == p) {
        return false;
    }
    return true;
}

void Game::refreshBossStateForCurrentRound() {
    if (ui.getState() != GameState::Xtreme) {
        activeBoss = BossType::None;
        activeBossRound = 0;
        bossMissingPinSlot = -1;
        bossDisabledPowerThisShot.reset();
        xtreme.setRoundTargetMultiplier(1.0f);
        xtreme.setRoundClearRewardBonus(0);
        return;
    }

    int round = xtreme.getRound();
    if (!isBossRound(round)) {
        activeBoss = BossType::None;
        activeBossRound = 0;
        bossMissingPinSlot = -1;
        bossDisabledPowerThisShot.reset();
        xtreme.setRoundTargetMultiplier(1.0f);
        xtreme.setRoundClearRewardBonus(0);
        return;
    }

    if (activeBossRound != round || activeBoss == BossType::None) {
        activeBossRound = round;
        bossDisabledPowerThisShot.reset();
        bossMissingPinSlot = -1;

        int pick = rand() % 5;
        switch (pick) {
            case 0: activeBoss = BossType::MissingPin; break;
            case 1: activeBoss = BossType::DoubleTarget; break;
            case 2: activeBoss = BossType::LowBaseScore; break;
            case 3: activeBoss = BossType::RandomPowerDisabledEachShot; break;
            default: activeBoss = BossType::HeavyPins; break;
        }

        if (activeBoss == BossType::MissingPin) {
            int maxSlots = std::max(1, activeItems.getActivePinSlotCount());
            bossMissingPinSlot = 1 + (rand() % maxSlots);
        }
    }

    float targetMultiplier = (activeBoss == BossType::DoubleTarget) ? 2.0f : 1.0f;
    xtreme.setRoundTargetMultiplier(targetMultiplier);
    xtreme.setRoundClearRewardBonus(3); // Boss round clear pays +3.
}

void Game::applyBossPinLayout(std::vector<Pin>& pinSet) {
    if (ui.getState() != GameState::Xtreme) return;
    if (activeBoss != BossType::MissingPin) return;
    if (activeBossRound != xtreme.getRound()) return;
    if (pinSet.empty()) return;

    int maxSlot = std::min((int)pinSet.size(), activeItems.getActivePinSlotCount());
    if (maxSlot <= 0) return;
    if (bossMissingPinSlot < 1 || bossMissingPinSlot > maxSlot) {
        bossMissingPinSlot = 1 + (rand() % maxSlot);
    }

    int idx = bossMissingPinSlot - 1;
    pinSet[idx].setFallen(true);
    pinSet[idx].setActive(false);
    pinSet[idx].setVel({0.0f, 0.0f});
    pinSet[idx].setAngularVel(0.0f);
}

void Game::rollBossDisabledPowerForShot() {
    bossDisabledPowerThisShot.reset();
    if (ui.getState() != GameState::Xtreme) return;
    if (activeBoss != BossType::RandomPowerDisabledEachShot) return;
    if (activeBossRound != xtreme.getRound()) return;

    std::vector<PowerType> candidates;
    auto addIfOwned = [&](PowerType p) {
        if (activeItems.hasPower(p) || activeItems.hasPurchasedPower(p)) {
            candidates.push_back(p);
        }
    };

    // Limit to powers that materially affect gameplay each shot.
    addIfOwned(PowerType::Greedy);
    addIfOwned(PowerType::RandomUpgrade);
    addIfOwned(PowerType::Bumpers);
    addIfOwned(PowerType::HomeBase);
    addIfOwned(PowerType::Confusion);
    addIfOwned(PowerType::Earthquake);

    if (candidates.empty()) return;
    bossDisabledPowerThisShot = candidates[rand() % candidates.size()];
}

void Game::applySavedPinSlotValues(std::vector<Pin>& pinSet) {
    int maxSlot = std::min((int)pinSet.size(), activeItems.getActivePinSlotCount());
    for (int i = 0; i < maxSlot; i++) {
        if (!pinSet[i].isActive()) continue;
        int saved = activeItems.pinSlotCurrentValues[i];
        if (saved <= 0) continue;

        // Snapshot stores display value (+1 already shown for LevelUp),
        // but Pin::value stores base value before LevelUp bonus.
        int internalValue = saved;
        if (pinSet[i].getPinType() == PinType::LevelUp) {
            internalValue = std::max(0, saved - 1);
        }
        pinSet[i].setValue(internalValue);
    }
}

void Game::updatePinSlotValueSnapshot(const std::vector<Pin>& pinSet) {
    activeItems.pinSlotCurrentValues.fill(0);
    int maxSlot = std::min((int)pinSet.size(), activeItems.getActivePinSlotCount());
    for (int i = 0; i < maxSlot; i++) {
        int value = pinSet[i].getValue();
        if (pinSet[i].getPinType() == PinType::LevelUp) {
            value += 1;
        }
        activeItems.pinSlotCurrentValues[i] = value;
    }
}

void Game::cancelPinPowerSelection() {
    pinPowerSelectionMode = PinPowerSelectionMode::None;
    pinPowerFirstIndex = -1;
}

int Game::findPinAtWorldPos(sf::Vector2f worldPos) const {
    int best = -1;
    float bestDist2 = 1.0e30f;
    for (int i = 0; i < (int)pins.size(); i++) {
        const Pin& pin = pins[i];
        if (!pin.isActive()) continue;
        if (pin.isFallen()) continue;
        sf::Vector2f d = pin.getPos() - worldPos;
        float dist2 = d.x * d.x + d.y * d.y;
        float hitR = std::max(24.0f, pin.getRadius() * 3.2f);
        if (dist2 <= hitR * hitR && dist2 < bestDist2) {
            bestDist2 = dist2;
            best = i;
        }
    }
    return best;
}

void Game::applyManualDuplicate(int sourceIndex, int targetIndex) {
    if (sourceIndex < 0 || sourceIndex >= (int)pins.size()) return;
    if (targetIndex < 0 || targetIndex >= (int)pins.size()) return;
    if (!pins[sourceIndex].isActive() || !pins[targetIndex].isActive()) return;
    if (sourceIndex == targetIndex) return;
    if (activeItems.duplicateCharges <= 0) return;

    PinType copiedType = pins[sourceIndex].getPinType();
    pins[targetIndex].setPinType(copiedType);

    if (copiedType != PinType::Normal) {
        activeItems.setPinAssignment(targetIndex + 1, copiedType);
    } else {
        activeItems.removePinAssignmentAtSlot(targetIndex + 1);
    }
    activeItems.duplicateCharges--;
    updatePinSlotValueSnapshot(pins);
}

void Game::applyManualSwap(int firstIndex, int secondIndex) {
    if (firstIndex < 0 || firstIndex >= (int)pins.size()) return;
    if (secondIndex < 0 || secondIndex >= (int)pins.size()) return;
    if (!pins[firstIndex].isActive() || !pins[secondIndex].isActive()) return;
    if (firstIndex == secondIndex) return;
    if (activeItems.swapCharges <= 0) return;

    // Swap pin state by slot (type + value), then persist the swapped slots.
    // Swapping full Pin objects can desync slot-assignment state for special pins.
    PinType firstType = pins[firstIndex].getPinType();
    PinType secondType = pins[secondIndex].getPinType();
    int firstValue = pins[firstIndex].getValue();
    int secondValue = pins[secondIndex].getValue();

    pins[firstIndex].setPinType(secondType);
    pins[secondIndex].setPinType(firstType);
    pins[firstIndex].setValue(secondValue);
    pins[secondIndex].setValue(firstValue);

    int firstSlot = firstIndex + 1;
    int secondSlot = secondIndex + 1;
    if (secondType == PinType::Normal) {
        activeItems.removePinAssignmentAtSlot(firstSlot);
    } else {
        activeItems.setPinAssignment(firstSlot, secondType);
    }
    if (firstType == PinType::Normal) {
        activeItems.removePinAssignmentAtSlot(secondSlot);
    } else {
        activeItems.setPinAssignment(secondSlot, firstType);
    }

    activeItems.swapCharges--;
    updatePinSlotValueSnapshot(pins);
}

bool Game::handlePinPowerSelectionClick(sf::Vector2f worldPos) {
    if (pinPowerSelectionMode == PinPowerSelectionMode::None) return false;

    int clicked = findPinAtWorldPos(worldPos);
    if (clicked < 0) return true;

    switch (pinPowerSelectionMode) {
        case PinPowerSelectionMode::DuplicatePickSource:
            pinPowerFirstIndex = clicked;
            pinPowerSelectionMode = PinPowerSelectionMode::DuplicatePickTarget;
            return true;
        case PinPowerSelectionMode::DuplicatePickTarget:
            if (pinPowerFirstIndex >= 0 && clicked != pinPowerFirstIndex) {
                applyManualDuplicate(pinPowerFirstIndex, clicked);
                cancelPinPowerSelection();
            }
            return true;
        case PinPowerSelectionMode::SwapPickFirst:
            pinPowerFirstIndex = clicked;
            pinPowerSelectionMode = PinPowerSelectionMode::SwapPickSecond;
            return true;
        case PinPowerSelectionMode::SwapPickSecond:
            if (pinPowerFirstIndex >= 0 && clicked != pinPowerFirstIndex) {
                applyManualSwap(pinPowerFirstIndex, clicked);
                cancelPinPowerSelection();
            }
            return true;
        case PinPowerSelectionMode::None:
        default:
            return false;
    }
}

void Game::loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> normalHighScore >> xtremeBestRound;
        file.close();
    } else {
        normalHighScore = 0;
        xtremeBestRound = 0;
    }
}

void Game::saveHighScore() {
    std::ofstream file("highscore.txt");
    if (file.is_open()) {
        file << normalHighScore << "\n";
        file << xtremeBestRound << "\n";
        file.close();
    }
}

void Game::run() {
    while (window.isOpen()) {
        handleEvents();
        float dt = clock.restart().asSeconds();
        update(dt);
        draw();
    }
}

void Game::requestRunReset() {
    GameState s = ui.getState();
    if (s == GameState::Menu || s == GameState::Settings || s == GameState::Tutorial) return;
    if (gameOver) {
        // On game-over screen, R should instantly reset without confirmation.
        confirmRunReset();
        return;
    }
    resetConfirmOpen = true;
}

void Game::confirmRunReset() {
    bool inXtremeRun = (ui.getState() == GameState::Xtreme) ||
                       (ui.getState() == GameState::Shop) ||
                       (ui.getState() == GameState::RoundSummary) ||
                       (ui.getState() == GameState::GameWon) ||
                       xtremeMode;

    if (inXtremeRun) {
        xtreme.reset();
        xtremeRoundStartTokens = xtreme.getTokens();
        ui.setState(GameState::Xtreme);
        xtremeMode = true;
    } else {
        scorer.resetGame();
        ui.setState(GameState::Playing);
        xtremeMode = false;
    }

    gameOver = false;
    endlessMode = false;
    resetConfirmOpen = false;
    roundSummaryLeadsToGameOver = false;
    roundSummaryPassed = false;
    pendingGameOverFromScore = false;
    postScorePauseTimer = 0.0f;
    pendingReset = false;
    bossIntroOpen = false;
    bossIntroShownRound = 0;
    xtremeLastRoundScoreProgress = 0;
    xtremeLastRoundTargetProgress = 0;
    equipBall(BallType::Normal);
    activeItems.resetAll();
    ui.resetEquippedBall();
    cancelPinPowerSelection();
    lane.bumpersOn = (ui.getState() == GameState::Playing) ? ui.getBumpersDefault() : false;
    if (ui.getState() == GameState::Xtreme) {
        ui.generateShopOffers(activeItems);
    }
    pins = createPins(lane.centerX(), 240.0f);
    resetBall();
    strikeConfetti.clear();
    audio.playBackgroundMusic();
}

void Game::triggerStrikeCelebration() {
    audio.playStrikeCheer(86.0f);

    // Sometimes trigger confetti (not every strike).
    if ((rand() % 100) >= 68) return;

    strikeConfetti.clear();
    int pieceCount = 140 + (rand() % 60);
    strikeConfetti.reserve(static_cast<size_t>(pieceCount));

    float emitCenterX = windowW * 0.5f;
    float emitSpreadX = windowW * 0.26f;
    for (int i = 0; i < pieceCount; i++) {
        ConfettiPiece p;
        p.pos = {
            emitCenterX + randomRange(-emitSpreadX, emitSpreadX),
            randomRange(-70.0f, -10.0f)
        };

        float angleDeg = randomRange(58.0f, 122.0f);
        float angleRad = angleDeg * 3.14159265f / 180.0f;
        float speed = randomRange(260.0f, 620.0f);
        p.vel = {
            std::cos(angleRad) * speed,
            std::sin(angleRad) * speed
        };

        p.color = randomConfettiColor();
        p.life = randomRange(1.1f, 2.2f);
        p.totalLife = p.life;
        p.size = randomRange(4.0f, 9.0f);
        p.rotation = randomRange(0.0f, 360.0f);
        p.spin = randomRange(-380.0f, 380.0f);
        strikeConfetti.push_back(p);
    }
}

void Game::updateStrikeConfetti(float dt) {
    if (strikeConfetti.empty()) return;

    const float gravity = 920.0f;
    const float drag = 0.985f;
    for (auto& p : strikeConfetti) {
        p.life -= dt;
        p.vel.y += gravity * dt;
        p.vel.x *= std::pow(drag, dt * 60.0f);
        p.pos += p.vel * dt;
        p.rotation += p.spin * dt;
    }

    strikeConfetti.erase(
        std::remove_if(strikeConfetti.begin(), strikeConfetti.end(),
            [&](const ConfettiPiece& p) {
                return p.life <= 0.0f || p.pos.y > windowH + 60.0f;
            }),
        strikeConfetti.end());
}

void Game::drawStrikeConfetti() {
    if (strikeConfetti.empty()) return;

    for (const auto& p : strikeConfetti) {
        float t = (p.totalLife > 0.0f) ? std::clamp(p.life / p.totalLife, 0.0f, 1.0f) : 0.0f;
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(30 + 225 * t);

        sf::RectangleShape q({p.size, p.size * 0.62f});
        q.setOrigin({q.getSize().x * 0.5f, q.getSize().y * 0.5f});
        q.setPosition(p.pos);
        q.setRotation(sf::degrees(p.rotation));
        q.setFillColor(c);
        window.draw(q);
    }
}

void Game::handleEvents() {
    while (true) {
        auto ev = window.pollEvent();
        if (!ev.has_value()) break;

        if (ev->is<sf::Event::Closed>()) window.close();

        if (ev->is<sf::Event::Resized>()) {
            auto r = ev->getIf<sf::Event::Resized>();
            float oldCenterX = lane.centerX();
            applyLetterbox(r->size.x, r->size.y);
            lane.init(windowW);
            float dx = lane.centerX() - oldCenterX;
            if (std::abs(dx) > 0.001f) {
                ball.setPos(ball.getPos() + sf::Vector2f(dx, 0.0f));
                for (auto& pin : pins) {
                    pin.translate(sf::Vector2f(dx, 0.0f));
                }
            }
        }

        if (ev->is<sf::Event::KeyPressed>() && resetConfirmOpen) {
            auto keyEv = ev->getIf<sf::Event::KeyPressed>();
            if (keyEv->code == sf::Keyboard::Key::Enter) {
                confirmRunReset();
            } else if (keyEv->code == sf::Keyboard::Key::Escape) {
                resetConfirmOpen = false;
            }
            continue;
        }

        if (ev->is<sf::Event::KeyPressed>() && bossIntroOpen) {
            auto keyEv = ev->getIf<sf::Event::KeyPressed>();
            if (keyEv->code == sf::Keyboard::Key::Enter ||
                keyEv->code == sf::Keyboard::Key::Space ||
                keyEv->code == sf::Keyboard::Key::Escape) {
                bossIntroOpen = false;
            }
            continue;
        }
        
        // Handle mouse clicks for menu
        if (ev->is<sf::Event::MouseButtonPressed>()) {
            auto mouseEv = ev->getIf<sf::Event::MouseButtonPressed>();
            if (mouseEv->button == sf::Mouse::Button::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                
                // Convert to world coordinates
                sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);

                if (bossIntroOpen) {
                    BossIntroLayout modal = makeBossIntroLayout(windowW, windowH);
                    if (pointInRectWithPad(modal.continueButton, worldPos, 8.0f)) {
                        bossIntroOpen = false;
                    }
                    continue;
                }

                if (resetConfirmOpen) {
                    ResetConfirmLayout modal = makeResetConfirmLayout(windowW, windowH);
                    if (pointInRectWithPad(modal.yesButton, worldPos, 8.0f)) {
                        confirmRunReset();
                    } else if (pointInRectWithPad(modal.noButton, worldPos, 8.0f)) {
                        resetConfirmOpen = false;
                    }
                    continue;
                }

                if (ui.getState() == GameState::Xtreme && !gameOver) {
                    if (!rollLocked && !pendingReset && handlePinPowerSelectionClick(worldPos)) {
                        continue;
                    }
                }
                
                if (ui.getState() == GameState::Menu) {
                    MenuButton clicked = ui.handleMenuClick(window, sf::Vector2i(worldPos.x, worldPos.y));
                    
                    if (clicked == MenuButton::Normal) {
                        xtremeMode = false;
                        endlessMode = false;
                        bossIntroOpen = false;
                        bossIntroShownRound = 0;
                        ui.setState(GameState::Playing);
                        strikeConfetti.clear();
                        scorer.resetGame();
                        gameOver = false;
                        roundSummaryLeadsToGameOver = false;
                        roundSummaryPassed = false;
                        pendingGameOverFromScore = false;
                        postScorePauseTimer = 0.0f;
                        xtremeLastRoundScoreProgress = 0;
                        xtremeLastRoundTargetProgress = 0;
                        equipBall(BallType::Normal);
                        activeItems.resetAll();
                        ui.resetEquippedBall();
                        cancelPinPowerSelection();
                        lane.bumpersOn = ui.getBumpersDefault();
                        pins = createPins(lane.centerX(), 240.0f);
                        resetBall();
                        audio.playBackgroundMusic();

                    } else if (clicked == MenuButton::Xtreme) {
                        ui.setState(GameState::Xtreme);
                        strikeConfetti.clear();
                        xtreme.reset();
                        bossIntroOpen = false;
                        bossIntroShownRound = 0;
                        xtremeMode = true;
                        endlessMode = false;
                        gameOver = false;
                        roundSummaryLeadsToGameOver = false;
                        roundSummaryPassed = false;
                        pendingGameOverFromScore = false;
                        postScorePauseTimer = 0.0f;
                        xtremeRoundStartTokens = xtreme.getTokens();
                        xtremeLastRoundScoreProgress = 0;
                        xtremeLastRoundTargetProgress = 0;
                        equipBall(BallType::Normal);
                        activeItems.resetAll();
                        ui.resetEquippedBall();
                        cancelPinPowerSelection();
                        ui.generateShopOffers(activeItems);
                        lane.bumpersOn = false;
                        pins = createPins(lane.centerX(), 240.0f);
                        resetBall();
                        audio.playBackgroundMusic();

                    } else if (clicked == MenuButton::Tutorial) {
                        ui.setState(GameState::Tutorial);

                    } else if (clicked == MenuButton::Settings) {
                        ui.setState(GameState::Settings);
                    }
                }
                if (ui.getState() == GameState::Tutorial) {
                    if (ui.handleTutorialClick(window, sf::Vector2i(worldPos.x, worldPos.y))) {
                        ui.setState(GameState::Menu);
                    }
                    continue;
                }
                if (ui.getState() == GameState::Settings) {
                    ui.handleSettingsClick(window, sf::Vector2i(worldPos.x, worldPos.y));
                }
                if (ui.getState() == GameState::RoundSummary) {
                    if (ui.handleRoundSummaryClick(sf::Vector2i(worldPos.x, worldPos.y))) {
                        if (roundSummaryLeadsToGameOver) {
                            ui.setState(GameState::Xtreme);
                            gameOver = true;
                            pendingGameOverFromScore = false;
                            postScorePauseTimer = 0.0f;
                            pendingReset = false;
                        } else {
                            ui.setState(GameState::Shop);
                            xtreme.consumeShopReady();
                            if (activeItems.powerSkip || activeItems.hasPurchasedPower(PowerType::Skip)) {
                                activeItems.powerSkip = true;
                                activeItems.skipCharges = 2;
                            }
                        }
                        roundSummaryLeadsToGameOver = false;
                        cancelPinPowerSelection();
                        continue;
                    }
                }
                if (ui.getState() == GameState::GameWon) {
                    GameWonAction wonAction = ui.handleGameWonClick(sf::Vector2i(worldPos.x, worldPos.y));
                    if (wonAction == GameWonAction::EndlessMode) {
                        endlessMode = true;
                        ui.setState(GameState::Shop);
                        cancelPinPowerSelection();
                        xtreme.consumeShopReady();
                        if (activeItems.powerSkip || activeItems.hasPurchasedPower(PowerType::Skip)) {
                            activeItems.powerSkip = true;
                            activeItems.skipCharges = 2;
                        }
                        continue;
                    }
                    if (wonAction == GameWonAction::RestartRun) {
                        confirmRunReset();
                        continue;
                    }
                    if (wonAction == GameWonAction::GoToMenu) {
                        ui.setState(GameState::Menu);
                        strikeConfetti.clear();
                        scorer.resetGame();
                        xtreme.reset();
                        bossIntroOpen = false;
                        bossIntroShownRound = 0;
                        xtremeMode = false;
                        endlessMode = false;
                        roundSummaryLeadsToGameOver = false;
                        roundSummaryPassed = false;
                        pendingGameOverFromScore = false;
                        postScorePauseTimer = 0.0f;
                        xtremeLastRoundScoreProgress = 0;
                        xtremeLastRoundTargetProgress = 0;
                        xtremeRoundStartTokens = xtreme.getTokens();
                        equipBall(BallType::Normal);
                        activeItems.resetAll();
                        ui.resetEquippedBall();
                        cancelPinPowerSelection();
                        resetBall();
                        pins = createPins(lane.centerX(), 240.0f);
                        audio.stopBackgroundMusic();
                        audio.stopBallRoll();
                        audio.playMenuMusic();
                        continue;
                    }
                }
                if (ui.getState() == GameState::Shop) {
                    sf::Vector2i worldPosI((int)worldPos.x, (int)worldPos.y);
                    int purchased = ui.handleShopClick(window, worldPosI, xtreme.getTokens(), activeItems);

                    auto erasePowerRecord = [&](PowerType p, bool all) {
                        int target = static_cast<int>(p);
                        if (all) {
                            activeItems.purchasedPowers.erase(
                                std::remove(activeItems.purchasedPowers.begin(),
                                            activeItems.purchasedPowers.end(),
                                            target),
                                activeItems.purchasedPowers.end());
                            return;
                        }
                        auto it = std::find(activeItems.purchasedPowers.begin(),
                                            activeItems.purchasedPowers.end(),
                                            target);
                        if (it != activeItems.purchasedPowers.end()) {
                            activeItems.purchasedPowers.erase(it);
                        }
                    };

                    auto isStackablePower = [](PowerType p) {
                        return p == PowerType::Duplicate ||
                               p == PowerType::SwapPins ||
                               p == PowerType::SevenEightNine;
                    };

                    auto powerCountsTowardLimit = [&](PowerType p) {
                        if (isStackablePower(p)) return false;
                        if (p == PowerType::ExtraPowerSlot) return false;
                        return true;
                    };

                    auto pinTypeCost = [](PinType t) {
                        switch (t) {
                            case PinType::Gold:        return 2;
                            case PinType::Mischievous: return 2;
                            case PinType::Exploding:   return 4;
                            case PinType::Light:       return 2;
                            case PinType::Big:         return 3;
                            case PinType::Ice:         return 2;
                            case PinType::CopyCat:     return 3;
                            case PinType::LuckyDucky:  return 4;
                            case PinType::LevelUp:     return 1;
                            case PinType::Lover:       return 3;
                            case PinType::ChangeIsGood:return 3;
                            case PinType::ThirdTime:   return 3;
                            default:                   return 1;
                        }
                    };

                    auto ballTypeCost = [](BallType t) {
                        switch (t) {
                            case BallType::BlackHole: return 4;
                            case BallType::Midas:     return 5;
                            case BallType::Upgrade:   return 3;
                            case BallType::Heavy:     return 2;
                            case BallType::Fastball:  return 2;
                            case BallType::OddBall:   return 3;
                            case BallType::EightBall: return 4;
                            case BallType::Icy:       return 4;
                            case BallType::Retrigger: return 4;
                            default:                  return 1;
                        }
                    };

                    auto shoeTypeCost = [](ShoeType t) {
                        switch (t) {
                            case ShoeType::Clown:    return 0;
                            case ShoeType::Running:  return 3;
                            case ShoeType::Moon:     return 4;
                            case ShoeType::Slippers: return 3;
                            case ShoeType::HighHeels:return 3;
                            case ShoeType::SteelCap: return 4;
                            default:                 return 0;
                        }
                    };

                    auto powerTypeCost = [](PowerType t) {
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
                    };

                    auto ownedPermanentPowerCount = [&]() {
                        int count = 0;
                        for (int i = 0; i <= (int)PowerType::ExtraPowerSlot; i++) {
                            PowerType p = static_cast<PowerType>(i);
                            if (!powerCountsTowardLimit(p)) continue;
                            if (activeItems.hasPower(p) || activeItems.hasPurchasedPower(p)) {
                                count++;
                            }
                        }
                        return count;
                    };

                    auto syncRunPowersToScorer = [&]() {
                        bool hasExtraBall = activeItems.powerExtraBall ||
                                            activeItems.hasPurchasedPower(PowerType::ExtraBall);
                        bool hasPassedGo = activeItems.powerPassedGo ||
                                           activeItems.hasPurchasedPower(PowerType::PassedGo);
                        bool hasMoMoney = activeItems.powerMoMoney ||
                                          activeItems.hasPurchasedPower(PowerType::MoMoney);
                        xtreme.setExtraBallEnabled(hasExtraBall);
                        xtreme.setPowerPassedGo(hasPassedGo);
                        xtreme.setPowerMoMoney(hasMoMoney);
                        float clampedBonus = std::min(activeItems.homeBaseComboBonus, kHomeBaseComboBonusCap);
                        xtreme.setBaseCombo(1.0f + clampedBonus);
                        if (activeItems.powerBumpers) lane.bumpersOn = true;
                    };

                    auto buildSellablePowerList = [&]() {
                        const int powerCount = static_cast<int>(PowerType::ExtraPowerSlot) + 1;
                        std::vector<PowerType> out;
                        out.reserve(powerCount);
                        for (int i = 0; i < powerCount; i++) {
                            PowerType p = static_cast<PowerType>(i);
                            if (!activeItems.hasPower(p)) continue;
                            // Extra Slot is a one-time unlock and not sellable.
                            if (p == PowerType::ExtraPowerSlot) continue;

                            int count = 1;
                            if (p == PowerType::Duplicate) count = std::max(0, activeItems.duplicateCharges);
                            else if (p == PowerType::SwapPins) count = std::max(0, activeItems.swapCharges);
                            else if (p == PowerType::SevenEightNine) count = std::max(0, activeItems.sevenEightNineCharges);

                            for (int n = 0; n < count; n++) out.push_back(p);
                        }
                        return out;
                    };

                    auto sellOnePower = [&](PowerType p) {
                        bool sold = false;
                        switch (p) {
                            case PowerType::Duplicate:
                                if (activeItems.duplicateCharges > 0) {
                                    activeItems.duplicateCharges--;
                                    erasePowerRecord(PowerType::Duplicate, false);
                                    sold = true;
                                }
                                break;
                            case PowerType::SwapPins:
                                if (activeItems.swapCharges > 0) {
                                    activeItems.swapCharges--;
                                    erasePowerRecord(PowerType::SwapPins, false);
                                    sold = true;
                                }
                                break;
                            case PowerType::SevenEightNine:
                                if (activeItems.sevenEightNineCharges > 0) {
                                    activeItems.sevenEightNineCharges--;
                                    erasePowerRecord(PowerType::SevenEightNine, false);
                                    sold = true;
                                }
                                break;
                            case PowerType::Greedy:
                                activeItems.powerGreedy = false;
                                erasePowerRecord(PowerType::Greedy, true);
                                sold = true;
                                break;
                            case PowerType::RandomUpgrade:
                                activeItems.powerRandomUpgrade = false;
                                erasePowerRecord(PowerType::RandomUpgrade, true);
                                sold = true;
                                break;
                            case PowerType::ExtraPins:
                                activeItems.powerExtraPins = false;
                                erasePowerRecord(PowerType::ExtraPins, true);
                                sold = true;
                                break;
                            case PowerType::ExtraBall:
                                activeItems.powerExtraBall = false;
                                erasePowerRecord(PowerType::ExtraBall, true);
                                sold = true;
                                break;
                            case PowerType::Bumpers:
                                activeItems.powerBumpers = false;
                                lane.bumpersOn = (ui.getState() == GameState::Playing) ? ui.getBumpersDefault() : false;
                                erasePowerRecord(PowerType::Bumpers, true);
                                sold = true;
                                break;
                            case PowerType::HomeBase:
                                activeItems.powerHomeBase = false;
                                activeItems.homeBaseComboBonus = 0.0f;
                                activeItems.homeBasePinsTowardNextCombo = 0;
                                erasePowerRecord(PowerType::HomeBase, true);
                                sold = true;
                                break;
                            case PowerType::Confusion:
                                activeItems.powerConfusion = false;
                                erasePowerRecord(PowerType::Confusion, true);
                                sold = true;
                                break;
                            case PowerType::Earthquake:
                                activeItems.powerEarthquake = false;
                                activeItems.earthquakeShotCounter = 0;
                                erasePowerRecord(PowerType::Earthquake, true);
                                sold = true;
                                break;
                            case PowerType::Skip:
                                activeItems.powerSkip = false;
                                activeItems.skipCharges = 0;
                                erasePowerRecord(PowerType::Skip, true);
                                sold = true;
                                break;
                            case PowerType::UpgradesForEveryone:
                                activeItems.powerUpgradesForEveryone = false;
                                erasePowerRecord(PowerType::UpgradesForEveryone, true);
                                sold = true;
                                break;
                            case PowerType::Sales:
                                activeItems.powerSales = false;
                                erasePowerRecord(PowerType::Sales, true);
                                sold = true;
                                break;
                            case PowerType::PassedGo:
                                activeItems.powerPassedGo = false;
                                erasePowerRecord(PowerType::PassedGo, true);
                                sold = true;
                                break;
                            case PowerType::MoMoney:
                                activeItems.powerMoMoney = false;
                                erasePowerRecord(PowerType::MoMoney, true);
                                sold = true;
                                break;
                            case PowerType::ExtraPowerSlot:
                                // One-time power: no selling once purchased.
                                break;
                        }
                        if (sold) {
                            int sellValue = powerTypeCost(p) / 2;
                            if (sellValue > 0) xtreme.addTokens(sellValue);
                            syncRunPowersToScorer();
                        }
                    };

                    if (purchased == UI::ShopActionReroll) {
                        if (activeItems.skipCharges > 0 && xtreme.getTokens() >= 1) {
                            xtreme.addTokens(-1);
                            activeItems.skipCharges--;
                            ui.generateShopOffers(activeItems);
                        }
                    } else if (purchased == UI::ShopActionSellPin) {
                        std::vector<ActiveItems::PinSlotAssignment> sortedPins =
                            activeItems.getSortedPinAssignments();
                        if (!sortedPins.empty()) {
                            ActiveItems::PinSlotAssignment toSell = sortedPins.back();
                            activeItems.removePinAssignmentAtSlot(toSell.slot);
                            int sellValue = pinTypeCost(toSell.type) / 2;
                            if (sellValue > 0) xtreme.addTokens(sellValue);
                        }
                    } else if (purchased <= UI::ShopActionSellPinByIndexBase &&
                               purchased > UI::ShopActionSellPowerByIndexBase) {
                        int pinIndex = UI::ShopActionSellPinByIndexBase - purchased;
                        std::vector<ActiveItems::PinSlotAssignment> sortedPins =
                            activeItems.getSortedPinAssignments();
                        if (pinIndex >= 0 && pinIndex < (int)sortedPins.size()) {
                            ActiveItems::PinSlotAssignment toSell = sortedPins[pinIndex];
                            activeItems.removePinAssignmentAtSlot(toSell.slot);
                            int sellValue = pinTypeCost(toSell.type) / 2;
                            if (sellValue > 0) xtreme.addTokens(sellValue);
                        }
                    } else if (purchased == UI::ShopActionSellBallSlot1 ||
                               purchased == UI::ShopActionSellBallSlot2 ||
                               purchased == UI::ShopActionSellBallSlot3) {
                        int slot = 1;
                        if (purchased == UI::ShopActionSellBallSlot2) slot = 2;
                        else if (purchased == UI::ShopActionSellBallSlot3) slot = 3;
                        BallType owned = activeItems.getBallForSlot(slot);
                        if (owned != BallType::Normal) {
                            activeItems.setBallForSlot(slot, BallType::Normal);
                            int sellValue = ballTypeCost(owned) / 2;
                            if (sellValue > 0) xtreme.addTokens(sellValue);
                        }
                    } else if (purchased == UI::ShopActionSellShoe) {
                        if (activeItems.shoeType != ShoeType::None) {
                            ShoeType owned = activeItems.shoeType;
                            activeItems.applyShoeType(ShoeType::None);
                            int sellValue = shoeTypeCost(owned) / 2;
                            if (sellValue > 0) xtreme.addTokens(sellValue);
                        }
                    } else if (purchased == UI::ShopActionSellPower) {
                        std::vector<PowerType> sellable = buildSellablePowerList();
                        if (!sellable.empty()) {
                            sellOnePower(sellable.back());
                        }
                    } else if (purchased <= UI::ShopActionSellPowerByIndexBase) {
                        int powerIndex = UI::ShopActionSellPowerByIndexBase - purchased;
                        std::vector<PowerType> sellable = buildSellablePowerList();
                        if (powerIndex >= 0 && powerIndex < (int)sellable.size()) {
                            sellOnePower(sellable[powerIndex]);
                        }
                    } else if (purchased >= 0) {
                        const auto& offers = ui.getShopOffers();
                        if (purchased >= (int)offers.size()) {
                            continue;
                        }
                        const auto& offer = offers[purchased];
                        bool canBuy = true;

                        if (offer.category == ShopItemCategory::Ball) {
                            bool hasExtraBall = activeItems.powerExtraBall ||
                                                activeItems.hasPurchasedPower(PowerType::ExtraBall);
                            int slotLimit = hasExtraBall ? 3 : 2;
                            int slot = std::clamp(ui.getSelectedBallSlot(), 1, slotLimit);
                            if (activeItems.getBallForSlot(slot) == offer.ballType) {
                                canBuy = false;
                            }
                        }
                        if (offer.category == ShopItemCategory::Shoe) {
                            if (activeItems.shoeType == offer.shoeType) {
                                canBuy = false;
                            }
                            if (offer.shoeType == ShoeType::Clown &&
                                activeItems.clownShoesPurchased) {
                                canBuy = false;
                            }
                        }
                        if (offer.category == ShopItemCategory::Power) {
                            const int maxPermanentPowers = activeItems.getMaxPermanentPowerSlots();
                            bool alreadyOwned = activeItems.hasPower(offer.powerType) ||
                                                activeItems.hasPurchasedPower(offer.powerType);
                            if (!isStackablePower(offer.powerType) && alreadyOwned) {
                                canBuy = false;
                            }
                            if (offer.powerType == PowerType::Duplicate &&
                                activeItems.duplicateCharges > 0) {
                                // Duplicate is now forced-use; don't allow stockpiling.
                                canBuy = false;
                            }
                            if (powerCountsTowardLimit(offer.powerType) &&
                                !alreadyOwned &&
                                ownedPermanentPowerCount() >= maxPermanentPowers) {
                                canBuy = false;
                            }
                        }
                        if (offer.category == ShopItemCategory::Pin) {
                            bool hasExtraPins = activeItems.powerExtraPins ||
                                                activeItems.hasPurchasedPower(PowerType::ExtraPins);
                            int pinLimit = hasExtraPins ? 12 : 10;
                            int targetSlot = std::clamp(ui.getSelectedPinSlot(), 1, pinLimit);
                            bool slotEmpty = !activeItems.hasPinAssignmentAtSlot(targetSlot);
                            if (!slotEmpty &&
                                activeItems.getPinTypeForSlot(targetSlot) == offer.pinType) {
                                canBuy = false;
                            }
                            if (!slotEmpty &&
                                activeItems.getPinTypeForSlot(targetSlot) != offer.pinType) {
                                // Slot is locked until the player explicitly sells it.
                                canBuy = false;
                            }
                            if (slotEmpty && activeItems.getPinAssignmentCount() >= pinLimit) {
                                canBuy = false;
                            }
                        }
                        if (xtreme.getTokens() < offer.cost) {
                            canBuy = false;
                        }

                        if (!canBuy) {
                            // blocked by caps/rules; ignore click
                        } else if (offer.category == ShopItemCategory::Ball) {
                            xtreme.addTokens(-offer.cost);
                            bool hasExtraBall = activeItems.powerExtraBall ||
                                                activeItems.hasPurchasedPower(PowerType::ExtraBall);
                            int slotLimit = hasExtraBall ? 3 : 2;
                            int slot = std::clamp(ui.getSelectedBallSlot(), 1, slotLimit);
                            BallType oldBall = activeItems.getBallForSlot(slot);
                            if (oldBall != BallType::Normal && oldBall != offer.ballType) {
                                xtreme.addTokens(ballTypeCost(oldBall) / 2);
                            }
                            activeItems.setBallForSlot(slot, offer.ballType);
                            ui.recordOfferPicked(offer);
                        } else if (offer.category == ShopItemCategory::Shoe) {
                            xtreme.addTokens(-offer.cost);
                            bool changedShoes = (activeItems.shoeType != offer.shoeType);
                            activeItems.applyShoeType(offer.shoeType);
                            if (offer.shoeType == ShoeType::Clown &&
                                changedShoes &&
                                !activeItems.clownBonusClaimed) {
                                xtreme.addTokens(10);
                                activeItems.clownBonusClaimed = true;
                            }
                            if (offer.shoeType == ShoeType::Clown) {
                                activeItems.clownShoesPurchased = true;
                            }
                            ui.recordOfferPicked(offer);
                        } else if (offer.category == ShopItemCategory::Pin) {
                            xtreme.addTokens(-offer.cost);
                            bool hasExtraPins = activeItems.powerExtraPins ||
                                                activeItems.hasPurchasedPower(PowerType::ExtraPins);
                            int pinLimit = hasExtraPins ? 12 : 10;
                            int targetSlot = std::clamp(ui.getSelectedPinSlot(), 1, pinLimit);
                            if (!activeItems.hasPinAssignmentAtSlot(targetSlot)) {
                                activeItems.setPinAssignment(targetSlot, offer.pinType);
                            }
                            ui.recordOfferPicked(offer);
                        } else {
                            xtreme.addTokens(-offer.cost);
                            activeItems.applyPower(offer.powerType);
                            if (offer.powerType == PowerType::Bumpers) {
                                lane.bumpersOn = true;
                            }
                            syncRunPowersToScorer();
                            ui.recordOfferPicked(offer);
                        }
                    }

                    // Continue button
                    const float continuePad = 12.0f;
                    if (worldPos.x > windowW/2.f - 110.f - continuePad && worldPos.x < windowW/2.f + 110.f + continuePad &&
                        worldPos.y > windowH - 100.f - continuePad    && worldPos.y < windowH - 40.f + continuePad) {
                        ui.setState(GameState::Xtreme);
                        postScorePauseTimer = 0.0f;
                        xtremeRoundStartTokens = xtreme.getTokens();
                        ui.generateShopOffers(activeItems);
                        // Fresh pins for the new round with purchased types applied
                        pins = createPins(lane.centerX(), 240.0f);
                        applyPurchasedPinTypes(pins);
                        applyPowerPinLayout(pins);
                        cancelPinPowerSelection();
                        resetBall();
                        openBossIntroIfNeeded();
                    }
                }
            }
        }
    }
}

void Game::resetBall() {
    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    rollLocked = false;
    rollDir = sf::Vector2f(0.0f, -1.0f);
    aimDeg = -90.0f;
    inGutter = false;
    gutterSide = 0;
    shotRollTimer = 0.0f;
    backlineJamTimer = 0.0f;

    // Reset per-shot item state
    activeItems.resetForNewShot();

    if (ui.getState() == GameState::Xtreme) {
        refreshBossStateForCurrentRound();
        rollBossDisabledPowerForShot();

        bool hasExtraBall = isPowerEnabledThisShot(PowerType::ExtraBall);
        bool hasPassedGo = isPowerEnabledThisShot(PowerType::PassedGo);
        bool hasMoMoney = isPowerEnabledThisShot(PowerType::MoMoney);
        xtreme.setExtraBallEnabled(hasExtraBall);
        xtreme.setPowerPassedGo(hasPassedGo);
        xtreme.setPowerMoMoney(hasMoMoney);

        float homeBaseBonus = isPowerEnabledThisShot(PowerType::HomeBase)
            ? std::min(activeItems.homeBaseComboBonus, kHomeBaseComboBonusCap)
            : 0.0f;
        float baseComboFloor =
            (activeBoss == BossType::LowBaseScore && activeBossRound == xtreme.getRound())
                ? 0.5f
                : 1.0f;
        xtreme.setBaseCombo(baseComboFloor + homeBaseBonus);
        xtreme.setBaseImpact(
            (activeBoss == BossType::LowBaseScore && activeBossRound == xtreme.getRound()) ? 5 : 10);

        lane.bumpersOn = isPowerEnabledThisShot(PowerType::Bumpers);
    } else if (activeItems.powerBumpers) {
        lane.bumpersOn = ui.getBumpersDefault() || activeItems.powerBumpers;
    }

    // Pick the active ball based on mode + shot number.
    // Xtreme maps shot 1/2/3 to Ball slot 1/2/3.
    if (ui.getState() == GameState::Xtreme) {
        int shot = xtreme.getShotInFrame();
        BallType shotBall = activeItems.getBallForShot(shot);
        equipBall(shotBall);
    } else {
        equipBall(activeItems.getBallForShot(1));
    }
    ball.setSlideMultiplier(activeItems.slideMultiplier);
    for (auto& pin : pins) {
        pin.setSlideMultiplier(activeItems.slideMultiplier);
    }

    // Prepare pin special behaviours for the new shot
    prepareNewShot();
    activeItems.pinsStandingAtShotStart = countStandingPins();

    // Stop rolling sound
    audio.stopBallRoll();
}


void Game::equipBall(BallType type) {
    activeItems.applyBallType(type);
    ball.setBallType(type);
    ball.applyItemMultipliers(activeItems.radiusMultiplier, activeItems.massMultiplier);
    switch (type) {
        case BallType::BlackHole:  ball.setColor({10,  0,   20});  break;
        case BallType::Midas:      ball.setColor({210, 170, 20});  break;
        case BallType::Upgrade:    ball.setColor({30,  80,  200}); break;
        case BallType::Heavy:      ball.setColor({60,  60,  65});  break;
        case BallType::Fastball:   ball.setColor({240, 240, 240}); break;
        case BallType::OddBall:    ball.setColor({60,  180, 60});  break;
        case BallType::EightBall:  ball.setColor({10,  10,  10});  break;
        case BallType::Icy:        ball.setColor({165, 225, 255}); break;
        case BallType::Retrigger:  ball.setColor({160, 170, 180}); break;
        default:                   ball.setColor({25,  55,  140}); break;
    }
}

void Game::applyBlackHoleGravity(float dt) {
    if (activeItems.ballType != BallType::BlackHole) return;
    if (!rollLocked) return;
    sf::Vector2f ballPos = ball.getPos();
    const float pullStrength = 180.0f;
    for (auto& pin : pins) {
        if (!pin.isActive() || pin.isFallen()) continue;
        sf::Vector2f diff = ballPos - pin.getPos();
        float dist = std::sqrt(diff.x*diff.x + diff.y*diff.y);
        if (dist < 1.f) continue;
        float force = pullStrength / (dist * 0.06f + 1.f);
        sf::Vector2f pull = (diff / dist) * force * dt;
        pin.setVel(pin.getVel() + pull);
    }
}

int Game::computePinValueWithItems(int pinIndex) const {
    int val = pins[pinIndex].getValue();
    if (pins[pinIndex].getPinType() == PinType::LevelUp) {
        val += 1;
    }
    switch (activeItems.ballType) {
        case BallType::EightBall: val = 8; break;
        case BallType::OddBall:
            val = (val % 2 != 0) ? (val * 2) : std::max(1, (val * 3 + 2) / 4);
            break;
        default: break;
    }
    return val;
}

int Game::computePinValueSumWithItems(const std::vector<int>& hitIndices) {
    int total = 0;
    int icyPinsHit = 0;
    for (int idx : hitIndices) {
        total += computePinValueWithItems(idx);
        if (pins[idx].getPinType() == PinType::Ice) {
            icyPinsHit++;
        }
    }
    // Retrigger means the 2nd pin hit scores 3x total:
    // 1x from the normal sum above, plus 2x extra here.
    if (activeItems.ballType == BallType::Retrigger && activeItems.retriggered)
        total += activeItems.retriggeredValue * 2;
    if (activeItems.ballType == BallType::Icy) {
        total += icyPinsHit * 15;
    }
    return total;
}

void Game::processExplosions() {
    const float blastRadius = 108.f;
    const float blastForce  = 900.f * 0.74f * 0.90f;

    for (auto& pin : pins) {
        if (!pin.isActive()) continue;
        if (pin.getPinType() != PinType::Exploding) continue;
        if (!pin.shouldExplode()) continue;

        pin.markExploded();
        audio.playExplodingPin(82.0f);
        sf::Vector2f ep = pin.getPos();

        // Knock over and push nearby pins
        for (auto& other : pins) {
            if (!other.isActive()) continue;
            if (&other == &pin) continue;

            sf::Vector2f diff = other.getPos() - ep;
            float dist = std::sqrt(diff.x*diff.x + diff.y*diff.y);
            if (dist < blastRadius && dist > 0.01f) {
                sf::Vector2f dir = diff / dist;
                float strength = blastForce * (1.f - dist / blastRadius);
                other.setVel(other.getVel() + dir * strength);
                if (!other.isFallen()) {
                    other.setFallen(true);
                    other.setAngularVel(((rand() % 200) - 100) * 0.05f);
                }
            }
        }
    }
}

void Game::prepareNewShot() {
    for (auto& pin : pins) {
        if (!pin.isActive()) continue;
        pin.rollLuckyDucky();
        if (!(activeItems.lockPinChangesMidRound && ui.getState() == GameState::Xtreme))
            pin.randomiseMischievous();
    }

    // 7 8 9 power: one-time effect if rack includes slot 9.
    if (activeItems.sevenEightNineCharges > 0) {
        int maxSlot = std::min((int)pins.size(), activeItems.getActivePinSlotCount());
        constexpr int idx7 = 6;
        constexpr int idx9 = 8;
        if (maxSlot > idx9 &&
            pins[idx7].isActive() &&
            pins[idx9].isActive()) {
            pins[idx7].setValue(pins[idx7].getValue() * 3);
            pins[idx9].setFallen(true);
            pins[idx9].setVel({0.0f, 0.0f});
            pins[idx9].setAngularVel(0.0f);
            pins[idx9].setActive(false);
            activeItems.sevenEightNinePin9Destroyed = true;
            activeItems.sevenEightNineCharges--;
        }
    }

    // Persist 7 8 9 destruction across reracks for the rest of the run.
    if (activeItems.sevenEightNinePin9Destroyed) {
        int maxSlot = std::min((int)pins.size(), activeItems.getActivePinSlotCount());
        constexpr int idx9 = 8;
        if (maxSlot > idx9 && pins[idx9].isActive()) {
            pins[idx9].setFallen(true);
            pins[idx9].setVel({0.0f, 0.0f});
            pins[idx9].setAngularVel(0.0f);
            pins[idx9].setActive(false);
        }
    }

    updatePinSlotValueSnapshot(pins);
}

void Game::startPendingReset() {
    pendingReset = true;
    pendingShotScored = false;
    pendingScoreVisualTimer = 0.0f;
    pendingScoreVisualDuration = 0.0f;
    pendingPhysicalPinsDownThisShot = 0;
    pendingStrikeThisShot = false;
    pendingRoundScoreBeforeShot = 0;
    pendingTargetBeforeShot = 0;
    resetTimer = 0.0f;
    shotRollTimer = 0.0f;
    backlineJamTimer = 0.0f;
}


void Game::finishPendingResetIfReady(float dt) {
    if (!pendingReset) return;

    if (!pendingShotScored) {
        resetTimer += dt;

        bool pinsStill = true;
        for (const auto& pin : pins) {
            if (!pin.isActive()) continue;
            if (length(pin.getVel()) > pinsStillSpeed) {
                pinsStill = false;
                break;
            }
        }

        bool ready = (pinsStill && resetTimer >= endBuffer) || (resetTimer >= maxResetWait);
        if (!ready) return;

        // Count pins knocked THIS ball and apply item modifiers
        int knockedThisBall = 0;
        std::vector<int> hitPinIndices;
        for (int i = 0; i < (int)pins.size(); i++) {
            if (!pins[i].isActive()) continue;
            if (pins[i].isFallen()) {
                knockedThisBall++;
                hitPinIndices.push_back(i);
            }
        }
        int physicalPinsDownThisShot = knockedThisBall;
        int shotBeforeRecord = (ui.getState() == GameState::Xtreme) ? xtreme.getShotInFrame() : 0;
        int frameBeforeRecord = (ui.getState() == GameState::Xtreme) ? xtreme.getFrameInRound() : 0;
        int roundBeforeRecord = (ui.getState() == GameState::Xtreme) ? xtreme.getRound() : 0;
        int roundScoreBeforeShot = (ui.getState() == GameState::Xtreme) ? xtreme.getRoundScore() : 0;
        int targetBeforeShot = (ui.getState() == GameState::Xtreme) ? xtreme.getTargetScore() : 0;
        bool earthquakeStrikeThisShot = false;

        if (ui.getState() == GameState::Xtreme && isPowerEnabledThisShot(PowerType::Earthquake)) {
            activeItems.earthquakeShotCounter++;
            if (activeItems.earthquakeShotCounter >= 10) {
                activeItems.earthquakeShotCounter = 0;
                earthquakeStrikeThisShot = true;

                // Force remaining standing pins down and count them in this shot.
                for (int i = 0; i < (int)pins.size(); i++) {
                    if (!pins[i].isActive() || pins[i].isFallen()) continue;
                    pins[i].setFallen(true);
                    hitPinIndices.push_back(i);
                    physicalPinsDownThisShot++;
                }
                knockedThisBall = physicalPinsDownThisShot;
            }
        }

        // ── Special pin effects at score time ────────────────────────────────
        for (int idx : hitPinIndices) {
            if (pins[idx].getPinType() == PinType::Gold) {
                xtreme.addTokens(1);
            }
        }

        for (int idx : hitPinIndices) {
            if (pins[idx].getPinType() == PinType::ThirdTime) {
                activeItems.thirdTimeGlobalKnocks++;
                if (activeItems.thirdTimeGlobalKnocks % 3 == 0) {
                    activeItems.thirdTimeComboBonus += 1; // one extra combo doubling
                }
            }
        }

        // Lover pin: each hit adds +1 value to the next slot pin.
        {
            int maxSlot = std::min((int)pins.size(), activeItems.getActivePinSlotCount());
            for (int idx : hitPinIndices) {
                if (idx < 0 || idx >= maxSlot) continue;
                if (pins[idx].getPinType() != PinType::Lover) continue;
                int nextIdx = idx + 1;
                if (nextIdx < 0 || nextIdx >= maxSlot) continue;
                if (!pins[nextIdx].isActive()) continue;
                pins[nextIdx].setValue(pins[nextIdx].getValue() + 1);
                if (nextIdx < (int)activeItems.pinChangeHitCountsThisShot.size()) {
                    activeItems.pinChangeEventsThisShot++;
                    activeItems.pinChangeHitCountsThisShot[nextIdx]++;
                }
            }
        }

        // Change Is Good: gains +2 for each changed "other" pin this shot.
        if (activeItems.pinChangeEventsThisShot > 0) {
            int maxSlot = std::min((int)pins.size(), activeItems.getActivePinSlotCount());
            for (int i = 0; i < maxSlot; i++) {
                if (!pins[i].isActive()) continue;
                if (pins[i].getPinType() != PinType::ChangeIsGood) continue;
                int ownChanges = 0;
                if (i < (int)activeItems.pinChangeHitCountsThisShot.size()) {
                    ownChanges = activeItems.pinChangeHitCountsThisShot[i];
                }
                int otherChanges = std::max(0, activeItems.pinChangeEventsThisShot - ownChanges);
                if (otherChanges > 0) {
                    pins[i].setValue(pins[i].getValue() + otherChanges * 2);
                }
            }
        }

        int pinValueSumThisBall = computePinValueSumWithItems(hitPinIndices);
        for (int idx : hitPinIndices) {
            if (pins[idx].getPinType() == PinType::LuckyDucky && pins[idx].isLuckyZero()) {
                pinValueSumThisBall -= computePinValueWithItems(idx);
            }
        }
        if (pinValueSumThisBall < 0) pinValueSumThisBall = 0;

        int greedyComboBonus = 0;
        if (ui.getState() == GameState::Xtreme && isPowerEnabledThisShot(PowerType::Greedy)) {
            greedyComboBonus = std::max(0, xtreme.getTokens() / 4);
        }

        int thirdTimeComboMultiplier = 1;
        if (activeItems.thirdTimeComboBonus > 0 && knockedThisBall > 0) {
            int doublings = std::min(activeItems.thirdTimeComboBonus, 4); // cap at x16
            thirdTimeComboMultiplier = (1 << doublings);
        }

        if (inGutter && knockedThisBall == 0) {
            knockedThisBall = 0;
        }

        bool strikeThisShot = false;
        bool triggerStrikeFx = false;
        if (ui.getState() == GameState::Xtreme) {
            // shotBeforeRecord is intentionally captured before recordShot(),
            // because recordShot advances the internal shot counter.
            strikeThisShot = (shotBeforeRecord == 1 &&
                              physicalPinsDownThisShot >= activeItems.pinsStandingAtShotStart);
            bool spareThisShot = (shotBeforeRecord > 1 &&
                                  physicalPinsDownThisShot >= activeItems.pinsStandingAtShotStart);
            if (isPowerEnabledThisShot(PowerType::Confusion) && spareThisShot) {
                strikeThisShot = true;
            }
            if (earthquakeStrikeThisShot) {
                strikeThisShot = true;
            }
            bool spareBonusThisShot = spareThisShot && !strikeThisShot;
            xtreme.recordShot(
                knockedThisBall,
                pinValueSumThisBall,
                strikeThisShot,
                spareBonusThisShot,
                static_cast<float>(thirdTimeComboMultiplier),
                greedyComboBonus);

            triggerStrikeFx = strikeThisShot;

            if (isPowerEnabledThisShot(PowerType::HomeBase) && physicalPinsDownThisShot > 0) {
                activeItems.homeBasePinsTowardNextCombo += physicalPinsDownThisShot;
                while (activeItems.homeBasePinsTowardNextCombo >= 20) {
                    activeItems.homeBasePinsTowardNextCombo -= 20;
                    if (activeItems.homeBaseComboBonus < kHomeBaseComboBonusCap) {
                        activeItems.homeBaseComboBonus =
                            std::min(kHomeBaseComboBonusCap, activeItems.homeBaseComboBonus + 1.0f);
                    }
                }
                float clampedBonus = std::min(activeItems.homeBaseComboBonus, kHomeBaseComboBonusCap);
                float baseComboFloor =
                    (activeBoss == BossType::LowBaseScore && activeBossRound == xtreme.getRound())
                        ? 0.5f
                        : 1.0f;
                xtreme.setBaseCombo(baseComboFloor + clampedBonus);
            }

            // Random upgrade applies once after each completed frame.
            if (xtreme.getShotInFrame() == 1 && isPowerEnabledThisShot(PowerType::RandomUpgrade)) {
                activeItems.pendingRandomPinUpgrades += 1;
            }
        } else {
            int normalFrameBefore = scorer.getCurrentFrame();
            int normalBallBefore = scorer.getCurrentBall();
            bool tenthBonusBallStrike =
                (normalFrameBefore == 9 && normalBallBefore == 2 && scorer.getFrames()[9].ball1 == 10) ||
                (normalFrameBefore == 9 && normalBallBefore == 3 &&
                 (scorer.getFrames()[9].ball1 == 10 || scorer.getFrames()[9].isSpare));
            triggerStrikeFx =
                (physicalPinsDownThisShot >= activeItems.pinsStandingAtShotStart) &&
                ((normalBallBefore == 1) || tenthBonusBallStrike);
            scorer.recordBall(knockedThisBall);
        }

        if (triggerStrikeFx) {
            triggerStrikeCelebration();
        }

        pendingPhysicalPinsDownThisShot = physicalPinsDownThisShot;
        pendingStrikeThisShot = strikeThisShot;
        pendingRoundScoreBeforeShot = roundScoreBeforeShot;
        pendingTargetBeforeShot = targetBeforeShot;
        pendingDisplayRoundBeforeShot = roundBeforeRecord;
        pendingDisplayFrameBeforeShot = frameBeforeRecord;
        pendingDisplayShotBeforeShot = shotBeforeRecord;
        pendingShotScored = true;
        pendingScoreVisualTimer = 0.0f;
        pendingScoreVisualDuration = 0.0f;

        if (ui.getState() == GameState::Xtreme) {
            // Keep shot/frame transitions waiting until the HUD scoring animation finishes.
            int impactTarget = std::max(10, xtreme.getLastImpact());
            int comboTarget = std::max(1, xtreme.getLastCombo());
            int shotTarget = std::max(0, xtreme.getLastShotScore());
            int formulaDelta = (impactTarget - 10) + (comboTarget - 1) * 3;
            float formulaDuration = std::clamp(0.25f + (float)formulaDelta * 0.02f, 0.25f, 0.75f);
            float countDuration = std::clamp(0.30f + (float)shotTarget / 320.0f, 0.30f, 1.10f);
            float bigDuration = (shotTarget > 0) ? 0.9f : 0.0f;
            pendingScoreVisualDuration = formulaDuration + countDuration + bigDuration + 0.05f;

            // Freeze motion while score animation is shown.
            ball.stop();
            for (auto& pin : pins) {
                if (!pin.isActive()) continue;
                pin.setVel({0.0f, 0.0f});
                pin.setAngularVel(0.0f);
            }
        }

        if (pendingScoreVisualDuration > 0.0f) {
            return;
        }
    } else {
        pendingScoreVisualTimer += dt;
        if (pendingScoreVisualTimer < pendingScoreVisualDuration) {
            return;
        }
    }

    // Handle pin resets based on game state
    if (ui.getState() == GameState::Xtreme) {
        if (xtreme.getShotInFrame() > 1) {
            // Keep frame progression shot-based. After a strike (or full clear),
            // rerack for the next shot in the same frame.
            bool rackCleared = (pendingPhysicalPinsDownThisShot >= activeItems.pinsStandingAtShotStart);
            bool shouldRerack = pendingStrikeThisShot || rackCleared;
            if (shouldRerack) {
                pins = createPins(lane.centerX(), 240.0f);
                applyPurchasedPinTypes(pins);
                applyPowerPinLayout(pins);
            } else {
                // Non-final shot done: deactivate fallen, reset standing.
                for (auto& pin : pins) {
                    if (pin.isFallen()) {
                        pin.setActive(false);
                    } else if (pin.isActive()) {
                        if (activeItems.lockPinChangesMidRound) {
                            pin.resetToOriginalPositionKeepType();
                        } else {
                            pin.resetToOriginalPosition();
                        }
                    }
                }
                // Re-apply purchased types to active pins unless shoes lock pin changes
                if (!activeItems.lockPinChangesMidRound) {
                    applyPurchasedPinTypes(pins);
                }
                applyPowerPinLayout(pins);
            }
        } else {
            // Final shot in frame done: fresh pins with purchased types.
            pins = createPins(lane.centerX(), 240.0f);
            applyPurchasedPinTypes(pins);
            applyPowerPinLayout(pins);
        }
    } else if (scorer.getCurrentFrame() < 10) {
        // Get the previous frame info to determine what to do
        int prevFrame = scorer.getCurrentFrame() > 0 ? scorer.getCurrentFrame() - 1 : 0;
        const auto& frames = scorer.getFrames();
        
        if (scorer.getCurrentBall() == 1 && scorer.getCurrentFrame() > 0 && 
            frames[prevFrame].isStrike) {
            // After a strike, reset all pins for new frame
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
        } else if (scorer.getCurrentBall() == 2) {
            // After ball 1 (not strike), remove fallen, reset standing
            for (auto& pin : pins) {
                if (pin.isFallen()) {
                    pin.setActive(false);
                } else if (pin.isActive()) {
                    pin.resetToOriginalPosition();
                }
            }
        } else if (scorer.getCurrentBall() == 1 && scorer.getCurrentFrame() > prevFrame) {
            // Frame advanced (spare or normal), reset all pins
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
        }
    } else {
        // 10th frame special handling
        const auto& frame10 = scorer.getFrames()[9];
        
        if (scorer.getCurrentBall() == 2) {
            if (frame10.isStrike) {
                // After strike on ball 1, reset all
                for (auto& pin : pins) {
                    pin.setActive(true);
                    pin.resetToOriginalPosition();
                }
            } else {
                // Not strike, remove fallen
                for (auto& pin : pins) {
                    if (pin.isFallen()) {
                        pin.setActive(false);
                    } else if (pin.isActive()) {
                        pin.resetToOriginalPosition();
                    }
                }
            }
        } else if (scorer.getCurrentBall() == 3) {
            if (frame10.isSpare || frame10.ball2 == 10) {
                // After spare or strike on ball 2, reset all
                for (auto& pin : pins) {
                    pin.setActive(true);
                    pin.resetToOriginalPosition();
                }
            } else if (frame10.isStrike && frame10.ball2 != 10) {
                // After strike but ball 2 wasn't strike, remove fallen
                for (auto& pin : pins) {
                    if (pin.isFallen()) {
                        pin.setActive(false);
                    } else if (pin.isActive()) {
                        pin.resetToOriginalPosition();
                    }
                }
            }
        }
    }

    updatePinSlotValueSnapshot(pins);

    // Check for transitions (game over / shop) and optionally pause so
    // players can see the score animation before screens change.
    bool needsPostScorePause = false;
    if (ui.getState() == GameState::Xtreme) {
        if (xtreme.isGameOver()) {
            bool lostOnActiveBossRound =
                (activeBoss != BossType::None &&
                 activeBossRound == xtreme.getRound() &&
                 isBossRound(xtreme.getRound()));
            if (lostOnActiveBossRound) {
                playBossLaughForCurrentBoss(100.0f);
            }

            finalXtremeRoundsCleared = std::max(0, xtreme.getRound() - 1);
            xtremeLastRoundScoreProgress = pendingRoundScoreBeforeShot + xtreme.getLastShotScore();
            xtremeLastRoundTargetProgress = pendingTargetBeforeShot;
            roundSummaryRoundNumber = xtreme.getRound();
            roundSummaryScore = xtremeLastRoundScoreProgress;
            roundSummaryTarget = xtremeLastRoundTargetProgress;
            roundSummaryTokensEarned = xtreme.getTokens() - xtremeRoundStartTokens;
            roundSummaryTokensTotal = xtreme.getTokens();
            roundSummaryPassed = false;
            roundSummaryLeadsToGameOver = true;
            ui.setState(GameState::RoundSummary);
            cancelPinPowerSelection();
            pendingGameOverFromScore = false;
            postScorePauseTimer = 0.0f;

            if (finalXtremeRoundsCleared > xtremeBestRound) {
                xtremeBestRound = finalXtremeRoundsCleared;
                saveHighScore();
            }
        } else if (xtreme.isShopReady()) {
            xtremeLastRoundScoreProgress = pendingRoundScoreBeforeShot + xtreme.getLastShotScore();
            xtremeLastRoundTargetProgress = pendingTargetBeforeShot;
            roundSummaryRoundNumber = std::max(1, xtreme.getRound() - 1);
            roundSummaryScore = xtremeLastRoundScoreProgress;
            roundSummaryTarget = xtremeLastRoundTargetProgress;
            roundSummaryTokensEarned = xtreme.getTokens() - xtremeRoundStartTokens;
            roundSummaryTokensTotal = xtreme.getTokens();
            roundSummaryPassed = true;
            roundSummaryLeadsToGameOver = false;
            int bossesCleared = roundSummaryRoundNumber / 5;
            if (!endlessMode && bossesCleared >= 3) {
                ui.setState(GameState::GameWon);
            } else {
                ui.setState(GameState::RoundSummary);
            }
            cancelPinPowerSelection();
            pendingGameOverFromScore = false;
            postScorePauseTimer = 0.0f;
        }
    } else {
        if (scorer.isGameOver()) {
            pendingGameOverFromScore = true;
            needsPostScorePause = true;

            finalNormalScore = scorer.getTotalScore();

            if (finalNormalScore > normalHighScore) {
                normalHighScore = finalNormalScore;
                saveHighScore();
            }
        }
    }

    resetBall();
    pendingReset = false;
    pendingShotScored = false;
    pendingScoreVisualTimer = 0.0f;
    pendingScoreVisualDuration = 0.0f;
    pendingPhysicalPinsDownThisShot = 0;
    pendingStrikeThisShot = false;
    pendingRoundScoreBeforeShot = 0;
    pendingTargetBeforeShot = 0;
    pendingDisplayRoundBeforeShot = 0;
    pendingDisplayFrameBeforeShot = 0;
    pendingDisplayShotBeforeShot = 0;
    if (needsPostScorePause && ui.getState() != GameState::RoundSummary) {
        postScorePauseTimer = pendingGameOverFromScore
            ? postScorePauseDurationGameOver
            : postScorePauseDurationShop;
    }
}

void Game::applyGuttersAndBumpers() {
    sf::Vector2f p = ball.getPos();
    sf::Vector2f v = ball.getVel();
    float r = ball.getRadius();

    float playL = lane.playLeft();
    float playR = lane.playRight();

    if (inGutter) {
        float targetX = p.x;
        if (gutterSide == -1) targetX = lane.left + lane.gutterWidth * 0.5f;
        if (gutterSide ==  1) targetX = lane.right - lane.gutterWidth * 0.5f;

        // Smooth drift into gutter lane instead of hard snapping.
        p.x += (targetX - p.x) * 0.18f;
        v.x *= 0.75f;
        if (std::abs(v.x) < 6.0f) v.x = 0.0f;

        // Keep rolling forward, with gentle speed-up to drain naturally.
        float targetVy = -420.0f;
        v.y += (targetVy - v.y) * 0.14f;
    } else {
        bool bumpersActive = lane.bumpersOn || isPowerEnabledThisShot(PowerType::Bumpers);
        if (bumpersActive) {
            // Left bumper
            if (p.x < playL + r && v.x < 0.0f) {
                p.x = playL + r;
                sf::Vector2f n(1.0f, 0.0f);
                v = v - 2.0f * dot(v, n) * n;
                v *= 1.03f;
                ball.setVel(v);
            }

            // Right bumper
            if (p.x > playR - r && v.x > 0.0f) {
                p.x = playR - r;
                sf::Vector2f n(-1.0f, 0.0f);
                v = v - 2.0f * dot(v, n) * n;
                v *= 1.03f;
                ball.setVel(v);
            }
        } else {
            // No bumpers - enter gutter
            if (p.x < playL + r) { inGutter = true; gutterSide = -1; }
            if (p.x > playR - r) { inGutter = true; gutterSide =  1; }
        }
    }

    ball.setPos(p);
    ball.setVel(v);
}

void Game::doCollisions() {
    float bossPinMassMult = 1.0f;
    if (ui.getState() == GameState::Xtreme &&
        activeBoss == BossType::HeavyPins &&
        activeBossRound == xtreme.getRound()) {
        bossPinMassMult = 1.15f;
    }

    // Ball -> pin
    {
        sf::Vector2f bp = ball.getPos();
        sf::Vector2f bv = ball.getVel();
        sf::Vector2f bvStart = bv;
        float br = ball.getRadius();
        float bm = ball.getMass() * 1.07f;  // slight bias so pins affect ball a bit less
        const float pinFallImpactThreshold = 57.0f;

        bool hitAnyPin = false;

        for (int pi = 0; pi < (int)pins.size(); pi++) {
            auto& pin = pins[pi];
            if (!pin.isActive()) continue;

            sf::Vector2f pp = pin.getPos();
            sf::Vector2f pv = pin.getVel();
            sf::Vector2f pvBefore = pv;

            float pinCollisionRadius = pin.getRadius() * (pin.isFallen() ? 0.86f : 1.00f);
            resolveCircleCollision(
                bp, bv, bm, br,
                pp, pv,
                (pin.isFallen() ? pin.getMass() : pin.getMass() * 0.6f) *
                    activeItems.pinMassMultiplier * bossPinMassMult,
                pinCollisionRadius,
                0.12f
            );

            pin.setPos(pp);
            pin.setVel(pv);

            float impact = length(pv - pvBefore);
            if (impact > 5.0f) hitAnyPin = true;

            // Play pin hit sound
            if (impact > 50.0f) {
                float impactVolume = std::min(100.0f, 40.0f + impact * 0.5f);
                audio.playRandomPinHit(impactVolume);
            }

            // Pin first contact this shot with a small random threshold variance.
            float randomThreshold =
                pinFallImpactThreshold + static_cast<float>((rand() % 5) - 2); // +/-2
            bool freshImpact = (!pin.isFallen() && impact > randomThreshold);

            // CopyCat: becomes the type of the first hit pin.
            // Do this before setFallen so copying Exploding arms correctly.
            if (freshImpact &&
                pin.getPinType() == PinType::CopyCat &&
                activeItems.firstBallHitPinIndex != pi &&
                activeItems.firstBallHitPinIndex >= 0) {
                PinType copyFrom = pins[activeItems.firstBallHitPinIndex].getPinType();
                if (!activeItems.lockPinChangesMidRound && copyFrom != PinType::CopyCat) {
                    if (pin.getPinType() != copyFrom &&
                        pi >= 0 &&
                        pi < (int)activeItems.pinChangeHitCountsThisShot.size()) {
                        activeItems.pinChangeEventsThisShot++;
                        activeItems.pinChangeHitCountsThisShot[pi]++;
                    }
                    pin.setPinType(copyFrom);
                }
            }

            // Pin falls
            if (freshImpact) {
                pin.setFallen(true);
                float randomSpin = static_cast<float>((rand() % 401) - 200) * 0.01f;
                float spin = (bv.x - pv.x) * 0.011f + randomSpin;
                pin.setAngularVel(std::clamp(spin, -7.5f, 7.5f));

                // Small random tumble kick keeps pin-fall motion from looking mirrored.
                sf::Vector2f tumbleVel = pin.getVel();
                tumbleVel.x += static_cast<float>((rand() % 401) - 200) * 0.03f;
                tumbleVel.y += static_cast<float>((rand() % 201) - 100) * 0.02f;
                pin.setVel(tumbleVel);
            }

            // ── Item effects on first meaningful contact ──────────────────
            if (freshImpact || (impact > pinFallImpactThreshold && !pin.isFallen())) {

                activeItems.pinsHitThisShot++;

                // Track very first pin hit by ball (for CopyCat)
                if (activeItems.firstBallHitPinIndex == -1) {
                    activeItems.firstBallHitPinIndex = pi;
                }

                // Upgrade ball: each hit pin gains +1 value
                if (activeItems.ballType == BallType::Upgrade) {
                    pin.setValue(pin.getValue() + 1);
                    if (pi >= 0 &&
                        pi < (int)activeItems.pinChangeHitCountsThisShot.size()) {
                        activeItems.pinChangeEventsThisShot++;
                        activeItems.pinChangeHitCountsThisShot[pi]++;
                    }
                }

                // Midas ball: only the first pin hit this shot turns into Gold.
                if (activeItems.ballType == BallType::Midas) {
                    bool firstPinThisShot = (activeItems.firstBallHitPinIndex == pi);
                    if (firstPinThisShot &&
                        !activeItems.lockPinChangesMidRound &&
                        pin.getPinType() != PinType::Gold) {
                        if (pi >= 0 &&
                            pi < (int)activeItems.pinChangeHitCountsThisShot.size()) {
                            activeItems.pinChangeEventsThisShot++;
                            activeItems.pinChangeHitCountsThisShot[pi]++;
                        }
                        pin.setPinType(PinType::Gold);
                        if (pi + 1 >= 1 && pi + 1 <= activeItems.getActivePinSlotCount()) {
                            activeItems.setPinAssignment(pi + 1, PinType::Gold);
                        }
                    }
                }

                // Retrigger: on 2nd pin hit, record its value for triple scoring
                if (activeItems.ballType == BallType::Retrigger &&
                    activeItems.pinsHitThisShot == 2 && !activeItems.retriggered) {
                    activeItems.retriggered = true;
                    activeItems.retriggeredValue = pin.getValue();
                }
            }
        }

        // Limit pin redirection
        if (hitAnyPin && rollLocked) {
            float maxDeltaSide = 110.0f;
            float dx = bv.x - bvStart.x;
            bv.x = bvStart.x + std::clamp(dx, -maxDeltaSide, maxDeltaSide);
            bv.x = std::clamp(bv.x, -180.0f, 180.0f);
            if (bv.y > -260.0f) bv.y = -260.0f;
        }

        ball.setPos(bp);
        ball.setVel(bv);
    }

    // Pin -> pin
    {
        for (size_t i = 0; i < pins.size(); i++) {
            if (!pins[i].isActive()) continue;
            for (size_t j = i + 1; j < pins.size(); j++) {
                if (!pins[j].isActive()) continue;

                sf::Vector2f p1 = pins[i].getPos(), v1 = pins[i].getVel();
                sf::Vector2f p2 = pins[j].getPos(), v2 = pins[j].getVel();
                sf::Vector2f v1B = v1, v2B = v2;

                float r1 = pins[i].getRadius() * (pins[i].isFallen() ? 0.86f : 1.00f);
                float r2 = pins[j].getRadius() * (pins[j].isFallen() ? 0.86f : 1.00f);
                resolveCircleCollision(p1, v1,
                                       pins[i].getMass() * activeItems.pinMassMultiplier * bossPinMassMult, r1,
                                       p2, v2,
                                       pins[j].getMass() * activeItems.pinMassMultiplier * bossPinMassMult, r2,
                                       0.50f);
                
                // Play collision sound
                float pinImpact = length(v1 - v1B) + length(v2 - v2B);
                if (pinImpact > 30.0f) {
                    float collisionVolume = std::min(80.0f, 30.0f + pinImpact * 0.3f);
                    audio.playPinCollision(collisionVolume);
                }

                pins[i].setPos(p1); pins[i].setVel(v1);
                pins[j].setPos(p2); pins[j].setVel(v2);
            }
        }
    }

    // Pin -> lane walls
    {
        float leftW = lane.playLeft(), rightW = lane.playRight();
        for (auto& pin : pins) {
            if (!pin.isActive()) continue;
            sf::Vector2f p = pin.getPos(), v = pin.getVel();
            float r = pin.getRadius();

            // Force fall if hit gutter or back
            if (p.x < leftW + r || p.x > rightW - r || p.y < lane.top + r) {
                if (!pin.isFallen()) pin.setFallen(true);
            }

            // Bounce off walls
            if (p.x < leftW + r && v.x < 0.0f) { p.x = leftW + r; v.x = -v.x * 0.25f; }
            if (p.x > rightW - r && v.x > 0.0f) { p.x = rightW - r; v.x = -v.x * 0.25f; }
            if (p.y < lane.top + r && v.y < 0.0f) { p.y = lane.top + r; v.y = -v.y * 0.15f; }

            pin.setPos(p); pin.setVel(v);
        }
    }
}

void Game::update(float dt) {
    // If in menu/settings, don't update game logic
    if (ui.getState() == GameState::Menu ||
        ui.getState() == GameState::Settings ||
        ui.getState() == GameState::Tutorial) {
        audio.playMenuMusic();
        audio.setMusicVolume(ui.getMusicVolume());
        audio.setSoundVolume(ui.getSoundVolume());
        return;
    }

    audio.setMusicVolume(ui.getMusicVolume());
    audio.setSoundVolume(ui.getSoundVolume());
    updateStrikeConfetti(dt);

    if (!window.hasFocus()) {
        return;
    }

    if (ui.getState() == GameState::RoundSummary ||
        ui.getState() == GameState::Shop ||
        ui.getState() == GameState::GameWon) {
        return;
    }

    if (resetConfirmOpen) {
        return;
    }

    if (bossIntroOpen) {
        return;
    }
    
    // Handle game over state
    if (gameOver) {
        static bool prevR = false;
        bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);

        static bool prevM = false;
        bool nowM = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M);

        if (nowM && !prevM) {
            // Back to menu
            ui.setState(GameState::Menu);
            strikeConfetti.clear();
            gameOver = false;
            endlessMode = false;
            bossIntroOpen = false;
            bossIntroShownRound = 0;
            roundSummaryLeadsToGameOver = false;
            roundSummaryPassed = false;
            pendingGameOverFromScore = false;
            postScorePauseTimer = 0.0f;
            pendingReset = false;
            xtremeLastRoundScoreProgress = 0;
            xtremeLastRoundTargetProgress = 0;
            equipBall(BallType::Normal);
            activeItems.resetAll();
            ui.resetEquippedBall();
            cancelPinPowerSelection();
            lane.bumpersOn = ui.getBumpersDefault();
            audio.playMenuMusic();
        }
        
        if (nowR && !prevR) {
            confirmRunReset();
        }
        
        prevR = nowR;
        prevM = nowM;
        return;
    }

    if (postScorePauseTimer > 0.0f) {
        postScorePauseTimer -= dt;
        if (postScorePauseTimer <= 0.0f) {
            postScorePauseTimer = 0.0f;

            if (pendingGameOverFromScore) {
                gameOver = true;
                pendingGameOverFromScore = false;
                return;
            }

            if (ui.getState() == GameState::Xtreme && xtreme.isShopReady()) {
                ui.setState(GameState::Shop);
                cancelPinPowerSelection();
                xtreme.consumeShopReady();
                if (activeItems.powerSkip || activeItems.hasPurchasedPower(PowerType::Skip)) {
                    activeItems.powerSkip = true;
                    activeItems.skipCharges = 2;
                }
                return;
            }
        } else {
            return;
        }
    }
    
    // Move/aim before roll
    if (!rollLocked && !pendingReset) {
        sf::Vector2f p = ball.getPos();
        float r = ball.getRadius();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            p.x -= moveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            p.x += moveSpeed * dt;

        float playL = lane.playLeft();
        float playR = lane.playRight();

        if (p.x < playL + r) p.x = playL + r;
        if (p.x > playR - r) p.x = playR - r;

        ball.setPos(p);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            aimDeg -= aimTurnSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            aimDeg += aimTurnSpeed * dt;

        if (aimDeg < -140.0f) aimDeg = -140.0f;
        if (aimDeg > -40.0f) aimDeg = -40.0f;
    }

    // Launch
    if (!rollLocked && !pendingReset &&
        pinPowerSelectionMode == PinPowerSelectionMode::None &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        float launchAim = aimDeg;
        if (activeItems.shoeType == ShoeType::Clown) {
            // Clown shoes add a noticeable random wobble to launch direction.
            launchAim += static_cast<float>((rand() % 29) - 14);
        }
        float a = degToRad(launchAim);
        rollDir = sf::Vector2f(std::cos(a), std::sin(a));
        float launchSpeed = 1600.0f * activeItems.speedMultiplier * activeItems.launchSpeedMultiplier;
        ball.launch(rollDir, launchSpeed);
        rollLocked = true;

        // Start rolling sound
        audio.startBallRoll();
    }

    // Toggle keys
    static bool prevB = false, prevR = false, prevM = false, prevC = false, prevN = false, prevV = false;

    bool nowB = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B);
    bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
    bool nowM = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M);
    bool nowC = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);
    bool nowN = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::N);
    bool nowV = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::V);

    if (nowB && !prevB) {
        if (ui.getState() == GameState::Playing) {
            lane.bumpersOn = !lane.bumpersOn;
        }
    }

    if (nowR && !prevR) {
        requestRunReset();
    }

    // C key - change ball color
    if (nowC && !prevC && ui.getState() == GameState::Playing) {
        int colorChoice = rand() % 8;
        sf::Color ballColor;
        
        switch(colorChoice) {
            case 0: ballColor = sf::Color(25, 55, 140); break;      // Blue
            case 1: ballColor = sf::Color(200, 30, 30); break;      // Red
            case 2: ballColor = sf::Color(20, 20, 20); break;       // Black
            case 3: ballColor = sf::Color(128, 0, 128); break;      // Purple
            case 4: ballColor = sf::Color(255, 140, 0); break;      // Orange
            case 5: ballColor = sf::Color(0, 120, 0); break;        // Green
            case 6: ballColor = sf::Color(255, 20, 147); break;     // Pink
            case 7: ballColor = sf::Color(180, 180, 0); break;      // Yellow
            default: ballColor = sf::Color(25, 55, 140); break;
        }
        
        ball.setColor(ballColor);
    }

    bool canChoosePinPowers = (ui.getState() == GameState::Xtreme &&
                               !rollLocked &&
                               !pendingReset &&
                               !gameOver);
    if (!canChoosePinPowers && pinPowerSelectionMode != PinPowerSelectionMode::None) {
        cancelPinPowerSelection();
    }
    if (canChoosePinPowers) {
        bool duplicateModeActive =
            (pinPowerSelectionMode == PinPowerSelectionMode::DuplicatePickSource ||
             pinPowerSelectionMode == PinPowerSelectionMode::DuplicatePickTarget);
        bool swapModeActive =
            (pinPowerSelectionMode == PinPowerSelectionMode::SwapPickFirst ||
             pinPowerSelectionMode == PinPowerSelectionMode::SwapPickSecond);

        // Duplicate is forced-use now: if you have a charge, selection mode is
        // automatically enabled and must be resolved before launch.
        if (activeItems.duplicateCharges > 0 && !duplicateModeActive) {
            pinPowerSelectionMode = PinPowerSelectionMode::DuplicatePickSource;
            pinPowerFirstIndex = -1;
            duplicateModeActive = true;
            swapModeActive = false;
        }

        if (nowN && !prevN) {
            if (duplicateModeActive) {
                // Duplicate cannot be cancelled while charges remain.
                if (activeItems.duplicateCharges <= 0) {
                    cancelPinPowerSelection();
                }
            } else if (activeItems.duplicateCharges > 0) {
                pinPowerSelectionMode = PinPowerSelectionMode::DuplicatePickSource;
                pinPowerFirstIndex = -1;
            }
        }
        if (nowV && !prevV) {
            if (activeItems.duplicateCharges > 0) {
                // Must spend Duplicate before Swap can be selected.
                // Ignore swap toggle while duplicate is pending.
            }
            else if (swapModeActive) {
                cancelPinPowerSelection();
            } else if (activeItems.swapCharges > 0) {
                pinPowerSelectionMode = PinPowerSelectionMode::SwapPickFirst;
                pinPowerFirstIndex = -1;
            }
        }
    }

    prevB = nowB; 
    prevR = nowR; 
    prevM = nowM; 
    prevC = nowC;
    prevN = nowN;
    prevV = nowV;

    // Update physics
    ball.update(dt);
    for (auto& pin : pins) pin.update(dt);

    // Keep ball moving forward
    if (rollLocked) {
        sf::Vector2f v = ball.getVel();
        float s = length(v);

        if (s > 0.0f && s < minRollSpeed) {
            sf::Vector2f dir = v / s;
            ball.setVel(dir * minRollSpeed);
        }
    }

    applyGuttersAndBumpers();
    doCollisions();
    applyBlackHoleGravity(dt);
    processExplosions();
    updatePinSlotValueSnapshot(pins);

    // Anti-softlock: if a roll is jammed near the backline (usually pin wedged),
    // or rolling for too long, end the shot so the game can progress.
    if (rollLocked && !pendingReset) {
        shotRollTimer += dt;

        sf::Vector2f p = ball.getPos();
        sf::Vector2f v = ball.getVel();
        bool nearBackline = p.y < (lane.top + ball.getRadius() + 85.0f);
        bool mostlySideways = std::abs(v.y) < 95.0f;

        if (nearBackline && mostlySideways) {
            backlineJamTimer += dt;
        } else {
            backlineJamTimer = 0.0f;
        }

        if (backlineJamTimer >= backlineJamTime || shotRollTimer >= maxShotRollTime) {
            ball.stop();
            rollLocked = false;
            audio.stopBallRoll();
            startPendingReset();
        }
    }

    // Start reset if ball hits back
    if (!pendingReset && ball.getPos().y < lane.top + ball.getRadius()) {
        ball.stop();
        rollLocked = false;
        audio.stopBallRoll();
        startPendingReset();
    }

    finishPendingResetIfReady(dt);

    if (postScorePauseTimer <= 0.0f &&
        !pendingReset &&
        !pendingGameOverFromScore &&
        ui.getState() == GameState::Xtreme &&
        xtreme.isShopReady()) {
        ui.setState(GameState::Shop);
        cancelPinPowerSelection();
        xtreme.consumeShopReady();
        if (activeItems.powerSkip || activeItems.hasPurchasedPower(PowerType::Skip)) {
            activeItems.powerSkip = true;
            activeItems.skipCharges = 2;
        }
        return;
    }
}

void Game::draw() {
    window.clear(sf::Color(84, 128, 208));

    // Draw menu if in menu state
    if (ui.getState() == GameState::Menu) {
        float dt = clock.getElapsedTime().asSeconds();
        ui.drawMenu(window, windowW, windowH, dt);
        window.display();
        return;
    }

    if (ui.getState() == GameState::Settings) {
        float dt = clock.getElapsedTime().asSeconds();
        ui.drawMenu(window, windowW, windowH, dt);
        ui.drawSettings(window, windowW, windowH);
        window.display();
        return;
    }

    if (ui.getState() == GameState::Tutorial) {
        float dt = clock.getElapsedTime().asSeconds();
        ui.drawMenu(window, windowW, windowH, dt);
        ui.drawTutorial(window, windowW, windowH);
        window.display();
        return;
    }

    if (ui.getState() == GameState::Shop) {
        ui.drawShop(window, xtreme.getTokens(), windowW, windowH, activeItems);
        window.display();
        return;
    }
    // Draw game
    lane.draw(window);
    for (const auto& pin : pins) pin.draw(window);
    if (pinPowerFirstIndex >= 0 &&
        pinPowerFirstIndex < (int)pins.size() &&
        pinPowerSelectionMode != PinPowerSelectionMode::None) {
        const Pin& selectedPin = pins[pinPowerFirstIndex];
        if (selectedPin.isActive()) {
            float highlightRadius = std::max(22.0f, selectedPin.getRadius() * 2.2f);
            sf::CircleShape ring(highlightRadius);
            ring.setOrigin({highlightRadius, highlightRadius});
            ring.setPosition(selectedPin.getPos());
            ring.setFillColor(sf::Color(0, 0, 0, 0));
            bool duplicateMode =
                (pinPowerSelectionMode == PinPowerSelectionMode::DuplicatePickSource ||
                 pinPowerSelectionMode == PinPowerSelectionMode::DuplicatePickTarget);
            ring.setOutlineColor(duplicateMode ? sf::Color(155, 210, 255) : sf::Color(165, 255, 180));
            ring.setOutlineThickness(3.0f);
            window.draw(ring);
        }
    }

    // Draw aim line
    if (!rollLocked && !pendingReset &&
        ui.getState() != GameState::RoundSummary &&
        ui.getState() != GameState::GameWon) {
        float a = degToRad(aimDeg);
        sf::Vector2f dir(std::cos(a), std::sin(a));
        sf::Vertex line[2] = { 
            {ball.getPos(), sf::Color::Yellow}, 
            {ball.getPos() + dir * 114.0f, sf::Color::Yellow} 
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    ball.draw(window);
    drawStrikeConfetti();

    // Draw UI and check for actions (like exiting to menu)
    GameAction action = GameAction::None;
    std::string pinPowerHintLine1;
    std::string pinPowerHintLine2;
    bool useLiveFormulaPreview = false;
    int liveImpactPreview = xtreme.getLastImpact();
    int liveComboPreview = xtreme.getLastCombo();
    int displayedRoundScore = xtreme.getRoundScore();
    if (ui.getState() == GameState::Xtreme) {
        if (rollLocked || pendingReset) {
            useLiveFormulaPreview = true;

            std::vector<int> liveHitIndices;
            liveHitIndices.reserve(pins.size());
            for (int i = 0; i < (int)pins.size(); i++) {
                if (!pins[i].isActive()) continue;
                if (pins[i].isFallen()) liveHitIndices.push_back(i);
            }
            int livePinsHit = static_cast<int>(liveHitIndices.size());
            int livePinValueSum = computePinValueSumWithItems(liveHitIndices);
            for (int idx : liveHitIndices) {
                if (pins[idx].getPinType() == PinType::LuckyDucky && pins[idx].isLuckyZero()) {
                    livePinValueSum -= computePinValueWithItems(idx);
                }
            }
            if (livePinValueSum < 0) livePinValueSum = 0;

            int greedyComboBonus = 0;
            if (isPowerEnabledThisShot(PowerType::Greedy)) {
                greedyComboBonus = std::max(0, xtreme.getTokens() / 4);
            }
            float homeBaseBonus = isPowerEnabledThisShot(PowerType::HomeBase)
                ? std::min(activeItems.homeBaseComboBonus, kHomeBaseComboBonusCap)
                : 0.0f;
            float liveBaseCombo =
                ((activeBoss == BossType::LowBaseScore && activeBossRound == xtreme.getRound()) ? 0.5f : 1.0f) +
                homeBaseBonus;
            float liveComboValue = static_cast<float>(livePinsHit + greedyComboBonus) + liveBaseCombo;

            int liveBaseImpact =
                (activeBoss == BossType::LowBaseScore && activeBossRound == xtreme.getRound()) ? 5 : 10;
            liveImpactPreview = liveBaseImpact + livePinValueSum;
            liveComboPreview = std::max(1, static_cast<int>(std::lround(liveComboValue)));
        }
        if (pendingReset && pendingShotScored &&
            pendingScoreVisualTimer < pendingScoreVisualDuration) {
            displayedRoundScore = pendingRoundScoreBeforeShot;
        }

        if (activeItems.duplicateCharges > 0) {
            pinPowerHintLine1 = "Duplicate(" + std::to_string(activeItems.duplicateCharges) +
                                ") must be used before roll";
        } else if (activeItems.swapCharges > 0) {
            pinPowerHintLine1 = "V Swap(" + std::to_string(activeItems.swapCharges) + ")";
        }
        switch (pinPowerSelectionMode) {
            case PinPowerSelectionMode::DuplicatePickSource:
                pinPowerHintLine2 = "Duplicate: click source pin";
                break;
            case PinPowerSelectionMode::DuplicatePickTarget:
                pinPowerHintLine2 = "Duplicate: click target pin";
                break;
            case PinPowerSelectionMode::SwapPickFirst:
                pinPowerHintLine2 = "Swap: click first pin";
                break;
            case PinPowerSelectionMode::SwapPickSecond:
                pinPowerHintLine2 = "Swap: click second pin";
                break;
            case PinPowerSelectionMode::None:
            default:
                break;
        }
    }
    if (ui.getState() == GameState::Xtreme) {
        int displayedRound = xtreme.getRound();
        int displayedFrame = xtreme.getFrameInRound();
        int displayedShot = xtreme.getShotInFrame();
        bool bossRoundNow =
            (activeBoss != BossType::None && activeBossRound == xtreme.getRound());

        if (pendingReset && pendingShotScored &&
            pendingScoreVisualTimer < pendingScoreVisualDuration &&
            pendingDisplayRoundBeforeShot > 0) {
            displayedRound = pendingDisplayRoundBeforeShot;
            displayedFrame = pendingDisplayFrameBeforeShot;
            displayedShot = pendingDisplayShotBeforeShot;
        }

        action = ui.drawXtremeHUD(
            window,
            displayedRound,
            displayedFrame,
            displayedShot,
            xtreme.getTotalShots(),
            xtreme.getTargetScore(),
            displayedRoundScore,
            xtreme.getTokens(),
            xtreme.getLastImpact(),
            xtreme.getLastCombo(),
            xtreme.getLastShotScore(),
            windowW,
            windowH,
            activeItems,
            pinPowerHintLine1,
            pinPowerHintLine2,
            useLiveFormulaPreview,
            liveImpactPreview,
            liveComboPreview,
            bossRoundNow,
            bossRoundNow ? getActiveBossName() : "",
            bossRoundNow ? getActiveBossDescription() : ""
        );
    } else if (ui.getState() == GameState::Playing) {
        action = ui.drawScorecard(window, scorer.getFrames(), scorer.getCurrentFrame(),
                         scorer.getCurrentBall(), normalHighScore, windowW, windowH);
    }

    if (ui.getState() == GameState::RoundSummary) {
        ui.drawRoundSummaryPopup(
            window,
            roundSummaryRoundNumber,
            roundSummaryScore,
            roundSummaryTarget,
            roundSummaryTokensEarned,
            roundSummaryTokensTotal,
            roundSummaryPassed,
            roundSummaryLeadsToGameOver,
            windowW,
            windowH
        );
    }
    if (ui.getState() == GameState::GameWon) {
        ui.drawGameWonPopup(
            window,
            std::max(0, xtreme.getRound() - 1),
            xtreme.getTokens(),
            windowW,
            windowH
        );
    }
    
    if (gameOver) {
        if (xtremeMode) {
            ui.drawGameOverScreen(window,
                GameOverMode::Xtreme,
                finalXtremeRoundsCleared,
                xtremeBestRound,
                windowW,
                windowH,
                xtremeLastRoundScoreProgress,
                xtremeLastRoundTargetProgress);
        } else {
            ui.drawGameOverScreen(window,
                GameOverMode::NormalBowling,
                finalNormalScore,
                normalHighScore,
                windowW,
                windowH);
        }
    }

    if (bossIntroOpen) {
        BossIntroLayout modal = makeBossIntroLayout(windowW, windowH);

        sf::RectangleShape dim(sf::Vector2f(windowW, windowH));
        dim.setFillColor(sf::Color(8, 12, 26, 170));
        window.draw(dim);

        sf::RectangleShape panel(modal.panel.size);
        panel.setPosition(modal.panel.position);
        panel.setFillColor(sf::Color(236, 241, 251, 250));
        panel.setOutlineColor(sf::Color(138, 36, 58));
        panel.setOutlineThickness(3.2f);
        window.draw(panel);

        sf::RectangleShape topBand(sf::Vector2f(modal.panel.size.x, 54.0f));
        topBand.setPosition({modal.panel.position.x, modal.panel.position.y});
        topBand.setFillColor(sf::Color(246, 96, 118, 250));
        topBand.setOutlineColor(sf::Color(121, 26, 46));
        topBand.setOutlineThickness(2.0f);
        window.draw(topBand);

        std::string bossName = getActiveBossName();
        std::string bossDesc = getActiveBossDescription();

        if (ui.isFontLoaded()) {
            sf::Text title(ui.getFont(), "BOSS ROUND " + std::to_string(xtreme.getRound()), 34);
            sf::FloatRect tb = title.getLocalBounds();
            title.setPosition({
                modal.panel.position.x + modal.panel.size.x * 0.5f - (tb.position.x + tb.size.x * 0.5f),
                modal.panel.position.y + 10.0f
            });
            title.setFillColor(sf::Color(255, 247, 236));
            title.setOutlineColor(sf::Color(111, 20, 39));
            title.setOutlineThickness(1.2f);
            window.draw(title);

            sf::Text name(ui.getFont(), bossName, 44);
            sf::FloatRect nb = name.getLocalBounds();
            name.setPosition({
                modal.panel.position.x + modal.panel.size.x * 0.5f - (nb.position.x + nb.size.x * 0.5f),
                modal.panel.position.y + 78.0f
            });
            name.setFillColor(sf::Color(24, 41, 80));
            name.setOutlineColor(sf::Color(246, 250, 255));
            name.setOutlineThickness(1.4f);
            window.draw(name);

            sf::Text desc(ui.getFont(), bossDesc, 28);
            sf::FloatRect db = desc.getLocalBounds();
            float descX = modal.panel.position.x + modal.panel.size.x * 0.5f - (db.position.x + db.size.x * 0.5f);
            float descMaxX = modal.panel.position.x + modal.panel.size.x - 30.0f - db.size.x;
            if (descX > descMaxX) descX = descMaxX;
            if (descX < modal.panel.position.x + 30.0f) descX = modal.panel.position.x + 30.0f;
            desc.setPosition({descX, modal.panel.position.y + 145.0f});
            desc.setFillColor(sf::Color(38, 62, 112));
            window.draw(desc);

            sf::Text reward(ui.getFont(), "Boss clear reward: +3 dollars", 24);
            sf::FloatRect rb = reward.getLocalBounds();
            reward.setPosition({
                modal.panel.position.x + modal.panel.size.x * 0.5f - (rb.position.x + rb.size.x * 0.5f),
                modal.panel.position.y + 205.0f
            });
            reward.setFillColor(sf::Color(204, 128, 24));
            reward.setOutlineColor(sf::Color(248, 252, 255));
            reward.setOutlineThickness(1.0f);
            window.draw(reward);

            sf::RectangleShape btn(modal.continueButton.size);
            btn.setPosition(modal.continueButton.position);
            btn.setFillColor(sf::Color(89, 213, 242));
            btn.setOutlineColor(sf::Color(54, 109, 194));
            btn.setOutlineThickness(2.8f);
            window.draw(btn);

            sf::Text btnText(ui.getFont(), "START FIGHT", 31);
            sf::FloatRect bb2 = btnText.getLocalBounds();
            btnText.setPosition({
                modal.continueButton.position.x + modal.continueButton.size.x * 0.5f -
                    (bb2.position.x + bb2.size.x * 0.5f),
                modal.continueButton.position.y + modal.continueButton.size.y * 0.5f -
                    (bb2.position.y + bb2.size.y * 0.58f)
            });
            btnText.setFillColor(sf::Color(16, 41, 88));
            window.draw(btnText);
        }

        action = GameAction::None;
    }

    if (resetConfirmOpen) {
        ResetConfirmLayout modal = makeResetConfirmLayout(windowW, windowH);

        sf::RectangleShape dim(sf::Vector2f(windowW, windowH));
        dim.setFillColor(sf::Color(20, 36, 76, 150));
        window.draw(dim);

        sf::RectangleShape panel(modal.panel.size);
        panel.setPosition(modal.panel.position);
        panel.setFillColor(sf::Color(231, 239, 252, 248));
        panel.setOutlineColor(sf::Color(106, 139, 206));
        panel.setOutlineThickness(3.0f);
        window.draw(panel);

        sf::RectangleShape panelAccent(sf::Vector2f(modal.panel.size.x - 8.0f, 7.0f));
        panelAccent.setPosition({modal.panel.position.x + 4.0f, modal.panel.position.y + 4.0f});
        panelAccent.setFillColor(sf::Color(70, 198, 232, 220));
        window.draw(panelAccent);

        auto drawConfirmButton = [&](const sf::FloatRect& rect,
                                     const std::string& label,
                                     sf::Color fill,
                                     sf::Color border) {
            sf::RectangleShape b(rect.size);
            b.setPosition(rect.position);
            b.setFillColor(fill);
            b.setOutlineColor(border);
            b.setOutlineThickness(2.0f);
            window.draw(b);

            if (ui.isFontLoaded()) {
                sf::Text t(ui.getFont(), label, 28);
                sf::FloatRect tb = t.getLocalBounds();
                t.setPosition({
                    rect.position.x + rect.size.x * 0.5f - (tb.position.x + tb.size.x * 0.5f),
                    rect.position.y + rect.size.y * 0.5f - (tb.position.y + tb.size.y * 0.58f)
                });
                t.setFillColor(sf::Color(26, 45, 84));
                t.setOutlineColor(sf::Color(245, 250, 255));
                t.setOutlineThickness(1.0f);
                window.draw(t);
            }
        };

        drawConfirmButton(modal.yesButton, "YES", sf::Color(94, 232, 152), sf::Color(198, 255, 223));
        drawConfirmButton(modal.noButton, "NO", sf::Color(242, 104, 124), sf::Color(255, 200, 214));

        if (ui.isFontLoaded()) {
            const float textPadX = 24.0f;
            const float maxTextWidth = std::max(80.0f, modal.panel.size.x - textPadX * 2.0f);

            sf::Text ask(ui.getFont(), "Reset run?", 44);
            while (ask.getCharacterSize() > 28 &&
                   ask.getLocalBounds().size.x > maxTextWidth) {
                ask.setCharacterSize(ask.getCharacterSize() - 1);
            }
            ask.setFillColor(sf::Color(24, 45, 90));
            ask.setOutlineColor(sf::Color(245, 250, 255));
            ask.setOutlineThickness(1.2f);
            ask.setPosition({modal.panel.position.x + 24.0f, modal.panel.position.y + 18.0f});
            window.draw(ask);

            sf::Text sub(ui.getFont(), "Are you sure?\nThis will restart the current run.", 22);
            while (sub.getCharacterSize() > 16 &&
                   sub.getLocalBounds().size.x > maxTextWidth) {
                sub.setCharacterSize(sub.getCharacterSize() - 1);
            }
            sub.setFillColor(sf::Color(53, 78, 130));
            sub.setPosition({modal.panel.position.x + 24.0f, modal.panel.position.y + 84.0f});
            window.draw(sub);
        }

        action = GameAction::None;
    }

    // Now handle the action returned from the scorecard UI
    if (action == GameAction::ExitToMenu) {
        ui.setState(GameState::Menu);
        strikeConfetti.clear();
        scorer.resetGame();
        xtreme.reset();
        bossIntroOpen = false;
        bossIntroShownRound = 0;
        xtremeMode = false;
        endlessMode = false;
        roundSummaryLeadsToGameOver = false;
        roundSummaryPassed = false;
        pendingGameOverFromScore = false;
        postScorePauseTimer = 0.0f;
        xtremeLastRoundScoreProgress = 0;
        xtremeLastRoundTargetProgress = 0;
        xtremeRoundStartTokens = xtreme.getTokens();
        equipBall(BallType::Normal);
        activeItems.resetAll();
        ui.resetEquippedBall();
        cancelPinPowerSelection();
        resetBall();
        pins = createPins(lane.centerX(), 240.0f);
        audio.stopBackgroundMusic();
        audio.stopBallRoll();
        audio.playMenuMusic();
    }

    window.display();
}

void Game::applyLetterbox(unsigned winW, unsigned winH) {
    if (winW == 0 || winH == 0) return;
    // Keep a fixed world height so gameplay framing (lane + ball) stays valid,
    // while expanding world width to fill widescreen windows without stretching.
    constexpr float designH = 1024.0f;
    float aspect = static_cast<float>(winW) / static_cast<float>(winH);
    windowH = designH;
    windowW = designH * aspect;

    view.setSize(sf::Vector2f(windowW, windowH));
    view.setCenter(sf::Vector2f(windowW * 0.5f, windowH * 0.5f));
    view.setViewport(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(1.f, 1.f)));
    window.setView(view);
}
