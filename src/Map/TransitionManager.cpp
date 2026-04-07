#include "TransitionManager.h"

#include <algorithm>

#include <SFML/Graphics/RenderWindow.hpp>

void TransitionManager::startTransition(const std::string& targetMap,
                                        const std::string& targetSpawn)
{
    if (m_state != State::Idle)
        return;

    m_pendingMap   = targetMap;
    m_pendingSpawn = targetSpawn;
    m_timer        = 0.f;
    m_state        = State::FadingOut;
}

bool TransitionManager::update(float dt)
{
    if (m_state == State::Idle)
        return false;

    m_timer += dt;

    if (m_state == State::FadingOut && m_timer >= m_fadeDuration)
    {
        // Mid-point: screen is fully black — load the new map.
        if (m_loadCallback)
            m_loadCallback(m_pendingMap, m_pendingSpawn);

        m_timer = 0.f;
        m_state = State::FadingIn;
    }
    else if (m_state == State::FadingIn && m_timer >= m_fadeDuration)
    {
        m_state = State::Idle;
    }

    return true;   // transition still active
}

void TransitionManager::render(sf::RenderWindow& window)
{
    if (m_state == State::Idle)
        return;

    float alpha = 0.f;
    if (m_state == State::FadingOut)
        alpha = std::min(m_timer / m_fadeDuration, 1.f);
    else if (m_state == State::FadingIn)
        alpha = 1.f - std::min(m_timer / m_fadeDuration, 1.f);

    sf::Vector2f size(static_cast<float>(window.getSize().x),
                      static_cast<float>(window.getSize().y));
    m_overlay.setSize(size);
    m_overlay.setPosition(0.f, 0.f);
    m_overlay.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha * 255.f)));

    // Draw in default (screen-space) view so it covers the whole window.
    sf::View prev = window.getView();
    sf::View uiView(sf::FloatRect(0.f, 0.f,
                                   static_cast<float>(window.getSize().x),
                                   static_cast<float>(window.getSize().y)));
    window.setView(uiView);
    window.draw(m_overlay);
    window.setView(prev);
}
