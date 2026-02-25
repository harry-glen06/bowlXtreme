#include "UI.h"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

// Helper functions
sf::ConvexShape createRoundedRect(sf::Vector2f size, float radius, sf::Color color, int pointsPerCorner = 10) {
    sf::ConvexShape shape;
    shape.setPointCount(pointsPerCorner * 4);
    shape.setFillColor(color);

    float width = size.x;
    float height = size.y;

    for (int i = 0; i < pointsPerCorner; i++) {
        float angle = i * 1.5708f / (pointsPerCorner - 1); // 0 to 90 degrees
        float x = width - radius + std::cos(angle) * radius;
        float y = height - radius + std::sin(angle) * radius;
        shape.setPoint(i, sf::Vector2f(x, y));
    }
    for (int i = 0; i < pointsPerCorner; i++) {
        float angle = 1.5708f + i * 1.5708f / (pointsPerCorner - 1); // 90 to 180
        float x = radius + std::cos(angle) * radius;
        float y = height - radius + std::sin(angle) * radius;
        shape.setPoint(pointsPerCorner + i, sf::Vector2f(x, y));
    }
    for (int i = 0; i < pointsPerCorner; i++) {
        float angle = 3.14159f + i * 1.5708f / (pointsPerCorner - 1); // 180 to 270
        float x = radius + std::cos(angle) * radius;
        float y = radius + std::sin(angle) * radius;
        shape.setPoint(pointsPerCorner * 2 + i, sf::Vector2f(x, y));
    }
    for (int i = 0; i < pointsPerCorner; i++) {
        float angle = 4.71239f + i * 1.5708f / (pointsPerCorner - 1); // 270 to 360
        float x = width - radius + std::cos(angle) * radius;
        float y = radius + std::sin(angle) * radius;
        shape.setPoint(pointsPerCorner * 3 + i, sf::Vector2f(x, y));
    }
    return shape;
}

static bool pointInRectPadded(const sf::FloatRect& rect, sf::Vector2f point, float pad = 0.0f) {
    return point.x >= rect.position.x - pad &&
           point.x <= rect.position.x + rect.size.x + pad &&
           point.y >= rect.position.y - pad &&
           point.y <= rect.position.y + rect.size.y + pad;
}

static bool pointInRectPadded(const sf::FloatRect& rect, sf::Vector2i point, float pad = 0.0f) {
    return pointInRectPadded(rect, sf::Vector2f((float)point.x, (float)point.y), pad);
}

static std::string formatCompactScore(int value);

namespace {
struct TutorialPageData {
    const char* title;
    std::array<const char*, 6> lines;
    int lineCount;
    sf::Color accent;
};

const std::array<TutorialPageData, 5> kTutorialPages = {{
    {
        "BASICS",
        {
            "Move left/right with A and D.",
            "Aim with Left and Right arrows.",
            "Press Space to launch the ball.",
            "Press M for menu, R to reset a run.",
            "",
            ""
        },
        4,
        sf::Color(80, 200, 220)
    },
    {
        "XTREME SCORING",
        {
            "Impact starts at 10, then adds hit pin values.",
            "Combo grows with pins hit on the shot.",
            "Shot score = Impact x Combo.",
            "Blue numbers on the left count up while scoring.",
            "",
            ""
        },
        4,
        sf::Color(80, 230, 255)
    },
    {
        "SHOP AND INVENTORY",
        {
            "Buy Balls, Pins, Shoes, and Powers with tokens.",
            "Balls are assigned per shot slot (Ball 1/2, and Ball 3 with Extra Ball).",
            "Pins are assigned by pin slot (P1..P10/12).",
            "Hover inventory items to see live status.",
            "",
            ""
        },
        4,
        sf::Color(110, 235, 160)
    },
    {
        "ROUND FLOW",
        {
            "Hit the target score before the round ends.",
            "After scoring finishes, a round summary appears.",
            "Use CONTINUE to move on to shop or game over.",
            "Reroll and sell are available in the shop.",
            "",
            ""
        },
        4,
        sf::Color(250, 210, 90)
    },
    {
        "POWER HOTKEYS",
        {
            "N: activate Duplicate charge and pick source/target.",
            "V: activate Swap charge and pick two pins.",
            "B and C work in Normal mode only.",
            "In Xtreme, Bumpers require power/settings rules.",
            "",
            ""
        },
        4,
        sf::Color(210, 175, 255)
    }
}};
} // namespace

UI::UI() {
    loadFont();
    loadPickRateStats();
}

void UI::loadFont() {
    fontLoaded = font.openFromFile("assets/arial.ttf");
}

bool UI::shouldTrackOfferCategory(ShopItemCategory category) {
    return category == ShopItemCategory::Ball ||
           category == ShopItemCategory::Pin ||
           category == ShopItemCategory::Power;
}

std::string UI::offerCategoryLabel(ShopItemCategory category) {
    switch (category) {
        case ShopItemCategory::Ball:  return "BALL";
        case ShopItemCategory::Pin:   return "PIN";
        case ShopItemCategory::Power: return "POWER";
        case ShopItemCategory::Shoe:  return "SHOE";
        default:                      return "ITEM";
    }
}

std::string UI::offerStatKey(const ShopOffer& offer) {
    return offerCategoryLabel(offer.category) + "|" + offer.name;
}

void UI::recordOfferShown(const ShopOffer& offer) {
    if (!shouldTrackOfferCategory(offer.category)) return;
    std::string key = offerStatKey(offer);
    PickRateEntry& entry = pickRateStats[key];
    if (entry.itemName.empty()) {
        entry.category = offerCategoryLabel(offer.category);
        entry.itemName = offer.name;
    }
    entry.shown++;
    savePickRateStats();
}

void UI::recordOfferPicked(const ShopOffer& offer) {
    if (!shouldTrackOfferCategory(offer.category)) return;
    std::string key = offerStatKey(offer);
    PickRateEntry& entry = pickRateStats[key];
    if (entry.itemName.empty()) {
        entry.category = offerCategoryLabel(offer.category);
        entry.itemName = offer.name;
    }
    entry.picked++;
    savePickRateStats();
}

void UI::loadPickRateStats() {
    pickRateStats.clear();
    std::ifstream in("pick_rates.csv");
    if (!in.is_open()) {
        savePickRateStats();
        return;
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (firstLine) {
            firstLine = false;
            if (line.rfind("category,item,shown,picked,pick_rate_percent", 0) == 0) {
                continue;
            }
        }

        std::stringstream ss(line);
        std::string category, itemName, shownStr, pickedStr;
        if (!std::getline(ss, category, ',')) continue;
        if (!std::getline(ss, itemName, ',')) continue;
        if (!std::getline(ss, shownStr, ',')) continue;
        if (!std::getline(ss, pickedStr, ',')) continue;

        PickRateEntry entry;
        entry.category = category;
        entry.itemName = itemName;
        entry.shown = std::max(0, std::atoi(shownStr.c_str()));
        entry.picked = std::max(0, std::atoi(pickedStr.c_str()));

        std::string key = category + "|" + itemName;
        pickRateStats[key] = entry;
    }
}

void UI::savePickRateStats() const {
    std::ofstream out("pick_rates.csv");
    if (!out.is_open()) return;

    out << "category,item,shown,picked,pick_rate_percent\n";
    for (const auto& kv : pickRateStats) {
        const PickRateEntry& entry = kv.second;
        double pct = (entry.shown > 0)
            ? (100.0 * static_cast<double>(entry.picked) / static_cast<double>(entry.shown))
            : 0.0;
        out << entry.category << ","
            << entry.itemName << ","
            << entry.shown << ","
            << entry.picked << ","
            << std::fixed << std::setprecision(2) << pct << "\n";
    }
}

void UI::initClouds(float windowW, float windowH) {
    clouds.clear();

    // Stratified cloud field keeps the full sky populated, including far right.
    const int cloudCount = 18;
    const float span = windowW + 360.0f;
    for (int i = 0; i < cloudCount; i++) {
        Cloud cloud;
        float t = (cloudCount > 0) ? (static_cast<float>(i) / static_cast<float>(cloudCount)) : 0.0f;
        float jitter = static_cast<float>((rand() % 57) - 28); // +/-28
        float x = -180.0f + t * span + jitter;
        cloud.position = sf::Vector2f(
            x,
            35.0f + static_cast<float>(rand() % std::max(120, static_cast<int>(windowH * 0.58f)))
        );
        cloud.speed = 10.0f + static_cast<float>(rand() % 26);  // 10-35 px/s
        cloud.size = 65.0f + static_cast<float>(rand() % 120);  // 65-185 size
        clouds.push_back(cloud);
    }
}

void UI::updateClouds(float dt, float windowW, float windowH) {
    const float wrapSpan = windowW + 360.0f;
    for (auto& cloud : clouds) {
        cloud.position.x += cloud.speed * dt;

        // Deterministic wrapping preserves cloud density across the screen.
        const float maxX = windowW + cloud.size * 1.6f;
        const float minX = -cloud.size * 2.2f;
        while (cloud.position.x > maxX) {
            cloud.position.x -= wrapSpan;
        }
        while (cloud.position.x < minX) {
            cloud.position.x += wrapSpan;
        }
    }
}

void UI::drawClouds(sf::RenderWindow& window) {
    const float wrapX = window.getView().getSize().x + 360.0f;
    for (const auto& cloud : clouds) {
        float y = cloud.position.y;
        float s = cloud.size;
        std::uint8_t alphaMain = static_cast<std::uint8_t>(
            std::clamp(118 + static_cast<int>((s - 65.0f) * 0.7f), 118, 208));
        std::uint8_t alphaSoft = static_cast<std::uint8_t>(std::max(60, (int)alphaMain - 52));

        auto drawPuff = [&](float baseX, float dx, float dy, float r, sf::Color c, float scaleY) {
            sf::CircleShape puff(r);
            puff.setOrigin({r, r});
            puff.setPosition({baseX + dx, y + dy});
            puff.setScale({1.28f, scaleY});
            puff.setFillColor(c);
            window.draw(puff);
        };

        auto drawCloudAt = [&](float baseX) {
            // Lower cool shadow gives depth against the sky.
            drawPuff(baseX, 0.0f,       s * 0.15f, s * 0.42f, sf::Color(108, 130, 170, alphaSoft), 0.78f);
            drawPuff(baseX, s * 0.44f,  s * 0.17f, s * 0.34f, sf::Color(108, 130, 170, alphaSoft), 0.78f);
            drawPuff(baseX, -s * 0.36f, s * 0.19f, s * 0.29f, sf::Color(108, 130, 170, alphaSoft), 0.78f);

            // Main fluffy body.
            drawPuff(baseX, -s * 0.44f, s * 0.06f, s * 0.40f, sf::Color(255, 255, 255, alphaMain), 0.84f);
            drawPuff(baseX, -s * 0.08f, -s * 0.02f, s * 0.48f, sf::Color(255, 255, 255, alphaMain), 0.86f);
            drawPuff(baseX,  s * 0.34f,  0.0f,      s * 0.42f, sf::Color(255, 255, 255, alphaMain), 0.84f);
            drawPuff(baseX,  s * 0.70f,  s * 0.07f, s * 0.30f, sf::Color(255, 255, 255, alphaMain), 0.80f);

            // Soft top highlight.
            drawPuff(baseX, -s * 0.15f, -s * 0.12f, s * 0.30f, sf::Color(255, 255, 255, 96), 0.72f);
        };

        drawCloudAt(cloud.position.x);
        drawCloudAt(cloud.position.x - wrapX);
        drawCloudAt(cloud.position.x + wrapX);
    }
}

namespace {
struct MenuLayout {
    float blockWidth = 0.0f;
    float blockHeight = 0.0f;
    float logoX = 0.0f;
    float logoY = 0.0f;
    float signW = 0.0f;
    float signH = 0.0f;
    float signX = 0.0f;
    float signY = 0.0f;
    float poleTop = 0.0f;
    float poleBottom = 0.0f;
    float poleW = 0.0f;
    float poleGap = 0.0f;
    float buttonWidth = 0.0f;
    float buttonHeight = 0.0f;
    float buttonSpacing = 0.0f;
    float buttonStartX = 0.0f;
    float buttonY = 0.0f;
    sf::FloatRect buttonContainerRect{};
    std::array<sf::FloatRect, 4> buttonRects{};
};

MenuLayout buildMenuLayout(float windowW, float windowH) {
    MenuLayout layout;

    layout.blockWidth = std::clamp(windowW * 0.082f, 72.0f, 120.0f);
    layout.blockHeight = std::clamp(windowH * 0.205f, 118.0f, 174.0f);
    layout.logoX = windowW * 0.5f - layout.blockWidth * 2.0f;
    layout.logoY = std::clamp(windowH * 0.098f, 32.0f, 110.0f);

    layout.signW = layout.blockWidth * 4.28f;
    layout.signH = std::clamp(windowH * 0.118f, 72.0f, 102.0f);
    layout.signX = windowW * 0.5f - layout.signW * 0.5f;
    layout.signY = layout.logoY + layout.blockHeight + std::clamp(windowH * 0.018f, 12.0f, 22.0f);

    layout.buttonHeight = std::clamp(windowH * 0.085f, 54.0f, 72.0f);
    layout.buttonSpacing = std::clamp(windowW * 0.012f, 10.0f, 20.0f);
    float sidePadding = std::clamp(windowW * 0.07f, 24.0f, 96.0f);
    float availableRowWidth = std::max(360.0f, windowW - sidePadding * 2.0f);
    float targetRowWidth = std::min(820.0f, availableRowWidth);
    layout.buttonWidth = std::clamp((targetRowWidth - layout.buttonSpacing * 3.0f) / 4.0f, 86.0f, 190.0f);
    float usedRowWidth = layout.buttonWidth * 4.0f + layout.buttonSpacing * 3.0f;
    layout.buttonStartX = windowW * 0.5f - usedRowWidth * 0.5f;
    layout.buttonStartX = std::max(10.0f, layout.buttonStartX);
    if (layout.buttonStartX + usedRowWidth > windowW - 10.0f) {
        layout.buttonStartX = windowW - 10.0f - usedRowWidth;
    }
    // Keep a balanced and predictable title-to-buttons spacing.
    float signBottom = layout.signY + layout.signH;
    float topGapMin = std::clamp(windowH * 0.07f, 56.0f, 92.0f);
    float bottomMargin = std::clamp(windowH * 0.10f, 72.0f, 124.0f);
    float containerH = layout.buttonHeight + 36.0f;
    float minContainerY = signBottom + topGapMin;
    float maxContainerY = windowH - bottomMargin - containerH;
    if (maxContainerY < minContainerY) {
        maxContainerY = minContainerY;
    }

    // Aim for a visually centered placement in the space below the title sign.
    float freeSpace = std::max(0.0f, maxContainerY - minContainerY);
    float targetContainerY = minContainerY + freeSpace * 0.52f;
    float containerY = std::clamp(targetContainerY, minContainerY, maxContainerY);
    layout.buttonY = containerY + 18.0f;

    layout.buttonContainerRect = sf::FloatRect(
        sf::Vector2f(layout.buttonStartX - 24.0f, containerY),
        sf::Vector2f(usedRowWidth + 48.0f, containerH));

    for (int i = 0; i < 4; i++) {
        layout.buttonRects[(size_t)i] = sf::FloatRect(
            sf::Vector2f(layout.buttonStartX + (layout.buttonWidth + layout.buttonSpacing) * static_cast<float>(i),
                         layout.buttonY),
            sf::Vector2f(layout.buttonWidth, layout.buttonHeight));
    }

    layout.poleTop = layout.logoY + layout.blockHeight * 0.95f;
    layout.poleBottom = layout.buttonContainerRect.position.y - 12.0f;
    if (layout.poleBottom < layout.poleTop + 70.0f) {
        layout.poleBottom = layout.poleTop + 70.0f;
    }
    layout.poleW = std::clamp(layout.blockWidth * 0.24f, 18.0f, 30.0f);
    layout.poleGap = std::clamp(layout.blockWidth * 0.18f, 8.0f, 16.0f);

    return layout;
}
} // namespace

void UI::drawMenu(sf::RenderWindow& window, float windowW, float windowH, float dt) {
    if (!fontLoaded) return;

    // Initialize clouds if needed
    if (clouds.empty()) {
        initClouds(windowW, windowH);
    }

    // Update and draw animated clouds
    updateClouds(dt, windowW, windowH);

    MenuLayout layout = buildMenuLayout(windowW, windowH);

    // Sky base gradient.
    sf::VertexArray sky(sf::PrimitiveType::TriangleStrip, 4);
    sky[0].position = {0.f, 0.f};
    sky[1].position = {windowW, 0.f};
    sky[2].position = {0.f, windowH};
    sky[3].position = {windowW, windowH};
    sky[0].color = sf::Color(58, 90, 197);
    sky[1].color = sf::Color(68, 101, 206);
    sky[2].color = sf::Color(128, 176, 242);
    sky[3].color = sf::Color(112, 171, 244);
    window.draw(sky);

    sf::VertexArray skyGlow(sf::PrimitiveType::TriangleStrip, 4);
    skyGlow[0].position = {0.f, windowH * 0.30f};
    skyGlow[1].position = {windowW, windowH * 0.30f};
    skyGlow[2].position = {0.f, windowH};
    skyGlow[3].position = {windowW, windowH};
    skyGlow[0].color = sf::Color(255, 255, 255, 30);
    skyGlow[1].color = sf::Color(255, 255, 255, 18);
    skyGlow[2].color = sf::Color(255, 255, 255, 90);
    skyGlow[3].color = sf::Color(255, 255, 255, 74);
    window.draw(skyGlow);

    drawClouds(window);

    auto tint = [](sf::Color c, int delta) {
        auto shift = [delta](std::uint8_t v) -> std::uint8_t {
            return static_cast<std::uint8_t>(std::clamp((int)v + delta, 0, 255));
        };
        return sf::Color(shift(c.r), shift(c.g), shift(c.b), c.a);
    };

    auto drawLogoBlock = [&](float x, float y, sf::Color fill, char letter, sf::Color textColor,
                             sf::Color outlineColor, float letterScale) {
        sf::RectangleShape shadow({layout.blockWidth, layout.blockHeight});
        shadow.setPosition({x + 4.0f, y + 5.0f});
        shadow.setFillColor(sf::Color(0, 0, 0, 70));
        window.draw(shadow);

        sf::RectangleShape block({layout.blockWidth, layout.blockHeight});
        block.setPosition({x, y});
        block.setFillColor(fill);
        block.setOutlineColor(sf::Color::Black);
        block.setOutlineThickness(2.8f);
        window.draw(block);

        unsigned int letterSize = static_cast<unsigned int>(layout.blockHeight * 0.60f * letterScale);
        sf::Text txt(font, std::string(1, letter), letterSize);
        txt.setStyle(sf::Text::Bold);
        sf::FloatRect b = txt.getLocalBounds();
        txt.setPosition({
            x + layout.blockWidth * 0.5f - (b.position.x + b.size.x * 0.5f),
            y + layout.blockHeight * 0.5f - (b.position.y + b.size.y * 0.52f)
        });
        txt.setFillColor(textColor);
        txt.setOutlineColor(outlineColor);
        txt.setOutlineThickness(2.2f);
        window.draw(txt);
    };

    drawLogoBlock(layout.logoX, layout.logoY + 2.0f, sf::Color(245, 44, 50), 'B',
                  sf::Color::White, sf::Color::Black, 1.0f);
    drawLogoBlock(layout.logoX + layout.blockWidth, layout.logoY - 12.0f, sf::Color(96, 208, 238), 'O',
                  sf::Color::White, sf::Color::Black, 1.0f);
    drawLogoBlock(layout.logoX + layout.blockWidth * 2.0f, layout.logoY + 9.0f, sf::Color(244, 243, 78), 'W',
                  sf::Color::Black, sf::Color::White, 0.86f);
    drawLogoBlock(layout.logoX + layout.blockWidth * 3.0f, layout.logoY - 5.0f, sf::Color(98, 202, 230), 'L',
                  sf::Color::White, sf::Color::Black, 1.0f);

    sf::RectangleShape signShadow({layout.signW, layout.signH});
    signShadow.setPosition({layout.signX + 5.0f, layout.signY + 6.0f});
    signShadow.setFillColor(sf::Color(0, 0, 0, 75));
    window.draw(signShadow);

    sf::ConvexShape signBox = createRoundedRect({layout.signW, layout.signH}, 10.0f, sf::Color(250, 247, 241));
    signBox.setPosition({layout.signX, layout.signY});
    signBox.setFillColor(sf::Color(250, 247, 241));
    signBox.setOutlineColor(sf::Color(230, 50, 50));
    signBox.setOutlineThickness(4.0f);
    window.draw(signBox);

    sf::Text xtremeText(font, "Xtreme", static_cast<unsigned int>(layout.signH * 0.74f));
    xtremeText.setLetterSpacing(1.35f);
    xtremeText.setStyle(sf::Text::Bold);
    sf::FloatRect xtb = xtremeText.getLocalBounds();
    xtremeText.setPosition({
        layout.signX + layout.signW * 0.5f - (xtb.position.x + xtb.size.x * 0.5f),
        layout.signY + layout.signH * 0.5f - (xtb.position.y + xtb.size.y * 0.58f)
    });
    xtremeText.setFillColor(sf::Color(230, 40, 42));
    xtremeText.setOutlineColor(sf::Color::Black);
    xtremeText.setOutlineThickness(1.5f);
    window.draw(xtremeText);

    sf::ConvexShape buttonContainer = createRoundedRect(layout.buttonContainerRect.size, 24.0f, sf::Color(96, 102, 112, 210));
    buttonContainer.setPosition(layout.buttonContainerRect.position);
    buttonContainer.setFillColor(sf::Color(96, 102, 112, 210));
    buttonContainer.setOutlineColor(sf::Color(210, 214, 226, 160));
    buttonContainer.setOutlineThickness(2.0f);
    window.draw(buttonContainer);

    sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    auto drawMenuButton = [&](int index,
                              const std::string& label,
                              sf::Color fill,
                              sf::Color textColor,
                              unsigned textSize) {
        const sf::FloatRect& rect = layout.buttonRects[(size_t)index];
        float x = rect.position.x;
        bool hovered = pointInRectPadded(rect, mouseWorld, 4.0f);

        sf::ConvexShape shadow = createRoundedRect(rect.size, 15.0f, sf::Color(0, 0, 0, 78));
        shadow.setPosition({x + 2.0f, rect.position.y + 4.0f});
        window.draw(shadow);

        sf::Color borderColor = hovered ? tint(fill, -18) : tint(fill, -38);
        sf::ConvexShape border = createRoundedRect(rect.size, 15.0f, borderColor);
        border.setPosition({x, rect.position.y});
        window.draw(border);

        sf::Color faceColor = hovered ? tint(fill, 16) : fill;
        sf::ConvexShape face = createRoundedRect({rect.size.x - 4.0f, rect.size.y - 4.0f}, 13.0f, faceColor);
        face.setPosition({x + 2.0f, rect.position.y + 2.0f});
        window.draw(face);

        sf::ConvexShape highlight = createRoundedRect({rect.size.x - 8.0f, (rect.size.y - 4.0f) * 0.45f}, 11.0f,
                                                      sf::Color(255, 255, 255, hovered ? 62 : 46));
        highlight.setPosition({x + 4.0f, rect.position.y + 4.0f});
        window.draw(highlight);

        sf::Text text(font, label, textSize);
        while (text.getCharacterSize() > 20 &&
               text.getLocalBounds().size.x > rect.size.x - 22.0f) {
            text.setCharacterSize(text.getCharacterSize() - 1);
        }
        sf::FloatRect tb = text.getLocalBounds();
        text.setPosition({
            x + rect.size.x * 0.5f - (tb.position.x + tb.size.x * 0.5f),
            rect.position.y + rect.size.y * 0.5f - (tb.position.y + tb.size.y * 0.54f)
        });
        text.setFillColor(textColor);
        text.setOutlineColor(hovered ? sf::Color::Black : sf::Color(0, 0, 0, 180));
        text.setOutlineThickness(hovered ? 1.6f : 1.2f);
        window.draw(text);
    };

    drawMenuButton(0, "Normal", sf::Color(235, 48, 56), sf::Color::White, 33);
    drawMenuButton(1, "Xtreme", sf::Color(92, 212, 240), sf::Color::White, 33);
    drawMenuButton(2, "Tutorial", sf::Color(110, 156, 255), sf::Color::White, 31);
    drawMenuButton(3, "Settings", sf::Color(248, 232, 64), sf::Color::Black, 30);
}

