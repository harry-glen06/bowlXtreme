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
    
    // Create 8 clouds at random positions
    for (int i = 0; i < 8; i++) {
        Cloud cloud;
        cloud.position = sf::Vector2f(
            rand() % (int)windowW,
            50.0f + rand() % (int)(windowH * 0.6f)
        );
        cloud.speed = 20.0f + rand() % 40;  // 20-60 pixels per second
        cloud.size = 40.0f + rand() % 60;   // 40-100 size
        clouds.push_back(cloud);
    }
}

void UI::updateClouds(float dt, float windowW) {
    for (auto& cloud : clouds) {
        cloud.position.x += cloud.speed * dt;
        
        // Wrap around when off screen
        if (cloud.position.x > windowW + cloud.size) {
            cloud.position.x = -cloud.size;
            cloud.position.y = 50.0f + rand() % 400;
        }
    }
}

void UI::drawClouds(sf::RenderWindow& window) {
    for (const auto& cloud : clouds) {
        // Draw fluffy cloud shape with multiple circles
        sf::CircleShape cloudCircle1(cloud.size * 0.6f);
        cloudCircle1.setFillColor(sf::Color(255, 255, 255, 180));
        cloudCircle1.setPosition(cloud.position);
        window.draw(cloudCircle1);
        
        sf::CircleShape cloudCircle2(cloud.size * 0.5f);
        cloudCircle2.setFillColor(sf::Color(255, 255, 255, 180));
        cloudCircle2.setPosition(sf::Vector2f(cloud.position.x + cloud.size * 0.5f, cloud.position.y - cloud.size * 0.2f));
        window.draw(cloudCircle2);
        
        sf::CircleShape cloudCircle3(cloud.size * 0.7f);
        cloudCircle3.setFillColor(sf::Color(255, 255, 255, 180));
        cloudCircle3.setPosition(sf::Vector2f(cloud.position.x + cloud.size * 0.3f, cloud.position.y + cloud.size * 0.1f));
        window.draw(cloudCircle3);
        
        sf::CircleShape cloudCircle4(cloud.size * 0.4f);
        cloudCircle4.setFillColor(sf::Color(255, 255, 255, 180));
        cloudCircle4.setPosition(sf::Vector2f(cloud.position.x + cloud.size * 0.9f, cloud.position.y + cloud.size * 0.15f));
        window.draw(cloudCircle4);
    }
}

void UI::drawMenu(sf::RenderWindow& window, float windowW, float windowH, float dt) {
    if (!fontLoaded) return;
    
    // Initialize clouds if needed
    if (clouds.empty()) {
        initClouds(windowW, windowH);
    }
    
    // Update and draw animated clouds
    updateClouds(dt, windowW);
    
    // Sky blue gradient background
    sf::RectangleShape skyTop(sf::Vector2f(windowW, windowH / 2));
    skyTop.setPosition(sf::Vector2f(0, 0));
    skyTop.setFillColor(sf::Color(100, 149, 237));  // Cornflower blue
    window.draw(skyTop);
    
    sf::RectangleShape skyBottom(sf::Vector2f(windowW, windowH / 2));
    skyBottom.setPosition(sf::Vector2f(0, windowH / 2));
    skyBottom.setFillColor(sf::Color(135, 206, 250));  // Light sky blue
    window.draw(skyBottom);
    
    drawClouds(window);
    
    // Draw "BOWL" logo with colorful blocks
    float logoX = windowW / 2 - 200;
    float logoY = windowH / 3 - 100;
    float blockWidth = 100.0f;
    float blockHeight = 150.0f;
    
    // B - Red
    sf::RectangleShape blockB(sf::Vector2f(blockWidth, blockHeight));
    blockB.setPosition(sf::Vector2f(logoX, logoY));
    blockB.setFillColor(sf::Color(220, 50, 50));
    blockB.setOutlineColor(sf::Color::Black);
    blockB.setOutlineThickness(3.0f);
    window.draw(blockB);
    
    sf::Text letterB(font, "B", 90);
    letterB.setPosition(sf::Vector2f(logoX + 20, logoY + 15));
    letterB.setFillColor(sf::Color::White);
    letterB.setOutlineColor(sf::Color::Black);
    letterB.setOutlineThickness(3.0f);
    window.draw(letterB);
    
    // O - Cyan
    sf::RectangleShape blockO(sf::Vector2f(blockWidth, blockHeight));
    blockO.setPosition(sf::Vector2f(logoX + blockWidth, logoY - 10));
    blockO.setFillColor(sf::Color(80, 200, 220));
    blockO.setOutlineColor(sf::Color::Black);
    blockO.setOutlineThickness(3.0f);
    window.draw(blockO);
    
    sf::Text letterO(font, "O", 90);
    letterO.setPosition(sf::Vector2f(logoX + blockWidth + 20, logoY + 5));
    letterO.setFillColor(sf::Color::White);
    letterO.setOutlineColor(sf::Color::Black);
    letterO.setOutlineThickness(3.0f);
    window.draw(letterO);
    
    // W - Yellow
    sf::RectangleShape blockW(sf::Vector2f(blockWidth, blockHeight));
    blockW.setPosition(sf::Vector2f(logoX + blockWidth * 2, logoY + 8));
    blockW.setFillColor(sf::Color(240, 220, 50));
    blockW.setOutlineColor(sf::Color::Black);
    blockW.setOutlineThickness(3.0f);
    window.draw(blockW);
    
    sf::Text letterW(font, "W", 90);
    letterW.setPosition(sf::Vector2f(logoX + blockWidth * 2 + 10, logoY + 23));
    letterW.setFillColor(sf::Color::Black);
    letterW.setOutlineColor(sf::Color::White);
    letterW.setOutlineThickness(2.0f);
    window.draw(letterW);
    
    // L - Light Blue
    sf::RectangleShape blockL(sf::Vector2f(blockWidth, blockHeight));
    blockL.setPosition(sf::Vector2f(logoX + blockWidth * 3, logoY + 3));
    blockL.setFillColor(sf::Color(120, 190, 230));
    blockL.setOutlineColor(sf::Color::Black);
    blockL.setOutlineThickness(3.0f);
    window.draw(blockL);
    
    sf::Text letterL(font, "L", 90);
    letterL.setPosition(sf::Vector2f(logoX + blockWidth * 3 + 30, logoY + 18));
    letterL.setFillColor(sf::Color::White);
    letterL.setOutlineColor(sf::Color::Black);
    letterL.setOutlineThickness(3.0f);
    window.draw(letterL);
    
    // "Xtreme" subtitle with retro sign look
    sf::RectangleShape xtremeBox(sf::Vector2f(450, 80));
    xtremeBox.setPosition(sf::Vector2f(windowW / 2 - 225, logoY + blockHeight + 20));
    xtremeBox.setFillColor(sf::Color(250, 240, 230));
    xtremeBox.setOutlineColor(sf::Color(200, 50, 50));
    xtremeBox.setOutlineThickness(5.0f);
    window.draw(xtremeBox);
    
    sf::Text xtremeText(font, "Xtreme", 60);
    xtremeText.setPosition(sf::Vector2f(windowW / 2 - 140, logoY + blockHeight + 20));
    xtremeText.setFillColor(sf::Color(220, 50, 50));
    xtremeText.setStyle(sf::Text::Bold);
    window.draw(xtremeText);
    
    // Menu buttons in rounded gray container
    float buttonY = windowH - 200;
    float buttonSpacing = 20.0f;
    float buttonWidth = 160.0f;
    float buttonHeight = 60.0f;
    
    // Button container background
    sf::ConvexShape buttonContainer = createRoundedRect(
        sf::Vector2f(buttonWidth * 3 + buttonSpacing * 4, buttonHeight + 40), 
        20.0f, 
        sf::Color(100, 100, 100, 200)
    );    
    buttonContainer.setPosition(sf::Vector2f(windowW / 2 - (buttonWidth * 3 + buttonSpacing * 4) / 2, buttonY - 20));
    buttonContainer.setFillColor(sf::Color(100, 100, 100, 200));
    window.draw(buttonContainer);
    
    // Normal button (Red)
    sf::ConvexShape normalButton = createRoundedRect(
    sf::Vector2f(buttonWidth, buttonHeight), 
    15.0f, 
    sf::Color(220, 50, 50)
    );
    normalButton.setPosition(sf::Vector2f(windowW / 2 - buttonWidth * 1.5f - buttonSpacing, buttonY));
    normalButton.setFillColor(sf::Color(220, 50, 50));
    window.draw(normalButton);
    
    sf::Text normalText(font, "Normal", 32);
    normalText.setPosition(sf::Vector2f(windowW / 2 - buttonWidth * 1.5f - buttonSpacing + 20, buttonY + 12));
    normalText.setFillColor(sf::Color::White);
    window.draw(normalText);
    
    // Xtreme button (Cyan)
    sf::ConvexShape xtremeButton = createRoundedRect(
    sf::Vector2f(buttonWidth, buttonHeight), 
    15.0f, 
    sf::Color(80, 200, 220)
    );
    xtremeButton.setPosition(sf::Vector2f(windowW / 2 - buttonWidth / 2, buttonY));
    xtremeButton.setFillColor(sf::Color(80, 200, 220));
    window.draw(xtremeButton);
    
    sf::Text xtremeButtonText(font, "Xtreme", 32);
    xtremeButtonText.setPosition(sf::Vector2f(windowW / 2 - buttonWidth / 2 + 15, buttonY + 12));
    xtremeButtonText.setFillColor(sf::Color::White);
    window.draw(xtremeButtonText);
    
    // Settings button (Yellow)
    sf::ConvexShape settingsButton = createRoundedRect(
    sf::Vector2f(buttonWidth, buttonHeight), 
    15.0f, 
    sf::Color(240, 220, 50)
    );
    settingsButton.setPosition(sf::Vector2f(windowW / 2 + buttonWidth / 2 + buttonSpacing, buttonY));
    settingsButton.setFillColor(sf::Color(240, 220, 50));
    window.draw(settingsButton);
    
    sf::Text settingsText(font, "Settings", 28);
    settingsText.setPosition(sf::Vector2f(windowW / 2 + buttonWidth / 2 + buttonSpacing + 15, buttonY + 15));
    settingsText.setFillColor(sf::Color::Black);
    window.draw(settingsText);
}

