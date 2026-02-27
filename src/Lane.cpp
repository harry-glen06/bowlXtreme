#include "Lane.h"

void Lane::init(float windowW) {
    left = (windowW - width) / 2.0f;
    right = left + width;
    bottom = top + height;

    laneRect = sf::RectangleShape(sf::Vector2f(width, height));
    laneRect.setPosition(sf::Vector2f(left, top));
    laneRect.setFillColor(sf::Color(160, 120, 70));

    leftGutter = sf::RectangleShape(sf::Vector2f(gutterWidth, height));
    leftGutter.setPosition(sf::Vector2f(left, top));
    leftGutter.setFillColor(sf::Color(35, 35, 35));

    rightGutter = sf::RectangleShape(sf::Vector2f(gutterWidth, height));
    rightGutter.setPosition(sf::Vector2f(right - gutterWidth, top));
    rightGutter.setFillColor(sf::Color(35, 35, 35));

    leftBumper = sf::RectangleShape(sf::Vector2f(bumperThickness, height));
    leftBumper.setPosition(sf::Vector2f(left + gutterWidth, top));
    leftBumper.setFillColor(sf::Color::White);

    rightBumper = sf::RectangleShape(sf::Vector2f(bumperThickness, height));
    rightBumper.setPosition(sf::Vector2f(right - gutterWidth - bumperThickness, top));
    rightBumper.setFillColor(sf::Color::White);

    float endZoneSize = 20.0f;

    topEnd = sf::RectangleShape(sf::Vector2f(width, endZoneSize));
    topEnd.setPosition(sf::Vector2f(left, top - endZoneSize));
    topEnd.setFillColor(sf::Color::Black);

    bottomEnd = sf::RectangleShape(sf::Vector2f(width, endZoneSize));
    bottomEnd.setPosition(sf::Vector2f(left, bottom));
    bottomEnd.setFillColor(sf::Color::Black);
}

void Lane::draw(sf::RenderWindow& window) const {
    // Outer drop shadow
    sf::RectangleShape frameShadow(sf::Vector2f(width + 40.0f, height + 56.0f));
    frameShadow.setPosition(sf::Vector2f(left - 20.0f, top - 28.0f));
    frameShadow.setFillColor(sf::Color(0, 0, 0, 86));
    window.draw(frameShadow);

    // Metallic lane frame
    sf::VertexArray frame(sf::PrimitiveType::TriangleStrip, 4);
    frame[0].position = {left - 14.0f, top - 22.0f};
    frame[1].position = {right + 14.0f, top - 22.0f};
    frame[2].position = {left - 14.0f, bottom + 22.0f};
    frame[3].position = {right + 14.0f, bottom + 22.0f};
    frame[0].color = sf::Color(54, 59, 74);
    frame[1].color = sf::Color(60, 66, 82);
    frame[2].color = sf::Color(28, 32, 46);
    frame[3].color = sf::Color(24, 30, 44);
    window.draw(frame);

    // Frame caps at top/bottom
    sf::RectangleShape topCap(sf::Vector2f(width + 28.0f, 18.0f));
    topCap.setPosition(sf::Vector2f(left - 14.0f, top - 28.0f));
    topCap.setFillColor(sf::Color(18, 21, 30));
    window.draw(topCap);
    sf::RectangleShape bottomCap(sf::Vector2f(width + 28.0f, 18.0f));
    bottomCap.setPosition(sf::Vector2f(left - 14.0f, bottom + 10.0f));
    bottomCap.setFillColor(sf::Color(18, 21, 30));
    window.draw(bottomCap);

    // Lane wood base gradient
    sf::VertexArray woodBase(sf::PrimitiveType::TriangleStrip, 4);
    woodBase[0].position = {left, top};
    woodBase[1].position = {right, top};
    woodBase[2].position = {left, bottom};
    woodBase[3].position = {right, bottom};
    woodBase[0].color = sf::Color(186, 146, 90);
    woodBase[1].color = sf::Color(180, 140, 86);
    woodBase[2].color = sf::Color(148, 106, 62);
    woodBase[3].color = sf::Color(142, 102, 60);
    window.draw(woodBase);

    // Planks
    const int plankCount = 11;
    for (int i = 0; i < plankCount; i++) {
        float stripeW = width / static_cast<float>(plankCount);
        sf::RectangleShape plank(sf::Vector2f(stripeW, height));
        plank.setPosition(sf::Vector2f(left + i * stripeW, top));
        if (i % 2 == 0) plank.setFillColor(sf::Color(176, 132, 80, 96));
        else            plank.setFillColor(sf::Color(160, 118, 72, 96));
        window.draw(plank);
    }

    // Center polish glow
    sf::RectangleShape polish(sf::Vector2f(width * 0.16f, height));
    polish.setPosition(sf::Vector2f(left + width * 0.42f, top));
    polish.setFillColor(sf::Color(255, 245, 214, 20));
    window.draw(polish);
    sf::RectangleShape polishLine(sf::Vector2f(2.0f, height));
    polishLine.setPosition(sf::Vector2f(left + width * 0.50f, top));
    polishLine.setFillColor(sf::Color(255, 245, 214, 34));
    window.draw(polishLine);

    // Gutter gradients
    auto drawGutter = [&](float x) {
        sf::VertexArray gut(sf::PrimitiveType::TriangleStrip, 4);
        gut[0].position = {x, top};
        gut[1].position = {x + gutterWidth, top};
        gut[2].position = {x, bottom};
        gut[3].position = {x + gutterWidth, bottom};
        gut[0].color = sf::Color(46, 54, 78);
        gut[1].color = sf::Color(36, 43, 66);
        gut[2].color = sf::Color(20, 25, 40);
        gut[3].color = sf::Color(16, 20, 34);
        window.draw(gut);
    };
    drawGutter(left);
    drawGutter(right - gutterWidth);

    // Bumpers
    if (bumpersOn) {
        sf::RectangleShape leftB({bumperThickness, height});
        leftB.setPosition({left + gutterWidth, top});
        leftB.setFillColor(sf::Color(190, 248, 255, 225));
        leftB.setOutlineColor(sf::Color(78, 180, 220));
        leftB.setOutlineThickness(1.0f);
        window.draw(leftB);

        sf::RectangleShape rightB({bumperThickness, height});
        rightB.setPosition({right - gutterWidth - bumperThickness, top});
        rightB.setFillColor(sf::Color(190, 248, 255, 225));
        rightB.setOutlineColor(sf::Color(78, 180, 220));
        rightB.setOutlineThickness(1.0f);
        window.draw(rightB);
    }

    // Thin inner border line
    sf::RectangleShape inner(sf::Vector2f(width, height));
    inner.setPosition(sf::Vector2f(left, top));
    inner.setFillColor(sf::Color::Transparent);
    inner.setOutlineThickness(3.0f);
    inner.setOutlineColor(sf::Color(86, 96, 120));
    window.draw(inner);
}


float Lane::playLeft() const {
    return left + gutterWidth + bumperThickness;
}

float Lane::playRight() const {
    return right - gutterWidth - bumperThickness;
}

float Lane::centerX() const {
    return left + width * 0.5f;
}
