// tests/shell/G29JoystickTest.cpp: the G29 read as a plain joystick (input/g29/G29Joystick.h, Windows's reader) with
// made-up frames of what SDL would show and a made-up table. No device is opened.
//   TABLES   none yet for any mode: nothing is driven from an axis nobody has probed
//   READER   none seen; at PS4; seen in either mode with no table (said, not read); a table's axes, buttons, gears and
//            d-pad into the state; unplugged: at rest, no longer present
#include "input/g29/G29Joystick.h"

#include <catch2/catch_test_macros.hpp>

using namespace tee::input::g29;

namespace
{
    // A made-up table (not the G29's: nobody has probed it on Windows yet): axis 0 the steering, 1 the throttle,
    // 2 the brake, 3 the clutch; button 4 the right paddle, 5 the left; buttons 12 and 18 the first gear and reverse;
    // hat 0 the d-pad.
    const JoystickTable* madeUp(Kind) noexcept
    {
        static const JoystickTable t = [] {
            JoystickTable x;
            x.axis = { { 0, 1, 2, 3 } };
            x.buttons = { { 4, kBitRightPaddle }, { 5, kBitLeftPaddle } };
            x.gears = { { 12, 1 }, { 18, -1 } };
            x.hat = 0;
            return x;
        }();
        return &t;
    }

    Seen at(int product, int version = 0)
    {
        Seen s;
        s.vendor = kVendor;
        s.product = product;
        s.version = version;
        s.axes = { 0, 32767, 32767, 32767 };                        // centred; the pedals at rest (their top)
        s.buttons.assign(24, false);
        s.hats = { 0 };
        return s;
    }
}

TEST_CASE("g29 joystick: no mode has a table until the owner's probe on Windows gives one", "[shell][input][g29]")
{
    for (const Kind k : { Kind::none, Kind::g29, Kind::compat, Kind::ps4 })
        CHECK(joystickTable(k) == nullptr);
    CHECK_FALSE(JoystickTable{}.mapped());
}

TEST_CASE("g29 joystick: seen and said in each mode with no table; read through a table; unplugged",
          "[shell][input][g29]")
{
    // none seen: nothing said
    JoystickReader none;
    CHECK_FALSE(none.update(nullptr));
    CHECK_FALSE(none.present());
    CHECK(none.status().empty());

    // the real (empty) tables: a G29 in its own mode is seen, not read; the line says to run the probe
    JoystickReader reader;
    Seen own = at(kG29);
    own.axes[1] = -32768;                                           // the throttle down: drives nothing
    CHECK(reader.update(&own));
    CHECK(reader.present());
    CHECK_FALSE(reader.reading());
    CHECK(reader.state() == State{});
    CHECK(reader.status().find("not yet mapped") != std::string::npos);
    CHECK(reader.status().find("--probe-input") != std::string::npos);
    CHECK_FALSE(reader.update(&own));                               // the same frame again: no change

    // compatibility mode (a G29's bcdDevice, as hid-lg4ff tells it): seen, its own line
    const Seen compat = at(kCompat, 0x1350);
    CHECK(reader.update(&compat));
    CHECK(reader.kind() == Kind::compat);
    CHECK(reader.status().find("compatibility mode") != std::string::npos);
    // another Logitech wheel in compatibility mode is not a G29: left alone
    const Seen other = at(kCompat, 0x1200);
    reader.update(&other);
    CHECK_FALSE(reader.present());

    // at PS4: the switch
    const Seen ps4 = at(kG29Ps4);
    reader.update(&ps4);
    CHECK(reader.status() == "set the switch on the wheel to PS3");
    CHECK_FALSE(reader.reading());

    // a table: the axes, the buttons, the gears and the d-pad into the state
    JoystickReader mapped(madeUp);
    Seen s = at(kG29);
    CHECK(mapped.update(&s));
    CHECK(mapped.reading());
    CHECK(mapped.status().find("G HUB") != std::string::npos);
    CHECK(mapped.state() == State{});                               // at rest: centred, released, neutral
    s.axes = { 32767, -32768, 0, 32767 };                           // full right, throttle down, brake half way
    s.buttons[4] = true;
    s.buttons[18] = true;
    s.hats[0] = 1 | 2;                                              // up and right
    mapped.update(&s);
    const State& st = mapped.state();
    CHECK(st.steering() > 0.99f);
    CHECK(st.pedal(Axis::throttle) > 0.99f);
    CHECK(st.pedal(Axis::brake) > 0.4f);
    CHECK(st.pedal(Axis::brake) < 0.6f);
    CHECK(st.pedal(Axis::clutch) == 0.0f);
    CHECK(st.button(kBitRightPaddle));
    CHECK_FALSE(st.button(kBitLeftPaddle));
    CHECK(st.gear == -1);
    int up = -1;
    for (int b = 0; b < static_cast<int>(std::size(kBits)); ++b)
        if (kBits[b].pad == Pad::up)
            up = b;
    CHECK(st.button(up));
    CHECK(st.button(up + 1));                                       // right
    CHECK_FALSE(st.button(up + 2));
    // compatibility mode through a table: its own line (what G HUB would add)
    const Seen c = at(kCompat, 0x1350);
    mapped.update(&c);
    CHECK(mapped.status().find("need Logitech G HUB") != std::string::npos);

    // an index the joystick does not have: at rest, not a crash
    Seen few = at(kG29);
    few.axes.resize(1);
    few.buttons.clear();
    few.hats.clear();
    mapped.update(&few);
    CHECK(mapped.state().pedal(Axis::throttle) == 0.0f);
    CHECK(mapped.state().buttons == 0u);

    // unplugged: at rest, no longer present
    CHECK(mapped.update(nullptr));
    CHECK_FALSE(mapped.present());
    CHECK_FALSE(mapped.reading());
    CHECK(mapped.state() == State{});
    CHECK(mapped.status().empty());
}
