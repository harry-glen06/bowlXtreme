#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>

#include "Lane.h"
#include "Ball.h"
#include "Pin.h"
#include "BowlingScorer.h"
#include "XtremeScorer.h"
#include "AudioManager.h"
#include "UI.h"
#include "Items.h"
#include "Pin.h"

class Game {
public:
    Game();
    void run();

private:
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
    void processExplosions();
    void prepareNewShot();

    sf::View view;
    void applyLetterbox(unsigned winW, unsigned winH);
    
    // High score tracking
    void loadHighScore();
    void saveHighScore();
    int normalHighScore = 0;
    int xtremeBestRound = 0;
    int finalNormalScore = 0;
    int finalXtremeRoundsCleared = 0;

    // Item system
    ActiveItems activeItems;
    void equipBall(BallType type);
    void applyBlackHoleGravity(float dt);
    int  computePinValueSumWithItems(const std::vector<int>& hitPinIndices);

private:
    const float windowW = 1024.0f;
    const float windowH = 1024.0f;

    sf::RenderWindow window;
    sf::Clock clock;

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

    // Gutter state
    bool inGutter = false;
    int gutterSide = 0;

    // Reset buffering
    bool pendingReset = false;
    float resetTimer = 0.0f;
    float maxResetWait = 2.0f;
    float pinsStillSpeed = 25.0f;
    float endBuffer = 0.25f;

    // Game over state
    bool gameOver = false;
    int finalScore = 0;

    bool xtremeMode = false;
    int bestRound = 0;
    
    // High score
    int highScore = 0;
};