#include "InputBindings.h"

#include <fstream>
#include <iostream>
#include <unordered_map>

#include <nlohmann/json.hpp>

static const char* SAVE_PATH = "saves/controls.json";

InputBindings& InputBindings::instance()
{
    static InputBindings inst;
    return inst;
}

void InputBindings::resetDefaults()
{
    moveLeftKey    = sf::Keyboard::A;
    moveLeftAlt    = sf::Keyboard::Left;
    moveRightKey   = sf::Keyboard::D;
    moveRightAlt   = sf::Keyboard::Right;
    jumpKey        = sf::Keyboard::Space;
    controllerJumpButton = 0;
}

void InputBindings::save() const
{
    nlohmann::json j;
    j["keyboard"]["moveLeft"]     = keyToString(moveLeftKey);
    j["keyboard"]["moveLeftAlt"]  = keyToString(moveLeftAlt);
    j["keyboard"]["moveRight"]    = keyToString(moveRightKey);
    j["keyboard"]["moveRightAlt"] = keyToString(moveRightAlt);
    j["keyboard"]["jump"]         = keyToString(jumpKey);
    j["controller"]["jumpButton"] = controllerJumpButton;

    std::ofstream file(SAVE_PATH);
    if (file.is_open())
    {
        file << j.dump(2);
        std::cout << "Controls saved to " << SAVE_PATH << '\n';
    }
    else
    {
        std::cerr << "Failed to save controls to " << SAVE_PATH << '\n';
    }
}

void InputBindings::load()
{
    std::ifstream file(SAVE_PATH);
    if (!file.is_open())
        return; // no saved bindings, keep defaults

    try
    {
        nlohmann::json j = nlohmann::json::parse(file);

        if (j.contains("keyboard"))
        {
            auto& kb = j["keyboard"];
            if (kb.contains("moveLeft"))     moveLeftKey    = stringToKey(kb["moveLeft"]);
            if (kb.contains("moveLeftAlt"))  moveLeftAlt    = stringToKey(kb["moveLeftAlt"]);
            if (kb.contains("moveRight"))    moveRightKey   = stringToKey(kb["moveRight"]);
            if (kb.contains("moveRightAlt")) moveRightAlt   = stringToKey(kb["moveRightAlt"]);
            if (kb.contains("jump"))         jumpKey        = stringToKey(kb["jump"]);
        }
        if (j.contains("controller"))
        {
            auto& ct = j["controller"];
            if (ct.contains("jumpButton")) controllerJumpButton = ct["jumpButton"].get<unsigned int>();
        }

        std::cout << "Controls loaded from " << SAVE_PATH << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error loading controls: " << e.what() << '\n';
    }
}

// ---- Key <-> String mapping ------------------------------------------------

struct KeyEntry { sf::Keyboard::Key key; const char* name; };

