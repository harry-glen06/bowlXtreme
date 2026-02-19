#include "Ball.h"
#include <cmath>
#include <algorithm>

static float vecLength(const sf::Vector2f& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}
static sf::Vector2f vecNormalize(const sf::Vector2f& v) {
    float len = vecLength(v);
    if (len <= 0.00001f) return {0.f, 0.f};
    return {v.x / len, v.y / len};
}

Ball::Ball(float r)
    : pos(0.f, 0.f), vel(0.f, 0.f),
      baseRadius(r), radius(r), mass(4.0f),
      frictionStrength(1.05f), spin(0.f),
      ballColor(25, 55, 140) {}

void Ball::reset(sf::Vector2f startPos) {
    pos  = startPos;
    vel  = {0.f, 0.f};
    spin = 0.f;
}

void Ball::launch(sf::Vector2f direction, float speed) {
    vel = vecNormalize(direction) * speed;
}

void Ball::update(float dt) {
    if (dt <= 0.f) return;
    pos += vel * dt;

    float speed = getSpeed();
    if (speed <= 0.f) return;

    float frictionScale = 1.0f;
    if (slideMultiplier > 0.001f) frictionScale = 1.0f / slideMultiplier;
    float newSpeed = speed * std::exp(-(frictionStrength * frictionScale) * dt);
    if (newSpeed < 0.8f) { stop(); return; }
    vel = vecNormalize(vel) * newSpeed;

    // Spin the ball visually while rolling
    if (speed > 10.f)
        spin += (vel.y < 0 ? -1.f : 1.f) * speed * dt * 0.04f;
}

void Ball::stop()              { vel = {0.f, 0.f}; }
sf::Vector2f Ball::getPos()  const { return pos; }
sf::Vector2f Ball::getVel()  const { return vel; }
float Ball::getRadius()      const { return radius; }
float Ball::getMass()        const { return mass; }
float Ball::getSpeed()       const { return vecLength(vel); }
void Ball::setPos(sf::Vector2f p)  { pos = p; }
void Ball::setVel(sf::Vector2f v)  { vel = v; }
void Ball::setColor(sf::Color c)   { ballColor = c; }
void Ball::setBallType(BallType t) { ballType = t; }
void Ball::setSlideMultiplier(float mult) {
    if (mult < 0.2f) mult = 0.2f;
    slideMultiplier = mult;
}

void Ball::applyItemMultipliers(float radiusMult, float massMult) {
    radius = baseRadius * radiusMult;
    mass   = 4.0f * massMult;
}

