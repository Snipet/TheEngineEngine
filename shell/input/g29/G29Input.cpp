// shell/input/g29/G29Input.cpp: G29Input.h.
#include "input/g29/G29Input.h"

#include "input/Driver.h"

#include <algorithm>
#include <iterator>

namespace tee::input::g29
{
    Buttons defaultButtons()
    {
        return { { kBitRightPaddle, Action::shiftUp }, { kBitLeftPaddle, Action::shiftDown }, { kBitPs, Action::start } };
    }

    const std::vector<Action>& bindableActions()
    {
        static const std::vector<Action> actions{ Action::start,    Action::shiftUp, Action::shiftDown, Action::neutral,
                                                  Action::handbrake, Action::splitter, Action::range,   Action::engineBrake };
        return actions;
    }

    std::string saveButtons(const Buttons& buttons)
    {
        std::string s;
        for (const Binding& b : buttons)
            if (b.bit >= 0 && b.action != Action::none)
                s += (s.empty() ? "" : ",") + std::to_string(b.bit) + "=" + std::string(actionName(b.action));
        return s;
    }

    Buttons loadButtons(const std::string& text, std::string& notes)
    {
        if (text.empty())
            return defaultButtons();
        Buttons out;
        std::size_t at = 0;
        while (at <= text.size())
        {
            const std::size_t comma = std::min(text.find(',', at), text.size());
            const std::string pair = text.substr(at, comma - at);
            at = comma + 1;
            if (pair.empty())
                continue;
            const std::size_t eq = pair.find('=');
            int bit = -1;
            if (eq != std::string::npos && eq > 0)
                try
                {
                    bit = std::stoi(pair.substr(0, eq));
                }
                catch (...)
                {
                    bit = -1;
                }
            const Action action = eq == std::string::npos ? Action::none : actionFromName(pair.substr(eq + 1));
            const bool known = std::find(bindableActions().begin(), bindableActions().end(), action) != bindableActions().end();
            if (bit < 0 || bit >= static_cast<int>(std::size(kBits)) || !known)
            {
                notes += (notes.empty() ? "" : "; ") + std::string("the wheel's binding \"") + pair + "\" was not read";
                continue;
            }
            out.push_back({ bit, action });
        }
        return out;
    }

    void ButtonLearn::begin(Action action, const State& now)
    {
        active_ = true;
        action_ = action;
        was_ = now.buttons;
        left_ = kSeconds;
    }

    bool ButtonLearn::watch(const State& now, double dt, Buttons& buttons)
    {
        if (!active_)
            return false;
        left_ -= dt;
        // a button that was down when learning began counts once it has been let go
        was_ &= now.buttons;
        const std::uint32_t pressed = now.buttons & ~was_;
        if (pressed == 0)
        {
            if (left_ <= 0.0)
                active_ = false;
            return false;
        }
        int bit = 0;
        while (((pressed >> bit) & 1u) == 0)
            ++bit;
        buttons.erase(std::remove_if(buttons.begin(), buttons.end(),
                                     [&](const Binding& b) { return b.bit == bit || b.action == action_; }),
                      buttons.end());
        buttons.push_back({ bit, action_ });
        active_ = false;
        return true;
    }

    void Input::setButtons(Buttons buttons)
    {
        buttons_ = std::move(buttons);
        releaseButtons_ = true;
    }

    void Input::apply(const State& now, bool reading, tee::input::Driver& driver, bool buttonsLive)
    {
        if (!reading)
        {
            if (active_)
            {
                driver.release(Source::wheel);
                was_ = State{};
                active_ = false;
            }
            return;
        }
        const bool first = !active_;
        active_ = true;
        driver.setWheel(now.pedal(Axis::throttle), now.pedal(Axis::brake), now.pedal(Axis::clutch));
        // the bindings changed, or a learn is listening: what the wheel's buttons held is let go, and a button held now
        // counts once it is pressed again
        if (releaseButtons_ || !buttonsLive)
        {
            for (const Action a : bindableActions())
                driver.set(Source::wheel, a, false);
            releaseButtons_ = false;
        }
        else
            for (const Binding& b : buttons_)
                if (now.button(b.bit) != was_.button(b.bit))
                    driver.set(Source::wheel, b.action, now.button(b.bit));
        // the shifter: a change of gate (and the first frame read, a gate already in: what the lever says from now on)
        if (now.gear != was_.gear || (first && now.gear != 0))
            driver.requestGear(now.gear);
        was_ = now;
    }
}