MenuButton UI::handleMenuClick(sf::RenderWindow& window, sf::Vector2i mousePos) {
    sf::View v = window.getView();
    float windowW = v.getSize().x;
    float windowH = v.getSize().y;
    MenuLayout layout = buildMenuLayout(windowW, windowH);
    const float clickPad = 10.0f;

    if (pointInRectPadded(layout.buttonRects[0], mousePos, clickPad)) {
        return MenuButton::Normal;
    }

    if (pointInRectPadded(layout.buttonRects[1], mousePos, clickPad)) {
        return MenuButton::Xtreme;
    }

    if (pointInRectPadded(layout.buttonRects[2], mousePos, clickPad)) {
        return MenuButton::Tutorial;
    }

    if (pointInRectPadded(layout.buttonRects[3], mousePos, clickPad)) {
        return MenuButton::Settings;
    }
    
    return MenuButton::None;
}

void UI::drawTutorial(sf::RenderWindow& window, float windowW, float windowH) {
    if (!fontLoaded) return;

    if (tutorialPage < 0) tutorialPage = 0;
    if (tutorialPage >= (int)kTutorialPages.size()) tutorialPage = (int)kTutorialPages.size() - 1;
    const TutorialPageData& page = kTutorialPages[(size_t)tutorialPage];

    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(22, 44, 90, 130));
    window.draw(overlay);

    float panelW = std::min(900.0f, windowW * 0.84f);
    float panelH = std::min(680.0f, windowH * 0.82f);
    float panelX = windowW * 0.5f - panelW * 0.5f;
    float panelY = windowH * 0.5f - panelH * 0.5f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(sf::Vector2f(panelX, panelY));
    panel.setFillColor(sf::Color(229, 239, 252, 246));
    panel.setOutlineColor(sf::Color(98, 130, 198));
    panel.setOutlineThickness(3.0f);
    window.draw(panel);

    sf::RectangleShape accentBar(sf::Vector2f(panelW, 8.0f));
    accentBar.setPosition(sf::Vector2f(panelX, panelY));
    accentBar.setFillColor(page.accent);
    window.draw(accentBar);

    sf::Text header(font, "TUTORIAL", 56);
    header.setPosition(sf::Vector2f(panelX + 42.0f, panelY + 30.0f));
    header.setFillColor(sf::Color(19, 39, 82));
    window.draw(header);

    sf::Text pageIndex(
        font,
        "Page " + std::to_string(tutorialPage + 1) + "/" + std::to_string((int)kTutorialPages.size()),
        26);
    sf::FloatRect pageBounds = pageIndex.getLocalBounds();
    pageIndex.setPosition(sf::Vector2f(panelX + panelW - pageBounds.size.x - 46.0f, panelY + 44.0f));
    pageIndex.setFillColor(sf::Color(59, 93, 157));
    window.draw(pageIndex);

    sf::Text title(font, page.title, 40);
    title.setPosition(sf::Vector2f(panelX + 42.0f, panelY + 120.0f));
    title.setFillColor(page.accent);
    window.draw(title);

    float lineY = panelY + 200.0f;
    for (int i = 0; i < page.lineCount; i++) {
        sf::Text line(font, std::string("- ") + page.lines[(size_t)i], 30);
        line.setPosition(sf::Vector2f(panelX + 50.0f, lineY));
        line.setFillColor(sf::Color(33, 58, 106));
        window.draw(line);
        lineY += 62.0f;
    }

    sf::Text helper(font, "Read each page, then press DONE.", 24);
    helper.setPosition(sf::Vector2f(panelX + 50.0f, panelY + panelH - 170.0f));
    helper.setFillColor(sf::Color(62, 92, 148));
    window.draw(helper);

    float buttonY = panelY + panelH - 108.0f;
    sf::FloatRect menuRect(sf::Vector2f(panelX + 40.0f, buttonY), sf::Vector2f(170.0f, 56.0f));
    sf::FloatRect backRect(sf::Vector2f(panelX + panelW * 0.5f - 85.0f, buttonY), sf::Vector2f(170.0f, 56.0f));
    sf::FloatRect nextRect(sf::Vector2f(panelX + panelW - 210.0f, buttonY), sf::Vector2f(170.0f, 56.0f));

    sf::RectangleShape menuButton(menuRect.size);
    menuButton.setPosition(menuRect.position);
    menuButton.setFillColor(sf::Color(245, 65, 91));
    menuButton.setOutlineColor(sf::Color(132, 28, 45));
    menuButton.setOutlineThickness(2.0f);
    window.draw(menuButton);

    sf::RectangleShape backButton(backRect.size);
    backButton.setPosition(backRect.position);
    backButton.setFillColor(tutorialPage > 0 ? sf::Color(95, 216, 241) : sf::Color(175, 195, 229));
    backButton.setOutlineColor(sf::Color(67, 111, 194));
    backButton.setOutlineThickness(2.0f);
    window.draw(backButton);

    sf::RectangleShape nextButton(nextRect.size);
    nextButton.setPosition(nextRect.position);
    nextButton.setFillColor(tutorialPage + 1 < (int)kTutorialPages.size()
        ? sf::Color(245, 228, 70)
        : sf::Color(101, 236, 162));
    nextButton.setOutlineColor(sf::Color(93, 124, 187));
    nextButton.setOutlineThickness(2.0f);
    window.draw(nextButton);

    auto drawCenteredButtonText = [&](const sf::FloatRect& rect, const std::string& text, unsigned size) {
        sf::Text label(font, text, size);
        sf::FloatRect b = label.getLocalBounds();
        label.setPosition(sf::Vector2f(
            rect.position.x + rect.size.x * 0.5f - (b.position.x + b.size.x * 0.5f),
            rect.position.y + rect.size.y * 0.5f - (b.position.y + b.size.y * 0.5f) - 1.0f));
        label.setFillColor(sf::Color(20, 40, 82));
        window.draw(label);
    };
    drawCenteredButtonText(menuRect, "MENU", 30);
    drawCenteredButtonText(backRect, "BACK", 30);
    drawCenteredButtonText(nextRect, (tutorialPage + 1 < (int)kTutorialPages.size()) ? "NEXT" : "DONE", 30);
}

bool UI::handleTutorialClick(sf::RenderWindow& window, sf::Vector2i mousePos) {
    sf::View v = window.getView();
    float windowW = v.getSize().x;
    float windowH = v.getSize().y;

    if (tutorialPage < 0) tutorialPage = 0;
    if (tutorialPage >= (int)kTutorialPages.size()) tutorialPage = (int)kTutorialPages.size() - 1;

    float panelW = std::min(900.0f, windowW * 0.84f);
    float panelH = std::min(680.0f, windowH * 0.82f);
    float panelX = windowW * 0.5f - panelW * 0.5f;
    float panelY = windowH * 0.5f - panelH * 0.5f;

    float buttonY = panelY + panelH - 108.0f;
    sf::FloatRect menuRect(sf::Vector2f(panelX + 40.0f, buttonY), sf::Vector2f(170.0f, 56.0f));
    sf::FloatRect backRect(sf::Vector2f(panelX + panelW * 0.5f - 85.0f, buttonY), sf::Vector2f(170.0f, 56.0f));
    sf::FloatRect nextRect(sf::Vector2f(panelX + panelW - 210.0f, buttonY), sf::Vector2f(170.0f, 56.0f));
    const float clickPad = 12.0f;

    if (pointInRectPadded(menuRect, mousePos, clickPad)) {
        tutorialPage = 0;
        return true;
    }

    if (pointInRectPadded(backRect, mousePos, clickPad) && tutorialPage > 0) {
        tutorialPage--;
        return false;
    }

    if (pointInRectPadded(nextRect, mousePos, clickPad)) {
        if (tutorialPage + 1 < (int)kTutorialPages.size()) {
            tutorialPage++;
            return false;
        }
        tutorialPage = 0;
        return true;
    }

    return false;
}

void UI::drawSettings(sf::RenderWindow& window, float windowW, float windowH) {
    if (!fontLoaded) return;
    
    // Semi-transparent overlay
    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(22, 44, 90, 132));
    window.draw(overlay);
    
    // Settings panel
    float panelW = 600;
    float panelH = 500;
    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(sf::Vector2f(windowW / 2 - panelW / 2, windowH / 2 - panelH / 2));
    panel.setFillColor(sf::Color(229, 239, 252, 246));
    panel.setOutlineColor(sf::Color(98, 130, 198));
    panel.setOutlineThickness(3.0f);
    window.draw(panel);
    
    float startX = windowW / 2 - panelW / 2 + 40;
    float startY = windowH / 2 - panelH / 2 + 40;
    
    // Title
    sf::Text title(font, "Settings", 50);
    title.setPosition(sf::Vector2f(startX, startY));
    title.setFillColor(sf::Color(19, 39, 82));
    window.draw(title);
    
    // Music Volume
    sf::Text musicLabel(font, "Music Volume", 30);
    musicLabel.setPosition(sf::Vector2f(startX, startY + 80));
    musicLabel.setFillColor(sf::Color(25, 46, 94));
    window.draw(musicLabel);
    
    // Music slider
    sf::RectangleShape musicSliderBg(sf::Vector2f(400, 10));
    musicSliderBg.setPosition(sf::Vector2f(startX, startY + 120));
    musicSliderBg.setFillColor(sf::Color(169, 194, 238));
    window.draw(musicSliderBg);
    
    sf::RectangleShape musicSliderFill(sf::Vector2f(400 * (musicVolume / 100.0f), 10));
    musicSliderFill.setPosition(sf::Vector2f(startX, startY + 120));
    musicSliderFill.setFillColor(sf::Color(95, 216, 241));
    window.draw(musicSliderFill);
    
    sf::Text musicValue(font, std::to_string((int)musicVolume), 25);
    musicValue.setPosition(sf::Vector2f(startX + 420, startY + 110));
    musicValue.setFillColor(sf::Color(24, 46, 94));
    window.draw(musicValue);
    
    // Sound Effects Volume
    sf::Text soundLabel(font, "Sound Volume", 30);
    soundLabel.setPosition(sf::Vector2f(startX, startY + 160));
    soundLabel.setFillColor(sf::Color(25, 46, 94));
    window.draw(soundLabel);
    
    // Sound slider
    sf::RectangleShape soundSliderBg(sf::Vector2f(400, 10));
    soundSliderBg.setPosition(sf::Vector2f(startX, startY + 200));
    soundSliderBg.setFillColor(sf::Color(169, 194, 238));
    window.draw(soundSliderBg);
    
    sf::RectangleShape soundSliderFill(sf::Vector2f(400 * (soundVolume / 100.0f), 10));
    soundSliderFill.setPosition(sf::Vector2f(startX, startY + 200));
    soundSliderFill.setFillColor(sf::Color(95, 216, 241));
    window.draw(soundSliderFill);
    
    sf::Text soundValue(font, std::to_string((int)soundVolume), 25);
    soundValue.setPosition(sf::Vector2f(startX + 420, startY + 190));
    soundValue.setFillColor(sf::Color(24, 46, 94));
    window.draw(soundValue);
    
    // Bumpers
    sf::Text bumpersLabel(font, "Bumpers", 30);
    bumpersLabel.setPosition(sf::Vector2f(startX, startY + 250));
    bumpersLabel.setFillColor(sf::Color(25, 46, 94));
    window.draw(bumpersLabel);
    
    // Bumpers checkbox
    sf::RectangleShape checkbox(sf::Vector2f(30, 30));
    checkbox.setPosition(sf::Vector2f(startX + 200, startY + 250));
    checkbox.setFillColor(bumpersDefault ? sf::Color(101, 236, 162) : sf::Color(173, 194, 226));
    checkbox.setOutlineColor(sf::Color(84, 117, 183));
    checkbox.setOutlineThickness(2.0f);
    window.draw(checkbox);
    
    sf::Text checkboxLabel(font, bumpersDefault ? "ON" : "OFF", 25);
    checkboxLabel.setPosition(sf::Vector2f(startX + 250, startY + 252));
    checkboxLabel.setFillColor(sf::Color(25, 46, 94));
    window.draw(checkboxLabel);
    
    // Back button
    sf::RectangleShape backButton(sf::Vector2f(200, 50));
    backButton.setPosition(sf::Vector2f(windowW / 2 - 100, startY + 350));
    backButton.setFillColor(sf::Color(245, 65, 91));
    backButton.setOutlineColor(sf::Color(138, 35, 52));
    backButton.setOutlineThickness(2.f);
    window.draw(backButton);
    
    sf::Text backText(font, "Back", 30);
    backText.setPosition(sf::Vector2f(windowW / 2 - 40, startY + 358));
    backText.setFillColor(sf::Color(248, 248, 252));
    window.draw(backText);
}

void UI::handleSettingsClick(sf::RenderWindow& window, sf::Vector2i mousePos) {
    sf::View v = window.getView();
    float windowW = v.getSize().x;
    float windowH = v.getSize().y;
    
    float panelW = 600;
    float panelH = 500;
    float startX = windowW / 2 - panelW / 2 + 40;
    float startY = windowH / 2 - panelH / 2 + 40;
    
    const float clickPad = 10.0f;
    sf::FloatRect musicRect(sf::Vector2f(startX, startY + 114), sf::Vector2f(400, 18));
    sf::FloatRect soundRect(sf::Vector2f(startX, startY + 194), sf::Vector2f(400, 18));
    sf::FloatRect bumpersRect(sf::Vector2f(startX + 200, startY + 250), sf::Vector2f(30, 30));
    sf::FloatRect backRect(sf::Vector2f(windowW / 2 - 100, startY + 350), sf::Vector2f(200, 50));

    // Music slider click
    if (pointInRectPadded(musicRect, mousePos, clickPad)) {
        float t = (mousePos.x - startX) / 400.0f;
        musicVolume = std::clamp(t * 100.0f, 0.0f, 100.0f);
    }

    // Sound slider click
    if (pointInRectPadded(soundRect, mousePos, clickPad)) {
        float t = (mousePos.x - startX) / 400.0f;
        soundVolume = std::clamp(t * 100.0f, 0.0f, 100.0f);
    }

    // Check bumpers checkbox
    if (pointInRectPadded(bumpersRect, mousePos, clickPad)) {
        bumpersDefault = !bumpersDefault;
    }
    
    // Check back button
    if (pointInRectPadded(backRect, mousePos, clickPad)) {
        state = GameState::Menu;
    }
    
    // TODO: Handle slider dragging (implement in next version if needed)
}

GameAction UI::drawScorecard(sf::RenderWindow& window, 
                       const std::array<FrameScore, 10>& frames,
                       int currentFrame, 
                       int currentBall, 
                       int normalHighScore,
                       float windowW, 
                       float windowH) {
    if (!fontLoaded) return GameAction::None;

    const float panelX = 14.0f;
    const float panelY = 24.0f;
    const float panelW = 264.0f;
    const float panelH = windowH - 48.0f;

    sf::ConvexShape panelShadow = createRoundedRect({panelW + 8.0f, panelH + 8.0f}, 16.0f, sf::Color(0, 0, 0, 58));
    panelShadow.setPosition({panelX + 4.0f, panelY + 6.0f});
    window.draw(panelShadow);

    sf::ConvexShape panel = createRoundedRect({panelW, panelH}, 16.0f, sf::Color(223, 234, 251, 242));
    panel.setPosition({panelX, panelY});
    panel.setOutlineColor(sf::Color(92, 126, 196));
    panel.setOutlineThickness(2.6f);
    window.draw(panel);

    sf::RectangleShape headBar({panelW - 8.0f, 54.0f});
    headBar.setPosition({panelX + 4.0f, panelY + 4.0f});
    headBar.setFillColor(sf::Color(105, 153, 231, 242));
    window.draw(headBar);

    sf::Text title(font, "CLASSIC MODE", 25);
    title.setPosition({panelX + 16.0f, panelY + 12.0f});
    title.setFillColor(sf::Color(19, 39, 82));
    window.draw(title);

    int shownFrame = std::clamp(currentFrame + 1, 1, 10);
    sf::Text frameInfo(font, "Frame " + std::to_string(shownFrame) + "  Ball " + std::to_string(currentBall), 16);
    frameInfo.setPosition({panelX + 16.0f, panelY + 62.0f});
    frameInfo.setFillColor(sf::Color(56, 84, 136));
    window.draw(frameInfo);

    float frameProgress = std::clamp((float)currentFrame / 10.0f, 0.0f, 1.0f);
    if (currentFrame < 10) {
        frameProgress = std::clamp(((float)currentFrame + ((float)currentBall - 1.0f) * 0.5f) / 10.0f, 0.0f, 1.0f);
    }
    sf::RectangleShape progBg({panelW - 34.0f, 10.0f});
    progBg.setPosition({panelX + 17.0f, panelY + 88.0f});
    progBg.setFillColor(sf::Color(182, 206, 245, 242));
    progBg.setOutlineColor(sf::Color(102, 137, 199));
    progBg.setOutlineThickness(1.2f);
    window.draw(progBg);

    sf::RectangleShape progFill({(panelW - 34.0f) * frameProgress, 10.0f});
    progFill.setPosition({panelX + 17.0f, panelY + 88.0f});
    progFill.setFillColor(sf::Color(86, 206, 170));
    window.draw(progFill);

    int liveTotal = 0;
    for (int i = 0; i < 10; i++) {
        if (frames[i].isComplete || i < currentFrame) liveTotal = frames[i].score;
    }

    sf::Text totalText(font, "Score  " + formatCompactScore(liveTotal), 30);
    totalText.setPosition({panelX + 16.0f, panelY + 104.0f});
    totalText.setFillColor(sf::Color(48, 165, 187));
    totalText.setOutlineColor(sf::Color(22, 35, 61));
    totalText.setOutlineThickness(2.0f);
    window.draw(totalText);

    const float cardX = panelX + 12.0f;
    const float cardW = panelW - 24.0f;
    const float rowH = 64.0f;
    const float cardsStartY = panelY + 146.0f;

    for (int i = 0; i < 10; i++) {
        float y = cardsStartY + rowH * (float)i;
        bool current = (i == currentFrame && currentFrame < 10);
        bool complete = (frames[i].isComplete || i < currentFrame);

        sf::Color cardColor = complete ? sf::Color(240, 246, 255, 242) : sf::Color(229, 239, 253, 242);
        sf::Color borderColor = current ? sf::Color(90, 232, 182) : sf::Color(118, 147, 210);
        sf::ConvexShape card = createRoundedRect({cardW, rowH - 6.0f}, 9.0f, cardColor);
        card.setPosition({cardX, y});
        card.setOutlineColor(borderColor);
        card.setOutlineThickness(current ? 2.6f : 1.4f);
        window.draw(card);

        sf::Text frameLabel(font, "F" + std::to_string(i + 1), 18);
        frameLabel.setPosition({cardX + 10.0f, y + 8.0f});
        frameLabel.setFillColor(current ? sf::Color(22, 129, 95) : sf::Color(44, 69, 117));
        window.draw(frameLabel);

        std::string ball1Str = "";
        std::string ball2Str = "";
        if (currentFrame > i || (currentFrame == i && currentBall > 1)) {
            ball1Str = (frames[i].ball1 == 0) ? "-" : std::to_string(frames[i].ball1);
        }
        if (currentFrame > i || (currentFrame == i && currentBall > 2)) {
            ball2Str = (frames[i].ball2 == 0) ? "-" : std::to_string(frames[i].ball2);
        }
        if (frames[i].isStrike && i < 9) {
            ball1Str = "X";
            ball2Str = "";
        } else if (frames[i].isSpare) {
            ball2Str = "/";
        } else if (frames[i].ball2 == 10) {
            ball2Str = "X";
        }

        sf::Text b1(font, ball1Str, 20);
        b1.setPosition({cardX + cardW - 92.0f, y + 7.0f});
        b1.setFillColor(sf::Color(19, 37, 78));
        window.draw(b1);

        sf::Text b2(font, ball2Str, 20);
        b2.setPosition({cardX + cardW - 66.0f, y + 7.0f});
        b2.setFillColor(sf::Color(19, 37, 78));
        window.draw(b2);

        if (i == 9 && (frames[i].ball3 > 0 || frames[i].ball3 == 10)) {
            std::string ball3Str = frames[i].ball3 == 10 ? "X" : std::to_string(frames[i].ball3);
            sf::Text b3(font, ball3Str, 16);
            b3.setPosition({cardX + cardW - 40.0f, y + 9.0f});
            b3.setFillColor(sf::Color(48, 73, 118));
            window.draw(b3);
        }

        if (complete) {
            sf::Text scoreText(font, formatCompactScore(frames[i].score), 24);
            scoreText.setPosition({cardX + cardW - 110.0f, y + 32.0f});
            scoreText.setFillColor(sf::Color(44, 156, 178));
            window.draw(scoreText);
        } else {
            sf::Text pending(font, "...", 22);
            pending.setPosition({cardX + cardW - 76.0f, y + 32.0f});
            pending.setFillColor(sf::Color(96, 122, 173));
            window.draw(pending);
        }
    }

    sf::Text highScoreDisplay(font, "High Score: " + formatCompactScore(normalHighScore), 18);
    highScoreDisplay.setPosition({panelX + 16.0f, panelY + panelH - 26.0f});
    highScoreDisplay.setFillColor(sf::Color(93, 228, 243));
    window.draw(highScoreDisplay);

    sf::FloatRect exitRect(sf::Vector2f(windowW - 146.0f, 18.0f), sf::Vector2f(122.0f, 44.0f));
    sf::ConvexShape exitBtn = createRoundedRect(exitRect.size, 10.0f, sf::Color(245, 230, 70, 235));
    exitBtn.setPosition(exitRect.position);
    exitBtn.setOutlineColor(sf::Color(223, 69, 84));
    exitBtn.setOutlineThickness(2.0f);
    window.draw(exitBtn);

    sf::Text exitText(font, "MENU", 22);
    sf::FloatRect eb = exitText.getLocalBounds();
    exitText.setPosition({
        exitRect.position.x + exitRect.size.x * 0.5f - (eb.position.x + eb.size.x * 0.5f),
        exitRect.position.y + exitRect.size.y * 0.5f - (eb.position.y + eb.size.y * 0.58f)
    });
    exitText.setFillColor(sf::Color(18, 39, 82));
    window.draw(exitText);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);
        if (pointInRectPadded(exitRect, worldPos, 8.0f)) {
            return GameAction::ExitToMenu;
        }
    }
    return GameAction::None;
}

