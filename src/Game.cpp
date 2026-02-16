#include "Game.h"
#include "Physics.h"
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>

Game::Game()
: window(sf::VideoMode(sf::Vector2u((unsigned)windowW, (unsigned)windowH)), "Bowling Prototype")
, ball(21.0f)
, hud(font, "", 20)
{
    window.setFramerateLimit(60);
    
    // Seed random number generator for random sounds
    srand(static_cast<unsigned>(time(nullptr)));

    // Fixed world camera
    view = sf::View(sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(windowW, lane.height + 50)));
    window.setView(view);

    // Apply letterbox once at startup using current window size
    auto s = window.getSize();
    applyLetterbox(s.x, s.y);


    lane.init(windowW);

    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    pins = createPins(lane.centerX(), 220.0f);

    fontOk = font.openFromFile("assets/arial.ttf");
    hud = sf::Text(font, "", 20);
    hud.setFillColor(sf::Color::White);
    hud.setPosition(sf::Vector2f(20.0f, 15.0f));
    
    // Load sound effects
    loadSounds();
}

void Game::loadSounds() {
    // Try to load sound files - if they don't exist, the game will still work
    if (ballRollBuffer.loadFromFile("assets/ball_roll.wav")) {
        ballRollSound.setBuffer(ballRollBuffer);
        ballRollSound.setLooping(true);  // SFML 3: setLooping instead of setLoop
        ballRollSound.setVolume(30.0f);
    }
    
    // Load 5 different pin hit sounds
    if (pinHitBuffer1.loadFromFile("assets/pin_hit1.wav")) {
        pinHitSound1.setBuffer(pinHitBuffer1);
        pinHitSound1.setVolume(70.0f);
    }
    
    if (pinHitBuffer2.loadFromFile("assets/pin_hit2.wav")) {
        pinHitSound2.setBuffer(pinHitBuffer2);
        pinHitSound2.setVolume(70.0f);
    }
    
    if (pinHitBuffer3.loadFromFile("assets/pin_hit3.wav")) {
        pinHitSound3.setBuffer(pinHitBuffer3);
        pinHitSound3.setVolume(70.0f);
    }
    
    if (pinHitBuffer4.loadFromFile("assets/pin_hit4.wav")) {
        pinHitSound4.setBuffer(pinHitBuffer4);
        pinHitSound4.setVolume(70.0f);
    }
    
    if (pinHitBuffer5.loadFromFile("assets/pin_hit5.wav")) {
        pinHitSound5.setBuffer(pinHitBuffer5);
        pinHitSound5.setVolume(70.0f);
    }
    
    // Optional: try to load dedicated pin collision sound
    // If it doesn't exist, we'll just reuse one of the pin hit sounds
    if (pinCollisionBuffer.loadFromFile("assets/pin_collision.wav")) {
        pinCollisionSound.setBuffer(pinCollisionBuffer);
        pinCollisionSound.setVolume(50.0f);
    } else {
        // Reuse pin_hit1 for collision sound if no dedicated file exists
        pinCollisionSound.setBuffer(pinHitBuffer1);
        pinCollisionSound.setVolume(40.0f);
    }
    
    soundsLoaded = true;
}

void Game::playRandomPinHit(float volume) {
    if (!soundsLoaded) return;
    
    // Pick a random sound from 1-5
    int randomSound = rand() % 5 + 1;
    
    sf::Sound* sound = nullptr;
    switch(randomSound) {
        case 1: sound = &pinHitSound1; break;
        case 2: sound = &pinHitSound2; break;
        case 3: sound = &pinHitSound3; break;
        case 4: sound = &pinHitSound4; break;
        case 5: sound = &pinHitSound5; break;
    }
    
    if (sound && sound->getStatus() != sf::Sound::Playing) {
        sound->setVolume(volume);
        sound->play();
    }
}

std::vector<Pin> Game::createPins(float centerX, float startY) {
    std::vector<Pin> out;

    float spacing = 45.0f; // keep
    float radius  = 12.0f; // keep

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
    pins = createPins(lane.centerX(), 220.0f);
}

