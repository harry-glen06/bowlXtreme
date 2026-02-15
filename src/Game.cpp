#include "Game.h"
#include "Physics.h"
#include <string>
#include <cmath>

Game::Game()
: window(sf::VideoMode(sf::Vector2u((unsigned)windowW, (unsigned)windowH)), "Bowling Prototype")
, ball(21.0f)
, hud(font, "", 20)
{
    window.setFramerateLimit(60);

    lane.init(windowW);

    ball.reset(sf::Vector2f(windowW / 2.0f, lane.bottom - 30.0f));
    pins = createPins(lane.centerX(), 220.0f);

    fontOk = font.openFromFile("assets/arial.ttf");
    hud = sf::Text(font, "", 20);
    hud.setFillColor(sf::Color::White);
    hud.setPosition(sf::Vector2f(20.0f, 15.0f));
}

std::vector<Pin> Game::createPins(float centerX, float startY) {
    std::vector<Pin> out;

    float spacing = 35.0f; // keep
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

            if (p.x < playL + r) {
                p.x = playL + r;
                if (v.x < 0.0f) v.x = -v.x * bounce;
                if (std::abs(v.x) < minSide) v.x = minSide;
            }
            if (p.x > playR - r) {
                p.x = playR - r;
                if (v.x > 0.0f) v.x = -v.x * bounce;
                if (std::abs(v.x) < minSide) v.x = -minSide;
            }
        } else {
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
        float br = ball.getRadius();
        float bm = 4.0f;

        for (auto& pin : pins) {
            if (!pin.isActive()) continue;

            sf::Vector2f pp = pin.getPos();
            sf::Vector2f pv = pin.getVel();

            sf::Vector2f pvBefore = pv;
            sf::Vector2f bvBefore = bv;

            resolveCircleCollision(
                bp, bv, bm, br,
                pp, pv, pin.getMass(), pin.getRadius(),
                0.55f
            );

            pin.setPos(pp);
            pin.setVel(pv);

            float impact = length(pv - pvBefore);

            if (impact > 5.0f) {
                bv *= 0.92f;
            }

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

                resolveCircleCollision(
                    p1, v1, pins[i].getMass(), pins[i].getRadius(),
                    p2, v2, pins[j].getMass(), pins[j].getRadius(),
                    rest
                );

                pins[i].setPos(p1);
                pins[i].setVel(v1);
                pins[j].setPos(p2);
                pins[j].setVel(v2);
            }
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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            p.x -= moveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            p.x += moveSpeed * dt;

        float playL = lane.playLeft();
        float playR = lane.playRight();

        if (p.x < playL + r) p.x = playL + r;
        if (p.x > playR - r) p.x = playR - r;

        ball.setPos(p);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            aimDeg -= aimTurnSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            aimDeg += aimTurnSpeed * dt;

        if (aimDeg < -140.0f) aimDeg = -140.0f;
        if (aimDeg > -40.0f) aimDeg = -40.0f;
    }

    // Launch
    if (!rollLocked && !pendingReset &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        float a = degToRad(aimDeg);
        rollDir = sf::Vector2f(std::cos(a), std::sin(a));
        ball.launch(rollDir, 900.0f);
        rollLocked = true;
    }

    // Toggle bumpers
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B)) {
        lane.bumpersOn = true;
    }

    // Update physics
    ball.update(dt);
    for (auto& pin : pins) pin.update(dt);

    if (rollLocked && ball.getSpeed() < minRollSpeed) {
        ball.setVel(rollDir * minRollSpeed);
    }

    applyGuttersAndBumpers();
    doCollisions();

    // Start reset if ball hits back
    if (!pendingReset && ball.getPos().y < lane.top + ball.getRadius()) {
        ball.stop();
        rollLocked = false;
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
