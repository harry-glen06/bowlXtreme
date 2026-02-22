#include "Pin.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <cstdint>

static float length(sf::Vector2f v) {
    return std::sqrt(v.x*v.x + v.y*v.y);
}

// ─── Constructor ─────────────────────────────────────────────────────────────
Pin::Pin(sf::Vector2f startPosition, float radius, int value)
    : pos(startPosition), startPos(startPosition),
      vel(0.f, 0.f),
      radius(radius),
      baseMass(2.8f), mass(2.8f),
      restitution(0.55f), frictionStrength(5.0f),
      angularVel(0.f), angle(0.f),
      fallen(false), active(true),
      value(value), baseValue(value)
{}

void Pin::setPinType(PinType t) {
    pinType = t;
    // Apply stat changes based on type
    switch (t) {
        case PinType::Light:
            mass  = baseMass * 0.45f;
            value = 2;
            baseValue = 2;
            break;
        case PinType::Big:
            mass  = baseMass * 2.2f;
            value = 10;
            baseValue = 10;
            break;
        case PinType::LuckyDucky:
            value = 20;
            baseValue = 20;
            mass = baseMass;
            break;
        default:
            mass = baseMass;
            break;
    }
}

void Pin::rollLuckyDucky() {
    // 35% chance to score 0 this shot
    luckyZero = (pinType == PinType::LuckyDucky) && ((rand() % 100) < 35);
}

void Pin::randomiseMischievous() {
    if (pinType == PinType::Mischievous)
        value = 1 + rand() % 15;
}

// ─── Reset ───────────────────────────────────────────────────────────────────
void Pin::resetToOriginalPosition() {
    pos         = startPos;
    vel         = {0.f, 0.f};
    angularVel  = 0.f;
    angle       = 0.f;
    fallen      = false;
    explodeArmed  = false;
    explodeTimer  = 0.f;
    hasExploded   = false;
    luckyZero     = false;
    // Reset type back to Normal — purchased types get re-applied by Game
    pinType = PinType::Normal;
    mass    = baseMass;
    value   = baseValue;
}

void Pin::resetToOriginalPositionKeepType() {
    pos         = startPos;
    vel         = {0.f, 0.f};
    angularVel  = 0.f;
    angle       = 0.f;
    fallen      = false;
    explodeArmed  = false;
    explodeTimer  = 0.f;
    hasExploded   = false;
    luckyZero     = false;
}

// ─── Update ──────────────────────────────────────────────────────────────────
void Pin::update(float dt) {
    if (!active) return;

    pos += vel * dt;

    // Ice pins slide faster when fallen
    float fric = fallen
        ? (pinType == PinType::Ice ? 19.f * 0.85f : 19.f)
        : 28.f;
    if (slideMultiplier > 0.001f) fric /= slideMultiplier;

    float speed = length(vel);
    if (speed > 0.f) {
        sf::Vector2f dir = vel / speed;
        float newSpeed = speed - fric * dt;
        if (newSpeed < 0.f) newSpeed = 0.f;
        vel = dir * newSpeed;
    }

    if (!fallen && speed < 40.f)   vel = {0.f, 0.f};
    if (!fallen && speed < 120.f)  vel = {0.f, 0.f};

    if (fallen) {
        angle      += angularVel * dt;
        angularVel *= std::pow(0.2f, dt);
        if (std::abs(angularVel) < 0.05f) angularVel = 0.f;

        // Exploding pin timer
        if (pinType == PinType::Exploding && explodeArmed && !hasExploded) {
            explodeTimer += dt;
        }
    } else {
        angle      = 0.f;
        angularVel = 0.f;
    }

    if (std::abs(vel.x) < 2.f && std::abs(vel.y) < 2.f) vel = {0.f, 0.f};
}

// ─── Setters ─────────────────────────────────────────────────────────────────
sf::Vector2f Pin::getPos()        const { return pos; }
sf::Vector2f Pin::getVel()        const { return vel; }
float        Pin::getRadius()     const { return radius; }
float        Pin::getMass()       const { return mass; }
float        Pin::getRestitution()const { return restitution; }
bool         Pin::isActive()      const { return active; }
bool         Pin::isFallen()      const { return fallen; }
float        Pin::getAngle()      const { return angle; }
float        Pin::getAngularVel() const { return angularVel; }

