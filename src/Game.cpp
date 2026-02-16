#include "Game.h"
#include "Physics.h"
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>

Game::Game()
: window(sf::VideoMode(sf::Vector2u((unsigned)windowW, (unsigned)windowH)), "Bowling Prototype")
, ball(25.0f)
, hud(font, "", 20)
{
    window.setFramerateLimit(60);
    
    // Seed random number generator for random sounds
    srand(static_cast<unsigned>(time(nullptr)));

    // Fixed world camera
    view = sf::View(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(windowW, windowH)));
    window.setView(view);

    // Apply letterbox once at startup using current window size
    auto s = window.getSize();
    applyLetterbox(s.x, s.y);

    lane.init(windowW);

    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    pins = createPins(lane.centerX(), 240.0f);

    fontOk = font.openFromFile("assets/arial.ttf");
    hud = sf::Text(font, "", 20);
    hud.setFillColor(sf::Color::White);
    hud.setPosition(sf::Vector2f(20.0f, 15.0f));
    
    for (auto& frame : frames) {
        frame = FrameScore();
    }
    currentFrame = 0;
    currentBall = 1;
    totalScore = 0;
    
    // Load sound effects and background music
    loadSounds();
}

int Game::getPinsKnocked() {
    int knocked = 0;
    for (auto& pin : pins) {
        if (!pin.isActive()) {
            knocked++;
        }
    }
    return knocked;
}

void Game::loadSounds() {
    // In SFML 3, we initialize the unique_ptr by passing the buffer into the constructor
    // This solves the "no default constructor" error.
    
    if (ballRollBuffer.loadFromFile("assets/ball_roll.wav")) {
        ballRollSound = std::make_unique<sf::Sound>(ballRollBuffer);
        ballRollSound->setLooping(true);  // SFML 3: setLooping instead of setLoop
        ballRollSound->setVolume(95.0f);
    }
    
    // Load 5 different pin hit sounds
    if (pinHitBuffer1.loadFromFile("assets/pin_hit1.wav")) {
        pinHitSound1 = std::make_unique<sf::Sound>(pinHitBuffer1);
        pinHitSound1->setVolume(5.0f);
    }
    
    if (pinHitBuffer2.loadFromFile("assets/pin_hit2.wav")) {
        pinHitSound2 = std::make_unique<sf::Sound>(pinHitBuffer2);
        pinHitSound2->setVolume(5.0f);
    }
    
    if (pinHitBuffer3.loadFromFile("assets/pin_hit3.wav")) {
        pinHitSound3 = std::make_unique<sf::Sound>(pinHitBuffer3);
        pinHitSound3->setVolume(5.0f);
    }
    
    if (pinHitBuffer4.loadFromFile("assets/pin_hit4.wav")) {
        pinHitSound4 = std::make_unique<sf::Sound>(pinHitBuffer4);
        pinHitSound4->setVolume(5.0f);
    }
    
    if (pinHitBuffer5.loadFromFile("assets/pin_hit5.wav")) {
        pinHitSound5 = std::make_unique<sf::Sound>(pinHitBuffer5);
        pinHitSound5->setVolume(5.0f);
    }
    
    // Optional: try to load dedicated pin collision sound
    if (pinCollisionBuffer.loadFromFile("assets/pin_collision.wav")) {
        pinCollisionSound = std::make_unique<sf::Sound>(pinCollisionBuffer);
        pinCollisionSound->setVolume(5.0f);
    } else if (pinHitSound1) {
        // Reuse pin_hit1 for collision sound if no dedicated file exists
        pinCollisionSound = std::make_unique<sf::Sound>(pinHitBuffer1);
        pinCollisionSound->setVolume(5.0f);
    }

    // Load Background Music
    if (backgroundMusic.openFromFile("assets/background_music.flac")) {
        backgroundMusic.setLooping(true);
        backgroundMusic.setVolume(masterVolume * 0.9f); // Keep music quieter than effects
        backgroundMusic.play();
    }
    
    soundsLoaded = true;
}

