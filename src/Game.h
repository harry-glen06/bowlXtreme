#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <vector>
#include <memory>

#include "Lane.h"
#include "Ball.h"
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
    void resetPins();

    void startPendingReset();
    void finishPendingResetIfReady(float dt);

    void applyGuttersAndBumpers();
    void doCollisions();
    void updateHud();
    
    void loadSounds();
    void playRandomPinHit(float volume = 70.0f);

    std::vector<Pin> createPins(float centerX, float startY);

    sf::View view;

    void applyLetterbox(unsigned winW, unsigned winH);

private:
    const float windowW = 1024.0f;
    const float windowH = 1024.0f;

    sf::RenderWindow window;
    sf::Clock clock;

    Lane lane;
    Ball ball;
    std::vector<Pin> pins;

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

    // Bowling scoring (proper 10-frame system)
    struct FrameScore {
        int ball1 = 0;        // Pins knocked on first ball
        int ball2 = 0;        // Pins knocked on second ball
        int ball3 = 0;        // Only for 10th frame
        int score = 0;        // Running total after this frame
        bool isStrike = false;
        bool isSpare = false;
        bool isComplete = false;
    };

    std::array<FrameScore, 10> frames;  // 10 frames
    int currentFrame = 0;  // 0-9 (frame 1-10)
    int currentBall = 1;   // 1, 2, or 3 (ball within frame)
    int totalScore = 0;
    void calculateScore();
    void drawScorecard(sf::RenderWindow& window);
    void drawGameOverScreen(sf::RenderWindow& window);
    int getPinsKnocked();  // Count fallen pins this shot

    // HUD
    sf::Font font;
    bool fontOk = false;
    sf::Text hud;
    
    // Sound effects
    sf::SoundBuffer ballRollBuffer;
    sf::SoundBuffer pinHitBuffer1;
    sf::SoundBuffer pinHitBuffer2;
    sf::SoundBuffer pinHitBuffer3;
    sf::SoundBuffer pinHitBuffer4;
    sf::SoundBuffer pinHitBuffer5;
    sf::SoundBuffer pinCollisionBuffer;
    
    std::unique_ptr<sf::Sound> ballRollSound;
    std::unique_ptr<sf::Sound> pinHitSound1;
    std::unique_ptr<sf::Sound> pinHitSound2;
    std::unique_ptr<sf::Sound> pinHitSound3;
    std::unique_ptr<sf::Sound> pinHitSound4;
    std::unique_ptr<sf::Sound> pinHitSound5;
    std::unique_ptr<sf::Sound> pinCollisionSound;

    private:
    sf::Music backgroundMusic;  // sf::Music streams from disk, saving memory
    float masterVolume = 50.0f; // Global volume control

    bool soundsLoaded = false;
    bool isBallRolling = false;

    // Game over state
    bool gameOver = false;
    int finalScore = 0;
    
    // High score tracking
    int highScore = 0;
    void loadHighScore();
    void saveHighScore();
};