#pragma once

#include <SFML/System/Vector2.hpp>

#include "../Core/Component.h"

class MovementComponent : public Component
{
public:
    explicit MovementComponent(float speed = 300.f);

    void update(float dt) override;

    float speed;
    sf::Vector2f velocity;
};