void Pin::setPos(sf::Vector2f p) { pos = p; }
void Pin::translate(sf::Vector2f delta) {
    pos += delta;
    startPos += delta;
}
void Pin::setVel(sf::Vector2f v) { vel = v; }
void Pin::setActive(bool a)      { active = a; }
void Pin::setSlideMultiplier(float mult) {
    if (mult < 0.2f) mult = 0.2f;
    slideMultiplier = mult;
}
void Pin::setAngularVel(float w) { angularVel = w; }

void Pin::setFallen(bool f) {
    fallen = f;
    if (!fallen) {
        angle = angularVel = 0.f;
        explodeArmed = false;
        explodeTimer = 0.f;
    } else {
        if (pinType == PinType::Exploding && !hasExploded)
            explodeArmed = true;
    }
}

// ─── Drawing helpers ─────────────────────────────────────────────────────────
// Shared pin silhouette — draws the classic bowling-pin shape in a given colour,
// already in local transform space (origin = pin centre).
static void drawPinShape(sf::RenderWindow& window, sf::RenderStates st,
                          float H, float maxW,
                          sf::Color bodyColorL, sf::Color bodyColorR,
                          bool drawStripes, sf::Color stripeColor,
                          bool drawBase,  sf::Color baseColor)
{
    // Real-pin-inspired profile (top -> bottom), based on regulation pin silhouette.
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
        if (t >= tk[k - 1]) return wk[k - 1];
        for (int i = 0; i < k - 1; i++) {
            if (t >= tk[i] && t <= tk[i + 1]) {
                float u = (t - tk[i]) / (tk[i + 1] - tk[i]);
                // Smooth interpolation keeps the neck/belly transitions natural.
                u = u * u * (3.f - 2.f * u);
                return wk[i] * (1.f - u) + wk[i + 1] * u;
            }
        }
        return wk[k - 1];
    };
    for (int i = 0; i < n; i++) {
        float t = static_cast<float>(i) / static_cast<float>(n - 1);
        ys[i] = -H * 0.5f + t * H;
        halfWs[i] = maxW * profileRatio(t);
    }

    auto getW = [&](float yPos) -> float {
        if (yPos <= ys[0]) return halfWs[0];
        if (yPos >= ys[n - 1]) return halfWs[n - 1];
        for (int i = 0; i < n-1; i++) {
            if (yPos >= ys[i] && yPos <= ys[i+1]) {
                float t = (yPos - ys[i]) / (ys[i + 1] - ys[i]);
                return halfWs[i] * (1.f - t) + halfWs[i + 1] * t;
            }
        }
        return halfWs[0];
    };

    // Body gradient
    sf::VertexArray body(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i < n; i++) {
        body.append({{-halfWs[i], ys[i]}, bodyColorL});
        body.append({{ halfWs[i], ys[i]}, bodyColorR});
    }
    window.draw(body, st);

    // Left highlight
    sf::VertexArray lhl(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i < n; i++) {
        lhl.append({{-halfWs[i] * 0.82f, ys[i]}, {255,255,255,96}});
        lhl.append({{-halfWs[i],         ys[i]}, {255,255,255,0}});
    }
    window.draw(lhl, st);

    // Specular
    sf::VertexArray spec(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i < n; i++) {
        float ny = (ys[i] + H * 0.5f) / H;
        int a = (int)(132 - std::abs(ny - 0.42f) * 185.f);
        a = std::max(0, a);
        spec.append({{-halfWs[i] * 0.45f, ys[i]}, {255,255,255,(uint8_t)a}});
        spec.append({{-halfWs[i] * 0.28f, ys[i]}, {255,255,255,(uint8_t)a}});
    }
    window.draw(spec, st);

    // Stripes
    if (drawStripes) {
        auto drawStripe = [&](float yc, float thick) {
            sf::VertexArray s(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= 10; i++) {
                float t = static_cast<float>(i) / 10.f;
                float y = yc - thick * 0.5f + thick * t;
                float w = getW(y);
                s.append({{-w, y}, stripeColor});
                s.append({{ w, y}, stripeColor});
            }
            window.draw(s, st);
        };
        drawStripe(-H * 0.235f, maxW * 0.22f);
        drawStripe(-H * 0.175f, maxW * 0.20f);
    }

    // Base ring
    if (drawBase) {
        float by = H * 0.445f, bt = maxW * 0.18f;
        sf::VertexArray br(sf::PrimitiveType::TriangleStrip);
        for (int i = 0; i <= 8; i++) {
            float t = static_cast<float>(i) / 8.f;
            float y = by - bt * 0.5f + bt * t;
            float w = getW(y);
            br.append({{-w, y}, baseColor});
            br.append({{ w, y}, baseColor});
        }
        window.draw(br, st);
    }
}

