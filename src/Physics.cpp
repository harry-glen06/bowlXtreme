#include "Physics.h"
#include <cmath>
#include <cstdlib>

float degToRad(float deg) {
    return deg * 3.1415926535f / 180.0f;
}

float dot(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

float length(Vector2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

void resolveCircleCollision(
    Vector2& p1, Vector2& v1, float m1, float r1,
    Vector2& p2, Vector2& v2, float m2, float r2,
    float restitution
) {
    Vector2 delta = {p2.x - p1.x, p2.y - p1.y};
    float dist = length(delta);
    float minDist = r1 + r2;

    if (dist >= minDist) return;
    Vector2 n;
    if (dist <= 0.0001f) {
        float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 6.283185307f;
        n = {std::cos(angle), std::sin(angle)};
        dist = 0.0001f;
    } else {
        n = {delta.x / dist, delta.y / dist};
    }

    float penetration = minDist - dist;
    float totalMass = m1 + m2;
    float share1 = (m2 / totalMass);
    float share2 = (m1 / totalMass);

    p1.x -= n.x * (penetration * share1);
    p1.y -= n.y * (penetration * share1);
    p2.x += n.x * (penetration * share2);
    p2.y += n.y * (penetration * share2);

    Vector2 rv = {v2.x - v1.x, v2.y - v1.y};
    float velAlongNormal = dot(rv, n);

    if (velAlongNormal > 0.0f) return;

    float invM1 = (m1 <= 0.0f) ? 0.0f : (1.0f / m1);
    float invM2 = (m2 <= 0.0f) ? 0.0f : (1.0f / m2);

    float j = -(1.0f + restitution) * velAlongNormal;
    j /= (invM1 + invM2);

    Vector2 impulse = {j * n.x, j * n.y};

    v1.x -= impulse.x * invM1;
    v1.y -= impulse.y * invM1;
    v2.x += impulse.x * invM2;
    v2.y += impulse.y * invM2;
}

void applyOilEffect(Vector2& velocity, int pinsStanding) {
    if (pinsStanding <= 1) return;
    float baseOilDrift = 1.0f + (rand() % 100) / 100.0f;
    float oilMultiplier = pinsStanding / 10.0f;
    float direction = (rand() % 2 == 0) ? 1.0f : -1.0f;
    velocity.x += baseOilDrift * oilMultiplier * direction;
}
