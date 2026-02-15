#include "Pin.h"
#include <cmath>

static float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Pin::Pin(sf::Vector2f startPos, float r) {
    pos = startPos;
    vel = sf::Vector2f(0.0f, 0.0f);

    radius = r;

    mass = 6.5f;
    restitution = 0.35f;

    // Standing vs fallen behaviour
    fallen = false;
    frictionStrength = 8.0f; // standing is sticky by default

    angle = 0.0f;
    angularVel = 0.0f;

    active = true;
}

void Pin::update(float dt) {
    if (!active) return;
    // Move
    pos += vel * dt;

    // Different friction for standing vs fallen
    float fric = fallen ? 17.0f : 28.0f;

    float speed = length(vel);
    if (speed > 0.0f) {
        sf::Vector2f dir = vel / speed;

        float drop = fric * dt;
        float newSpeed = speed - drop;
        if (newSpeed < 0.0f) newSpeed = 0.0f;

        vel = dir * newSpeed;
    }

    // Standing pins should "stick" and stop quickly
    if (!fallen && speed < 40.0f) {
        vel = sf::Vector2f(0.0f, 0.0f);
    }

    // Fallen pins can rotate
    if (fallen) {
        angle += angularVel * dt;

        // angular damping
        angularVel *= std::pow(0.2f, dt);

        if (std::abs(angularVel) < 0.05f)
            angularVel = 0.0f;
    } else {
        // keep upright
        angle = 0.0f;
        angularVel = 0.0f;
    }

    // Tiny velocity cutoff
    if (std::abs(vel.x) < 2.0f && std::abs(vel.y) < 2.0f) {
        vel = sf::Vector2f(0.0f, 0.0f);
    }

    // Standing pins should stop quickly (feel heavy)
    if (!fallen && speed < 120.0f) {
        vel = sf::Vector2f(0.0f, 0.0f);
    }
}

void Pin::draw(sf::RenderWindow& window) const {
    if (!active) return;
    // Draw pin as a simple capsule (rectangle + 2 circles), rotated by angle
    // If standing: draw tall. If fallen: draw sideways (90 degrees)
    float bodyW = radius * 1.2f;
    float bodyH = radius * 3.2f;

    float drawAngle = angle;
    if (fallen) {
        drawAngle += 90.0f; // sideways look
    }

    // Body
    sf::RectangleShape body(sf::Vector2f(bodyW, bodyH));
    body.setOrigin(sf::Vector2f(bodyW * 0.5f, bodyH * 0.5f));
    body.setPosition(pos);
    body.setRotation(sf::degrees(drawAngle));
    body.setFillColor(sf::Color::White);

    // End caps
    sf::CircleShape cap(radius * 0.6f);
    cap.setOrigin(sf::Vector2f(radius * 0.6f, radius * 0.6f));
    cap.setFillColor(sf::Color::White);

    // Place caps along the long axis (up/down before rotation)
    sf::Transform t;
    t.translate(pos);
    t.rotate(sf::degrees(drawAngle));

    sf::Vector2f topLocal(0.0f, -bodyH * 0.5f);
    sf::Vector2f botLocal(0.0f,  bodyH * 0.5f);

    sf::Vector2f top = t.transformPoint(topLocal);
    sf::Vector2f bot = t.transformPoint(botLocal);

    sf::CircleShape topCap = cap;
    topCap.setPosition(top);

    sf::CircleShape botCap = cap;
    botCap.setPosition(bot);

    // Add a tiny red stripe so it looks like a bowling pin
    sf::RectangleShape stripe(sf::Vector2f(bodyW, radius * 0.35f));
    stripe.setOrigin(sf::Vector2f(bodyW * 0.5f, (radius * 0.35f) * 0.5f));
    stripe.setPosition(pos);
    stripe.setRotation(sf::degrees(drawAngle));
    stripe.setFillColor(sf::Color(200, 40, 40));

    window.draw(body);
    window.draw(topCap);
    window.draw(botCap);
    window.draw(stripe);
}

// Getters
sf::Vector2f Pin::getPos() const { return pos; }
sf::Vector2f Pin::getVel() const { return vel; }
float Pin::getRadius() const { return radius; }
float Pin::getMass() const { return mass; }
float Pin::getRestitution() const { return restitution; }
bool Pin::isActive() const { return active; }

bool Pin::isFallen() const { return fallen; }
float Pin::getAngle() const { return angle; }
float Pin::getAngularVel() const { return angularVel; }

// Setters
void Pin::setPos(sf::Vector2f p) { pos = p; }
void Pin::setVel(sf::Vector2f v) { vel = v; }
void Pin::setActive(bool a) { active = a; }

void Pin::setFallen(bool f) {
    fallen = f;
    if (!fallen) {
        angle = 0.0f;
        angularVel = 0.0f;
    }
}

void Pin::setAngularVel(float w) { angularVel = w; }
