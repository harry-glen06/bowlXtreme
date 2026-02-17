#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "BowlingScorer.h"

class UI {
public:
    UI();
    
    void loadFont();
    
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
    
    bool isFontLoaded() const { return fontLoaded; }
    
private:
    sf::Font font;
    bool fontLoaded = false;
};
