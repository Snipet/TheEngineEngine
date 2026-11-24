// shell/input/WheelMap.h: a wheel's axes and buttons as the driver's controls, by a table a device (Wheel.h is the
// seam a real device comes through; nothing opens one yet). Plain C++, no SDL: a device's driver fills a WheelState
// a frame, and applyWheel() puts it into the Driver as the gamepad's controls (Source::gamepad), so nothing
// downstream changes when a wheel arrives. Tested with a made-up device (tests/shell/WheelTest.cpp); no real wheel has
// been near it.
//
//   A table   for each pedal and the steering, which axis and how (inverted; a pedal's rest and floor, since pedals
//             report from -1 up, from 1 down or from 0 up depending on the device; a dead zone at rest); for each
//             button the action it is (a paddle a shift, a button the start key); an H-pattern shifter's buttons as
//             gears. One table a device, named by the device's name (the settings file will keep them).
//   Learn     Learn::begin(role) then Learn::watch(state) each frame: the first axis that moves well away from where it
//             was when learning began (a pedal pressed, the wheel turned), or the first button pressed, is bound to the
//             role. So a person maps a device they have by doing what the role says.
#pragma once

#include "input/Keys.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace tee::input
{
    class Driver;

    // What a device reports, once a frame: its axes (-1..1, as SDL gives them) and buttons.
    struct WheelState
    {
        std::vector<float> axes;
        std::vector<bool> buttons;
    };

    enum class AxisRole : std::uint8_t { steer, throttle, brake, clutch, count };

    struct AxisMap
    {
        int axis = -1;              // the device's axis; -1: none
        float rest = -1.0f;         // the value at rest (a pedal let go; the wheel centred: 0)
        float floor = 1.0f;         // the value at the end of its travel (a pedal on the floor; the wheel full right)
        float deadZone = 0.03f;     // of the travel, near rest, read as rest
    };

    struct ButtonMap
    {
        int button = -1;
        Action action = Action::none;   // shiftUp, shiftDown, start, neutral, gear1..gear6, reverse, handbrake
    };

    struct WheelMap
    {
        std::string device;             // the device's name, as it reports it
        std::array<AxisMap, static_cast<std::size_t>(AxisRole::count)> axes{};
        std::vector<ButtonMap> buttons;

        // A table for a device that reports the usual way (a Logitech G29's under SDL: the wheel axis 0, the pedals
        // 1 to 3 from 1 at rest down to -1, paddles 4 and 5): a starting point a learn corrects.
        static WheelMap typical(std::string device);
    };

    // A pedal's or the steering's value from its axis: 0..1 for a pedal, -1..1 for the steering. 0 when unmapped or the
    // axis is not there.
    float axisValue(const AxisMap&, AxisRole, const WheelState&) noexcept;

    // The state into the driver: the pedals and steering as analogue (with the clutch), the buttons as actions held
    // while down. `previous` is the state of the frame before (a button's press and release are its changes); it may be
    // empty.
    void applyWheel(const WheelMap&, const WheelState& now, const WheelState& previous, Driver&);

    class Learn
    {
    public:
        // What is being learned: an axis role, or a button's action.
        void beginAxis(AxisRole role, const WheelState& now);
        void beginButton(Action action, const WheelState& now);
        bool active() const noexcept { return active_; }
        void cancel() noexcept { active_ = false; }

        // Each frame while active: true when something was bound into `map` (and learning ends).
        bool watch(const WheelState& now, WheelMap& map);

    private:
        bool active_ = false, axis_ = false;
        AxisRole role_ = AxisRole::steer;
        Action action_ = Action::none;
        WheelState start_;
    };
}