void Game::resetBall() {
    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    rollLocked = false;
    rollDir = sf::Vector2f(0.0f, -1.0f);
    aimDeg = -90.0f;
    inGutter = false;
    gutterSide = 0;
    
    // Stop rolling sound
    if (isBallRolling) {
        ballRollSound.stop();
        isBallRolling = false;
    }
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

    // Count fallen pins and remove them
    int knockedThisShot = 0;
    for (auto& pin : pins) {
        if (!pin.isActive()) continue;
        if (pin.isFallen()) {
            knockedThisShot++;
            pin.setActive(false);
        }
    }

    totalScore += knockedThisShot;

    // Strike
    if (shot == 1 && knockedThisShot == 10) {
        frame++;
        shot = 1;
        resetPins();
        resetBall();
        pendingReset = false;
        return;
    }

    // Normal shot advance
    shot++;
    if (shot == 3) {
        frame++;
        shot = 1;
        resetPins();
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
            float minSide = 80.0f;
            float bounce = 0.75f;

            // Left bumper: only bounce if moving left into it
            if (p.x < playL + r && v.x < 0.0f) {
                p.x = playL + r;

                float speed = length(v);

                sf::Vector2f n(1.0f, 0.0f);   // bumper normal pointing right
                v = v - 2.0f * dot(v, n) * n; // reflect direction

                v *= 1.03f; // tiny boost (real bumpers feel springy)

                ball.setVel(v);
            }

            // Right bumper: only bounce if moving right into it
            if (p.x > playR - r && v.x > 0.0f) {
                p.x = playR - r;

                float speed = length(v);

                sf::Vector2f n(-1.0f, 0.0f);  // bumper normal pointing left
                v = v - 2.0f * dot(v, n) * n;

                v *= 1.03f;

                ball.setVel(v);
            }
        } else {
            // bumpers OFF: enter gutter and lock slide mode
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
    sf::Vector2f bvStart = bv;           // for limiting pin-caused sideways change
    float br = ball.getRadius();
    float bm = 4.0f;                     // ball heavier

    bool slowedThisFrame = false;
    bool hitAnyPin = false;

    for (auto& pin : pins) {
        if (!pin.isActive()) continue;

        sf::Vector2f pp = pin.getPos();
        sf::Vector2f pv = pin.getVel();

        sf::Vector2f pvBefore = pv;
        sf::Vector2f bvBefore = bv;

        // Much lower restitution = less bouncy pin hits
        resolveCircleCollision(
            bp, bv, bm, br,
            pp, pv, (pin.isFallen() ? pin.getMass() : pin.getMass() * 0.6f), pin.getRadius(),
            0.12f
        );

        pin.setPos(pp);
        pin.setVel(pv);

        float impact = length(pv - pvBefore);

        if (impact > 5.0f) hitAnyPin = true;
        
        // Play random pin hit sound based on impact strength
        if (impact > 50.0f) {
            float impactVolume = std::min(100.0f, 40.0f + impact * 0.5f);
            playRandomPinHit(impactVolume);
        }

        // Tiny slowdown, only once per frame
        if (!slowedThisFrame && impact > 5.0f) {
            bv *= 0.99f;                 // very gentle slowdown
            slowedThisFrame = true;
        }

        // Fall + spin
        float fallThreshold = 80.0f;
        float spinScale = 0.01f;

        if (!pin.isFallen() && impact > fallThreshold) {
            pin.setFallen(true);

            float spin = (bvBefore.x - pvBefore.x) * spinScale;
            if (spin > 6.0f) spin = 6.0f;
            if (spin < -6.0f) spin = -6.0f;

            pin.setAngularVel(spin);
        }
    }

    // Limit how much pins can redirect the ball (bowling feel)
    if (hitAnyPin && rollLocked) {
        // Clamp sideways change caused by pins
        float maxDeltaSide = 120.0f;
        float dx = bv.x - bvStart.x;
        if (dx >  maxDeltaSide) bv.x = bvStart.x + maxDeltaSide;
        if (dx < -maxDeltaSide) bv.x = bvStart.x - maxDeltaSide;

        // Also clamp absolute sideways speed
        float maxSide = 180.0f;
        if (bv.x >  maxSide) bv.x =  maxSide;
        if (bv.x < -maxSide) bv.x = -maxSide;

        // Never allow pins to remove forward motion or send it backwards
        float minForward = 260.0f;
        if (bv.y > -minForward) bv.y = -minForward;
    }

    ball.setPos(bp);
    ball.setVel(bv);
}

    // Pin -> pin
    {
        float rest = 0.20f;
        for (size_t i = 0; i < pins.size(); i++) {
            if (!pins[i].isActive()) continue;
            for (size_t j = i + 1; j < pins.size(); j++) {
                if (!pins[j].isActive()) continue;

                sf::Vector2f p1 = pins[i].getPos();
                sf::Vector2f v1 = pins[i].getVel();

                sf::Vector2f p2 = pins[j].getPos();
                sf::Vector2f v2 = pins[j].getVel();
                
                sf::Vector2f v1Before = v1;
                sf::Vector2f v2Before = v2;

                resolveCircleCollision(
                    p1, v1, pins[i].getMass(), pins[i].getRadius(),
                    p2, v2, pins[j].getMass(), pins[j].getRadius(),
                    rest
                );
                
                // Calculate collision impact for sound
                float pinImpact = length(v1 - v1Before) + length(v2 - v2Before);
                if (soundsLoaded && pinImpact > 30.0f && pinCollisionSound.getStatus() != sf::Sound::Playing) {
                    float collisionVolume = std::min(80.0f, 30.0f + pinImpact * 0.3f);
                    pinCollisionSound.setVolume(collisionVolume);
                    pinCollisionSound.play();
                }

                pins[i].setPos(p1);
                pins[i].setVel(v1);
                pins[j].setPos(p2);
                pins[j].setVel(v2);
            }
        }
    }

    // Pin -> lane walls / gutters / back
    {
        float leftWall  = lane.playLeft();   // inside area between bumpers
        float rightWall = lane.playRight();

        float sideRest = 0.25f;  // tiny bounce
        float backRest = 0.15f;  // even smaller bounce

        for (auto& pin : pins) {
            if (!pin.isActive()) continue;

            sf::Vector2f p = pin.getPos();
            sf::Vector2f v = pin.getVel();
            float r = pin.getRadius();

            // If a pin hits the gutter zones or back end, force it to fall
            bool hitGutter = (p.x < leftWall + r) || (p.x > rightWall - r);
            bool hitBack   = (p.y < lane.top + r);

            if (hitGutter || hitBack) {
                if (!pin.isFallen()) pin.setFallen(true);
            }

            // Bounce off left/right walls (only if moving into the wall)
            if (p.x < leftWall + r && v.x < 0.0f) {
                p.x = leftWall + r;
                v.x = -v.x * sideRest;
            }
            if (p.x > rightWall - r && v.x > 0.0f) {
                p.x = rightWall - r;
                v.x = -v.x * sideRest;
            }

            // Bounce off the back (top of lane)
            if (p.y < lane.top + r && v.y < 0.0f) {
                p.y = lane.top + r;
                v.y = -v.y * backRest;
            }

            pin.setPos(p);
            pin.setVel(v);
        }
    }

}

