#pragma once

#include <string>
#include <functional>

class Renderer;
class Map;

// Manages fade-to-black room transitions.
// Call update() every frame; when a transition is active it drives the fade
// and invokes the provided load callback at the mid-point.
class TransitionManager
{
public:
    // Signature of the callback that loads a new map and repositions the player.
    // Parameters: targetMapFile, targetSpawnName.
    using LoadCallback = std::function<void(const std::string&, const std::string&)>;

    void setLoadCallback(LoadCallback cb) { m_loadCallback = std::move(cb); }

    // Begin a transition to the given map / spawn.  Ignored if already active.
    void startTransition(const std::string& targetMap,
                         const std::string& targetSpawn);

    // Advance the transition timer.  Returns true while a transition is active
    // (the caller should skip normal gameplay updates).
    bool update(float dt);

    // Draw the fade overlay in screen space.
    void render(Renderer& renderer);

    bool isActive() const { return m_state != State::Idle; }

private:
    enum class State { Idle, FadingOut, FadingIn };

    State       m_state      = State::Idle;
    float       m_timer      = 0.f;
    float       m_fadeDuration = 0.4f;   // seconds per half (out or in)
    std::string m_pendingMap;
    std::string m_pendingSpawn;

    LoadCallback       m_loadCallback;
};
