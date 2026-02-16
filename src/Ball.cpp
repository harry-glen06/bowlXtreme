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
      frictionStrength(1.05f),
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
    // Shadow (soft)
    float shadowR = radius * 1.10f;
    sf::CircleShape shadow(shadowR);
    shadow.setOrigin(sf::Vector2f(shadowR, shadowR));
    shadow.setPosition(sf::Vector2f(pos.x + radius * 0.18f, pos.y + radius * 0.22f));
    shadow.setFillColor(sf::Color(0, 0, 0, 70));
    window.draw(shadow);

    // Ball base
    sf::CircleShape ball(radius);
    ball.setOrigin(sf::Vector2f(radius, radius));
    ball.setPosition(pos);

    // Pick your ball colour here
    sf::Color base(25, 55, 140);
    ball.setFillColor(base);
    window.draw(ball);

    // Dark rim (gives depth)
    sf::CircleShape rim(radius * 0.98f);
    rim.setOrigin(sf::Vector2f(radius * 0.98f, radius * 0.98f));
    rim.setPosition(pos);
    rim.setFillColor(sf::Color::Transparent);
    rim.setOutlineThickness(radius * 0.10f);
    rim.setOutlineColor(sf::Color(0, 0, 0, 45));
    window.draw(rim);

    // Inner gradient-ish layers (fake lighting)
    sf::CircleShape layer1(radius * 0.92f);
    layer1.setOrigin(sf::Vector2f(radius * 0.92f, radius * 0.92f));
    layer1.setPosition(sf::Vector2f(pos.x - radius * 0.06f, pos.y - radius * 0.08f));
    layer1.setFillColor(sf::Color(base.r + 20, base.g + 20, base.b + 20, 90));
    window.draw(layer1);

    sf::CircleShape layer2(radius * 0.78f);
    layer2.setOrigin(sf::Vector2f(radius * 0.78f, radius * 0.78f));
    layer2.setPosition(sf::Vector2f(pos.x - radius * 0.12f, pos.y - radius * 0.16f));
    layer2.setFillColor(sf::Color(255, 255, 255, 35));
    window.draw(layer2);

    // Gloss highlight
    sf::CircleShape highlight(radius * 0.36f);
    highlight.setOrigin(sf::Vector2f(radius * 0.36f, radius * 0.36f));
    highlight.setPosition(sf::Vector2f(pos.x - radius * 0.34f, pos.y - radius * 0.38f));
    highlight.setFillColor(sf::Color(255, 255, 255, 65));
    window.draw(highlight);

    // Finger holes (3 holes in a triangle)
    // We rotate them slightly using spin, but if spin is 0 it still looks fine.
    float holeR = radius * 0.15f;
    sf::CircleShape hole(holeR);
    hole.setOrigin(sf::Vector2f(holeR, holeR));
    hole.setFillColor(sf::Color(0, 0, 0, 150));

    // Local offsets for holes
    sf::Vector2f h1(-radius * 0.18f, -radius * 0.10f);
    sf::Vector2f h2( radius * 0.18f, -radius * 0.10f);
    sf::Vector2f h3( 0.0f,            radius * 0.15f);

    // Rotate offsets by spin (spin is in your Ball class already)
    float a = spin; // small values look best (0 to maybe 2)
    float c = std::cos(a);
    float s = std::sin(a);

    auto rot = [&](sf::Vector2f v) {
        return sf::Vector2f(v.x * c - v.y * s, v.x * s + v.y * c);
    };

    hole.setPosition(pos + rot(h1));
    window.draw(hole);

    hole.setPosition(pos + rot(h2));
    window.draw(hole);

    hole.setPosition(pos + rot(h3));
    window.draw(hole);

    // Small specular dot
    sf::CircleShape dot(radius * 0.10f);
    dot.setOrigin(sf::Vector2f(radius * 0.10f, radius * 0.10f));
    dot.setPosition(sf::Vector2f(pos.x - radius * 0.20f, pos.y - radius * 0.22f));
    dot.setFillColor(sf::Color(255, 255, 255, 80));
    window.draw(dot);
}
