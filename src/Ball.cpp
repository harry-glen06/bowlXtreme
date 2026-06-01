#include "Ball.h"
#include "rlgl.h"
#include <cmath>
#include <algorithm>

static float vecLength(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}
static Vector2 vecNormalize(Vector2 v) {
    float len = vecLength(v);
    if (len <= 0.00001f) return {0.f, 0.f};
    return {v.x / len, v.y / len};
}

static Color clrLighten(Color c, int amount, unsigned char alpha = 255) {
    return Color{
        (unsigned char)std::clamp((int)c.r + amount, 0, 255),
        (unsigned char)std::clamp((int)c.g + amount, 0, 255),
        (unsigned char)std::clamp((int)c.b + amount, 0, 255),
        alpha
    };
}
static Color clrDarken(Color c, int amount, unsigned char alpha = 255) {
    return Color{
        (unsigned char)std::clamp((int)c.r - amount, 0, 255),
        (unsigned char)std::clamp((int)c.g - amount, 0, 255),
        (unsigned char)std::clamp((int)c.b - amount, 0, 255),
        alpha
    };
}

Ball::Ball(float r)
    : pos{0.f, 0.f}, vel{0.f, 0.f},
      baseRadius(r), radius(r), mass(4.0f),
      frictionStrength(1.05f), spin(0.f),
      ballColor{25, 55, 140, 255} {}

void Ball::reset(Vector2 startPos) {
    pos  = startPos;
    vel  = {0.f, 0.f};
    spin = 0.f;
}

void Ball::launch(Vector2 direction, float speed) {
    vel = vecNormalize(direction);
    vel.x *= speed;
    vel.y *= speed;
}

void Ball::update(float dt) {
    if (dt <= 0.f) return;
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    float speed = getSpeed();
    if (speed <= 0.f) return;

    float frictionScale = 1.0f;
    if (slideMultiplier > 0.001f) frictionScale = 1.0f / slideMultiplier;
    float newSpeed = speed * expf(-(frictionStrength * frictionScale) * dt);
    if (newSpeed < 0.8f) { stop(); return; }
    Vector2 n = vecNormalize(vel);
    vel = {n.x * newSpeed, n.y * newSpeed};

    if (speed > 10.f)
        spin += (vel.y < 0 ? -1.f : 1.f) * speed * dt * 0.04f;
}

void Ball::stop()             { vel = {0.f, 0.f}; }
Vector2 Ball::getPos()  const { return pos; }
Vector2 Ball::getVel()  const { return vel; }
float Ball::getRadius() const { return radius; }
float Ball::getMass()   const { return mass; }
float Ball::getSpeed()  const { return vecLength(vel); }
void Ball::setPos(Vector2 p)  { pos = p; }
void Ball::setVel(Vector2 v)  { vel = v; }
void Ball::setColor(Color c)  { ballColor = c; }
void Ball::setBallType(BallType t) { ballType = t; }
void Ball::setSlideMultiplier(float mult) {
    if (mult < 0.2f) mult = 0.2f;
    slideMultiplier = mult;
}

void Ball::applyItemMultipliers(float radiusMult, float massMult) {
    radius = baseRadius * radiusMult;
    mass   = 4.0f * massMult;
}

