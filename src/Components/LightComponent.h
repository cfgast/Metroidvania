#pragma once

#include "../Core/Component.h"
#include "../Rendering/Light.h"

class LightComponent : public Component
{
public:
    void setLight(const Light& light) { m_light = light; }

    // Returns m_light with position updated to owner's world position.
    Light getLight() const;

    void update(float dt) override;

private:
    Light m_light;
};
