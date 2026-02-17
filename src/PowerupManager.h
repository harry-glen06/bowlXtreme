#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

// Define the types of things an upgrade can change
enum class UpgradeEffect {
    BallMass,      // Make the ball heavier (hits pins harder)
    BallSize,      // Larger ball (easier to hit pins)
    ScoreMult,     // Multiply points earned
    PinGravity,    // Pins stay "floaty" or fly further
    BumperPower,   // Bumpers give the ball a speed boost
};

struct UpgradeCard {
    std::string title;
    std::string description;
    UpgradeEffect effect;
    float value;      // e.g., 1.5 for 50% increase
    sf::Color color;  // For the UI card color
};

class UpgradeManager {
public:
    UpgradeManager();

    // Fill the pool with all possible upgrades
    void initPool();

    // Pick 3 random, unique cards from the pool
    std::vector<UpgradeCard> getRandomSelection(int count = 3);

    // Apply the chosen upgrade to the player's permanent "Active" list
    void applyUpgrade(const UpgradeCard& choice);

    // Getters for the Game class to use during physics/scoring
    float getTotalMassMultiplier() const;
    float getScoreMultiplier() const;
    float getBallSizeMultiplier() const;

private:
    std::vector<UpgradeCard> pool;
    std::vector<UpgradeCard> activeUpgrades;
};

#endif