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
    // Frame ends (black like your screenshot)
    window.draw(topEnd);
    window.draw(bottomEnd);

    // Dark outer border behind everything
    sf::RectangleShape border(sf::Vector2f(width + 24.0f, height + 40.0f));
    border.setPosition(sf::Vector2f(left - 12.0f, top - 20.0f));
    border.setFillColor(sf::Color(30, 30, 30));
    window.draw(border);

    // Lane body
    window.draw(laneRect);

    // Wood stripes (simple planks)
    for (int i = 0; i < 9; i++) {
        float stripeW = width / 9.0f;
        sf::RectangleShape plank(sf::Vector2f(stripeW, height));
        plank.setPosition(sf::Vector2f(left + i * stripeW, top));

        // alternate slightly different wood colors
        if (i % 2 == 0) plank.setFillColor(sf::Color(170, 130, 80));
        else            plank.setFillColor(sf::Color(160, 120, 70));

        window.draw(plank);
    }

    // Gutters
    window.draw(leftGutter);
    window.draw(rightGutter);

    // Bumpers (only if on)
    if (bumpersOn) {
        window.draw(leftBumper);
        window.draw(rightBumper);
    }

    // REMOVED: Pin deck zone (brown section at top)
    // sf::RectangleShape pinDeck(sf::Vector2f(width, 70.0f));
    // pinDeck.setPosition(sf::Vector2f(left, top));
    // pinDeck.setFillColor(sf::Color(190, 160, 110));
    // window.draw(pinDeck);


    // Thin inner border line (gives it that "lane frame" look)
    sf::RectangleShape inner(sf::Vector2f(width, height));
    inner.setPosition(sf::Vector2f(left, top));
    inner.setFillColor(sf::Color::Transparent);
    inner.setOutlineThickness(3.0f);
    inner.setOutlineColor(sf::Color(90, 90, 90));
    window.draw(inner);
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