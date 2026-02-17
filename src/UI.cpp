#include "UI.h"

UI::UI() {
    loadFont();
}

void UI::loadFont() {
    fontLoaded = font.openFromFile("assets/arial.ttf");
}

void UI::drawScorecard(sf::RenderWindow& window, const std::array<FrameScore, 10>& frames,
                       int currentFrame, int currentBall, float windowW, float windowH) {
    if (!fontOk) return;
    
    float startX = 10.0f;   // Left edge
    float startY = 100.0f;  // Start below top
    float frameWidth = 70.0f;
    float frameHeight = 60.0f;
    
    for (int i = 0; i < 10; i++) {
        float x = startX;
        float y = startY + i * frameHeight;  // ← Stack vertically instead of horizontally
        
        // Frame box
        sf::RectangleShape box(sf::Vector2f(frameWidth - 2, frameHeight - 2));
        box.setPosition(sf::Vector2f(x, y));  // Use y instead of startY
        box.setFillColor(sf::Color(40, 40, 40));
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(1.0f);
        window.draw(box);
        
        // Frame number
        sf::Text frameNum(font, std::to_string(i + 1), 14);
        frameNum.setPosition(sf::Vector2f(x + 5, y + 2));  
        frameNum.setFillColor(sf::Color(150, 150, 150));
        window.draw(frameNum);
        
        // Ball scores (top right)
        std::string ball1Str = "";
        std::string ball2Str = "";

        // Ball 1 - only show if shot has been taken
        if (currentFrame > i || (currentFrame == i && currentBall > 1)) {
            // Shot 1 has been taken
            if (frames[i].ball1 == 0) {
                ball1Str = "-";  // Miss
            } else {
                ball1Str = std::to_string(frames[i].ball1);
            }
        }

        // Ball 2 - only show if shot has been taken
        if (currentFrame > i || (currentFrame == i && currentBall > 2)) {
            // Shot 2 has been taken
            if (frames[i].ball2 == 0) {
                ball2Str = "-";  // Miss
            } else {
                ball2Str = std::to_string(frames[i].ball2);
            }
        }

        // Then apply strike/spare symbols
        if (frames[i].isStrike && i < 9) {
            ball1Str = "X";
            ball2Str = "";  // No second ball on strike
        } else if (frames[i].isSpare) {
            ball2Str = "/";
        } else if (frames[i].ball2 == 10) {
            ball2Str = "X";
        }
        
        sf::Text ball1Text(font, ball1Str, 16);
        ball1Text.setPosition(sf::Vector2f(x + frameWidth - 45, y + 18));  
        window.draw(ball1Text);
        
        sf::Text ball2Text(font, ball2Str, 16);
        ball2Text.setPosition(sf::Vector2f(x + frameWidth - 25, y + 18));  
        window.draw(ball2Text);
        
        // 10th frame has 3 balls
        if (i == 9 && frames[i].ball3 > 0) {
            std::string ball3Str = frames[i].ball3 == 10 ? "X" : std::to_string(frames[i].ball3);
            sf::Text ball3Text(font, ball3Str, 16);
            ball3Text.setPosition(sf::Vector2f(x + frameWidth - 15, y + 5));  
            window.draw(ball3Text);
        }
        
        // Frame total
        if (frames[i].isComplete || i < currentFrame) {
            sf::Text scoreText(font, std::to_string(frames[i].score), 20);
            scoreText.setPosition(sf::Vector2f(x + frameWidth / 2 - 15, y + 35));  
            scoreText.setFillColor(sf::Color::Yellow);
            window.draw(scoreText);
        }
    }
    
    // Current frame indicator (vertical - goes down)
    float indicatorY = startY + currentFrame * frameHeight;  // ← Calculate Y, not X
    sf::RectangleShape indicator(sf::Vector2f(3, frameHeight - 2));  // ← Vertical bar (width 3, height full)
    indicator.setPosition(sf::Vector2f(startX + frameWidth, indicatorY));  // ← Position to the right of the frame
    indicator.setFillColor(sf::Color::Green);
    window.draw(indicator);
        
    // Show high score at bottom of scorecard
    sf::Text highScoreDisplay(font, "High Score: " + std::to_string(highScore), 16);
    highScoreDisplay.setPosition(sf::Vector2f(startX + 5, startY + (10 * frameHeight) + 10));
    highScoreDisplay.setFillColor(sf::Color::Cyan);
    window.draw(highScoreDisplay);
}

void UI::drawGameOverScreen(sf::RenderWindow& window, int finalScore, int highScore,
                            float windowW, float windowH) {
    if (!fontOk) return;
    
    // Semi-transparent overlay
    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(overlay);
    
    // Game Over text
    sf::Text gameOverText(font, "GAME FINISHED!", 80);
    gameOverText.setFillColor(sf::Color::Green);
    gameOverText.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = gameOverText.getLocalBounds();
    gameOverText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 - 220
    ));
    window.draw(gameOverText);
    
    // Final score
    sf::Text scoreText(font, "Your Score: " + std::to_string(finalScore), 50);
    scoreText.setFillColor(sf::Color::Yellow);
    bounds = scoreText.getLocalBounds();
    scoreText.setPosition(sf::Vector2f(
        windowW / 2 -bounds.size.x / 2,
        windowH / 2 - 100
    ));
    window.draw(scoreText);

    // NEW: High score
    sf::Text highScoreText(font, "High Score: " + std::to_string(highScore), 40);
    if (finalScore >= highScore && finalScore > 0) {
        highScoreText.setString("NEW HIGH SCORE!");
        highScoreText.setFillColor(sf::Color::Green);
    } else {
        highScoreText.setFillColor(sf::Color::Red);
    }
    bounds = highScoreText.getLocalBounds();
    highScoreText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 - 20
    ));
    window.draw(highScoreText);
    
    // Restart instruction
    sf::Text restartText(font, "Press R to Restart", 30);
    restartText.setFillColor(sf::Color::White);
    bounds = restartText.getLocalBounds();
    restartText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 + 100
    ));
    window.draw(restartText);
}