GameAction UI::drawXtremeHUD(sf::RenderWindow& window,
                             int round,
                             int frameInRound,
                             int shotInFrame,
                             int totalShots,
                             int targetScore,
                             int roundScore,
                             int tokens,
                             int impact,
                             int combo,
                             int lastShotScore,
                             float windowW,
                             float windowH,
                             const ActiveItems& items,
                             const std::string& pinPowerHintLine1,
                             const std::string& pinPowerHintLine2,
                             bool useLiveFormulaPreview,
                             int liveImpactPreview,
                             int liveComboPreview,
                             bool bossRound,
                             const std::string& bossName,
                             const std::string& bossDescription) {
    if (!fontLoaded) return GameAction::None;
    if (state != GameState::Xtreme) return GameAction::None;
    if (!bossRound) {
        bossInfoPopupOpen = false;
    }

    float dt = 0.0f;
    if (!hudAnimClockPrimed) {
        hudAnimClock.restart();
        hudAnimClockPrimed = true;
    } else {
        dt = hudAnimClock.restart().asSeconds();
    }

    int formulaBaseImpact = 10;
    int formulaBaseCombo = 1;
    if (bossRound && bossName == "THE TWINS") {
        formulaBaseImpact = 5;
        formulaBaseCombo = 0;
    }
    if (!hudCounting && !hudCountingFormula && !hudShowBigScore &&
        (hudFormulaBaseImpact != formulaBaseImpact || hudFormulaBaseCombo != formulaBaseCombo)) {
        hudFormulaBaseImpact = formulaBaseImpact;
        hudFormulaBaseCombo = formulaBaseCombo;
        hudImpactTarget = formulaBaseImpact;
        hudComboTarget = formulaBaseCombo;
        hudImpactValue = static_cast<float>(formulaBaseImpact);
        hudComboValue = static_cast<float>(formulaBaseCombo);
    }

    if (totalShots <= 0) {
        hudLastShotCounter = 0;
        hudCountTarget = 0;
        hudCountValue = 0.0f;
        hudCountTimer = 0.0f;
        hudFormulaTimer = 0.0f;
        hudFormulaBaseImpact = formulaBaseImpact;
        hudFormulaBaseCombo = formulaBaseCombo;
        hudImpactTarget = formulaBaseImpact;
        hudComboTarget = formulaBaseCombo;
        hudImpactValue = static_cast<float>(formulaBaseImpact);
        hudComboValue = static_cast<float>(formulaBaseCombo);
        hudCounting = false;
        hudCountingFormula = false;
        hudShowBigScore = false;
        hudBigTimer = 0.0f;
    } else if (totalShots != hudLastShotCounter) {
        hudLastShotCounter = totalShots;
        hudCountTarget = std::max(0, lastShotScore);
        hudImpactTarget = std::max(0, impact);
        hudComboTarget = std::max(0, combo);
        hudCountValue = 0.0f;
        hudImpactValue = static_cast<float>(formulaBaseImpact);
        hudComboValue = static_cast<float>(formulaBaseCombo);
        hudFormulaTimer = 0.0f;
        int formulaDelta = std::max(
            0,
            (hudImpactTarget - formulaBaseImpact) + (hudComboTarget - formulaBaseCombo) * 3);
        hudFormulaDuration = std::clamp(0.25f + (float)formulaDelta * 0.02f, 0.25f, 0.75f);
        hudCountTimer = 0.0f;
        hudCountDuration = std::clamp(0.30f + (float)hudCountTarget / 320.0f, 0.30f, 1.10f);
        hudCountingFormula = true;
        hudCounting = false;
        hudShowBigScore = false;
        hudBigTimer = 0.0f;
    }

    if (hudCountingFormula) {
        hudFormulaTimer += dt;
        float t = (hudFormulaDuration > 0.001f) ? (hudFormulaTimer / hudFormulaDuration) : 1.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        float eased = 1.0f - std::pow(1.0f - t, 3.0f);
        hudImpactValue = (float)formulaBaseImpact + (float)(hudImpactTarget - formulaBaseImpact) * eased;
        hudComboValue = (float)formulaBaseCombo + (float)(hudComboTarget - formulaBaseCombo) * eased;

        if (t >= 1.0f) {
            hudImpactValue = (float)hudImpactTarget;
            hudComboValue = (float)hudComboTarget;
            hudCountingFormula = false;
            hudCounting = true;
            hudCountTimer = 0.0f;
            hudCountValue = 0.0f;
        }
    } else if (hudCounting) {
        hudCountTimer += dt;
        float t = (hudCountDuration > 0.001f) ? (hudCountTimer / hudCountDuration) : 1.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        float eased = 1.0f - std::pow(1.0f - t, 3.0f);
        hudCountValue = (float)hudCountTarget * eased;
        if (t >= 1.0f) {
            hudCountValue = (float)hudCountTarget;
            hudCounting = false;
            hudShowBigScore = true;
            hudBigTimer = 0.0f;
        }
    } else if (hudShowBigScore) {
        hudBigTimer += dt;
        if (hudBigTimer >= 0.9f) {
            hudShowBigScore = false;
            hudCountTarget = 0;
            hudCountValue = 0.0f;
            hudFormulaTimer = 0.0f;
            hudFormulaBaseImpact = formulaBaseImpact;
            hudFormulaBaseCombo = formulaBaseCombo;
            hudImpactTarget = formulaBaseImpact;
            hudComboTarget = formulaBaseCombo;
            hudImpactValue = static_cast<float>(formulaBaseImpact);
            hudComboValue = static_cast<float>(formulaBaseCombo);
        }
    }

    sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    bool mouseDownNow = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool mousePressedEdge = mouseDownNow && !xtremeMouseWasDown;
    xtremeMouseWasDown = mouseDownNow;
    bool bossClickConsumed = false;

    const float sideMargin = std::clamp(windowW * 0.017f, 14.0f, 28.0f);
    const float panelTop = std::clamp(windowH * 0.03f, 18.0f, 34.0f);
    const float panelBottomMargin = std::clamp(windowH * 0.034f, 22.0f, 40.0f);
    const float minSidePanelW = 230.0f;
    const float desiredSidePanelW = std::clamp(windowW * 0.205f, 250.0f, 372.0f);
    const float maxSidePanelW = std::max(minSidePanelW, (windowW - 360.0f - sideMargin * 2.0f - 24.0f) * 0.5f);
    const float sidePanelW = std::clamp(desiredSidePanelW, minSidePanelW, maxSidePanelW);
    const float sidePanelH = windowH - panelTop - panelBottomMargin;
    const float leftPanelX = sideMargin;
    const float leftPanelY = panelTop;

    // Left panel
    sf::ConvexShape leftPanelShadow = createRoundedRect(
        {sidePanelW + 10.0f, sidePanelH + 10.0f}, 18.0f, sf::Color(0, 0, 0, 64));
    leftPanelShadow.setPosition({leftPanelX + 5.0f, leftPanelY + 6.0f});
    window.draw(leftPanelShadow);

    sf::ConvexShape leftPanel = createRoundedRect(
        {sidePanelW, sidePanelH}, 18.0f, sf::Color(223, 234, 251, 242));
    leftPanel.setPosition({leftPanelX, leftPanelY});
    leftPanel.setOutlineColor(sf::Color(92, 126, 196));
    leftPanel.setOutlineThickness(2.5f);
    window.draw(leftPanel);

    sf::RectangleShape leftHead({sidePanelW - 6.0f, 58.0f});
    leftHead.setPosition({leftPanelX + 3.0f, leftPanelY + 3.0f});
    leftHead.setFillColor(sf::Color(105, 153, 231, 242));
    window.draw(leftHead);

    sf::RectangleShape leftHeadAccent({sidePanelW - 20.0f, 3.0f});
    leftHeadAccent.setPosition({leftPanelX + 10.0f, leftPanelY + 52.0f});
    leftHeadAccent.setFillColor(sf::Color(195, 224, 255, 226));
    window.draw(leftHeadAccent);

    float lx = leftPanelX + 18.0f;
    float y = leftPanelY + 9.0f;
    float bossBadgeBottomY = leftPanelY;
    const float leftTextMaxW = sidePanelW - 34.0f;
    static float bossPulseTime = 0.0f;
    bossPulseTime += std::clamp(dt, 0.0f, 0.05f);
    float bossPulse = 0.5f + 0.5f * std::sin(bossPulseTime * 6.2f);
    float bossBadgeW = std::min(164.0f, sidePanelW * 0.52f);
    sf::FloatRect bossBadgeRect{};
    sf::FloatRect bossInfoRect{};
    auto fitTextToPanel = [&](sf::Text& text, float maxW, unsigned minSize) {
        while (text.getCharacterSize() > minSize && text.getLocalBounds().size.x > maxW) {
            text.setCharacterSize(text.getCharacterSize() - 1);
        }
    };

    sf::Text modeText(font, "XTREME MODE", 30);
    fitTextToPanel(modeText, leftTextMaxW, 15);
    modeText.setPosition({lx, y});
    modeText.setFillColor(sf::Color(19, 39, 82));
    window.draw(modeText);

    if (bossRound) {
        float badgeX = lx;
        float badgeY = leftPanelY + 58.0f;
        bossBadgeRect = sf::FloatRect({badgeX, badgeY}, {bossBadgeW, 24.0f});
        sf::ConvexShape bossBadge = createRoundedRect(
            {bossBadgeRect.size.x, bossBadgeRect.size.y},
            8.0f,
            sf::Color(255, (std::uint8_t)(115 + 34 * bossPulse), (std::uint8_t)(128 + 20 * bossPulse), 244));
        bossBadge.setPosition(bossBadgeRect.position);
        bossBadge.setOutlineColor(sf::Color(130, 36, 58));
        bossBadge.setOutlineThickness(1.8f);
        window.draw(bossBadge);

        sf::Text bossText(font, bossName, 14);
        fitTextToPanel(bossText, bossBadgeRect.size.x - 12.0f, 11);
        sf::FloatRect bb = bossText.getLocalBounds();
        bossText.setPosition({bossBadgeRect.position.x + bossBadgeRect.size.x * 0.5f -
                                  (bb.position.x + bb.size.x * 0.5f),
                              bossBadgeRect.position.y + 3.0f});
        bossText.setFillColor(sf::Color(255, 248, 232));
        bossText.setOutlineColor(sf::Color(111, 20, 39));
        bossText.setOutlineThickness(1.0f);
        window.draw(bossText);

        if (mousePressedEdge && pointInRectPadded(bossBadgeRect, mouseWorld, 7.0f)) {
            bossInfoPopupOpen = !bossInfoPopupOpen;
        }
        bossBadgeBottomY = badgeY + bossBadgeRect.size.y;
    } else {
        bossInfoPopupOpen = false;
    }
    if (bossRound) {
        y = std::max(y + 64.0f, bossBadgeBottomY + 12.0f);
    } else {
        y += 64.0f;
    }

    float targetProgress = 0.0f;
    if (targetScore > 0) {
        targetProgress = std::clamp((float)roundScore / (float)targetScore, 0.0f, 1.0f);
    }
    const float targetCardH = 98.0f;
    sf::Color targetCardFill = bossRound ? sf::Color(252, 228, 220, 246)
                                         : sf::Color(247, 240, 209, 246);
    sf::ConvexShape targetCard = createRoundedRect({leftTextMaxW, targetCardH}, 10.0f, targetCardFill);
    targetCard.setPosition({lx, y});
    targetCard.setOutlineColor(
        targetProgress >= 1.0f ? sf::Color(110, 240, 145)
                               : (bossRound ? sf::Color(245, 108, 122) : sf::Color(250, 214, 120)));
    targetCard.setOutlineThickness(2.0f);
    window.draw(targetCard);

    sf::Text targetLabel(font, bossRound ? "BOSS TARGET" : "TARGET SCORE", 20);
    fitTextToPanel(targetLabel, leftTextMaxW - 20.0f, 13);
    targetLabel.setPosition({lx + 10.0f, y + 8.0f});
    targetLabel.setFillColor(bossRound ? sf::Color(132, 42, 56) : sf::Color(109, 66, 26));
    window.draw(targetLabel);

    sf::Text targetValue(font,
                         formatCompactScore(roundScore) + " / " + formatCompactScore(targetScore),
                         36);
    fitTextToPanel(targetValue, leftTextMaxW - 20.0f, 20);
    targetValue.setPosition({lx + 10.0f, y + 30.0f});
    targetValue.setFillColor(targetProgress >= 1.0f
                             ? sf::Color(33, 130, 78)
                             : (bossRound ? sf::Color(122, 39, 56) : sf::Color(35, 58, 107)));
    targetValue.setOutlineColor(sf::Color(246, 250, 255));
    targetValue.setOutlineThickness(2.0f);
    window.draw(targetValue);

    const float targetBarY = y + targetCardH - 18.0f;
    sf::RectangleShape targetTrack({leftTextMaxW - 14.0f, 10.0f});
    targetTrack.setPosition({lx + 7.0f, targetBarY});
    targetTrack.setFillColor(bossRound ? sf::Color(238, 190, 186, 242)
                                       : sf::Color(221, 205, 157, 242));
    targetTrack.setOutlineColor(bossRound ? sf::Color(173, 80, 94)
                                          : sf::Color(174, 137, 76));
    targetTrack.setOutlineThickness(1.1f);
    window.draw(targetTrack);

    sf::RectangleShape targetFill({(leftTextMaxW - 14.0f) * targetProgress, 10.0f});
    targetFill.setPosition({lx + 7.0f, targetBarY});
    targetFill.setFillColor(
        targetProgress >= 1.0f ? sf::Color(110, 244, 150)
                               : (bossRound
                                      ? sf::Color(252, (std::uint8_t)(124 + 24 * bossPulse),
                                                  (std::uint8_t)(118 + 18 * bossPulse))
                                      : sf::Color(243, 209, 98)));
    window.draw(targetFill);

    // Add more breathing room so all rows below target card start lower.
    y += targetCardH + 36.0f;

    sf::Text t1(font, "round " + std::to_string(round), 40);
    t1.setPosition(sf::Vector2f(lx, y));
    t1.setFillColor(sf::Color(19, 39, 82));
    window.draw(t1);
    y += 42;

    sf::Text t2(font, "frame " + std::to_string(frameInRound), 30);
    t2.setPosition(sf::Vector2f(lx, y));
    t2.setFillColor(sf::Color(47, 73, 122));
    window.draw(t2);
    y += 36;

    sf::Text t3(font, "shot " + std::to_string(shotInFrame), 30);
    t3.setPosition(sf::Vector2f(lx, y));
    t3.setFillColor(sf::Color(47, 73, 122));
    window.draw(t3);
    y += 48;

    int shownImpact = std::max(0, (int)std::round(hudImpactValue));
    int shownCombo = std::max(0, (int)std::round(hudComboValue));
    if (useLiveFormulaPreview) {
        shownImpact = std::max(0, liveImpactPreview);
        shownCombo = std::max(0, liveComboPreview);
    }
    sf::Text big(font, formatCompactScore(shownImpact) + " X " + formatCompactScore(shownCombo), 58);
    fitTextToPanel(big, leftTextMaxW, 24);
    big.setPosition(sf::Vector2f(lx, y));
    big.setFillColor(sf::Color(54, 175, 196));
    big.setOutlineColor(sf::Color(22, 21, 17));
    big.setOutlineThickness(3.0f);
    window.draw(big);
    y += 78;

    sf::Text explain(font, "Impact x Pin Combo", 25);
    explain.setPosition(sf::Vector2f(lx, y));
    explain.setFillColor(sf::Color(236, 66, 91));
    window.draw(explain);
    y += 38;

    int shownShotAdd = 0;
    if (hudCounting) {
        shownShotAdd = (int)std::round(hudCountValue);
    } else if (hudShowBigScore) {
        shownShotAdd = hudCountTarget;
    }
    sf::RectangleShape shotRow({leftTextMaxW, 34.0f});
    shotRow.setPosition({lx, y - 2.0f});
    shotRow.setFillColor(sf::Color(230, 240, 255, 242));
    shotRow.setOutlineColor(sf::Color(112, 144, 206));
    shotRow.setOutlineThickness(1.4f);
    window.draw(shotRow);

    sf::Text shotScore(font, "shot add: " + formatCompactScore(shownShotAdd), 23);
    fitTextToPanel(shotScore, leftTextMaxW - 10.0f, 14);
    shotScore.setPosition(sf::Vector2f(lx + 8.0f, y + 1.0f));
    shotScore.setFillColor(hudCounting ? sf::Color(18, 158, 112) : sf::Color(25, 44, 88));
    window.draw(shotScore);
    y += 40;

    sf::RectangleShape roundRow({leftTextMaxW, 39.0f});
    roundRow.setPosition({lx, y - 2.0f});
    roundRow.setFillColor(sf::Color(230, 240, 255, 242));
    roundRow.setOutlineColor(sf::Color(112, 144, 206));
    roundRow.setOutlineThickness(1.5f);
    window.draw(roundRow);

    sf::Text roundScoreText(font, "round score: " + formatCompactScore(roundScore), 30);
    fitTextToPanel(roundScoreText, leftTextMaxW - 10.0f, 16);
    roundScoreText.setPosition(sf::Vector2f(lx + 8.0f, y + 2.0f));
    roundScoreText.setFillColor(sf::Color(24, 42, 86));
    window.draw(roundScoreText);
    y += 50;

    sf::Text tokenText(font, "tokens: " + formatCompactScore(tokens), 29);
    fitTextToPanel(tokenText, leftTextMaxW, 16);
    tokenText.setPosition(sf::Vector2f(lx, y));
    tokenText.setFillColor(sf::Color(224, 145, 18));
    window.draw(tokenText);

    if (!pinPowerHintLine1.empty()) {
        y += 38.f;
        sf::Text hint1(font, pinPowerHintLine1, 18);
        hint1.setPosition(sf::Vector2f(lx, y));
        hint1.setFillColor(sf::Color(40, 170, 210));
        window.draw(hint1);
    }
    if (!pinPowerHintLine2.empty()) {
        y += 24.f;
        sf::Text hint2(font, pinPowerHintLine2, 18);
        hint2.setPosition(sf::Vector2f(lx, y));
        hint2.setFillColor(sf::Color(229, 136, 33));
        window.draw(hint2);
    }

    if (hudShowBigScore && hudCountTarget > 0) {
        float t = std::clamp(hudBigTimer / 0.9f, 0.0f, 1.0f);
        float pulse = 1.0f + 0.12f * std::sin(t * 6.283185f);
        float fade = (t > 0.72f) ? (1.0f - (t - 0.72f) / 0.28f) : 1.0f;
        fade = std::clamp(fade, 0.0f, 1.0f);
        std::uint8_t alpha = (std::uint8_t)(255.0f * fade);

        sf::Text bigShot(font, "+" + formatCompactScore(hudCountTarget), 88);
        fitTextToPanel(bigShot, sidePanelW * 0.82f, 40);
        bigShot.setStyle(sf::Text::Bold);
        bigShot.setScale({pulse, pulse});
        bigShot.setFillColor(sf::Color(243, 137, 66, alpha));
        bigShot.setOutlineColor(sf::Color(245, 248, 255, alpha));
        bigShot.setOutlineThickness(4.0f);

        sf::FloatRect bb = bigShot.getLocalBounds();
        float centerX = leftPanelX + sidePanelW * 0.50f;
        float centerY = leftPanelY + sidePanelH * 0.56f;
        bigShot.setPosition({centerX - bb.size.x * 0.5f, centerY - bb.size.y * 0.5f});
        window.draw(bigShot);
    }

    // Right inventory panel.
    const float rightPanelW = sidePanelW;
    const float rightPanelX = windowW - rightPanelW - sideMargin;
    const float rightPanelY = panelTop;
    const float rightPanelH = sidePanelH;

    sf::ConvexShape rightPanelShadow = createRoundedRect(
        {rightPanelW + 10.0f, rightPanelH + 10.0f}, 18.0f, sf::Color(0, 0, 0, 64));
    rightPanelShadow.setPosition({rightPanelX + 5.0f, rightPanelY + 6.0f});
    window.draw(rightPanelShadow);

    sf::ConvexShape rightPanel = createRoundedRect(
        {rightPanelW, rightPanelH}, 18.0f, sf::Color(223, 234, 251, 242));
    rightPanel.setPosition({rightPanelX, rightPanelY});
    rightPanel.setOutlineColor(sf::Color(92, 126, 196));
    rightPanel.setOutlineThickness(2.3f);
    window.draw(rightPanel);

    sf::RectangleShape invHead({rightPanelW - 6.0f, 50.0f});
    invHead.setPosition({rightPanelX + 3.0f, rightPanelY + 3.0f});
    invHead.setFillColor(sf::Color(105, 153, 231, 242));
    window.draw(invHead);

    sf::Text invTitle(font, "INVENTORY", 33);
    sf::FloatRect invB = invTitle.getLocalBounds();
    invTitle.setPosition({rightPanelX + rightPanelW * 0.5f - (invB.position.x + invB.size.x * 0.5f),
                          rightPanelY + 10.0f});
    invTitle.setFillColor(sf::Color(19, 39, 82));
    window.draw(invTitle);

    drawInventoryPanel(window, items, rightPanelX + 8.0f, rightPanelY + 62.0f, rightPanelW - 16.0f);

    if (bossRound && bossInfoPopupOpen) {
        float infoW = std::clamp(sidePanelW - 20.0f, 230.0f, 360.0f);
        float infoH = 144.0f;
        float infoX = leftPanelX + 10.0f;
        float infoY = leftPanelY + 96.0f;
        bossInfoRect = sf::FloatRect({infoX, infoY}, {infoW, infoH});

        sf::RectangleShape infoShadow({infoW, infoH});
        infoShadow.setPosition({infoX + 3.0f, infoY + 3.0f});
        infoShadow.setFillColor(sf::Color(0, 0, 0, 70));
        window.draw(infoShadow);

        sf::ConvexShape infoPanel = createRoundedRect({infoW, infoH}, 10.0f, sf::Color(233, 241, 253, 250));
        infoPanel.setPosition({infoX, infoY});
        infoPanel.setOutlineColor(sf::Color(133, 39, 60));
        infoPanel.setOutlineThickness(2.2f);
        window.draw(infoPanel);

        sf::RectangleShape infoBand({infoW - 4.0f, 30.0f});
        infoBand.setPosition({infoX + 2.0f, infoY + 2.0f});
        infoBand.setFillColor(sf::Color(247, 110, 126, 244));
        window.draw(infoBand);

        sf::Text name(font, bossName, 18);
        sf::FloatRect nb = name.getLocalBounds();
        name.setPosition({infoX + infoW * 0.5f - (nb.position.x + nb.size.x * 0.5f), infoY + 7.0f});
        name.setFillColor(sf::Color(255, 247, 236));
        name.setOutlineColor(sf::Color(109, 18, 37));
        name.setOutlineThickness(1.0f);
        window.draw(name);

        sf::Text desc(font, bossDescription, 20);
        fitTextToPanel(desc, infoW - 24.0f, 12);
        sf::FloatRect db = desc.getLocalBounds();
        float descX = infoX + infoW * 0.5f - (db.position.x + db.size.x * 0.5f);
        float descMinX = infoX + 12.0f;
        float descMaxX = infoX + infoW - db.size.x - 12.0f;
        if (descX < descMinX) descX = descMinX;
        if (descX > descMaxX) descX = descMaxX;
        desc.setPosition({descX, infoY + 54.0f});
        desc.setFillColor(sf::Color(35, 59, 109));
        window.draw(desc);

        sf::Text tip(font, "Click badge to hide", 14);
        tip.setPosition({infoX + 12.0f, infoY + infoH - 24.0f});
        tip.setFillColor(sf::Color(94, 112, 151));
        window.draw(tip);
    }

    if (bossRound && bossInfoPopupOpen && mousePressedEdge) {
        bossClickConsumed = true;
        bool clickedPopup = pointInRectPadded(bossInfoRect, mouseWorld, 4.0f);
        bool clickedBadge = pointInRectPadded(bossBadgeRect, mouseWorld, 7.0f);
        if (!clickedPopup && !clickedBadge) {
            bossInfoPopupOpen = false;
        }
    }

    // Menu button.
    float exitW = 130.0f;
    float exitH = 44.0f;
    float exitX = rightPanelX - exitW - 14.0f;
    float minExitX = leftPanelX + sidePanelW + 16.0f;
    float maxExitX = windowW - exitW - sideMargin;
    if (maxExitX < minExitX) maxExitX = minExitX;
    exitX = std::clamp(exitX, minExitX, maxExitX);
    sf::FloatRect exitRect(sf::Vector2f(exitX, panelTop - 2.0f), sf::Vector2f(exitW, exitH));
    bool menuHover = pointInRectPadded(exitRect, mouseWorld, 5.0f);

    sf::ConvexShape exitBtn = createRoundedRect(exitRect.size, 10.0f, menuHover
        ? sf::Color(249, 236, 70, 240)
        : sf::Color(241, 222, 58, 232));
    exitBtn.setPosition(exitRect.position);
    exitBtn.setOutlineThickness(2.0f);
    exitBtn.setOutlineColor(menuHover ? sf::Color(247, 81, 94) : sf::Color(208, 61, 75));
    window.draw(exitBtn);

    sf::ConvexShape exitGlow = createRoundedRect({exitRect.size.x - 6.0f, 14.0f}, 8.0f,
                                                  sf::Color(255, 255, 255, menuHover ? 88 : 60));
    exitGlow.setPosition({exitRect.position.x + 3.0f, exitRect.position.y + 3.0f});
    window.draw(exitGlow);

    sf::Text exitText(font, "MENU", 22);
    sf::FloatRect eb = exitText.getLocalBounds();
    exitText.setPosition({
        exitRect.position.x + exitRect.size.x * 0.5f - (eb.position.x + eb.size.x * 0.5f),
        exitRect.position.y + exitRect.size.y * 0.5f - (eb.position.y + eb.size.y * 0.58f)
    });
    exitText.setFillColor(sf::Color(15, 33, 72));
    window.draw(exitText);

    if (mousePressedEdge && !bossClickConsumed) {
        if (pointInRectPadded(exitRect, mouseWorld, 8.0f)) {
            return GameAction::ExitToMenu;
        }
    }

    // Bottom bar hidden in Xtreme to avoid duplicate inventory UI.

    return GameAction::None;
}