void Game::playRandomPinHit(float volume) {
    if (!soundsLoaded) return;
    
    // Pick a random sound from 1-5
    int randomSound = rand() % 5 + 1;
    
    sf::Sound* sound = nullptr;
    // Use .get() to get the raw pointer address from the unique_ptr
    switch(randomSound) {
        case 1: sound = pinHitSound1.get(); break;
        case 2: sound = pinHitSound2.get(); break;
        case 3: sound = pinHitSound3.get(); break;
        case 4: sound = pinHitSound4.get(); break;
        case 5: sound = pinHitSound5.get(); break;
    }
    
    // Check if the sound exists (it might be null if the file failed to load)
    if (sound && sound->getStatus() != sf::Sound::Status::Playing) {
        sound->setVolume(volume);
        sound->play();
    }
}

std::vector<Pin> Game::createPins(float centerX, float startY) {
    std::vector<Pin> out;

    float spacing =50.0f; // keep
    float radius  = 9.0f; // keep

    for (int row = 0; row < 4; row++) {
        int count = row + 1;
        float y = startY - row * spacing;

        float rowWidth = (count - 1) * spacing;
        float startX = centerX - rowWidth / 2.0f;

        for (int i = 0; i < count; i++) {
            float x = startX + i * spacing;
            out.emplace_back(sf::Vector2f(x, y), radius);
        }
    }

    return out;
}

void Game::run() {
    while (window.isOpen()) {
        handleEvents();
        float dt = clock.restart().asSeconds();
        update(dt);
        draw();
    }
}

void Game::handleEvents() {
    while (true) {
        auto ev = window.pollEvent();
        if (!ev.has_value()) break;

        if (ev->is<sf::Event::Closed>()) window.close();

        if (ev->is<sf::Event::Resized>()) {
            auto r = ev->getIf<sf::Event::Resized>();
            applyLetterbox(r->size.x, r->size.y);
        }
    }
}

void Game::resetPins() {
    pins = createPins(lane.centerX(), 240.0f);
}

void Game::resetBall() {
    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    rollLocked = false;
    rollDir = sf::Vector2f(0.0f, -1.0f);
    aimDeg = -90.0f;
    inGutter = false;
    gutterSide = 0;
    
    // Stop rolling sound - using arrow operator for unique_ptr
    if (isBallRolling && ballRollSound) {
        ballRollSound->stop();
        isBallRolling = false;
    }
}

void Game::startPendingReset() {
    pendingReset = true;
    resetTimer = 0.0f;
}

void Game::calculateScore() {
    totalScore = 0;
    
    for (int i = 0; i < 10; i++) {
        if (!frames[i].isComplete && i != currentFrame) continue;
        
        if (i < 9) {
            // Frames 1-9
            if (frames[i].isStrike) {
                // Strike: 10 + next 2 balls
                frames[i].score = 10;
                
                if (i + 1 < 10) {
                    frames[i].score += frames[i + 1].ball1;
                    
                    if (frames[i + 1].isStrike && i + 2 < 10) {
                        // Next frame is also strike
                        frames[i].score += frames[i + 2].ball1;
                    } else {
                        frames[i].score += frames[i + 1].ball2;
                    }
                }
                
            } else if (frames[i].isSpare) {
                // Spare: 10 + next 1 ball
                frames[i].score = 10;
                
                if (i + 1 < 10) {
                    frames[i].score += frames[i + 1].ball1;
                }
                
            } else {
                // Normal: just add the pins
                frames[i].score = frames[i].ball1 + frames[i].ball2;
            }
            
        } else {
            // 10th frame - just add all balls
            frames[i].score = frames[i].ball1 + frames[i].ball2 + frames[i].ball3;
        }
        
        // Running total
        if (i > 0) {
            totalScore = frames[i - 1].score;
        }
        totalScore += frames[i].score;
        
        // Store running total in frame
        frames[i].score = totalScore;
    }
}