// ─── Shared helper: draw the classic bowling-ball shell ──────────────────────
static void drawBallShell(sf::RenderWindow& window, sf::Vector2f pos,
                           float radius, sf::Color color, float spin) {
    // Shadow
    float sr = radius * 1.10f;
    sf::CircleShape shadow(sr);
    shadow.setOrigin({sr, sr});
    shadow.setPosition({pos.x + radius * 0.18f, pos.y + radius * 0.22f});
    shadow.setFillColor({0, 0, 0, 70});
    window.draw(shadow);

    // Base
    sf::CircleShape ball(radius);
    ball.setOrigin({radius, radius});
    ball.setPosition(pos);
    ball.setFillColor(color);
    window.draw(ball);

    // Rim
    sf::CircleShape rim(radius * 0.98f);
    rim.setOrigin({radius * 0.98f, radius * 0.98f});
    rim.setPosition(pos);
    rim.setFillColor(sf::Color::Transparent);
    rim.setOutlineThickness(radius * 0.10f);
    rim.setOutlineColor({0, 0, 0, 45});
    window.draw(rim);

    // Inner glow
    sf::CircleShape layer1(radius * 0.92f);
    layer1.setOrigin({radius * 0.92f, radius * 0.92f});
    layer1.setPosition({pos.x - radius * 0.06f, pos.y - radius * 0.08f});
    layer1.setFillColor({(uint8_t)std::min(255, color.r + 20),
                         (uint8_t)std::min(255, color.g + 20),
                         (uint8_t)std::min(255, color.b + 20), 90});
    window.draw(layer1);

    // Highlight
    sf::CircleShape hl(radius * 0.36f);
    hl.setOrigin({radius * 0.36f, radius * 0.36f});
    hl.setPosition({pos.x - radius * 0.34f, pos.y - radius * 0.38f});
    hl.setFillColor({255, 255, 255, 65});
    window.draw(hl);

    // Finger holes
    float holeR = radius * 0.15f;
    sf::CircleShape hole(holeR);
    hole.setOrigin({holeR, holeR});
    hole.setFillColor({0, 0, 0, 150});

    float c = std::cos(spin), s = std::sin(spin);
    auto rot = [&](sf::Vector2f v) {
        return sf::Vector2f(v.x*c - v.y*s, v.x*s + v.y*c);
    };
    sf::Vector2f h1(-radius*0.18f, -radius*0.10f);
    sf::Vector2f h2( radius*0.18f, -radius*0.10f);
    sf::Vector2f h3( 0.f,           radius*0.15f);

    hole.setPosition(pos + rot(h1)); window.draw(hole);
    hole.setPosition(pos + rot(h2)); window.draw(hole);
    hole.setPosition(pos + rot(h3)); window.draw(hole);

    // Specular dot
    sf::CircleShape dot(radius * 0.10f);
    dot.setOrigin({radius*0.10f, radius*0.10f});
    dot.setPosition({pos.x - radius*0.20f, pos.y - radius*0.22f});
    dot.setFillColor({255, 255, 255, 80});
    window.draw(dot);
}

// ─── Per-ball draw functions ─────────────────────────────────────────────────

void Ball::drawNormal(sf::RenderWindow& window) const {
    drawBallShell(window, pos, radius, ballColor, spin);
}

void Ball::drawBlackHole(sf::RenderWindow& window) const {
    // Pulsing purple/dark swirl rings
    float pulse = std::sin(spin * 2.f) * 0.5f + 0.5f;

    // Outer glow ring
    float glowR = radius * 1.35f;
    sf::CircleShape glow(glowR);
    glow.setOrigin({glowR, glowR});
    glow.setPosition(pos);
    glow.setFillColor({80, 0, 120, (uint8_t)(60 + 40 * pulse)});
    window.draw(glow);

    drawBallShell(window, pos, radius, {10, 0, 20}, spin);

    // Swirl arcs drawn as thin rings
    for (int i = 0; i < 3; i++) {
        float ringR = radius * (0.5f + i * 0.18f);
        sf::CircleShape ring(ringR);
        ring.setOrigin({ringR, ringR});
        ring.setPosition(pos);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor({(uint8_t)(120 + i*30), (uint8_t)0, (uint8_t)200, (uint8_t)180});
        window.draw(ring);
    }

    // Centre white dot (event horizon)
    float dotR = radius * 0.12f;
    sf::CircleShape dot(dotR);
    dot.setOrigin({dotR, dotR});
    dot.setPosition(pos);
    dot.setFillColor({255, 255, 255, 200});
    window.draw(dot);
}

void Ball::drawMidas(sf::RenderWindow& window) const {
    // Gold ball with shimmer
    float shimmer = std::sin(spin * 3.f) * 0.5f + 0.5f;
    sf::Color gold((uint8_t)(200 + 30*shimmer), (uint8_t)(160 + 20*shimmer), 20);
    drawBallShell(window, pos, radius, gold, spin);

    // Crown symbol on top
    float cr = radius * 0.35f;
    sf::CircleShape crown(cr);
    crown.setOrigin({cr, cr});
    crown.setPosition({pos.x - radius*0.1f, pos.y - radius*0.25f});
    crown.setFillColor({255, 215, 0, 160});
    window.draw(crown);
}

