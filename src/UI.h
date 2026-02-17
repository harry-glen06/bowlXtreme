#pragma once
#include <SFML/Graphics.hpp>
#include <array>

struct FrameScore {
    int ball1 = 0;
    int ball2 = 0;
    int ball3 = 0;
    int score = 0;
    bool isStrike = false;
    bool isSpare = false;
    bool isComplete = false;
};

class UI {
public:
    UI();
    
    void loadFont();
    void drawScorecard(sf::RenderWindow& window, const std::array<FrameScore, 10>& frames, 
                       int currentFrame, int currentBall, float windowW, float windowH);
    void drawGameOverScreen(sf::RenderWindow& window, int finalScore, int highScore, 
                           float windowW, float windowH);
    
private:
    sf::Font font;
    bool fontLoaded = false;
};