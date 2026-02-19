#pragma once
#include <SFML/Graphics.hpp>
#include "Items.h"

class Ball {
private:
    sf::Vector2f pos;
    sf::Vector2f vel;

    float baseRadius;         // original radius (never changes)
    float radius;             // effective radius (modified by items)
    float mass;               // base mass, modified by items
    float frictionStrength;
    float slideMultiplier = 1.0f;

    float spin;

    sf::Color ballColor;
    BallType ballType = BallType::Normal;

public:
    Ball(float radius);

    void reset(sf::Vector2f startPos);
    void launch(sf::Vector2f direction, float speed);
    void update(float dt);
    void stop();

    sf::Vector2f getPos() const;
    sf::Vector2f getVel() const;
    float getRadius() const;
    float getMass() const;
    float getSpeed() const;

    void setPos(sf::Vector2f p);
    void setVel(sf::Vector2f v);
    void setColor(sf::Color color);
    void setBallType(BallType type);
    void setSlideMultiplier(float mult);

    // Apply item multipliers (called when equipping a ball)
    void applyItemMultipliers(float radiusMult, float massMult);

    void draw(sf::RenderWindow& window) const;

private:
    void drawNormal(sf::RenderWindow& window) const;
    void drawBlackHole(sf::RenderWindow& window) const;
    void drawMidas(sf::RenderWindow& window) const;
    void drawUpgrade(sf::RenderWindow& window) const;
    void drawHeavy(sf::RenderWindow& window) const;
    void drawFastball(sf::RenderWindow& window) const;
    void drawOddBall(sf::RenderWindow& window) const;
    void drawEightBall(sf::RenderWindow& window) const;
    void drawRetrigger(sf::RenderWindow& window) const;
};
