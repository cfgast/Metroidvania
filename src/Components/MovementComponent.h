#pragma once

#include <glm/vec2.hpp>

#include "../Core/Component.h"

class MovementComponent : public Component
{
public:
    explicit MovementComponent(float speed = 300.f);

    void update(float dt) override;

    float speed;
    glm::vec2 velocity { 0.f, 0.f };
};
