#pragma once
#include <SFML/Graphics.hpp>

struct Lane {
    float width = 250.0f;
    float top = 40.0f;
    float height = 900.0f;

    float gutterWidth = 28.0f;
    float bumperThickness = 6.0f;

    float left = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    bool bumpersOn = false;

    sf::RectangleShape laneRect;
    sf::RectangleShape leftGutter;
    sf::RectangleShape rightGutter;
    sf::RectangleShape leftBumper;
    sf::RectangleShape rightBumper;
    sf::RectangleShape topEnd;
    sf::RectangleShape bottomEnd;

    void init(float windowW);
    void draw(sf::RenderWindow& window) const;

    float playLeft() const;
    float playRight() const;
    float centerX() const;
};
