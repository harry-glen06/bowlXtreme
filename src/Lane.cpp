#include "Lane.h"

void Lane::init(float windowW) {
    left = (windowW - width) / 2.0f;
    right = left + width;
    bottom = top + height;

    laneRect = sf::RectangleShape(sf::Vector2f(width, height));
    laneRect.setPosition(sf::Vector2f(left, top));
    laneRect.setFillColor(sf::Color(160, 120, 70));

    leftGutter = sf::RectangleShape(sf::Vector2f(gutterWidth, height));
    leftGutter.setPosition(sf::Vector2f(left, top));
    leftGutter.setFillColor(sf::Color(35, 35, 35));

    rightGutter = sf::RectangleShape(sf::Vector2f(gutterWidth, height));
    rightGutter.setPosition(sf::Vector2f(right - gutterWidth, top));
    rightGutter.setFillColor(sf::Color(35, 35, 35));

    leftBumper = sf::RectangleShape(sf::Vector2f(bumperThickness, height));
    leftBumper.setPosition(sf::Vector2f(left + gutterWidth, top));
    leftBumper.setFillColor(sf::Color::White);

    rightBumper = sf::RectangleShape(sf::Vector2f(bumperThickness, height));
    rightBumper.setPosition(sf::Vector2f(right - gutterWidth - bumperThickness, top));
    rightBumper.setFillColor(sf::Color::White);

    float endZoneSize = 20.0f;

    topEnd = sf::RectangleShape(sf::Vector2f(width, endZoneSize));
    topEnd.setPosition(sf::Vector2f(left, top - endZoneSize));
    topEnd.setFillColor(sf::Color::Black);

    bottomEnd = sf::RectangleShape(sf::Vector2f(width, endZoneSize));
    bottomEnd.setPosition(sf::Vector2f(left, bottom));
    bottomEnd.setFillColor(sf::Color::Black);
}

void Lane::draw(sf::RenderWindow& window) const {
    window.draw(topEnd);
    window.draw(bottomEnd);

    window.draw(laneRect);
    window.draw(leftGutter);
    window.draw(rightGutter);

    if (bumpersOn) {
        window.draw(leftBumper);
        window.draw(rightBumper);
    }
}

float Lane::playLeft() const {
    return left + gutterWidth + bumperThickness;
}

float Lane::playRight() const {
    return right - gutterWidth - bumperThickness;
}

float Lane::centerX() const {
    return left + width * 0.5f;
}