void Game::updateHud() {
    if (!fontOk) return;

    hud.setString(
        "Frame: " + std::to_string(frame) +
        "   Shot: " + std::to_string(shot) +
        "   Score: " + std::to_string(totalScore) +
        "   Bumpers: " + std::string(lane.bumpersOn ? "ON" : "OFF")
    );
}

void Game::update(float dt) {
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
        ball.launch(rollDir, 1200.0f);
        rollLocked = true;
        
        // Start rolling sound
        if (soundsLoaded && ballRollSound.getStatus() != sf::Sound::Playing) {
            ballRollSound.play();
            isBallRolling = true;
        }
    }

    // One press toggles (edge detect)
    static bool prevB = false;
    static bool prevR = false;

    bool nowB = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B);
    bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);

    if (nowB && !prevB) {
        lane.bumpersOn = !lane.bumpersOn;
    }

    if (nowR && !prevR) {
        totalScore = 0;
        frame = 1;
        shot = 1;
        pendingReset = false;
        resetPins();
        resetBall();
    }

    prevB = nowB;
    prevR = nowR;

    // Update physics
    ball.update(dt);
    for (auto& pin : pins) pin.update(dt);

    // Keep ball moving forward without changing its direction
    if (rollLocked) {
        sf::Vector2f v = ball.getVel();
        float s = length(v);

        if (s > 0.0f && s < minRollSpeed) {
            sf::Vector2f dir = v / s;     // use current direction
            ball.setVel(dir * minRollSpeed);
        }
    }

    applyGuttersAndBumpers();
    doCollisions();

    // Start reset if ball hits back
    if (!pendingReset && ball.getPos().y < lane.top + ball.getRadius()) {
        ball.stop();
        rollLocked = false;
        
        // Stop rolling sound
        if (isBallRolling) {
            ballRollSound.stop();
            isBallRolling = false;
        }
        
        startPendingReset();
    }

    finishPendingResetIfReady(dt);

    updateHud();
}

void Game::draw() {
    window.clear(sf::Color(20, 20, 20));

    lane.draw(window);

    for (const auto& pin : pins) pin.draw(window);

    if (!rollLocked && !pendingReset) {
        float a = degToRad(aimDeg);
        sf::Vector2f dir(std::cos(a), std::sin(a));

        sf::Vertex line[2];
        line[0].position = ball.getPos();
        line[0].color = sf::Color::Yellow;
        line[1].position = ball.getPos() + dir * 100.0f;
        line[1].color = sf::Color::Yellow;

        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    ball.draw(window);

    if (fontOk) window.draw(hud);

    window.display();
}

void Game::applyLetterbox(unsigned winW, unsigned winH) {
    float windowRatio = (float)winW / (float)winH;
    float viewRatio = windowW / windowH;

    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX  = 0.0f;
    float posY  = 0.0f;

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