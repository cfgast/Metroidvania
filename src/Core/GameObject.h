#pragma once

#include <memory>
#include <vector>
#include <type_traits>

#include <SFML/System/Vector2.hpp>

#include "Component.h"

class Renderer;

class GameObject
{
public:
    sf::Vector2f position;

    template<typename T, typename... Args>
    T* addComponent(Args&&... args)
    {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        ptr->setOwner(this);
        m_components.push_back(std::move(component));
        return ptr;
    }

    template<typename T>
    T* getComponent()
    {
        for (auto& component : m_components)
        {
            if (T* result = dynamic_cast<T*>(component.get()))
                return result;
        }
        return nullptr;
    }

    void update(float dt);
    void render(Renderer& renderer);

private:
    std::vector<std::unique_ptr<Component>> m_components;
};
