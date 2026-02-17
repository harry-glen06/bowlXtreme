#pragma once
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

class BowlingScorer {
public:
    BowlingScorer();
    
    // Frame management
    void recordBall(int knockedPins);
    void calculateScore();
    void resetGame();
    
    // Getters
    int getCurrentFrame() const { return currentFrame; }
    int getCurrentBall() const { return currentBall; }
    int getTotalScore() const { return totalScore; }
    bool isGameOver() const { return gameOver; }
    const std::array<FrameScore, 10>& getFrames() const { return frames; }
    
    // Frame state queries
    bool shouldRemoveFallenPins() const;
    bool shouldResetAllPins() const;
    
private:
    std::array<FrameScore, 10> frames;
    int currentFrame = 0;  // 0-9
    int currentBall = 1;   // 1, 2, or 3
    int totalScore = 0;
    bool gameOver = false;
};
