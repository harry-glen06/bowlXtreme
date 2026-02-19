#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

enum class PinType {
    Normal,
    Gold,           // Knocked = +1 token
    Mischievous,    // Value randomises 1-15 each shot
    Exploding,      // Knocked over -> explodes after 1s, knocks nearby pins
    Light,          // Worth 2, very easy to knock over (low mass)
    Big,            // Worth 7, hard to knock over (high mass)
    Ice,            // Slides 15% faster when fallen
    CopyCat,        // Becomes the type of the first ball-hit pin this shot
    LuckyDucky,     // Worth 20, 35% chance to score 0
    ThirdTime,      // Every 3rd time scored, combo x2 (counter persists whole run)
};

class Pin {
private:
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Vector2f startPos;

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
    int baseValue = 0;      // value before Mischievous randomisation

    PinType pinType = PinType::Normal;

    // Exploding pin
    bool  explodeArmed  = false;   // true once fallen
    float explodeTimer  = 0.f;
    bool  hasExploded   = false;

    // ThirdTime pin
    int timesScored = 0;           // persists whole run

    // LuckyDucky
    bool luckyZero = false;        // set at shot-start if this shot scores 0
    float slideMultiplier = 1.0f;

public:
    Pin(sf::Vector2f startPosition, float radius, int value = 0);

    void update(float dt);
    void draw(sf::RenderWindow& window) const;
    void resetToOriginalPosition();
    void resetToOriginalPositionKeepType();

    // Type
    PinType getPinType() const { return pinType; }
    void    setPinType(PinType t);

    // Accessors
    sf::Vector2f getPos() const;
    sf::Vector2f getVel() const;
    float getRadius() const;
    float getMass()   const;
    float getRestitution() const;
    bool  isActive()  const;

    void setPos(sf::Vector2f p);
    void setVel(sf::Vector2f v);
    void setActive(bool a);
    void setSlideMultiplier(float mult);

    bool  isFallen()  const;
    void  setFallen(bool f);
    float getAngle()  const;
    float getAngularVel() const;
    void  setAngularVel(float w);

    int  getValue() const { return value; }
    void setValue(int v)  { value = v; }

    // Exploding
    bool  shouldExplode()  const { return explodeArmed && !hasExploded && explodeTimer >= 1.f; }
    bool  hasExplodedAlready() const { return hasExploded; }
    void  markExploded()         { hasExploded = true; explodeArmed = false; }
    float getExplodeTimer() const { return explodeTimer; }

    // ThirdTime
    int  getTimesScored() const { return timesScored; }
    void incrementTimesScored() { timesScored++; }

    // LuckyDucky
    bool isLuckyZero() const { return luckyZero; }
    void rollLuckyDucky();   // call at shot start to set luckyZero

    // Mischievous — randomise value, called at start of each shot
    void randomiseMischievous();
};