// ─── Shared shell ─────────────────────────────────────────────────────────────
static void drawBallShell(Vector2 pos, float radius, Color color, float spin) {
    // Contact shadow (ellipse)
    float cR = radius * 1.05f;
    DrawEllipse((int)(pos.x + radius * 0.16f), (int)(pos.y + radius * 0.48f),
                cR * 1.34f, cR * 0.56f, {0, 0, 0, 86});

    // Ambient shadow
    float ssR = radius * 0.92f;
    DrawCircleV({pos.x + radius * 0.20f, pos.y + radius * 0.18f}, ssR, {0, 0, 0, 44});

    // Base sphere
    DrawCircleV(pos, radius, clrDarken(color, 24));

    // Layered lighting from top-left
    Vector2 lightDir = vecNormalize({-0.55f, -0.82f});
    Vector2 lightPos = {pos.x + lightDir.x * (radius * 0.14f),
                        pos.y + lightDir.y * (radius * 0.14f)};
    for (int i = 0; i < 7; i++) {
        float t = (float)i / 6.0f;
        float layerR = radius * (0.96f - t * 0.48f);
        int lift = (int)(54.0f - t * 28.0f);
        unsigned char a = (unsigned char)std::clamp(60 - i * 6, 14, 60);
        DrawCircleV(lightPos, layerR, clrLighten(color, lift, a));
    }

    // Rim / AO
    DrawRing(pos, radius * 0.84f, radius * 0.98f, 0, 360, 36, {0, 0, 0, 62});

    // Bottom reflected light (ellipse)
    DrawEllipse((int)(pos.x + radius * 0.05f), (int)(pos.y + radius * 0.37f),
                radius * 0.42f * 1.25f, radius * 0.42f * 0.58f, {255, 255, 255, 32});

    // Finger holes
    float holeLipR  = radius * 0.175f;
    float holeInnerR = radius * 0.122f;
    float c = cosf(spin), s = sinf(spin);
    auto rot = [&](Vector2 v) -> Vector2 {
        return {pos.x + v.x * c - v.y * s, pos.y + v.x * s + v.y * c};
    };
    Vector2 h1 = {-radius * 0.18f, -radius * 0.10f};
    Vector2 h2 = { radius * 0.18f, -radius * 0.10f};
    Vector2 h3 = {0.f,              radius * 0.15f};
    auto drawHole = [&](Vector2 local) {
        Vector2 hp = rot(local);
        DrawCircleV({hp.x + radius * 0.010f, hp.y + radius * 0.012f}, holeLipR, clrDarken(color, 60, 188));
        DrawCircleV(hp, holeInnerR, {4, 6, 12, 214});
        DrawCircleV({hp.x - holeInnerR * 0.25f, hp.y - holeInnerR * 0.30f},
                    holeInnerR * 0.46f, {255, 255, 255, 34});
    };
    drawHole(h1);
    drawHole(h2);
    drawHole(h3);

    // Specular highlights
    DrawCircleV({pos.x - radius * 0.40f, pos.y - radius * 0.44f}, radius * 0.30f, {255, 255, 255, 82});
    DrawCircleV({pos.x - radius * 0.17f, pos.y - radius * 0.22f}, radius * 0.11f, {255, 255, 255, 106});
}

// ─── Per-ball draw functions ──────────────────────────────────────────────────

void Ball::drawNormal() const {
    drawBallShell(pos, radius, ballColor, spin);
}

void Ball::drawBlackHole() const {
    float pulse = sinf(spin * 2.f) * 0.5f + 0.5f;
    float glowR = radius * 1.35f;
    DrawCircleV(pos, glowR, {80, 0, 120, (unsigned char)(60 + 40 * pulse)});
    drawBallShell(pos, radius, {10, 0, 20, 255}, spin);
    for (int i = 0; i < 3; i++) {
        float ringR = radius * (0.5f + i * 0.18f);
        DrawRing(pos, ringR - 1.f, ringR + 1.f, 0, 360, 36,
                 {(unsigned char)(120 + i * 30), 0, 200, 180});
    }
    DrawCircleV(pos, radius * 0.12f, {255, 255, 255, 200});
}

void Ball::drawMidas() const {
    float shimmer = sinf(spin * 3.f) * 0.5f + 0.5f;
    Color gold = {(unsigned char)(200 + 30 * shimmer), (unsigned char)(160 + 20 * shimmer), 20, 255};
    drawBallShell(pos, radius, gold, spin);
    DrawCircleV({pos.x - radius * 0.1f, pos.y - radius * 0.25f}, radius * 0.35f, {255, 215, 0, 160});
}

void Ball::drawUpgrade() const {
    drawBallShell(pos, radius, {30, 80, 200, 255}, spin);

    float shaftW = radius * 0.25f, shaftH = radius * 0.55f;
    DrawRectangle((int)(pos.x - shaftW / 2.f), (int)(pos.y + radius * 0.20f - shaftH),
                  (int)shaftW, (int)shaftH, {180, 220, 255, 200});

    float hw = radius * 0.45f, hh = radius * 0.40f;
    DrawTriangle(
        {pos.x, pos.y - radius * 0.25f},
        {pos.x + hw, pos.y + radius * 0.10f},
        {pos.x - hw, pos.y + radius * 0.10f},
        {180, 220, 255, 210});
}

void Ball::drawHeavy() const {
    Color iron = {60, 60, 65, 255};
    DrawEllipse((int)(pos.x + radius * 0.22f), (int)(pos.y + radius * 0.28f),
                radius * 1.30f, radius * 1.30f, {0, 0, 0, 90});
    drawBallShell(pos, radius, iron, spin);
    for (int i = -1; i <= 1; i++) {
        float lx = pos.x + i * radius * 0.28f;
        DrawRectangle((int)(lx - 1.f), (int)(pos.y - radius * 0.30f), 2, (int)(radius * 0.60f), {30, 30, 30, 160});
    }
}