void UI::drawGameOverScreen(sf::RenderWindow& window, 
                            GameOverMode mode,
                            int finalScore, 
                            int highScore,
                            float windowW, 
                            float windowH,
                            int progressScore,
                            int progressTarget) {
    if (!fontLoaded) return;
    
    // Semi-transparent overlay
    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(24, 46, 94, 132));
    window.draw(overlay);
    
    // Game Over text
    sf::Text gameOverText(font, "GAME OVER!", 80);
    gameOverText.setFillColor(sf::Color(245, 78, 104));
    gameOverText.setOutlineColor(sf::Color(12, 30, 66));
    gameOverText.setOutlineThickness(2.2f);
    gameOverText.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = gameOverText.getLocalBounds();
    gameOverText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 - 220
    ));
    window.draw(gameOverText);
    
    // Final score
    std::string mainLabel = (mode == GameOverMode::Xtreme)
    ? "Rounds Cleared: "
    : "Final Score: ";
    sf::Text scoreText(font, mainLabel + std::to_string(finalScore), 50);
    scoreText.setFillColor(sf::Color(246, 226, 72));
    scoreText.setOutlineColor(sf::Color(14, 32, 70));
    scoreText.setOutlineThickness(1.6f);
    bounds = scoreText.getLocalBounds();
    scoreText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 - 100
    ));
    window.draw(scoreText);

    if (mode == GameOverMode::Xtreme && progressTarget > 0) {
        bool passed = progressScore >= progressTarget;
        sf::Text progressText(font,
                              "Score: " + std::to_string(progressScore) + "/" + std::to_string(progressTarget) +
                                  (passed ? "  PASSED" : "  FAILED"),
                              34);
        progressText.setFillColor(passed ? sf::Color(110, 255, 140) : sf::Color(255, 140, 140));
        progressText.setOutlineColor(sf::Color(14, 32, 70));
        progressText.setOutlineThickness(1.2f);
        sf::FloatRect pb = progressText.getLocalBounds();
        progressText.setPosition(sf::Vector2f(
            windowW / 2 - pb.size.x / 2,
            windowH / 2 - 38
        ));
        window.draw(progressText);
    }

    // High score
    std::string bestLabel = (mode == GameOverMode::Xtreme)
    ? "Best Round: "
    : "High Score: ";
    sf::Text highScoreText(font, bestLabel + std::to_string(highScore), 40);
    if (finalScore == highScore && finalScore > 0) {
        highScoreText.setString((mode == GameOverMode::Xtreme) ? "NEW BEST ROUND!" : "NEW HIGH SCORE!");
        highScoreText.setFillColor(sf::Color(90, 250, 165));
    } else {
        highScoreText.setFillColor(sf::Color(112, 219, 244));
    }
    bounds = highScoreText.getLocalBounds();
    highScoreText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 + 20
    ));
    window.draw(highScoreText);
    
    // Restart instruction
    sf::Text restartText(font, "Press R to Restart", 30);
    restartText.setFillColor(sf::Color(236, 243, 255));
    restartText.setOutlineColor(sf::Color(14, 32, 70));
    restartText.setOutlineThickness(1.0f);
    bounds = restartText.getLocalBounds();
    restartText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 + 100
    ));
    window.draw(restartText);

    sf::Text menuText(font, "Press M for Menu", 30);
    menuText.setFillColor(sf::Color(236, 243, 255));
    menuText.setOutlineColor(sf::Color(14, 32, 70));
    menuText.setOutlineThickness(1.0f);
    bounds = menuText.getLocalBounds();
    menuText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 + 140
    ));
    window.draw(menuText);
}

void UI::drawRoundSummaryPopup(sf::RenderWindow& window,
                               int roundNumber,
                               int roundScore,
                               int targetScore,
                               int tokensEarned,
                               int tokensTotal,
                               bool passed,
                               bool leadsToGameOver,
                               const std::string& bossName,
                               float windowW,
                               float windowH) {
    if (!fontLoaded) return;
    bool bossRound = (roundNumber > 0 && (roundNumber % 5) == 0);

    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(22, 44, 90, 122));
    window.draw(overlay);

    const float panelW = 620.f;
    const float panelH = 420.f;
    const float panelX = windowW * 0.5f - panelW * 0.5f;
    const float panelY = windowH * 0.5f - panelH * 0.5f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(sf::Vector2f(panelX, panelY));
    panel.setFillColor(sf::Color(230, 239, 252, 248));
    panel.setOutlineColor(sf::Color(98, 130, 198));
    panel.setOutlineThickness(3.f);
    window.draw(panel);

    // Round track strip: boss rounds are shown with skull markers.
    const float stripW = panelW;
    const float stripH = 42.f;
    const float stripX = panelX;
    const float stripY = panelY - 40.f;
    sf::ConvexShape strip = createRoundedRect({stripW, stripH}, 10.f, sf::Color(246, 234, 170, 245));
    strip.setPosition({stripX, stripY});
    strip.setFillColor(sf::Color(246, 234, 170, 245));
    strip.setOutlineColor(sf::Color(117, 96, 42));
    strip.setOutlineThickness(2.f);
    window.draw(strip);

    auto drawSkullMarker = [&](float cx, float cy, float scale, sf::Color fill, sf::Color outline) {
        float headR = 6.8f * scale;
        sf::CircleShape head(headR);
        head.setOrigin({headR, headR});
        head.setPosition({cx, cy - 1.2f * scale});
        head.setFillColor(fill);
        head.setOutlineColor(outline);
        head.setOutlineThickness(1.1f);
        window.draw(head);

        sf::RectangleShape jaw({headR * 1.46f, headR * 0.86f});
        jaw.setOrigin({jaw.getSize().x * 0.5f, 0.f});
        jaw.setPosition({cx, cy + headR * 0.20f});
        jaw.setFillColor(fill);
        jaw.setOutlineColor(outline);
        jaw.setOutlineThickness(1.0f);
        window.draw(jaw);

        sf::CircleShape eye(headR * 0.18f);
        eye.setOrigin({eye.getRadius(), eye.getRadius()});
        eye.setFillColor(sf::Color(20, 22, 28));
        eye.setPosition({cx - headR * 0.42f, cy - headR * 0.14f});
        window.draw(eye);
        eye.setPosition({cx + headR * 0.42f, cy - headR * 0.14f});
        window.draw(eye);
    };

    auto drawCheckMarker = [&](float cx, float cy, float scale) {
        float r = 7.0f * scale;
        sf::CircleShape bubble(r);
        bubble.setOrigin({r, r});
        bubble.setPosition({cx, cy});
        bubble.setFillColor(sf::Color(76, 230, 123));
        bubble.setOutlineColor(sf::Color(28, 94, 57));
        bubble.setOutlineThickness(1.2f);
        window.draw(bubble);

        sf::RectangleShape segA({r * 0.72f, r * 0.20f});
        segA.setOrigin({segA.getSize().x * 0.5f, segA.getSize().y * 0.5f});
        segA.setPosition({cx - r * 0.20f, cy + r * 0.20f});
        segA.setRotation(sf::degrees(42.0f));
        segA.setFillColor(sf::Color(250, 255, 250));
        window.draw(segA);

        sf::RectangleShape segB({r * 1.18f, r * 0.20f});
        segB.setOrigin({segB.getSize().x * 0.5f, segB.getSize().y * 0.5f});
        segB.setPosition({cx + r * 0.18f, cy - r * 0.12f});
        segB.setRotation(sf::degrees(-47.0f));
        segB.setFillColor(sf::Color(250, 255, 250));
        window.draw(segB);
    };

    const int markerCount = 15;
    int startRound = std::max(1, roundNumber - 6);
    int endRound = startRound + markerCount - 1;
    if (roundNumber < 8) {
        startRound = 1;
        endRound = markerCount;
    } else if (roundNumber > endRound) {
        startRound = roundNumber - markerCount / 2;
        if (startRound < 1) startRound = 1;
    }
    const float bossStripBadgeW = 150.f;
    const float markerLeft = stripX + 20.f;
    float markerRight = stripX + stripW - 20.f;
    if (bossRound) {
        // Reserve room on the right side of the strip for the boss badge.
        markerRight -= (bossStripBadgeW + 16.f);
    }
    const float markerY = stripY + stripH * 0.5f;
    const float step = (markerCount > 1) ? ((markerRight - markerLeft) / (float)(markerCount - 1)) : 0.f;

    for (int i = 0; i < markerCount; i++) {
        int markerRound = startRound + i;
        float cx = markerLeft + step * (float)i;
        bool isBoss = (markerRound % 5 == 0);
        bool isCurrent = (markerRound == roundNumber);
        bool completed = (markerRound < roundNumber) || (isCurrent && passed);

        if (completed) {
            drawCheckMarker(cx, markerY, isCurrent ? 1.06f : 0.96f);
            continue;
        }

        if (isBoss) {
            sf::Color fill = sf::Color(206, 200, 176);
            sf::Color outline = sf::Color(87, 77, 53);
            if (isCurrent) {
                fill = passed ? sf::Color(147, 252, 186) : sf::Color(255, 168, 182);
                outline = passed ? sf::Color(20, 102, 60) : sf::Color(116, 30, 54);
            }
            drawSkullMarker(cx, markerY, 1.0f, fill, outline);
        } else {
            float r = 6.2f;
            sf::CircleShape dot(r);
            dot.setOrigin({r, r});
            dot.setPosition({cx, markerY});
            if (isCurrent) {
                dot.setFillColor(passed ? sf::Color(81, 233, 133) : sf::Color(244, 98, 118));
                dot.setOutlineColor(sf::Color(32, 51, 96));
                dot.setOutlineThickness(1.3f);
            } else {
                dot.setFillColor(sf::Color(19, 22, 30));
            }
            window.draw(dot);
        }
    }

    sf::Text title(font, "ROUND " + std::to_string(roundNumber) + " SUMMARY", 42);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition(sf::Vector2f(panelX + panelW * 0.5f - titleBounds.size.x * 0.5f, panelY + 18.f));
    title.setFillColor(sf::Color(19, 39, 82));
    title.setOutlineColor(sf::Color(242, 248, 255));
    title.setOutlineThickness(1.5f);
    window.draw(title);

    sf::Text status(font, passed ? "PASSED" : "FAILED", 44);
    sf::FloatRect statusBounds = status.getLocalBounds();
    status.setPosition(sf::Vector2f(panelX + panelW * 0.5f - statusBounds.size.x * 0.5f, panelY + 72.f));
    status.setFillColor(passed ? sf::Color(70, 236, 136) : sf::Color(246, 86, 108));
    status.setOutlineColor(sf::Color(18, 35, 67));
    status.setOutlineThickness(1.5f);
    window.draw(status);

    auto fitPopupText = [&](sf::Text& text, float maxW, unsigned minSize) {
        while (text.getCharacterSize() > minSize && text.getLocalBounds().size.x > maxW) {
            text.setCharacterSize(text.getCharacterSize() - 1);
        }
    };

    bool showBossDefeatedLine = bossRound && passed && !bossName.empty();
    float summaryYOffset = 0.f;
    if (showBossDefeatedLine) {
        sf::Text bossDefeated(font, bossName + " DEFEATED", 30);
        fitPopupText(bossDefeated, panelW - 80.f, 18);
        sf::FloatRect bdb = bossDefeated.getLocalBounds();
        bossDefeated.setPosition({
            panelX + panelW * 0.5f - (bdb.position.x + bdb.size.x * 0.5f),
            panelY + 116.f
        });
        bossDefeated.setFillColor(sf::Color(245, 92, 112));
        bossDefeated.setOutlineColor(sf::Color(18, 35, 67));
        bossDefeated.setOutlineThickness(1.2f);
        window.draw(bossDefeated);
        summaryYOffset = 24.f;
    }

    if (bossRound) {
        const float bossStripBadgeH = 30.f;
        const float bossBadgeX = stripX + stripW - bossStripBadgeW - 10.f;
        const float bossBadgeY = stripY + (stripH - bossStripBadgeH) * 0.5f;
        sf::ConvexShape bossBadge =
            createRoundedRect({bossStripBadgeW, bossStripBadgeH}, 8.0f, sf::Color(245, 107, 122, 245));
        bossBadge.setPosition({bossBadgeX, bossBadgeY});
        bossBadge.setOutlineColor(sf::Color(130, 35, 56));
        bossBadge.setOutlineThickness(1.8f);
        window.draw(bossBadge);

        sf::Text bossText(font, "BOSS ROUND", 19);
        sf::FloatRect btb = bossText.getLocalBounds();
        bossText.setPosition({bossBadgeX + bossStripBadgeW * 0.5f - (btb.position.x + btb.size.x * 0.5f),
                              bossBadgeY + 4.0f});
        bossText.setFillColor(sf::Color(255, 248, 232));
        bossText.setOutlineColor(sf::Color(111, 20, 39));
        bossText.setOutlineThickness(1.0f);
        window.draw(bossText);
    }

    sf::RectangleShape divider(sf::Vector2f(panelW - 70.f, 2.f));
    divider.setPosition(sf::Vector2f(panelX + 35.f, panelY + 132.f + summaryYOffset));
    divider.setFillColor(sf::Color(132, 162, 220, 190));
    window.draw(divider);

    sf::Text ratio(font,
                   "Score: " + std::to_string(roundScore) + "/" + std::to_string(targetScore),
                   40);
    ratio.setPosition(sf::Vector2f(panelX + 36.f, panelY + 150.f + summaryYOffset));
    ratio.setFillColor(sf::Color(246, 224, 79));
    ratio.setOutlineColor(sf::Color(16, 34, 68));
    ratio.setOutlineThickness(1.4f);
    window.draw(ratio);

    std::string tokenEarnedText = "Tokens earned: ";
    if (tokensEarned >= 0) tokenEarnedText += "+";
    tokenEarnedText += std::to_string(tokensEarned);
    sf::Text earned(font, tokenEarnedText, 34);
    earned.setPosition(sf::Vector2f(panelX + 36.f, panelY + 222.f + summaryYOffset));
    earned.setFillColor(sf::Color(46, 202, 232));
    earned.setOutlineColor(sf::Color(16, 34, 68));
    earned.setOutlineThickness(1.2f);
    window.draw(earned);

    sf::Text total(font, "Tokens total: " + std::to_string(tokensTotal), 30);
    total.setPosition(sf::Vector2f(panelX + 36.f, panelY + 274.f + summaryYOffset));
    total.setFillColor(sf::Color(25, 46, 92));
    total.setOutlineColor(sf::Color(244, 249, 255));
    total.setOutlineThickness(1.0f);
    window.draw(total);

    const float btnW = 320.f;
    const float btnH = 62.f;
    const float btnX = panelX + panelW * 0.5f - btnW * 0.5f;
    const float btnY = panelY + panelH - 86.f;
    roundSummaryContinueRect = sf::FloatRect(sf::Vector2f(btnX, btnY), sf::Vector2f(btnW, btnH));

    sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
    btn.setPosition(sf::Vector2f(btnX, btnY));
    btn.setFillColor(leadsToGameOver ? sf::Color(245, 92, 112) : sf::Color(94, 216, 242));
    btn.setOutlineColor(sf::Color(79, 121, 198));
    btn.setOutlineThickness(3.f);
    window.draw(btn);

    sf::Text btnText(font, leadsToGameOver ? "CONTINUE" : "CONTINUE TO SHOP", 32);
    sf::FloatRect bb = btnText.getLocalBounds();
    btnText.setPosition(sf::Vector2f(btnX + btnW * 0.5f - bb.size.x * 0.5f,
                                     btnY + btnH * 0.5f - bb.size.y * 0.5f - 6.f));
    btnText.setFillColor(leadsToGameOver ? sf::Color(252, 252, 255) : sf::Color(19, 39, 82));
    btnText.setOutlineColor(leadsToGameOver ? sf::Color(21, 38, 74) : sf::Color(216, 234, 255));
    btnText.setOutlineThickness(1.0f);
    window.draw(btnText);
}

bool UI::handleRoundSummaryClick(sf::Vector2i mousePos) const {
    return pointInRectPadded(roundSummaryContinueRect, mousePos, 10.0f);
}

void UI::drawGameWonPopup(sf::RenderWindow& window,
                          int roundsCleared,
                          int tokensTotal,
                          float windowW,
                          float windowH) {
    if (!fontLoaded) return;

    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(16, 32, 68, 150));
    window.draw(overlay);

    const float panelW = 680.f;
    const float panelH = 500.f;
    const float panelX = windowW * 0.5f - panelW * 0.5f;
    const float panelY = windowH * 0.5f - panelH * 0.5f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(233, 241, 252, 249));
    panel.setOutlineColor(sf::Color(95, 128, 194));
    panel.setOutlineThickness(3.0f);
    window.draw(panel);

    sf::RectangleShape topBand(sf::Vector2f(panelW, 54.f));
    topBand.setPosition({panelX, panelY});
    topBand.setFillColor(sf::Color(247, 229, 153));
    topBand.setOutlineColor(sf::Color(120, 96, 40));
    topBand.setOutlineThickness(2.0f);
    window.draw(topBand);

    auto drawSkull = [&](float cx, float cy, float scale, sf::Color fill, sf::Color outline) {
        float headR = 12.0f * scale;
        sf::CircleShape head(headR);
        head.setOrigin({headR, headR});
        head.setPosition({cx, cy - 1.5f * scale});
        head.setFillColor(fill);
        head.setOutlineColor(outline);
        head.setOutlineThickness(1.3f);
        window.draw(head);

        sf::RectangleShape jaw({headR * 1.48f, headR * 0.88f});
        jaw.setOrigin({jaw.getSize().x * 0.5f, 0.0f});
        jaw.setPosition({cx, cy + headR * 0.24f});
        jaw.setFillColor(fill);
        jaw.setOutlineColor(outline);
        jaw.setOutlineThickness(1.1f);
        window.draw(jaw);

        sf::CircleShape eye(headR * 0.20f);
        eye.setOrigin({eye.getRadius(), eye.getRadius()});
        eye.setFillColor(sf::Color(22, 24, 30));
        eye.setPosition({cx - headR * 0.42f, cy - headR * 0.16f});
        window.draw(eye);
        eye.setPosition({cx + headR * 0.42f, cy - headR * 0.16f});
        window.draw(eye);
    };

    drawSkull(panelX + 42.f, panelY + 27.f, 1.0f, sf::Color(236, 244, 255), sf::Color(24, 43, 84));
    drawSkull(panelX + panelW - 42.f, panelY + 27.f, 1.0f, sf::Color(236, 244, 255), sf::Color(24, 43, 84));

    sf::Text title(font, "GAME WON", 58);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition({panelX + panelW * 0.5f - (tb.position.x + tb.size.x * 0.5f), panelY + 78.f});
    title.setFillColor(sf::Color(24, 47, 92));
    title.setOutlineColor(sf::Color(247, 252, 255));
    title.setOutlineThickness(1.8f);
    window.draw(title);

    sf::Text sub(font, "Third boss defeated", 36);
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setPosition({panelX + panelW * 0.5f - (sb.position.x + sb.size.x * 0.5f), panelY + 150.f});
    sub.setFillColor(sf::Color(40, 74, 138));
    window.draw(sub);

    sf::Text stats(font,
                   "Rounds cleared: " + std::to_string(roundsCleared) +
                   "    Tokens: " + std::to_string(tokensTotal),
                   28);
    sf::FloatRect stb = stats.getLocalBounds();
    stats.setPosition({panelX + panelW * 0.5f - (stb.position.x + stb.size.x * 0.5f), panelY + 202.f});
    stats.setFillColor(sf::Color(30, 58, 112));
    window.draw(stats);

    const float btnW = panelW - 96.f;
    const float btnH = 58.f;
    const float btnX = panelX + 48.f;
    const float gap = 16.f;
    const float buttonsTotalH = btnH * 3.0f + gap * 2.0f;
    const float minBtnY = stats.getPosition().y + stats.getLocalBounds().size.y + 20.0f;
    float btnY = panelY + panelH - buttonsTotalH - 18.0f;
    if (btnY < minBtnY) btnY = minBtnY;
    if (btnY + buttonsTotalH > panelY + panelH - 10.0f) {
        btnY = panelY + panelH - buttonsTotalH - 10.0f;
    }

    gameWonEndlessRect = sf::FloatRect({btnX, btnY}, {btnW, btnH});
    gameWonRestartRect = sf::FloatRect({btnX, btnY + btnH + gap}, {btnW, btnH});
    gameWonMenuRect = sf::FloatRect({btnX, btnY + (btnH + gap) * 2.0f}, {btnW, btnH});

    auto drawButton = [&](const sf::FloatRect& rect, const std::string& label, sf::Color fill, sf::Color textColor) {
        sf::RectangleShape btn(rect.size);
        btn.setPosition(rect.position);
        btn.setFillColor(fill);
        btn.setOutlineColor(sf::Color(75, 111, 186));
        btn.setOutlineThickness(2.5f);
        window.draw(btn);

        sf::Text txt(font, label, 31);
        sf::FloatRect bb = txt.getLocalBounds();
        txt.setPosition({
            rect.position.x + rect.size.x * 0.5f - (bb.position.x + bb.size.x * 0.5f),
            rect.position.y + rect.size.y * 0.5f - (bb.position.y + bb.size.y * 0.56f)
        });
        txt.setFillColor(textColor);
        txt.setOutlineColor(sf::Color(18, 33, 69));
        txt.setOutlineThickness(1.0f);
        window.draw(txt);
    };

    drawButton(gameWonEndlessRect, "ENDLESS MODE", sf::Color(96, 220, 241), sf::Color(16, 42, 90));
    drawButton(gameWonRestartRect, "RESTART RUN", sf::Color(124, 167, 248), sf::Color(18, 37, 77));
    drawButton(gameWonMenuRect, "GAME MENU", sf::Color(245, 225, 84), sf::Color(38, 34, 15));
}