// ─── Draw ─────────────────────────────────────────────────────────────────────
void Pin::draw(sf::RenderWindow& window) const {
    if (!active) return;

    float drawAngle = 0.f;
    if (fallen) {
        drawAngle = angle;
        if (std::abs(drawAngle) < 10.f) drawAngle = 75.f;
    }

    // Tuned pin profile: 6.5% fatter and ~3% shorter than current.
    float H    = radius * 7.38f;
    float maxW = radius * 1.96f;

    // Ground shadow.
    if (!fallen) {
        float shadowR = radius * 0.92f;
        for (int i = 0; i < 4; i++) {
            float r = shadowR - i * 1.2f;
            if (r <= 0.5f) continue;
            sf::CircleShape shadow(r);
            shadow.setOrigin({r, r});
            shadow.setPosition({pos.x, pos.y + H * 0.49f});
            shadow.setScale({1.62f, 0.38f});
            const auto shadowAlpha = static_cast<std::uint8_t>(30 - i * 5);
            shadow.setFillColor(sf::Color(0, 0, 0, shadowAlpha));
            window.draw(shadow);
        }
    } else {
        // Fallen-pin shadow: compact floor contact, avoids long black streaks.
        float shadowR = radius * 0.72f;
        for (int i = 0; i < 3; i++) {
            float r = shadowR - i * 0.55f;
            if (r <= 0.5f) continue;
            sf::CircleShape shadow(r);
            shadow.setOrigin({r, r});
            shadow.setPosition({pos.x, pos.y + radius * 0.34f});
            shadow.setScale({1.45f, 0.30f});
            const auto shadowAlpha = static_cast<std::uint8_t>(18 - i * 4);
            shadow.setFillColor(sf::Color(0, 0, 0, shadowAlpha));
            window.draw(shadow);
        }
    }

    sf::Transform t;
    t.translate(pos);
    t.rotate(sf::degrees(drawAngle));
    sf::RenderStates st;
    st.transform = t;

    switch (pinType) {

    case PinType::Gold: {
        // Shimmering gold body, gold stripes
        drawPinShape(window, st, H, maxW,
            {210,175,50}, {255,220,80},
            true,  {180,140,20},
            true,  {200,160,30});
        // Gold sparkle dots
        for (int i = 0; i < 3; i++) {
            float gy = -H*.3f + i * H*.15f;
            float gx = -maxW*.2f + i * maxW*.2f;
            sf::CircleShape dot(radius*.12f);
            dot.setOrigin({radius*.12f, radius*.12f});
            dot.setPosition({gx, gy});
            dot.setFillColor({255,255,180,200});
            window.draw(dot, st);
        }
        break;
    }

    case PinType::Mischievous: {
        // Purple/magenta swirly pin
        drawPinShape(window, st, H, maxW,
            {120,20,160}, {180,40,220},
            true,  {80,0,120},
            true,  {100,0,140});
        // "?" overlay
        sf::CircleShape q(radius*.35f);
        q.setOrigin({radius*.35f, radius*.35f});
        q.setPosition({0.f, -H*.25f});
        q.setFillColor({255,255,255,60});
        window.draw(q, st);
        break;
    }

    case PinType::Exploding: {
        // Orange/red pin, countdown indicator
        drawPinShape(window, st, H, maxW,
            {200,80,20}, {240,120,30},
            true,  {160,40,10},
            true,  {180,60,15});
        // Fuse dot on top — pulses as timer counts down
        float pulse = explodeArmed
            ? (std::sin(explodeTimer * 8.f) * 0.5f + 0.5f)
            : 1.f;
        float dotR = radius * .22f * pulse;
        sf::CircleShape fuse(dotR);
        fuse.setOrigin({dotR, dotR});
        fuse.setPosition({0.f, -H*.52f});
        fuse.setFillColor({255,(uint8_t)(50+100*(1-pulse)),0,220});
        window.draw(fuse, st);
        break;
    }

    case PinType::Light: {
        // Pale blue, thin-looking
        drawPinShape(window, st, H, maxW,
            {140,200,240}, {200,230,255},
            true,  {100,170,220},
            true,  {120,190,230});
        break;
    }

    case PinType::Big: {
        // Dark red, chunky (radius visually emphasised via extra outline)
        drawPinShape(window, st, H, maxW,
            {140,20,20}, {200,40,40},
            true,  {100,10,10},
            true,  {120,15,15});
        sf::CircleShape ring(radius*1.05f);
        ring.setOrigin({radius*1.05f, radius*1.05f});
        ring.setPosition({0.f, 0.f});
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(3.f);
        ring.setOutlineColor({200,40,40,120});
        window.draw(ring, st);
        break;
    }

    case PinType::Ice: {
        // Icy cyan/white
        drawPinShape(window, st, H, maxW,
            {180,230,255}, {220,245,255},
            true,  {140,210,245},
            true,  {160,220,250});
        // Ice crystal flare
        for (int i = 0; i < 4; i++) {
            float a = i * 3.14159f / 2.f;
            float ex = std::cos(a) * maxW * .45f;
            float ey = std::sin(a) * maxW * .45f - H*.15f;
            sf::Vertex line[2] = {
                {{0.f, -H*.15f}, {255,255,255,180}},
                {{ex, ey},       {200,240,255,0}}
            };
            window.draw(line, 2, sf::PrimitiveType::Lines, st);
        }
        break;
    }

    case PinType::CopyCat: {
        // Mirrored silver/grey with dashed outline
        drawPinShape(window, st, H, maxW,
            {150,150,155}, {200,200,205},
            false, {},
            true,  {130,130,135});
        // Dashed outline approximation
        for (int i = 0; i < 6; i++) {
            float fy = -H*.4f + i * H*.16f;
            float fw = maxW * .92f;
            sf::RectangleShape dash({fw*2.f, 2.f});
            dash.setOrigin({fw, 1.f});
            dash.setPosition({0.f, fy});
            dash.setFillColor({80,80,200,160});
            window.draw(dash, st);
        }
        break;
    }

    case PinType::LuckyDucky: {
        // Bright yellow duck-like pin
        drawPinShape(window, st, H, maxW,
            {230,200,20}, {255,230,40},
            false, {},
            true,  {200,170,15});
        // Duck bill triangle
        sf::ConvexShape bill;
        bill.setPointCount(3);
        bill.setPoint(0, {0.f,        -H*.44f});
        bill.setPoint(1, {maxW*.5f,   -H*.50f});
        bill.setPoint(2, {maxW*.5f,   -H*.38f});
        bill.setFillColor({255,140,0});
        window.draw(bill, st);
        // Eye dot
        sf::CircleShape eye(radius*.15f);
        eye.setOrigin({radius*.15f, radius*.15f});
        eye.setPosition({maxW*.15f, -H*.47f});
        eye.setFillColor({20,20,20});
        window.draw(eye, st);
        break;
    }

    case PinType::ThirdTime: {
        // Green pin with "x2" indicator ring
        drawPinShape(window, st, H, maxW,
            {20,140,50}, {40,190,70},
            true,  {10,100,30},
            true,  {15,120,40});
        // Ring around belly
        sf::CircleShape ring(maxW*.95f);
        ring.setOrigin({maxW*.95f, maxW*.95f});
        ring.setPosition({0.f, -H*.16f});
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(3.f);
        ring.setOutlineColor({150,255,150,180});
        window.draw(ring, st);
        break;
    }

    default: {
        // Normal white pin
        drawPinShape(window, st, H, maxW,
            {160,160,160}, {240,240,240},
            true,  {180,30,30},
            true,  {190,170,100});
        break;
    }
    }

    // Value label (small number at the belly of standing pins)
    // Only draw if pin has a non-zero value and is standing
    // (Rendered as a tiny dot cluster; no font dependency)
    // Skip for now — Game draws value labels via UI if desired
}