MenuButton UI::handleMenuClick(sf::RenderWindow& window, sf::Vector2i mousePos) {
    sf::View v = window.getView();
    float windowW = v.getSize().x;
    float windowH = v.getSize().y;
    
    float buttonY = windowH - 200;
    float buttonSpacing = 20.0f;
    float buttonWidth = 160.0f;
    float buttonHeight = 60.0f;
    const float clickPad = 10.0f;

    sf::FloatRect normalRect(
        sf::Vector2f(windowW / 2 - buttonWidth * 1.5f - buttonSpacing, buttonY),
        sf::Vector2f(buttonWidth, buttonHeight));
    sf::FloatRect xtremeRect(
        sf::Vector2f(windowW / 2 - buttonWidth / 2, buttonY),
        sf::Vector2f(buttonWidth, buttonHeight));
    sf::FloatRect settingsRect(
        sf::Vector2f(windowW / 2 + buttonWidth / 2 + buttonSpacing, buttonY),
        sf::Vector2f(buttonWidth, buttonHeight));
    
    // Check Normal button
    if (pointInRectPadded(normalRect, mousePos, clickPad)) {
        return MenuButton::Normal;
    }
    
    // Check Xtreme button
    if (pointInRectPadded(xtremeRect, mousePos, clickPad)) {
        return MenuButton::Xtreme;
    }
    
    // Check Settings button
    if (pointInRectPadded(settingsRect, mousePos, clickPad)) {
        return MenuButton::Settings;
    }
    
    return MenuButton::None;
}

