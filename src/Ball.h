#pragma once
#include <SFML/Graphics.hpp>

class Ball {
private:
    sf::Vector2f pos;
    sf::Vector2f vel;

    float radius;
    float frictionStrength;   // how fast it slows down

    float spin;               // default 0, used later
    
    sf::Color ballColor;      // NEW: Store ball color

public:
    Ball(float radius);

    void reset(sf::Vector2f startPos);
    void launch(sf::Vector2f direction, float speed);
    void update(float dt);
    void stop();

    sf::Vector2f getPos() const;
    sf::Vector2f getVel() const;
    float getRadius() const;
    float getSpeed() const;

    void setPos(sf::Vector2f p);
    void setVel(sf::Vector2f v);
    void setColor(sf::Color color);  // NEW: Set ball color

    void draw(sf::RenderWindow& window) const;
};