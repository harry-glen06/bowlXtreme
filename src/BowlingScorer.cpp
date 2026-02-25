#include "BowlingScorer.h"

BowlingScorer::BowlingScorer() {
    resetGame();
}

void BowlingScorer::resetGame() {
    for (auto& frame : frames) {
        frame = FrameScore();
    }
    currentFrame = 0;
    currentBall = 1;
    totalScore = 0;
    gameOver = false;
}

void BowlingScorer::recordBall(int knockedPins) {
    // Record the score for this ball
    if (currentBall == 1) {
        frames[currentFrame].ball1 = knockedPins;
    } else if (currentBall == 2) {
        frames[currentFrame].ball2 = knockedPins;
    } else if (currentBall == 3) {
        frames[currentFrame].ball3 = knockedPins;
    }
    
    // Handle frame progression
    if (currentFrame < 9) {
        // Frames 1-9
        if (currentBall == 1 && knockedPins == 10) {
            // STRIKE!
            frames[currentFrame].isStrike = true;
            frames[currentFrame].isComplete = true;
            currentFrame++;
            currentBall = 1;
            calculateScore();
            
        } else if (currentBall == 1) {
            // First ball, not a strike - go to ball 2
            currentBall = 2;
            
        } else if (currentBall == 2) {
            // Second ball complete
            if (frames[currentFrame].ball1 + frames[currentFrame].ball2 == 10) {
                frames[currentFrame].isSpare = true;
            }
            frames[currentFrame].isComplete = true;
            currentFrame++;
            currentBall = 1;
            calculateScore();
        }
        
    } else {
        // 10TH FRAME - Special rules
        if (currentBall == 1) {
            if (knockedPins == 10) {
                // Strike in 10th - get 2 more balls
                frames[currentFrame].isStrike = true;
                currentBall = 2;
            } else {
                // Not a strike - go to ball 2
                currentBall = 2;
            }
            
        } else if (currentBall == 2) {
            int total = frames[currentFrame].ball1 + frames[currentFrame].ball2;
            
            if (total == 10) {
                // Spare in 10th - get 1 more ball
                frames[currentFrame].isSpare = true;
                currentBall = 3;
                
            } else if (frames[currentFrame].isStrike) {
                // Had strike on ball 1, now ball 2 done - get ball 3
                currentBall = 3;
                
            } else {
                // No strike or spare - game over
                frames[currentFrame].isComplete = true;
                calculateScore();
                gameOver = true;
            }
            
        } else if (currentBall == 3) {
            // 10th frame complete
            frames[currentFrame].isComplete = true;
            calculateScore();
            gameOver = true;
        }
    }
}

void BowlingScorer::calculateScore() {
    int runningTotal = 0;
    totalScore = 0;

    for (int i = 0; i < 10; i++) {
        if (!frames[i].isComplete && i != currentFrame) continue;

        int frameScore = 0;
        if (i < 9) {
            // Frames 1-9
            if (frames[i].isStrike) {
                // Strike: 10 + next 2 balls
                frameScore = 10;

                if (i + 1 < 10) {
                    frameScore += frames[i + 1].ball1;

                    if (frames[i + 1].isStrike && i + 2 < 10) {
                        // Next frame is also strike
                        frameScore += frames[i + 2].ball1;
                    } else {
                        frameScore += frames[i + 1].ball2;
                    }
                }
            } else if (frames[i].isSpare) {
                // Spare: 10 + next 1 ball
                frameScore = 10;
                if (i + 1 < 10) {
                    frameScore += frames[i + 1].ball1;
                }
            } else {
                // Normal: just add the pins
                frameScore = frames[i].ball1 + frames[i].ball2;
            }
        } else {
            // 10th frame - just add all balls
            frameScore = frames[i].ball1 + frames[i].ball2 + frames[i].ball3;
        }

        runningTotal += frameScore;
        frames[i].score = runningTotal; // Display/API keeps cumulative frame score.
    }

    totalScore = runningTotal;
}

bool BowlingScorer::shouldRemoveFallenPins() const {
    // Remove fallen pins after ball 1 (not a strike) in frames 1-9
    if (currentFrame < 9 && currentBall == 2) {
        return true;
    }
    
    // Remove fallen pins in 10th frame under certain conditions
    if (currentFrame == 9) {
        // After ball 1 if not a strike
        if (currentBall == 2 && !frames[currentFrame].isStrike) {
            return true;
        }
        // After ball 2 if had strike but ball 2 wasn't a strike
        if (currentBall == 3 && frames[currentFrame].isStrike && frames[currentFrame].ball2 != 10) {
            return true;
        }
    }
    
    return false;
}

bool BowlingScorer::shouldResetAllPins() const {
    // Reset all pins after strike in frames 1-9
    if (currentFrame > 0 && currentFrame < 9 &&
        frames[currentFrame - 1].isStrike && currentBall == 1) {
        return true;
    }
    
    // Reset all pins in 10th frame after strike on ball 1
    if (currentFrame == 9 && currentBall == 2 && frames[currentFrame].isStrike) {
        return true;
    }
    
    // Reset all pins in 10th frame after spare
    if (currentFrame == 9 && currentBall == 3 && frames[currentFrame].isSpare) {
        return true;
    }
    
    // Reset all pins in 10th frame after strike on ball 2
    if (currentFrame == 9 && currentBall == 3 && frames[currentFrame].ball2 == 10) {
        return true;
    }
    
    return false;
}
