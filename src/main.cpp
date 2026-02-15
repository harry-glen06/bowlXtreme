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
static float dot(sf::Vector2f a, sf::Vector2f b) {
    return a.x * b.x + a.y * b.y;
}

static float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

static sf::Vector2f normalize(sf::Vector2f v) {
    float len = length(v);
    if (len == 0.0f) return sf::Vector2f(0.0f, 0.0f);
    return v / len;
}

// Impulse collision for two moving circles
static void resolveCircleCollision(
    sf::Vector2f& p1, sf::Vector2f& v1, float m1, float r1,
    sf::Vector2f& p2, sf::Vector2f& v2, float m2, float r2,
    float restitution
) {
    sf::Vector2f delta = p2 - p1;
    float dist = length(delta);
    float minDist = r1 + r2;

    if (dist <= 0.0001f) return;
    if (dist >= minDist) return;

    sf::Vector2f n = delta / dist;

    // Push them apart so they don't overlap
    float penetration = minDist - dist;
    float totalMass = m1 + m2;
    float share1 = (m2 / totalMass);
    float share2 = (m1 / totalMass);

    p1 -= n * (penetration * share1);
    p2 += n * (penetration * share2);

    // Relative velocity along normal
    sf::Vector2f rv = v2 - v1;
    float velAlongNormal = dot(rv, n);

    // If separating, do nothing
    if (velAlongNormal > 0.0f) return;

    float e = restitution;

    float invM1 = (m1 <= 0.0f) ? 0.0f : (1.0f / m1);
    float invM2 = (m2 <= 0.0f) ? 0.0f : (1.0f / m2);

    float j = -(1.0f + e) * velAlongNormal;
    j /= (invM1 + invM2);

    sf::Vector2f impulse = j * n;

    v1 -= impulse * invM1;
    v2 += impulse * invM2;
}