static const KeyEntry KEY_TABLE[] = {
    { sf::Keyboard::A,         "A" },
    { sf::Keyboard::B,         "B" },
    { sf::Keyboard::C,         "C" },
    { sf::Keyboard::D,         "D" },
    { sf::Keyboard::E,         "E" },
    { sf::Keyboard::F,         "F" },
    { sf::Keyboard::G,         "G" },
    { sf::Keyboard::H,         "H" },
    { sf::Keyboard::I,         "I" },
    { sf::Keyboard::J,         "J" },
    { sf::Keyboard::K,         "K" },
    { sf::Keyboard::L,         "L" },
    { sf::Keyboard::M,         "M" },
    { sf::Keyboard::N,         "N" },
    { sf::Keyboard::O,         "O" },
    { sf::Keyboard::P,         "P" },
    { sf::Keyboard::Q,         "Q" },
    { sf::Keyboard::R,         "R" },
    { sf::Keyboard::S,         "S" },
    { sf::Keyboard::T,         "T" },
    { sf::Keyboard::U,         "U" },
    { sf::Keyboard::V,         "V" },
    { sf::Keyboard::W,         "W" },
    { sf::Keyboard::X,         "X" },
    { sf::Keyboard::Y,         "Y" },
    { sf::Keyboard::Z,         "Z" },
    { sf::Keyboard::Num0,      "0" },
    { sf::Keyboard::Num1,      "1" },
    { sf::Keyboard::Num2,      "2" },
    { sf::Keyboard::Num3,      "3" },
    { sf::Keyboard::Num4,      "4" },
    { sf::Keyboard::Num5,      "5" },
    { sf::Keyboard::Num6,      "6" },
    { sf::Keyboard::Num7,      "7" },
    { sf::Keyboard::Num8,      "8" },
    { sf::Keyboard::Num9,      "9" },
    { sf::Keyboard::Escape,    "Escape" },
    { sf::Keyboard::LControl,  "LCtrl" },
    { sf::Keyboard::LShift,    "LShift" },
    { sf::Keyboard::LAlt,      "LAlt" },
    { sf::Keyboard::RControl,  "RCtrl" },
    { sf::Keyboard::RShift,    "RShift" },
    { sf::Keyboard::RAlt,      "RAlt" },
    { sf::Keyboard::Space,     "Space" },
    { sf::Keyboard::Enter,     "Enter" },
    { sf::Keyboard::Backspace, "Backspace" },
    { sf::Keyboard::Tab,       "Tab" },
    { sf::Keyboard::Left,      "Left" },
    { sf::Keyboard::Right,     "Right" },
    { sf::Keyboard::Up,        "Up" },
    { sf::Keyboard::Down,      "Down" },
    { sf::Keyboard::LBracket,  "[" },
    { sf::Keyboard::RBracket,  "]" },
    { sf::Keyboard::Semicolon, ";" },
    { sf::Keyboard::Comma,     "," },
    { sf::Keyboard::Period,    "." },
    { sf::Keyboard::Quote,     "'" },
    { sf::Keyboard::Slash,     "/" },
    { sf::Keyboard::Backslash, "\\" },
    { sf::Keyboard::Tilde,     "~" },
    { sf::Keyboard::Equal,     "=" },
    { sf::Keyboard::Hyphen,    "-" },
    { sf::Keyboard::Insert,    "Insert" },
    { sf::Keyboard::Delete,    "Delete" },
    { sf::Keyboard::Home,      "Home" },
    { sf::Keyboard::End,       "End" },
    { sf::Keyboard::PageUp,    "PageUp" },
    { sf::Keyboard::PageDown,  "PageDown" },
    { sf::Keyboard::F1,        "F1" },
    { sf::Keyboard::F2,        "F2" },
    { sf::Keyboard::F3,        "F3" },
    { sf::Keyboard::F4,        "F4" },
    { sf::Keyboard::F5,        "F5" },
    { sf::Keyboard::F6,        "F6" },
    { sf::Keyboard::F7,        "F7" },
    { sf::Keyboard::F8,        "F8" },
    { sf::Keyboard::F9,        "F9" },
    { sf::Keyboard::F10,       "F10" },
    { sf::Keyboard::F11,       "F11" },
    { sf::Keyboard::F12,       "F12" },
    { sf::Keyboard::Numpad0,   "Num0" },
    { sf::Keyboard::Numpad1,   "Num1" },
    { sf::Keyboard::Numpad2,   "Num2" },
    { sf::Keyboard::Numpad3,   "Num3" },
    { sf::Keyboard::Numpad4,   "Num4" },
    { sf::Keyboard::Numpad5,   "Num5" },
    { sf::Keyboard::Numpad6,   "Num6" },
    { sf::Keyboard::Numpad7,   "Num7" },
    { sf::Keyboard::Numpad8,   "Num8" },
    { sf::Keyboard::Numpad9,   "Num9" },
};

static const size_t KEY_TABLE_SIZE = sizeof(KEY_TABLE) / sizeof(KEY_TABLE[0]);

std::string InputBindings::keyToString(sf::Keyboard::Key key)
{
    for (size_t i = 0; i < KEY_TABLE_SIZE; ++i)
    {
        if (KEY_TABLE[i].key == key)
            return KEY_TABLE[i].name;
    }
    return "Key" + std::to_string(static_cast<int>(key));
}

sf::Keyboard::Key InputBindings::stringToKey(const std::string& name)
{
    for (size_t i = 0; i < KEY_TABLE_SIZE; ++i)
    {
        if (name == KEY_TABLE[i].name)
            return KEY_TABLE[i].key;
    }
    // Try parsing "KeyNN" format
    if (name.rfind("Key", 0) == 0)
    {
        try { return static_cast<sf::Keyboard::Key>(std::stoi(name.substr(3))); }
        catch (...) {}
    }
    return sf::Keyboard::Unknown;
}

std::string InputBindings::buttonToString(unsigned int button)
{
    static const char* BUTTON_NAMES[] = {
        "A", "B", "X", "Y", "LB", "RB", "Back", "Start", "L3", "R3"
    };
    if (button < 10)
        return std::string("Button ") + BUTTON_NAMES[button];
    return "Button " + std::to_string(button);
}
