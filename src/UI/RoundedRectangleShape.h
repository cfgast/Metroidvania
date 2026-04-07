#pragma once

#include <SFML/Graphics/ConvexShape.hpp>
#include <cmath>

// Custom SFML shape that draws a rectangle with rounded corners.
class RoundedRectangleShape : public sf::ConvexShape
{
public:
    RoundedRectangleShape(const sf::Vector2f& size = {0.f, 0.f},
                          float radius = 8.f,
                          unsigned int cornerPoints = 6)
    {
        setParameters(size, radius, cornerPoints);
    }

    void setParameters(const sf::Vector2f& size, float radius, unsigned int cornerPoints = 6)
    {
        m_size = size;
        m_radius = radius;
        m_cornerPoints = cornerPoints;
        rebuild();
    }

    void setRectSize(const sf::Vector2f& size)
    {
        m_size = size;
        rebuild();
    }

    void setCornerRadius(float radius)
    {
        m_radius = radius;
        rebuild();
    }

    sf::Vector2f getRectSize() const { return m_size; }
    float getCornerRadius() const { return m_radius; }

private:
    void rebuild()
    {
        float r = std::min(m_radius, std::min(m_size.x * 0.5f, m_size.y * 0.5f));
        unsigned int total = m_cornerPoints * 4;
        setPointCount(total);

        // Top-right corner
        for (unsigned int i = 0; i < m_cornerPoints; ++i)
        {
            float angle = static_cast<float>(i) / static_cast<float>(m_cornerPoints - 1) * 90.f;
            float rad = angle * 3.14159265f / 180.f;
            float x = m_size.x - r + r * std::cos(rad);
            float y = r - r * std::sin(rad);
            setPoint(i, {x, y});
        }

        // Bottom-right corner
        for (unsigned int i = 0; i < m_cornerPoints; ++i)
        {
            float angle = static_cast<float>(i) / static_cast<float>(m_cornerPoints - 1) * 90.f;
            float rad = angle * 3.14159265f / 180.f;
            float x = m_size.x - r + r * std::sin(rad);
            float y = m_size.y - r + r * std::cos(rad);
            setPoint(m_cornerPoints + i, {x, y});
        }

        // Bottom-left corner
        for (unsigned int i = 0; i < m_cornerPoints; ++i)
        {
            float angle = static_cast<float>(i) / static_cast<float>(m_cornerPoints - 1) * 90.f;
            float rad = angle * 3.14159265f / 180.f;
            float x = r - r * std::cos(rad);
            float y = m_size.y - r + r * std::sin(rad);
            setPoint(2 * m_cornerPoints + i, {x, y});
        }

        // Top-left corner
        for (unsigned int i = 0; i < m_cornerPoints; ++i)
        {
            float angle = static_cast<float>(i) / static_cast<float>(m_cornerPoints - 1) * 90.f;
            float rad = angle * 3.14159265f / 180.f;
            float x = r - r * std::sin(rad);
            float y = r - r * std::cos(rad);
            setPoint(3 * m_cornerPoints + i, {x, y});
        }
    }

    sf::Vector2f m_size;
    float m_radius = 8.f;
    unsigned int m_cornerPoints = 6;
};
