#include "Lane.h"
#include "rlgl.h"

void Lane::init(float windowW) {
    left   = (windowW - width) / 2.0f;
    right  = left + width;
    bottom = top + height;
}

static void drawGradientQuad(float x0, float y0, float x1, float y1,
                              Color c00, Color c10, Color c01, Color c11) {
    rlBegin(RL_TRIANGLES);
    rlColor4ub(c00.r, c00.g, c00.b, c00.a); rlVertex2f(x0, y0);
    rlColor4ub(c10.r, c10.g, c10.b, c10.a); rlVertex2f(x1, y0);
    rlColor4ub(c01.r, c01.g, c01.b, c01.a); rlVertex2f(x0, y1);

    rlColor4ub(c10.r, c10.g, c10.b, c10.a); rlVertex2f(x1, y0);
    rlColor4ub(c11.r, c11.g, c11.b, c11.a); rlVertex2f(x1, y1);
    rlColor4ub(c01.r, c01.g, c01.b, c01.a); rlVertex2f(x0, y1);
    rlEnd();
}

void Lane::draw() const {
    // Outer drop shadow
    DrawRectangle((int)(left - 20.0f), (int)(top - 28.0f),
                  (int)(width + 40.0f), (int)(height + 56.0f),
                  {0, 0, 0, 86});

    // Metallic lane frame
    drawGradientQuad(left - 14.0f, top - 22.0f, right + 14.0f, bottom + 22.0f,
        {54, 59, 74, 255}, {60, 66, 82, 255}, {28, 32, 46, 255}, {24, 30, 44, 255});

    // Frame caps
    DrawRectangle((int)(left - 14.0f), (int)(top - 28.0f), (int)(width + 28.0f), 18, {18, 21, 30, 255});
    DrawRectangle((int)(left - 14.0f), (int)(bottom + 10.0f), (int)(width + 28.0f), 18, {18, 21, 30, 255});

    // Lane wood base gradient
    drawGradientQuad(left, top, right, bottom,
        {186, 146, 90, 255}, {180, 140, 86, 255}, {148, 106, 62, 255}, {142, 102, 60, 255});

    // Planks
    const int plankCount = 11;
    for (int i = 0; i < plankCount; i++) {
        float stripeW = width / (float)plankCount;
        Color pc = (i % 2 == 0)
            ? Color{176, 132, 80, 96}
            : Color{160, 118, 72, 96};
        DrawRectangle((int)(left + i * stripeW), (int)top, (int)stripeW, (int)height, pc);
    }

    // Center polish glow
    DrawRectangle((int)(left + width * 0.42f), (int)top, (int)(width * 0.16f), (int)height, {255, 245, 214, 20});
    DrawRectangle((int)(left + width * 0.50f), (int)top, 2, (int)height, {255, 245, 214, 34});

    // Gutter gradients
    auto drawGutter = [&](float x) {
        drawGradientQuad(x, top, x + gutterWidth, bottom,
            {46, 54, 78, 255}, {36, 43, 66, 255}, {20, 25, 40, 255}, {16, 20, 34, 255});
    };
    drawGutter(left);
    drawGutter(right - gutterWidth);

    // Bumpers
    if (bumpersOn) {
        DrawRectangle((int)(left + gutterWidth), (int)top, (int)bumperThickness, (int)height, {190, 248, 255, 225});
        DrawRectangleLinesEx({left + gutterWidth, top, bumperThickness, height}, 1.0f, {78, 180, 220, 255});

        DrawRectangle((int)(right - gutterWidth - bumperThickness), (int)top, (int)bumperThickness, (int)height, {190, 248, 255, 225});
        DrawRectangleLinesEx({right - gutterWidth - bumperThickness, top, bumperThickness, height}, 1.0f, {78, 180, 220, 255});
    }

    // Inner border
    DrawRectangleLinesEx({left, top, width, height}, 3.0f, {86, 96, 120, 255});
}

float Lane::playLeft()  const { return left + gutterWidth + bumperThickness; }
float Lane::playRight() const { return right - gutterWidth - bumperThickness; }
float Lane::centerX()   const { return left + width * 0.5f; }
