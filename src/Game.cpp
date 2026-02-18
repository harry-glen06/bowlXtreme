#include "Game.h"
#include "Physics.h"
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <fstream>

static int bestRound = 0;

Game::Game()
: window(sf::VideoMode(sf::Vector2u((unsigned)windowW, (unsigned)windowH)), "Bowling Prototype")
, ball(25.0f)
{
    window.setFramerateLimit(60);
    
    // Seed random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    // Fixed world camera
    view = sf::View(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(windowW, windowH)));
    window.setView(view);

    // Apply letterbox once at startup
    auto s = window.getSize();
    applyLetterbox(s.x, s.y);

    lane.init(windowW);

    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    pins = createPins(lane.centerX(), 240.0f);
    
    // Audio automatically initializes itself
    // Start on menu, so no game music yet
    
    // Load high score
    loadHighScore();
    
    // Start in menu state
    ui.setState(GameState::Menu);
    audio.playMenuMusic();
}

std::vector<Pin> Game::createPins(float centerX, float startY) {
    std::vector<Pin> out;

    float spacing = 55.0f;
    float radius  = 9.0f;

    int pinValue = 1;
    for (int row = 0; row < 4; row++) {
        int count = row + 1;
        float y = startY - row * spacing;

        float rowWidth = (count - 1) * spacing;
        float startX = centerX - rowWidth / 2.0f;

        for (int i = 0; i < count; i++) {
            float x = startX + i * spacing;
            out.emplace_back(sf::Vector2f(x, y), radius, pinValue);
            pinValue++;
        }
    }

    return out;
}

void Game::loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    } else {
        highScore = 0;
    }
}

void Game::saveHighScore() {
    if (finalScore > highScore) {
        highScore = finalScore;
        std::ofstream file("highscore.txt");
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }
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
        
        // Handle mouse clicks for menu
        if (ev->is<sf::Event::MouseButtonPressed>()) {
            auto mouseEv = ev->getIf<sf::Event::MouseButtonPressed>();
            if (mouseEv->button == sf::Mouse::Button::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                
                // Convert to world coordinates
                sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);
                
                if (ui.getState() == GameState::Menu) {
                    MenuButton clicked = ui.handleMenuClick(window, sf::Vector2i(worldPos.x, worldPos.y));
                    
                    if (clicked == MenuButton::Normal) {
                        // Start normal game
                        ui.setState(GameState::Playing);
                        scorer.resetGame();
                        gameOver = false;
                        
                        // Apply settings
                        lane.bumpersOn = ui.getBumpersDefault();
                        
                        // Reset everything
                        for (auto& pin : pins) {
                            pin.setActive(true);
                            pin.resetToOriginalPosition();
                        }
                        resetBall();
                        
                        // Start game music
                        // SWITCH MUSIC:
                        audio.playBackgroundMusic(); // This will stop menu music and start .flac
                        
                    } else if (clicked == MenuButton::Xtreme) {
                        // Start Xtreme mode
                        ui.setState(GameState::Xtreme);
                        xtreme.reset();
                        xtremeMode = true;
                        gameOver = false;

                        // Apply settings
                        lane.bumpersOn = ui.getBumpersDefault();

                        // Reset everything
                        for (auto& pin : pins) {
                            pin.setActive(true);
                            pin.resetToOriginalPosition();
                        }
                        resetBall();

                        audio.playBackgroundMusic();
                        
                    } else if (clicked == MenuButton::Settings) {
                        // Show settings (we'll implement this next)
                    }
                }
            }
        }
    }
}

void Game::resetBall() {
    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    rollLocked = false;
    rollDir = sf::Vector2f(0.0f, -1.0f);
    aimDeg = -90.0f;
    inGutter = false;
    gutterSide = 0;
    
    // Stop rolling sound
    audio.stopBallRoll();
}