void UI::drawSettings(sf::RenderWindow& window, float windowW, float windowH) {
    if (!fontLoaded) return;
    
    // Semi-transparent overlay
    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);
    
    // Settings panel
    float panelW = 600;
    float panelH = 500;
    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(sf::Vector2f(windowW / 2 - panelW / 2, windowH / 2 - panelH / 2));
    panel.setFillColor(sf::Color(40, 40, 40));
    panel.setOutlineColor(sf::Color::White);
    panel.setOutlineThickness(3.0f);
    window.draw(panel);
    
    float startX = windowW / 2 - panelW / 2 + 40;
    float startY = windowH / 2 - panelH / 2 + 40;
    
    // Title
    sf::Text title(font, "Settings", 50);
    title.setPosition(sf::Vector2f(startX, startY));
    title.setFillColor(sf::Color::White);
    window.draw(title);
    
    // Music Volume
    sf::Text musicLabel(font, "Music Volume", 30);
    musicLabel.setPosition(sf::Vector2f(startX, startY + 80));
    musicLabel.setFillColor(sf::Color::White);
    window.draw(musicLabel);
    
    // Music slider
    sf::RectangleShape musicSliderBg(sf::Vector2f(400, 10));
    musicSliderBg.setPosition(sf::Vector2f(startX, startY + 120));
    musicSliderBg.setFillColor(sf::Color(100, 100, 100));
    window.draw(musicSliderBg);
    
    sf::RectangleShape musicSliderFill(sf::Vector2f(400 * (musicVolume / 100.0f), 10));
    musicSliderFill.setPosition(sf::Vector2f(startX, startY + 120));
    musicSliderFill.setFillColor(sf::Color::Green);
    window.draw(musicSliderFill);
    
    sf::Text musicValue(font, std::to_string((int)musicVolume), 25);
    musicValue.setPosition(sf::Vector2f(startX + 420, startY + 110));
    musicValue.setFillColor(sf::Color::White);
    window.draw(musicValue);
    
    // Sound Effects Volume
    sf::Text soundLabel(font, "Sound Volume", 30);
    soundLabel.setPosition(sf::Vector2f(startX, startY + 160));
    soundLabel.setFillColor(sf::Color::White);
    window.draw(soundLabel);
    
    // Sound slider
    sf::RectangleShape soundSliderBg(sf::Vector2f(400, 10));
    soundSliderBg.setPosition(sf::Vector2f(startX, startY + 200));
    soundSliderBg.setFillColor(sf::Color(100, 100, 100));
    window.draw(soundSliderBg);
    
    sf::RectangleShape soundSliderFill(sf::Vector2f(400 * (soundVolume / 100.0f), 10));
    soundSliderFill.setPosition(sf::Vector2f(startX, startY + 200));
    soundSliderFill.setFillColor(sf::Color::Green);
    window.draw(soundSliderFill);
    
    sf::Text soundValue(font, std::to_string((int)soundVolume), 25);
    soundValue.setPosition(sf::Vector2f(startX + 420, startY + 190));
    soundValue.setFillColor(sf::Color::White);
    window.draw(soundValue);
    
    // Bumpers
    sf::Text bumpersLabel(font, "Bumpers", 30);
    bumpersLabel.setPosition(sf::Vector2f(startX, startY + 250));
    bumpersLabel.setFillColor(sf::Color::White);
    window.draw(bumpersLabel);
    
    // Bumpers checkbox
    sf::RectangleShape checkbox(sf::Vector2f(30, 30));
    checkbox.setPosition(sf::Vector2f(startX + 200, startY + 250));
    checkbox.setFillColor(bumpersDefault ? sf::Color::Green : sf::Color(100, 100, 100));
    checkbox.setOutlineColor(sf::Color::White);
    checkbox.setOutlineThickness(2.0f);
    window.draw(checkbox);
    
    sf::Text checkboxLabel(font, bumpersDefault ? "ON" : "OFF", 25);
    checkboxLabel.setPosition(sf::Vector2f(startX + 250, startY + 252));
    checkboxLabel.setFillColor(sf::Color::White);
    window.draw(checkboxLabel);
    
    // Back button
    sf::RectangleShape backButton(sf::Vector2f(200, 50));
    backButton.setPosition(sf::Vector2f(windowW / 2 - 100, startY + 350));
    backButton.setFillColor(sf::Color(220, 50, 50));
    window.draw(backButton);
    
    sf::Text backText(font, "Back", 30);
    backText.setPosition(sf::Vector2f(windowW / 2 - 40, startY + 358));
    backText.setFillColor(sf::Color::White);
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
    
    float startX = 10.0f;
    float startY = 100.0f;
    float frameWidth = 70.0f;
    float frameHeight = 60.0f;
    
    for (int i = 0; i < 10; i++) {
        float x = startX;
        float y = startY + i * frameHeight;
        
        // Frame box
        sf::RectangleShape box(sf::Vector2f(frameWidth - 2, frameHeight - 2));
        box.setPosition(sf::Vector2f(x, y));
        box.setFillColor(sf::Color(40, 40, 40));
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(1.0f);
        window.draw(box);
        
        // Frame number
        sf::Text frameNum(font, std::to_string(i + 1), 14);
        frameNum.setPosition(sf::Vector2f(x + 5, y + 2));
        frameNum.setFillColor(sf::Color(150, 150, 150));
        window.draw(frameNum);
        
        // Ball scores
        std::string ball1Str = "";
        std::string ball2Str = "";

        if (currentFrame > i || (currentFrame == i && currentBall > 1)) {
            if (frames[i].ball1 == 0) {
                ball1Str = "-";
            } else {
                ball1Str = std::to_string(frames[i].ball1);
            }
        }

        if (currentFrame > i || (currentFrame == i && currentBall > 2)) {
            if (frames[i].ball2 == 0) {
                ball2Str = "-";
            } else {
                ball2Str = std::to_string(frames[i].ball2);
            }
        }

        if (frames[i].isStrike && i < 9) {
            ball1Str = "X";
            ball2Str = "";
        } else if (frames[i].isSpare) {
            ball2Str = "/";
        } else if (frames[i].ball2 == 10) {
            ball2Str = "X";
        }
        
        sf::Text ball1Text(font, ball1Str, 16);
        ball1Text.setPosition(sf::Vector2f(x + frameWidth - 45, y + 18));
        window.draw(ball1Text);
        
        sf::Text ball2Text(font, ball2Str, 16);
        ball2Text.setPosition(sf::Vector2f(x + frameWidth - 25, y + 18));
        window.draw(ball2Text);
        
        // 10th frame has 3 balls
        if (i == 9 && frames[i].ball3 > 0) {
            std::string ball3Str = frames[i].ball3 == 10 ? "X" : std::to_string(frames[i].ball3);
            sf::Text ball3Text(font, ball3Str, 16);
            ball3Text.setPosition(sf::Vector2f(x + frameWidth - 15, y + 5));
            window.draw(ball3Text);
        }
        
        // Frame total
        if (frames[i].isComplete || i < currentFrame) {
            sf::Text scoreText(font, std::to_string(frames[i].score), 20);
            scoreText.setPosition(sf::Vector2f(x + frameWidth / 2 - 15, y + 35));
            scoreText.setFillColor(sf::Color::Yellow);
            window.draw(scoreText);
        }
        
    }
    
    // Current frame indicator
    float indicatorY = startY + currentFrame * frameHeight;
    sf::RectangleShape indicator(sf::Vector2f(3, frameHeight - 2));
    indicator.setPosition(sf::Vector2f(startX + frameWidth, indicatorY));
    indicator.setFillColor(sf::Color::Green);
    window.draw(indicator);
        
    // Show high score at bottom
    sf::Text highScoreDisplay(font, "High Score: " + std::to_string(normalHighScore), 16);
    highScoreDisplay.setPosition(sf::Vector2f(startX + 5, startY + (10 * frameHeight) + 10));
    highScoreDisplay.setFillColor(sf::Color::Cyan);
    window.draw(highScoreDisplay);

    // Define the Exit Button area
    sf::RectangleShape exitBtn(sf::Vector2f(120, 40));
    exitBtn.setPosition(sf::Vector2f(windowW - 140, 20)); // Top right corner
    exitBtn.setFillColor(sf::Color(100, 100, 100, 200));
    exitBtn.setOutlineThickness(2);
    exitBtn.setOutlineColor(sf::Color::White);
    // Draw the Button
    window.draw(exitBtn);
    sf::Text exitText(font, "MENU", 20);
    exitText.setPosition(sf::Vector2f(windowW - 110, 27));
    exitText.setFillColor(sf::Color::White);
    window.draw(exitText);
    // Handle Click Detection
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);
        sf::FloatRect hit = exitBtn.getGlobalBounds();
        if (pointInRectPadded(hit, worldPos, 8.0f)) {
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
                             int liveComboPreview) {
    if (!fontLoaded) return GameAction::None;
    if (state != GameState::Xtreme) return GameAction::None;

    float dt = 0.0f;
    if (!hudAnimClockPrimed) {
        hudAnimClock.restart();
        hudAnimClockPrimed = true;
    } else {
        dt = hudAnimClock.restart().asSeconds();
    }

    if (totalShots <= 0) {
        hudLastShotCounter = 0;
        hudCountTarget = 0;
        hudCountValue = 0.0f;
        hudCountTimer = 0.0f;
        hudFormulaTimer = 0.0f;
        hudImpactTarget = 10;
        hudComboTarget = 1;
        hudImpactValue = 10.0f;
        hudComboValue = 1.0f;
        hudCounting = false;
        hudCountingFormula = false;
        hudShowBigScore = false;
        hudBigTimer = 0.0f;
    } else if (totalShots != hudLastShotCounter) {
        hudLastShotCounter = totalShots;
        hudCountTarget = std::max(0, lastShotScore);
        hudImpactTarget = std::max(10, impact);
        hudComboTarget = std::max(1, combo);
        hudCountValue = 0.0f;
        hudImpactValue = 10.0f;
        hudComboValue = 1.0f;
        hudFormulaTimer = 0.0f;
        int formulaDelta = (hudImpactTarget - 10) + (hudComboTarget - 1) * 3;
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
        hudImpactValue = 10.0f + (float)(hudImpactTarget - 10) * eased;
        hudComboValue = 1.0f + (float)(hudComboTarget - 1) * eased;

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
            hudImpactTarget = 10;
            hudComboTarget = 1;
            hudImpactValue = 10.0f;
            hudComboValue = 1.0f;
        }
    }

    // Left info panel (larger + cleaner styling).
    const float leftPanelX = 28.0f;
    const float leftPanelY = 30.0f;
    const float leftPanelW = 300.0f;
    const float leftPanelH = windowH - 64.0f;

    sf::RectangleShape leftPanelShadow(sf::Vector2f(leftPanelW + 8.0f, leftPanelH + 8.0f));
    leftPanelShadow.setPosition(sf::Vector2f(leftPanelX + 5.0f, leftPanelY + 7.0f));
    leftPanelShadow.setFillColor(sf::Color(0, 0, 0, 90));
    window.draw(leftPanelShadow);

    sf::RectangleShape leftPanel(sf::Vector2f(leftPanelW, leftPanelH));
    leftPanel.setPosition(sf::Vector2f(leftPanelX, leftPanelY));
    leftPanel.setFillColor(sf::Color(67, 69, 79, 230));
    leftPanel.setOutlineColor(sf::Color(132, 140, 164));
    leftPanel.setOutlineThickness(3.0f);
    window.draw(leftPanel);

    sf::RectangleShape leftPanelTopBar(sf::Vector2f(leftPanelW - 8.0f, 5.0f));
    leftPanelTopBar.setPosition(sf::Vector2f(leftPanelX + 4.0f, leftPanelY + 4.0f));
    leftPanelTopBar.setFillColor(sf::Color(120, 220, 255, 170));
    window.draw(leftPanelTopBar);

    float lx = leftPanelX + 22.0f;
    float y = leftPanelY + 20.0f;

    sf::Text t1(font, "round " + std::to_string(round), 42);
    t1.setPosition(sf::Vector2f(lx, y));
    t1.setFillColor(sf::Color(238, 240, 250));
    window.draw(t1);
    y += 48;

    sf::Text t2(font, "frame " + std::to_string(frameInRound), 32);
    t2.setPosition(sf::Vector2f(lx, y));
    t2.setFillColor(sf::Color(226, 228, 238));
    window.draw(t2);
    y += 40;

    sf::Text t3(font, "shot " + std::to_string(shotInFrame), 32);
    t3.setPosition(sf::Vector2f(lx, y));
    t3.setFillColor(sf::Color(226, 228, 238));
    window.draw(t3);
    y += 58;

    sf::Text target(font, "score at least " + std::to_string(targetScore), 24);
    target.setPosition(sf::Vector2f(lx, y));
    target.setFillColor(sf::Color(245, 224, 138));
    window.draw(target);
    y += 72;

    int shownImpact = std::max(10, (int)std::round(hudImpactValue));
    int shownCombo = std::max(1, (int)std::round(hudComboValue));
    if (useLiveFormulaPreview) {
        shownImpact = std::max(10, liveImpactPreview);
        shownCombo = std::max(1, liveComboPreview);
    }
    sf::Text big(font, std::to_string(shownImpact) + " X " + std::to_string(shownCombo), 60);
    big.setPosition(sf::Vector2f(lx, y));
    big.setFillColor(sf::Color(120, 240, 255));
    big.setOutlineColor(sf::Color(8, 12, 18));
    big.setOutlineThickness(3.0f);
    window.draw(big);
    y += 84;

    sf::Text explain(font, "Impact x Pin Combo", 24);
    explain.setPosition(sf::Vector2f(lx, y));
    explain.setFillColor(sf::Color(255, 108, 108));
    window.draw(explain);
    y += 38;

    int shownShotAdd = 0;
    if (hudCounting) {
        shownShotAdd = (int)std::round(hudCountValue);
    } else if (hudShowBigScore) {
        shownShotAdd = hudCountTarget;
    }
    sf::Text shotScore(font, "shot add: " + std::to_string(shownShotAdd), 24);
    shotScore.setPosition(sf::Vector2f(lx, y));
    shotScore.setFillColor(hudCounting ? sf::Color(100, 255, 170) : sf::Color(235, 236, 245));
    window.draw(shotScore);
    y += 38;

    sf::Text roundScoreText(font, "round score: " + std::to_string(roundScore), 32);
    roundScoreText.setPosition(sf::Vector2f(lx, y));
    roundScoreText.setFillColor(sf::Color(246, 247, 255));
    window.draw(roundScoreText);

    y += 46;
    sf::Text tokenText(font, "tokens: " + std::to_string(tokens), 30);
    tokenText.setPosition(sf::Vector2f(lx, y));
    tokenText.setFillColor(sf::Color(255, 215, 0));
    window.draw(tokenText);

    if (!pinPowerHintLine1.empty()) {
        y += 38.f;
        sf::Text hint1(font, pinPowerHintLine1, 18);
        hint1.setPosition(sf::Vector2f(lx, y));
        hint1.setFillColor(sf::Color(230, 240, 255));
        window.draw(hint1);
    }
    if (!pinPowerHintLine2.empty()) {
        y += 24.f;
        sf::Text hint2(font, pinPowerHintLine2, 18);
        hint2.setPosition(sf::Vector2f(lx, y));
        hint2.setFillColor(sf::Color(255, 210, 140));
        window.draw(hint2);
    }

    if (hudShowBigScore && hudCountTarget > 0) {
        float t = std::clamp(hudBigTimer / 0.9f, 0.0f, 1.0f);
        float pulse = 1.0f + 0.12f * std::sin(t * 6.283185f);
        float fade = (t > 0.72f) ? (1.0f - (t - 0.72f) / 0.28f) : 1.0f;
        fade = std::clamp(fade, 0.0f, 1.0f);
        std::uint8_t alpha = (std::uint8_t)(255.0f * fade);

        sf::Text bigShot(font, "+" + std::to_string(hudCountTarget), 88);
        bigShot.setStyle(sf::Text::Bold);
        bigShot.setScale({pulse, pulse});
        bigShot.setFillColor(sf::Color(255, 230, 110, alpha));
        bigShot.setOutlineColor(sf::Color(20, 20, 20, alpha));
        bigShot.setOutlineThickness(4.0f);

        sf::FloatRect bb = bigShot.getLocalBounds();
        float centerX = leftPanelX + leftPanelW * 0.50f;
        float centerY = leftPanelY + leftPanelH * 0.56f;
        bigShot.setPosition({centerX - bb.size.x * 0.5f, centerY - bb.size.y * 0.5f});
        window.draw(bigShot);
    }

    // Right inventory panel (moved further right)
    const float inventoryOffsetX = 60.f;
    sf::RectangleShape rightPanel(sf::Vector2f(300, windowH - 80));
    rightPanel.setPosition(sf::Vector2f(windowW - 360 + inventoryOffsetX, 40));
    rightPanel.setFillColor(sf::Color(50, 50, 60));
    rightPanel.setOutlineColor(sf::Color::Black);
    rightPanel.setOutlineThickness(3.0f);
    window.draw(rightPanel);

    sf::Text invTitle(font, "inventory", 36);
    invTitle.setPosition(sf::Vector2f(windowW - 345 + inventoryOffsetX, 55));
    invTitle.setFillColor(sf::Color(200, 200, 200));
    window.draw(invTitle);

    drawInventoryPanel(window, items, windowW - 355 + inventoryOffsetX, 110, 290);

    // Menu button
    sf::RectangleShape exitBtn(sf::Vector2f(120, 40));
    exitBtn.setPosition(sf::Vector2f(windowW - 140, 20));
    exitBtn.setFillColor(sf::Color(100, 100, 100, 200));
    exitBtn.setOutlineThickness(2);
    exitBtn.setOutlineColor(sf::Color::White);
    window.draw(exitBtn);

    sf::Text exitText(font, "MENU", 20);
    exitText.setPosition(sf::Vector2f(windowW - 110, 27));
    exitText.setFillColor(sf::Color::White);
    window.draw(exitText);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);
        sf::FloatRect hit = exitBtn.getGlobalBounds();
        if (pointInRectPadded(hit, worldPos, 8.0f)) {
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
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(overlay);
    
    // Game Over text
    sf::Text gameOverText(font, "GAME OVER!", 80);
    gameOverText.setFillColor(sf::Color::Green);
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
    scoreText.setFillColor(sf::Color::Yellow);
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
        highScoreText.setFillColor(sf::Color::Green);
    } else {
        highScoreText.setFillColor(sf::Color::Cyan);
    }
    bounds = highScoreText.getLocalBounds();
    highScoreText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 + 20
    ));
    window.draw(highScoreText);
    
    // Restart instruction
    sf::Text restartText(font, "Press R to Restart", 30);
    restartText.setFillColor(sf::Color::White);
    bounds = restartText.getLocalBounds();
    restartText.setPosition(sf::Vector2f(
        windowW / 2 - bounds.size.x / 2,
        windowH / 2 + 100
    ));
    window.draw(restartText);

    sf::Text menuText(font, "Press M for Menu", 30);
    menuText.setFillColor(sf::Color::White);
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
                               float windowW,
                               float windowH) {
    if (!fontLoaded) return;

    sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
    overlay.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(overlay);

    const float panelW = 620.f;
    const float panelH = 420.f;
    const float panelX = windowW * 0.5f - panelW * 0.5f;
    const float panelY = windowH * 0.5f - panelH * 0.5f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(sf::Vector2f(panelX, panelY));
    panel.setFillColor(sf::Color(30, 32, 45, 245));
    panel.setOutlineColor(sf::Color(180, 190, 235));
    panel.setOutlineThickness(3.f);
    window.draw(panel);

    sf::Text title(font, "ROUND " + std::to_string(roundNumber) + " SUMMARY", 42);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition(sf::Vector2f(panelX + panelW * 0.5f - titleBounds.size.x * 0.5f, panelY + 18.f));
    title.setFillColor(sf::Color(245, 245, 255));
    window.draw(title);

    sf::Text status(font, passed ? "PASSED" : "FAILED", 44);
    sf::FloatRect statusBounds = status.getLocalBounds();
    status.setPosition(sf::Vector2f(panelX + panelW * 0.5f - statusBounds.size.x * 0.5f, panelY + 72.f));
    status.setFillColor(passed ? sf::Color(100, 255, 140) : sf::Color(255, 130, 130));
    window.draw(status);

    sf::RectangleShape divider(sf::Vector2f(panelW - 70.f, 2.f));
    divider.setPosition(sf::Vector2f(panelX + 35.f, panelY + 132.f));
    divider.setFillColor(sf::Color(110, 120, 165, 180));
    window.draw(divider);

    sf::Text ratio(font,
                   "Score: " + std::to_string(roundScore) + "/" + std::to_string(targetScore),
                   40);
    ratio.setPosition(sf::Vector2f(panelX + 36.f, panelY + 150.f));
    ratio.setFillColor(sf::Color(255, 220, 110));
    window.draw(ratio);

    std::string tokenEarnedText = "Tokens earned: ";
    if (tokensEarned >= 0) tokenEarnedText += "+";
    tokenEarnedText += std::to_string(tokensEarned);
    sf::Text earned(font, tokenEarnedText, 34);
    earned.setPosition(sf::Vector2f(panelX + 36.f, panelY + 222.f));
    earned.setFillColor(sf::Color(120, 235, 255));
    window.draw(earned);

    sf::Text total(font, "Tokens total: " + std::to_string(tokensTotal), 30);
    total.setPosition(sf::Vector2f(panelX + 36.f, panelY + 274.f));
    total.setFillColor(sf::Color(230, 230, 240));
    window.draw(total);

    const float btnW = 320.f;
    const float btnH = 62.f;
    const float btnX = panelX + panelW * 0.5f - btnW * 0.5f;
    const float btnY = panelY + panelH - 86.f;
    roundSummaryContinueRect = sf::FloatRect(sf::Vector2f(btnX, btnY), sf::Vector2f(btnW, btnH));

    sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
    btn.setPosition(sf::Vector2f(btnX, btnY));
    btn.setFillColor(leadsToGameOver ? sf::Color(180, 80, 80) : sf::Color(80, 190, 220));
    btn.setOutlineColor(sf::Color::Black);
    btn.setOutlineThickness(3.f);
    window.draw(btn);

    sf::Text btnText(font, leadsToGameOver ? "CONTINUE" : "CONTINUE TO SHOP", 32);
    sf::FloatRect bb = btnText.getLocalBounds();
    btnText.setPosition(sf::Vector2f(btnX + btnW * 0.5f - bb.size.x * 0.5f,
                                     btnY + btnH * 0.5f - bb.size.y * 0.5f - 6.f));
    btnText.setFillColor(sf::Color::Black);
    window.draw(btnText);
}

