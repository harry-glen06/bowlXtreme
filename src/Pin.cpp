#include "Pin.h"
#include <cmath>

// Constructor
Pin::Pin(sf::Vector2f startPos, float r) {
    pos = startPos;
    vel = sf::Vector2f(0.0f, 0.0f);

    radius = r;

    mass = 1.0f;               // pins are light
    frictionStrength = 2.2f;   // slows naturally
    restitution = 0.35f;       // small bounce

    active = true;
}

// Update physics
void Pin::update(float dt) {
    // Move
    pos += vel * dt;

    // Apply friction (natural slowing)
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);

    if (speed > 0.0f) {
        sf::Vector2f frictionDir = vel / speed;

        float drop = frictionStrength * dt;
        float newSpeed = speed - drop;

        if (newSpeed < 0.0f)
            newSpeed = 0.0f;

        vel = frictionDir * newSpeed;
    }

    // If almost stopped, fully stop
    if (std::abs(vel.x) < 2.0f && std::abs(vel.y) < 2.0f) {
        vel = sf::Vector2f(0.0f, 0.0f);
    }
}

// Draw pin
void Pin::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(radius);
    shape.setOrigin(sf::Vector2f(radius, radius));
    shape.setPosition(pos);

    // White pin look
    shape.setFillColor(sf::Color::White);

    window.draw(shape);
}

// Getters
sf::Vector2f Pin::getPos() const { return pos; }
sf::Vector2f Pin::getVel() const { return vel; }
float Pin::getRadius() const { return radius; }
float Pin::getMass() const { return mass; }
float Pin::getRestitution() const { return restitution; }
bool Pin::isActive() const { return active; }

// Setters
void Pin::setPos(sf::Vector2f p) { pos = p; }
void Pin::setVel(sf::Vector2f v) { vel = v; }
void Pin::setActive(bool a) { active = a; }