void Game::startPendingReset() {
    pendingReset = true;
    resetTimer = 0.0f;
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
    int pinValueSumThisBall = 0;
    for (auto& pin : pins) {
        if (!pin.isActive()) continue;
        if (pin.isFallen()) {
            knockedThisBall++;
            pinValueSumThisBall += pin.getValue();
        }
    }

    // FIXED: Gutter ball only if ball went in gutter AND didn't knock down any pins
    if (inGutter && knockedThisBall == 0) {
        knockedThisBall = 0;  // Confirm it's a gutter ball (0 pins)
    }
    // If ball hit pins THEN went in gutter, still count the pins!

    // Record score
    if (ui.getState() == GameState::Xtreme) {
        xtreme.recordShot(knockedThisBall, pinValueSumThisBall);
    } else {
        scorer.recordBall(knockedThisBall);
    }
    
    // Handle pin resets based on game state
    if (ui.getState() == GameState::Xtreme) {
        // Xtreme: 2 frames per round, 2 shots per frame
        // After shot 1: remove fallen, reset standing
        // After shot 2: reset all pins
        if (xtreme.getShotInFrame() == 2) {
            // We just advanced from shot 1 -> shot 2
            for (auto& pin : pins) {
                if (pin.isFallen()) {
                    pin.setActive(false);
                } else if (pin.isActive()) {
                    pin.resetToOriginalPosition();
                }
            }
        } else {
            // We just finished shot 2 -> next frame, reset all pins
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
        }
    } else if (scorer.getCurrentFrame() < 10) {
        // Get the previous frame info to determine what to do
        int prevFrame = scorer.getCurrentFrame() > 0 ? scorer.getCurrentFrame() - 1 : 0;
        const auto& frames = scorer.getFrames();
        
        if (scorer.getCurrentBall() == 1 && scorer.getCurrentFrame() > 0 && 
            frames[prevFrame].isStrike) {
            // After a strike, reset all pins for new frame
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
        } else if (scorer.getCurrentBall() == 2) {
            // After ball 1 (not strike), remove fallen, reset standing
            for (auto& pin : pins) {
                if (pin.isFallen()) {
                    pin.setActive(false);
                } else if (pin.isActive()) {
                    pin.resetToOriginalPosition();
                }
            }
        } else if (scorer.getCurrentBall() == 1 && scorer.getCurrentFrame() > prevFrame) {
            // Frame advanced (spare or normal), reset all pins
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
        }
    } else {
        // 10th frame special handling
        const auto& frame10 = scorer.getFrames()[9];
        
        if (scorer.getCurrentBall() == 2) {
            if (frame10.isStrike) {
                // After strike on ball 1, reset all
                for (auto& pin : pins) {
                    pin.setActive(true);
                    pin.resetToOriginalPosition();
                }
            } else {
                // Not strike, remove fallen
                for (auto& pin : pins) {
                    if (pin.isFallen()) {
                        pin.setActive(false);
                    } else if (pin.isActive()) {
                        pin.resetToOriginalPosition();
                    }
                }
            }
        } else if (scorer.getCurrentBall() == 3) {
            if (frame10.isSpare || frame10.ball2 == 10) {
                // After spare or strike on ball 2, reset all
                for (auto& pin : pins) {
                    pin.setActive(true);
                    pin.resetToOriginalPosition();
                }
            } else if (frame10.isStrike && frame10.ball2 != 10) {
                // After strike but ball 2 wasn't strike, remove fallen
                for (auto& pin : pins) {
                    if (pin.isFallen()) {
                        pin.setActive(false);
                    } else if (pin.isActive()) {
                        pin.resetToOriginalPosition();
                    }
                }
            }
        }
    }
    
    // Check for game over
    if (ui.getState() == GameState::Xtreme) {
        if (xtreme.isGameOver()) {
            gameOver = true;
            int finalRound = xtreme.getRoundScore();
            saveHighScore();
        }
    } else {
        if (scorer.isGameOver()) {
            gameOver = true;
            finalScore = scorer.getTotalScore();
            saveHighScore();
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
            // Left bumper
            if (p.x < playL + r && v.x < 0.0f) {
                p.x = playL + r;
                sf::Vector2f n(1.0f, 0.0f);
                v = v - 2.0f * dot(v, n) * n;
                v *= 1.03f;
                ball.setVel(v);
            }

            // Right bumper
            if (p.x > playR - r && v.x > 0.0f) {
                p.x = playR - r;
                sf::Vector2f n(-1.0f, 0.0f);
                v = v - 2.0f * dot(v, n) * n;
                v *= 1.03f;
                ball.setVel(v);
            }
        } else {
            // No bumpers - enter gutter
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
            
            // Play pin hit sound
            if (impact > 50.0f) {
                float impactVolume = std::min(100.0f, 40.0f + impact * 0.5f);
                audio.playRandomPinHit(impactVolume);
            }

            // Pin falls
            if (!pin.isFallen() && impact > 50.0f) {
                pin.setFallen(true);
                float spin = (bv.x - pv.x) * 0.01f;
                pin.setAngularVel(std::clamp(spin, -6.0f, 6.0f));
            }
        }

        // Limit pin redirection
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
                
                // Play collision sound
                float pinImpact = length(v1 - v1B) + length(v2 - v2B);
                if (pinImpact > 30.0f) {
                    float collisionVolume = std::min(80.0f, 30.0f + pinImpact * 0.3f);
                    audio.playPinCollision(collisionVolume);
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

            // Force fall if hit gutter or back
            if (p.x < leftW + r || p.x > rightW - r || p.y < lane.top + r) {
                if (!pin.isFallen()) pin.setFallen(true);
            }

            // Bounce off walls
            if (p.x < leftW + r && v.x < 0.0f) { p.x = leftW + r; v.x = -v.x * 0.25f; }
            if (p.x > rightW - r && v.x > 0.0f) { p.x = rightW - r; v.x = -v.x * 0.25f; }
            if (p.y < lane.top + r && v.y < 0.0f) { p.y = lane.top + r; v.y = -v.y * 0.15f; }

            pin.setPos(p); pin.setVel(v);
        }
    }
}

