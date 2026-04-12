#include "LightComponent.h"

#include "../Core/GameObject.h"

Light LightComponent::getLight() const
{
    Light l = m_light;
    if (getOwner())
        l.position = getOwner()->position;
    return l;
}

void LightComponent::update(float /*dt*/)
{
}
