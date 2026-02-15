// Ball.cpp
#include "Ball.h"
#include <cmath>

static float vecLength(const sf::Vector2f& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

static sf::Vector2f vecNormalize(const sf::Vector2f& v) {
    float len = vecLength(v);
    if (len <= 0.00001f) return sf::Vector2f(0.0f, 0.0f);
    return sf::Vector2f(v.x / len, v.y / len);
}

Ball::Ball(float radius)
    : pos(0.0f, 0.0f),
      vel(0.0f, 0.0f),
      radius(radius),
      frictionStrength(1.2f),
      spin(0.0f) {}

void Ball::reset(sf::Vector2f startPos) {
    pos = startPos;
    vel = sf::Vector2f(0.0f, 0.0f);
    spin = 0.0f;
}

void Ball::launch(sf::Vector2f direction, float speed) {
    sf::Vector2f dir = vecNormalize(direction);
    vel = dir * speed;
}

void Ball::update(float dt) {
    if (dt <= 0.0f) return;

    // Move
    pos += vel * dt;

    // Friction (smooth slowdown, frame rate independent)
    float speed = getSpeed();
    if (speed <= 0.0f) return;

    float newSpeed = speed * std::exp(-frictionStrength * dt);

    // Stop threshold
    if (newSpeed < 0.8f) {
        stop();
        return;
    }

    vel = vecNormalize(vel) * newSpeed;
}

void Ball::stop() {
    vel = sf::Vector2f(0.0f, 0.0f);
}

sf::Vector2f Ball::getPos() const {
    return pos;
}

sf::Vector2f Ball::getVel() const {
    return vel;
}

float Ball::getRadius() const {
    return radius;
}

float Ball::getSpeed() const {
    return vecLength(vel);
}

void Ball::setPos(sf::Vector2f p) {
    pos = p;
}

void Ball::setVel(sf::Vector2f v) {
    vel = v;
}

void Ball::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(radius);
    shape.setOrigin(sf::Vector2f(radius, radius));
    shape.setPosition(pos);
    shape.setFillColor(sf::Color::Blue);
    window.draw(shape);
}
