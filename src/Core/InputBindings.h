#pragma once

#include <string>

#include <SFML/Window/Keyboard.hpp>

// Singleton that holds user-configurable keyboard and controller bindings.
// Persists to saves/controls.json.
class InputBindings
{
public:
    static InputBindings& instance();

    // Keyboard bindings
    sf::Keyboard::Key moveLeftKey    = sf::Keyboard::A;
    sf::Keyboard::Key moveLeftAlt    = sf::Keyboard::Left;
    sf::Keyboard::Key moveRightKey   = sf::Keyboard::D;
    sf::Keyboard::Key moveRightAlt   = sf::Keyboard::Right;
    sf::Keyboard::Key jumpKey        = sf::Keyboard::Space;

    // Controller bindings
    unsigned int controllerJumpButton = 0;

    void resetDefaults();
    void save() const;
    void load();

    // Human-readable names for display and serialization
    static std::string keyToString(sf::Keyboard::Key key);
    static sf::Keyboard::Key stringToKey(const std::string& name);
    static std::string buttonToString(unsigned int button);

private:
    InputBindings() = default;
};
