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

static sf::Color lighten(const sf::Color& c, int amount, std::uint8_t alpha = 255) {
    return sf::Color(
        static_cast<std::uint8_t>(std::clamp((int)c.r + amount, 0, 255)),
        static_cast<std::uint8_t>(std::clamp((int)c.g + amount, 0, 255)),
        static_cast<std::uint8_t>(std::clamp((int)c.b + amount, 0, 255)),
        alpha
    );
}

static sf::Color darken(const sf::Color& c, int amount, std::uint8_t alpha = 255) {
    return sf::Color(
        static_cast<std::uint8_t>(std::clamp((int)c.r - amount, 0, 255)),
        static_cast<std::uint8_t>(std::clamp((int)c.g - amount, 0, 255)),
        static_cast<std::uint8_t>(std::clamp((int)c.b - amount, 0, 255)),
        alpha
    );
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
    // Contact shadow (elliptical) for grounded depth.
    float contactR = radius * 1.05f;
    sf::CircleShape contactShadow(contactR);
    contactShadow.setOrigin({contactR, contactR});
    contactShadow.setPosition({pos.x + radius * 0.16f, pos.y + radius * 0.48f});
    contactShadow.setScale({1.34f, 0.56f});
    contactShadow.setFillColor(sf::Color(0, 0, 0, 86));
    window.draw(contactShadow);

    // Ambient shadow to opposite side of the key light.
    float sideShadowR = radius * 0.92f;
    sf::CircleShape sideShadow(sideShadowR);
    sideShadow.setOrigin({sideShadowR, sideShadowR});
    sideShadow.setPosition({pos.x + radius * 0.20f, pos.y + radius * 0.18f});
    sideShadow.setFillColor(sf::Color(0, 0, 0, 44));
    window.draw(sideShadow);

    // Base sphere.
    sf::CircleShape ball(radius);
    ball.setOrigin({radius, radius});
    ball.setPosition(pos);
    ball.setFillColor(darken(color, 24));
    window.draw(ball);

    // Layered radial lighting (fake PBR style) from top-left.
    sf::Vector2f lightDir = vecNormalize({-0.55f, -0.82f});
    sf::Vector2f lightPos = pos + lightDir * (radius * 0.14f);
    for (int i = 0; i < 7; i++) {
        float t = static_cast<float>(i) / 6.0f;
        float layerR = radius * (0.96f - t * 0.48f);
        sf::CircleShape lit(layerR);
        lit.setOrigin({layerR, layerR});
        lit.setPosition(lightPos);
        int lift = static_cast<int>(54.0f - t * 28.0f);
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(60 - i * 6, 14, 60));
        lit.setFillColor(lighten(color, lift, a));
        window.draw(lit);
    }

    // Dark rim / ambient occlusion near edges.
    sf::CircleShape rim(radius * 0.98f);
    rim.setOrigin({radius * 0.98f, radius * 0.98f});
    rim.setPosition(pos);
    rim.setFillColor(sf::Color::Transparent);
    rim.setOutlineThickness(radius * 0.14f);
    rim.setOutlineColor(sf::Color(0, 0, 0, 62));
    window.draw(rim);

    // Bottom reflected light.
    sf::CircleShape bounce(radius * 0.42f);
    bounce.setOrigin({radius * 0.42f, radius * 0.42f});
    bounce.setPosition({pos.x + radius * 0.05f, pos.y + radius * 0.37f});
    bounce.setScale({1.25f, 0.58f});
    bounce.setFillColor(sf::Color(255, 255, 255, 32));
    window.draw(bounce);

    // Finger holes with depth/lip shading.
    float holeLipR = radius * 0.175f;
    float holeInnerR = radius * 0.122f;
    sf::CircleShape holeLip(holeLipR);
    holeLip.setOrigin({holeLipR, holeLipR});
    holeLip.setFillColor(darken(color, 60, 188));

    sf::CircleShape holeInner(holeInnerR);
    holeInner.setOrigin({holeInnerR, holeInnerR});
    holeInner.setFillColor(sf::Color(4, 6, 12, 214));

    sf::CircleShape holeSpec(holeInnerR * 0.46f);
    holeSpec.setOrigin({holeInnerR * 0.46f, holeInnerR * 0.46f});
    holeSpec.setFillColor(sf::Color(255, 255, 255, 34));

    float c = std::cos(spin), s = std::sin(spin);
    auto rot = [&](sf::Vector2f v) {
        return sf::Vector2f(v.x*c - v.y*s, v.x*s + v.y*c);
    };
    sf::Vector2f h1(-radius*0.18f, -radius*0.10f);
    sf::Vector2f h2( radius*0.18f, -radius*0.10f);
    sf::Vector2f h3( 0.f,           radius*0.15f);

    auto drawHole = [&](sf::Vector2f localPos) {
        sf::Vector2f hp = pos + rot(localPos);
        holeLip.setPosition(hp + sf::Vector2f(radius * 0.010f, radius * 0.012f));
        window.draw(holeLip);
        holeInner.setPosition(hp);
        window.draw(holeInner);
        holeSpec.setPosition(hp + sf::Vector2f(-holeInnerR * 0.25f, -holeInnerR * 0.30f));
        window.draw(holeSpec);
    };
    drawHole(h1);
    drawHole(h2);
    drawHole(h3);

    // Primary and secondary specular highlights.
    sf::CircleShape hlA(radius * 0.30f);
    hlA.setOrigin({radius * 0.30f, radius * 0.30f});
    hlA.setPosition({pos.x - radius * 0.40f, pos.y - radius * 0.44f});
    hlA.setFillColor(sf::Color(255, 255, 255, 82));
    window.draw(hlA);

    sf::CircleShape hlB(radius * 0.11f);
    hlB.setOrigin({radius * 0.11f, radius * 0.11f});
    hlB.setPosition({pos.x - radius * 0.17f, pos.y - radius * 0.22f});
    hlB.setFillColor(sf::Color(255, 255, 255, 106));
    window.draw(hlB);
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
    // Two-colour ball: left green, right orange, with a clean curved split.
    float c = std::cos(spin);
    float s = std::sin(spin);
    drawBallShell(window, pos, radius, {60, 180, 60}, spin);

    auto rot = [&](sf::Vector2f v) -> sf::Vector2f {
        return pos + sf::Vector2f(v.x*c - v.y*s, v.x*s + v.y*c);
    };

    // Right semicircle overlay (curved edge, no square clipping artifact).
    const int segs = 28;
    const float rr = radius * 0.985f;
    sf::ConvexShape half;
    half.setPointCount(static_cast<std::size_t>(segs + 2));
    half.setPoint(0, pos);
    for (int i = 0; i <= segs; i++) {
        float t = -1.5707963f + (3.1415926f * static_cast<float>(i) / static_cast<float>(segs)); // -90..+90 deg
        sf::Vector2f local(rr * std::cos(t), rr * std::sin(t));
        half.setPoint(static_cast<std::size_t>(i + 1), rot(local));
    }
    half.setFillColor({236, 156, 52, 212});
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
        case BallType::Icy:        drawNormal(window);     break;
        case BallType::Retrigger:  drawRetrigger(window);  break;
        default:                   drawNormal(window);     break;
    }
}
