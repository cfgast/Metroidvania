#pragma once

class Renderer;
class GameObject;

class Component
{
public:
    virtual ~Component() = default;

    virtual void update(float dt) {}
    virtual void render(Renderer& renderer) {}

    void setOwner(GameObject* owner) { m_owner = owner; }
    GameObject* getOwner() const { return m_owner; }

private:
    GameObject* m_owner = nullptr;
};