GameWonAction UI::handleGameWonClick(sf::Vector2i mousePos) const {
    if (pointInRectPadded(gameWonEndlessRect, mousePos, 10.0f)) return GameWonAction::EndlessMode;
    if (pointInRectPadded(gameWonRestartRect, mousePos, 10.0f)) return GameWonAction::RestartRun;
    if (pointInRectPadded(gameWonMenuRect, mousePos, 10.0f)) return GameWonAction::GoToMenu;
    return GameWonAction::None;
}

// ─── Inventory helpers ────────────────────────────────────────────────────────

static sf::Color ballPreviewColor(BallType t) {
    switch (t) {
        case BallType::BlackHole:  return {10,  0,   20};
        case BallType::Midas:      return {210, 170, 20};
        case BallType::Upgrade:    return {30,  80,  200};
        case BallType::Heavy:      return {60,  60,  65};
        case BallType::Fastball:   return {240, 240, 240};
        case BallType::OddBall:    return {60,  180, 60};
        case BallType::EightBall:  return {10,  10,  10};
        case BallType::Icy:        return {165, 225, 255};
        case BallType::Retrigger:  return {160, 170, 180};
        default:                   return {25,  55,  140};
    }
}

static std::string ballShortName(BallType t) {
    switch (t) {
        case BallType::BlackHole:  return "Black Hole";
        case BallType::Midas:      return "Midas";
        case BallType::Upgrade:    return "Upgrade";
        case BallType::Heavy:      return "Heavy";
        case BallType::Fastball:   return "Fastball";
        case BallType::OddBall:    return "Odd Ball";
        case BallType::EightBall:  return "8-Ball";
        case BallType::Icy:        return "Icy Ball";
        case BallType::Retrigger:  return "Retrigger";
        default:                   return "Normal";
    }
}

static std::string shoeShortName(ShoeType t) {
    switch (t) {
        case ShoeType::Clown:   return "Clown Shoes";
        case ShoeType::Running: return "Running Shoes";
        case ShoeType::Moon:    return "Moon Boots";
        case ShoeType::Slippers:return "Slippers";
        case ShoeType::HighHeels:return "High Heels";
        case ShoeType::SteelCap:return "Steel Caps";
        default:                return "Bowling Shoes";
    }
}

static sf::Color shoePreviewColor(ShoeType t) {
    switch (t) {
        case ShoeType::Clown:   return {220, 40, 40};
        case ShoeType::Running: return {255, 255, 255};
        case ShoeType::Moon:    return {190, 210, 255};
        case ShoeType::Slippers:return {230, 180, 140};
        case ShoeType::HighHeels:return {255, 120, 190};
        case ShoeType::SteelCap:return {120, 130, 145};
        default:                return {120, 120, 120};
    }
}

static sf::Color pinPreviewColor(int pt) {
    switch (static_cast<PinType>(pt)) {
        case PinType::Gold:        return {210, 175, 50};
        case PinType::Mischievous: return {140, 30,  180};
        case PinType::Exploding:   return {220, 80,  20};
        case PinType::Light:       return {140, 200, 240};
        case PinType::Big:         return {160, 20,  20};
        case PinType::Ice:         return {180, 230, 255};
        case PinType::CopyCat:     return {150, 150, 155};
        case PinType::LuckyDucky:  return {230, 200, 20};
        case PinType::LevelUp:     return {100, 185, 245};
        case PinType::Lover:       return {220, 60, 110};
        case PinType::ChangeIsGood:return {225, 175, 60};
        case PinType::ThirdTime:   return {20,  160, 50};
        default:                   return {220, 220, 220};
    }
}

static std::string pinShortName(int pt) {
    switch (static_cast<PinType>(pt)) {
        case PinType::Gold:        return "Gold";
        case PinType::Mischievous: return "Mischief";
        case PinType::Exploding:   return "Exploding";
        case PinType::Light:       return "Light";
        case PinType::Big:         return "Big";
        case PinType::Ice:         return "Ice";
        case PinType::CopyCat:     return "Copy Cat";
        case PinType::LuckyDucky:  return "Lucky Ducky";
        case PinType::LevelUp:     return "Level Up";
        case PinType::Lover:       return "Lover";
        case PinType::ChangeIsGood:return "Change Is Good";
        case PinType::ThirdTime:   return "3rd Time";
        default:                   return "Normal";
    }
}

static std::string powerShortName(PowerType t) {
    switch (t) {
        case PowerType::Greedy:              return "Greedy";
        case PowerType::RandomUpgrade:       return "Random Upgrade";
        case PowerType::ExtraPins:           return "Extra Pins";
        case PowerType::ExtraBall:           return "Extra Ball";
        case PowerType::Duplicate:           return "Duplicate";
        case PowerType::Bumpers:             return "Bumpers";
        case PowerType::SwapPins:            return "Swap Pins";
        case PowerType::HomeBase:            return "Home Base";
        case PowerType::Confusion:           return "Confusion";
        case PowerType::Earthquake:          return "Earthquake";
        case PowerType::Skip:                return "Reroll";
        case PowerType::UpgradesForEveryone: return "Upgrades+3";
        case PowerType::Sales:               return "Sales";
        case PowerType::PassedGo:            return "Passed Go";
        case PowerType::MoMoney:             return "Mo Money";
        case PowerType::SevenEightNine:      return "7 8 9";
        case PowerType::ExtraPowerSlot:      return "Extra Slot";
        default:                             return "Power";
    }
}

static sf::Color powerInventoryColor(PowerType t) {
    switch (t) {
        case PowerType::Greedy:              return {240, 210, 70};
        case PowerType::RandomUpgrade:       return {120, 220, 255};
        case PowerType::ExtraPins:           return {255, 170, 70};
        case PowerType::ExtraBall:           return {120, 170, 255};
        case PowerType::Duplicate:           return {210, 150, 255};
        case PowerType::Bumpers:             return {255, 120, 120};
        case PowerType::SwapPins:            return {140, 255, 170};
        case PowerType::HomeBase:            return {255, 220, 180};
        case PowerType::Confusion:           return {200, 130, 255};
        case PowerType::Earthquake:          return {255, 140, 80};
        case PowerType::Skip:                return {255, 255, 255};
        case PowerType::UpgradesForEveryone: return {130, 255, 210};
        case PowerType::Sales:               return {120, 255, 120};
        case PowerType::PassedGo:            return {255, 240, 120};
        case PowerType::MoMoney:             return {255, 200, 70};
        case PowerType::SevenEightNine:      return {255, 170, 120};
        case PowerType::ExtraPowerSlot:      return {255, 175, 110};
        default:                             return {200, 200, 200};
    }
}

static std::string inventoryBallDesc(BallType t) {
    switch (t) {
        case BallType::BlackHole:  return "Pins drift toward ball.\n8% smaller.";
        case BallType::Midas:      return "First pin hit each shot\nturns gold. Gold pin = +1 token.";
        case BallType::Upgrade:    return "Each pin hit gains +1 value.\n10% lighter, 5% faster.";
        case BallType::Heavy:      return "15% heavier.\nKnocks pins over easier.";
        case BallType::Fastball:   return "5% lighter, 15% faster.";
        case BallType::OddBall:    return "Odd pins x2 score.\nEven pins x0.75 score.";
        case BallType::EightBall:  return "All pins are worth 8.";
        case BallType::Icy:        return "For each Ice pin hit,\n+15 score.";
        case BallType::Retrigger:  return "2nd pin hit scores 3x.";
        default:                   return "A standard bowling ball.";
    }
}

static std::string inventoryPinDesc(PinType t) {
    switch (t) {
        case PinType::Gold:        return "One pin turns gold.\nKnock it for +1 token.";
        case PinType::Mischievous: return "One pin randomises value\n1-15 each shot.";
        case PinType::Exploding:   return "One pin explodes shortly\nafter being knocked.";
        case PinType::Light:       return "One pin is worth 2\nbut easy to knock.";
        case PinType::Big:         return "One pin is worth 10\nbut hard to knock.";
        case PinType::Ice:         return "One pin slides 15% faster\nwhen fallen.";
        case PinType::CopyCat:     return "Copies the type of\nthe first pin hit.";
        case PinType::LuckyDucky:  return "Worth 20 points.\n35% chance to score 0.";
        case PinType::LevelUp:     return "This pin is always\nworth +1 extra point.";
        case PinType::Lover:       return "When hit, adds +1 value\nto the next slot pin.";
        case PinType::ChangeIsGood:return "Whenever another pin changes,\nthis pin gains +2 value.";
        case PinType::ThirdTime:   return "Every 3rd time this pin is hit:\ncombo x2 this shot.";
        default:                   return "Normal pin.";
    }
}

static std::string inventoryShoeDesc(ShoeType t) {
    switch (t) {
        case ShoeType::Clown:    return "Wobble launch angle.\n8% slower.\nFirst buy: +10 dollars.";
        case ShoeType::Running:  return "Ball launches 18% faster.";
        case ShoeType::Moon:     return "All pins are 26% lighter.";
        case ShoeType::Slippers: return "Everything slides 18% more.";
        case ShoeType::HighHeels:return "Rack spacing is 8% tighter.";
        case ShoeType::SteelCap: return "Pins cannot change\nduring a round.";
        default:                 return "Default shoe stats.";
    }
}

static std::string inventoryPowerDesc(PowerType t) {
    switch (t) {
        case PowerType::Greedy:              return "Gain +1 combo for\nevery 4 dollars.";
        case PowerType::RandomUpgrade:       return "After each frame,\nrandom pin +1 value.";
        case PowerType::ExtraPins:           return "Adds two extra pins\nto the rack.";
        case PowerType::ExtraBall:           return "Adds one extra shot\nper frame.";
        case PowerType::Duplicate:           return "Press N, then choose\nsource and target pin.";
        case PowerType::Bumpers:             return "No gutter balls\nfor the rest of run.";
        case PowerType::SwapPins:            return "Press V, then choose\ntwo pins to swap.";
        case PowerType::HomeBase:            return "Every 20 pins hit,\nbase combo +1.";
        case PowerType::Confusion:           return "Spares score like\nstrikes.";
        case PowerType::Earthquake:          return "Every 10 shots,\na free strike.";
        case PowerType::Skip:                return "Permanent:\n2 rerolls each shop.";
        case PowerType::UpgradesForEveryone: return "Future shops show\nthree extra items.";
        case PowerType::Sales:               return "Everything in shop\nis 20% cheaper.";
        case PowerType::PassedGo:            return "Round clears pay\n3 more dollars.";
        case PowerType::MoMoney:             return "Interest every 2 dollars,\nnot 3.";
        case PowerType::SevenEightNine:      return "Triples pin 7 value,\ndestroys pin 9. One use.";
        case PowerType::ExtraPowerSlot:      return "One-time:\nmax power slots 4 -> 5.";
        default:                             return "Power effect.";
    }
}

static std::string inventoryPinStatus(const ActiveItems& items, int slot, PinType type) {
    int value = 0;
    if (slot >= 1 && slot <= (int)items.pinSlotCurrentValues.size()) {
        value = items.pinSlotCurrentValues[slot - 1];
    }
    if (value <= 0) value = slot;

    std::string status = "(currently: value ";
    status += (value > 0) ? std::to_string(value) : "-";
    if (type == PinType::ThirdTime) {
        int progress = items.thirdTimeGlobalKnocks % 3;
        status += ", " + std::to_string(progress) + "/3";
    }
    status += ")";
    return status;
}

static std::string formatCompactFloat(float value, int decimals = 2) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << value;
    std::string out = ss.str();
    while (out.size() > 1 && out.back() == '0') out.pop_back();
    if (!out.empty() && out.back() == '.') out.pop_back();
    return out;
}

static std::string formatCompactScore(int value) {
    long long absValue = std::llabs(static_cast<long long>(value));
    auto withSuffix = [&](double scaled, const char* suffix) {
        std::ostringstream ss;
        if (scaled >= 100.0) ss << std::fixed << std::setprecision(0);
        else if (scaled >= 10.0) ss << std::fixed << std::setprecision(1);
        else ss << std::fixed << std::setprecision(2);
        ss << scaled;
        std::string out = ss.str();
        while (!out.empty() && out.back() == '0') out.pop_back();
        if (!out.empty() && out.back() == '.') out.pop_back();
        if (value < 0) out = "-" + out;
        out += suffix;
        return out;
    };
    if (absValue >= 1000000000LL) {
        return withSuffix((double)absValue / 1000000000.0, "B");
    }
    if (absValue >= 1000000LL) {
        return withSuffix((double)absValue / 1000000.0, "M");
    }
    if (absValue >= 10000LL) {
        return withSuffix((double)absValue / 1000.0, "K");
    }
    return std::to_string(value);
}

static std::string inventoryShoeStatus(const ActiveItems& items, ShoeType type) {
    switch (type) {
        case ShoeType::Clown:
            return items.clownBonusClaimed
                ? "(currently: +10 bonus already claimed, speed x" + formatCompactFloat(items.launchSpeedMultiplier) + ")"
                : "(currently: +10 bonus on first buy, speed x" + formatCompactFloat(items.launchSpeedMultiplier) + ")";
        case ShoeType::Running:
            return "(currently: launch speed x" + formatCompactFloat(items.launchSpeedMultiplier) + ")";
        case ShoeType::Moon:
            return "(currently: pin mass x" + formatCompactFloat(items.pinMassMultiplier) + ")";
        case ShoeType::Slippers:
            return "(currently: slide x" + formatCompactFloat(items.slideMultiplier) + ")";
        case ShoeType::HighHeels:
            return "(currently: rack spacing x0.92)";
        case ShoeType::SteelCap:
            return "(currently: lock pin changes ON)";
        default:
            return "(currently: default)";
    }
}

static std::string inventoryPowerStatus(const ActiveItems& items, PowerType t, int count) {
    switch (t) {
        case PowerType::Greedy:
            return "(currently: +1 combo / 4 dollars)";
        case PowerType::RandomUpgrade:
            return "(currently: +" + std::to_string(items.pendingRandomPinUpgrades) + " pending)";
        case PowerType::ExtraPins:
            return "(currently: rack size " + std::to_string(items.getActivePinSlotCount()) + ")";
        case PowerType::ExtraBall:
            return "(currently: +1 shot per frame)";
        case PowerType::Duplicate:
            return "(currently: " + std::to_string(items.duplicateCharges) + " charge)";
        case PowerType::Bumpers:
            return "(currently: ON)";
        case PowerType::SwapPins:
            return "(currently: " + std::to_string(items.swapCharges) + " charge)";
        case PowerType::HomeBase:
            return "(currently: +" + std::to_string((int)std::lround(items.homeBaseComboBonus)) +
                   " combo, " + std::to_string(items.homeBasePinsTowardNextCombo) + "/20)";
        case PowerType::Confusion:
            return "(currently: active)";
        case PowerType::Earthquake:
            return "(currently: " + std::to_string(items.earthquakeShotCounter) + "/10)";
        case PowerType::Skip:
            return "(currently: " + std::to_string(items.skipCharges) + " rerolls)";
        case PowerType::UpgradesForEveryone:
            return "(currently: +3 shop items)";
        case PowerType::Sales:
            return "(currently: 20% discount)";
        case PowerType::PassedGo:
            return "(currently: +3 clear payout)";
        case PowerType::MoMoney:
            return "(currently: interest every 2)";
        case PowerType::SevenEightNine:
            return "(currently: " + std::to_string(items.sevenEightNineCharges) + " charge)";
        case PowerType::ExtraPowerSlot:
            return "(currently: max " + std::to_string(items.getMaxPermanentPowerSlots()) + " slots)";
        default:
            break;
    }
    if (count > 1) {
        return "(currently: x" + std::to_string(count) + ")";
    }
    return "";
}

static std::vector<std::string> splitTooltipLines(const std::string& text) {
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back(text);
    }
    return lines;
}

// Draws a mini pin silhouette icon at (cx, cy) with given colour
static void drawMiniPin(sf::RenderWindow& window, float cx, float cy, float size, sf::Color color) {
    // Simplified pin shape: small oval body + tiny head
    float bodyH = size * 1.6f;
    float bodyW = size * 0.55f;

    // Body
    sf::CircleShape body(bodyW);
    body.setOrigin({bodyW, bodyW});
    body.setPosition({cx, cy + size * 0.3f});
    body.setFillColor(color);
    body.setOutlineColor({0,0,0,120});
    body.setOutlineThickness(1.5f);
    window.draw(body);

    // Neck (thin rectangle)
    sf::RectangleShape neck({bodyW * 0.55f, size * 0.5f});
    neck.setOrigin({bodyW * 0.275f, 0});
    neck.setPosition({cx, cy - size * 0.3f});
    neck.setFillColor(color);
    window.draw(neck);

    // Head
    float headR = bodyW * 0.5f;
    sf::CircleShape head(headR);
    head.setOrigin({headR, headR});
    head.setPosition({cx, cy - size * 0.7f});
    head.setFillColor(color);
    head.setOutlineColor({0,0,0,120});
    head.setOutlineThickness(1.5f);
    window.draw(head);

    // Highlight
    sf::CircleShape hl(headR * 0.4f);
    hl.setOrigin({headR * 0.4f, headR * 0.4f});
    hl.setPosition({cx - headR*0.2f, cy - size*0.8f});
    hl.setFillColor({255,255,255,80});
    window.draw(hl);
}

