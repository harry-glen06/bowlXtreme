#pragma once
#include <SFML/Graphics.hpp>

float dot(sf::Vector2f a, sf::Vector2f b);
float length(sf::Vector2f v);
float degToRad(float deg);

// Impulse collision for two moving circles
void resolveCircleCollision(
    sf::Vector2f& p1, sf::Vector2f& v1, float m1, float r1,
    sf::Vector2f& p2, sf::Vector2f& v2, float m2, float r2,
    float restitution
);