void Game::drawScorecard(sf::RenderWindow& window) {
    if (!fontOk) return;
    
    float startX = 50.0f;
    float startY = 20.0f;
    float frameWidth = 90.0f;
    float frameHeight = 60.0f;
    
    for (int i = 0; i < 10; i++) {
        float x = startX + i * frameWidth;
        
        // Frame box
        sf::RectangleShape box(sf::Vector2f(frameWidth - 2, frameHeight));
        box.setPosition(sf::Vector2f(x, startY));  // ← FIXED
        box.setFillColor(sf::Color(40, 40, 40));
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(1.0f);
        window.draw(box);
        
        // Frame number
        sf::Text frameNum(font, std::to_string(i + 1), 14);
        frameNum.setPosition(sf::Vector2f(x + 5, startY + 2));  // ← FIXED
        frameNum.setFillColor(sf::Color(150, 150, 150));
        window.draw(frameNum);
        
        // Ball scores
        std::string ball1Str = frames[i].ball1 > 0 ? std::to_string(frames[i].ball1) : "";
        std::string ball2Str = frames[i].ball2 > 0 ? std::to_string(frames[i].ball2) : "";
        
        if (frames[i].isStrike && i < 9) {
            ball1Str = "X";
        } else if (frames[i].isSpare) {
            ball2Str = "/";
        } else if (frames[i].ball2 == 10) {
            ball2Str = "X";
        }
        
        sf::Text ball1Text(font, ball1Str, 16);
        ball1Text.setPosition(sf::Vector2f(x + frameWidth - 45, startY + 18));  // ← FIXED
        window.draw(ball1Text);
        
        sf::Text ball2Text(font, ball2Str, 16);
        ball2Text.setPosition(sf::Vector2f(x + frameWidth - 25, startY + 18));  // ← FIXED
        window.draw(ball2Text);
        
        // 10th frame has 3 balls
        if (i == 9 && frames[i].ball3 > 0) {
            std::string ball3Str = frames[i].ball3 == 10 ? "X" : std::to_string(frames[i].ball3);
            sf::Text ball3Text(font, ball3Str, 16);
            ball3Text.setPosition(sf::Vector2f(x + frameWidth - 15, startY + 5));  // ← FIXED
            window.draw(ball3Text);
        }
        
        // Frame total
        if (frames[i].isComplete || i < currentFrame) {
            sf::Text scoreText(font, std::to_string(frames[i].score), 20);
            scoreText.setPosition(sf::Vector2f(x + frameWidth / 2 - 15, startY + 35));  // ← FIXED
            scoreText.setFillColor(sf::Color::Yellow);
            window.draw(scoreText);
        }
    }
    
    // Current frame indicator
    float indicatorX = startX + currentFrame * frameWidth;
    sf::RectangleShape indicator(sf::Vector2f(frameWidth - 2, 3));
    indicator.setPosition(sf::Vector2f(indicatorX, startY + frameHeight + 2));  // ← FIXED
    indicator.setFillColor(sf::Color::Green);
    window.draw(indicator);
}

void Game::finishPendingResetIfReady(float dt) {
    if (!pendingReset) return;

    resetTimer += dt;

    bool pinsStill = true;
    for (const auto& pin : pins) {
        if (!pin.isActive()) continue;
        if (length(pin.getVel()) > pinsStillSpeed) {
            pinsStill = false;
            break;
        }
    }

    bool ready = (pinsStill && resetTimer >= endBuffer) || (resetTimer >= maxResetWait);
    if (!ready) return;

    // Count pins knocked THIS ball
    int knockedThisBall = 0;
    for (auto& pin : pins) {
        if (!pin.isActive()) continue;
        if (pin.isFallen()) {
            knockedThisBall++;
        }
    }

    // Record the score for this ball
    if (currentBall == 1) {
        frames[currentFrame].ball1 = knockedThisBall;
    } else if (currentBall == 2) {
        frames[currentFrame].ball2 = knockedThisBall;
    } else if (currentBall == 3) {  // Only 10th frame
        frames[currentFrame].ball3 = knockedThisBall;
    }

    // Handle different frame scenarios
    if (currentFrame < 9) {
        // Frames 1-9
        if (currentBall == 1 && knockedThisBall == 10) {
            // STRIKE!
            frames[currentFrame].isStrike = true;
            frames[currentFrame].isComplete = true;
            
            // Reset all pins for next frame
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
            
            currentFrame++;
            currentBall = 1;
            calculateScore();
            
        } else if (currentBall == 1) {
            // First ball, not a strike - remove fallen pins
            for (auto& pin : pins) {
                if (pin.isFallen()) {
                    pin.setActive(false);
                } else if (pin.isActive()) {
                    pin.resetToOriginalPosition();
                }
            }
            
            currentBall = 2;
            
        } else if (currentBall == 2) {
            // Second ball complete
            if (frames[currentFrame].ball1 + frames[currentFrame].ball2 == 10) {
                frames[currentFrame].isSpare = true;
            }
            frames[currentFrame].isComplete = true;
            
            // Reset all pins for next frame
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
            
            currentFrame++;
            currentBall = 1;
            calculateScore();
        }
        
    } else {
        // 10TH FRAME - Special rules
        if (currentBall == 1) {
            if (knockedThisBall == 10) {
                // Strike in 10th - get 2 more balls
                frames[currentFrame].isStrike = true;
                
                // Reset all pins
                for (auto& pin : pins) {
                    pin.setActive(true);
                    pin.resetToOriginalPosition();
                }
                
                currentBall = 2;
                
            } else {
                // Not a strike - remove fallen pins
                for (auto& pin : pins) {
                    if (pin.isFallen()) {
                        pin.setActive(false);
                    } else if (pin.isActive()) {
                        pin.resetToOriginalPosition();
                    }
                }
                
                currentBall = 2;
            }
            
        } else if (currentBall == 2) {
            int total = frames[currentFrame].ball1 + frames[currentFrame].ball2;
            
            if (total == 10) {
                // Spare in 10th - get 1 more ball
                frames[currentFrame].isSpare = true;
                
                // Reset all pins
                for (auto& pin : pins) {
                    pin.setActive(true);
                    pin.resetToOriginalPosition();
                }
                
                currentBall = 3;
                
            } else if (frames[currentFrame].isStrike) {
                // Had strike on ball 1, now ball 2 done
                if (frames[currentFrame].ball2 == 10) {
                    // Another strike - reset pins
                    for (auto& pin : pins) {
                        pin.setActive(true);
                        pin.resetToOriginalPosition();
                    }
                } else {
                    // Not a strike on ball 2 - remove fallen
                    for (auto& pin : pins) {
                        if (pin.isFallen()) {
                            pin.setActive(false);
                        } else if (pin.isActive()) {
                            pin.resetToOriginalPosition();
                        }
                    }
                }
                
                currentBall = 3;
                
            } else {
                // No strike or spare - game over
                frames[currentFrame].isComplete = true;
                calculateScore();
            }
            
        } else if (currentBall == 3) {
            // 10th frame complete
            frames[currentFrame].isComplete = true;
            calculateScore();
        }
    }

    resetBall();
    pendingReset = false;
}