void Game::update(float dt) {
    // If in menu, don't update game logic
    if (ui.getState() == GameState::Menu) {
        audio.playMenuMusic();
        return;
    }
    
    // Handle game over state
    if (gameOver) {
        static bool prevR = false;
        bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);

        static bool prevM = false;
        bool nowM = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M);

        if (nowM && !prevM) {
            // Back to menu
            ui.setState(GameState::Menu);
            gameOver = false;
            pendingReset = false;
            audio.playMenuMusic();
        }
        
        if (nowR && !prevR) {
            // Reset entire game
            if (ui.getState() == GameState::Xtreme) {
                xtreme.reset();
            } else {
                scorer.resetGame();
            }
            gameOver = false;
            finalScore = 0;
            pendingReset = false;
            
            for (auto& pin : pins) {
                pin.setActive(true);
                pin.resetToOriginalPosition();
            }
            
            resetBall();
        }
        
        prevR = nowR;
        prevM = nowM;
        return;
    }
    
    // Move/aim before roll
    if (!rollLocked && !pendingReset) {
        sf::Vector2f p = ball.getPos();
        float r = ball.getRadius();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            p.x -= moveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            p.x += moveSpeed * dt;

        float playL = lane.playLeft();
        float playR = lane.playRight();

        if (p.x < playL + r) p.x = playL + r;
        if (p.x > playR - r) p.x = playR - r;

        ball.setPos(p);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            aimDeg -= aimTurnSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            aimDeg += aimTurnSpeed * dt;

        if (aimDeg < -140.0f) aimDeg = -140.0f;
        if (aimDeg > -40.0f) aimDeg = -40.0f;
    }

    // Launch
    if (!rollLocked && !pendingReset &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        float a = degToRad(aimDeg);
        rollDir = sf::Vector2f(std::cos(a), std::sin(a));
        ball.launch(rollDir, 1600.0f);
        rollLocked = true;
        
        // Start rolling sound
        audio.startBallRoll();
    }

    // Toggle keys
    static bool prevB = false, prevR = false, prevM = false, prevC = false;

    bool nowB = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B);
    bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
    bool nowM = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M);
    bool nowC = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);

    if (nowB && !prevB) {
        lane.bumpersOn = !lane.bumpersOn;
    }

    if (nowR && !prevR) {
        if (ui.getState() == GameState::Xtreme) {
            xtreme.reset();
        } else {
            scorer.resetGame();
        }
        gameOver = false;
        finalScore = 0;
        pendingReset = false;
        
        for (auto& pin : pins) {
            pin.setActive(true);
            pin.resetToOriginalPosition();
        }
        
        resetBall();
    }

    // C key - change ball color
    if (nowC && !prevC) {
        int colorChoice = rand() % 8;
        sf::Color ballColor;
        
        switch(colorChoice) {
            case 0: ballColor = sf::Color(25, 55, 140); break;      // Blue
            case 1: ballColor = sf::Color(200, 30, 30); break;      // Red
            case 2: ballColor = sf::Color(20, 20, 20); break;       // Black
            case 3: ballColor = sf::Color(128, 0, 128); break;      // Purple
            case 4: ballColor = sf::Color(255, 140, 0); break;      // Orange
            case 5: ballColor = sf::Color(0, 120, 0); break;        // Green
            case 6: ballColor = sf::Color(255, 20, 147); break;     // Pink
            case 7: ballColor = sf::Color(180, 180, 0); break;      // Yellow
            default: ballColor = sf::Color(25, 55, 140); break;
        }
        
        ball.setColor(ballColor);
    }

    prevB = nowB; 
    prevR = nowR; 
    prevM = nowM; 
    prevC = nowC;

    // Update physics
    ball.update(dt);
    for (auto& pin : pins) pin.update(dt);

    // Keep ball moving forward
    if (rollLocked) {
        sf::Vector2f v = ball.getVel();
        float s = length(v);

        if (s > 0.0f && s < minRollSpeed) {
            sf::Vector2f dir = v / s;
            ball.setVel(dir * minRollSpeed);
        }
    }

    applyGuttersAndBumpers();
    doCollisions();

    // Start reset if ball hits back
    if (!pendingReset && ball.getPos().y < lane.top + ball.getRadius()) {
        ball.stop();
        rollLocked = false;
        audio.stopBallRoll();
        startPendingReset();
    }

    finishPendingResetIfReady(dt);
}