// Core inventory panel — draws ball + pins into a rect starting at (x,y) with given width
void UI::drawInventoryPanel(sf::RenderWindow& window, const ActiveItems& items,
                             float x, float y, float width) {
    if (!fontLoaded) return;

    struct HoverTooltipEntry {
        sf::FloatRect rect{};
        std::string title;
        std::string description;
        std::string status;
        sf::Color accent = sf::Color::White;
    };
    std::vector<HoverTooltipEntry> hoverEntries;
    hoverEntries.reserve(48);
    auto addHover = [&](const sf::FloatRect& rect,
                        const std::string& title,
                        const std::string& desc,
                        const std::string& status,
                        sf::Color accent) {
        hoverEntries.push_back({rect, title, desc, status, accent});
    };

    float iy = y;
    const sf::Color sectionColor = sf::Color(23, 46, 96);
    const sf::Color subLabelColor = sf::Color(58, 82, 138);
    const sf::Color nameColor = sf::Color(20, 40, 84);
    const sf::Color textNormal = sf::Color(39, 63, 113);
    const sf::Color textSpecial = sf::Color(26, 51, 101);

    // ── Ball section (shot slots) ───────────────────────────────────────────
    sf::Text ballLabel(font, "BALLS", 20);
    ballLabel.setPosition({x + 10.f, iy});
    ballLabel.setFillColor(sectionColor);
    window.draw(ballLabel);
    iy += 28.f;

    bool hasExtraBallSlot =
        items.powerExtraBall || items.hasPurchasedPower(PowerType::ExtraBall);
    int ballSlotCount = hasExtraBallSlot ? 3 : 2;
    float ballR = hasExtraBallSlot ? 18.f : 22.f;

    auto drawBallSlot = [&](int slot, float sx, BallType bt) {
        sf::Text slotLabel(font, "Ball " + std::to_string(slot), 14);
        sf::FloatRect slb = slotLabel.getLocalBounds();
        slotLabel.setPosition({sx - slb.size.x * 0.5f, iy});
        slotLabel.setFillColor(subLabelColor);
        window.draw(slotLabel);

        sf::CircleShape ballCircle(ballR);
        ballCircle.setOrigin({ballR, ballR});
        ballCircle.setPosition({sx, iy + 20.f + ballR});
        ballCircle.setFillColor(ballPreviewColor(bt));
        ballCircle.setOutlineColor({255,255,255,60});
        ballCircle.setOutlineThickness(2.f);
        window.draw(ballCircle);

        sf::Text ballName(font, ballShortName(bt), 14);
        sf::FloatRect nb = ballName.getLocalBounds();
        ballName.setPosition({sx - nb.size.x * 0.5f, iy + 20.f + ballR * 2.f + 4.f});
        ballName.setFillColor(nameColor);
        window.draw(ballName);

        float rectW = std::max(90.f, ballR * 2.f + 58.f);
        float rectH = ballR * 2.f + 46.f;
        addHover(sf::FloatRect({sx - rectW * 0.5f, iy - 2.f}, {rectW, rectH}),
                 "Ball " + std::to_string(slot) + " " + ballShortName(bt),
                 inventoryBallDesc(bt),
                 "",
                 ballPreviewColor(bt));
    };

    if (ballSlotCount == 3) {
        drawBallSlot(1, x + width * 0.20f, items.getBallForSlot(1));
        drawBallSlot(2, x + width * 0.50f, items.getBallForSlot(2));
        drawBallSlot(3, x + width * 0.80f, items.getBallForSlot(3));
    } else {
        drawBallSlot(1, x + width * 0.28f, items.getBallForSlot(1));
        drawBallSlot(2, x + width * 0.72f, items.getBallForSlot(2));
    }
    iy += ballR * 2.f + 52.f;

    // ── Shoes section ────────────────────────────────────────────────────────
    sf::Text shoeLabel(font, "SHOES", 20);
    shoeLabel.setPosition({x + 10.f, iy});
    shoeLabel.setFillColor(sectionColor);
    window.draw(shoeLabel);
    iy += 24.f;

    sf::CircleShape shoeCircle(12.f);
    shoeCircle.setOrigin({12.f, 12.f});
    shoeCircle.setPosition({x + 26.f, iy + 12.f});
    shoeCircle.setFillColor(shoePreviewColor(items.shoeType));
    shoeCircle.setOutlineColor({255,255,255,80});
    shoeCircle.setOutlineThickness(2.f);
    window.draw(shoeCircle);

    sf::Text shoeName(font, shoeShortName(items.shoeType), 15);
    shoeName.setPosition({x + 46.f, iy + 2.f});
    shoeName.setFillColor(nameColor);
    window.draw(shoeName);
    addHover(sf::FloatRect({x + 8.f, iy - 2.f}, {width - 16.f, 30.f}),
             shoeShortName(items.shoeType),
             inventoryShoeDesc(items.shoeType),
             inventoryShoeStatus(items, items.shoeType),
             shoePreviewColor(items.shoeType));
    iy += 32.f;

    // ── Pins section (always show all slots with live values) ───────────────
    sf::Text pinsLabel(font, "PINS", 20);
    pinsLabel.setPosition({x + 10.f, iy});
    pinsLabel.setFillColor(sectionColor);
    window.draw(pinsLabel);
    iy += 28.f;

    int slotCount = std::clamp(items.getActivePinSlotCount(), 1, 12);
    bool twoColumnPins = slotCount > 8;
    const float lineH = 18.f;
    const float pinColW = twoColumnPins ? (width * 0.49f) : (width - 16.f);
    for (int slot = 1; slot <= slotCount; slot++) {
        int index = slot - 1;
        int row = twoColumnPins ? (index / 2) : index;
        int col = twoColumnPins ? (index % 2) : 0;
        float rowY = iy + row * lineH;
        float rowX = x + 10.f + col * pinColW;

        PinType slotType = items.getPinTypeForSlot(slot);
        int pt = static_cast<int>(slotType);
        int shownValue = items.pinSlotCurrentValues[index];
        if (shownValue <= 0) shownValue = slot;

        float iconX = rowX + 8.f;
        drawMiniPin(window, iconX, rowY + 8.f, 10.5f, pinPreviewColor(pt));

        std::string label = "P" + std::to_string(slot) + " " + pinShortName(pt);
        sf::Text pinText(font, label, 13);
        pinText.setPosition({rowX + 24.f, rowY});
        pinText.setFillColor((slotType == PinType::Normal)
                             ? textNormal
                             : textSpecial);
        window.draw(pinText);

        addHover(sf::FloatRect({rowX + 4.f, rowY - 1.f}, {pinColW - 6.f, lineH}),
                 "P" + std::to_string(slot) + " " + pinShortName(pt),
                 inventoryPinDesc(slotType),
                 inventoryPinStatus(items, slot, slotType),
                 pinPreviewColor(pt));
    }
    int pinRows = twoColumnPins ? ((slotCount + 1) / 2) : slotCount;
    iy += pinRows * lineH;
    iy += 8.f;

    const int powerCount = static_cast<int>(PowerType::ExtraPowerSlot) + 1;
    std::vector<int> counts(powerCount, 0);
    for (int raw : items.purchasedPowers) {
        if (raw >= 0 && raw < powerCount) counts[raw]++;
    }

    // Keep inventory display aligned with live power state, even if purchase
    // history ever goes out of sync.
    auto forceOwned = [&](PowerType p, bool owned) {
        if (owned) counts[(int)p] = std::max(counts[(int)p], 1);
    };
    forceOwned(PowerType::Greedy, items.powerGreedy);
    forceOwned(PowerType::RandomUpgrade, items.powerRandomUpgrade);
    forceOwned(PowerType::ExtraPins,
               items.powerExtraPins || items.hasPurchasedPower(PowerType::ExtraPins));
    forceOwned(PowerType::ExtraBall,
               items.powerExtraBall || items.hasPurchasedPower(PowerType::ExtraBall));
    forceOwned(PowerType::Bumpers, items.powerBumpers);
    forceOwned(PowerType::HomeBase, items.powerHomeBase);
    forceOwned(PowerType::Confusion, items.powerConfusion);
    forceOwned(PowerType::Earthquake, items.powerEarthquake);
    forceOwned(PowerType::Skip, items.powerSkip || items.hasPurchasedPower(PowerType::Skip));
    forceOwned(PowerType::UpgradesForEveryone, items.powerUpgradesForEveryone);
    forceOwned(PowerType::Sales, items.powerSales);
    forceOwned(PowerType::PassedGo, items.powerPassedGo);
    forceOwned(PowerType::MoMoney, items.powerMoMoney);
    forceOwned(PowerType::ExtraPowerSlot, items.powerExtraPowerSlot);

    counts[(int)PowerType::Duplicate] = items.duplicateCharges;
    counts[(int)PowerType::SwapPins] = items.swapCharges;
    counts[(int)PowerType::SevenEightNine] = items.sevenEightNineCharges;
    counts[(int)PowerType::Skip] = items.skipCharges;

    bool hasShownPowers = false;
    for (int c : counts) {
        if (c > 0) { hasShownPowers = true; break; }
    }

    if (hasShownPowers) {
        // ── Powers section ────────────────────────────────────────────────────
        sf::Text powersLabel(font, "POWERS", 20);
        powersLabel.setPosition({x + 10.f, iy});
        powersLabel.setFillColor(sectionColor);
        window.draw(powersLabel);
        iy += 26.f;

        const float colW = width * 0.48f;
        int shown = 0;
        for (int p = 0; p < powerCount; p++) {
            if (counts[p] <= 0) continue;
            int row = shown / 2;
            int col = shown % 2;

            std::string label = powerShortName(static_cast<PowerType>(p));
            if (counts[p] > 1) label += " x" + std::to_string(counts[p]);

            sf::Text powerText(font, label, 14);
            powerText.setPosition({x + 10.f + col * colW, iy + row * 18.f});
            powerText.setFillColor(textSpecial);
            window.draw(powerText);
            addHover(sf::FloatRect({x + 8.f + col * colW, iy + row * 18.f - 1.f},
                                   {colW - 8.f, 18.f}),
                     powerShortName(static_cast<PowerType>(p)),
                     inventoryPowerDesc(static_cast<PowerType>(p)),
                     inventoryPowerStatus(items, static_cast<PowerType>(p), counts[p]),
                     powerInventoryColor(static_cast<PowerType>(p)));
            shown++;
        }
    }

    sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    auto isInside = [](const sf::FloatRect& r, sf::Vector2f p) {
        return p.x >= r.position.x && p.x <= r.position.x + r.size.x &&
               p.y >= r.position.y && p.y <= r.position.y + r.size.y;
    };

    const HoverTooltipEntry* hovered = nullptr;
    for (const auto& entry : hoverEntries) {
        if (isInside(entry.rect, mouse)) {
            hovered = &entry;
        }
    }

    if (!hovered) return;

    const int titleSize = 18;
    const int bodySize = 14;
    const int statusSize = 13;
    std::vector<std::string> bodyLines = splitTooltipLines(hovered->description);

    float maxTextW = 0.f;
    auto updateWidth = [&](const std::string& text, int size) {
        if (text.empty()) return;
        sf::Text t(font, text, size);
        sf::FloatRect b = t.getLocalBounds();
        maxTextW = std::max(maxTextW, b.size.x);
    };

    updateWidth(hovered->title, titleSize);
    for (const auto& line : bodyLines) updateWidth(line, bodySize);
    updateWidth(hovered->status, statusSize);

    float panelW = std::clamp(maxTextW + 22.f, 190.f, 360.f);
    float panelH = 16.f + 24.f + (float)bodyLines.size() * 18.f + 10.f;
    if (!hovered->status.empty()) panelH += 18.f;

    sf::View currentView = window.getView();
    float viewLeft = currentView.getCenter().x - currentView.getSize().x * 0.5f;
    float viewTop = currentView.getCenter().y - currentView.getSize().y * 0.5f;
    float viewRight = viewLeft + currentView.getSize().x;
    float viewBottom = viewTop + currentView.getSize().y;

    auto clampWithFallback = [](float value, float minV, float maxV) {
        if (maxV < minV) return minV;
        return std::clamp(value, minV, maxV);
    };

    float tipX = mouse.x + 14.f;
    float tipY = mouse.y + 14.f;
    if (tipX + panelW > viewRight - 4.f) tipX = mouse.x - panelW - 14.f;
    if (tipY + panelH > viewBottom - 4.f) tipY = mouse.y - panelH - 14.f;
    tipX = clampWithFallback(tipX, viewLeft + 4.f, viewRight - panelW - 4.f);
    tipY = clampWithFallback(tipY, viewTop + 4.f, viewBottom - panelH - 4.f);

    sf::RectangleShape shadow({panelW, panelH});
    shadow.setPosition({tipX + 2.f, tipY + 2.f});
    shadow.setFillColor({0, 0, 0, 70});
    window.draw(shadow);

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({tipX, tipY});
    panel.setFillColor({232, 241, 253, 248});
    panel.setOutlineColor({hovered->accent.r, hovered->accent.g, hovered->accent.b, 210});
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    sf::RectangleShape accentBar({panelW - 4.f, 4.f});
    accentBar.setPosition({tipX + 2.f, tipY + 2.f});
    accentBar.setFillColor({hovered->accent.r, hovered->accent.g, hovered->accent.b, 215});
    window.draw(accentBar);

    sf::Text title(font, hovered->title, titleSize);
    title.setPosition({tipX + 10.f, tipY + 8.f});
    title.setFillColor({20, 39, 82});
    window.draw(title);

    float textY = tipY + 33.f;
    for (const auto& line : bodyLines) {
        sf::Text body(font, line, bodySize);
        body.setPosition({tipX + 10.f, textY});
        body.setFillColor({37, 58, 104});
        window.draw(body);
        textY += 18.f;
    }

    if (!hovered->status.empty()) {
        sf::Text status(font, hovered->status, statusSize);
        status.setPosition({tipX + 10.f, textY + 2.f});
        status.setFillColor({30, 133, 126});
        window.draw(status);
    }
}

// Bottom bar shown during gameplay
void UI::drawInventoryBar(sf::RenderWindow& window, const ActiveItems& items,
                           float windowW, float windowH) {
    if (!fontLoaded) return;
    if (state != GameState::Xtreme) return;
    // Only show if player has something non-default
    bool hasExtraBallSlot =
        items.powerExtraBall || items.hasPurchasedPower(PowerType::ExtraBall);
    bool hasBall = (items.getBallForSlot(1) != BallType::Normal) ||
                   (items.getBallForSlot(2) != BallType::Normal) ||
                   (hasExtraBallSlot && items.getBallForSlot(3) != BallType::Normal);
    bool hasShoe = (items.shoeType != ShoeType::None);
    bool hasPins = !items.pinSlotAssignments.empty();
    if (!hasBall && !hasPins && !hasShoe) return;

    float barH   = 70.f;
    float barY   = windowH - barH;
    float iconSize = 20.f;
    float padding  = 12.f;

    // Semi-transparent bar background
    sf::RectangleShape bar({windowW, barH});
    bar.setPosition({0.f, barY});
    bar.setFillColor({20, 20, 30, 200});
    bar.setOutlineColor({80, 80, 100});
    bar.setOutlineThickness(2.f);
    window.draw(bar);

    float cx = padding + iconSize;

    // Ball icon
    if (hasBall) {
        float br = iconSize * 0.82f;
        int rowCount = hasExtraBallSlot ? 3 : 2;
        float rowStep = 11.f;
        float startY = barY + barH * 0.5f - rowStep * (rowCount - 1) * 0.5f;
        float maxNameW = 0.0f;

        for (int slot = 1; slot <= rowCount; slot++) {
            float rowY = startY + (slot - 1) * rowStep;
            sf::CircleShape ballIcon(br);
            ballIcon.setOrigin({br, br});
            ballIcon.setPosition({cx, rowY});
            ballIcon.setFillColor(ballPreviewColor(items.getBallForSlot(slot)));
            ballIcon.setOutlineColor({255, 255, 255, 60});
            ballIcon.setOutlineThickness(2.f);
            window.draw(ballIcon);

            sf::Text label(font, "Ball " + std::to_string(slot) + " " +
                                     ballShortName(items.getBallForSlot(slot)), 12);
            label.setPosition({cx + br + 4.f, rowY - 9.f});
            label.setFillColor(sf::Color::White);
            window.draw(label);

            sf::FloatRect lb = label.getLocalBounds();
            maxNameW = std::max(maxNameW, lb.size.x);
        }

        cx += br + maxNameW + padding * 2.f + 10.f;

        // Divider
        if (hasPins) {
            sf::RectangleShape div({2.f, barH * 0.6f});
            div.setPosition({cx, barY + barH * 0.2f});
            div.setFillColor({100, 100, 120});
            window.draw(div);
            cx += padding;
        }
    }

    if (hasShoe) {
        float sr = iconSize * 0.9f;
        sf::CircleShape shoe(sr);
        shoe.setOrigin({sr, sr});
        shoe.setPosition({cx, barY + barH/2.f});
        shoe.setFillColor(shoePreviewColor(items.shoeType));
        shoe.setOutlineColor({255,255,255,70});
        shoe.setOutlineThickness(2.f);
        window.draw(shoe);

        sf::Text sn(font, shoeShortName(items.shoeType), 13);
        sn.setPosition({cx + sr + 5.f, barY + barH/2.f - 9.f});
        sn.setFillColor(sf::Color::White);
        window.draw(sn);

        sf::FloatRect snb = sn.getLocalBounds();
        cx += sr + snb.size.x + padding * 2.f + 8.f;

        if (hasPins) {
            sf::RectangleShape div({2.f, barH * 0.6f});
            div.setPosition({cx, barY + barH * 0.2f});
            div.setFillColor({100, 100, 120});
            window.draw(div);
            cx += padding;
        }
    }

    // Pin icons
    std::vector<ActiveItems::PinSlotAssignment> sortedPins = items.getSortedPinAssignments();
    for (const auto& assigned : sortedPins) {
        int pt = static_cast<int>(assigned.type);
        drawMiniPin(window, cx, barY + barH/2.f, iconSize * 0.85f, pinPreviewColor(pt));

        sf::Text pn(font, pinShortName(pt), 13);
        pn.setPosition({cx + iconSize + 2.f, barY + barH/2.f - 9.f});
        pn.setFillColor({210, 210, 210});
        window.draw(pn);

        sf::FloatRect pnb = pn.getLocalBounds();
        cx += iconSize + pnb.size.x + padding * 2.f;
    }
}

struct ShopOwnedPanelLayout {
    float panelX = 0.f;
    float panelY = 0.f;
    float panelW = 0.f;
    float panelH = 0.f;
    sf::FloatRect sellBall1;
    sf::FloatRect sellBall2;
    sf::FloatRect sellBall3;
    sf::FloatRect sellShoe;
    bool hasExtraBallSlot = false;
};

struct ShopOwnedDynamicLayout {
    std::vector<sf::FloatRect> pinSellRects;
    std::vector<sf::FloatRect> powerSellRects;
    std::vector<PowerType> sellablePowers;
    float pinHeaderY = 0.f;
    float powerHeaderY = 0.f;
    float contentTop = 0.f;
};

struct ShopCardLayout {
    float cardW = 210.f;
    float cardH = 340.f;
    float cardGap = 20.f;
    float firstCardY = 150.f;
    float areaMinX = 28.f;
    float areaMaxX = 28.f;
    float areaW = 210.f;
    int cardsPerRow = 1;
};

static bool pointInRect(const sf::FloatRect& rect, sf::Vector2i point, float pad = 0.0f) {
    return pointInRectPadded(rect, point, pad);
}

static ShopOwnedPanelLayout computeShopOwnedPanelLayout(float windowW, float windowH,
                                                        std::size_t offerCount,
                                                        const ActiveItems& items) {
    ShopOwnedPanelLayout layout;
    layout.panelW = std::clamp(windowW * 0.24f, 270.f, 330.f);
    layout.panelH = std::clamp(windowH - 130.f, 430.f, 760.f);
    if (layout.panelH < 430.f) layout.panelH = 430.f;
    layout.panelX = windowW - layout.panelW - 18.f;
    layout.panelY = 118.f;
    if (offerCount > 4) {
        // Keep the panel on the right in larger shops, but tighten it slightly
        // so extra cards still have room.
        layout.panelW = std::clamp(windowW * 0.21f, 240.f, 285.f);
        layout.panelH = std::clamp(windowH - 120.f, 420.f, 760.f);
        if (layout.panelH < 410.f) layout.panelH = 410.f;
        layout.panelX = windowW - layout.panelW - 16.f;
        layout.panelY = 110.f;
    }
    if (layout.panelY + layout.panelH > windowH - 16.f) {
        layout.panelY = std::max(40.f, windowH - layout.panelH - 16.f);
    }

    layout.hasExtraBallSlot =
        items.powerExtraBall || items.hasPurchasedPower(PowerType::ExtraBall);

    const float pad = 12.f;
    const float gap = 8.f;
    const float btnH = 34.f;
    const float actionTop = layout.panelY + 52.f;
    const float actionW = layout.panelW - pad * 2.f;
    const float halfW = (actionW - gap) * 0.5f;

    layout.sellBall1 = sf::FloatRect({layout.panelX + pad, actionTop}, {halfW, btnH});
    layout.sellBall2 = sf::FloatRect({layout.panelX + pad + halfW + gap, actionTop}, {halfW, btnH});
    if (layout.hasExtraBallSlot) {
        layout.sellBall3 = sf::FloatRect({layout.panelX + pad, actionTop + btnH + gap}, {actionW, btnH});
        layout.sellShoe  = sf::FloatRect({layout.panelX + pad, actionTop + (btnH + gap) * 2.0f}, {actionW, btnH});
    } else {
        layout.sellBall3 = sf::FloatRect({layout.panelX + pad, actionTop + btnH + gap}, {0.f, 0.f});
        layout.sellShoe  = sf::FloatRect({layout.panelX + pad, actionTop + btnH + gap}, {actionW, btnH});
    }
    return layout;
}

static ShopCardLayout computeShopCardLayout(float windowW,
                                            const ShopOwnedPanelLayout& ownedLayout,
                                            std::size_t offerCount) {
    ShopCardLayout out;
    const bool compact = offerCount > 4;
    if (compact) {
        out.cardW = 178.f;
        out.cardH = 246.f;
        out.cardGap = 14.f;
        out.firstCardY = 146.f;
    }
    // Reserve the left control column (ball slot + pin slot controls), so cards
    // never cover it in smaller windowed layouts.
    out.areaMinX = std::max(out.areaMinX, compact ? 210.f : 250.f);
    out.areaMaxX = windowW - 28.f;

    float panelMid = ownedLayout.panelX + ownedLayout.panelW * 0.5f;
    if (panelMid >= windowW * 0.5f) {
        // Inventory panel on right.
        out.areaMaxX = std::min(out.areaMaxX, ownedLayout.panelX - 22.f);
    } else {
        // Inventory panel on left.
        out.areaMinX = std::max(out.areaMinX, ownedLayout.panelX + ownedLayout.panelW + 22.f);
    }

    if (out.areaMaxX < out.areaMinX + out.cardW) {
        out.areaMaxX = out.areaMinX + out.cardW;
    }
    out.areaW = out.areaMaxX - out.areaMinX;
    if (out.areaW < out.cardW) out.areaW = out.cardW;

    int maxCardsPerRow = 4;

    if (out.areaW > out.cardW) {
        out.cardsPerRow = std::min(maxCardsPerRow,
            std::max(1, static_cast<int>((out.areaW + out.cardGap) / (out.cardW + out.cardGap))));
    }
    int offerCountInt = static_cast<int>(std::max<std::size_t>(1, offerCount));
    out.cardsPerRow = std::min(out.cardsPerRow, offerCountInt);
    return out;
}

static int ballTypeCost(BallType t);
static int pinTypeCost(PinType t);
static int shoeTypeCost(ShoeType t);
static int powerTypeCost(PowerType t);

static std::vector<PowerType> buildSellablePowerList(const ActiveItems& items) {
    const int powerCount = static_cast<int>(PowerType::ExtraPowerSlot) + 1;
    std::vector<PowerType> out;
    out.reserve(powerCount);
    for (int p = 0; p < powerCount; p++) {
        PowerType t = static_cast<PowerType>(p);
        if (!items.hasPower(t)) continue;
        if (t == PowerType::ExtraPowerSlot) continue; // one-time, not sellable

        int count = 1;
        if (t == PowerType::Duplicate) count = std::max(0, items.duplicateCharges);
        else if (t == PowerType::SwapPins) count = std::max(0, items.swapCharges);
        else if (t == PowerType::SevenEightNine) count = std::max(0, items.sevenEightNineCharges);

        for (int i = 0; i < count; i++) out.push_back(t);
    }
    return out;
}

static ShopOwnedDynamicLayout computeShopOwnedDynamicLayout(const ShopOwnedPanelLayout& layout,
                                                            const ActiveItems& items) {
    ShopOwnedDynamicLayout out;
    out.sellablePowers = buildSellablePowerList(items);

    const float pad = 12.f;
    const float gap = 8.f;
    const float headerH = 18.f;
    const float rowH = 24.f;
    const float listW = layout.panelW - pad * 2.f;
    const float colW = (listW - gap) * 0.5f;
    float y = layout.sellShoe.position.y + layout.sellShoe.size.y + 12.f;

    out.pinHeaderY = y;
    y += headerH;
    int pinCount = static_cast<int>(items.getSortedPinAssignments().size());
    out.pinSellRects.reserve(pinCount);
    for (int i = 0; i < pinCount; i++) {
        int row = i / 2;
        int col = i % 2;
        float x = layout.panelX + pad + col * (colW + gap);
        float ry = y + row * rowH;
        out.pinSellRects.emplace_back(sf::FloatRect({x, ry}, {colW, rowH}));
    }
    y += ((pinCount + 1) / 2) * rowH + 10.f;

    out.powerHeaderY = y;
    y += headerH;
    int powerCount = static_cast<int>(out.sellablePowers.size());
    out.powerSellRects.reserve(powerCount);
    for (int i = 0; i < powerCount; i++) {
        int row = i / 2;
        int col = i % 2;
        float x = layout.panelX + pad + col * (colW + gap);
        float ry = y + row * rowH;
        out.powerSellRects.emplace_back(sf::FloatRect({x, ry}, {colW, rowH}));
    }
    y += ((powerCount + 1) / 2) * rowH + 12.f;

    out.contentTop = y;
    float minContentTop = layout.panelY + 240.f;
    float maxContentTop = layout.panelY + layout.panelH - 40.f;
    if (out.contentTop < minContentTop) out.contentTop = minContentTop;
    if (out.contentTop > maxContentTop) out.contentTop = maxContentTop;
    return out;
}

