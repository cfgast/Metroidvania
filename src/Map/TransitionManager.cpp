#include "TransitionManager.h"

#include <algorithm>

#include "../Rendering/Renderer.h"

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

void TransitionManager::render(Renderer& renderer)
{
    if (m_state == State::Idle)
        return;

    float alpha = 0.f;
    if (m_state == State::FadingOut)
        alpha = std::min(m_timer / m_fadeDuration, 1.f);
    else if (m_state == State::FadingIn)
        alpha = 1.f - std::min(m_timer / m_fadeDuration, 1.f);

    float winW, winH;
    renderer.getWindowSize(winW, winH);

    // Draw in screen space so the overlay covers the whole window.
    renderer.resetView();
    renderer.drawRect(0.f, 0.f, winW, winH, 0.f, 0.f, 0.f, alpha);
}
