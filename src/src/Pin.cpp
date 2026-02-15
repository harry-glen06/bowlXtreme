#include "Pin.h"

Pin::Pin(sf::Vector2f position, float radius)
    : pos(position), radius(radius), knocked(false) {}

void Pin::knock() {
    knocked = true;
}

bool Pin::isKnocked() const {
    return knocked;
}

sf::Vector2f Pin::getPos() const {
    return pos;
}

float Pin::getRadius() const {
    return radius;
}

void Pin::draw(sf::RenderWindow& window) const {
    if (knocked) return;

    sf::CircleShape shape(radius);
    shape.setOrigin(sf::Vector2f(radius, radius)); // SFML 3 style
    shape.setPosition(pos);
    shape.setFillColor(sf::Color(245, 245, 240));
    window.draw(shape);
}
