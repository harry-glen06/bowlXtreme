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
    const float windowW = 1000.0f;
    const float windowH = 800.0f;

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

    // Bowling state
    int totalScore = 0;
    int frame = 1;
    int shot = 1;

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
};