// shell/input/g29/G29Input.cpp: G29Input.h.
#include "input/g29/G29Input.h"

#include "input/Driver.h"

namespace tee::input::g29
{
    void Input::apply(const State& now, bool reading, tee::input::Driver& driver)
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
        const auto edge = [&](int bit, Action action) {
            if (now.button(bit) != was_.button(bit))
                driver.set(Source::wheel, action, now.button(bit));
        };
        edge(kBitRightPaddle, Action::shiftUp);
        edge(kBitLeftPaddle, Action::shiftDown);
        edge(kBitPs, Action::start);
        // the shifter: a change of gate (and the first frame read, a gate already in: what the lever says from now on)
        if (now.gear != was_.gear || (first && now.gear != 0))
            driver.requestGear(now.gear);
        was_ = now;
    }
}