void Game::applyGuttersAndBumpers() {
    sf::Vector2f p = ball.getPos();
    sf::Vector2f v = ball.getVel();
    float r = ball.getRadius();

    float playL = lane.playLeft();
    float playR = lane.playRight();

    if (inGutter) {
        if (gutterSide == -1) p.x = lane.left + lane.gutterWidth * 0.5f;
        if (gutterSide ==  1) p.x = lane.right - lane.gutterWidth * 0.5f;

        v.x = 0.0f;
        v.y = -420.0f;
    } else {
        if (lane.bumpersOn) {
            // Left bumper bounce
            if (p.x < playL + r && v.x < 0.0f) {
                p.x = playL + r;
                sf::Vector2f n(1.0f, 0.0f);
                v = v - 2.0f * dot(v, n) * n;
                v *= 1.03f; // tiny boost
                ball.setVel(v);
            }

            // Right bumper bounce
            if (p.x > playR - r && v.x > 0.0f) {
                p.x = playR - r;
                sf::Vector2f n(-1.0f, 0.0f);
                v = v - 2.0f * dot(v, n) * n;
                v *= 1.03f;
                ball.setVel(v);
            }
        } else {
            // bumpers OFF: enter gutter
            if (p.x < playL + r) { inGutter = true; gutterSide = -1; }
            if (p.x > playR - r) { inGutter = true; gutterSide =  1; }
        }
    }

    ball.setPos(p);
    ball.setVel(v);
}
void Game::doCollisions() {
    // Ball -> pin
    {
        sf::Vector2f bp = ball.getPos();
        sf::Vector2f bv = ball.getVel();
        sf::Vector2f bvStart = bv;
        float br = ball.getRadius();
        float bm = 4.0f; 

        bool hitAnyPin = false;

        for (auto& pin : pins) {
            if (!pin.isActive()) continue;

            sf::Vector2f pp = pin.getPos();
            sf::Vector2f pv = pin.getVel();
            sf::Vector2f pvBefore = pv;

            resolveCircleCollision(
                bp, bv, bm, br,
                pp, pv, (pin.isFallen() ? pin.getMass() : pin.getMass() * 0.6f), pin.getRadius(),
                0.12f
            );

            pin.setPos(pp);
            pin.setVel(pv);

            float impact = length(pv - pvBefore);
            if (impact > 5.0f) hitAnyPin = true;
            
            if (impact > 50.0f) {
                float impactVolume = std::min(100.0f, 40.0f + impact * 0.5f);
                playRandomPinHit(impactVolume);
            }

            if (!pin.isFallen() && impact > 60.0f) {
                pin.setFallen(true);
                float spin = (bv.x - pv.x) * 0.01f;
                pin.setAngularVel(std::clamp(spin, -6.0f, 6.0f));
            }
        }

        // Limit pin redirection for bowling feel
        if (hitAnyPin && rollLocked) {
            float maxDeltaSide = 120.0f;
            float dx = bv.x - bvStart.x;
            bv.x = bvStart.x + std::clamp(dx, -maxDeltaSide, maxDeltaSide);
            bv.x = std::clamp(bv.x, -180.0f, 180.0f);
            if (bv.y > -260.0f) bv.y = -260.0f;
        }

        ball.setPos(bp);
        ball.setVel(bv);
    }

    // Pin -> pin
    {
        for (size_t i = 0; i < pins.size(); i++) {
            if (!pins[i].isActive()) continue;
            for (size_t j = i + 1; j < pins.size(); j++) {
                if (!pins[j].isActive()) continue;

                sf::Vector2f p1 = pins[i].getPos(), v1 = pins[i].getVel();
                sf::Vector2f p2 = pins[j].getPos(), v2 = pins[j].getVel();
                sf::Vector2f v1B = v1, v2B = v2;

                resolveCircleCollision(p1, v1, pins[i].getMass(), pins[i].getRadius(),
                                       p2, v2, pins[j].getMass(), pins[j].getRadius(), 0.50f);
                
                float pinImpact = length(v1 - v1B) + length(v2 - v2B);
                if (soundsLoaded && pinCollisionSound && pinImpact > 30.0f && pinCollisionSound->getStatus() != sf::Sound::Status::Playing) {
                    pinCollisionSound->setVolume(std::min(80.0f, 30.0f + pinImpact * 0.3f));
                    pinCollisionSound->play();
                }

                pins[i].setPos(p1); pins[i].setVel(v1);
                pins[j].setPos(p2); pins[j].setVel(v2);
            }
        }
    }

    // Pin -> lane walls
    {
        float leftW = lane.playLeft(), rightW = lane.playRight();
        for (auto& pin : pins) {
            if (!pin.isActive()) continue;
            sf::Vector2f p = pin.getPos(), v = pin.getVel();
            float r = pin.getRadius();

            if (p.x < leftW + r || p.x > rightW - r || p.y < lane.top + r) {
                if (!pin.isFallen()) pin.setFallen(true);
            }

            if (p.x < leftW + r && v.x < 0.0f) { p.x = leftW + r; v.x = -v.x * 0.25f; }
            if (p.x > rightW - r && v.x > 0.0f) { p.x = rightW - r; v.x = -v.x * 0.25f; }
            if (p.y < lane.top + r && v.y < 0.0f) { p.y = lane.top + r; v.y = -v.y * 0.15f; }

            pin.setPos(p); pin.setVel(v);
        }
    }
}

