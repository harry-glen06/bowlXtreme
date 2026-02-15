#include "Physics.h"
#include <cmath>

float degToRad(float deg) {
    return deg * 3.1415926535f / 180.0f;
}

float dot(sf::Vector2f a, sf::Vector2f b) {
    return a.x * b.x + a.y * b.y;
}

float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

void resolveCircleCollision(
    sf::Vector2f& p1, sf::Vector2f& v1, float m1, float r1,
    sf::Vector2f& p2, sf::Vector2f& v2, float m2, float r2,
    float restitution
) {
    sf::Vector2f delta = p2 - p1;
    float dist = length(delta);
    float minDist = r1 + r2;

    if (dist <= 0.0001f) return;
    if (dist >= minDist) return;

    sf::Vector2f n = delta / dist;

    float penetration = minDist - dist;
    float totalMass = m1 + m2;
    float share1 = (m2 / totalMass);
    float share2 = (m1 / totalMass);

    p1 -= n * (penetration * share1);
    p2 += n * (penetration * share2);

    sf::Vector2f rv = v2 - v1;
    float velAlongNormal = dot(rv, n);

    if (velAlongNormal > 0.0f) return;

    float invM1 = (m1 <= 0.0f) ? 0.0f : (1.0f / m1);
    float invM2 = (m2 <= 0.0f) ? 0.0f : (1.0f / m2);

    float j = -(1.0f + restitution) * velAlongNormal;
    j /= (invM1 + invM2);

    sf::Vector2f impulse = j * n;

    v1 -= impulse * invM1;
    v2 += impulse * invM2;
}
