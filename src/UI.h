#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include "BowlingScorer.h"
#include "Items.h"
#include "Pin.h"

enum class GameState {
    Menu,
    Playing,
    Xtreme,
    Shop,
    GameOver
};

enum class GameOverMode {
    NormalBowling,
    Xtreme
};

enum class MenuButton {
    None,
    Normal,
    Xtreme,
    Settings,
};

enum class GameAction {
    None,
    ExitToMenu
};

enum class ShopItemCategory { Ball, Pin };

struct ShopOffer {
    ShopItemCategory category = ShopItemCategory::Ball;
    BallType ballType = BallType::Normal;
    PinType  pinType  = PinType::Normal;
    std::string name;
    std::string description;
    int cost;
};

class UI {
public:
    UI();
    
    void loadFont();
    
    // Menu
    void drawMenu(sf::RenderWindow& window, float windowW, float windowH, float dt);
    MenuButton handleMenuClick(sf::RenderWindow& window, sf::Vector2i mousePos);
    
    // Settings
    void drawSettings(sf::RenderWindow& window, float windowW, float windowH);
    void handleSettingsClick(sf::RenderWindow& window, sf::Vector2i mousePos);

    // Shop
    void drawShop(sf::RenderWindow& window, int tokens, float windowW, float windowH, const ActiveItems& items);
    // Returns the BallType purchased (-1 = none, index into offers)
    int  handleShopClick(sf::RenderWindow& window, sf::Vector2i mousePos, int tokens);
    void generateShopOffers();
    void resetEquippedBall() { equippedBall = BallType::Normal; }
    const std::vector<ShopOffer>& getShopOffers() const { return shopOffers; }

    // Shared inventory drawing
    void drawInventoryBar(sf::RenderWindow& window, const ActiveItems& items, float windowW, float windowH);
    void drawInventoryInShop(sf::RenderWindow& window, const ActiveItems& items, float windowW, float windowH);
    
    // Game UI
    GameAction drawScorecard(sf::RenderWindow& window, 
                       const std::array<FrameScore, 10>& frames, 
                       int currentFrame, 
                       int currentBall, 
                       int highScore,
                       float windowW, 
                       float windowH);

    // Xtreme HUD (new scoring)
    GameAction drawXtremeHUD(sf::RenderWindow& window,
                             int round,
                             int frameInRound,
                             int shotInFrame,
                             int targetScore,
                             int roundScore,
                             int tokens,
                             int impact,
                             int combo,
                             int lastShotScore,
                             float windowW,
                             float windowH,
                             const ActiveItems& items);
    
    void drawGameOverScreen(sf::RenderWindow& window,
                           GameOverMode mode,
                           int finalScore, 
                           int highScore, 
                           float windowW, 
                           float windowH);
    
    // State management
    GameState getState() const { return state; }
    void setState(GameState newState) { state = newState; }
    
    // Settings getters
    bool getBumpersDefault() const { return bumpersDefault; }
    float getMusicVolume() const { return musicVolume; }
    float getSoundVolume() const { return soundVolume; }
    int getDefaultBallColor() const { return defaultBallColor; }
    
    bool isFontLoaded() const { return fontLoaded; }
    
private:
    sf::Font font;
    bool fontLoaded = false;
    
    GameState state = GameState::Menu;
    
    // Cloud animation
    struct Cloud {
        sf::Vector2f position;
        float speed;
        float size;
    };
    std::vector<Cloud> clouds;
    void initClouds(float windowW, float windowH);
    void updateClouds(float dt, float windowW);
    void drawClouds(sf::RenderWindow& window);
    
    // Settings
    bool bumpersDefault = false;
    float musicVolume = 50.0f;
    float soundVolume = 70.0f;
    int defaultBallColor = 0;
    bool inSettings = false;

    std::vector<ShopOffer> shopOffers;
    BallType equippedBall = BallType::Normal;

    // Shared inventory rendering core
    void drawInventoryPanel(sf::RenderWindow& window, const ActiveItems& items, float x, float y, float width);
};