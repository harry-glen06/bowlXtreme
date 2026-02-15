#pragma once
#include <SFML/Graphics.hpp>

class Pin {
private:
    sf::Vector2f pos;
    float radius;
    bool knocked;

public:
    Pin(sf::Vector2f position, float radius);

    void knock();
    bool isKnocked() const;

    sf::Vector2f getPos() const;
    float getRadius() const;

    void draw(sf::RenderWindow& window) const;
};