void Ball::drawUpgrade(sf::RenderWindow& window) const {
    drawBallShell(window, pos, radius, {30, 80, 200}, spin);

    // Blue upward arrow overlay
    // Arrow shaft
    float shaftW = radius * 0.25f;
    float shaftH = radius * 0.55f;
    sf::RectangleShape shaft({shaftW, shaftH});
    shaft.setOrigin({shaftW/2.f, shaftH});
    shaft.setPosition({pos.x, pos.y + radius*0.20f});
    shaft.setFillColor({180, 220, 255, 200});
    window.draw(shaft);

    // Arrow head (triangle)
    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    float hw = radius * 0.45f;
    float hh = radius * 0.40f;
    arrow.setPoint(0, {pos.x,        pos.y - radius*0.25f});
    arrow.setPoint(1, {pos.x - hw,   pos.y + radius*0.10f});
    arrow.setPoint(2, {pos.x + hw,   pos.y + radius*0.10f});
    arrow.setFillColor({180, 220, 255, 210});
    window.draw(arrow);
}

void Ball::drawHeavy(sf::RenderWindow& window) const {
    // Dark grey/iron look, bigger shadow
    sf::Color iron(60, 60, 65);
    float sr = radius * 1.30f;
    sf::CircleShape bigShadow(sr);
    bigShadow.setOrigin({sr, sr});
    bigShadow.setPosition({pos.x + radius*0.22f, pos.y + radius*0.28f});
    bigShadow.setFillColor({0, 0, 0, 90});
    window.draw(bigShadow);

    drawBallShell(window, pos, radius, iron, spin);

    // Weight lines
    for (int i = -1; i <= 1; i++) {
        float lx = pos.x + i * radius * 0.28f;
        sf::RectangleShape line({2.f, radius * 0.60f});
        line.setOrigin({1.f, radius * 0.30f});
        line.setPosition({lx, pos.y});
        line.setFillColor({30, 30, 30, 160});
        window.draw(line);
    }
}

