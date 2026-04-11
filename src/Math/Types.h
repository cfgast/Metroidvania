#pragma once

#include <cstdint>
#include <glm/vec2.hpp>

// Axis-aligned rectangle (float).  Drop-in replacement for sf::FloatRect.
struct Rect
{
    float x = 0.f;
    float y = 0.f;
    float width  = 0.f;
    float height = 0.f;

    Rect() = default;
    Rect(float x, float y, float w, float h)
        : x(x), y(y), width(w), height(h) {}

    bool intersects(const Rect& other) const
    {
        return x < other.x + other.width  && x + width  > other.x &&
               y < other.y + other.height && y + height > other.y;
    }

    bool contains(glm::vec2 point) const
    {
        return point.x >= x && point.x <= x + width &&
               point.y >= y && point.y <= y + height;
    }
};

// Axis-aligned rectangle (int).  Drop-in replacement for sf::IntRect.
struct IntRect
{
    int x = 0;
    int y = 0;
    int width  = 0;
    int height = 0;

    IntRect() = default;
    IntRect(int x, int y, int w, int h)
        : x(x), y(y), width(w), height(h) {}
};

// RGBA color stored as bytes.  Drop-in replacement for sf::Color.
struct Color
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    float rf() const { return r / 255.f; }
    float gf() const { return g / 255.f; }
    float bf() const { return b / 255.f; }
    float af() const { return a / 255.f; }
};
