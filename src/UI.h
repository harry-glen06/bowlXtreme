#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <map>
#include <string>
#include <vector>
#include "BowlingScorer.h"
#include "Items.h"
#include "Pin.h"

enum class GameState {
    Menu,
    Settings,
    Tutorial,
    Playing,
    Xtreme,
    Shop,
    RoundSummary,
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
    Tutorial,
    Settings,
};

enum class GameAction {
    None,
    ExitToMenu
};

enum class ShopItemCategory { Ball, Pin, Shoe, Power };

struct ShopOffer {
    ShopItemCategory category = ShopItemCategory::Ball;
    BallType ballType = BallType::Normal;
    PinType  pinType  = PinType::Normal;
    ShoeType shoeType = ShoeType::None;
    PowerType powerType = PowerType::Greedy;
    std::string name;
    std::string description;
    int cost;
};

class UI {
public:
    static constexpr int ShopActionNone = -1;
    static constexpr int ShopActionReroll = -2;
    static constexpr int ShopActionSellPin = -3;
    static constexpr int ShopActionSellBallSlot1 = -4;
    static constexpr int ShopActionSellBallSlot2 = -5;
    static constexpr int ShopActionSellShoe = -6;
    static constexpr int ShopActionSellPower = -7;
    static constexpr int ShopActionSellPinByIndexBase = -1000;   // action = base - pinIndex
    static constexpr int ShopActionSellPowerByIndexBase = -2000; // action = base - powerIndex

    UI();
    
    void loadFont();
    
    // Menu
    void drawMenu(sf::RenderWindow& window, float windowW, float windowH, float dt);
    MenuButton handleMenuClick(sf::RenderWindow& window, sf::Vector2i mousePos);

    // Tutorial
    void drawTutorial(sf::RenderWindow& window, float windowW, float windowH);
    bool handleTutorialClick(sf::RenderWindow& window, sf::Vector2i mousePos);
    
    // Settings
    void drawSettings(sf::RenderWindow& window, float windowW, float windowH);
    void handleSettingsClick(sf::RenderWindow& window, sf::Vector2i mousePos);

    // Shop
    void drawShop(sf::RenderWindow& window, int tokens, float windowW, float windowH, const ActiveItems& items);
    // Returns purchase index (>=0) or one of the ShopAction* constants.
    // Per-item sell actions use ShopActionSellPinByIndexBase / ShopActionSellPowerByIndexBase.
    int  handleShopClick(sf::RenderWindow& window, sf::Vector2i mousePos, int tokens, const ActiveItems& items);
    void generateShopOffers(const ActiveItems& items);
    void recordOfferPicked(const ShopOffer& offer);
    void resetEquippedBall() { selectedBallSlot = 1; selectedPinSlot = 1; }
    int  getSelectedBallSlot() const { return selectedBallSlot; }
    int  getSelectedPinSlot() const { return selectedPinSlot; }
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
                             int totalShots,
                             int targetScore,
                             int roundScore,
                             int tokens,
                             int impact,
                             int combo,
                             int lastShotScore,
                             float windowW,
                             float windowH,
                             const ActiveItems& items,
                             const std::string& pinPowerHintLine1,
                             const std::string& pinPowerHintLine2,
                             bool useLiveFormulaPreview,
                             int liveImpactPreview,
                             int liveComboPreview);
    
    void drawGameOverScreen(sf::RenderWindow& window,
                           GameOverMode mode,
                           int finalScore, 
                           int highScore, 
                           float windowW,
                           float windowH,
                           int progressScore = -1,
                           int progressTarget = -1);
    void drawRoundSummaryPopup(sf::RenderWindow& window,
                               int roundNumber,
                               int roundScore,
                               int targetScore,
                               int tokensEarned,
                               int tokensTotal,
                               bool passed,
                               bool leadsToGameOver,
                               float windowW,
                               float windowH);
    bool handleRoundSummaryClick(sf::Vector2i mousePos) const;
    
    // State management
    GameState getState() const { return state; }
    void setState(GameState newState) { state = newState; }
    
    // Settings getters
    bool getBumpersDefault() const { return bumpersDefault; }
    float getMusicVolume() const { return musicVolume; }
    float getSoundVolume() const { return soundVolume; }
    int getDefaultBallColor() const { return defaultBallColor; }
    
    bool isFontLoaded() const { return fontLoaded; }
    const sf::Font& getFont() const { return font; }
    
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
    void updateClouds(float dt, float windowW, float windowH);
    void drawClouds(sf::RenderWindow& window);
    
    // Settings
    bool bumpersDefault = false;
    float musicVolume = 50.0f;
    float soundVolume = 70.0f;
    int defaultBallColor = 0;
    bool inSettings = false;

    std::vector<ShopOffer> shopOffers;
    int selectedBallSlot = 1;
    int selectedPinSlot = 1;
    int tutorialPage = 0;

    // Xtreme shot-score animation state
    int  hudLastShotCounter = -1;
    int  hudCountTarget = 0;
    float hudCountValue = 0.0f;
    float hudCountTimer = 0.0f;
    float hudCountDuration = 0.6f;
    float hudFormulaTimer = 0.0f;
    float hudFormulaDuration = 0.5f;
    int hudImpactTarget = 10;
    int hudComboTarget = 1;
    float hudImpactValue = 10.0f;
    float hudComboValue = 1.0f;
    bool hudCounting = false;
    bool hudCountingFormula = false;
    bool hudShowBigScore = false;
    float hudBigTimer = 0.0f;
    sf::Clock hudAnimClock;
    bool hudAnimClockPrimed = false;

    struct PickRateEntry {
        std::string category;
        std::string itemName;
        int shown = 0;
        int picked = 0;
    };
    std::map<std::string, PickRateEntry> pickRateStats;
    void recordOfferShown(const ShopOffer& offer);
    void loadPickRateStats();
    void savePickRateStats() const;
    static bool shouldTrackOfferCategory(ShopItemCategory category);
    static std::string offerCategoryLabel(ShopItemCategory category);
    static std::string offerStatKey(const ShopOffer& offer);

    // Shared inventory rendering core
    void drawInventoryPanel(sf::RenderWindow& window, const ActiveItems& items, float x, float y, float width);
    sf::FloatRect roundSummaryContinueRect{};
};
