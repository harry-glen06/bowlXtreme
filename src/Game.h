#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

#include "Lane.h"
#include "Ball.h"
#include "Pin.h"
#include "BowlingScorer.h"
#include "XtremeScorer.h"
#include "AudioManager.h"
#include "UI.h"
#include "Items.h"

class Game {
public:
    Game();
    void run();

private:
    enum class PinPowerSelectionMode {
        None,
        DuplicatePickSource,
        DuplicatePickTarget,
        SwapPickFirst,
        SwapPickSecond
    };

    enum class BossType {
        None,
        MissingPin,
        DoubleTarget,
        LowBaseScore,
        RandomPowerDisabledEachShot,
        HeavyPins
    };

    void handleEvents();
    void update(float dt);
    void draw();

    void resetBall();

    void startPendingReset();
    void finishPendingResetIfReady(float dt);

    void applyGuttersAndBumpers();
    void doCollisions();

    std::vector<Pin> createPins(float centerX, float startY);
    void applyPurchasedPinTypes(std::vector<Pin>& pinSet);
    void applyPowerPinLayout(std::vector<Pin>& pinSet);
    void applyBossPinLayout(std::vector<Pin>& pinSet);
    bool isBossRound(int round) const;
    void refreshBossStateForCurrentRound();
    void rollBossDisabledPowerForShot();
    bool isPowerEnabledThisShot(PowerType p) const;
    void playBossLaughForCurrentBoss(float volume);
    int getShotBallSlot(int shotInFrame) const;
    std::string getActiveBossName() const;
    std::string getActiveBossDescription() const;
    void applyPendingRandomPinUpgrades(std::vector<Pin>& pinSet);
    void applySavedPinSlotValues(std::vector<Pin>& pinSet);
    void updatePinSlotValueSnapshot(const std::vector<Pin>& pinSet);
    void applyChangeIsGoodBonusForValueChange(std::vector<Pin>& pinSet, int changedPinIndex, bool includeChangedPin = false);
    void applyChangeIsGoodBonusForNewPinPurchase(int purchasedSlot);
    int countStandingPins() const;
    void processExplosions();
    void prepareNewShot();
    void cancelPinPowerSelection();
    int  findPinAtWorldPos(sf::Vector2f worldPos) const;
    bool handlePinPowerSelectionClick(sf::Vector2f worldPos);
    void applyManualDuplicate(int sourceIndex, int targetIndex);
    void applyManualSwap(int firstIndex, int secondIndex);
    void requestRunReset();
    void confirmRunReset();
    void openBossIntroIfNeeded();
    void triggerStrikeCelebration();
    void updateStrikeConfetti(float dt);
    void drawStrikeConfetti();

    sf::View view;
    void applyLetterbox(unsigned winW, unsigned winH);
    
    // High score tracking
    void loadHighScore();
    void saveHighScore();
    void loadTutorialSeenFlag();
    void saveTutorialSeenFlag() const;
    int normalHighScore = 0;
    int xtremeBestRound = 0;
    bool hasSeenTutorial = false;
    int finalNormalScore = 0;
    int finalXtremeRoundsCleared = 0;

    // Item system
    ActiveItems activeItems;
    void equipBall(BallType type);
    void applyBlackHoleGravity(float dt);
    int  computePinValueWithItems(int pinIndex) const;
    int  computePinValueSumWithItems(const std::vector<int>& hitPinIndices);

private:
    float windowW = 1024.0f;
    float windowH = 1024.0f;

    sf::RenderWindow window;
    sf::Clock clock;
    float menuAnimDt = 0.0f;

    Lane lane;
    Ball ball;
    std::vector<Pin> pins;

    // Subsystems
    BowlingScorer scorer;
    XtremeScorer xtreme;
    AudioManager audio;
    UI ui;

    // Aim + move
    float moveSpeed = 400.0f;
    float aimDeg = -90.0f;
    float aimTurnSpeed = 140.0f;

    // Rolling state
    bool rollLocked = false;
    sf::Vector2f rollDir = sf::Vector2f(0.0f, -1.0f);
    float minRollSpeed = 250.0f;
    float shotRollTimer = 0.0f;
    float backlineJamTimer = 0.0f;
    float maxShotRollTime = 7.0f;
    float backlineJamTime = 0.70f;
    float explodingPinExtraResolveDelay = 1.2f;
    bool oilEffectPendingThisFrame = false;

    // Gutter state
    bool inGutter = false;
    int gutterSide = 0;

    // Reset buffering
    bool pendingReset = false;
    bool pendingShotScored = false;
    float pendingScoreVisualTimer = 0.0f;
    float pendingScoreVisualDuration = 0.0f;
    int pendingPhysicalPinsDownThisShot = 0;
    bool pendingStrikeThisShot = false;
    int pendingRoundScoreBeforeShot = 0;
    int pendingTargetBeforeShot = 0;
    int pendingDisplayRoundBeforeShot = 0;
    int pendingDisplayFrameBeforeShot = 0;
    int pendingDisplayShotBeforeShot = 0;
    float resetTimer = 0.0f;
    float maxResetWait = 2.0f;
    float pinsStillSpeed = 25.0f;
    float endBuffer = 0.25f;
    float postScorePauseTimer = 0.0f;
    float postScorePauseDurationShop = 1.8f;
    float postScorePauseDurationGameOver = 1.0f;
    bool pendingGameOverFromScore = false;
    int xtremeRoundStartTokens = 0;
    int xtremeLastRoundScoreProgress = 0;
    int xtremeLastRoundTargetProgress = 0;
    int xtremeLastShotAddDisplay = 0;
    bool roundSummaryLeadsToGameOver = false;
    bool roundSummaryPassed = false;
    int roundSummaryRoundNumber = 1;
    int roundSummaryScore = 0;
    int roundSummaryTarget = 0;
    int roundSummaryTokensEarned = 0;
    int roundSummaryTokensTotal = 0;
    std::string roundSummaryBossName;

    // Game over state
    bool gameOver = false;
    bool resetConfirmOpen = false;

    bool xtremeMode = false;
    bool endlessMode = false;
    bool devTestMode = false;
    bool xtremeFirstShotStrikeThisFrame = false;
    int bestRound = 0;
    bool bossIntroOpen = false;
    int bossIntroShownRound = 0;
    BossType activeBoss = BossType::None;
    int activeBossRound = 0;
    int bossMissingPinSlot = -1;
    std::optional<PowerType> bossDisabledPowerThisShot;
    PinPowerSelectionMode pinPowerSelectionMode = PinPowerSelectionMode::None;
    int pinPowerFirstIndex = -1;

    struct ConfettiPiece {
        sf::Vector2f pos{};
        sf::Vector2f vel{};
        sf::Color color = sf::Color::White;
        float life = 0.0f;
        float totalLife = 0.0f;
        float size = 5.0f;
        float rotation = 0.0f;
        float spin = 0.0f;
    };
    std::vector<ConfettiPiece> strikeConfetti;
};
