#pragma once
#include "raylib.h"

enum class PinType {
    Normal,
    Gold,
    Mischievous,
    Exploding,
    Light,
    Big,
    Ice,
    CopyCat,
    LuckyDucky,
    LevelUp,
    Lover,
    ChangeIsGood,
    ThirdTime,
};

class Pin {
private:
    Vector2 pos;
    Vector2 vel;
    Vector2 startPos;

    float radius;
    float baseMass;
    float mass;

    float frictionStrength;
    float restitution;

    float angle;
    float angularVel;
    bool fallen;
    bool active;

    int value = 0;
    int baseValue = 0;

    PinType pinType = PinType::Normal;

    bool  explodeArmed  = false;
    float explodeTimer  = 0.f;
    bool  hasExploded   = false;

    int timesScored = 0;

    bool luckyZero = false;
    float slideMultiplier = 1.0f;

public:
    Pin(Vector2 startPosition, float radius, int value = 0);

    void update(float dt);
    void draw() const;
    void resetToOriginalPosition();
    void resetToOriginalPositionKeepType();

    PinType getPinType() const { return pinType; }
    void    setPinType(PinType t);

    Vector2 getPos() const;
    Vector2 getVel() const;
    float getRadius() const;
    float getMass()   const;
    float getRestitution() const;
    bool  isActive()  const;

    void setPos(Vector2 p);
    void translate(Vector2 delta);
    void setVel(Vector2 v);
    void setActive(bool a);
    void setSlideMultiplier(float mult);

    bool  isFallen()  const;
    void  setFallen(bool f);
    float getAngle()  const;
    float getAngularVel() const;
    void  setAngularVel(float w);

    int  getValue() const { return value; }
    void setValue(int v)  { value = v; }

    bool  shouldExplode()     const { return explodeArmed && !hasExploded && explodeTimer >= 1.f; }
    bool  hasExplodedAlready()const { return hasExploded; }
    void  markExploded()            { hasExploded = true; explodeArmed = false; }
    float getExplodeTimer()   const { return explodeTimer; }

    int  getTimesScored()    const { return timesScored; }
    void incrementTimesScored()    { timesScored++; }

    bool isLuckyZero() const { return luckyZero; }
    void rollLuckyDucky();

    void randomiseMischievous();
};
