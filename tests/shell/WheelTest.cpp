// tests/shell/WheelTest.cpp: a wheel's mapping (shell/input/WheelMap.h) with a made-up device behind the Wheel seam:
// its pedals, steering and paddles reach the Driver's frame; a learn binds what a person moves. No real wheel was used.
#include "input/Driver.h"
#include "input/Wheel.h"
#include "input/WheelMap.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace tee;
using namespace tee::input;
using Catch::Approx;

namespace
{
    // A made-up wheel: six axes, twelve buttons, its pedals resting at +1 and pressed toward -1 (as a G29's under SDL).
    class MadeUpWheel final : public Wheel
    {
    public:
        explicit MadeUpWheel(WheelMap map) : map_(std::move(map))
        {
            state.axes.assign(6, 0.0f);
            state.axes[1] = state.axes[2] = state.axes[3] = 1.0f;
            state.buttons.assign(12, false);
        }
        std::string name() const override { return "Made-up Racing Wheel"; }
        void poll(Driver& d) override
        {
            applyWheel(map_, state, last_, d);
            last_ = state;
        }
        bool setRange(float degrees) override
        {
            range = degrees;
            return true;
        }
        bool setAutocentre(float) override { return false; }

        WheelState state;
        float range = 900.0f;

    private:
        WheelMap map_;
        WheelState last_;
    };

    const ControlsFrame& step(Driver& d, Wheel& w)
    {
        w.poll(d);
        return d.advance(1.0f / 60.0f, EngineSeen{ true, true, 1 });
    }
}

TEST_CASE("shell: a made-up wheel's pedals, steering and paddles reach the driver's frame", "[shell][wheel]")
{
    DriverSettings settings;
    settings.autoClutch = false;
    Driver d(settings);
    MadeUpWheel wheel(WheelMap::typical("Made-up Racing Wheel"));

    // at rest: nothing
    ControlsFrame f = step(d, wheel);
    CHECK(f.throttle == 0.0f);
    CHECK(f.brake == 0.0f);
    CHECK(f.clutch == 0.0f);
    CHECK(f.steer == 0.0f);

    // a pedal half down (+1 to -1: half is 0), the brake to the floor, the clutch a quarter, the wheel a quarter right
    wheel.state.axes[1] = 0.0f;
    wheel.state.axes[2] = -1.0f;
    wheel.state.axes[3] = 0.5f;
    wheel.state.axes[0] = 0.25f;
    f = step(d, wheel);
    CHECK(f.throttle == Approx(0.5).margin(0.02));
    CHECK(f.brake == Approx(1.0));
    CHECK(f.clutch == Approx(0.25).margin(0.03));
    CHECK(f.steer == Approx(0.25));
    // analogue: no easing, the pedal is where the foot is at once
    wheel.state.axes[1] = 1.0f;
    f = step(d, wheel);
    CHECK(f.throttle == 0.0f);

    // a pedal resting a hair off its stop reads nothing (the dead zone)
    wheel.state.axes[2] = 0.97f;
    f = step(d, wheel);
    CHECK(f.brake == 0.0f);

    // the right paddle: a shift up, counted once a press however long it is held
    const int up = f.shiftUp;
    wheel.state.buttons[4] = true;
    f = step(d, wheel);
    f = step(d, wheel);
    CHECK(f.shiftUp == up + 1);
    wheel.state.buttons[4] = false;
    f = step(d, wheel);
    wheel.state.buttons[4] = true;
    f = step(d, wheel);
    CHECK(f.shiftUp == up + 2);

    // the seam's own calls
    CHECK(wheel.setRange(540.0f));
    CHECK(wheel.range == 540.0f);
    CHECK_FALSE(wheel.setAutocentre(0.5f));
    CHECK(wheels() == 0);       // no real device is opened yet

    // the device unplugged: what it held is let go
    wheel.state.axes[1] = -1.0f;
    f = step(d, wheel);
    CHECK(f.throttle == Approx(1.0));
    d.release(Source::gamepad);
    f = d.advance(1.0f / 60.0f, EngineSeen{ true, true, 1 });
    CHECK(f.throttle == 0.0f);
    CHECK(f.clutch == 0.0f);
}

TEST_CASE("shell: a wheel learned: the axis moved and the button pressed are bound to what was asked", "[shell][wheel]")
{
    WheelMap map;
    map.device = "Made-up Racing Wheel";
    WheelState s;
    s.axes = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    s.buttons.assign(12, false);

    Learn learn;
    // the throttle: a person presses the pedal on axis 2 (this device's order is not the usual one)
    learn.beginAxis(AxisRole::throttle, s);
    CHECK_FALSE(learn.watch(s, map));
    s.axes[2] = 0.8f;                     // a little: not yet
    CHECK_FALSE(learn.watch(s, map));
    s.axes[2] = -0.6f;
    REQUIRE(learn.watch(s, map));
    CHECK_FALSE(learn.active());
    const AxisMap& t = map.axes[static_cast<std::size_t>(AxisRole::throttle)];
    CHECK(t.axis == 2);
    CHECK(t.rest == 1.0f);
    CHECK(t.floor == -1.0f);
    s.axes[2] = -1.0f;
    CHECK(axisValue(t, AxisRole::throttle, s) == Approx(1.0));
    s.axes[2] = 1.0f;
    CHECK(axisValue(t, AxisRole::throttle, s) == 0.0f);

    // the steering: turned right on axis 0
    learn.beginAxis(AxisRole::steer, s);
    s.axes[0] = 0.7f;
    REQUIRE(learn.watch(s, map));
    const AxisMap& st = map.axes[static_cast<std::size_t>(AxisRole::steer)];
    CHECK(st.axis == 0);
    s.axes[0] = -0.5f;
    CHECK(axisValue(st, AxisRole::steer, s) == Approx(-0.5));

    // a button: the start key on button 7; a button held when learning began is not taken
    s.buttons[3] = true;
    learn.beginButton(Action::start, s);
    CHECK_FALSE(learn.watch(s, map));
    s.buttons[7] = true;
    REQUIRE(learn.watch(s, map));
    REQUIRE(map.buttons.size() == 1);
    CHECK(map.buttons[0].button == 7);
    CHECK(map.buttons[0].action == Action::start);

    // learned again on another button: one binding for the action
    s.buttons.assign(12, false);
    learn.beginButton(Action::start, s);
    s.buttons[8] = true;
    REQUIRE(learn.watch(s, map));
    REQUIRE(map.buttons.size() == 1);
    CHECK(map.buttons[0].button == 8);

    // an unmapped role and an axis the device does not have read nothing
    CHECK(axisValue(map.axes[static_cast<std::size_t>(AxisRole::clutch)], AxisRole::clutch, s) == 0.0f);
    AxisMap missing{ 40, 1.0f, -1.0f, 0.0f };
    CHECK(axisValue(missing, AxisRole::brake, s) == 0.0f);
}
