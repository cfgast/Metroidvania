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
    moveLeftKey    = KeyCode::A;
    moveLeftAlt    = KeyCode::Left;
    moveRightKey   = KeyCode::D;
    moveRightAlt   = KeyCode::Right;
    jumpKey        = KeyCode::Space;
    dashKey        = KeyCode::LShift;
    attackKey      = KeyCode::X;
    controllerJumpButton   = 0;
    controllerDashButton   = 5;
    controllerAttackButton = 2;
}

void InputBindings::save() const
{
    nlohmann::json j;
    j["keyboard"]["moveLeft"]     = keyToString(moveLeftKey);
    j["keyboard"]["moveLeftAlt"]  = keyToString(moveLeftAlt);
    j["keyboard"]["moveRight"]    = keyToString(moveRightKey);
    j["keyboard"]["moveRightAlt"] = keyToString(moveRightAlt);
    j["keyboard"]["jump"]         = keyToString(jumpKey);
    j["keyboard"]["dash"]         = keyToString(dashKey);
    j["keyboard"]["attack"]       = keyToString(attackKey);
    j["controller"]["jumpButton"]   = controllerJumpButton;
    j["controller"]["dashButton"]   = controllerDashButton;
    j["controller"]["attackButton"] = controllerAttackButton;

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
            if (kb.contains("dash"))         dashKey        = stringToKey(kb["dash"]);
            if (kb.contains("attack"))       attackKey      = stringToKey(kb["attack"]);
        }
        if (j.contains("controller"))
        {
            auto& ct = j["controller"];
            if (ct.contains("jumpButton"))   controllerJumpButton   = ct["jumpButton"].get<unsigned int>();
            if (ct.contains("dashButton"))   controllerDashButton   = ct["dashButton"].get<unsigned int>();
            if (ct.contains("attackButton")) controllerAttackButton = ct["attackButton"].get<unsigned int>();
        }

        std::cout << "Controls loaded from " << SAVE_PATH << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error loading controls: " << e.what() << '\n';
    }
}

// ---- Key <-> String mapping ------------------------------------------------

struct KeyEntry { KeyCode key; const char* name; };

static const KeyEntry KEY_TABLE[] = {
    { KeyCode::A,         "A" },
    { KeyCode::B,         "B" },
    { KeyCode::C,         "C" },
    { KeyCode::D,         "D" },
    { KeyCode::E,         "E" },
    { KeyCode::F,         "F" },
    { KeyCode::G,         "G" },
    { KeyCode::H,         "H" },
    { KeyCode::I,         "I" },
    { KeyCode::J,         "J" },
    { KeyCode::K,         "K" },
    { KeyCode::L,         "L" },
    { KeyCode::M,         "M" },
    { KeyCode::N,         "N" },
    { KeyCode::O,         "O" },
    { KeyCode::P,         "P" },
    { KeyCode::Q,         "Q" },
    { KeyCode::R,         "R" },
    { KeyCode::S,         "S" },
    { KeyCode::T,         "T" },
    { KeyCode::U,         "U" },
    { KeyCode::V,         "V" },
    { KeyCode::W,         "W" },
    { KeyCode::X,         "X" },
    { KeyCode::Y,         "Y" },
    { KeyCode::Z,         "Z" },
    { KeyCode::Num0,      "0" },
    { KeyCode::Num1,      "1" },
    { KeyCode::Num2,      "2" },
    { KeyCode::Num3,      "3" },
    { KeyCode::Num4,      "4" },
    { KeyCode::Num5,      "5" },
    { KeyCode::Num6,      "6" },
    { KeyCode::Num7,      "7" },
    { KeyCode::Num8,      "8" },
    { KeyCode::Num9,      "9" },
    { KeyCode::Escape,    "Escape" },
    { KeyCode::LControl,  "LCtrl" },
    { KeyCode::LShift,    "LShift" },
    { KeyCode::LAlt,      "LAlt" },
    { KeyCode::RControl,  "RCtrl" },
    { KeyCode::RShift,    "RShift" },
    { KeyCode::RAlt,      "RAlt" },
    { KeyCode::Space,     "Space" },
    { KeyCode::Enter,     "Enter" },
    { KeyCode::Backspace, "Backspace" },
    { KeyCode::Tab,       "Tab" },
    { KeyCode::Left,      "Left" },
    { KeyCode::Right,     "Right" },
    { KeyCode::Up,        "Up" },
    { KeyCode::Down,      "Down" },
    { KeyCode::LBracket,  "[" },
    { KeyCode::RBracket,  "]" },
    { KeyCode::Semicolon, ";" },
    { KeyCode::Comma,     "," },
    { KeyCode::Period,    "." },
    { KeyCode::Quote,     "'" },
    { KeyCode::Slash,     "/" },
    { KeyCode::Backslash, "\\" },
    { KeyCode::Tilde,     "~" },
    { KeyCode::Equal,     "=" },
    { KeyCode::Hyphen,    "-" },
    { KeyCode::Insert,    "Insert" },
    { KeyCode::Delete,    "Delete" },
    { KeyCode::Home,      "Home" },
    { KeyCode::End,       "End" },
    { KeyCode::PageUp,    "PageUp" },
    { KeyCode::PageDown,  "PageDown" },
    { KeyCode::F1,        "F1" },
    { KeyCode::F2,        "F2" },
    { KeyCode::F3,        "F3" },
    { KeyCode::F4,        "F4" },
    { KeyCode::F5,        "F5" },
    { KeyCode::F6,        "F6" },
    { KeyCode::F7,        "F7" },
    { KeyCode::F8,        "F8" },
    { KeyCode::F9,        "F9" },
    { KeyCode::F10,       "F10" },
    { KeyCode::F11,       "F11" },
    { KeyCode::F12,       "F12" },
    { KeyCode::Numpad0,   "Num0" },
    { KeyCode::Numpad1,   "Num1" },
    { KeyCode::Numpad2,   "Num2" },
    { KeyCode::Numpad3,   "Num3" },
    { KeyCode::Numpad4,   "Num4" },
    { KeyCode::Numpad5,   "Num5" },
    { KeyCode::Numpad6,   "Num6" },
    { KeyCode::Numpad7,   "Num7" },
    { KeyCode::Numpad8,   "Num8" },
    { KeyCode::Numpad9,   "Num9" },
};

static const size_t KEY_TABLE_SIZE = sizeof(KEY_TABLE) / sizeof(KEY_TABLE[0]);

std::string InputBindings::keyToString(KeyCode key)
{
    for (size_t i = 0; i < KEY_TABLE_SIZE; ++i)
    {
        if (KEY_TABLE[i].key == key)
            return KEY_TABLE[i].name;
    }
    return "Key" + std::to_string(static_cast<int>(key));
}

KeyCode InputBindings::stringToKey(const std::string& name)
{
    for (size_t i = 0; i < KEY_TABLE_SIZE; ++i)
    {
        if (name == KEY_TABLE[i].name)
            return KEY_TABLE[i].key;
    }
    // Try parsing "KeyNN" format
    if (name.rfind("Key", 0) == 0)
    {
        try { return static_cast<KeyCode>(std::stoi(name.substr(3))); }
        catch (...) {}
    }
    return KeyCode::Unknown;
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