// Inventory panel shown inside the shop screen
void UI::drawInventoryInShop(sf::RenderWindow& window, const ActiveItems& items,
                              float windowW, float windowH) {
    if (!fontLoaded) return;
    if (state != GameState::Shop) return;

    ShopOwnedPanelLayout layout = computeShopOwnedPanelLayout(windowW, windowH, shopOffers.size(), items);

    sf::RectangleShape panel({layout.panelW, layout.panelH});
    panel.setPosition({layout.panelX, layout.panelY});
    panel.setFillColor({220, 233, 252, 248});
    panel.setOutlineColor({96, 128, 196});
    panel.setOutlineThickness(2.2f);
    window.draw(panel);

    sf::RectangleShape head({layout.panelW, 36.f});
    head.setPosition({layout.panelX, layout.panelY});
    head.setFillColor({102, 150, 230, 245});
    window.draw(head);

    sf::Text title(font, "OWNED INVENTORY", 24);
    title.setPosition({layout.panelX + 12.f, layout.panelY + 4.f});
    title.setFillColor({20, 39, 80});
    window.draw(title);

    ShopOwnedDynamicLayout dynamic = computeShopOwnedDynamicLayout(layout, items);

    const float actionAreaY = layout.sellBall1.position.y - 8.f;
    const float actionAreaH = (dynamic.contentTop - actionAreaY) - 8.f;
    if (actionAreaH > 20.f) {
        sf::RectangleShape actionArea({layout.panelW - 14.f, actionAreaH});
        actionArea.setPosition({layout.panelX + 7.f, actionAreaY});
        actionArea.setFillColor({203, 220, 246, 236});
        actionArea.setOutlineColor({119, 145, 202, 190});
        actionArea.setOutlineThickness(1.4f);
        window.draw(actionArea);
    }

    BallType slot1Ball = items.getBallForSlot(1);
    BallType slot2Ball = items.getBallForSlot(2);
    BallType slot3Ball = items.getBallForSlot(3);
    bool canSellS1 = (slot1Ball != BallType::Normal);
    bool canSellS2 = (slot2Ball != BallType::Normal);
    bool canSellS3 = layout.hasExtraBallSlot && (slot3Ball != BallType::Normal);
    bool canSellShoe = (items.shoeType != ShoeType::None);
    int s1Value = canSellS1 ? (ballTypeCost(slot1Ball) / 2) : 0;
    int s2Value = canSellS2 ? (ballTypeCost(slot2Ball) / 2) : 0;
    int s3Value = canSellS3 ? (ballTypeCost(slot3Ball) / 2) : 0;
    int shoeValue = canSellShoe ? (shoeTypeCost(items.shoeType) / 2) : 0;

    sf::Text quick(font, "QUICK SELL", 14);
    quick.setPosition({layout.panelX + 14.f, layout.sellBall1.position.y - 20.f});
    quick.setFillColor({41, 66, 115});
    window.draw(quick);

    auto drawSellButton = [&](const sf::FloatRect& rect, const std::string& label, bool enabled, int charSize) {
        sf::RectangleShape button(rect.size);
        button.setPosition(rect.position);
        button.setFillColor(enabled ? sf::Color(252, 224, 74) : sf::Color(183, 199, 232));
        button.setOutlineColor(enabled ? sf::Color(234, 75, 88) : sf::Color(124, 146, 200));
        button.setOutlineThickness(2.f);
        window.draw(button);

        sf::Text text(font, label, charSize);
        text.setPosition({rect.position.x + 9.f, rect.position.y + (rect.size.y - (float)charSize) * 0.5f - 2.f});
        text.setFillColor(enabled ? sf::Color(23, 40, 82) : sf::Color(81, 102, 150));
        window.draw(text);
    };

    drawSellButton(layout.sellBall1, "Sell Ball 1 (+" + std::to_string(s1Value) + ")", canSellS1, 15);
    drawSellButton(layout.sellBall2, "Sell Ball 2 (+" + std::to_string(s2Value) + ")", canSellS2, 15);
    if (layout.hasExtraBallSlot) {
        drawSellButton(layout.sellBall3, "Sell Ball 3 (+" + std::to_string(s3Value) + ")", canSellS3, 15);
    }
    drawSellButton(layout.sellShoe, "Sell Shoe (+" + std::to_string(shoeValue) + ")", canSellShoe, 18);

    sf::Text pinHeader(font, "SELL PINS", 15);
    pinHeader.setPosition({layout.panelX + 10.f, dynamic.pinHeaderY});
    pinHeader.setFillColor({34, 58, 108});
    window.draw(pinHeader);

    std::vector<ActiveItems::PinSlotAssignment> sortedPins = items.getSortedPinAssignments();
    for (int i = 0; i < (int)dynamic.pinSellRects.size() && i < (int)sortedPins.size(); i++) {
        PinType type = sortedPins[i].type;
        int raw = static_cast<int>(type);
        int value = pinTypeCost(type) / 2;
        std::string label = "P" + std::to_string(sortedPins[i].slot) + " " +
                            pinShortName(raw) + " +" + std::to_string(value);
        drawSellButton(dynamic.pinSellRects[i], label, true, 16);
    }
    if (dynamic.pinSellRects.empty()) {
        sf::Text none(font, "None", 14);
        none.setPosition({layout.panelX + 12.f, dynamic.pinHeaderY + 14.f});
        none.setFillColor({78, 99, 142});
        window.draw(none);
    }

    sf::Text powerHeader(font, "SELL POWERS", 15);
    powerHeader.setPosition({layout.panelX + 10.f, dynamic.powerHeaderY});
    powerHeader.setFillColor({34, 58, 108});
    window.draw(powerHeader);

    for (int i = 0; i < (int)dynamic.powerSellRects.size(); i++) {
        PowerType p = dynamic.sellablePowers[i];
        int value = powerTypeCost(p) / 2;
        std::string label = powerShortName(p) + " +" + std::to_string(value);
        drawSellButton(dynamic.powerSellRects[i], label, true, 16);
    }
    if (dynamic.powerSellRects.empty()) {
        sf::Text none(font, "None", 14);
        none.setPosition({layout.panelX + 12.f, dynamic.powerHeaderY + 14.f});
        none.setFillColor({78, 99, 142});
        window.draw(none);
    }

    // Clip inventory content to the panel interior so large loadouts never
    // draw outside the box.
    const float clipPadX = 6.f;
    const float clipPadBottom = 8.f;
    const float clipX = layout.panelX + clipPadX;
    const float clipY = dynamic.contentTop;
    const float clipW = layout.panelW - clipPadX * 2.f;
    const float clipH = layout.panelH - (dynamic.contentTop - layout.panelY) - clipPadBottom;

    if (clipW > 1.f && clipH > 1.f) {
        sf::View prevView = window.getView();
        sf::View clipView(sf::FloatRect(sf::Vector2f(clipX, clipY), sf::Vector2f(clipW, clipH)));
        sf::FloatRect baseVp = prevView.getViewport();
        sf::FloatRect clipVp(
            sf::Vector2f(
                baseVp.position.x + (clipX / windowW) * baseVp.size.x,
                baseVp.position.y + (clipY / windowH) * baseVp.size.y),
            sf::Vector2f(
                (clipW / windowW) * baseVp.size.x,
                (clipH / windowH) * baseVp.size.y));
        clipView.setViewport(clipVp);
        window.setView(clipView);

        drawInventoryPanel(window, items, layout.panelX + 6.f, dynamic.contentTop, layout.panelW - 12.f);

        window.setView(prevView);
    }
}

// ─── Shop ─────────────────────────────────────────────────────────────────────

static std::string ballTypeName(BallType t) {
    switch (t) {
        case BallType::BlackHole:  return "Black Hole";
        case BallType::Midas:      return "Midas Ball";
        case BallType::Upgrade:    return "Upgrade Ball";
        case BallType::Heavy:      return "Heavy Ball";
        case BallType::Fastball:   return "Fastball";
        case BallType::OddBall:    return "Odd Ball";
        case BallType::EightBall:  return "8-Ball";
        case BallType::Icy:        return "Icy Ball";
        case BallType::Retrigger:  return "Retrigger";
        default:                   return "Normal";
    }
}

static std::string ballTypeDesc(BallType t) {
    switch (t) {
        case BallType::BlackHole:  return "Pins drift toward\nball. 8% smaller.";
        case BallType::Midas:      return "First pin hit each shot\nturns gold. Gold pin = +1 token.";
        case BallType::Upgrade:    return "Each pin hit gains\n+1 value. 10% lighter,\n5% faster.";
        case BallType::Heavy:      return "15% heavier.\nKnocks pins over easier.";
        case BallType::Fastball:   return "5% lighter, 15% faster.";
        case BallType::OddBall:    return "Odd pins x2 score.\nEven pins x0.75 score.";
        case BallType::EightBall:  return "All pins worth 8.";
        case BallType::Icy:        return "Each Ice pin hit\ngives +15 score.";
        case BallType::Retrigger:  return "2nd pin hit scores 3x.";
        default:                   return "A standard bowling ball.";
    }
}

static int ballTypeCost(BallType t) {
    switch (t) {
        case BallType::BlackHole:  return 4;
        case BallType::Midas:      return 5;
        case BallType::Upgrade:    return 3;
        case BallType::Heavy:      return 2;
        case BallType::Fastball:   return 2;
        case BallType::OddBall:    return 3;
        case BallType::EightBall:  return 4;
        case BallType::Icy:        return 4;
        case BallType::Retrigger:  return 4;
        default:                   return 1;
    }
}

static int pinTypeCost(PinType t) {
    switch (t) {
        case PinType::Gold:        return 2;
        case PinType::Mischievous: return 2;
        case PinType::Exploding:   return 4;
        case PinType::Light:       return 2;
        case PinType::Big:         return 3;
        case PinType::Ice:         return 2;
        case PinType::CopyCat:     return 3;
        case PinType::LuckyDucky:  return 4;
        case PinType::LevelUp:     return 1;
        case PinType::Lover:       return 3;
        case PinType::ChangeIsGood:return 3;
        case PinType::ThirdTime:   return 3;
        default:                   return 1;
    }
}

static int shoeTypeCost(ShoeType t) {
    switch (t) {
        case ShoeType::Clown:    return 0;
        case ShoeType::Running:  return 3;
        case ShoeType::Moon:     return 4;
        case ShoeType::Slippers: return 3;
        case ShoeType::HighHeels:return 3;
        case ShoeType::SteelCap: return 4;
        default:                 return 0;
    }
}

static int powerTypeCost(PowerType t) {
    switch (t) {
        case PowerType::Greedy:              return 5;
        case PowerType::RandomUpgrade:       return 3;
        case PowerType::ExtraPins:           return 5;
        case PowerType::ExtraBall:           return 7;
        case PowerType::Duplicate:           return 2;
        case PowerType::Bumpers:             return 5;
        case PowerType::SwapPins:            return 2;
        case PowerType::HomeBase:            return 5;
        case PowerType::Confusion:           return 3;
        case PowerType::Earthquake:          return 6;
        case PowerType::Skip:                return 1;
        case PowerType::UpgradesForEveryone: return 4;
        case PowerType::Sales:               return 4;
        case PowerType::PassedGo:            return 4;
        case PowerType::MoMoney:             return 4;
        case PowerType::SevenEightNine:      return 3;
        case PowerType::ExtraPowerSlot:      return 4;
        default:                             return 0;
    }
}

static bool powerIsStackable(PowerType t) {
    switch (t) {
        case PowerType::Duplicate:
        case PowerType::SwapPins:
        case PowerType::SevenEightNine:
            return true;
        default:
            return false;
    }
}

static bool powerCountsTowardLimit(PowerType t) {
    if (powerIsStackable(t)) return false;
    if (t == PowerType::ExtraPowerSlot) return false;
    return true;
}

static int ownedPermanentPowerCount(const ActiveItems& items) {
    int count = 0;
    for (int i = 0; i <= (int)PowerType::ExtraPowerSlot; i++) {
        PowerType p = static_cast<PowerType>(i);
        if (!powerCountsTowardLimit(p)) continue;
        if (items.hasPower(p) || items.hasPurchasedPower(p)) {
            count++;
        }
    }
    return count;
}

static bool canBuyPowerWithLimit(const ActiveItems& items, PowerType p, int maxPowers) {
    if (!powerCountsTowardLimit(p)) return true;
    if (items.hasPower(p) || items.hasPurchasedPower(p)) return true;
    return ownedPermanentPowerCount(items) < maxPowers;
}

static sf::Color powerPreviewColor(PowerType t) {
    switch (t) {
        case PowerType::Greedy:              return {240, 210, 70};
        case PowerType::RandomUpgrade:       return {120, 220, 255};
        case PowerType::ExtraPins:           return {255, 170, 70};
        case PowerType::ExtraBall:           return {120, 170, 255};
        case PowerType::Duplicate:           return {210, 150, 255};
        case PowerType::Bumpers:             return {255, 120, 120};
        case PowerType::SwapPins:            return {140, 255, 170};
        case PowerType::HomeBase:            return {255, 220, 180};
        case PowerType::Confusion:           return {200, 130, 255};
        case PowerType::Earthquake:          return {255, 140, 80};
        case PowerType::Skip:                return {255, 255, 255};
        case PowerType::UpgradesForEveryone: return {130, 255, 210};
        case PowerType::Sales:               return {120, 255, 120};
        case PowerType::PassedGo:            return {255, 240, 120};
        case PowerType::MoMoney:             return {255, 200, 70};
        case PowerType::SevenEightNine:      return {255, 170, 120};
        case PowerType::ExtraPowerSlot:      return {255, 175, 110};
        default:                             return {200, 200, 200};
    }
}

void UI::generateShopOffers(const ActiveItems& items) {
    shopOffers.clear();

    // Build a combined pool of balls, pins, shoes and powers.
    struct RawOffer {
        ShopItemCategory category;
        BallType ballType = BallType::Normal;
        PinType  pinType  = PinType::Normal;
        ShoeType shoeType = ShoeType::None;
        PowerType powerType = PowerType::Greedy;
        std::string name, description;
        int cost;
    };

    std::vector<RawOffer> pool = {
        // Balls
        {ShopItemCategory::Ball, BallType::BlackHole, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Black Hole",   "Pins drift toward ball.\n8% smaller.",               4},
        {ShopItemCategory::Ball, BallType::Midas, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Midas Ball",   "First pin hit each shot\nturns gold. Gold pin +1 token.", 5},
        {ShopItemCategory::Ball, BallType::Upgrade, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Upgrade Ball", "Hit pins +1 value.\n10% lighter, 5% faster.",        3},
        {ShopItemCategory::Ball, BallType::Heavy, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Heavy Ball",   "15% heavier.\nKnocks pins easier.",                  2},
        {ShopItemCategory::Ball, BallType::Fastball, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Fastball",     "5% lighter.\n15% faster.",                           2},
        {ShopItemCategory::Ball, BallType::OddBall, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Odd Ball",     "Odd pins x2.\nEven pins x0.75.",                     3},
        {ShopItemCategory::Ball, BallType::EightBall, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "8-Ball",       "All pins worth 8.",                                   4},
        {ShopItemCategory::Ball, BallType::Icy, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Icy Ball",     "Each Ice pin hit\nadds +15 score.",                    4},
        {ShopItemCategory::Ball, BallType::Retrigger, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Retrigger",    "2nd pin hit\nscores 3x.",                            4},
        // Pins
        {ShopItemCategory::Pin, BallType::Normal, PinType::Gold, ShoeType::None, PowerType::Greedy,
         "Gold Pin",     "One pin turns gold.\nKnock it for +1 token.",        2},
        {ShopItemCategory::Pin, BallType::Normal, PinType::Mischievous, ShoeType::None, PowerType::Greedy,
         "Mischievous",  "One pin randomises\nvalue 1-15 each shot.",          2},
        {ShopItemCategory::Pin, BallType::Normal, PinType::Exploding, ShoeType::None, PowerType::Greedy,
         "Exploding Pin","One pin explodes\n1s after being knocked.",           5},
        {ShopItemCategory::Pin, BallType::Normal, PinType::Light, ShoeType::None, PowerType::Greedy,
         "Light Pin",    "One pin worth 2\nbut very easy to knock.",           2},
        {ShopItemCategory::Pin, BallType::Normal, PinType::Big, ShoeType::None, PowerType::Greedy,
         "Big Pin",      "One pin worth 10\nbut hard to knock.",               3},
        {ShopItemCategory::Pin, BallType::Normal, PinType::Ice, ShoeType::None, PowerType::Greedy,
         "Ice Pin",      "One pin slides\n15% faster when fallen.",            2},
        {ShopItemCategory::Pin, BallType::Normal, PinType::CopyCat, ShoeType::None, PowerType::Greedy,
         "Copy Cat",     "Copies the type of\nthe first pin hit.",             3},
        {ShopItemCategory::Pin, BallType::Normal, PinType::LuckyDucky, ShoeType::None, PowerType::Greedy,
         "Lucky Ducky",  "Worth 20 pts.\n35% chance to score 0.",             4},
        {ShopItemCategory::Pin, BallType::Normal, PinType::LevelUp, ShoeType::None, PowerType::Greedy,
         "Level Up",     "This pin is worth\n+1 point.",                       1},
        {ShopItemCategory::Pin, BallType::Normal, PinType::Lover, ShoeType::None, PowerType::Greedy,
         "Lover",        "On hit, +1 value\nto the next slot pin.",            3},
        {ShopItemCategory::Pin, BallType::Normal, PinType::ChangeIsGood, ShoeType::None, PowerType::Greedy,
         "Change Is Good","When another pin changes,\nthis pin gains +2 value.", 3},
        {ShopItemCategory::Pin, BallType::Normal, PinType::ThirdTime, ShoeType::None, PowerType::Greedy,
         "3rd Time",     "Every 3rd-Time this pin is hit:\ncombo x2 this shot.",     3},
        // Shoes
        {ShopItemCategory::Shoe, BallType::Normal, PinType::Normal, ShoeType::Clown, PowerType::Greedy,
         "Clown Shoes",  "Wobble launch angle.\n8% slower.\nFirst buy: +10 dollars.", 0},
        {ShopItemCategory::Shoe, BallType::Normal, PinType::Normal, ShoeType::Running, PowerType::Greedy,
         "Running Shoes","Ball launches\n18% faster.",                         3},
        {ShopItemCategory::Shoe, BallType::Normal, PinType::Normal, ShoeType::Moon, PowerType::Greedy,
         "Moon Boots",   "All pins are\n26% lighter.",                         4},
        {ShopItemCategory::Shoe, BallType::Normal, PinType::Normal, ShoeType::Slippers, PowerType::Greedy,
         "Slippers",     "Everything slides\n18% more.",                       3},
        {ShopItemCategory::Shoe, BallType::Normal, PinType::Normal, ShoeType::HighHeels, PowerType::Greedy,
         "High Heels",   "Pins are 8%\ncloser together.",                      3},
        {ShopItemCategory::Shoe, BallType::Normal, PinType::Normal, ShoeType::SteelCap, PowerType::Greedy,
         "Steel Caps",   "Pins cannot change\nduring a round.",                4},
        // Powers
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::Greedy,
         "Greedy",       "Gain +1 combo for\nevery 4 dollars.",                5},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::RandomUpgrade,
         "Random Upgrade","After each frame,\nrandom pin +1 value.",             3},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::ExtraPins,
         "Extra Pins",   "Adds two extra pins\nto the rack.",                   5},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::ExtraBall,
         "Extra Ball",   "Adds one extra shot\nper frame.",                     7},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::Duplicate,
         "Duplicate",    "Press N, then choose\nsource and target. 1 use.",     2},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::Bumpers,
         "Bumpers",      "No gutter balls\nfor the rest of run.",               5},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::SwapPins,
         "Swap Pins",    "Press V, then choose\ntwo pins to swap. 1 use.",      2},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::SevenEightNine,
         "7 8 9",        "Triples pin 7 value,\ndestroys pin 9. 1 use.",       3},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::HomeBase,
         "Home Base",    "Every 20 pins hit,\nbase combo +1.",                 5},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::Confusion,
         "Confusion",    "Spares score like\nstrikes.",                         3},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::Earthquake,
         "Earthquake",   "Every 10 shots,\na free strike.",                     6},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::Skip,
         "Reroll",       "Permanent:\n2 rerolls each shop.",                    1},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::UpgradesForEveryone,
         "Upgrades+3",   "Future shops show\nthree extra items.",               4},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::Sales,
         "Sales",        "Everything in shop\nis 20% cheaper.",                 4},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::PassedGo,
         "Passed Go",    "Round clears pay\n3 more dollars.",                   4},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::MoMoney,
         "Mo Money",     "Interest every 2\ndollars, not 3.",                   4},
        {ShopItemCategory::Power, BallType::Normal, PinType::Normal, ShoeType::None, PowerType::ExtraPowerSlot,
         "Extra Slot",   "One-time:\nmax power slots 4 -> 5.",                  4},
    };

    auto hasOwnedPower = [&](PowerType p) {
        return items.hasPower(p) || items.hasPurchasedPower(p);
    };

    std::vector<RawOffer> filtered;
    filtered.reserve(pool.size());
    for (const auto& offer : pool) {
        RawOffer adjusted = offer;
        if (items.powerSales && adjusted.cost > 0) {
            int discounted = static_cast<int>(std::lround(static_cast<float>(adjusted.cost) * 0.8f));
            adjusted.cost = std::max(1, discounted);
        }
        if (adjusted.category == ShopItemCategory::Shoe &&
            adjusted.shoeType == items.shoeType) {
            continue;
        }
        if (adjusted.category == ShopItemCategory::Shoe &&
            adjusted.shoeType == ShoeType::Clown &&
            items.clownShoesPurchased) {
            continue;
        }
        if (adjusted.category == ShopItemCategory::Power &&
            !powerIsStackable(adjusted.powerType) &&
            hasOwnedPower(adjusted.powerType)) {
            continue;
        }
        filtered.push_back(adjusted);
    }

    // Shuffle pool for random slots
    for (int i = (int)filtered.size()-1; i > 0; i--) {
        int j = rand() % (i+1);
        std::swap(filtered[i], filtered[j]);
    }

    int maxOffers = hasOwnedPower(PowerType::UpgradesForEveryone) ? 7 : 4;
    int count = std::min(maxOffers, (int)filtered.size());

    for (int i = 0; i < count; i++) {
        ShopOffer o;
        o.category    = filtered[i].category;
        o.ballType    = filtered[i].ballType;
        o.pinType     = filtered[i].pinType;
        o.shoeType    = filtered[i].shoeType;
        o.powerType   = filtered[i].powerType;
        o.name        = filtered[i].name;
        o.description = filtered[i].description;
        o.cost        = filtered[i].cost;
        shopOffers.push_back(o);
        recordOfferShown(o);
    }
}