/*
void Game::updateHud() {
    if (!fontOk) return;

    // Correctly check music status for HUD
    std::string musicStatus = (backgroundMusic.getStatus() == sf::SoundSource::Status::Playing) ? "ON" : "OFF";

    hud.setString(
        "Frame: " + std::to_string(frame) +
        "   Shot: " + std::to_string(shot) +
        "   Score: " + std::to_string(totalScore) +
        "   Bumpers: " + std::string(lane.bumpersOn ? "ON" : "OFF") +
        "   Music: " + musicStatus
    );
}
*/

void Game::update(float dt) {
    // Aiming and Movement logic
    if (!rollLocked && !pendingReset) {
        sf::Vector2f p = ball.getPos();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) p.x -= moveSpeed * dt * 0.8;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) p.x += moveSpeed * dt * 0.8;

        float playL = lane.playLeft(), playR = lane.playRight();
        p.x = std::clamp(p.x, playL + ball.getRadius(), playR - ball.getRadius());
        ball.setPos(p);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) aimDeg -= aimTurnSpeed * dt * 0.6;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) aimDeg += aimTurnSpeed * dt * 0.6;
        aimDeg = std::clamp(aimDeg, -140.0f, -40.0f);
    }

    // Launch Logic
    if (!rollLocked && !pendingReset && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        float a = degToRad(aimDeg);
        rollDir = sf::Vector2f(std::cos(a), std::sin(a));
        ball.launch(rollDir, 1500.0f);
        rollLocked = true;
        if (soundsLoaded && ballRollSound) {
            ballRollSound->play();
            isBallRolling = true;
        }
    }

    // Toggle logic (Edge detection)
    static bool prevB = false, prevR = false, prevM = false, prevC =false;
    bool nowB = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B);
    bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
    bool nowM = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M);
    bool nowC = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);

    if (nowB && !prevB) lane.bumpersOn = !lane.bumpersOn;
    
    // Music Toggle logic
    if (nowM && !prevM) {
        if (backgroundMusic.getStatus() == sf::SoundSource::Status::Playing)
            backgroundMusic.pause();
        else
            backgroundMusic.play();
    }

    if (nowR && !prevR) {
        // Reset entire game
        for (auto& frame : frames) {
            frame = FrameScore();
        }
        currentFrame = 0;
        currentBall = 1;
        totalScore = 0;
        pendingReset = false;
        
        for (auto& pin : pins) {
            pin.setActive(true);
            pin.resetToOriginalPosition();
        }
        
        resetBall();
    }

    if (nowC && !prevC) {
    int colorChoice = rand() % 8;
    sf::Color ballColor;
    
    switch(colorChoice) {
        case 0: ballColor = sf::Color(25, 55, 140); break;      // Deep Blue
        case 1: ballColor = sf::Color(200, 30, 30); break;      // Red
        case 2: ballColor = sf::Color(20, 20, 20); break;       // Black
        case 3: ballColor = sf::Color(128, 0, 128); break;      // Purple
        case 4: ballColor = sf::Color(255, 140, 0); break;      // Orange
        case 5: ballColor = sf::Color(0, 120, 0); break;        // Dark Green
        case 6: ballColor = sf::Color(255, 20, 147); break;     // Pink
        case 7: ballColor = sf::Color(180, 180, 0); break;      // Yellow/Gold
        default: ballColor = sf::Color(25, 55, 140); break;
    }
    
    ball.setColor(ballColor);
}

    prevB = nowB; prevR = nowR; prevM = nowM; prevC = nowC;

    // Physics updates
    ball.update(dt);
    for (auto& pin : pins) pin.update(dt);

    if (rollLocked) {
        sf::Vector2f v = ball.getVel();
        float s = length(v);
        if (s > 0.0f && s < minRollSpeed) ball.setVel((v / s) * minRollSpeed);
    }

    applyGuttersAndBumpers();
    // Apply oil effect
    if (rollLocked) {
        int pinsStanding = 0;
        for (const auto& pin : pins) {
            if (pin.isActive()) pinsStanding++;
        }
        
        sf::Vector2f v = ball.getVel();
        applyOilEffect(v, pinsStanding);
        ball.setVel(v);
    }
    doCollisions();

    // Reset check
    if (!pendingReset && ball.getPos().y < lane.top + ball.getRadius()) {
        ball.stop();
        rollLocked = false;
        if (isBallRolling && ballRollSound) { ballRollSound->stop(); isBallRolling = false; }
        startPendingReset();
    }

    finishPendingResetIfReady(dt);
}