bool UI::handleRoundSummaryClick(sf::Vector2i mousePos) const {
    return pointInRectPadded(roundSummaryContinueRect, mousePos, 10.0f);
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
        case BallType::Midas:      return "Hit pins turn gold.\nEach gold pin gives +1 token.";
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
        case PinType::ThirdTime:   return "Every 3rd 3rd-Time knock\ndoubles combo.";
        default:                   return "Normal pin.";
    }
}

static std::string inventoryShoeDesc(ShoeType t) {
    switch (t) {
        case ShoeType::Clown:    return "Funny wobble:\nlaunch angle shifts a bit.";
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
        case PowerType::RandomUpgrade:       return "After each frame,\na random pin gains +1 value.";
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

static std::string inventoryShoeStatus(const ActiveItems& items, ShoeType type) {
    switch (type) {
        case ShoeType::Clown:
            return items.clownBonusClaimed
                ? "(currently: +10 bonus already claimed)"
                : "(currently: +10 bonus on first buy)";
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

    // ── Ball section (2 slots: shot 1 + shot 2) ─────────────────────────────
    sf::Text ballLabel(font, "BALLS", 20);
    ballLabel.setPosition({x + 10.f, iy});
    ballLabel.setFillColor({180, 180, 180});
    window.draw(ballLabel);
    iy += 28.f;

    float ballR = 22.f;
    float slot1X = x + width * 0.28f;
    float slot2X = x + width * 0.72f;

    auto drawBallSlot = [&](int slot, float sx, BallType bt) {
        sf::Text slotLabel(font, "S" + std::to_string(slot), 14);
        sf::FloatRect slb = slotLabel.getLocalBounds();
        slotLabel.setPosition({sx - slb.size.x * 0.5f, iy});
        slotLabel.setFillColor({185, 185, 205});
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
        ballName.setFillColor(sf::Color::White);
        window.draw(ballName);

        float rectW = std::max(90.f, ballR * 2.f + 58.f);
        float rectH = ballR * 2.f + 46.f;
        addHover(sf::FloatRect({sx - rectW * 0.5f, iy - 2.f}, {rectW, rectH}),
                 "S" + std::to_string(slot) + " " + ballShortName(bt),
                 inventoryBallDesc(bt),
                 "",
                 ballPreviewColor(bt));
    };

    drawBallSlot(1, slot1X, items.getBallForShot(1));
    drawBallSlot(2, slot2X, items.getBallForShot(2));
    iy += ballR * 2.f + 52.f;

    // ── Shoes section ────────────────────────────────────────────────────────
    sf::Text shoeLabel(font, "SHOES", 20);
    shoeLabel.setPosition({x + 10.f, iy});
    shoeLabel.setFillColor({180, 180, 180});
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
    shoeName.setFillColor(sf::Color::White);
    window.draw(shoeName);
    addHover(sf::FloatRect({x + 8.f, iy - 2.f}, {width - 16.f, 30.f}),
             shoeShortName(items.shoeType),
             inventoryShoeDesc(items.shoeType),
             inventoryShoeStatus(items, items.shoeType),
             shoePreviewColor(items.shoeType));
    iy += 32.f;

    std::vector<ActiveItems::PinSlotAssignment> sortedPins = items.getSortedPinAssignments();
    if (!sortedPins.empty()) {
        // ── Pins section ──────────────────────────────────────────────────────
        sf::Text pinsLabel(font, "PINS", 20);
        pinsLabel.setPosition({x + 10.f, iy});
        pinsLabel.setFillColor({180, 180, 180});
        window.draw(pinsLabel);
        iy += 28.f;

        const float lineH = 20.f;
        for (const auto& assigned : sortedPins) {
            int pt = static_cast<int>(assigned.type);
            float iconX = x + 18.f;
            float rowY = iy + 8.f;
            drawMiniPin(window, iconX, rowY, 11.f, pinPreviewColor(pt));

            int shownValue = 0;
            if (assigned.slot >= 1 && assigned.slot <= 12) {
                shownValue = items.pinSlotCurrentValues[assigned.slot - 1];
            }
            std::string valueText = (shownValue > 0) ? std::to_string(shownValue) : "-";
            std::string label = "P" + std::to_string(assigned.slot) + " " +
                                pinShortName(pt) + " (" + valueText + ")";

            sf::Text pinText(font, label, 14);
            pinText.setPosition({x + 34.f, iy});
            pinText.setFillColor({210, 210, 210});
            window.draw(pinText);
            addHover(sf::FloatRect({x + 8.f, iy - 1.f}, {width - 16.f, lineH}),
                     "P" + std::to_string(assigned.slot) + " " + pinShortName(pt),
                     inventoryPinDesc(assigned.type),
                     inventoryPinStatus(items, assigned.slot, assigned.type),
                     pinPreviewColor(pt));
            iy += lineH;
        }
        iy += 8.f;
    }

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
    forceOwned(PowerType::ExtraPins, items.powerExtraPins);
    forceOwned(PowerType::ExtraBall, items.powerExtraBall);
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
        powersLabel.setFillColor({180, 180, 180});
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
            powerText.setFillColor({220, 220, 180});
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
    shadow.setFillColor({0, 0, 0, 110});
    window.draw(shadow);

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({tipX, tipY});
    panel.setFillColor({15, 18, 34, 242});
    panel.setOutlineColor({hovered->accent.r, hovered->accent.g, hovered->accent.b, 210});
    panel.setOutlineThickness(2.f);
    window.draw(panel);

    sf::RectangleShape accentBar({panelW - 4.f, 4.f});
    accentBar.setPosition({tipX + 2.f, tipY + 2.f});
    accentBar.setFillColor({hovered->accent.r, hovered->accent.g, hovered->accent.b, 215});
    window.draw(accentBar);

    sf::Text title(font, hovered->title, titleSize);
    title.setPosition({tipX + 10.f, tipY + 8.f});
    title.setFillColor({245, 245, 250});
    window.draw(title);

    float textY = tipY + 33.f;
    for (const auto& line : bodyLines) {
        sf::Text body(font, line, bodySize);
        body.setPosition({tipX + 10.f, textY});
        body.setFillColor({214, 217, 230});
        window.draw(body);
        textY += 18.f;
    }

    if (!hovered->status.empty()) {
        sf::Text status(font, hovered->status, statusSize);
        status.setPosition({tipX + 10.f, textY + 2.f});
        status.setFillColor({120, 235, 255});
        window.draw(status);
    }
}

// Bottom bar shown during gameplay
void UI::drawInventoryBar(sf::RenderWindow& window, const ActiveItems& items,
                           float windowW, float windowH) {
    if (!fontLoaded) return;
    if (state != GameState::Xtreme) return;
    // Only show if player has something non-default
    bool hasBall = (items.getBallForShot(1) != BallType::Normal) ||
                   (items.getBallForShot(2) != BallType::Normal);
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
        float br = iconSize;
        sf::CircleShape ball1(br);
        ball1.setOrigin({br, br});
        ball1.setPosition({cx, barY + barH/2.f - 11.f});
        ball1.setFillColor(ballPreviewColor(items.getBallForShot(1)));
        ball1.setOutlineColor({255,255,255,60});
        ball1.setOutlineThickness(2.f);
        window.draw(ball1);

        sf::Text b1(font, "S1 " + ballShortName(items.getBallForShot(1)), 13);
        b1.setPosition({cx + br + 4.f, barY + barH/2.f - 20.f});
        b1.setFillColor(sf::Color::White);
        window.draw(b1);

        sf::CircleShape ball2(br);
        ball2.setOrigin({br, br});
        ball2.setPosition({cx, barY + barH/2.f + 11.f});
        ball2.setFillColor(ballPreviewColor(items.getBallForShot(2)));
        ball2.setOutlineColor({255,255,255,60});
        ball2.setOutlineThickness(2.f);
        window.draw(ball2);

        sf::Text b2(font, "S2 " + ballShortName(items.getBallForShot(2)), 13);
        b2.setPosition({cx + br + 4.f, barY + barH/2.f + 2.f});
        b2.setFillColor(sf::Color::White);
        window.draw(b2);

        // Measure text to advance cx
        sf::FloatRect b1b = b1.getLocalBounds();
        sf::FloatRect b2b = b2.getLocalBounds();
        float maxNameW = std::max(b1b.size.x, b2b.size.x);
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
    sf::FloatRect sellShoe;
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

static ShopOwnedPanelLayout computeShopOwnedPanelLayout(float windowW, float windowH, std::size_t offerCount) {
    ShopOwnedPanelLayout layout;
    layout.panelW = std::clamp(windowW * 0.24f, 270.f, 330.f);
    layout.panelH = std::min(620.f, windowH - 170.f);
    if (layout.panelH < 430.f) layout.panelH = 430.f;
    layout.panelX = windowW - layout.panelW - 22.f;
    layout.panelY = 132.f;
    if (offerCount > 4) {
        // Extra item rows need a slightly narrower panel to keep card space readable.
        layout.panelW = std::clamp(windowW * 0.22f, 250.f, 290.f);
        layout.panelH = std::min(600.f, windowH - 160.f);
        if (layout.panelH < 420.f) layout.panelH = 420.f;
        layout.panelX = 16.f;
        layout.panelY = 118.f;
    }

    const float pad = 12.f;
    const float gap = 8.f;
    const float btnH = 34.f;
    const float actionTop = layout.panelY + 52.f;
    const float actionW = layout.panelW - pad * 2.f;
    const float halfW = (actionW - gap) * 0.5f;

    layout.sellBall1 = sf::FloatRect({layout.panelX + pad, actionTop}, {halfW, btnH});
    layout.sellBall2 = sf::FloatRect({layout.panelX + pad + halfW + gap, actionTop}, {halfW, btnH});
    layout.sellShoe  = sf::FloatRect({layout.panelX + pad, actionTop + btnH + gap}, {actionW, btnH});
    return layout;
}

static ShopCardLayout computeShopCardLayout(float windowW,
                                            const ShopOwnedPanelLayout& ownedLayout,
                                            std::size_t offerCount) {
    ShopCardLayout out;
    // Reserve the left control column (ball slot + pin slot controls), so cards
    // never cover it in smaller windowed layouts.
    out.areaMinX = std::max(out.areaMinX, 250.f);
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
    std::vector<int> remaining(powerCount, 0);

    for (int p = 0; p < powerCount; p++) {
        PowerType t = static_cast<PowerType>(p);
        if (!items.hasPower(t)) continue;
        // Extra Slot is one-time and cannot be sold in-run.
        if (t == PowerType::ExtraPowerSlot) continue;
        if (t == PowerType::Duplicate) {
            remaining[p] = std::max(0, items.duplicateCharges);
        } else if (t == PowerType::SwapPins) {
            remaining[p] = std::max(0, items.swapCharges);
        } else if (t == PowerType::SevenEightNine) {
            remaining[p] = std::max(0, items.sevenEightNineCharges);
        } else {
            remaining[p] = 1;
        }
    }

    std::vector<PowerType> out;
    out.reserve(items.purchasedPowers.size());
    for (int raw : items.purchasedPowers) {
        if (raw < 0 || raw >= powerCount) continue;
        if (remaining[raw] <= 0) continue;
        out.push_back(static_cast<PowerType>(raw));
        remaining[raw]--;
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
    const float rowH = 28.f;
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

    ShopOwnedPanelLayout layout = computeShopOwnedPanelLayout(windowW, windowH, shopOffers.size());

    sf::RectangleShape panel({layout.panelW, layout.panelH});
    panel.setPosition({layout.panelX, layout.panelY});
    panel.setFillColor({20, 22, 40, 250});
    panel.setOutlineColor({120, 128, 175});
    panel.setOutlineThickness(2.2f);
    window.draw(panel);

    sf::RectangleShape head({layout.panelW, 36.f});
    head.setPosition({layout.panelX, layout.panelY});
    head.setFillColor({38, 44, 78, 245});
    window.draw(head);

    sf::Text title(font, "OWNED INVENTORY", 24);
    title.setPosition({layout.panelX + 12.f, layout.panelY + 4.f});
    title.setFillColor({210, 218, 245});
    window.draw(title);

    ShopOwnedDynamicLayout dynamic = computeShopOwnedDynamicLayout(layout, items);

    const float actionAreaY = layout.sellBall1.position.y - 8.f;
    const float actionAreaH = (dynamic.contentTop - actionAreaY) - 8.f;
    if (actionAreaH > 20.f) {
        sf::RectangleShape actionArea({layout.panelW - 14.f, actionAreaH});
        actionArea.setPosition({layout.panelX + 7.f, actionAreaY});
        actionArea.setFillColor({30, 33, 56, 232});
        actionArea.setOutlineColor({96, 104, 150, 180});
        actionArea.setOutlineThickness(1.4f);
        window.draw(actionArea);
    }

    BallType slot1Ball = items.getBallForSlot(1);
    BallType slot2Ball = items.getBallForSlot(2);
    bool canSellS1 = (slot1Ball != BallType::Normal);
    bool canSellS2 = (slot2Ball != BallType::Normal);
    bool canSellShoe = (items.shoeType != ShoeType::None);
    int s1Value = canSellS1 ? (ballTypeCost(slot1Ball) / 2) : 0;
    int s2Value = canSellS2 ? (ballTypeCost(slot2Ball) / 2) : 0;
    int shoeValue = canSellShoe ? (shoeTypeCost(items.shoeType) / 2) : 0;

    sf::Text quick(font, "QUICK SELL", 14);
    quick.setPosition({layout.panelX + 14.f, layout.sellBall1.position.y - 20.f});
    quick.setFillColor({170, 182, 225});
    window.draw(quick);

    auto drawSellButton = [&](const sf::FloatRect& rect, const std::string& label, bool enabled, int charSize) {
        sf::RectangleShape button(rect.size);
        button.setPosition(rect.position);
        button.setFillColor(enabled ? sf::Color(186, 114, 58) : sf::Color(68, 70, 92));
        button.setOutlineColor(enabled ? sf::Color(242, 180, 132) : sf::Color(92, 95, 122));
        button.setOutlineThickness(2.f);
        window.draw(button);

        sf::Text text(font, label, charSize);
        text.setPosition({rect.position.x + 9.f, rect.position.y + (rect.size.y - (float)charSize) * 0.5f - 2.f});
        text.setFillColor(enabled ? sf::Color(245, 246, 252) : sf::Color(155, 158, 180));
        window.draw(text);
    };

    drawSellButton(layout.sellBall1, "Sell S1 (+" + std::to_string(s1Value) + ")", canSellS1, 19);
    drawSellButton(layout.sellBall2, "Sell S2 (+" + std::to_string(s2Value) + ")", canSellS2, 19);
    drawSellButton(layout.sellShoe, "Sell Shoe (+" + std::to_string(shoeValue) + ")", canSellShoe, 18);

    sf::Text pinHeader(font, "SELL PINS", 15);
    pinHeader.setPosition({layout.panelX + 10.f, dynamic.pinHeaderY});
    pinHeader.setFillColor({184, 195, 235});
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
        none.setFillColor({132, 136, 160});
        window.draw(none);
    }

    sf::Text powerHeader(font, "SELL POWERS", 15);
    powerHeader.setPosition({layout.panelX + 10.f, dynamic.powerHeaderY});
    powerHeader.setFillColor({184, 195, 235});
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
        none.setFillColor({132, 136, 160});
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
        case BallType::Midas:      return "Hit pins turn gold.\nEach gold pin = +1 token.";
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
         "Midas Ball",   "Hit pins turn gold.\nEach = +1 token.",              5},
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
         "Exploding Pin","One pin explodes\n1s after being knocked.",           4},
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
         "3rd Time",     "Every 3rd 3rd-Time\nknock doubles combo.",           3},
        // Shoes
        {ShopItemCategory::Shoe, BallType::Normal, PinType::Normal, ShoeType::Clown, PowerType::Greedy,
         "Clown Shoes",  "Funny wobble:\nlaunch angle shifts a bit.\nFirst buy: +10 dollars.", 0},
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
         "Random Upgrade","After each frame,\na random pin gains +1 value.",    3},
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
    ShopOwnedPanelLayout ownedLayout = computeShopOwnedPanelLayout(windowW, windowH, shopOffers.size());

    // Background
    sf::RectangleShape bg(sf::Vector2f(windowW, windowH));
    bg.setFillColor(sf::Color(25, 25, 35));
    window.draw(bg);

    // Title
    sf::Text title(font, "SHOP", 80);
    title.setPosition({windowW/2.f - 80.f, 30.f});
    title.setFillColor(sf::Color(255, 215, 0));
    title.setOutlineColor(sf::Color::Black);
    title.setOutlineThickness(3);
    window.draw(title);

    // Tokens display
    sf::Text tokenText(font, "Tokens: " + std::to_string(tokens), 36);
    tokenText.setPosition({windowW - 240.f, 40.f});
    tokenText.setFillColor(sf::Color(255, 215, 0));
    tokenText.setOutlineColor(sf::Color::Black);
    tokenText.setOutlineThickness(2);
    window.draw(tokenText);

    const float skipX = windowW - 250.f;
    const float skipY = 88.f;
    const float skipW = 220.f;
    const float skipH = 40.f;
    if (items.skipCharges > 0) {
        bool canUseSkip = tokens >= 1;
        sf::RectangleShape skipBtn({skipW, skipH});
        skipBtn.setPosition({skipX, skipY});
        skipBtn.setFillColor(canUseSkip ? sf::Color(120, 170, 240) : sf::Color(70, 80, 100));
        skipBtn.setOutlineColor(sf::Color::Black);
        skipBtn.setOutlineThickness(2.f);
        window.draw(skipBtn);

        sf::Text skipText(font,
                          "REROLL (1) x" + std::to_string(items.skipCharges),
                          20);
        skipText.setPosition({skipX + 14.f, skipY + 8.f});
        skipText.setFillColor(sf::Color::White);
        window.draw(skipText);
    }

    bool hasExtraPinsPower =
        items.powerExtraPins || items.hasPurchasedPower(PowerType::ExtraPins);
    int pinBuyLimit = hasExtraPinsPower ? 12 : 10;
    selectedPinSlot = std::clamp(selectedPinSlot, 1, pinBuyLimit);

    // Ball slot selector (which slot a purchased ball should fill)
    sf::Text slotTitle(font, "Ball Slot", 22);
    slotTitle.setPosition({40.f, 38.f});
    slotTitle.setFillColor({220, 220, 230});
    window.draw(slotTitle);

    const float slotY = 70.f;
    const float slotW = 130.f;
    const float slotH = 44.f;
    const float slot1X = 40.f;
    const float slot2X = slot1X + slotW + 12.f;

    auto drawSlotButton = [&](int slot, float x) {
        bool selected = (selectedBallSlot == slot);
        sf::RectangleShape b({slotW, slotH});
        b.setPosition({x, slotY});
        b.setFillColor(selected ? sf::Color(70, 170, 120) : sf::Color(55, 55, 70));
        b.setOutlineColor(selected ? sf::Color(200, 255, 220) : sf::Color(130, 130, 160));
        b.setOutlineThickness(2.f);
        window.draw(b);

        sf::Text t(font, "SHOT " + std::to_string(slot), 20);
        t.setPosition({x + 14.f, slotY + 10.f});
        t.setFillColor(sf::Color::White);
        window.draw(t);
    };

    drawSlotButton(1, slot1X);
    drawSlotButton(2, slot2X);

    sf::Text slotCurrent(font, "Current: " + ballShortName(items.getBallForSlot(selectedBallSlot)), 20);
    slotCurrent.setPosition({40.f, 122.f});
    slotCurrent.setFillColor({200, 200, 200});
    window.draw(slotCurrent);

    sf::Text pinSlotTitle(font, "Pin Slot", 20);
    pinSlotTitle.setPosition({40.f, 150.f});
    pinSlotTitle.setFillColor({220, 220, 230});
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
    pinPrev.setFillColor(sf::Color(55, 55, 70));
    pinPrev.setOutlineColor(sf::Color(130, 130, 160));
    pinPrev.setOutlineThickness(2.f);
    window.draw(pinPrev);

    sf::Text prevText(font, "<", 20);
    prevText.setPosition({pinPrevX + 10.f, pinSlotY + 4.f});
    prevText.setFillColor(sf::Color::White);
    window.draw(prevText);

    bool assigned = items.hasPinAssignmentAtSlot(selectedPinSlot);
    sf::RectangleShape pinBadge({pinBadgeW, pinBtnH});
    pinBadge.setPosition({pinBadgeX, pinSlotY});
    pinBadge.setFillColor(assigned ? sf::Color(70, 90, 120) : sf::Color(52, 52, 70));
    pinBadge.setOutlineColor(sf::Color(175, 255, 205));
    pinBadge.setOutlineThickness(2.f);
    window.draw(pinBadge);

    sf::Text pinBadgeText(font,
                          "P" + std::to_string(selectedPinSlot) + "/" + std::to_string(pinBuyLimit),
                          16);
    pinBadgeText.setPosition({pinBadgeX + 11.f, pinSlotY + 7.f});
    pinBadgeText.setFillColor(sf::Color::White);
    window.draw(pinBadgeText);

    sf::RectangleShape pinNext({pinBtnW, pinBtnH});
    pinNext.setPosition({pinNextX, pinSlotY});
    pinNext.setFillColor(sf::Color(55, 55, 70));
    pinNext.setOutlineColor(sf::Color(130, 130, 160));
    pinNext.setOutlineThickness(2.f);
    window.draw(pinNext);

    sf::Text nextText(font, ">", 20);
    nextText.setPosition({pinNextX + 10.f, pinSlotY + 4.f});
    nextText.setFillColor(sf::Color::White);
    window.draw(nextText);

    PinType selectedPinType = items.getPinTypeForSlot(selectedPinSlot);
    sf::Text pinSlotCurrent(font,
        "Current P" + std::to_string(selectedPinSlot) + ": " +
        pinShortName(static_cast<int>(selectedPinType)), 18);
    float pinInfoY = pinSlotY + pinBtnH + 6.f;
    pinSlotCurrent.setPosition({40.f, pinInfoY});
    pinSlotCurrent.setFillColor({190, 198, 215});
    window.draw(pinSlotCurrent);

    ShopCardLayout cardLayout = computeShopCardLayout(windowW, ownedLayout, shopOffers.size());

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

        if (isPin) {
            PinType selectedType = items.getPinTypeForSlot(selectedPinSlot);
            isOwned = (selectedType == offer.pinType);
        }
        bool pinLimitBlocked = false;
        if (isPin && !isOwned) {
            bool slotEmpty = !items.hasPinAssignmentAtSlot(selectedPinSlot);
            pinLimitBlocked = slotEmpty && (items.getPinAssignmentCount() >= pinBuyLimit);
        }

        // Card background tint by category
        sf::Color cardColor = isOwned         ? sf::Color(40, 100, 40)  :
                              (!canAfford || powerLimitBlocked || pinLimitBlocked)
                                              ? sf::Color(40, 40, 40)   :
                              isBall          ? sf::Color(40, 50, 80)   :
                              isPin           ? sf::Color(40, 70, 50)   :
                              isShoe          ? sf::Color(70, 45, 45)   :
                                                sf::Color(65, 50, 35);
        sf::Color borderColor = isOwned       ? sf::Color(80, 220, 80)  :
                                (!canAfford || powerLimitBlocked || pinLimitBlocked)
                                              ? sf::Color(80, 80, 80)   :
                                isBall        ? sf::Color(140, 160, 255):
                                isPin         ? sf::Color(120, 220, 140):
                                isShoe        ? sf::Color(240, 170, 130):
                                                sf::Color(255, 210, 120);

        sf::RectangleShape card({cardLayout.cardW, cardLayout.cardH});
        card.setPosition({cx, cardY});
        card.setFillColor(cardColor);
        card.setOutlineColor(borderColor);
        card.setOutlineThickness(3);
        window.draw(card);

        // Category badge
        sf::RectangleShape badge({cardLayout.cardW, 28.f});
        badge.setPosition({cx, cardY});
        badge.setFillColor(isBall ? sf::Color(60,80,150) :
                          isPin  ? sf::Color(40,110,70) :
                          isShoe ? sf::Color(120,70,60) :
                                   sf::Color(120,95,35));
        window.draw(badge);

        sf::Text catLabel(font, isBall ? "BALL" : (isPin ? "PIN" : (isShoe ? "SHOE" : "POWER")), 20);
        catLabel.setPosition({cx + 10.f, cardY + 4.f});
        catLabel.setFillColor(sf::Color::White);
        window.draw(catLabel);

        // Item name
        sf::Text nameText(font, offer.name, 26);
        nameText.setPosition({cx + 10.f, cardY + 34.f});
        nameText.setFillColor(sf::Color::White);
        window.draw(nameText);

        // Preview circle
        float pr = 36.f;
        sf::CircleShape preview(pr);
        preview.setOrigin({pr, pr});
        preview.setPosition({cx + cardLayout.cardW/2.f, cardY + 135.f});

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
        float dy = cardY + 198.f;
        std::string line;
        for (char ch : desc) {
            if (ch == '\n') {
                sf::Text lt(font, line, 20);
                lt.setPosition({cx + 12.f, dy});
                lt.setFillColor(sf::Color(200, 200, 200));
                window.draw(lt);
                dy += 26.f;
                line.clear();
            } else line += ch;
        }
        if (!line.empty()) {
            sf::Text lt(font, line, 20);
            lt.setPosition({cx + 12.f, dy});
            lt.setFillColor(sf::Color(200, 200, 200));
            window.draw(lt);
        }

        // Buy button
        sf::Color btnColor = isOwned     ? sf::Color(70, 70, 80) :
                             (canAfford && !powerLimitBlocked && !pinLimitBlocked)
                                         ? sf::Color(80, 200, 120) :
                                           sf::Color(80, 80, 80);
        sf::RectangleShape btn({cardLayout.cardW - 20.f, 44.f});
        btn.setPosition({cx + 10.f, cardY + cardLayout.cardH - 56.f});
        btn.setFillColor(btnColor);
        btn.setOutlineColor(sf::Color::Black);
        btn.setOutlineThickness(2);
        window.draw(btn);

        std::string btnLabel;
        if (isOwned) {
            if (isBall || isShoe) btnLabel = "EQUIPPED";
            else btnLabel = "OWNED";
        } else if (powerLimitBlocked) {
            btnLabel = "Power limit (" + std::to_string(maxPermanentPowers) + ")";
        } else if (pinLimitBlocked) {
            btnLabel = "Pin limit reached";
        } else if (canAfford) {
            btnLabel = "BUY (" + std::to_string(offer.cost) + " tokens)";
        } else {
            btnLabel = "Need " + std::to_string(offer.cost) + " tokens";
        }
        sf::Text btnText(font, btnLabel, 20);
        btnText.setPosition({cx + 18.f, cardY + cardLayout.cardH - 48.f});
        btnText.setFillColor(sf::Color::White);
        window.draw(btnText);
    }

    // Continue button
    sf::RectangleShape cont({220.f, 60.f});
    cont.setPosition({windowW/2.f - 110.f, windowH - 100.f});
    cont.setFillColor(sf::Color(80, 200, 220));
    cont.setOutlineColor(sf::Color::Black);
    cont.setOutlineThickness(3);
    window.draw(cont);

    sf::Text contText(font, "CONTINUE", 30);
    contText.setPosition({windowW/2.f - 90.f, windowH - 88.f});
    contText.setFillColor(sf::Color::Black);
    window.draw(contText);

    // Show what the player already owns
    drawInventoryInShop(window, items, windowW, windowH);
}

