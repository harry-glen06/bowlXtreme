#pragma once
#include "raylib.h"

struct Lane {
    float width = 300.0f;
    float top = 40.0f;
    float height = 940.0f;

    float gutterWidth = 28.0f;
    float bumperThickness = 6.0f;

    float left = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    bool bumpersOn = false;

    void init(float windowW);
    void draw() const;

    float playLeft() const;
    float playRight() const;
    float centerX() const;
};