void Game::draw() {
    window.clear(sf::Color(20, 20, 20));

    lane.draw(window);
    for (const auto& pin : pins) pin.draw(window);

    if (!rollLocked && !pendingReset) {
        float a = degToRad(aimDeg);
        sf::Vector2f dir(std::cos(a), std::sin(a));
        sf::Vertex line[2] = { 
            {ball.getPos(), sf::Color::Yellow}, 
            {ball.getPos() + dir * 100.0f, sf::Color::Yellow} 
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    ball.draw(window);
    
    // Draw scorecard instead of old HUD
    drawScorecard(window);

    window.display();
}

void Game::applyLetterbox(unsigned winW, unsigned winH) {
    float windowRatio = (float)winW / (float)winH;
    float viewRatio = windowW / windowH;
    float sizeX = 1.0f, sizeY = 1.0f, posX = 0.0f, posY = 0.0f;

    if (windowRatio > viewRatio) { sizeX = viewRatio / windowRatio; posX = (1.0f - sizeX) * 0.5f; }
    else { sizeY = windowRatio / viewRatio; posY = (1.0f - sizeY) * 0.5f; }

    view.setViewport(sf::FloatRect(sf::Vector2f(posX, posY), sf::Vector2f(sizeX, sizeY)));
    window.setView(view);
}