// Create 10 pins in bowling triangle (1 at front, 4 at back)
std::vector<Pin> createPins(float centerX, float startY) {
    std::vector<Pin> pins;
    float spacing = 35.0f;   // keep
    float radius = 12.0f;    // keep

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
    // Score (simple)
    int totalScore = 0;

    // Frame/shot
    int frame = 1;
    int shot = 1;

    const float windowW = 900.0f;
    const float windowH = 600.0f;

    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u((unsigned)windowW, (unsigned)windowH)),
        "Bowling Prototype"
    );
    window.setFramerateLimit(60);

    // Lane numbers (keep your lane size)
    float laneWidth = 250.0f;
    float laneTop = 40.0f;
    float laneHeight = 520.0f;

    float laneLeft = (windowW - laneWidth) / 2.0f;
    float laneRight = laneLeft + laneWidth;
    float laneBottom = laneTop + laneHeight;

    // Gutters and bumpers
    float gutterWidth = 28.0f;
    float bumperThickness = 6.0f;
    bool bumpersOn = false;

    // Bumper + gutter behavior
    float bumperBounce = 0.75f;      // bounce strength
    float gutterSlideSpeed = 420.0f; // slide speed up lane

    bool inGutter = false;
    int gutterSide = 0; // -1 left, +1 right, 0 none

    // Lane visuals
    sf::RectangleShape lane(sf::Vector2f(laneWidth, laneHeight));
    lane.setPosition(sf::Vector2f(laneLeft, laneTop));
    lane.setFillColor(sf::Color(160, 120, 70));

    sf::RectangleShape leftGutter(sf::Vector2f(gutterWidth, laneHeight));
    leftGutter.setPosition(sf::Vector2f(laneLeft, laneTop));
    leftGutter.setFillColor(sf::Color(35, 35, 35));

    sf::RectangleShape rightGutter(sf::Vector2f(gutterWidth, laneHeight));
    rightGutter.setPosition(sf::Vector2f(laneRight - gutterWidth, laneTop));
    rightGutter.setFillColor(sf::Color(35, 35, 35));

    sf::RectangleShape leftBumper(sf::Vector2f(bumperThickness, laneHeight));
    leftBumper.setPosition(sf::Vector2f(laneLeft + gutterWidth, laneTop));
    leftBumper.setFillColor(sf::Color::White);

    sf::RectangleShape rightBumper(sf::Vector2f(bumperThickness, laneHeight));
    rightBumper.setPosition(sf::Vector2f(laneRight - gutterWidth - bumperThickness, laneTop));
    rightBumper.setFillColor(sf::Color::White);

    float endZoneSize = 20.0f;

    sf::RectangleShape topEnd(sf::Vector2f(laneWidth, endZoneSize));
    topEnd.setPosition(sf::Vector2f(laneLeft, laneTop - endZoneSize));
    topEnd.setFillColor(sf::Color::Black);

    sf::RectangleShape bottomEnd(sf::Vector2f(laneWidth, endZoneSize));
    bottomEnd.setPosition(sf::Vector2f(laneLeft, laneBottom));
    bottomEnd.setFillColor(sf::Color::Black);

    // Playable area between bumpers (inside the lane)
    float playLeft = laneLeft + gutterWidth + bumperThickness;
    float playRight = laneRight - gutterWidth - bumperThickness;

    // Ball (keep)
    Ball ball(21.0f);
    sf::Vector2f ballStart(windowW / 2.0f, laneBottom - 30.0f);
    ball.reset(ballStart);

    // Pins (keep startY)
    std::vector<Pin> pins = createPins(laneLeft + laneWidth / 2.0f, 220.0f);

    // Movement and aim (keep)
    float moveSpeed = 400.0f;
    float aimDeg = -90.0f;
    float aimTurnSpeed = 140.0f;

    // Roll lock
    bool rollLocked = false;
    sf::Vector2f rollDir(0.0f, -1.0f);
    float minRollSpeed = 250.0f;

    // Reset helpers (must be inside main)
    auto resetPins = [&]() {
        pins = createPins(laneLeft + laneWidth / 2.0f, 220.0f);
    };

    auto resetBall = [&]() {
        ballStart = sf::Vector2f(windowW / 2.0f, laneBottom - 30.0f);
        ball.reset(ballStart);

        rollLocked = false;
        rollDir = sf::Vector2f(0.0f, -1.0f);
        aimDeg = -90.0f;

        inGutter = false;
        gutterSide = 0;
    };

    // Scoreboard (SFML 3)
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

            // clamp inside playable area (so you can't start in gutter)
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

        // R resets everything
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
            totalScore = 0;
            frame = 1;
            shot = 1;
            resetPins();
            resetBall();
        }

        ball.update(dt);
        for (auto& pin : pins) {
            pin.update(dt);
        }


        // Keep rolling until end
        if (rollLocked && ball.getSpeed() < minRollSpeed) {
            ball.setVel(rollDir * minRollSpeed);
        }

        // Bumper bounce / gutter slide logic
        {
            sf::Vector2f p = ball.getPos();
            sf::Vector2f v = ball.getVel();
            float r = ball.getRadius();

            // If already in gutter, force it to slide forward to the end
            if (inGutter) {
                if (gutterSide == -1) {
                    p.x = laneLeft + gutterWidth * 0.5f;
                } else if (gutterSide == 1) {
                    p.x = laneRight - gutterWidth * 0.5f;
                }

                v.x = 0.0f;
                v.y = -gutterSlideSpeed;

                ball.setPos(p);
                ball.setVel(v);
            } else {
                // Not in gutter yet
                if (bumpersOn) {
                    float minSide = 80.0f;   // strength of sideways push after bounce
                    // Bounce off bumpers
                    // Left bumper: only bounce if moving left into it
                    if (p.x < playLeft + r) {
                        p.x = playLeft + r;
                        if (v.x < 0.0f) v.x = -v.x * bumperBounce;
                            if (std::abs(v.x) < minSide){
                                v.x = minSide;
                        }
                    }

                    // Right bumper: only bounce if moving right into it
                    if (p.x > playRight - r) {
                        p.x = playRight - r;
                        if (v.x > 0.0f) v.x = -v.x * bumperBounce;
                            if (std::abs(v.x) < minSide){
                                v.x = minSide;
                            }
                    }
                } else {
                    // Enter gutter -> lock gutter slide mode
                    if (p.x < playLeft + r) {
                        inGutter = true;
                        gutterSide = -1;
                    } else if (p.x > playRight - r) {
                        inGutter = true;
                        gutterSide = 1;
                    }
                }

                // bottom clamp
                if (p.y > laneBottom - r) {
                    p.y = laneBottom - r;
                    if (v.y > 0.0f) v.y = 0.0f;
                }

                ball.setPos(p);
                ball.setVel(v);
            }

            // Back of lane reached: end shot, update shot/frame, reset pins+ball
            p = ball.getPos();
            if (p.y < laneTop + r) {
                ball.stop();
                rollLocked = false;

                shot++;
                if (shot == 3) {
                    frame++;
                    shot = 1;
                }

                resetPins();
                resetBall();
            }
        }


        // Ball -> pin impulse collisions
        {
            sf::Vector2f bp = ball.getPos();
            sf::Vector2f bv = ball.getVel();
            float br = ball.getRadius();
            float bm = 4.0f; // ball is heavier

            for (auto& pin : pins) {
                sf::Vector2f pp = pin.getPos();
                sf::Vector2f pv = pin.getVel();
                float pr = pin.getRadius();
                float pm = pin.getMass();

                float restitution = 0.55f; // ball vs pin

                resolveCircleCollision(
                    bp, bv, bm, br,
                    pp, pv, pm, pr,
                    restitution
                );

                pin.setPos(pp);
                pin.setVel(pv);
            }

            ball.setPos(bp);
            ball.setVel(bv);
        }

        // Pin -> pin impulse collisions (pairs)
        {
            float restitution = 0.35f; // pin vs pin

            for (size_t i = 0; i < pins.size(); i++) {
                for (size_t j = i + 1; j < pins.size(); j++) {
                    sf::Vector2f p1 = pins[i].getPos();
                    sf::Vector2f v1 = pins[i].getVel();
                    float m1 = pins[i].getMass();
                    float r1 = pins[i].getRadius();

                    sf::Vector2f p2 = pins[j].getPos();
                    sf::Vector2f v2 = pins[j].getVel();
                    float m2 = pins[j].getMass();
                    float r2 = pins[j].getRadius();

                    resolveCircleCollision(
                        p1, v1, m1, r1,
                        p2, v2, m2, r2,
                        restitution
                    );

                    pins[i].setPos(p1);
                    pins[i].setVel(v1);

                    pins[j].setPos(p2);
                    pins[j].setVel(v2);
                }
            }
        }

        // HUD
        if (fontOk) {
            hud.setString(
                "Frame: " + std::to_string(frame) +
                "   Shot: " + std::to_string(shot) +
                "   Score: " + std::to_string(totalScore) +
                "   Bumpers: " + std::string(bumpersOn ? "ON" : "OFF")
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

        if (fontOk) window.draw(hud);

        window.display();
    }

    return 0;
}