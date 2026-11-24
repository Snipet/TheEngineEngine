// shell/input/Keys.cpp: the default bindings and the names (Keys.h).
#include "input/Keys.h"

#include <array>

namespace tee::input
{
    namespace
    {
        struct KeyNameRow
        {
            int key;
            std::string_view name;
        };

        constexpr std::array<KeyNameRow, 30> kKeyNames{ {
            { key::kJ, "j" },           { key::kX, "x" },           { key::kZ, "z" },
            { key::kA, "a" },           { key::kD, "d" },           { key::kE, "e" },
            { key::kN, "n" },           { key::kQ, "q" },           { key::kR, "r" },
            { key::kS, "s" },           { key::kW, "w" },           { key::k1, "1" },
            { key::k2, "2" },           { key::k3, "3" },           { key::k4, "4" },
            { key::k5, "5" },           { key::k6, "6" },           { key::kReturn, "return" },
            { key::kEscape, "escape" }, { key::kBackspace, "backspace" }, { key::kTab, "tab" },
            { key::kSpace, "space" },   { key::kPageUp, "pageup" }, { key::kPageDown, "pagedown" },
            { key::kRight, "right" },   { key::kLeft, "left" },     { key::kDown, "down" },
            { key::kUp, "up" },         { key::kLeftShift, "lshift" }, { key::kKeypadEnter, "enter" },
        } };

        constexpr std::array<std::string_view, static_cast<std::size_t>(Action::count)> kActionNames{
            "none",     "throttle", "brake", "clutch", "handbrake", "start", "shiftup", "shiftdown",
            "neutral",  "gear1",    "gear2", "gear3",  "gear4",     "gear5", "gear6",   "reverse",
            "splitter", "range",    "enginebrake",
        };
    }

    std::vector<Binding> defaultBindings()
    {
        return {
            { key::kUp, Action::throttle },       { key::kW, Action::throttle },
            { key::kDown, Action::brake },        { key::kS, Action::brake },
            { key::kLeftShift, Action::clutch },  { key::kSpace, Action::handbrake },
            { key::kReturn, Action::start },      { key::kKeypadEnter, Action::start },
            { key::kE, Action::shiftUp },         { key::kPageUp, Action::shiftUp },
            { key::kQ, Action::shiftDown },       { key::kPageDown, Action::shiftDown },
            { key::kN, Action::neutral },
            { key::k1, Action::gear1 },           { key::k2, Action::gear2 },
            { key::k3, Action::gear3 },           { key::k4, Action::gear4 },
            { key::k5, Action::gear5 },           { key::k6, Action::gear6 },
            { key::kZ, Action::splitter },        { key::kX, Action::range },
            { key::kJ, Action::engineBrake },
        };
    }

    Action actionOf(const std::vector<Binding>& bindings, int key) noexcept
    {
        for (const Binding& b : bindings)
            if (b.key == key)
                return b.action;
        return Action::none;
    }

    std::string_view keyName(int key) noexcept
    {
        for (const KeyNameRow& row : kKeyNames)
            if (row.key == key)
                return row.name;
        return {};
    }

    int keyFromName(std::string_view name) noexcept
    {
        for (const KeyNameRow& row : kKeyNames)
            if (row.name == name)
                return row.key;
        return 0;
    }

    std::string_view actionName(Action a) noexcept
    {
        const auto i = static_cast<std::size_t>(a);
        return i < kActionNames.size() ? kActionNames[i] : std::string_view{};
    }

    Action actionFromName(std::string_view name) noexcept
    {
        for (std::size_t i = 1; i < kActionNames.size(); ++i)
            if (kActionNames[i] == name)
                return static_cast<Action>(i);
        return Action::none;
    }
}
