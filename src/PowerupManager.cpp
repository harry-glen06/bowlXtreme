#include "UpgradeManager.h"
#include <algorithm>
#include <random>

UpgradeManager::UpgradeManager() {
    initPool();
}

void UpgradeManager::initPool() {
    pool.clear();
    // Add various cards to the pool
    pool.push_back({"Wrecking Ball", "Increase ball mass by 50%", UpgradeEffect::BallMass, 1.5f, sf::Color::Red});
    pool.push_back({"Beach Ball", "Increase ball size by 25%", UpgradeEffect::BallSize, 1.25f, sf::Color::Cyan});
    pool.push_back({"Golden Pins", "Double all points earned", UpgradeEffect::ScoreMult, 2.0f, sf::Color::Yellow});
    pool.push_back({"Super Bumpers", "Bumpers kick the ball faster", UpgradeEffect::BumperPower, 1.3f, sf::Color::Green});
}

std::vector<UpgradeCard> UpgradeManager::getRandomSelection(int count) {
    std::vector<UpgradeCard> selection = pool;
    
    // Shuffle the pool
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(selection.begin(), selection.end(), g);

    // Return the top 'count' cards
    if (selection.size() > (size_t)count) {
        selection.resize(count);
    }
    return selection;
}

void UpgradeManager::applyUpgrade(const UpgradeCard& choice) {
    activeUpgrades.push_back(choice);
}

float UpgradeManager::getTotalMassMultiplier() const {
    float mult = 1.0f;
    for (const auto& u : activeUpgrades) {
        if (u.effect == UpgradeEffect::BallMass) mult *= u.value;
    }
    return mult;
}

float UpgradeManager::getScoreMultiplier() const {
    float mult = 1.0f;
    for (const auto& u : activeUpgrades) {
        if (u.effect == UpgradeEffect::ScoreMult) mult *= u.value;
    }
    return mult;
}

float UpgradeManager::getBallSizeMultiplier() const {
    float mult = 1.0f;
    for (const auto& u : activeUpgrades) {
        if (u.effect == UpgradeEffect::BallSize) mult *= u.value;
    }
    return mult;
}