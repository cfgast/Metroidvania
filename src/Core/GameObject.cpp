#include "GameObject.h"

void GameObject::update(float dt)
{
    for (auto& component : m_components)
        component->update(dt);
}

void GameObject::render(Renderer& renderer)
{
    for (auto& component : m_components)
        component->render(renderer);
}
