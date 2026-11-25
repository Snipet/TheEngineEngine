// shell/input/g29/G29Joystick.cpp: G29Joystick.h.
#include "input/g29/G29Joystick.h"

namespace tee::input::g29
{
    bool JoystickTable::mapped() const noexcept
    {
        for (const int a : axis)
            if (a >= 0)
                return true;
        return !buttons.empty() || !gears.empty() || hat >= 0;
    }

    const JoystickTable* joystickTable(Kind) noexcept
    {
        // None yet, by design: each mode's table comes from the owner's --probe-input on Windows, not from a guess.
        return nullptr;
    }

    State joystickState(const JoystickTable& table, const Seen& seen) noexcept
    {
        State s;
        for (int i = 0; i < kAxes; ++i)
        {
            const int index = table.axis[static_cast<std::size_t>(i)];
            if (index < 0 || index >= static_cast<int>(seen.axes.size()))
                continue;
            s.v14[static_cast<std::size_t>(i)] =
                Map::axisValue(static_cast<Axis>(i), table.restAtMax[static_cast<std::size_t>(i)],
                               seen.axes[static_cast<std::size_t>(index)], -32768, 32767);
        }
        const auto down = [&seen](int b) { return b >= 0 && b < static_cast<int>(seen.buttons.size()) && seen.buttons[static_cast<std::size_t>(b)]; };
        for (const auto& [button, bit] : table.buttons)
            if (bit >= 0 && bit < 32 && down(button))
                s.buttons |= 1u << bit;
        for (const auto& [button, gear] : table.gears)
            if (down(button))
                s.gear = gear;
        if (table.hat >= 0 && table.hat < static_cast<int>(seen.hats.size()))
        {
            // the d-pad's four bits of the mask, in kBits' order: up, right, down, left (SDL's hat bits are the same)
            int first = -1;
            for (int b = 0; b < static_cast<int>(std::size(kBits)); ++b)
                if (kBits[b].pad == Pad::up)
                    first = b;
            const std::uint8_t h = seen.hats[static_cast<std::size_t>(table.hat)];
            for (int k = 0; k < 4 && first >= 0; ++k)
                if ((h >> k) & 1u)
                    s.buttons |= 1u << (first + k);
        }
        return s;
    }

    std::string JoystickReader::statusFor(Kind kind, bool mapped)
    {
        switch (kind)
        {
            case Kind::ps4: return "set the switch on the wheel to PS3";
            case Kind::g29:
                return mapped ? "G29 read (Logitech G HUB): range, spring and lights are G HUB's"
                              : "G29 seen (its own mode, as with Logitech G HUB): its controls are not yet mapped on "
                                "this system: run TheEngineEngine --probe-input and paste what it prints";
            case Kind::compat:
                return mapped ? "G29 read as a joystick: pedals and buttons; the clutch, the shifter, the range, the "
                                "spring and the lights need Logitech G HUB"
                              : "G29 seen (compatibility mode): its controls are not yet mapped on this system: run "
                                "TheEngineEngine --probe-input and paste what it prints";
            default: return {};
        }
    }

    bool JoystickReader::update(const Seen* seen)
    {
        const Kind kind = seen != nullptr ? kindOf(seen->vendor, seen->product, seen->version) : Kind::none;
        const JoystickTable* table = kind == Kind::none || kind == Kind::ps4 ? nullptr : tableFor_(kind);
        const bool mapped = table != nullptr && table->mapped();
        const std::string status = statusFor(kind, mapped);
        const bool changed = kind != kind_ || mapped != reading_ || status != status_;
        kind_ = kind;
        reading_ = mapped;
        status_ = status;
        state_ = mapped ? joystickState(*table, *seen) : State{};
        return changed;
    }
}