void Ball::drawFastball(sf::RenderWindow& window) const {
    // White baseball with red stitching
    drawBallShell(window, pos, radius, {240, 240, 240}, spin);

    // Red stitching curves (two arcs approximated as lines)
    float c = std::cos(spin), s = std::sin(spin);
    auto rot = [&](sf::Vector2f v) {
        return pos + sf::Vector2f(v.x*c - v.y*s, v.x*s + v.y*c);
    };

    sf::Color red(210, 30, 30, 200);
    int segs = 8;
    for (int side : {-1, 1}) {
        for (int i = 0; i < segs; i++) {
            float t0 = (float)i / segs * 1.2f - 0.6f;
            float t1 = (float)(i+1) / segs * 1.2f - 0.6f;
            float ox = side * radius * 0.28f;
            sf::Vector2f a = rot({ox + std::sin(t0)*radius*0.12f, t0*radius*0.60f});
            sf::Vector2f b = rot({ox + std::sin(t1)*radius*0.12f, t1*radius*0.60f});
            sf::Vertex line[2] = {{a, red}, {b, red}};
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }
}

void Ball::drawOddBall(sf::RenderWindow& window) const {
    // Half green (odd), half orange (even), split diagonally
    float c = std::cos(spin), s = std::sin(spin);

    // Left half green
    drawBallShell(window, pos, radius, {60, 180, 60}, spin);

    // Right half orange using a clipping rectangle trick (draw on top)
    sf::ConvexShape half;
    half.setPointCount(4);
    float r = radius * 1.2f;
    // Rotated right half
    auto rot = [&](sf::Vector2f v) {
        return pos + sf::Vector2f(v.x*c - v.y*s, v.x*s + v.y*c);
    };
    half.setPoint(0, rot({0.f, -r}));
    half.setPoint(1, rot({r,   -r}));
    half.setPoint(2, rot({r,    r}));
    half.setPoint(3, rot({0.f,  r}));
    half.setFillColor({220, 120, 30, 200});
    window.draw(half);

    // Dividing line
    sf::Vertex divLine[2] = {
        {rot({0.f, -radius}), sf::Color::White},
        {rot({0.f,  radius}), sf::Color::White}
    };
    window.draw(divLine, 2, sf::PrimitiveType::Lines);

    // "?" label
    sf::CircleShape dot(radius * 0.12f);
    dot.setOrigin({radius*0.12f, radius*0.12f});
    dot.setPosition({pos.x - radius*0.20f, pos.y - radius*0.22f});
    dot.setFillColor({255, 255, 255, 180});
    window.draw(dot);
}

void Ball::drawEightBall(sf::RenderWindow& window) const {
    drawBallShell(window, pos, radius, {10, 10, 10}, spin);

    // White circle with "8"
    float cr = radius * 0.42f;
    sf::CircleShape circle(cr);
    circle.setOrigin({cr, cr});
    circle.setPosition(pos);
    circle.setFillColor({245, 245, 245});
    window.draw(circle);

    // Draw "8" as two stacked rings (no font needed)
    float tr = cr * 0.38f;
    for (int i : {-1, 1}) {
        sf::CircleShape ring(tr);
        ring.setOrigin({tr, tr});
        ring.setPosition({pos.x, pos.y + i * tr * 0.85f});
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(cr * 0.15f);
        ring.setOutlineColor({10, 10, 10});
        window.draw(ring);
    }
}

void Ball::drawRetrigger(sf::RenderWindow& window) const {
    // Silver with a circular arrow motif
    drawBallShell(window, pos, radius, {160, 170, 180}, spin);

    // Circular arrow: two arcs + arrowheads drawn as a thick ring with a gap
    float ar = radius * 0.52f;
    sf::CircleShape arc(ar);
    arc.setOrigin({ar, ar});
    arc.setPosition(pos);
    arc.setFillColor(sf::Color::Transparent);
    arc.setOutlineThickness(radius * 0.12f);
    arc.setOutlineColor({255, 255, 255, 180});
    window.draw(arc);

    // Two small arrowhead triangles
    float c = std::cos(spin), s = std::sin(spin);
    auto rot = [&](sf::Vector2f v) {
        return pos + sf::Vector2f(v.x*c - v.y*s, v.x*s + v.y*c);
    };
    for (int side : {1, -1}) {
        sf::ConvexShape arrow;
        arrow.setPointCount(3);
        float ax = (float)side * ar;
        float aw = radius * 0.18f, ah = radius * 0.22f;
        arrow.setPoint(0, rot({ax,        -ah/2.f}));
        arrow.setPoint(1, rot({ax - side*aw, ah/2.f}));
        arrow.setPoint(2, rot({ax + side*aw, ah/2.f}));
        arrow.setFillColor({255, 255, 255, 200});
        window.draw(arrow);
    }

    // "x2" text replacement: two dots
    for (int i : {-1, 1}) {
        sf::CircleShape dot(radius * 0.08f);
        dot.setOrigin({radius*0.08f, radius*0.08f});
        dot.setPosition({pos.x + i*radius*0.22f, pos.y + radius*0.05f});
        dot.setFillColor({30, 30, 30, 200});
        window.draw(dot);
    }
}

// ─── Main draw dispatcher ────────────────────────────────────────────────────
void Ball::draw(sf::RenderWindow& window) const {
    switch (ballType) {
        case BallType::BlackHole:  drawBlackHole(window);  break;
        case BallType::Midas:      drawMidas(window);      break;
        case BallType::Upgrade:    drawUpgrade(window);    break;
        case BallType::Heavy:      drawHeavy(window);      break;
        case BallType::Fastball:   drawFastball(window);   break;
        case BallType::OddBall:    drawOddBall(window);    break;
        case BallType::EightBall:  drawEightBall(window);  break;
        case BallType::Retrigger:  drawRetrigger(window);  break;
        default:                   drawNormal(window);     break;
    }
}
