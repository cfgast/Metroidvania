#pragma once

#include "../Core/Component.h"

struct InputState
{
    bool moveLeft  = false;
    bool moveRight = false;
    bool jump      = false;
};

// Reads keyboard state each frame and exposes it as an InputState.
// For remote/AI players, inject an InputState directly via setInputState().
class InputComponent : public Component
{
public:
    void update(float dt) override;

    const InputState& getInputState() const { return m_state; }

    // Allow external code (e.g. networking) to override keyboard input.
    void setInputState(const InputState& state) { m_state = state; m_useExternal = true; }
    void clearExternalInput() { m_useExternal = false; }

private:
    InputState m_state;
    bool m_useExternal = false;
};