int UI::handleShopClick(sf::RenderWindow& window, sf::Vector2i mousePos, int tokens, const ActiveItems& items) {
    sf::View v = window.getView();
    float windowW = v.getSize().x;
    float windowH = v.getSize().y;

    // Slot selector buttons
    const float slotY = 70.f;
    const float slotW = 130.f;
    const float slotH = 44.f;
    const float slot1X = 40.f;
    const float slot2X = slot1X + slotW + 12.f;
    const float buttonPad = 10.0f;

    sf::FloatRect slot1Rect(sf::Vector2f(slot1X, slotY), sf::Vector2f(slotW, slotH));
    sf::FloatRect slot2Rect(sf::Vector2f(slot2X, slotY), sf::Vector2f(slotW, slotH));

    if (pointInRect(slot1Rect, mousePos, buttonPad)) {
        selectedBallSlot = 1;
        return ShopActionNone;
    }
    if (pointInRect(slot2Rect, mousePos, buttonPad)) {
        selectedBallSlot = 2;
        return ShopActionNone;
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

    const float skipX = windowW - 250.f;
    const float skipY = 88.f;
    const float skipW = 220.f;
    const float skipH = 40.f;
    sf::FloatRect rerollRect(sf::Vector2f(skipX, skipY), sf::Vector2f(skipW, skipH));
    if (items.skipCharges > 0 && pointInRect(rerollRect, mousePos, buttonPad)) {
        if (tokens >= 1) return ShopActionReroll;
        return ShopActionNone;
    }

    ShopOwnedPanelLayout layout = computeShopOwnedPanelLayout(windowW, windowH, shopOffers.size());
    ShopOwnedDynamicLayout dynamic = computeShopOwnedDynamicLayout(layout, items);
    if (pointInRect(layout.sellBall1, mousePos, 6.0f) && items.getBallForSlot(1) != BallType::Normal) {
        return ShopActionSellBallSlot1;
    }
    if (pointInRect(layout.sellBall2, mousePos, 6.0f) && items.getBallForSlot(2) != BallType::Normal) {
        return ShopActionSellBallSlot2;
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

    ShopCardLayout cardLayout = computeShopCardLayout(windowW, layout, shopOffers.size());
    const int maxPermanentPowers = items.getMaxPermanentPowerSlots();

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
        float btnY = cardY + cardLayout.cardH - 56.f;
        float btnW = cardLayout.cardW - 20.f;
        float btnH = 44.f;

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
            if (offer.category == ShopItemCategory::Power) {
                canBuy = canBuy && canBuyPowerWithLimit(items, offer.powerType, maxPermanentPowers);
            }
            if (offer.category == ShopItemCategory::Pin) {
                bool slotEmpty = !items.hasPinAssignmentAtSlot(selectedPinSlot);
                if (slotEmpty) {
                    canBuy = canBuy && (items.getPinAssignmentCount() < pinBuyLimit);
                }
            }

            if (canBuy) {
                return i;
            }
        }
    }
    return ShopActionNone;
}
