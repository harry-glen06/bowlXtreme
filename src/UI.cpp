#include "UI.h"
#include <cmath>
#include <cstdlib>

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

UI::UI() {
    loadFont();
}

void UI::loadFont() {
    fontLoaded = font.openFromFile("assets/arial.ttf");
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
    float windowW = 1024.0f; //window.getSize().x;
    float windowH = 1024.0f; //window.getSize().y;
    
    float buttonY = windowH - 200;
    float buttonSpacing = 20.0f;
    float buttonWidth = 160.0f;
    float buttonHeight = 60.0f;
    
    // Check Normal button
    if (mousePos.x >= windowW / 2 - buttonWidth * 1.5f - buttonSpacing &&
        mousePos.x <= windowW / 2 - buttonWidth * 1.5f - buttonSpacing + buttonWidth &&
        mousePos.y >= buttonY && mousePos.y <= buttonY + buttonHeight) {
        return MenuButton::Normal;
    }
    
    // Check Xtreme button
    if (mousePos.x >= windowW / 2 - buttonWidth / 2 &&
        mousePos.x <= windowW / 2 - buttonWidth / 2 + buttonWidth &&
        mousePos.y >= buttonY && mousePos.y <= buttonY + buttonHeight) {
        return MenuButton::Xtreme;
    }
    
    // Check Settings button
    if (mousePos.x >= windowW / 2 + buttonWidth / 2 + buttonSpacing &&
        mousePos.x <= windowW / 2 + buttonWidth / 2 + buttonSpacing + buttonWidth &&
        mousePos.y >= buttonY && mousePos.y <= buttonY + buttonHeight) {
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
    float windowW = window.getSize().x;
    float windowH = window.getSize().y;
    
    float panelW = 600;
    float panelH = 500;
    float startX = windowW / 2 - panelW / 2 + 40;
    float startY = windowH / 2 - panelH / 2 + 40;
    
    // Check bumpers checkbox
    if (mousePos.x >= startX + 200 && mousePos.x <= startX + 230 &&
        mousePos.y >= startY + 250 && mousePos.y <= startY + 280) {
        bumpersDefault = !bumpersDefault;
    }
    
    // Check back button
    if (mousePos.x >= windowW / 2 - 100 && mousePos.x <= windowW / 2 + 100 &&
        mousePos.y >= startY + 350 && mousePos.y <= startY + 400) {
        inSettings = false;
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

        if (exitBtn.getGlobalBounds().contains(worldPos)) {
            return GameAction::ExitToMenu;
        }
    }
    return GameAction::None;
}

GameAction UI::drawXtremeHUD(sf::RenderWindow& window,
                             int round,
                             int frameInRound,
                             int shotInFrame,
                             int targetScore,
                             int roundScore,
                             int impact,
                             int combo,
                             int lastShotScore,
                             float windowW,
                             float windowH) {
    if (!fontLoaded) return GameAction::None;

    // Left info panel
    sf::RectangleShape leftPanel(sf::Vector2f(250, windowH - 80));
    leftPanel.setPosition(sf::Vector2f(40, 40));
    leftPanel.setFillColor(sf::Color(70, 70, 70));
    leftPanel.setOutlineColor(sf::Color::Black);
    leftPanel.setOutlineThickness(3.0f);
    window.draw(leftPanel);

    float lx = 60.0f;
    float y = 60.0f;

    sf::Text t1(font, "round " + std::to_string(round), 36);
    t1.setPosition(sf::Vector2f(lx, y));
    t1.setFillColor(sf::Color::Black);
    window.draw(t1);
    y += 44;

    sf::Text t2(font, "frame " + std::to_string(frameInRound), 30);
    t2.setPosition(sf::Vector2f(lx, y));
    t2.setFillColor(sf::Color::Black);
    window.draw(t2);
    y += 38;

    sf::Text t3(font, "shot " + std::to_string(shotInFrame), 30);
    t3.setPosition(sf::Vector2f(lx, y));
    t3.setFillColor(sf::Color::Black);
    window.draw(t3);
    y += 60;

    sf::Text target(font, "score at least " + std::to_string(targetScore), 22);
    target.setPosition(sf::Vector2f(lx, y));
    target.setFillColor(sf::Color::Black);
    window.draw(target);
    y += 70;

    // Big impact x combo
    sf::Text big(font, std::to_string(impact) + " X " + std::to_string(combo), 56);
    big.setPosition(sf::Vector2f(lx, y));
    big.setFillColor(sf::Color(120, 240, 255));
    big.setOutlineColor(sf::Color::Black);
    big.setOutlineThickness(3.0f);
    window.draw(big);
    y += 80;

    sf::Text explain(font, "Impact x Pin Combo", 22);
    explain.setPosition(sf::Vector2f(lx, y));
    explain.setFillColor(sf::Color(255, 80, 80));
    window.draw(explain);
    y += 34;

    sf::Text shotScore(font, "shot score: " + std::to_string(lastShotScore), 22);
    shotScore.setPosition(sf::Vector2f(lx, y));
    shotScore.setFillColor(sf::Color::Black);
    window.draw(shotScore);
    y += 34;

    sf::Text roundScoreText(font, "round score: " + std::to_string(roundScore), 32);
    roundScoreText.setPosition(sf::Vector2f(lx, y));
    roundScoreText.setFillColor(sf::Color::Black);
    window.draw(roundScoreText);

    // Right items panel (placeholder categories)
    sf::RectangleShape rightPanel(sf::Vector2f(300, windowH - 80));
    rightPanel.setPosition(sf::Vector2f(windowW - 360, 40));
    rightPanel.setFillColor(sf::Color(90, 90, 90));
    rightPanel.setOutlineColor(sf::Color::Black);
    rightPanel.setOutlineThickness(3.0f);
    window.draw(rightPanel);

    sf::Text shopTitle(font, "items", 72);
    shopTitle.setPosition(sf::Vector2f(windowW - 330, 70));
    shopTitle.setFillColor(sf::Color::Black);
    window.draw(shopTitle);

    sf::Text c1(font, "shoes", 52);
    c1.setPosition(sf::Vector2f(windowW - 330, 170));
    c1.setFillColor(sf::Color::Black);
    window.draw(c1);

    sf::Text c2(font, "balls", 52);
    c2.setPosition(sf::Vector2f(windowW - 330, 320));
    c2.setFillColor(sf::Color::Black);
    window.draw(c2);

    sf::Text c3(font, "powers", 52);
    c3.setPosition(sf::Vector2f(windowW - 330, 540));
    c3.setFillColor(sf::Color::Black);
    window.draw(c3);

    // Menu button (top-right)
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
        if (exitBtn.getGlobalBounds().contains(worldPos)) {
            return GameAction::ExitToMenu;
        }
    }

    return GameAction::None;
}

void UI::drawGameOverScreen(sf::RenderWindow& window, 
                            GameOverMode mode,
                            int finalScore, 
                            int highScore,
                            float windowW, 
                            float windowH) {
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