void UI::drawShop(sf::RenderWindow& window, int tokens, float windowW, float windowH, const ActiveItems& items) {
    if (!fontLoaded) return;
    if (state != GameState::Shop) return;

    if (shopOffers.empty()) generateShopOffers(items);
    ShopOwnedPanelLayout ownedLayout = computeShopOwnedPanelLayout(windowW, windowH, shopOffers.size(), items);
    ShopCardLayout cardLayout = computeShopCardLayout(windowW, ownedLayout, shopOffers.size());

    // Background (menu-style sky gradient).
    sf::VertexArray bg(sf::PrimitiveType::TriangleStrip, 4);
    bg[0].position = {0.f, 0.f};
    bg[1].position = {windowW, 0.f};
    bg[2].position = {0.f, windowH};
    bg[3].position = {windowW, windowH};
    bg[0].color = sf::Color(62, 104, 196);
    bg[1].color = sf::Color(70, 112, 204);
    bg[2].color = sf::Color(138, 180, 242);
    bg[3].color = sf::Color(126, 172, 239);
    window.draw(bg);

    // Title
    sf::Text title(font, "SHOP", 80);
    title.setPosition({windowW/2.f - 80.f, 30.f});
    title.setFillColor(sf::Color(245, 52, 88));
    title.setOutlineColor(sf::Color(20, 32, 66));
    title.setOutlineThickness(3);
    window.draw(title);

    // Tokens display
    sf::Text tokenText(font, "Tokens: " + std::to_string(tokens), 36);
    tokenText.setPosition({windowW - 240.f, 40.f});
    tokenText.setFillColor(sf::Color(245, 230, 70));
    tokenText.setOutlineColor(sf::Color(20, 32, 66));
    tokenText.setOutlineThickness(2);
    window.draw(tokenText);

    const float skipW = 220.f;
    const float skipH = 40.f;
    float skipX = cardLayout.areaMinX + (cardLayout.areaW - skipW) * 0.5f;
    float skipY = cardLayout.firstCardY - skipH - 12.f;
    if (skipY < 88.f) skipY = 88.f;
    if (items.skipCharges > 0) {
        bool canUseSkip = tokens >= 1;
        sf::RectangleShape skipBtn({skipW, skipH});
        skipBtn.setPosition({skipX, skipY});
        skipBtn.setFillColor(canUseSkip ? sf::Color(96, 216, 242) : sf::Color(177, 196, 232));
        skipBtn.setOutlineColor(sf::Color(64, 111, 194));
        skipBtn.setOutlineThickness(2.f);
        window.draw(skipBtn);

        sf::Text skipText(font,
                          "REROLL (1) x" + std::to_string(items.skipCharges),
                          20);
        skipText.setPosition({skipX + 14.f, skipY + 8.f});
        skipText.setFillColor(sf::Color(19, 39, 82));
        window.draw(skipText);
    }

    bool hasExtraPinsPower =
        items.powerExtraPins || items.hasPurchasedPower(PowerType::ExtraPins);
    int pinBuyLimit = hasExtraPinsPower ? 12 : 10;
    selectedPinSlot = std::clamp(selectedPinSlot, 1, pinBuyLimit);
    bool hasExtraBallSlot =
        items.powerExtraBall || items.hasPurchasedPower(PowerType::ExtraBall);
    int ballSlotCount = hasExtraBallSlot ? 3 : 2;
    selectedBallSlot = std::clamp(selectedBallSlot, 1, ballSlotCount);

    // Ball slot selector (which slot a purchased ball should fill)
    sf::Text slotTitle(font, "Ball Slot", 22);
    slotTitle.setPosition({40.f, 38.f});
    slotTitle.setFillColor({20, 40, 86});
    window.draw(slotTitle);

    const float slotY = 70.f;
    const float slotW = (ballSlotCount == 3) ? 88.f : 130.f;
    const float slotH = 44.f;
    const float slot1X = 40.f;
    const float slotGap = (ballSlotCount == 3) ? 8.f : 12.f;

    auto drawSlotButton = [&](int slot, float x) {
        bool selected = (selectedBallSlot == slot);
        sf::RectangleShape b({slotW, slotH});
        b.setPosition({x, slotY});
        b.setFillColor(selected ? sf::Color(94, 216, 241) : sf::Color(183, 206, 241));
        b.setOutlineColor(selected ? sf::Color(59, 114, 199) : sf::Color(102, 131, 194));
        b.setOutlineThickness(2.f);
        window.draw(b);

        int slotTextSize = (ballSlotCount == 3) ? 17 : 20;
        sf::Text t(font, "SHOT " + std::to_string(slot), slotTextSize);
        t.setPosition({x + (ballSlotCount == 3 ? 10.f : 14.f),
                       slotY + (ballSlotCount == 3 ? 12.f : 10.f)});
        t.setFillColor(sf::Color(19, 39, 82));
        window.draw(t);
    };

    for (int slot = 1; slot <= ballSlotCount; slot++) {
        float slotX = slot1X + (slot - 1) * (slotW + slotGap);
        drawSlotButton(slot, slotX);
    }

    sf::Text slotCurrent(font, "Current: " + ballShortName(items.getBallForSlot(selectedBallSlot)), 20);
    slotCurrent.setPosition({40.f, 122.f});
    slotCurrent.setFillColor({24, 46, 95});
    window.draw(slotCurrent);

    sf::Text pinSlotTitle(font, "Pin Slot", 20);
    pinSlotTitle.setPosition({40.f, 150.f});
    pinSlotTitle.setFillColor({20, 40, 86});
    window.draw(pinSlotTitle);

    const float pinSlotY = 176.f;
    const float pinBtnW = 34.f;
    const float pinBtnH = 34.f;
    const float pinPrevX = 40.f;
    const float pinBadgeX = pinPrevX + pinBtnW + 8.f;
    const float pinBadgeW = 94.f;
    const float pinNextX = pinBadgeX + pinBadgeW + 8.f;

    sf::RectangleShape pinPrev({pinBtnW, pinBtnH});
    pinPrev.setPosition({pinPrevX, pinSlotY});
    pinPrev.setFillColor(sf::Color(183, 206, 241));
    pinPrev.setOutlineColor(sf::Color(102, 131, 194));
    pinPrev.setOutlineThickness(2.f);
    window.draw(pinPrev);

    sf::Text prevText(font, "<", 20);
    prevText.setPosition({pinPrevX + 10.f, pinSlotY + 4.f});
    prevText.setFillColor(sf::Color(19, 39, 82));
    window.draw(prevText);

    bool assigned = items.hasPinAssignmentAtSlot(selectedPinSlot);
    sf::RectangleShape pinBadge({pinBadgeW, pinBtnH});
    pinBadge.setPosition({pinBadgeX, pinSlotY});
    pinBadge.setFillColor(assigned ? sf::Color(120, 230, 189) : sf::Color(183, 206, 241));
    pinBadge.setOutlineColor(sf::Color(89, 124, 194));
    pinBadge.setOutlineThickness(2.f);
    window.draw(pinBadge);

    sf::Text pinBadgeText(font,
                          "P" + std::to_string(selectedPinSlot) + "/" + std::to_string(pinBuyLimit),
                          16);
    pinBadgeText.setPosition({pinBadgeX + 11.f, pinSlotY + 7.f});
    pinBadgeText.setFillColor(sf::Color(19, 39, 82));
    window.draw(pinBadgeText);

    sf::RectangleShape pinNext({pinBtnW, pinBtnH});
    pinNext.setPosition({pinNextX, pinSlotY});
    pinNext.setFillColor(sf::Color(183, 206, 241));
    pinNext.setOutlineColor(sf::Color(102, 131, 194));
    pinNext.setOutlineThickness(2.f);
    window.draw(pinNext);

    sf::Text nextText(font, ">", 20);
    nextText.setPosition({pinNextX + 10.f, pinSlotY + 4.f});
    nextText.setFillColor(sf::Color(19, 39, 82));
    window.draw(nextText);

    PinType selectedPinType = items.getPinTypeForSlot(selectedPinSlot);
    sf::Text pinSlotCurrent(font,
        "Current P" + std::to_string(selectedPinSlot) + ": " +
        pinShortName(static_cast<int>(selectedPinType)), 18);
    float pinInfoY = pinSlotY + pinBtnH + 6.f;
    pinSlotCurrent.setPosition({40.f, pinInfoY});
    pinSlotCurrent.setFillColor({24, 46, 95});
    window.draw(pinSlotCurrent);

    const bool compactCards = shopOffers.size() > 4;
    const float buyBtnH = compactCards ? 38.f : 44.f;
    const float buyBtnBottomMargin = compactCards ? 10.f : 12.f;
    const unsigned catTextSize = compactCards ? 16 : 20;
    const unsigned nameTextSize = compactCards ? 18 : 26;
    const unsigned descTextSize = compactCards ? 16 : 20;
    const float descLineStep = compactCards ? 20.f : 26.f;
    const float previewR = compactCards ? 28.f : 36.f;
    const float previewY = compactCards ? 108.f : 135.f;
    const float descStartY = compactCards ? 145.f : 198.f;
    auto fitCardText = [&](sf::Text& text, float maxW, unsigned minSize) {
        while (text.getCharacterSize() > minSize &&
               text.getLocalBounds().size.x > maxW) {
            text.setCharacterSize(text.getCharacterSize() - 1);
        }
    };

    for (int i = 0; i < (int)shopOffers.size(); i++) {
        const auto& offer = shopOffers[i];
        int row = i / cardLayout.cardsPerRow;
        int col = i % cardLayout.cardsPerRow;
        int remaining = (int)shopOffers.size() - row * cardLayout.cardsPerRow;
        int cardsInRow = std::min(cardLayout.cardsPerRow, remaining);
        float rowTotalW = cardsInRow * cardLayout.cardW + (cardsInRow - 1) * cardLayout.cardGap;
        float rowStartX = cardLayout.areaMinX + (cardLayout.areaW - rowTotalW) * 0.5f;
        if (rowStartX < cardLayout.areaMinX) rowStartX = cardLayout.areaMinX;
        float cx = rowStartX + col * (cardLayout.cardW + cardLayout.cardGap);
        float cardY = cardLayout.firstCardY + row * (cardLayout.cardH + cardLayout.cardGap);
        bool canAfford = tokens >= offer.cost;
        bool isBall = (offer.category == ShopItemCategory::Ball);
        bool isPin  = (offer.category == ShopItemCategory::Pin);
        bool isShoe = (offer.category == ShopItemCategory::Shoe);
        bool isPower = (offer.category == ShopItemCategory::Power);
        bool isStackablePower = isPower && powerIsStackable(offer.powerType);
        bool isOwned = false;
        if (isBall) isOwned = (items.getBallForSlot(selectedBallSlot) == offer.ballType);
        if (isShoe) isOwned = (items.shoeType == offer.shoeType);
        if (isPower && !isStackablePower) {
            isOwned = items.hasPower(offer.powerType) || items.hasPurchasedPower(offer.powerType);
        }

        const int maxPermanentPowers = items.getMaxPermanentPowerSlots();
        bool powerLimitBlocked = isPower && !isOwned &&
                                 !canBuyPowerWithLimit(items, offer.powerType, maxPermanentPowers);
        bool duplicateChargeBlocked = isPower &&
                                      offer.powerType == PowerType::Duplicate &&
                                      items.duplicateCharges > 0;

        bool pinSlotLocked = false;
        if (isPin) {
            PinType selectedType = items.getPinTypeForSlot(selectedPinSlot);
            isOwned = (selectedType == offer.pinType);
        }
        bool clownLocked = isShoe &&
                           offer.shoeType == ShoeType::Clown &&
                           items.clownShoesPurchased &&
                           items.shoeType != ShoeType::Clown;
        bool pinLimitBlocked = false;
        if (isPin && !isOwned) {
            bool slotEmpty = !items.hasPinAssignmentAtSlot(selectedPinSlot);
            pinLimitBlocked = slotEmpty && (items.getPinAssignmentCount() >= pinBuyLimit);
            pinSlotLocked = !slotEmpty &&
                            (items.getPinTypeForSlot(selectedPinSlot) != offer.pinType);
        }
        bool blocked =
            !canAfford || powerLimitBlocked || duplicateChargeBlocked ||
            pinLimitBlocked || pinSlotLocked || clownLocked;

        // Card background tint by category
        sf::Color cardColor = isOwned         ? sf::Color(182, 238, 195)  :
                              blocked
                                              ? sf::Color(207, 210, 220)   :
                              isBall          ? sf::Color(205, 219, 255)   :
                              isPin           ? sf::Color(202, 239, 216)   :
                              isShoe          ? sf::Color(246, 216, 202)   :
                                                sf::Color(247, 230, 190);
        sf::Color borderColor = isOwned       ? sf::Color(73, 184, 111)  :
                                blocked
                                              ? sf::Color(151, 156, 174)   :
                                isBall        ? sf::Color(120, 145, 230):
                                isPin         ? sf::Color(98, 189, 125):
                                isShoe        ? sf::Color(214, 137, 99):
                                                sf::Color(210, 163, 72);

        sf::RectangleShape card({cardLayout.cardW, cardLayout.cardH});
        card.setPosition({cx, cardY});
        card.setFillColor(cardColor);
        card.setOutlineColor(borderColor);
        card.setOutlineThickness(3);
        window.draw(card);

        // Category badge
        sf::RectangleShape badge({cardLayout.cardW, 28.f});
        badge.setPosition({cx, cardY});
        badge.setFillColor(isBall ? sf::Color(138,165,235) :
                          isPin  ? sf::Color(126,199,152) :
                          isShoe ? sf::Color(226,157,128) :
                                   sf::Color(232,191,108));
        window.draw(badge);

        sf::Text catLabel(font, isBall ? "BALL" : (isPin ? "PIN" : (isShoe ? "SHOE" : "POWER")), catTextSize);
        catLabel.setPosition({cx + 10.f, cardY + 4.f});
        catLabel.setFillColor(sf::Color(35, 46, 78));
        window.draw(catLabel);

        // Item name
        sf::Text nameText(font, offer.name, nameTextSize);
        fitCardText(nameText, cardLayout.cardW - 16.f, 14);
        nameText.setPosition({cx + 10.f, cardY + 34.f});
        nameText.setFillColor(sf::Color(35, 46, 78));
        window.draw(nameText);

        // Preview circle
        sf::CircleShape preview(previewR);
        preview.setOrigin({previewR, previewR});
        preview.setPosition({cx + cardLayout.cardW/2.f, cardY + previewY});

        if (isBall) {
            sf::Color pc;
            switch (offer.ballType) {
                case BallType::BlackHole:  pc = {10,  0,   20};  break;
                case BallType::Midas:      pc = {210, 170, 20};  break;
                case BallType::Upgrade:    pc = {30,  80,  200}; break;
                case BallType::Heavy:      pc = {60,  60,  65};  break;
                case BallType::Fastball:   pc = {240, 240, 240}; break;
                case BallType::OddBall:    pc = {60,  180, 60};  break;
                case BallType::EightBall:  pc = {10,  10,  10};  break;
                case BallType::Icy:        pc = {165, 225, 255}; break;
                case BallType::Retrigger:  pc = {160, 170, 180}; break;
                default:                   pc = {25,  55,  140}; break;
            }
            preview.setFillColor(pc);
        } else if (isPin) {
            // Pin preview colour
            sf::Color pc;
            switch (offer.pinType) {
                case PinType::Gold:       pc = {210, 175, 50};  break;
                case PinType::Mischievous:pc = {140, 30,  180}; break;
                case PinType::Exploding:  pc = {220, 80,  20};  break;
                case PinType::Light:      pc = {140, 200, 240}; break;
                case PinType::Big:        pc = {160, 20,  20};  break;
                case PinType::Ice:        pc = {180, 230, 255}; break;
                case PinType::CopyCat:    pc = {150, 150, 155}; break;
                case PinType::LuckyDucky: pc = {230, 200, 20};  break;
                case PinType::LevelUp:    pc = {100, 185, 245}; break;
                case PinType::Lover:      pc = {220, 60, 110};  break;
                case PinType::ChangeIsGood:pc = {225, 175, 60}; break;
                case PinType::ThirdTime:  pc = {20,  160, 50};  break;
                default:                  pc = {220, 220, 220}; break;
            }
            preview.setFillColor(pc);
            // Small "PIN" indicator
            preview.setOutlineThickness(3.f);
            preview.setOutlineColor({255,255,255,100});
        } else {
            preview.setFillColor(isShoe ? shoePreviewColor(offer.shoeType) : powerPreviewColor(offer.powerType));
            preview.setOutlineThickness(3.f);
            preview.setOutlineColor({255,255,255,100});
        }
        window.draw(preview);

        // Description
        std::string desc = offer.description;
        float dy = cardY + descStartY;
        float buyBtnY = cardY + cardLayout.cardH - (buyBtnH + buyBtnBottomMargin);
        float descBottomY = buyBtnY - 8.f;
        std::string line;
        auto drawDescLine = [&](const std::string& s) {
            if (s.empty()) return;
            if (dy + descLineStep > descBottomY) return;
            sf::Text lt(font, s, descTextSize);
            fitCardText(lt, cardLayout.cardW - 24.f, 12);
            lt.setPosition({cx + 12.f, dy});
            lt.setFillColor(sf::Color(66, 76, 108));
            window.draw(lt);
            dy += descLineStep;
        };
        for (char ch : desc) {
            if (ch == '\n') {
                drawDescLine(line);
                line.clear();
            } else line += ch;
        }
        drawDescLine(line);

        // Buy button
        sf::Color btnColor = isOwned     ? sf::Color(145, 161, 200) :
                             (!blocked)
                                         ? sf::Color(103, 216, 166) :
                                           sf::Color(178, 183, 200);
        sf::RectangleShape btn({cardLayout.cardW - 20.f, buyBtnH});
        btn.setPosition({cx + 10.f, cardY + cardLayout.cardH - (buyBtnH + buyBtnBottomMargin)});
        btn.setFillColor(btnColor);
        btn.setOutlineColor(sf::Color(87, 98, 133));
        btn.setOutlineThickness(2);
        window.draw(btn);

        std::string btnLabel;
        if (isOwned) {
            if (isBall || isShoe) btnLabel = "EQUIPPED";
            else btnLabel = "OWNED";
        } else if (clownLocked) {
            btnLabel = "One-time only";
        } else if (duplicateChargeBlocked) {
            btnLabel = "Use Duplicate first";
        } else if (powerLimitBlocked) {
            btnLabel = "Power limit (" + std::to_string(maxPermanentPowers) + ")";
        } else if (pinSlotLocked) {
            btnLabel = "Sell current pin first";
        } else if (pinLimitBlocked) {
            btnLabel = "Pin limit reached";
        } else if (canAfford) {
            btnLabel = "BUY (" + std::to_string(offer.cost) + " tokens)";
        } else {
            btnLabel = "Need " + std::to_string(offer.cost) + " tokens";
        }
        sf::Text btnText(font, btnLabel, compactCards ? 17 : 20);
        fitCardText(btnText, cardLayout.cardW - 28.f, 14);
        btnText.setPosition({cx + 14.f, cardY + cardLayout.cardH - (buyBtnH + buyBtnBottomMargin) + (compactCards ? 7.f : 8.f)});
        btnText.setFillColor(sf::Color(32, 41, 71));
        window.draw(btnText);
    }

    // Continue button
    sf::RectangleShape cont({220.f, 60.f});
    cont.setPosition({windowW/2.f - 110.f, windowH - 100.f});
    cont.setFillColor(sf::Color(96, 216, 242));
    cont.setOutlineColor(sf::Color(68, 114, 198));
    cont.setOutlineThickness(3);
    window.draw(cont);

    sf::Text contText(font, "CONTINUE", 30);
    contText.setPosition({windowW/2.f - 90.f, windowH - 88.f});
    contText.setFillColor(sf::Color(19, 39, 82));
    window.draw(contText);

    // Show what the player already owns
    drawInventoryInShop(window, items, windowW, windowH);
}

int UI::handleShopClick(sf::RenderWindow& window, sf::Vector2i mousePos, int tokens, const ActiveItems& items) {
    sf::View v = window.getView();
    float windowW = v.getSize().x;
    float windowH = v.getSize().y;

    // Slot selector buttons
    bool hasExtraBallSlot =
        items.powerExtraBall || items.hasPurchasedPower(PowerType::ExtraBall);
    int ballSlotCount = hasExtraBallSlot ? 3 : 2;
    selectedBallSlot = std::clamp(selectedBallSlot, 1, ballSlotCount);

    const float slotY = 70.f;
    const float slotW = (ballSlotCount == 3) ? 88.f : 130.f;
    const float slotH = 44.f;
    const float slot1X = 40.f;
    const float slotGap = (ballSlotCount == 3) ? 8.f : 12.f;
    const float buttonPad = 10.0f;

    for (int slot = 1; slot <= ballSlotCount; slot++) {
        float slotX = slot1X + (slot - 1) * (slotW + slotGap);
        sf::FloatRect slotRect(sf::Vector2f(slotX, slotY), sf::Vector2f(slotW, slotH));
        if (pointInRect(slotRect, mousePos, buttonPad)) {
            selectedBallSlot = slot;
            return ShopActionNone;
        }
    }

    bool hasExtraPinsPower =
        items.powerExtraPins || items.hasPurchasedPower(PowerType::ExtraPins);
    int pinBuyLimit = hasExtraPinsPower ? 12 : 10;
    selectedPinSlot = std::clamp(selectedPinSlot, 1, pinBuyLimit);
    const float pinSlotY = 176.f;
    const float pinBtnW = 34.f;
    const float pinBtnH = 34.f;
    const float pinPrevX = 40.f;
    const float pinBadgeX = pinPrevX + pinBtnW + 8.f;
    const float pinBadgeW = 94.f;
    const float pinNextX = pinBadgeX + pinBadgeW + 8.f;
    sf::FloatRect pinPrevRect(sf::Vector2f(pinPrevX, pinSlotY), sf::Vector2f(pinBtnW, pinBtnH));
    sf::FloatRect pinNextRect(sf::Vector2f(pinNextX, pinSlotY), sf::Vector2f(pinBtnW, pinBtnH));
    sf::FloatRect pinBadgeRect(sf::Vector2f(pinBadgeX, pinSlotY), sf::Vector2f(pinBadgeW, pinBtnH));
    if (pointInRect(pinPrevRect, mousePos, buttonPad)) {
        selectedPinSlot = (selectedPinSlot <= 1) ? pinBuyLimit : (selectedPinSlot - 1);
        return ShopActionNone;
    }
    if (pointInRect(pinNextRect, mousePos, buttonPad)) {
        selectedPinSlot = (selectedPinSlot >= pinBuyLimit) ? 1 : (selectedPinSlot + 1);
        return ShopActionNone;
    }
    if (pointInRect(pinBadgeRect, mousePos, buttonPad)) {
        return ShopActionNone;
    }

    ShopOwnedPanelLayout layout = computeShopOwnedPanelLayout(windowW, windowH, shopOffers.size(), items);
    ShopCardLayout cardLayout = computeShopCardLayout(windowW, layout, shopOffers.size());

    const float skipW = 220.f;
    const float skipH = 40.f;
    float skipX = cardLayout.areaMinX + (cardLayout.areaW - skipW) * 0.5f;
    float skipY = cardLayout.firstCardY - skipH - 12.f;
    if (skipY < 88.f) skipY = 88.f;
    sf::FloatRect rerollRect(sf::Vector2f(skipX, skipY), sf::Vector2f(skipW, skipH));
    if (items.skipCharges > 0 && pointInRect(rerollRect, mousePos, buttonPad)) {
        if (tokens >= 1) return ShopActionReroll;
        return ShopActionNone;
    }

    ShopOwnedDynamicLayout dynamic = computeShopOwnedDynamicLayout(layout, items);
    if (pointInRect(layout.sellBall1, mousePos, 6.0f) && items.getBallForSlot(1) != BallType::Normal) {
        return ShopActionSellBallSlot1;
    }
    if (pointInRect(layout.sellBall2, mousePos, 6.0f) && items.getBallForSlot(2) != BallType::Normal) {
        return ShopActionSellBallSlot2;
    }
    if (layout.hasExtraBallSlot &&
        pointInRect(layout.sellBall3, mousePos, 6.0f) &&
        items.getBallForSlot(3) != BallType::Normal) {
        return ShopActionSellBallSlot3;
    }
    if (pointInRect(layout.sellShoe, mousePos, 6.0f) && items.shoeType != ShoeType::None) {
        return ShopActionSellShoe;
    }
    for (int i = 0; i < (int)dynamic.pinSellRects.size(); i++) {
        if (pointInRect(dynamic.pinSellRects[i], mousePos, 6.0f)) {
            return ShopActionSellPinByIndexBase - i;
        }
    }
    for (int i = 0; i < (int)dynamic.powerSellRects.size(); i++) {
        if (pointInRect(dynamic.powerSellRects[i], mousePos, 6.0f)) {
            return ShopActionSellPowerByIndexBase - i;
        }
    }

    const int maxPermanentPowers = items.getMaxPermanentPowerSlots();
    const bool compactCards = shopOffers.size() > 4;
    const float buyBtnH = compactCards ? 38.f : 44.f;
    const float buyBtnBottomMargin = compactCards ? 10.f : 12.f;

    for (int i = 0; i < (int)shopOffers.size(); i++) {
        int row = i / cardLayout.cardsPerRow;
        int col = i % cardLayout.cardsPerRow;
        int remaining = (int)shopOffers.size() - row * cardLayout.cardsPerRow;
        int cardsInRow = std::min(cardLayout.cardsPerRow, remaining);
        float rowTotalW = cardsInRow * cardLayout.cardW + (cardsInRow - 1) * cardLayout.cardGap;
        float rowStartX = cardLayout.areaMinX + (cardLayout.areaW - rowTotalW) * 0.5f;
        if (rowStartX < cardLayout.areaMinX) rowStartX = cardLayout.areaMinX;
        float cardY = cardLayout.firstCardY + row * (cardLayout.cardH + cardLayout.cardGap);
        float cx  = rowStartX + col * (cardLayout.cardW + cardLayout.cardGap);
        float btnX = cx + 10.f;
        float btnY = cardY + cardLayout.cardH - (buyBtnH + buyBtnBottomMargin);
        float btnW = cardLayout.cardW - 20.f;
        float btnH = buyBtnH;

        sf::FloatRect buyRect(sf::Vector2f(btnX, btnY), sf::Vector2f(btnW, btnH));
        if (pointInRect(buyRect, mousePos, 10.0f)) {
            const auto& offer = shopOffers[i];
            bool isAlreadyOwned = false;
            if (offer.category == ShopItemCategory::Ball) {
                isAlreadyOwned = (items.getBallForSlot(selectedBallSlot) == offer.ballType);
            } else if (offer.category == ShopItemCategory::Shoe) {
                isAlreadyOwned = (items.shoeType == offer.shoeType);
            } else if (offer.category == ShopItemCategory::Power) {
                if (!powerIsStackable(offer.powerType)) {
                    isAlreadyOwned = items.hasPower(offer.powerType) || items.hasPurchasedPower(offer.powerType);
                }
            } else if (offer.category == ShopItemCategory::Pin) {
                isAlreadyOwned = (items.getPinTypeForSlot(selectedPinSlot) == offer.pinType);
            }

            bool canBuy = (tokens >= offer.cost) && !isAlreadyOwned;
            if (offer.category == ShopItemCategory::Shoe &&
                offer.shoeType == ShoeType::Clown &&
                items.clownShoesPurchased) {
                canBuy = false;
            }
            if (offer.category == ShopItemCategory::Power) {
                canBuy = canBuy && canBuyPowerWithLimit(items, offer.powerType, maxPermanentPowers);
                if (offer.powerType == PowerType::Duplicate && items.duplicateCharges > 0) {
                    canBuy = false;
                }
            }
            if (offer.category == ShopItemCategory::Pin) {
                bool slotEmpty = !items.hasPinAssignmentAtSlot(selectedPinSlot);
                if (slotEmpty) {
                    canBuy = canBuy && (items.getPinAssignmentCount() < pinBuyLimit);
                } else if (items.getPinTypeForSlot(selectedPinSlot) != offer.pinType) {
                    canBuy = false;
                }
            }

            if (canBuy) {
                return i;
            }
        }
    }
    return ShopActionNone;
}
