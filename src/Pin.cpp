#include "Pin.h"
#include <cmath>

static float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Pin::Pin(sf::Vector2f startPos, float r) {
    pos = startPos;
    vel = sf::Vector2f(0.0f, 0.0f);

    radius = r;

    mass = 6.5f;
    restitution = 0.35f;

    // Standing vs fallen behaviour
    fallen = false;
    frictionStrength = 8.0f; // standing is sticky by default

    angle = 0.0f;
    angularVel = 0.0f;

    active = true;
}

void Pin::update(float dt) {
    if (!active) return;
    // Move
    pos += vel * dt;

    // Different friction for standing vs fallen
    float fric = fallen ? 17.0f : 28.0f;

    float speed = length(vel);
    if (speed > 0.0f) {
        sf::Vector2f dir = vel / speed;

        float drop = fric * dt;
        float newSpeed = speed - drop;
        if (newSpeed < 0.0f) newSpeed = 0.0f;

        vel = dir * newSpeed;
    }

    // Standing pins should "stick" and stop quickly
    if (!fallen && speed < 40.0f) {
        vel = sf::Vector2f(0.0f, 0.0f);
    }

    // Fallen pins can rotate
    if (fallen) {
        angle += angularVel * dt;

        // angular damping
        angularVel *= std::pow(0.2f, dt);

        if (std::abs(angularVel) < 0.05f)
            angularVel = 0.0f;
    } else {
        // keep upright
        angle = 0.0f;
        angularVel = 0.0f;
    }

    // Tiny velocity cutoff
    if (std::abs(vel.x) < 2.0f && std::abs(vel.y) < 2.0f) {
        vel = sf::Vector2f(0.0f, 0.0f);
    }

    // Standing pins should stop quickly (feel heavy)
    if (!fallen && speed < 120.0f) {
        vel = sf::Vector2f(0.0f, 0.0f);
    }
}

void Pin::draw(sf::RenderWindow& window) const {
    if (!active) return;

    // Shadow
    float shadowR = radius * 0.9f;
    sf::CircleShape shadow(shadowR);
    shadow.setOrigin(sf::Vector2f(shadowR, shadowR));
    shadow.setPosition(sf::Vector2f(pos.x + radius * 0.15f, pos.y + radius * 0.18f));
    shadow.setFillColor(sf::Color(0, 0, 0, 70));
    window.draw(shadow);

    // Angle handling
    float drawAngle = 0.0f;
    if (fallen) {
        drawAngle = angle;
        if (std::abs(drawAngle) < 10.0f) drawAngle = 75.0f;
    }

    // One transform for the whole pin
    sf::Transform t;
    t.translate(pos);
    t.rotate(sf::degrees(drawAngle));

    sf::RenderStates st;
    st.transform = t;

    // Shape sizing
    float H = radius * 4.2f;   // total height
    float maxW = radius * 2.1f; // belly width

    // --- Pin silhouette (convex shape) ---
    // We build half-widths at different heights and mirror them.
    // Local space: y goes up negative, down positive.
    struct Slice { float y; float halfW; };

    // More flared base + classic belly + neck
    Slice slices[] = {
        { -H * 0.52f, maxW * 0.30f }, // top cap
        { -H * 0.45f, maxW * 0.34f }, // neck
        { -H * 0.35f, maxW * 0.55f }, // shoulder
        { -H * 0.18f, maxW * 0.72f }, // upper belly
        {  0.00f,     maxW * 0.78f }, // widest belly
        {  H * 0.20f, maxW * 0.66f }, // lower belly
        {  H * 0.36f, maxW * 0.62f }, // taper to base
        {  H * 0.48f, maxW * 0.78f }, // flared base (wider)
        {  H * 0.54f, maxW * 0.86f }, // flare lip
        {  H * 0.58f, maxW * 0.80f }  // bottom edge
    };

    const int n = (int)(sizeof(slices) / sizeof(slices[0]));
    sf::ConvexShape body;
    body.setPointCount((size_t)(n * 2));

    // Left side top->bottom
    for (int i = 0; i < n; i++) {
        body.setPoint((size_t)i, sf::Vector2f(-slices[i].halfW, slices[i].y));
    }
    // Right side bottom->top
    for (int i = 0; i < n; i++) {
        int src = n - 1 - i;
        body.setPoint((size_t)(n + i), sf::Vector2f(slices[src].halfW, slices[src].y));
    }

    body.setFillColor(sf::Color(245, 245, 245));
    window.draw(body, st);

    // Subtle highlight to make it feel glossy
    sf::ConvexShape highlight = body;
    highlight.setFillColor(sf::Color(255, 255, 255, 45));
    // Nudge highlight slightly left/up in local space by editing points
    for (size_t i = 0; i < highlight.getPointCount(); i++) {
        sf::Vector2f p = highlight.getPoint(i);
        p.x -= radius * 0.10f;
        p.y -= radius * 0.18f;
        highlight.setPoint(i, p);
    }
    window.draw(highlight, st);

    // --- Two red stripes ---
    auto drawStripe = [&](float yLocal, float thickness) {
        sf::RectangleShape stripe(sf::Vector2f(maxW * 1.10f, thickness));
        stripe.setOrigin(sf::Vector2f(stripe.getSize().x * 0.5f, stripe.getSize().y * 0.5f));
        stripe.setPosition(sf::Vector2f(0.0f, yLocal));
        stripe.setFillColor(sf::Color(190, 30, 30));
        window.draw(stripe, st);
    };

    float stripeThick = radius * 0.30f;
    drawStripe(-H * 0.28f, stripeThick); // top stripe
    drawStripe(-H * 0.20f, stripeThick); // bottom stripe

    // Small base ring (tiny yellow-ish like real pins)
    sf::RectangleShape baseRing(sf::Vector2f(maxW * 0.9f, radius * 0.12f));
    baseRing.setOrigin(sf::Vector2f(baseRing.getSize().x * 0.5f, baseRing.getSize().y * 0.5f));
    baseRing.setPosition(sf::Vector2f(0.0f, H * 0.56f));
    baseRing.setFillColor(sf::Color(200, 185, 120));
    window.draw(baseRing, st);
}

// Getters
sf::Vector2f Pin::getPos() const { return pos; }
sf::Vector2f Pin::getVel() const { return vel; }
float Pin::getRadius() const { return radius; }
float Pin::getMass() const { return mass; }
float Pin::getRestitution() const { return restitution; }
bool Pin::isActive() const { return active; }

bool Pin::isFallen() const { return fallen; }
float Pin::getAngle() const { return angle; }
float Pin::getAngularVel() const { return angularVel; }

// Setters
void Pin::setPos(sf::Vector2f p) { pos = p; }
void Pin::setVel(sf::Vector2f v) { vel = v; }
void Pin::setActive(bool a) { active = a; }

void Pin::setFallen(bool f) {
    fallen = f;
    if (!fallen) {
        angle = 0.0f;
        angularVel = 0.0f;
    }
}

void Pin::setAngularVel(float w) { angularVel = w; }
