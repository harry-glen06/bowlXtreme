#pragma once
#include <SFML/Graphics.hpp>

class Pin {
private:
    sf::Vector2f pos;
    sf::Vector2f vel;

    float radius;
    float mass;

    float frictionStrength;
    float restitution;

    bool active;

public:
    Pin(sf::Vector2f startPos, float r);

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    sf::Vector2f getPos() const;
    sf::Vector2f getVel() const;
    float getRadius() const;
    float getMass() const;
    float getRestitution() const;
    bool isActive() const;

    void setPos(sf::Vector2f p);
    void setVel(sf::Vector2f v);
    void setActive(bool a);
};
