#include "Game.h"
#include "Physics.h"
#include "Items.h"
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <fstream>

namespace {
constexpr float kHomeBaseComboBonusCap = 8.0f;
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
        // Spawn as a wider front row so they stay in-lane and upright.
        float y = startY + spacing * 0.85f;
        out.emplace_back(sf::Vector2f(centerX - spacing, y), radius, pinValue++);
        out.emplace_back(sf::Vector2f(centerX + spacing, y), radius, pinValue++);
    }

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
    if (activeItems.pinSlotAssignments.empty()) return;

    int maxSlot = std::min((int)pinSet.size(), activeItems.getActivePinSlotCount());
    for (const auto& assigned : activeItems.pinSlotAssignments) {
        if (assigned.slot < 1 || assigned.slot > maxSlot) continue;
        int idx = assigned.slot - 1;
        if (!pinSet[idx].isActive()) continue;
        pinSet[idx].setPinType(assigned.type);
    }
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
    // Duplicate and Swap are now manual (player-chosen), not random-on-rerack.
    applyPendingRandomPinUpgrades(pinSet);
    updatePinSlotValueSnapshot(pinSet);
}

void Game::updatePinSlotValueSnapshot(const std::vector<Pin>& pinSet) {
    activeItems.pinSlotCurrentValues.fill(0);
    int maxSlot = std::min((int)pinSet.size(), activeItems.getActivePinSlotCount());
    for (int i = 0; i < maxSlot; i++) {
        activeItems.pinSlotCurrentValues[i] = pinSet[i].getValue();
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
        float hitR = std::max(20.0f, pin.getRadius() * 2.8f);
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

    std::swap(pins[firstIndex], pins[secondIndex]);
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
        
        // Handle mouse clicks for menu
        if (ev->is<sf::Event::MouseButtonPressed>()) {
            auto mouseEv = ev->getIf<sf::Event::MouseButtonPressed>();
            if (mouseEv->button == sf::Mouse::Button::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                
                // Convert to world coordinates
                sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);

                if (ui.getState() == GameState::Xtreme && !gameOver) {
                    if (!rollLocked && !pendingReset && handlePinPowerSelectionClick(worldPos)) {
                        continue;
                    }
                }
                
                if (ui.getState() == GameState::Menu) {
                    MenuButton clicked = ui.handleMenuClick(window, sf::Vector2i(worldPos.x, worldPos.y));
                    
                    if (clicked == MenuButton::Normal) {
                        xtremeMode = false;
                        ui.setState(GameState::Playing);
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
                        xtreme.reset();
                        xtremeMode = true;
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

                    } else if (clicked == MenuButton::Settings) {
                        ui.setState(GameState::Settings);
                    }
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
                        return p == PowerType::Duplicate || p == PowerType::SwapPins;
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
                        std::vector<int> remaining(powerCount, 0);
                        for (int i = 0; i < powerCount; i++) {
                            PowerType p = static_cast<PowerType>(i);
                            if (!activeItems.hasPower(p)) continue;
                            // Extra Slot is a one-time unlock and not sellable.
                            if (p == PowerType::ExtraPowerSlot) continue;
                            if (p == PowerType::Duplicate) {
                                remaining[i] = std::max(0, activeItems.duplicateCharges);
                            } else if (p == PowerType::SwapPins) {
                                remaining[i] = std::max(0, activeItems.swapCharges);
                            } else {
                                remaining[i] = 1;
                            }
                        }
                        std::vector<PowerType> out;
                        out.reserve(activeItems.purchasedPowers.size());
                        for (int raw : activeItems.purchasedPowers) {
                            if (raw < 0 || raw >= powerCount) continue;
                            if (remaining[raw] <= 0) continue;
                            out.push_back(static_cast<PowerType>(raw));
                            remaining[raw]--;
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
                               purchased == UI::ShopActionSellBallSlot2) {
                        int slot = (purchased == UI::ShopActionSellBallSlot2) ? 2 : 1;
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
                            int slot = ui.getSelectedBallSlot();
                            if (activeItems.getBallForSlot(slot) == offer.ballType) {
                                canBuy = false;
                            }
                        }
                        if (offer.category == ShopItemCategory::Shoe) {
                            if (activeItems.shoeType == offer.shoeType) {
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
                            int slot = ui.getSelectedBallSlot();
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
                            ui.recordOfferPicked(offer);
                        } else if (offer.category == ShopItemCategory::Pin) {
                            xtreme.addTokens(-offer.cost);
                            bool hasExtraPins = activeItems.powerExtraPins ||
                                                activeItems.hasPurchasedPower(PowerType::ExtraPins);
                            int pinLimit = hasExtraPins ? 12 : 10;
                            int targetSlot = std::clamp(ui.getSelectedPinSlot(), 1, pinLimit);

                            PinType previous = PinType::Normal;
                            bool replaced = activeItems.removePinAssignmentAtSlot(targetSlot, &previous);
                            if (replaced && previous != PinType::Normal) {
                                int sellValue = pinTypeCost(previous) / 2;
                                if (sellValue > 0) xtreme.addTokens(sellValue);
                            }
                            activeItems.setPinAssignment(targetSlot, offer.pinType);
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
                    if (worldPos.x > windowW/2.f - 110.f && worldPos.x < windowW/2.f + 110.f &&
                        worldPos.y > windowH - 100.f    && worldPos.y < windowH - 40.f) {
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
    }
    if (activeItems.powerBumpers) {
        lane.bumpersOn = true;
    }

    // Pick the active ball based on mode + shot number.
    // Xtreme has two shots per frame, so slot 1 -> shot 1 and slot 2 -> shot 2.
    if (ui.getState() == GameState::Xtreme) {
        int shot = xtreme.getShotInFrame();
        equipBall(activeItems.getBallForShot(shot));
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
    for (int idx : hitIndices) {
        total += computePinValueWithItems(idx);
    }
    // Retrigger means the 2nd pin hit scores 3x total:
    // 1x from the normal sum above, plus 2x extra here.
    if (activeItems.ballType == BallType::Retrigger && activeItems.retriggered)
        total += activeItems.retriggeredValue * 2;
    return total;
}

void Game::processExplosions() {
    const float blastRadius = 108.f;
    const float blastForce  = 900.f * 0.74f;

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
        int roundScoreBeforeShot = (ui.getState() == GameState::Xtreme) ? xtreme.getRoundScore() : 0;
        int targetBeforeShot = (ui.getState() == GameState::Xtreme) ? xtreme.getTargetScore() : 0;
        bool earthquakeStrikeThisShot = false;

        if (ui.getState() == GameState::Xtreme && activeItems.powerEarthquake) {
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

        int pinValueSumThisBall = computePinValueSumWithItems(hitPinIndices);
        for (int idx : hitPinIndices) {
            if (pins[idx].getPinType() == PinType::LuckyDucky && pins[idx].isLuckyZero()) {
                pinValueSumThisBall -= computePinValueWithItems(idx);
            }
        }
        if (pinValueSumThisBall < 0) pinValueSumThisBall = 0;

        int greedyComboBonus = 0;
        if (ui.getState() == GameState::Xtreme && activeItems.powerGreedy) {
            greedyComboBonus = std::max(0, xtreme.getTokens() / 4);
        }

        if (activeItems.ballType == BallType::Midas) {
            for (int idx : activeItems.goldPinIndices) {
                if (pins[idx].isFallen() && pins[idx].getPinType() != PinType::Gold) {
                    xtreme.addTokens(1);
                }
            }
            activeItems.goldPinIndices.clear();
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
        if (ui.getState() == GameState::Xtreme) {
            // shotBeforeRecord is intentionally captured before recordShot(),
            // because recordShot advances the internal shot counter.
            strikeThisShot = (shotBeforeRecord == 1 &&
                              physicalPinsDownThisShot >= activeItems.pinsStandingAtShotStart);
            bool spareThisShot = (shotBeforeRecord > 1 &&
                                  physicalPinsDownThisShot >= activeItems.pinsStandingAtShotStart);
            if (activeItems.powerConfusion && spareThisShot) {
                strikeThisShot = true;
            }
            if (earthquakeStrikeThisShot) {
                strikeThisShot = true;
            }
            xtreme.recordShot(
                knockedThisBall,
                pinValueSumThisBall,
                strikeThisShot,
                static_cast<float>(thirdTimeComboMultiplier),
                greedyComboBonus);

            if (activeItems.powerHomeBase && physicalPinsDownThisShot > 0) {
                activeItems.homeBasePinsTowardNextCombo += physicalPinsDownThisShot;
                while (activeItems.homeBasePinsTowardNextCombo >= 20) {
                    activeItems.homeBasePinsTowardNextCombo -= 20;
                    if (activeItems.homeBaseComboBonus < kHomeBaseComboBonusCap) {
                        activeItems.homeBaseComboBonus =
                            std::min(kHomeBaseComboBonusCap, activeItems.homeBaseComboBonus + 1.0f);
                    }
                }
                float clampedBonus = std::min(activeItems.homeBaseComboBonus, kHomeBaseComboBonusCap);
                xtreme.setBaseCombo(1.0f + clampedBonus);
            }

            // Random upgrade applies once after each completed frame.
            if (xtreme.getShotInFrame() == 1 && activeItems.powerRandomUpgrade) {
                activeItems.pendingRandomPinUpgrades += 1;
            }
        } else {
            scorer.recordBall(knockedThisBall);
        }

        pendingPhysicalPinsDownThisShot = physicalPinsDownThisShot;
        pendingStrikeThisShot = strikeThisShot;
        pendingRoundScoreBeforeShot = roundScoreBeforeShot;
        pendingTargetBeforeShot = targetBeforeShot;
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
            ui.setState(GameState::RoundSummary);
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
        bool bumpersActive = lane.bumpersOn || activeItems.powerBumpers;
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
    // Ball -> pin
    {
        sf::Vector2f bp = ball.getPos();
        sf::Vector2f bv = ball.getVel();
        sf::Vector2f bvStart = bv;
        float br = ball.getRadius();
        float bm = ball.getMass() * 1.07f;  // slight bias so pins affect ball a bit less
        const float pinFallImpactThreshold = 48.0f;

        bool hitAnyPin = false;

        for (int pi = 0; pi < (int)pins.size(); pi++) {
            auto& pin = pins[pi];
            if (!pin.isActive()) continue;

            sf::Vector2f pp = pin.getPos();
            sf::Vector2f pv = pin.getVel();
            sf::Vector2f pvBefore = pv;

            resolveCircleCollision(
                bp, bv, bm, br,
                pp, pv,
                (pin.isFallen() ? pin.getMass() : pin.getMass() * 0.6f) * activeItems.pinMassMultiplier,
                pin.getRadius(),
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

            // Pin first contact this shot (impact threshold)
            bool freshImpact = (!pin.isFallen() && impact > pinFallImpactThreshold);

            // CopyCat: becomes the type of the first hit pin.
            // Do this before setFallen so copying Exploding arms correctly.
            if (freshImpact &&
                pin.getPinType() == PinType::CopyCat &&
                activeItems.firstBallHitPinIndex != pi &&
                activeItems.firstBallHitPinIndex >= 0) {
                PinType copyFrom = pins[activeItems.firstBallHitPinIndex].getPinType();
                if (!activeItems.lockPinChangesMidRound && copyFrom != PinType::CopyCat) {
                    pin.setPinType(copyFrom);
                }
            }

            // Pin falls
            if (freshImpact) {
                pin.setFallen(true);
                float spin = (bv.x - pv.x) * 0.01f;
                pin.setAngularVel(std::clamp(spin, -6.0f, 6.0f));
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
                }

                // Midas ball: mark pin as gold (store index)
                if (activeItems.ballType == BallType::Midas) {
                    // Gold pins already pay via their own effect; don't double-count.
                    if (pin.getPinType() != PinType::Gold) {
                        bool alreadyGold = false;
                        for (int idx : activeItems.goldPinIndices)
                            if (idx == pi) { alreadyGold = true; break; }
                        if (!alreadyGold)
                            activeItems.goldPinIndices.push_back(pi);
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

                resolveCircleCollision(p1, v1,
                                       pins[i].getMass() * activeItems.pinMassMultiplier, pins[i].getRadius(),
                                       p2, v2,
                                       pins[j].getMass() * activeItems.pinMassMultiplier, pins[j].getRadius(),
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
    if (ui.getState() == GameState::Menu || ui.getState() == GameState::Settings) {
        audio.playMenuMusic();
        audio.setMusicVolume(ui.getMusicVolume());
        audio.setSoundVolume(ui.getSoundVolume());
        return;
    }

    audio.setMusicVolume(ui.getMusicVolume());
    audio.setSoundVolume(ui.getSoundVolume());

    if (ui.getState() == GameState::RoundSummary) {
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
            gameOver = false;
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
            if (ui.getState() == GameState::Xtreme) {
                xtreme.reset();
                xtremeRoundStartTokens = xtreme.getTokens();
            } else {
                scorer.resetGame();
            }
            gameOver = false;
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
            lane.bumpersOn = (ui.getState() == GameState::Playing) ? ui.getBumpersDefault() : false;
            pins = createPins(lane.centerX(), 240.0f);
            resetBall();
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
        if (ui.getState() == GameState::Xtreme) {
            xtreme.reset();
            xtremeRoundStartTokens = xtreme.getTokens();
        } else {
            scorer.resetGame();
        }
        gameOver = false;
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
        lane.bumpersOn = (ui.getState() == GameState::Playing) ? ui.getBumpersDefault() : false;
        if (ui.getState() == GameState::Xtreme) {
            ui.generateShopOffers(activeItems);
        }
        pins = createPins(lane.centerX(), 240.0f);
        resetBall();
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
        if (nowN && !prevN) {
            bool duplicateModeActive =
                (pinPowerSelectionMode == PinPowerSelectionMode::DuplicatePickSource ||
                 pinPowerSelectionMode == PinPowerSelectionMode::DuplicatePickTarget);
            if (duplicateModeActive) {
                cancelPinPowerSelection();
            } else if (activeItems.duplicateCharges > 0) {
                pinPowerSelectionMode = PinPowerSelectionMode::DuplicatePickSource;
                pinPowerFirstIndex = -1;
            }
        }
        if (nowV && !prevV) {
            bool swapModeActive =
                (pinPowerSelectionMode == PinPowerSelectionMode::SwapPickFirst ||
                 pinPowerSelectionMode == PinPowerSelectionMode::SwapPickSecond);
            if (swapModeActive) {
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
    window.clear(sf::Color(20, 20, 20));

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
    if (!rollLocked && !pendingReset && ui.getState() != GameState::RoundSummary) {
        float a = degToRad(aimDeg);
        sf::Vector2f dir(std::cos(a), std::sin(a));
        sf::Vertex line[2] = { 
            {ball.getPos(), sf::Color::Yellow}, 
            {ball.getPos() + dir * 100.0f, sf::Color::Yellow} 
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    ball.draw(window);

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
            if (activeItems.powerGreedy) greedyComboBonus = std::max(0, xtreme.getTokens() / 4);
            float liveBaseCombo = 1.0f + std::min(activeItems.homeBaseComboBonus, kHomeBaseComboBonusCap);
            float liveComboValue = static_cast<float>(livePinsHit + greedyComboBonus) + liveBaseCombo;

            liveImpactPreview = 10 + livePinValueSum;
            liveComboPreview = std::max(1, static_cast<int>(std::lround(liveComboValue)));
        }
        if (pendingReset && pendingShotScored &&
            pendingScoreVisualTimer < pendingScoreVisualDuration) {
            displayedRoundScore = pendingRoundScoreBeforeShot;
        }

        if (activeItems.duplicateCharges > 0 || activeItems.swapCharges > 0) {
            pinPowerHintLine1 = "N Duplicate(" + std::to_string(activeItems.duplicateCharges) +
                                ")   V Swap(" + std::to_string(activeItems.swapCharges) + ")";
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
        action = ui.drawXtremeHUD(
            window,
            xtreme.getRound(),
            xtreme.getFrameInRound(),
            xtreme.getShotInFrame(),
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
            liveComboPreview
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

    // Now handle the action returned from the scorecard UI
    if (action == GameAction::ExitToMenu) {
        ui.setState(GameState::Menu);
        scorer.resetGame();
        xtreme.reset();
        xtremeMode = false;
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
