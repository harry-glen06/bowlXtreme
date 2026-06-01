#pragma once
#include "raylib.h"

float dot(Vector2 a, Vector2 b);
float length(Vector2 v);
float degToRad(float deg);

void resolveCircleCollision(
    Vector2& p1, Vector2& v1, float m1, float r1,
    Vector2& p2, Vector2& v2, float m2, float r2,
    float restitution
);

void applyOilEffect(Vector2& velocity, int pinsStanding);