void Ball::drawFastball() const {
    drawBallShell(pos, radius, {240, 240, 240, 255}, spin);
    float c = cosf(spin), s = sinf(spin);
    auto rot = [&](Vector2 v) -> Vector2 {
        return {pos.x + v.x * c - v.y * s, pos.y + v.x * s + v.y * c};
    };
    Color red = {210, 30, 30, 200};
    int segs = 8;
    for (int side : {-1, 1}) {
        for (int i = 0; i < segs; i++) {
            float t0 = (float)i / segs * 1.2f - 0.6f;
            float t1 = (float)(i + 1) / segs * 1.2f - 0.6f;
            float ox = (float)side * radius * 0.28f;
            Vector2 a = rot({ox + sinf(t0) * radius * 0.12f, t0 * radius * 0.60f});
            Vector2 b = rot({ox + sinf(t1) * radius * 0.12f, t1 * radius * 0.60f});
            DrawLineV(a, b, red);
        }
    }
}

void Ball::drawOddBall() const {
    float c = cosf(spin), s = sinf(spin);
    drawBallShell(pos, radius, {60, 180, 60, 255}, spin);
    auto rot = [&](Vector2 v) -> Vector2 {
        return {pos.x + v.x * c - v.y * s, pos.y + v.x * s + v.y * c};
    };
    const int segs = 28;
    const float rr = radius * 0.985f;
    // Right semi-circle overlay
    Vector2 pts[segs + 2];
    pts[0] = pos;
    for (int i = 0; i <= segs; i++) {
        float t = -1.5707963f + (3.1415926f * (float)i / (float)segs);
        pts[i + 1] = rot({rr * cosf(t), rr * sinf(t)});
    }
    DrawTriangleFan(pts, segs + 2, {236, 156, 52, 212});
    DrawLineV(rot({0.f, -radius}), rot({0.f, radius}), WHITE);
    DrawCircleV({pos.x - radius * 0.20f, pos.y - radius * 0.22f}, radius * 0.12f, {255, 255, 255, 180});
}

void Ball::drawEightBall() const {
    drawBallShell(pos, radius, {10, 10, 10, 255}, spin);
    float cr = radius * 0.42f;
    DrawCircleV(pos, cr, {245, 245, 245, 255});
    float tr = cr * 0.38f;
    for (int i : {-1, 1}) {
        DrawRing({pos.x, pos.y + (float)i * tr * 0.85f}, tr - cr * 0.15f / 2, tr + cr * 0.15f / 2,
                 0, 360, 24, {10, 10, 10, 255});
    }
}

void Ball::drawRetrigger() const {
    drawBallShell(pos, radius, {160, 170, 180, 255}, spin);
    float ar = radius * 0.52f;
    DrawRing(pos, ar, ar + radius * 0.12f, 0, 360, 36, {255, 255, 255, 180});

    float c = cosf(spin), s = sinf(spin);
    auto rot = [&](Vector2 v) -> Vector2 {
        return {pos.x + v.x * c - v.y * s, pos.y + v.x * s + v.y * c};
    };
    for (int side : {1, -1}) {
        float ax = (float)side * ar;
        float aw = radius * 0.18f, ah = radius * 0.22f;
        DrawTriangle(
            rot({ax, -ah / 2.f}),
            rot({ax + (float)side * aw, ah / 2.f}),
            rot({ax - (float)side * aw, ah / 2.f}),
            {255, 255, 255, 200});
    }
    for (int i : {-1, 1}) {
        DrawCircleV({pos.x + (float)i * radius * 0.22f, pos.y + radius * 0.05f}, radius * 0.08f, {30, 30, 30, 200});
    }
}

void Ball::draw() const {
    switch (ballType) {
        case BallType::BlackHole:  drawBlackHole();  break;
        case BallType::Midas:      drawMidas();      break;
        case BallType::Upgrade:    drawUpgrade();    break;
        case BallType::Heavy:      drawHeavy();      break;
        case BallType::Fastball:   drawFastball();   break;
        case BallType::OddBall:    drawOddBall();    break;
        case BallType::EightBall:  drawEightBall();  break;
        case BallType::Icy:        drawNormal();     break;
        case BallType::Retrigger:  drawRetrigger();  break;
        default:                   drawNormal();     break;
    }
}
