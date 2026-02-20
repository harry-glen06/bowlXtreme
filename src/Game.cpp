#include "Game.h"
#include "Physics.h"
#include "Items.h"
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <fstream>

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
    if (hasExtraPins) {
        // Spawn as a wider front row so they stay in-lane and upright.
        float y = startY + spacing * 0.85f;
        out.emplace_back(sf::Vector2f(centerX - spacing, y), radius, pinValue++);
        out.emplace_back(sf::Vector2f(centerX + spacing, y), radius, pinValue++);
    }

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
    if (activeItems.purchasedPinTypes.empty()) return;

    // Collect active Normal pins and shuffle them
    std::vector<int> available;
    for (int i = 0; i < (int)pinSet.size(); i++) {
        if (pinSet[i].isActive() && pinSet[i].getPinType() == PinType::Normal)
            available.push_back(i);
    }
    bool lockTypesThisRound = activeItems.lockPinChangesMidRound && ui.getState() == GameState::Xtreme;
    if (!lockTypesThisRound) {
        for (int i = (int)available.size()-1; i > 0; i--) {
            int j = rand() % (i+1);
            std::swap(available[i], available[j]);
        }
    }

    // Assign one pin per purchased type
    int slot = 0;
    for (int pt : activeItems.purchasedPinTypes) {
        if (slot >= (int)available.size()) break;
        pinSet[available[slot]].setPinType(static_cast<PinType>(pt));
        slot++;
    }
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
}

