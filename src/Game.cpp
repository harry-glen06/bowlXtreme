#include "Game.h"
#include "Physics.h"
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>

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
    
    // Load sound effects and background music
    loadSounds();
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
        pinHitSound1->setVolume(8.0f);
    }
    
    if (pinHitBuffer2.loadFromFile("assets/pin_hit2.wav")) {
        pinHitSound2 = std::make_unique<sf::Sound>(pinHitBuffer2);
        pinHitSound2->setVolume(8.0f);
    }
    
    if (pinHitBuffer3.loadFromFile("assets/pin_hit3.wav")) {
        pinHitSound3 = std::make_unique<sf::Sound>(pinHitBuffer3);
        pinHitSound3->setVolume(8.0f);
    }
    
    if (pinHitBuffer4.loadFromFile("assets/pin_hit4.wav")) {
        pinHitSound4 = std::make_unique<sf::Sound>(pinHitBuffer4);
        pinHitSound4->setVolume(8.0f);
    }
    
    if (pinHitBuffer5.loadFromFile("assets/pin_hit5.wav")) {
        pinHitSound5 = std::make_unique<sf::Sound>(pinHitBuffer5);
        pinHitSound5->setVolume(8.0f);
    }
    
    // Optional: try to load dedicated pin collision sound
    if (pinCollisionBuffer.loadFromFile("assets/pin_collision.wav")) {
        pinCollisionSound = std::make_unique<sf::Sound>(pinCollisionBuffer);
        pinCollisionSound->setVolume(8.0f);
    } else if (pinHitSound1) {
        // Reuse pin_hit1 for collision sound if no dedicated file exists
        pinCollisionSound = std::make_unique<sf::Sound>(pinHitBuffer1);
        pinCollisionSound->setVolume(8.0f);
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

            if (!pin.isFallen() && impact > 80.0f) {
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
                                       p2, v2, pins[j].getMass(), pins[j].getRadius(), 0.20f);
                
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

void Game::update(float dt) {
    // Aiming and Movement logic
    if (!rollLocked && !pendingReset) {
        sf::Vector2f p = ball.getPos();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) p.x -= moveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) p.x += moveSpeed * dt;

        float playL = lane.playLeft(), playR = lane.playRight();
        p.x = std::clamp(p.x, playL + ball.getRadius(), playR - ball.getRadius());
        ball.setPos(p);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) aimDeg -= aimTurnSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) aimDeg += aimTurnSpeed * dt;
        aimDeg = std::clamp(aimDeg, -140.0f, -40.0f);
    }

    // Launch Logic
    if (!rollLocked && !pendingReset && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        float a = degToRad(aimDeg);
        rollDir = sf::Vector2f(std::cos(a), std::sin(a));
        ball.launch(rollDir, 1200.0f);
        rollLocked = true;
        if (soundsLoaded && ballRollSound) {
            ballRollSound->play();
            isBallRolling = true;
        }
    }

    // Toggle logic (Edge detection)
    static bool prevB = false, prevR = false, prevM = false;
    bool nowB = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B);
    bool nowR = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
    bool nowM = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M);

    if (nowB && !prevB) lane.bumpersOn = !lane.bumpersOn;
    
    // Music Toggle logic
    if (nowM && !prevM) {
        if (backgroundMusic.getStatus() == sf::SoundSource::Status::Playing)
            backgroundMusic.pause();
        else
            backgroundMusic.play();
    }

    if (nowR && !prevR) {
        totalScore = 0; frame = 1; shot = 1;
        pendingReset = false; resetPins(); resetBall();
    }

    prevB = nowB; prevR = nowR; prevM = nowM;

    // Physics updates
    ball.update(dt);
    for (auto& pin : pins) pin.update(dt);

    if (rollLocked) {
        sf::Vector2f v = ball.getVel();
        float s = length(v);
        if (s > 0.0f && s < minRollSpeed) ball.setVel((v / s) * minRollSpeed);
    }

    applyGuttersAndBumpers();
    doCollisions();

    // Reset check
    if (!pendingReset && ball.getPos().y < lane.top + ball.getRadius()) {
        ball.stop();
        rollLocked = false;
        if (isBallRolling && ballRollSound) { ballRollSound->stop(); isBallRolling = false; }
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
        sf::Vertex line[2] = { {ball.getPos(), sf::Color::Yellow}, {ball.getPos() + dir * 100.0f, sf::Color::Yellow} };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    ball.draw(window);
    if (fontOk) window.draw(hud);
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