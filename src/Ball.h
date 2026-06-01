#pragma once
#include "raylib.h"
#include "Items.h"

class Ball {
private:
    Vector2 pos;
    Vector2 vel;

    float baseRadius;
    float radius;
    float mass;
    float frictionStrength;
    float slideMultiplier = 1.0f;

    float spin;

    Color ballColor;
    BallType ballType = BallType::Normal;

public:
    Ball(float radius);

    void reset(Vector2 startPos);
    void launch(Vector2 direction, float speed);
    void update(float dt);
    void stop();

    Vector2 getPos() const;
    Vector2 getVel() const;
    float getRadius() const;
    float getMass() const;
    float getSpeed() const;

    void setPos(Vector2 p);
    void setVel(Vector2 v);
    void setColor(Color color);
    void setBallType(BallType type);
    void setSlideMultiplier(float mult);

    void applyItemMultipliers(float radiusMult, float massMult);

    void draw() const;

private:
    void drawNormal() const;
    void drawBlackHole() const;
    void drawMidas() const;
    void drawUpgrade() const;
    void drawHeavy() const;
    void drawFastball() const;
    void drawOddBall() const;
    void drawEightBall() const;
    void drawRetrigger() const;
};