void Game::applyPowerPinLayout(std::vector<Pin>& pinSet) {
    if (activeItems.swapCharges > 0) {
        std::vector<int> active;
        for (int i = 0; i < (int)pinSet.size(); i++) {
            if (pinSet[i].isActive()) active.push_back(i);
        }
        if (active.size() >= 2) {
            int a = rand() % active.size();
            int b = rand() % active.size();
            while (b == a) b = rand() % active.size();
            std::swap(pinSet[active[a]], pinSet[active[b]]);
            activeItems.swapCharges--;
        }
    }

    if (activeItems.duplicateCharges > 0) {
        std::vector<int> sourcesSpecial;
        std::vector<int> sourcesAny;
        std::vector<int> targets;
        for (int i = 0; i < (int)pinSet.size(); i++) {
            if (!pinSet[i].isActive()) continue;
            sourcesAny.push_back(i);
            if (pinSet[i].getPinType() != PinType::Normal) sourcesSpecial.push_back(i);
            targets.push_back(i);
        }

        if (sourcesAny.size() >= 2) {
            std::vector<int>& sourcePool = sourcesSpecial.empty() ? sourcesAny : sourcesSpecial;
            int src = sourcePool[rand() % sourcePool.size()];

            int dst = targets[rand() % targets.size()];
            while (dst == src) dst = targets[rand() % targets.size()];

            pinSet[dst].setPinType(pinSet[src].getPinType());
            activeItems.duplicateCharges--;
        }
    }

    applyPendingRandomPinUpgrades(pinSet);
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
            applyLetterbox(r->size.x, r->size.y);
        }
        
        // Handle mouse clicks for menu
        if (ev->is<sf::Event::MouseButtonPressed>()) {
            auto mouseEv = ev->getIf<sf::Event::MouseButtonPressed>();
            if (mouseEv->button == sf::Mouse::Button::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                
                // Convert to world coordinates
                sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);
                
                if (ui.getState() == GameState::Menu) {
                    MenuButton clicked = ui.handleMenuClick(window, sf::Vector2i(worldPos.x, worldPos.y));
                    
                    if (clicked == MenuButton::Normal) {
                        xtremeMode = false;
                        ui.setState(GameState::Playing);
                        scorer.resetGame();
                        gameOver = false;
                        pendingGameOverFromScore = false;
                        postScorePauseTimer = 0.0f;
                        equipBall(BallType::Normal);
                        activeItems.resetAll();
                        ui.resetEquippedBall();
                        lane.bumpersOn = ui.getBumpersDefault();
                        pins = createPins(lane.centerX(), 240.0f);
                        resetBall();
                        audio.playBackgroundMusic();

                    } else if (clicked == MenuButton::Xtreme) {
                        ui.setState(GameState::Xtreme);
                        xtreme.reset();
                        xtremeMode = true;
                        gameOver = false;
                        pendingGameOverFromScore = false;
                        postScorePauseTimer = 0.0f;
                        equipBall(BallType::Normal);
                        activeItems.resetAll();
                        ui.resetEquippedBall();
                        ui.generateShopOffers(activeItems);
                        lane.bumpersOn = ui.getBumpersDefault();
                        pins = createPins(lane.centerX(), 240.0f);
                        resetBall();
                        audio.playBackgroundMusic();

                    } else if (clicked == MenuButton::Settings) {
                        // Show settings (we'll implement this next)
                    }
                }
                if (ui.getState() == GameState::Shop) {
                    sf::Vector2i worldPosI((int)worldPos.x, (int)worldPos.y);
                    int purchased = ui.handleShopClick(window, worldPosI, xtreme.getTokens(), activeItems);
                    const int sellReturnOffset = 10000;

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

                    auto ownedPermanentPowerCount = [&]() {
                        int count = 0;
                        for (int i = 0; i <= (int)PowerType::MoMoney; i++) {
                            PowerType p = static_cast<PowerType>(i);
                            if (isStackablePower(p)) continue;
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
                        xtreme.setBaseCombo(1.0f + activeItems.homeBaseComboBonus);
                        if (activeItems.powerBumpers) lane.bumpersOn = true;
                    };

                    if (purchased == -2) {
                        if (activeItems.skipCharges > 0 && xtreme.getTokens() >= 1) {
                            xtreme.addTokens(-1);
                            activeItems.skipCharges--;
                            ui.generateShopOffers(activeItems);
                        }
                    } else if (purchased >= sellReturnOffset) {
                        int sellIndex = purchased - sellReturnOffset;
                        const auto& offers = ui.getShopOffers();
                        if (sellIndex >= 0 && sellIndex < (int)offers.size()) {
                            const auto& offer = offers[sellIndex];
                            int sellValue = offer.cost / 2;
                            bool sold = false;

                            if (offer.category == ShopItemCategory::Ball) {
                                int slot = ui.getSelectedBallSlot();
                                if (activeItems.getBallForSlot(slot) == offer.ballType) {
                                    activeItems.setBallForSlot(slot, BallType::Normal);
                                    sold = true;
                                }
                            } else if (offer.category == ShopItemCategory::Shoe) {
                                if (activeItems.shoeType == offer.shoeType) {
                                    activeItems.applyShoeType(ShoeType::None);
                                    sold = true;
                                }
                            } else if (offer.category == ShopItemCategory::Pin) {
                                int target = static_cast<int>(offer.pinType);
                                auto it = std::find(activeItems.purchasedPinTypes.begin(),
                                                    activeItems.purchasedPinTypes.end(),
                                                    target);
                                if (it != activeItems.purchasedPinTypes.end()) {
                                    activeItems.purchasedPinTypes.erase(it);
                                    sold = true;
                                }
                            } else if (offer.category == ShopItemCategory::Power) {
                                switch (offer.powerType) {
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
                                        lane.bumpersOn = ui.getBumpersDefault();
                                        erasePowerRecord(PowerType::Bumpers, true);
                                        sold = true;
                                        break;
                                    case PowerType::HomeBase:
                                        activeItems.powerHomeBase = false;
                                        activeItems.homeBaseComboBonus = 0.0f;
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
                                }
                                if (sold) {
                                    syncRunPowersToScorer();
                                }
                            }

                            if (sold) {
                                if (sellValue > 0) xtreme.addTokens(sellValue);
                                ui.generateShopOffers(activeItems);
                            }
                        }
                    } else if (purchased >= 0) {
                        const auto& offers = ui.getShopOffers();
                        if (purchased >= (int)offers.size()) {
                            continue;
                        }
                        const auto& offer = offers[purchased];
                        bool canBuy = true;

                        if (offer.category == ShopItemCategory::Power) {
                            const int maxPermanentPowers = 4;
                            bool alreadyOwned = activeItems.hasPower(offer.powerType) ||
                                                activeItems.hasPurchasedPower(offer.powerType);
                            if (!isStackablePower(offer.powerType) &&
                                !alreadyOwned &&
                                ownedPermanentPowerCount() >= maxPermanentPowers) {
                                canBuy = false;
                            }
                        }
                        if (offer.category == ShopItemCategory::Pin) {
                            bool hasExtraPins = activeItems.powerExtraPins ||
                                                activeItems.hasPurchasedPower(PowerType::ExtraPins);
                            int pinLimit = hasExtraPins ? 12 : 10;
                            if ((int)activeItems.purchasedPinTypes.size() >= pinLimit) {
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
                            auto ballBaseCost = [](BallType t) {
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
                            if (oldBall != BallType::Normal && oldBall != offer.ballType) {
                                xtreme.addTokens(ballBaseCost(oldBall) / 2);
                            }
                            activeItems.setBallForSlot(slot, offer.ballType);
                        } else if (offer.category == ShopItemCategory::Shoe) {
                            xtreme.addTokens(-offer.cost);
                            bool changedShoes = (activeItems.shoeType != offer.shoeType);
                            activeItems.applyShoeType(offer.shoeType);
                            if (offer.shoeType == ShoeType::Clown && changedShoes) {
                                xtreme.addTokens(10);
                            }
                        } else if (offer.category == ShopItemCategory::Pin) {
                            xtreme.addTokens(-offer.cost);
                            // Pin purchase: store type, will be applied next frame
                            activeItems.purchasedPinTypes.push_back(
                                static_cast<int>(offer.pinType));
                        } else {
                            xtreme.addTokens(-offer.cost);
                            activeItems.applyPower(offer.powerType);
                            if (offer.powerType == PowerType::Bumpers) {
                                lane.bumpersOn = true;
                            }
                            syncRunPowersToScorer();
                        }
                    }

                    // Continue button
                    if (worldPos.x > windowW/2.f - 110.f && worldPos.x < windowW/2.f + 110.f &&
                        worldPos.y > windowH - 100.f    && worldPos.y < windowH - 40.f) {
                        ui.setState(GameState::Xtreme);
                        postScorePauseTimer = 0.0f;
                        ui.generateShopOffers(activeItems);
                        // Fresh pins for the new round with purchased types applied
                        pins = createPins(lane.centerX(), 240.0f);
                        applyPurchasedPinTypes(pins);
                        applyPowerPinLayout(pins);
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
                        xtreme.setBaseCombo(1.0f + activeItems.homeBaseComboBonus);
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
        case BallType::OddBall: val = (val % 2 != 0) ? val * 2 : val / 2; break;
        default: break;
    }
    return val;
}

int Game::computePinValueSumWithItems(const std::vector<int>& hitIndices) {
    int total = 0;
    for (int idx : hitIndices) {
        total += computePinValueWithItems(idx);
    }
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
}

void Game::startPendingReset() {
    pendingReset = true;
    resetTimer = 0.0f;
    shotRollTimer = 0.0f;
    backlineJamTimer = 0.0f;
}


void Game::finishPendingResetIfReady(float dt) {
    if (!pendingReset) return;

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

    // ── Special pin effects at score time ────────────────────────────────────

    // Gold pins: +1 token each when knocked
    for (int idx : hitPinIndices) {
        if (pins[idx].getPinType() == PinType::Gold) {
            xtreme.addTokens(1);
        }
    }

    // ThirdTime: increment counter for each ThirdTime pin knocked.
    // Every 3rd score event on that pin grants one combo doubling.
    for (int idx : hitPinIndices) {
        if (pins[idx].getPinType() == PinType::ThirdTime) {
            pins[idx].incrementTimesScored();
            if (pins[idx].getTimesScored() % 3 == 0) {
                activeItems.thirdTimeComboBonus += 1; // +1 to pinsHit (combo) count
            }
        }
    }

    // CopyCat: already resolved in doCollisions (type changed on first hit)

    // Compute value sum with ball item effects + pin effects
    int pinValueSumThisBall = computePinValueSumWithItems(hitPinIndices);

    // LuckyDucky: if luckyZero, subtract that pin's actual contribution.
    for (int idx : hitPinIndices) {
        if (pins[idx].getPinType() == PinType::LuckyDucky && pins[idx].isLuckyZero()) {
            pinValueSumThisBall -= computePinValueWithItems(idx);
            knockedThisBall--;   // doesn't count toward pin-count scoring either
        }
    }
    if (knockedThisBall < 0) knockedThisBall = 0;
    if (pinValueSumThisBall < 0) pinValueSumThisBall = 0;

    // Greedy: every current dollar/token adds +5 score contribution.
    if (ui.getState() == GameState::Xtreme && activeItems.powerGreedy) {
        pinValueSumThisBall += xtreme.getTokens() * 5;
    }

    // Midas ball: gold-marked pins grant +1 token each
    if (activeItems.ballType == BallType::Midas) {
        for (int idx : activeItems.goldPinIndices) {
            if (pins[idx].isFallen() && pins[idx].getPinType() != PinType::Gold) {
                xtreme.addTokens(1);
            }
        }
        activeItems.goldPinIndices.clear();
    }

    // ThirdTime: each trigger doubles combo once.
    if (activeItems.thirdTimeComboBonus > 0 && knockedThisBall > 0) {
        int doublings = std::min(activeItems.thirdTimeComboBonus, 4); // cap at x16
        knockedThisBall *= (1 << doublings);
    }

    // Gutter ball
    if (inGutter && knockedThisBall == 0) {
        knockedThisBall = 0;
    }

    // Record score
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
        xtreme.recordShot(knockedThisBall, pinValueSumThisBall, strikeThisShot);

        if (activeItems.powerHomeBase && physicalPinsDownThisShot > 0) {
            activeItems.homeBaseComboBonus += 0.01f;
            xtreme.setBaseCombo(1.0f + activeItems.homeBaseComboBonus);
        }

        // Random upgrade applies once after each completed frame.
        if (xtreme.getShotInFrame() == 1 && activeItems.powerRandomUpgrade) {
            activeItems.pendingRandomPinUpgrades += 1;
        }
    } else {
        scorer.recordBall(knockedThisBall);
    }

    // Handle pin resets based on game state
    if (ui.getState() == GameState::Xtreme) {
        if (xtreme.getShotInFrame() > 1) {
            // Keep frame progression shot-based. After a strike (or full clear),
            // rerack for the next shot in the same frame.
            bool rackCleared = (physicalPinsDownThisShot >= activeItems.pinsStandingAtShotStart);
            bool shouldRerack = strikeThisShot || rackCleared;
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
    
    // Check for transitions (game over / shop) and optionally pause so
    // players can see the score animation before screens change.
    bool needsPostScorePause = false;
    if (ui.getState() == GameState::Xtreme) {
        if (xtreme.isGameOver()) {
            pendingGameOverFromScore = true;
            needsPostScorePause = true;

            finalXtremeRoundsCleared = std::max(0, xtreme.getRound() - 1);

            if (finalXtremeRoundsCleared > xtremeBestRound) {
                xtremeBestRound = finalXtremeRoundsCleared;
                saveHighScore();
            }
        } else if (xtreme.isShopReady()) {
            needsPostScorePause = true;
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
    if (needsPostScorePause) {
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
        if (gutterSide == -1) p.x = lane.left + lane.gutterWidth * 0.5f;
        if (gutterSide ==  1) p.x = lane.right - lane.gutterWidth * 0.5f;

        v.x = 0.0f;
        v.y = -420.0f;
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
        float bm = ball.getMass();  // respects item mass multiplier
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
            float maxDeltaSide = 120.0f;
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
    // If in menu, don't update game logic
    if (ui.getState() == GameState::Menu) {
        audio.playMenuMusic();
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
            pendingGameOverFromScore = false;
            postScorePauseTimer = 0.0f;
            pendingReset = false;
            equipBall(BallType::Normal);
            activeItems.resetAll();
            ui.resetEquippedBall();
            audio.playMenuMusic();
        }
        
        if (nowR && !prevR) {
            if (ui.getState() == GameState::Xtreme) {
                xtreme.reset();
            } else {
                scorer.resetGame();
            }
            gameOver = false;
            pendingGameOverFromScore = false;
            postScorePauseTimer = 0.0f;
            pendingReset = false;
            equipBall(BallType::Normal);
            activeItems.resetAll();
            ui.resetEquippedBall();
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
    static bool prevB = false, prevR = false, prevM = false, prevC = false;

    bool nowB = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B);
    bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
    bool nowM = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M);
    bool nowC = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);

    if (nowB && !prevB) {
        if (!activeItems.powerBumpers) {
            lane.bumpersOn = !lane.bumpersOn;
        } else {
            lane.bumpersOn = true;
        }
    }

    if (nowR && !prevR) {
        if (ui.getState() == GameState::Xtreme) {
            xtreme.reset();
        } else {
            scorer.resetGame();
        }
        gameOver = false;
        pendingGameOverFromScore = false;
        postScorePauseTimer = 0.0f;
        pendingReset = false;
        equipBall(BallType::Normal);
        activeItems.resetAll();
        ui.resetEquippedBall();
        if (ui.getState() == GameState::Xtreme) {
            ui.generateShopOffers(activeItems);
        }
        pins = createPins(lane.centerX(), 240.0f);
        resetBall();
    }

    // C key - change ball color
    if (nowC && !prevC) {
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

    prevB = nowB; 
    prevR = nowR; 
    prevM = nowM; 
    prevC = nowC;

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
        !pendingGameOverFromScore &&
        ui.getState() == GameState::Xtreme &&
        xtreme.isShopReady()) {
        ui.setState(GameState::Shop);
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

    if (ui.getState() == GameState::Shop) {
        ui.drawShop(window, xtreme.getTokens(), windowW, windowH, activeItems);
        window.display();
        return;
    }
    // Draw game
    lane.draw(window);
    for (const auto& pin : pins) pin.draw(window);

    // Draw aim line
    if (!rollLocked && !pendingReset) {
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
    if (ui.getState() == GameState::Xtreme) {
        action = ui.drawXtremeHUD(
            window,
            xtreme.getRound(),
            xtreme.getFrameInRound(),
            xtreme.getShotInFrame(),
            xtreme.getTotalShots(),
            xtreme.getTargetScore(),
            xtreme.getRoundScore(),
            xtreme.getTokens(),
            xtreme.getLastImpact(),
            xtreme.getLastCombo(),
            xtreme.getLastShotScore(),
            windowW,
            windowH,
            activeItems
        );
    } else {
        action = ui.drawScorecard(window, scorer.getFrames(), scorer.getCurrentFrame(),
                         scorer.getCurrentBall(), normalHighScore, windowW, windowH);
    }
    
    if (gameOver) {
        if (xtremeMode) {
            ui.drawGameOverScreen(window,
                GameOverMode::Xtreme,
                finalXtremeRoundsCleared,
                xtremeBestRound,
                windowW,
                windowH);
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
        pendingGameOverFromScore = false;
        postScorePauseTimer = 0.0f;
        equipBall(BallType::Normal);
        activeItems.resetAll();
        ui.resetEquippedBall();
        resetBall();
        pins = createPins(lane.centerX(), 240.0f);
        audio.stopBackgroundMusic();
        audio.stopBallRoll();
        audio.playMenuMusic();
    }

    window.display();
}

void Game::applyLetterbox(unsigned winW, unsigned winH) {
    float windowRatio = (float)winW / (float)winH;
    float viewRatio = windowW / windowH;
    float sizeX = 1.0f, sizeY = 1.0f, posX = 0.0f, posY = 0.0f;

    if (windowRatio > viewRatio) { 
        sizeX = viewRatio / windowRatio; 
        posX = (1.0f - sizeX) * 0.5f; 
    } else { 
        sizeY = windowRatio / viewRatio; 
        posY = (1.0f - sizeY) * 0.5f; 
    }

    view.setViewport(sf::FloatRect(sf::Vector2f(posX, posY), sf::Vector2f(sizeX, sizeY)));
    window.setView(view);
}
