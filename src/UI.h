#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include "BowlingScorer.h"

enum class GameState {
    Menu,
    Playing,
    GameOver
};

enum class MenuButton {
    None,
    Normal,
    Xtreme,
    Settings
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
    
    // Game UI
    void drawScorecard(sf::RenderWindow& window, 
                       const std::array<FrameScore, 10>& frames, 
                       int currentFrame, 
                       int currentBall, 
                       int highScore,
                       float windowW, 
                       float windowH);
    
    void drawGameOverScreen(sf::RenderWindow& window, 
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
    float musicVolume = 50.0f;  // 0-100
    float soundVolume = 70.0f;  // 0-100
    int defaultBallColor = 0;   // 0-7 (color index)
    bool inSettings = false;
};