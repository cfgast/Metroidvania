#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

enum class Ability : uint8_t
{
    DoubleJump,
    WallSlide,
    Dash
};

inline Ability abilityFromString(const std::string& s)
{
    if (s == "DoubleJump") return Ability::DoubleJump;
    if (s == "WallSlide")  return Ability::WallSlide;
    if (s == "Dash")       return Ability::Dash;
    throw std::runtime_error("Unknown ability: " + s);
}

inline std::string abilityToString(Ability a)
{
    switch (a)
    {
        case Ability::DoubleJump: return "DoubleJump";
        case Ability::WallSlide:  return "WallSlide";
        case Ability::Dash:       return "Dash";
    }
    return "Unknown";
}
