#include "Pin.h"
#include <cmath>

static float length(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Pin::Pin(sf::Vector2f startPosition, float radius, int value)
    : pos(startPosition),
      startPos(startPosition),  //Save the original position
      vel(0.0f, 0.0f),
      radius(radius),
      mass(2.8f),
      restitution(0.55f),
      frictionStrength(5.0f),
      angularVel(0.0f),
      angle(0.0f),
      fallen(false),
      active(true),
      value(value)
{
}

void Pin::resetToOriginalPosition() {
    pos = startPos;                    // Move back to original triangle spot
    vel = sf::Vector2f(0.0f, 0.0f);    // Stop moving
    angularVel = 0.0f;                 // Stop spinning
    angle = 0.0f;                      // Stand upright
    fallen = false;                    // Mark as standing
    // NOTE: Don't change 'active' - that's handled by Game logic
}
void Pin::update(float dt) {
    if (!active) return;
    // Move
    pos += vel * dt;

    // Different friction for standing vs fallen
    float fric = fallen ? 19.0f : 28.0f;

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

    // Enhanced shadow with soft edges
    float shadowR = radius * 1.4f;
    for (int i = 0; i < 5; i++) {
        sf::CircleShape shadow(shadowR - i * 3.0f);
        shadow.setOrigin(sf::Vector2f(shadow.getRadius(), shadow.getRadius()));
        shadow.setPosition(sf::Vector2f(pos.x + radius * 0.3f, pos.y + radius * 0.3f));
        shadow.setFillColor(sf::Color(0, 0, 0, 30 - i * 4));
        window.draw(shadow);
    }

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
    float H = radius * 5.0f;   // total height
    float maxW = radius * 2.0f; // belly width

    // --- Official USBC Bowling Pin Specifications ---
    // Total height: 15" (measuring from bottom)
    // All measurements are diameter readings at specific heights from bottom
    // Converting diameter to radius (halfW) as percentage of max diameter (4.766" at 6.5")
    
    struct Slice { float y; float halfW; };
    
    // Note: y-coordinates are inverted (negative = top, positive = bottom)
    // Converting bottom-up measurements to top-down coordinates
    
    Slice slices[] = {
        // Top of pin (15" from bottom = -H * 0.58)
        { -H * 0.58f, maxW * 0.127f },  // 1.212" dia at 15" = 25.4% of max
        
        // Head section
        { -H * 0.56f, maxW * 0.165f },  // ~1.57" dia at 14.5"
        { -H * 0.54f, maxW * 0.200f },  // ~1.9" dia at 14"
        { -H * 0.52f, maxW * 0.235f },  // 2.24" dia at 13.5"
        { -H * 0.50f, maxW * 0.270f },  // 2.57" dia at 13"
        
        // Upper neck
        { -H * 0.48f, maxW * 0.295f },  // 2.81" dia at 12.5"
        { -H * 0.46f, maxW * 0.315f },  // 3.0" dia at 12"
        { -H * 0.44f, maxW * 0.330f },  // 3.14" dia at 11.5"
        { -H * 0.42f, maxW * 0.340f },  // 3.237" dia at 11" (neck area)
        
        // Neck to shoulder transition
        { -H * 0.40f, maxW * 0.345f },  // 3.288" dia at 10.5"
        { -H * 0.38f, maxW * 0.360f },  // 3.426" dia at 10"
        { -H * 0.36f, maxW * 0.385f },  // 3.664" dia at 9.5"
        { -H * 0.34f, maxW * 0.420f },  // 4.0" dia at 9"
        
        // Shoulder expansion
        { -H * 0.32f, maxW * 0.470f },  // 4.47" dia at 8.5"
        { -H * 0.30f, maxW * 0.535f },  // ~5.09" dia at 8"
        { -H * 0.28f, maxW * 0.610f },  // ~5.8" dia at 7.5"
        { -H * 0.26f, maxW * 0.690f },  // ~6.56" dia at 7"
        
        // Approaching widest point (belly)
        { -H * 0.24f, maxW * 0.780f },  // ~7.42" dia at 6.5" (widest point: 4.766" radius)
        { -H * 0.22f, maxW * 0.870f },  // approaching max
        { -H * 0.20f, maxW * 0.950f },  // 
        { -H * 0.18f, maxW * 0.990f },  // near widest
        { -H * 0.16f, maxW * 1.000f },  // 4.766" dia at ~6.5" HEIGHT (WIDEST)
        { -H * 0.14f, maxW * 1.000f },  
        { -H * 0.12f, maxW * 0.995f },  
        
        // Starting to taper
        { -H * 0.10f, maxW * 0.980f },  // 4.667" dia at 6"
        { -H * 0.08f, maxW * 0.960f },  
        { -H * 0.06f, maxW * 0.935f },  // 4.457" dia at 5.5"
        { -H * 0.04f, maxW * 0.905f },  
        { -H * 0.02f, maxW * 0.875f },  // 4.17" dia at 5"
        {  H * 0.00f, maxW * 0.850f },  
        
        // Lower body taper
        {  H * 0.02f, maxW * 0.825f },  // 3.933" dia at 4.5"
        {  H * 0.04f, maxW * 0.800f },  
        {  H * 0.06f, maxW * 0.780f },  // 3.719" dia at 4"
        {  H * 0.08f, maxW * 0.765f },  
        {  H * 0.10f, maxW * 0.755f },  // 3.6" dia at 3.5"
        {  H * 0.12f, maxW * 0.750f },  
        {  H * 0.14f, maxW * 0.750f },  // 3.576" dia at 3"
        
        // Base begins to flare (around 2.5" from bottom)
        {  H * 0.16f, maxW * 0.755f },  
        {  H * 0.20f, maxW * 0.770f },  // 3.671" dia at 2.5"
        {  H * 0.24f, maxW * 0.795f },  
        {  H * 0.28f, maxW * 0.825f },  // 3.933" dia at 2"
        {  H * 0.32f, maxW * 0.855f },  
        {  H * 0.36f, maxW * 0.880f },  // 4.195" dia at 1.5"
        {  H * 0.40f, maxW * 0.900f },  
        {  H * 0.44f, maxW * 0.915f },  // 4.362" dia at 1"
        {  H * 0.48f, maxW * 0.925f },  
        {  H * 0.52f, maxW * 0.930f },  // 4.433" dia at 0.5"
        {  H * 0.56f, maxW * 0.930f },  
        {  H * 0.58f, maxW * 0.925f }   // bottom edge
    };

    const int n = (int)(sizeof(slices) / sizeof(slices[0]));
    
    // Create gradient using vertex array for realistic cylinder shading
    sf::VertexArray pinGradient(sf::PrimitiveType::TriangleStrip);
    
    for (int i = 0; i < n; i++) {
        float normalizedY = (slices[i].y + H * 0.58f) / (H * 1.16f); // 0 at top, 1 at bottom
        
        // Left side - darker (shadow side)
        sf::Vertex leftVert;
        leftVert.position = sf::Vector2f(-slices[i].halfW, slices[i].y);
        // Gradient from light gray to darker gray
        int grayValue = 160 + (int)(normalizedY * 40.0f);
        leftVert.color = sf::Color(grayValue, grayValue, grayValue);
        pinGradient.append(leftVert);
        
        // Right side - lighter (lit side)
        sf::Vertex rightVert;
        rightVert.position = sf::Vector2f(slices[i].halfW, slices[i].y);
        grayValue = 240 + (int)(normalizedY * 10.0f);
        rightVert.color = sf::Color(grayValue, grayValue, grayValue);
        pinGradient.append(rightVert);
    }
    
    window.draw(pinGradient, st);

    // Add cylinder edge highlights for 3D roundness
    // Left highlight (bright edge)
    sf::VertexArray leftHighlight(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i < n; i++) {
        float normalizedY = (slices[i].y + H * 0.58f) / (H * 1.16f);
        
        // Inner edge
        sf::Vertex v1;
        v1.position = sf::Vector2f(-slices[i].halfW * 0.85f, slices[i].y);
        v1.color = sf::Color(255, 255, 255, 120 - (int)(normalizedY * 40.0f));
        leftHighlight.append(v1);
        
        // Outer edge
        sf::Vertex v2;
        v2.position = sf::Vector2f(-slices[i].halfW, slices[i].y);
        v2.color = sf::Color(255, 255, 255, 0);
        leftHighlight.append(v2);
    }
    window.draw(leftHighlight, st);
    
    // Right edge darkening for depth
    sf::VertexArray rightDarkening(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i < n; i++) {
        // Inner
        sf::Vertex v1;
        v1.position = sf::Vector2f(slices[i].halfW * 0.85f, slices[i].y);
        v1.color = sf::Color(200, 200, 200, 0);
        rightDarkening.append(v1);
        
        // Outer edge
        sf::Vertex v2;
        v2.position = sf::Vector2f(slices[i].halfW, slices[i].y);
        v2.color = sf::Color(140, 140, 140, 60);
        rightDarkening.append(v2);
    }
    window.draw(rightDarkening, st);

    // Specular highlight stripe (glossy reflection)
    sf::VertexArray specular(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i < n; i++) {
        float normalizedY = (slices[i].y + H * 0.58f) / (H * 1.16f);
        
        // Narrow vertical highlight
        sf::Vertex v1;
        v1.position = sf::Vector2f(-slices[i].halfW * 0.45f, slices[i].y);
        int alpha = 140 - (int)(std::abs(normalizedY - 0.4f) * 200.0f);
        if (alpha < 0) alpha = 0;
        v1.color = sf::Color(255, 255, 255, alpha);
        specular.append(v1);
        
        sf::Vertex v2;
        v2.position = sf::Vector2f(-slices[i].halfW * 0.30f, slices[i].y);
        v2.color = sf::Color(255, 255, 255, alpha);
        specular.append(v2);
    }
    window.draw(specular, st);

    // --- Red stripes that wrap around the pin contour ---
    // Helper function to find width at a given Y position
    auto getWidthAtY = [&](float yPos) -> float {
        // Find the slice closest to this Y position
        for (int i = 0; i < n - 1; i++) {
            if (yPos >= slices[i].y && yPos <= slices[i + 1].y) {
                // Interpolate between the two slices
                float t = (yPos - slices[i].y) / (slices[i + 1].y - slices[i].y);
                return slices[i].halfW * (1.0f - t) + slices[i + 1].halfW * t;
            }
        }
        return maxW * 0.5f; // fallback
    };
    
    // Draw curved stripes that follow pin contour
    auto drawCurvedStripe = [&](float yCenter, float thickness) {
        sf::VertexArray stripe(sf::PrimitiveType::TriangleStrip);
        
        // Sample points along the stripe height
        int samples = 8;
        for (int i = 0; i <= samples; i++) {
            float t = (float)i / samples;
            float y = yCenter - thickness * 0.5f + thickness * t;
            float width = getWidthAtY(y);
            
            // Create gradient from dark on left to bright on right
            int leftRed = 160 + (int)(t * 20);
            int rightRed = 200 + (int)(t * 20);
            
            // Left vertex
            sf::Vertex vLeft;
            vLeft.position = sf::Vector2f(-width, y);
            vLeft.color = sf::Color(leftRed, 25, 25);
            stripe.append(vLeft);
            
            // Right vertex
            sf::Vertex vRight;
            vRight.position = sf::Vector2f(width, y);
            vRight.color = sf::Color(rightRed, 35, 35);
            stripe.append(vRight);
        }
        
        window.draw(stripe, st);
        
        // Add highlight on the stripe
        sf::VertexArray stripeHighlight(sf::PrimitiveType::TriangleStrip);
        for (int i = 0; i <= samples; i++) {
            float t = (float)i / samples;
            float y = yCenter - thickness * 0.4f + thickness * 0.8f * t;
            float width = getWidthAtY(y) * 0.35f;
            
            // Narrow highlight on left side
            sf::Vertex vLeft;
            vLeft.position = sf::Vector2f(-width - width * 0.8f, y);
            vLeft.color = sf::Color(255, 100, 100, 80);
            stripeHighlight.append(vLeft);
            
            sf::Vertex vRight;
            vRight.position = sf::Vector2f(-width - width * 0.2f, y);
            vRight.color = sf::Color(255, 100, 100, 80);
            stripeHighlight.append(vRight);
        }
        
        window.draw(stripeHighlight, st);
    };

    float stripeThick = radius * 0.30f;
    drawCurvedStripe(-H * 0.285f, stripeThick);  // neck stripe
    drawCurvedStripe(-H * 0.195f, stripeThick);  // upper body stripe

    // Base ring that wraps around the base contour
    float baseY = H * 0.51f;
    float baseThickness = radius * 0.18f;
    
    sf::VertexArray baseRing(sf::PrimitiveType::TriangleStrip);
    int baseSamples = 6;
    for (int i = 0; i <= baseSamples; i++) {
        float t = (float)i / baseSamples;
        float y = baseY - baseThickness * 0.5f + baseThickness * t;
        float width = getWidthAtY(y);
        
        // Gradient gold color
        int goldR = 190 + (int)(t * 20);
        int goldG = 170 + (int)(t * 20);
        
        // Left vertex
        sf::Vertex vLeft;
        vLeft.position = sf::Vector2f(-width, y);
        vLeft.color = sf::Color(goldR, goldG, 100);
        baseRing.append(vLeft);
        
        // Right vertex
        sf::Vertex vRight;
        vRight.position = sf::Vector2f(width, y);
        vRight.color = sf::Color(goldR + 20, goldG + 20, 120);
        baseRing.append(vRight);
    }
    
    window.draw(baseRing, st);
    
    // Highlight on base ring
    sf::VertexArray baseHighlight(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i <= baseSamples; i++) {
        float t = (float)i / baseSamples;
        float y = baseY - baseThickness * 0.4f + baseThickness * 0.8f * t;
        float width = getWidthAtY(y) * 0.30f;
        
        sf::Vertex vLeft;
        vLeft.position = sf::Vector2f(-width - width * 1.2f, y);
        vLeft.color = sf::Color(250, 235, 170, 100);
        baseHighlight.append(vLeft);
        
        sf::Vertex vRight;
        vRight.position = sf::Vector2f(-width - width * 0.3f, y);
        vRight.color = sf::Color(250, 235, 170, 100);
        baseHighlight.append(vRight);
    }
    
    window.draw(baseHighlight, st);
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