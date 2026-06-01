#include "Pin.h"
#include "rlgl.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <cstdint>

static float pinVecLen(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }

Pin::Pin(Vector2 startPosition, float radius, int value)
    : pos(startPosition), startPos(startPosition),
      vel{0.f, 0.f},
      radius(radius),
      baseMass(2.8f), mass(2.8f),
      restitution(0.55f), frictionStrength(5.0f),
      angularVel(0.f), angle(0.f),
      fallen(false), active(true),
      value(value), baseValue(value)
{}

void Pin::setPinType(PinType t) {
    pinType = t;
    switch (t) {
        case PinType::Light:     mass = baseMass * 0.45f; value = 2; baseValue = 2; break;
        case PinType::Big:       mass = baseMass * 2.2f;  value = 10; baseValue = 10; break;
        case PinType::LuckyDucky: value = 20; baseValue = 20; mass = baseMass; break;
        default:                 mass = baseMass; break;
    }
}

void Pin::rollLuckyDucky() {
    luckyZero = (pinType == PinType::LuckyDucky) && ((rand() % 100) < 35);
}

void Pin::randomiseMischievous() {
    if (pinType == PinType::Mischievous)
        value = 1 + rand() % 15;
}

void Pin::resetToOriginalPosition() {
    pos = startPos; vel = {0.f, 0.f}; angularVel = 0.f; angle = 0.f;
    fallen = false; explodeArmed = false; explodeTimer = 0.f;
    hasExploded = false; luckyZero = false;
    pinType = PinType::Normal; mass = baseMass; value = baseValue;
}

void Pin::resetToOriginalPositionKeepType() {
    pos = startPos; vel = {0.f, 0.f}; angularVel = 0.f; angle = 0.f;
    fallen = false; explodeArmed = false; explodeTimer = 0.f;
    hasExploded = false; luckyZero = false;
}

void Pin::update(float dt) {
    if (!active) return;
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    float fric = fallen
        ? (pinType == PinType::Ice ? 19.f * 0.85f : 19.f)
        : 28.f;
    if (slideMultiplier > 0.001f) fric /= slideMultiplier;

    float speed = pinVecLen(vel);
    if (speed > 0.f) {
        float newSpeed = speed - fric * dt;
        if (newSpeed < 0.f) newSpeed = 0.f;
        vel = {vel.x / speed * newSpeed, vel.y / speed * newSpeed};
    }
    if (!fallen && speed < 40.f) vel = {0.f, 0.f};

    if (fallen) {
        angle += angularVel * dt;
        angularVel *= powf(0.2f, dt);
        if (fabsf(angularVel) < 0.05f) angularVel = 0.f;
        if (pinType == PinType::Exploding && explodeArmed && !hasExploded)
            explodeTimer += dt;
    } else {
        angle = 0.f; angularVel = 0.f;
    }
    if (fabsf(vel.x) < 2.f && fabsf(vel.y) < 2.f) vel = {0.f, 0.f};
}

Vector2 Pin::getPos()          const { return pos; }
Vector2 Pin::getVel()          const { return vel; }
float   Pin::getRadius()       const { return radius; }
float   Pin::getMass()         const { return mass; }
float   Pin::getRestitution()  const { return restitution; }
bool    Pin::isActive()        const { return active; }
bool    Pin::isFallen()        const { return fallen; }
float   Pin::getAngle()        const { return angle; }
float   Pin::getAngularVel()   const { return angularVel; }
void    Pin::setPos(Vector2 p)       { pos = p; }
void    Pin::translate(Vector2 d)    { pos.x += d.x; pos.y += d.y; startPos.x += d.x; startPos.y += d.y; }
void    Pin::setVel(Vector2 v)       { vel = v; }
void    Pin::setActive(bool a)       { active = a; }
void    Pin::setSlideMultiplier(float mult) { if (mult < 0.2f) mult = 0.2f; slideMultiplier = mult; }
void    Pin::setAngularVel(float w)  { angularVel = w; }

void Pin::setFallen(bool f) {
    fallen = f;
    if (!fallen) {
        angle = angularVel = 0.f;
        explodeArmed = false; explodeTimer = 0.f;
    } else {
        if (pinType == PinType::Exploding && !hasExploded)
            explodeArmed = true;
    }
}

