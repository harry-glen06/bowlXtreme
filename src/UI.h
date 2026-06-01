#pragma once
#include "raylib.h"
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
    GameWon,
    GameOver
};

enum class GameWonAction {
    None,
    EndlessMode,
    RestartRun,
    GoToMenu
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
    static constexpr int ShopActionSellBallSlot3 = -6;
    static constexpr int ShopActionSellShoe = -7;
    static constexpr int ShopActionSellPower = -8;
    static constexpr int ShopActionSellPinByIndexBase = -1000;
    static constexpr int ShopActionSellPowerByIndexBase = -2000;

    UI();

    void loadFont();

    // Menu
    void drawMenu(float windowW, float windowH, float dt);
    MenuButton handleMenuClick(float windowW, float windowH, Vector2 mousePos);

    // Tutorial
    void drawTutorial(float windowW, float windowH);
    bool handleTutorialClick(float windowW, float windowH, Vector2 mousePos);

    // Settings
    void drawSettings(float windowW, float windowH);
    void handleSettingsClick(float windowW, float windowH, Vector2 mousePos);
    void handleSettingsDrag(float windowW, float windowH, Vector2 mousePos);
    void stopSettingsDrag();

    // Shop
    void drawShop(int tokens, float windowW, float windowH, const ActiveItems& items);
    int  handleShopClick(Vector2 mousePos, int tokens, const ActiveItems& items);
    void handleShopScroll(Vector2 mousePos, float wheelDelta, float windowW, float windowH, const ActiveItems& items);
    void generateShopOffers(const ActiveItems& items);
    void recordOfferPicked(const ShopOffer& offer);
    void resetEquippedBall() { selectedBallSlot = 1; selectedPinSlot = 1; }
    int  getSelectedBallSlot() const { return selectedBallSlot; }
    int  getSelectedPinSlot() const { return selectedPinSlot; }
    const std::vector<ShopOffer>& getShopOffers() const { return shopOffers; }

    // Shared inventory drawing
    void drawInventoryBar(const ActiveItems& items, float windowW, float windowH);
    void drawInventoryInShop(const ActiveItems& items, float windowW, float windowH);

    // Game UI
    GameAction drawScorecard(const std::array<FrameScore, 10>& frames,
                             int currentFrame,
                             int currentBall,
                             int highScore,
                             float windowW,
                             float windowH);

    // Xtreme HUD
    GameAction drawXtremeHUD(float dt,
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
                             int liveComboPreview,
                             bool bossRound,
                             const std::string& bossName,
                             const std::string& bossDescription);

    void drawGameOverScreen(GameOverMode mode,
                            int finalScore,
                            int highScore,
                            float windowW,
                            float windowH,
                            int progressScore = -1,
                            int progressTarget = -1);
    void drawRoundSummaryPopup(int roundNumber,
                               int roundScore,
                               int targetScore,
                               int tokensEarned,
                               int tokensTotal,
                               bool passed,
                               bool leadsToGameOver,
                               const std::string& bossName,
                               float windowW,
                               float windowH);
    bool handleRoundSummaryClick(Vector2 mousePos) const;
    void drawGameWonPopup(int roundsCleared,
                          int tokensTotal,
                          float windowW,
                          float windowH);
    GameWonAction handleGameWonClick(Vector2 mousePos) const;

    // State management
    GameState getState() const { return state; }
    void setState(GameState newState) { state = newState; }

    // Settings getters
    bool getBumpersDefault() const { return bumpersDefault; }
    float getMusicVolume() const { return musicVolume; }
    float getSoundVolume() const { return soundVolume; }
    int getDefaultBallColor() const { return defaultBallColor; }

    bool isFontLoaded() const { return fontLoaded; }
    const Font& getFont() const { return font; }

    void setCamera(Camera2D cam) { camera = cam; }
    void setWorldMousePos(Vector2 pos) { worldMousePos = pos; }

private:
    Camera2D camera = {};
    float lastWindowW = 1024.0f;
    float lastWindowH = 768.0f;
    Vector2 worldMousePos = {};
    Font font{};
    bool fontLoaded = false;

    GameState state = GameState::Menu;

    // Cloud animation
    struct Cloud {
        Vector2 position;
        float speed;
        float size;
    };
    std::vector<Cloud> clouds;
    void initClouds(float windowW, float windowH);
    void updateClouds(float dt, float windowW, float windowH);
    void drawClouds();

    // Settings
    bool bumpersDefault = false;
    float musicVolume = 50.0f;
    float soundVolume = 70.0f;
    int defaultBallColor = 0;
    bool inSettings = false;
    bool draggingMusicSlider = false;
    bool draggingSoundSlider = false;

    std::vector<ShopOffer> shopOffers;
    int selectedBallSlot = 1;
    int selectedPinSlot = 1;
    float shopOwnedScroll = 0.0f;
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
    int hudFormulaBaseImpact = 10;
    int hudFormulaBaseCombo = 1;
    bool hudCounting = false;
    bool hudCountingFormula = false;
    bool hudShowBigScore = false;
    float hudBigTimer = 0.0f;
    bool bossInfoPopupOpen = false;
    bool xtremeMouseWasDown = false;

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
    void drawInventoryPanel(const ActiveItems& items, float x, float y, float width);
    Rectangle roundSummaryContinueRect{};
    Rectangle gameWonEndlessRect{};
    Rectangle gameWonRestartRect{};
    Rectangle gameWonMenuRect{};
};
