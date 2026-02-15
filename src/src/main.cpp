#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <cmath>
#include <string>

#include "Ball.h"
#include "Pin.h"

static float degToRad(float deg) {
    return deg * 3.1415926535f / 180.0f;
}

static int countKnocked(const std::vector<Pin>& pins) {
    int k = 0;
    for (const auto& p : pins) {
        if (p.isKnocked()) k++;
    }
    return k;
}

// Create 10 pins in bowling triangle (1 at front, 4 at back)
std::vector<Pin> createPins(float centerX, float startY) {
    std::vector<Pin> pins;
    float spacing = 35.0f;   // keep your values
    float radius = 12.0f;    // keep your values

    for (int row = 0; row < 4; row++) {
        int count = row + 1;                 // 1,2,3,4
        float y = startY - row * spacing;    // go up lane

        float rowWidth = (count - 1) * spacing;
        float startX = centerX - rowWidth / 2.0f;

        for (int i = 0; i < count; i++) {
            float x = startX + i * spacing;
            pins.emplace_back(sf::Vector2f(x, y), radius);
        }
    }

    return pins;
}

int main() {
    const float windowW = 900.0f;
    const float windowH = 600.0f;

    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u((unsigned)windowW, (unsigned)windowH)),
        "Bowling Prototype"
    );
    window.setFramerateLimit(60);

    // Lane numbers (keeping your lane size)
    float laneWidth = 246.0f;
    float laneTop = 40.0f;
    float laneHeight = 520.0f;

    float laneLeft = (windowW - laneWidth) / 2.0f;
    float laneRight = laneLeft + laneWidth;
    float laneBottom = laneTop + laneHeight;

    // Gutters and bumpers (new)
    float gutterWidth = 28.0f;      // width of gutter area (inside lane)
    float bumperThickness = 6.0f;   // bumper thickness (inside lane)
    bool bumpersOn = true;          // bumpers instead of bouncy walls

    // Lane visuals
    sf::RectangleShape lane(sf::Vector2f(laneWidth, laneHeight));
    lane.setPosition(sf::Vector2f(laneLeft, laneTop));
    lane.setFillColor(sf::Color(160, 120, 70));

    // Draw gutters as darker strips inside lane
    sf::RectangleShape leftGutter(sf::Vector2f(gutterWidth, laneHeight));
    leftGutter.setPosition(sf::Vector2f(laneLeft, laneTop));
    leftGutter.setFillColor(sf::Color(35, 35, 35));

    sf::RectangleShape rightGutter(sf::Vector2f(gutterWidth, laneHeight));
    rightGutter.setPosition(sf::Vector2f(laneRight - gutterWidth, laneTop));
    rightGutter.setFillColor(sf::Color(35, 35, 35));

    // Draw bumpers as thin bright strips right next to gutters
    sf::RectangleShape leftBumper(sf::Vector2f(bumperThickness, laneHeight));
    leftBumper.setPosition(sf::Vector2f(laneLeft + gutterWidth, laneTop));
    leftBumper.setFillColor(sf::Color::White);

    sf::RectangleShape rightBumper(sf::Vector2f(bumperThickness, laneHeight));
    rightBumper.setPosition(sf::Vector2f(laneRight - gutterWidth - bumperThickness, laneTop));
    rightBumper.setFillColor(sf::Color::White);

    float endZoneSize = 20.0f; // keep
    sf::RectangleShape topEnd(sf::Vector2f(laneWidth, endZoneSize));
    topEnd.setPosition(sf::Vector2f(laneLeft, laneTop - endZoneSize));
    topEnd.setFillColor(sf::Color::Black);

    sf::RectangleShape bottomEnd(sf::Vector2f(laneWidth, endZoneSize));
    bottomEnd.setPosition(sf::Vector2f(laneLeft, laneBottom));
    bottomEnd.setFillColor(sf::Color::Black);

    // Playable area (between bumpers)
    float playLeft = laneLeft + gutterWidth + bumperThickness;
    float playRight = laneRight - gutterWidth - bumperThickness;

    // Ball (keep your size)
    Ball ball(21.0f);
    sf::Vector2f ballStart(windowW / 2.0f, laneBottom - 30.0f);
    ball.reset(ballStart);

    // Pins (keep your startY)
    auto pins = createPins(laneLeft + laneWidth / 2.0f, 220.0f);

    // Movement and aim (keeping your values)
    float moveSpeed = 400.0f;
    float aimDeg = -90.0f;
    float aimTurnSpeed = 140.0f;

    // Roll lock and roll direction
    bool rollLocked = false;
    sf::Vector2f rollDir(0.0f, -1.0f);
    float minRollSpeed = 250.0f;

    // Bowling flow (new)
    int frame = 1;
    int shot = 1;               // 1 or 2
    int totalScore = 0;         // simple total pins knocked (not real bowling scoring yet)
    int frameStartKnocked = 0;  // how many pins were already down when the frame/shot started

    auto startShot = [&](bool keepBallX) {
        sf::Vector2f p = ballStart;
        if (keepBallX) p.x = ball.getPos().x;

        // clamp start x inside playable area
        float r = ball.getRadius();
        if (p.x < playLeft + r) p.x = playLeft + r;
        if (p.x > playRight - r) p.x = playRight - r;

        ball.reset(p);
        rollLocked = false;

        // snapshot how many pins are already down (for scoring this shot)
        frameStartKnocked = countKnocked(pins);
    };

    auto startNewFrame = [&]() {
        pins = createPins(laneLeft + laneWidth / 2.0f, 220.0f);
        shot = 1;
        frameStartKnocked = 0;
        aimDeg = -90.0f;
        startShot(false);
    };

    // Scoreboard 
    sf::Font font;
    bool fontOk = font.openFromFile("assets/arial.ttf");

    sf::Text hud(font, "", 20);
    hud.setFillColor(sf::Color::White);
    hud.setPosition(sf::Vector2f(20.0f, 15.0f));

    sf::Clock clock;

    while (window.isOpen()) {
        // Events (SFML 3)
        while (true) {
            std::optional<sf::Event> ev = window.pollEvent();
            if (!ev.has_value()) break;

            if (ev->is<sf::Event::Closed>()) window.close();
        }

        float dt = clock.restart().asSeconds();

        // Move and aim only when not rolling
        if (!rollLocked) {
            sf::Vector2f p = ball.getPos();
            float r = ball.getRadius();

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
                p.x -= moveSpeed * dt;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
                p.x += moveSpeed * dt;

            // clamp inside playable area
            if (p.x < playLeft + r) p.x = playLeft + r;
            if (p.x > playRight - r) p.x = playRight - r;

            ball.setPos(p);

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
                aimDeg -= aimTurnSpeed * dt;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                aimDeg += aimTurnSpeed * dt;

            if (aimDeg < -140.0f) aimDeg = -140.0f;
            if (aimDeg > -40.0f) aimDeg = -40.0f;
        }

        // Roll once per shot
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && !rollLocked) {
            float a = degToRad(aimDeg);
            rollDir = sf::Vector2f(std::cos(a), std::sin(a));

            ball.launch(rollDir, 650.0f);
            rollLocked = true;
        }

        // Manual reset key (starts new frame)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
            totalScore = 0;
            frame = 1;
            startNewFrame();
        }

        ball.update(dt);

        // Keep rolling until end
        if (rollLocked && ball.getSpeed() < minRollSpeed) {
            ball.setVel(rollDir * minRollSpeed);
        }

        // Gutters / bumpers behavior (no bouncy walls)
        {
            sf::Vector2f p = ball.getPos();
            sf::Vector2f v = ball.getVel();
            float r = ball.getRadius();

            // Top end: end the shot when ball reaches the back
            if (p.y < laneTop + r) {
                p.y = laneTop + r;
                ball.setPos(p);
                ball.stop();
                rollLocked = false;

                // score this shot: how many new pins fell
                int nowKnocked = countKnocked(pins);
                int shotPins = nowKnocked - frameStartKnocked;
                if (shotPins < 0) shotPins = 0;
                totalScore += shotPins;

                // Strike rule (simple): if all 10 are down after shot 1, go next frame
                if (nowKnocked >= 10 || shot == 2) {
                    frame += 1;
                    startNewFrame();
                } else {
                    shot = 2;
                    aimDeg = -90.0f;
                    startShot(true); // keep ball X for shot 2
                }
            }

            // Bottom clamp (keep ball on screen)
            if (p.y > laneBottom - r) {
                p.y = laneBottom - r;
                ball.setPos(p);
            }

            // Left/right: gutters or bumpers
            if (bumpersOn) {
                // bumpers: clamp and kill sideways velocity
                if (p.x < playLeft + r) {
                    p.x = playLeft + r;
                    v.x = 0.0f;
                }
                if (p.x > playRight - r) {
                    p.x = playRight - r;
                    v.x = 0.0f;
                }
                ball.setPos(p);
                ball.setVel(v);
            } else {
                // gutters: if ball enters gutter, end the shot
                float gutterLeftMax = laneLeft + gutterWidth + r;
                float gutterRightMin = laneRight - gutterWidth - r;

                if (p.x < gutterLeftMax || p.x > gutterRightMin) {
                    ball.stop();
                    rollLocked = false;

                    int nowKnocked = countKnocked(pins);
                    int shotPins = nowKnocked - frameStartKnocked;
                    if (shotPins < 0) shotPins = 0;
                    totalScore += shotPins;

                    if (nowKnocked >= 10 || shot == 2) {
                        frame += 1;
                        startNewFrame();
                    } else {
                        shot = 2;
                        aimDeg = -90.0f;
                        startShot(true);
                    }
                }
            }
        }

        // Ball -> pin collision (knockdown)
        {
            sf::Vector2f bp = ball.getPos();
            float br = ball.getRadius();

            for (auto& pin : pins) {
                if (pin.isKnocked()) continue;

                sf::Vector2f pp = pin.getPos();
                float pr = pin.getRadius();

                float dx = bp.x - pp.x;
                float dy = bp.y - pp.y;
                float hitDist = br + pr;

                if (dx * dx + dy * dy < hitDist * hitDist) {
                    pin.knock();
                    ball.setVel(ball.getVel() * 0.85f);
                }
            }
        }

        // HUD text
        if (font.getInfo().family != "") {
            int knocked = countKnocked(pins);
            std::string bumperText = bumpersOn ? "ON" : "OFF";
            hud.setString(
                "Frame: " + std::to_string(frame) +
                "   Shot: " + std::to_string(shot) +
                "   Total: " + std::to_string(totalScore) +
                "   Down: " + std::to_string(knocked) + "/10" +
                "   Bumpers: " + bumperText
            );
        }

        // Draw
        window.clear(sf::Color(20, 20, 20));

        window.draw(topEnd);
        window.draw(bottomEnd);

        window.draw(lane);
        window.draw(leftGutter);
        window.draw(rightGutter);

        if (bumpersOn) {
            window.draw(leftBumper);
            window.draw(rightBumper);
        }

        for (const auto& pin : pins)
            pin.draw(window);

        // Aim line only when not rolling
        if (!rollLocked) {
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

        if (font.getInfo().family != "")
            window.draw(hud);

        window.display();
    }

    return 0;
}