// ─── Drawing ──────────────────────────────────────────────────────────────────

// Draw triangle strip via rlgl (affected by current rlgl matrix).
static void rlTriStrip(const std::vector<std::pair<float, float>>& verts,
                       const std::vector<Color>& colors) {
    int n = (int)verts.size();
    rlBegin(RL_TRIANGLES);
    for (int i = 2; i < n; i++) {
        if ((i & 1) == 0) {
            rlColor4ub(colors[i-2].r, colors[i-2].g, colors[i-2].b, colors[i-2].a);
            rlVertex2f(verts[i-2].first, verts[i-2].second);
            rlColor4ub(colors[i-1].r, colors[i-1].g, colors[i-1].b, colors[i-1].a);
            rlVertex2f(verts[i-1].first, verts[i-1].second);
            rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
            rlVertex2f(verts[i].first, verts[i].second);
        } else {
            rlColor4ub(colors[i-1].r, colors[i-1].g, colors[i-1].b, colors[i-1].a);
            rlVertex2f(verts[i-1].first, verts[i-1].second);
            rlColor4ub(colors[i-2].r, colors[i-2].g, colors[i-2].b, colors[i-2].a);
            rlVertex2f(verts[i-2].first, verts[i-2].second);
            rlColor4ub(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
            rlVertex2f(verts[i].first, verts[i].second);
        }
    }
    rlEnd();
}

static void drawPinShape(float H, float maxW,
                          Color bodyColorL, Color bodyColorR,
                          bool drawStripes, Color stripeColor,
                          bool drawBase,  Color baseColor)
{
    constexpr int n = 96;
    std::array<float, n> ys{};
    std::array<float, n> halfWs{};

    auto profileRatio = [](float t) -> float {
        static const float tk[] = {0.00f, 0.04f, 0.10f, 0.17f, 0.24f, 0.31f, 0.39f,
                                   0.49f, 0.59f, 0.68f, 0.77f, 0.86f, 0.94f, 1.00f};
        static const float wk[] = {0.085f, 0.19f, 0.31f, 0.36f, 0.31f, 0.23f, 0.24f,
                                   0.43f, 0.61f, 0.59f, 0.50f, 0.39f, 0.29f, 0.23f};
        constexpr int k = (int)(sizeof(tk) / sizeof(tk[0]));
        if (t <= tk[0]) return wk[0];
        if (t >= tk[k-1]) return wk[k-1];
        for (int i = 0; i < k-1; i++) {
            if (t >= tk[i] && t <= tk[i+1]) {
                float u = (t - tk[i]) / (tk[i+1] - tk[i]);
                u = u * u * (3.f - 2.f * u);
                return wk[i] * (1.f - u) + wk[i+1] * u;
            }
        }
        return wk[k-1];
    };
    for (int i = 0; i < n; i++) {
        float t = (float)i / (float)(n - 1);
        ys[i] = -H * 0.5f + t * H;
        halfWs[i] = maxW * profileRatio(t);
    }
    auto getW = [&](float yPos) -> float {
        if (yPos <= ys[0]) return halfWs[0];
        if (yPos >= ys[n-1]) return halfWs[n-1];
        for (int i = 0; i < n-1; i++) {
            if (yPos >= ys[i] && yPos <= ys[i+1]) {
                float t = (yPos - ys[i]) / (ys[i+1] - ys[i]);
                return halfWs[i] * (1.f - t) + halfWs[i+1] * t;
            }
        }
        return halfWs[0];
    };

    // Body gradient
    rlBegin(RL_TRIANGLES);
    for (int i = 2; i < n * 2; i++) {
        int a = (i - 2) / 2, b = (i - 1) / 2, c2 = i / 2;
        auto cv = [&](int idx) -> Color {
            return (idx % 2 == 0) ? bodyColorL : bodyColorR;
        };
        auto xv = [&](int idx) -> float {
            return (idx % 2 == 0) ? -halfWs[idx/2] : halfWs[idx/2];
        };
        auto yv = [&](int idx) -> float { return ys[idx/2]; };
        if ((i & 1) == 0) {
            rlColor4ub(cv(i-2).r,cv(i-2).g,cv(i-2).b,cv(i-2).a); rlVertex2f(xv(i-2), yv(i-2));
            rlColor4ub(cv(i-1).r,cv(i-1).g,cv(i-1).b,cv(i-1).a); rlVertex2f(xv(i-1), yv(i-1));
            rlColor4ub(cv(i).r,  cv(i).g,  cv(i).b,  cv(i).a);   rlVertex2f(xv(i),   yv(i));
        } else {
            rlColor4ub(cv(i-1).r,cv(i-1).g,cv(i-1).b,cv(i-1).a); rlVertex2f(xv(i-1), yv(i-1));
            rlColor4ub(cv(i-2).r,cv(i-2).g,cv(i-2).b,cv(i-2).a); rlVertex2f(xv(i-2), yv(i-2));
            rlColor4ub(cv(i).r,  cv(i).g,  cv(i).b,  cv(i).a);   rlVertex2f(xv(i),   yv(i));
        }
    }
    rlEnd();

    // Left highlight
    rlBegin(RL_TRIANGLES);
    for (int i = 2; i < n * 2; i++) {
        auto xv2 = [&](int idx) -> float {
            return (idx % 2 == 0) ? -halfWs[idx/2] * 0.82f : -halfWs[idx/2];
        };
        auto yv2 = [&](int idx) -> float { return ys[idx/2]; };
        Color c0 = {255,255,255,(unsigned char)((i%2==0)?96:0)};
        Color c1_col = {255,255,255,(unsigned char)(((i-1)%2==0)?96:0)};
        Color c2_col = {255,255,255,(unsigned char)(((i-2)%2==0)?96:0)};
        if ((i & 1) == 0) {
            rlColor4ub(c2_col.r,c2_col.g,c2_col.b,c2_col.a); rlVertex2f(xv2(i-2), yv2(i-2));
            rlColor4ub(c1_col.r,c1_col.g,c1_col.b,c1_col.a); rlVertex2f(xv2(i-1), yv2(i-1));
            rlColor4ub(c0.r,c0.g,c0.b,c0.a);                  rlVertex2f(xv2(i),   yv2(i));
        } else {
            rlColor4ub(c1_col.r,c1_col.g,c1_col.b,c1_col.a); rlVertex2f(xv2(i-1), yv2(i-1));
            rlColor4ub(c2_col.r,c2_col.g,c2_col.b,c2_col.a); rlVertex2f(xv2(i-2), yv2(i-2));
            rlColor4ub(c0.r,c0.g,c0.b,c0.a);                  rlVertex2f(xv2(i),   yv2(i));
        }
    }
    rlEnd();

    // Specular highlight
    rlBegin(RL_TRIANGLES);
    for (int i = 2; i < n * 2; i++) {
        auto ny = [&](int idx) -> float { return (ys[idx/2] + H*0.5f) / H; };
        auto xa = [&](int idx) -> float { return (idx%2==0) ? -halfWs[idx/2]*0.45f : -halfWs[idx/2]*0.28f; };
        auto ya2 = [&](int idx) -> float { return ys[idx/2]; };
        auto alph = [&](int idx) -> unsigned char {
            int a2 = (int)(132 - fabsf(ny(idx) - 0.42f) * 185.f);
            return (unsigned char)std::max(0, a2);
        };
        if ((i & 1) == 0) {
            rlColor4ub(255,255,255,alph(i-2)); rlVertex2f(xa(i-2), ya2(i-2));
            rlColor4ub(255,255,255,alph(i-1)); rlVertex2f(xa(i-1), ya2(i-1));
            rlColor4ub(255,255,255,alph(i));   rlVertex2f(xa(i),   ya2(i));
        } else {
            rlColor4ub(255,255,255,alph(i-1)); rlVertex2f(xa(i-1), ya2(i-1));
            rlColor4ub(255,255,255,alph(i-2)); rlVertex2f(xa(i-2), ya2(i-2));
            rlColor4ub(255,255,255,alph(i));   rlVertex2f(xa(i),   ya2(i));
        }
    }
    rlEnd();

    // Stripes
    if (drawStripes) {
        auto drawStripe = [&](float yc, float thick) {
            rlBegin(RL_TRIANGLES);
            for (int i = 1; i <= 10; i++) {
                float t0 = (float)(i-1) / 10.f;
                float t1 = (float)i / 10.f;
                float y0 = yc - thick*0.5f + thick*t0;
                float y1 = yc - thick*0.5f + thick*t1;
                float w0 = getW(y0), w1 = getW(y1);
                rlColor4ub(stripeColor.r,stripeColor.g,stripeColor.b,stripeColor.a);
                rlVertex2f(-w0, y0); rlVertex2f(w0, y0); rlVertex2f(-w1, y1);
                rlVertex2f(w0, y0);  rlVertex2f(w1, y1); rlVertex2f(-w1, y1);
            }
            rlEnd();
        };
        drawStripe(-H * 0.235f, maxW * 0.22f);
        drawStripe(-H * 0.175f, maxW * 0.20f);
    }

    // Base ring
    if (drawBase) {
        float by = H * 0.445f, bt = maxW * 0.18f;
        rlBegin(RL_TRIANGLES);
        for (int i = 1; i <= 8; i++) {
            float t0 = (float)(i-1)/8.f, t1 = (float)i/8.f;
            float y0 = by - bt*0.5f + bt*t0, y1 = by - bt*0.5f + bt*t1;
            float w0 = getW(y0), w1 = getW(y1);
            rlColor4ub(baseColor.r,baseColor.g,baseColor.b,baseColor.a);
            rlVertex2f(-w0, y0); rlVertex2f(w0, y0); rlVertex2f(-w1, y1);
            rlVertex2f(w0, y0);  rlVertex2f(w1, y1); rlVertex2f(-w1, y1);
        }
        rlEnd();
    }
}

void Pin::draw() const {
    if (!active) return;

    float drawAngle = 0.f;
    if (fallen) {
        drawAngle = angle;
        if (fabsf(drawAngle) < 10.f) drawAngle = 75.f;
    }

    float H    = radius * 7.38f;
    float maxW = radius * 1.96f;

    // Ground shadow (world space)
    if (!fallen) {
        for (int i = 0; i < 4; i++) {
            float r = radius * 0.92f - i * 1.2f;
            if (r <= 0.5f) continue;
            unsigned char a = (unsigned char)(30 - i * 5);
            DrawEllipse((int)pos.x, (int)(pos.y + H * 0.49f), r * 1.62f, r * 0.38f, {0,0,0,a});
        }
    } else {
        for (int i = 0; i < 3; i++) {
            float r = radius * 0.72f - i * 0.55f;
            if (r <= 0.5f) continue;
            unsigned char a = (unsigned char)(18 - i * 4);
            DrawEllipse((int)pos.x, (int)(pos.y + radius * 0.34f), r * 1.45f, r * 0.30f, {0,0,0,a});
        }
    }

    // Pin body with transform
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, 0.0f);
    rlRotatef(drawAngle, 0.0f, 0.0f, 1.0f);

    switch (pinType) {

    case PinType::Gold:
        drawPinShape(H, maxW, {210,175,50,255}, {255,220,80,255}, true, {180,140,20,255}, true, {200,160,30,255});
        for (int i = 0; i < 3; i++) {
            float gy = -H*.3f + i * H*.15f;
            float gx = -maxW*.2f + i * maxW*.2f;
            DrawCircleV({gx, gy}, radius*.12f, {255,255,180,200});
        }
        break;

    case PinType::Mischievous:
        drawPinShape(H, maxW, {120,20,160,255}, {180,40,220,255}, true, {80,0,120,255}, true, {100,0,140,255});
        DrawCircleV({0.f, -H*.25f}, radius*.35f, {255,255,255,60});
        break;

    case PinType::Exploding: {
        drawPinShape(H, maxW, {200,80,20,255}, {240,120,30,255}, true, {160,40,10,255}, true, {180,60,15,255});
        float pulse = explodeArmed ? (sinf(explodeTimer * 8.f) * 0.5f + 0.5f) : 1.f;
        float dotR = radius * .22f * pulse;
        unsigned char gr = (unsigned char)(50 + 100 * (1 - pulse));
        DrawCircleV({0.f, -H*.52f}, dotR, {255, gr, 0, 220});
        break;
    }

    case PinType::Light:
        drawPinShape(H, maxW, {140,200,240,255}, {200,230,255,255}, true, {100,170,220,255}, true, {120,190,230,255});
        break;

    case PinType::Big:
        drawPinShape(H, maxW, {140,20,20,255}, {200,40,40,255}, true, {100,10,10,255}, true, {120,15,15,255});
        DrawRing({0.f, 0.f}, radius*1.05f - 1.5f, radius*1.05f + 1.5f, 0, 360, 24, {200,40,40,120});
        break;

    case PinType::Ice: {
        drawPinShape(H, maxW, {180,230,255,255}, {220,245,255,255}, true, {140,210,245,255}, true, {160,220,250,255});
        for (int i = 0; i < 4; i++) {
            float a = i * 3.14159f / 2.f;
            float ex = cosf(a) * maxW * .45f;
            float ey = sinf(a) * maxW * .45f - H*.15f;
            rlBegin(RL_LINES);
            rlColor4ub(255,255,255,180); rlVertex2f(0.f, -H*.15f);
            rlColor4ub(200,240,255,0);   rlVertex2f(ex, ey);
            rlEnd();
        }
        break;
    }

    case PinType::CopyCat:
        drawPinShape(H, maxW, {150,150,155,255}, {200,200,205,255}, false, {0,0,0,0}, true, {130,130,135,255});
        for (int i = 0; i < 6; i++) {
            float fy = -H*.4f + i * H*.16f;
            float fw = maxW * .92f;
            DrawRectangle((int)(-fw), (int)(fy - 1.f), (int)(fw * 2.f), 2, {80,80,200,160});
        }
        break;

    case PinType::LuckyDucky: {
        drawPinShape(H, maxW, {230,200,20,255}, {255,230,40,255}, false, {0,0,0,0}, true, {200,170,15,255});
        DrawTriangle({maxW*.5f, -H*.50f}, {0.f, -H*.44f}, {maxW*.5f, -H*.38f}, {255,140,0,255});
        DrawCircleV({maxW*.15f, -H*.47f}, radius*.15f, {20,20,20,255});
        break;
    }

    case PinType::LevelUp:
        drawPinShape(H, maxW, {235,245,255,255}, {255,255,255,255}, true, {80,165,235,255}, true, {170,210,245,255});
        DrawRectangle((int)(-maxW*0.11f), (int)(-H*0.14f - H*0.09f), (int)(maxW*0.22f), (int)(H*0.18f), {80,165,235,210});
        DrawRectangle((int)(-maxW*0.31f), (int)(-H*0.14f - H*0.035f), (int)(maxW*0.62f), (int)(H*0.07f), {80,165,235,210});
        break;

    case PinType::Lover: {
        drawPinShape(H, maxW, {255,170,190,255}, {255,210,225,255}, true, {210,80,130,255}, true, {220,120,160,255});
        DrawCircleV({-maxW*.16f, -H*.18f}, maxW*.22f, {220,60,110,210});
        DrawCircleV({ maxW*.16f, -H*.18f}, maxW*.22f, {220,60,110,210});
        DrawTriangle({maxW*.38f, -H*.14f}, {0.f, -H*.34f}, {-maxW*.38f, -H*.14f}, {220,60,110,210});
        break;
    }

    case PinType::ChangeIsGood:
        drawPinShape(H, maxW, {255,238,175,255}, {255,248,210,255}, true, {200,155,35,255}, true, {210,170,70,255});
        DrawCircleV({-maxW*.22f, -H*.18f}, maxW*.16f, {205,160,30,220});
        DrawCircleV({ maxW*.22f, -H*.18f}, maxW*.16f, {205,160,30,220});
        DrawRectangle((int)(-maxW*0.27f), (int)(-H*0.18f - H*0.03f), (int)(maxW*0.54f), (int)(H*0.06f), {205,160,30,210});
        break;

    case PinType::ThirdTime:
        drawPinShape(H, maxW, {20,140,50,255}, {40,190,70,255}, true, {10,100,30,255}, true, {15,120,40,255});
        DrawRing({0.f, -H*.16f}, maxW*.95f - 1.5f, maxW*.95f + 1.5f, 0, 360, 24, {150,255,150,180});
        break;

    default:
        drawPinShape(H, maxW, {160,160,160,255}, {240,240,240,255}, true, {180,30,30,255}, true, {190,170,100,255});
        break;
    }

    rlPopMatrix();
}