void Game::draw() {
    window.clear(sf::Color(20, 20, 20));

    // Draw menu if in menu state
    if (ui.getState() == GameState::Menu) {
        float dt = clock.getElapsedTime().asSeconds();
        ui.drawMenu(window, windowW, windowH, dt);
        window.display();
        return;
    }

    // Draw game
    lane.draw(window);
    for (const auto& pin : pins) pin.draw(window);

    // Draw aim line
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

    // Draw UI and check for actions (like exiting to menu)
    GameAction action = GameAction::None;
    if (ui.getState() == GameState::Xtreme) {
        action = ui.drawXtremeHUD(
            window,
            xtreme.getRound(),
            xtreme.getFrameInRound(),
            xtreme.getShotInFrame(),
            xtreme.getTargetScore(),
            xtreme.getRoundScore(),
            xtreme.getLastImpact(),
            xtreme.getLastCombo(),
            xtreme.getLastShotScore(),
            windowW,
            windowH
        );
    } else {
        action = ui.drawScorecard(window, scorer.getFrames(), scorer.getCurrentFrame(),
                         scorer.getCurrentBall(), highScore, windowW, windowH);
    }
    
    if (gameOver) {
        if (xtremeMode) {
            int finalRound = xtreme.getRoundScore() - 1; // rounds cleared
            bestRound = std::max(bestRound, finalRound);

            ui.drawGameOverScreen(window,
                                GameOverMode::Xtreme,
                                finalRound,
                                bestRound,
                                windowW,
                                windowH);
        } else {
            int finalScore = scorer.getTotalScore();
            highScore = std::max(highScore, finalScore);

            ui.drawGameOverScreen(window,
                                GameOverMode::NormalBowling,
                                finalScore,
                                highScore,
                                windowW,
                                windowH);
        }
    }

    // Now handle the action returned from the scorecard UI
    if (action == GameAction::ExitToMenu) {
        ui.setState(GameState::Menu);
        scorer.resetGame();
        xtreme.reset();
        xtremeMode = false;
        resetBall();
        pins = createPins(lane.centerX(), 240.0f);
        audio.stopBackgroundMusic();
        audio.stopBallRoll();
        audio.playMenuMusic();
    }

    window.display();
}

void Game::applyLetterbox(unsigned winW, unsigned winH) {
    float windowRatio = (float)winW / (float)winH;
    float viewRatio = windowW / windowH;
    float sizeX = 1.0f, sizeY = 1.0f, posX = 0.0f, posY = 0.0f;

    if (windowRatio > viewRatio) { 
        sizeX = viewRatio / windowRatio; 
        posX = (1.0f - sizeX) * 0.5f; 
    } else { 
        sizeY = windowRatio / viewRatio; 
        posY = (1.0f - sizeY) * 0.5f; 
    }

    view.setViewport(sf::FloatRect(sf::Vector2f(posX, posY), sf::Vector2f(sizeX, sizeY)));
    window.setView(view);
}