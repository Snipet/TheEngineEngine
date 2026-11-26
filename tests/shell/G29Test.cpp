// tests/shell/G29Test.cpp: the Logitech G29 (input/g29/), ported from TestingPlayground's Tools/probes/plugin/wheel.cpp
// (47bac40; SEED.tsv) with the MIDI taken out: made-up HID values and a fake transport whose devices record what they
// are sent. No test opens a real device: the macOS transport (g29/RacingWheelMac.cpp) is CI's to compile and the
// owner's to run.
//   KIND, COMMANDS, LIGHTS   hid-lg4ff's ids and bytes, as there
//   MAP                      the steering, the pedals (deadband), the shifter, the buttons, the d-pad, rest
//   DRIVER                   the decisions (read, off, unplug, range, others, switch, failed, denied, lights, quit)
//   INPUT                    the state into the Driver: pedals, paddles, the start key, the shifter, let go
#include "input/Driver.h"
#include "input/g29/G29.h"
#include "input/g29/G29Driver.h"
#include "input/g29/G29Input.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace tee::input;
namespace wh = tee::input::g29;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr std::uint32_t kDesk = wh::kDesktop, kBtn = wh::kButtonPage;
    constexpr std::uint32_t kX = 0x30, kY = 0x31, kZ = 0x32, kRz = 0x35;

    std::string text(const wh::Report& r)
    {
        std::string s;
        char b[4];
        for (const std::uint8_t x : r)
        {
            std::snprintf(b, sizeof b, "%02x", x);
            s += (s.empty() ? "" : " ") + std::string(b);
        }
        return s;
    }

    // A transport of made-up devices: each named, holding the values the test gives it; what the driver does, in order,
    // as lines ("g29: f8 81 ...", "listen g29", "arm lights 0.025").
    struct Fake final : wh::Transport
    {
        std::map<wh::Device, std::string> names;
        std::map<wh::Device, std::vector<wh::Value>> held;
        std::vector<std::string> log;
        int failing = 0;                                     // a report whose first two bytes are `failing`'s fails
        bool armed[2] = { false, false };
        int changes = 0;
        int asked = -1;                                      // the shift lights asked for (lights())

        static const char* timerName(wh::Timer t) { return t == wh::Timer::switched ? "switch" : "lights"; }
        std::string name(wh::Device d) const { return d == nullptr ? "none" : names.at(d); }

        int report(wh::Device d, const wh::Report& r) override
        {
            log.push_back(name(d) + ": " + text(r));
            return ((r[0] << 8) | r[1]) == failing ? static_cast<int>(0xE00002BCu) : 0;   // kIOReturnError
        }
        std::vector<wh::Value> values(wh::Device d) override { return held[d]; }
        void listen(wh::Device d) override { log.push_back("listen " + name(d)); }
        void arm(wh::Timer t, double seconds) override
        {
            armed[static_cast<int>(t)] = true;
            char b[64];
            std::snprintf(b, sizeof b, "arm %s %.3f", timerName(t), seconds);
            log.emplace_back(b);
        }
        void disarm(wh::Timer t) override
        {
            if (armed[static_cast<int>(t)])
                log.push_back(std::string("disarm ") + timerName(t));
            armed[static_cast<int>(t)] = false;
        }
        void fire(wh::Driver& d, wh::Timer t)
        {
            armed[static_cast<int>(t)] = false;
            d.fire(t);
        }
        void changed() override { ++changes; }
        int lights() override { return asked; }

        // The lines since the last look, joined by " | " (the timers' left out unless asked).
        std::string take(bool timers = false)
        {
            std::string s;
            for (const std::string& l : log)
                if (timers || (l.rfind("arm ", 0) != 0 && l.rfind("disarm ", 0) != 0))
                    s += (s.empty() ? "" : " | ") + l;
            log.clear();
            return s;
        }
    };

    wh::Value hid(std::uint32_t page, std::uint32_t usage, std::int64_t value, std::int64_t min, std::int64_t max,
                  std::uint64_t timestamp)
    {
        return { page, usage, value, min, max, 16, timestamp };
    }
}

TEST_CASE("g29: the device told apart as hid-lg4ff does; its commands' bytes; the shift lights' report",
          "[shell][input][g29]")
{
    CHECK(wh::kindOf(0x046D, 0xC24F, 0x0001) == wh::Kind::g29);
    CHECK(wh::kindOf(0x046D, 0xC294, 0x1350) == wh::Kind::compat);
    CHECK(wh::kindOf(0x046D, 0xC294, 0x1357) == wh::Kind::compat);
    CHECK(wh::kindOf(0x046D, 0xC294, 0x89A1) == wh::Kind::compat);
    CHECK(wh::kindOf(0x046D, 0xC294, 0x1230) == wh::Kind::none);       // a G27 in compatibility mode
    CHECK(wh::kindOf(0x046D, 0xC294, 0x0000) == wh::Kind::none);
    CHECK(wh::kindOf(0x046D, 0xC260, 0x8900) == wh::Kind::ps4);
    CHECK(wh::kindOf(0x054C, 0xC24F, 0) == wh::Kind::none);
    CHECK(wh::isG29(0x046D, 0xC24F));
    CHECK(wh::isG29(0x046D, 0xC294));
    CHECK(wh::isG29(0x046D, 0xC260));
    CHECK_FALSE(wh::isG29(0x046D, 0xC29B));                           // a G27: a gamepad's to take if SDL maps it
    CHECK_FALSE(wh::isG29(0x054C, 0xC24F));

    CHECK(text(wh::kToNative[0]) == "f8 0a 00 00 00 00 00");
    CHECK(text(wh::kToNative[1]) == "f8 09 05 01 01 00 00");
    CHECK(text(wh::rangeReport(900)) == "f8 81 84 03 00 00 00");
    CHECK(text(wh::rangeReport(2000)) == "f8 81 84 03 00 00 00");
    CHECK(text(wh::springReports(wh::kSpring)[0]) == "fe 0d 03 03 48 00 00");
    CHECK(text(wh::springReports(0xFFFF)[0]) == "fe 0d 07 07 ff 00 00");
    CHECK(text(wh::springReports(wh::kSpring)[1]) == "14 00 00 00 00 00 00");
    CHECK(text(wh::kSpringOff) == "f5 00 00 00 00 00 00");

    CHECK(text(wh::lightsReport(0x15)) == "f8 12 15 00 00 00 01");
    CHECK(text(wh::lightsReport(0xFF)) == "f8 12 1f 00 00 00 01");
    CHECK(text(wh::lightsReport(0)) == "f8 12 00 00 00 00 01");
    CHECK(wh::litMask(0) == 0);
    CHECK(wh::litMask(1) == 0x01);
    CHECK(wh::litMask(3) == 0x07);
    CHECK(wh::litMask(5) == 0x1F);
    CHECK(wh::litMask(9) == 0x1F);
    CHECK(wh::litMask(-1) == 0);
}

TEST_CASE("g29: the map: the steering, the pedals past their deadband, the shifter, the buttons, the d-pad, rest",
          "[shell][input][g29]")
{
    wh::Map map;
    CHECK(map.state() == wh::State{});                               // at rest: centred, released, neutral, none
    CHECK(map.input(kDesk, kX, 65535, 0, 65535));
    CHECK(map.state().v14[0] == 16383);
    CHECK_THAT(map.state().steering(), WithinAbs(1.0, 1e-6));
    map.input(kDesk, kX, 32768, 0, 65535);
    CHECK(map.state().v14[0] == 8192);
    CHECK_THAT(map.state().steering(), WithinAbs(0.0, 1e-6));
    map.input(kDesk, kX, 0, 0, 65535);
    CHECK_THAT(map.state().steering(), WithinAbs(-1.0, 1e-6));
    CHECK(wh::Map::axisValue(wh::Axis::steering, false, 512, 0, 1023) == 8192);
    CHECK(wh::Map::axisValue(wh::Axis::steering, false, 1023, 0, 1023) == 16368);

    map.input(kDesk, kZ, 255, 0, 255);
    CHECK(map.state().v14[1] == 0);                                  // Logitech's pedals: their maximum at rest
    map.input(kDesk, kZ, 0, 0, 255);
    CHECK_THAT(map.state().pedal(wh::Axis::throttle), WithinAbs(1.0, 1e-6));
    map.input(kDesk, kZ, 128, 0, 255);
    CHECK_THAT(map.state().pedal(wh::Axis::throttle), WithinAbs(124.0 / 252.0, 1e-3));   // past 3 of 255
    map.input(kDesk, kRz, 0, 0, 255);
    map.input(kDesk, kY, 255, 0, 255);
    CHECK_THAT(map.state().pedal(wh::Axis::brake), WithinAbs(1.0, 1e-6));
    CHECK(map.state().pedal(wh::Axis::clutch) == 0.0f);
    map.input(kDesk, kY, 0, 0, 255);
    CHECK_THAT(map.state().pedal(wh::Axis::clutch), WithinAbs(1.0, 1e-6));
    CHECK(wh::Map::axisValue(wh::Axis::throttle, false, 255, 0, 255) == 16383);
    CHECK(wh::Map::axisValue(wh::Axis::brake, true, 252, 0, 255) == 0);         // 3 of 255 down: rest
    CHECK(wh::Map::axisValue(wh::Axis::brake, true, 251, 0, 255) == 65);        // a step past it
    CHECK(wh::Map::axisValue(wh::Axis::clutch, true, 1013, 0, 1023) == 0);      // 10 of 1023
    CHECK(wh::Map::axisValue(wh::Axis::clutch, true, 0, 0, 1023) == 16383);

    for (int b = 13; b <= 18; ++b)
    {
        map.input(kBtn, static_cast<std::uint32_t>(b), 1, 0, 1);
        CHECK(map.state().gear == b - 12);
        map.input(kBtn, static_cast<std::uint32_t>(b), 0, 0, 1);
    }
    CHECK(map.state().gear == 0);
    map.input(kBtn, 19, 1, 0, 1);
    CHECK(map.state().gear == -1);
    map.input(kBtn, 19, 0, 0, 1);
    CHECK(map.state().gear == 0);

    map.input(kBtn, 1, 1, 0, 1);                                     // cross: bit 0
    map.input(kBtn, 20, 1, 0, 1);                                    // +: bit 12
    map.input(kBtn, 25, 1, 0, 1);                                    // PS: bit 17
    map.input(kBtn, 5, 1, 0, 1);                                     // the right paddle: bit 4
    CHECK(map.state().buttons == (1u | (1u << 12) | (1u << 17) | (1u << 4)));
    CHECK(map.state().button(wh::kBitPs));
    CHECK(map.state().button(wh::kBitRightPaddle));
    CHECK_FALSE(map.state().button(wh::kBitLeftPaddle));
    CHECK_FALSE(map.input(kBtn, 30, 1, 0, 1));                       // no row
    map.input(kDesk, wh::kHat, 2, 0, 7);
    CHECK(map.state().button(19));                                   // d-pad right
    map.input(kDesk, wh::kHat, 1, 0, 7);
    CHECK((map.state().button(18) && map.state().button(19)));       // up-right
    map.input(kDesk, wh::kHat, 8, 0, 7);
    CHECK_FALSE((map.state().button(18) || map.state().button(19) || map.state().button(20) || map.state().button(21)));
    CHECK_FALSE(map.input(0xFF00, 0x01, 5, 0, 255));                 // another page

    map.rest();
    CHECK(map.state() == wh::State{});

    std::int64_t v = -1, lo = 0, hi = -1;
    wh::normalise(v, lo, hi, 16);
    CHECK(hi == 65535);
    CHECK(v == 65535);
    std::int64_t v2 = 300, lo2 = 0, hi2 = 1023;
    wh::normalise(v2, lo2, hi2, 10);
    CHECK((hi2 == 1023 && v2 == 300));
    CHECK(wh::bitNames().rfind("0 cross, 1 square", 0) == 0);
}

TEST_CASE("g29: the driver's decisions over fake devices: read, off, unplug, range, switch, failed, denied, lights, quit",
          "[shell][input][g29]")
{
    int tags[5]{};
    const wh::Device g29 = &tags[0], compat = &tags[1], compat2 = &tags[2], ps4 = &tags[3], other = &tags[4];
    Fake f;
    f.names = { { g29, "g29" }, { compat, "compat" }, { compat2, "compat2" }, { ps4, "ps4" }, { other, "other" } };
    // What IOKit holds before reports: zeros with no timestamp (a pedal at 0 is pressed, the steering full left, the hat
    // up, the PS button down); the steering from a report (three quarters right).
    f.held[g29] = { hid(kDesk, kX, 49152, 0, 65535, 7), hid(kDesk, kZ, 0, 0, 255, 0), hid(kDesk, kRz, 0, 0, 255, 0),
                    hid(kDesk, kY, 0, 0, 255, 0), hid(kDesk, wh::kHat, 0, 0, 7, 0), hid(kBtn, 25, 1, 0, 1, 0) };
    const std::string range = "g29: f8 81 84 03 00 00 00", spring = "g29: fe 0d 03 03 48 00 00 | g29: 14 00 00 00 00 00 00";
    const std::string off = "g29: f5 00 00 00 00 00 00";
    const std::string toNative = ": f8 0a 00 00 00 00 00 | ", toNative2 = ": f8 09 05 01 01 00 00";
    const std::string failed = "it did not switch to its full mode: plug it in again, then turn this off and on";
    const auto seeded = [](const wh::State& s) {
        // the seed rule: only what a report carried (the steering, 12288), the rest at rest
        return s.v14[0] == 12288 && s.v14[1] == 0 && s.v14[2] == 0 && s.v14[3] == 0 && s.gear == 0 && s.buttons == 0;
    };

    // READ
    wh::Driver d(f, true);
    d.opened(0);
    CHECK_FALSE(d.present());
    d.added(g29, wh::Kind::g29);
    CHECK(f.take() == range + " | " + spring + " | listen g29");
    CHECK(d.reading());
    CHECK(seeded(d.state()));
    CHECK(d.status().empty());
    d.value(g29, hid(kDesk, kZ, 0, 0, 255, 9));                    // the throttle down, from a report
    CHECK_THAT(d.state().pedal(wh::Axis::throttle), WithinAbs(1.0, 1e-6));
    d.value(g29, hid(0xFF00, 1, 5, 0, 255, 11));                   // no row
    d.value(compat, hid(kDesk, kRz, 0, 0, 255, 11));               // not the one read
    CHECK(d.state().pedal(wh::Axis::brake) == 0.0f);
    CHECK(f.take(true).empty());

    // OFF
    d.setOn(false);
    CHECK(f.take() == "listen none | " + off);
    CHECK_FALSE(d.reading());
    CHECK(d.state() == wh::State{});                                // at rest: nothing stays held
    CHECK((d.present() && !d.on() && d.status().empty()));
    d.setOn(true);
    CHECK(f.take() == range + " | " + spring + " | listen g29");
    f.failing = 0xf500;
    d.setOn(false);
    f.failing = 0;
    CHECK(d.status() == "its spring may still be on (0xe00002bc): unplug it to stop it");
    d.setOn(true);
    f.take();
    CHECK(d.status().empty());

    // UNPLUG
    d.removed(g29);
    CHECK(f.take() == "listen none");                               // nothing sent to a wheel that is gone
    CHECK_FALSE(d.present());

    // RANGE
    f.failing = 0xf881;
    d.added(g29, wh::Kind::g29);
    f.failing = 0;
    CHECK(f.take() == range + " | listen g29");                     // no spring after a range that failed
    CHECK(d.status() == "its range and spring were not set (0xe00002bc)");
    CHECK(d.reading());                                             // read all the same
    d.removed(g29);
    f.take();

    // OTHERS
    d.added(ps4, wh::Kind::ps4);
    CHECK(f.take(true).empty());
    CHECK(d.status() == "set the switch on the wheel to PS3");
    d.removed(ps4);
    d.added(other, wh::Kind::none);
    CHECK_FALSE(d.present());
    CHECK(f.take(true).empty());

    // SWITCH
    d.added(compat, wh::Kind::compat);
    CHECK(f.take(true) == "compat" + toNative + "compat" + toNative2 + " | arm switch 8.000");
    CHECK(d.status() == "switching to its full mode");
    d.removed(compat);                                              // re-enumerating
    CHECK((d.present() && f.armed[0]));
    CHECK(d.status() == "switching to its full mode");
    CHECK(f.take(true).empty());
    d.added(g29, wh::Kind::g29);
    CHECK(f.take(true) == "disarm switch | " + range + " | " + spring + " | listen g29 | arm lights 0.025");
    CHECK(d.status().empty());
    d.removed(g29);                                                 // unplugged and plugged in again: compatibility mode
    d.added(compat2, wh::Kind::compat);
    CHECK(f.take(true) == "listen none | disarm lights");          // not switched again this "on"
    CHECK(d.status() == "back in its PS3 mode: turn this off and on to switch it");
    d.setOn(false);
    CHECK(d.status() == "in its PS3 mode: switched to its full mode when on");
    d.setOn(true);
    CHECK(f.take(true) == "compat2" + toNative + "compat2" + toNative2 + " | arm switch 8.000");

    // FAILED
    d.removed(compat2);
    d.added(compat, wh::Kind::compat);                              // it came back in its compatibility mode
    CHECK(f.take(true) == "disarm switch");
    CHECK(d.status() == failed);
    d.setOn(false);
    d.setOn(true);
    f.take();
    d.removed(compat);
    f.fire(d, wh::Timer::switched);                                 // 8 s and nothing came back
    CHECK(d.present());
    CHECK(d.status() == failed);
    d.setOn(false);
    CHECK_FALSE(d.present());
    f.failing = 0xf80a;
    d.added(compat, wh::Kind::compat);
    d.setOn(true);
    f.failing = 0;
    CHECK(f.take(true) == "compat: f8 0a 00 00 00 00 00");
    CHECK(d.status() == "it could not be switched to its full mode (0xe00002bc): plug it in again, then turn this off and on");
    d.removed(compat);
    d.setOn(false);
    f.take(true);

    // DENIED
    {
        wh::Driver denied(f, true);
        denied.opened(wh::Driver::kNotPermitted);
        denied.added(compat, wh::Kind::compat);
        CHECK(f.take(true).empty());
#if defined(__APPLE__)
        CHECK(denied.status() == "macOS keeps it from this app: allow it in Privacy & Security, Input Monitoring");
#else
        CHECK(denied.status()
              == "the system keeps it from this app: its device needs your read and write access (see docs/controls.md)");
#endif
        denied.quit();
    }

    // LIGHTS
    d.setOn(true);
    d.added(g29, wh::Kind::g29);
    f.take(true);
    {
        const std::string lights = "g29: f8 12 ";
        f.fire(d, wh::Timer::lights);                               // none asked
        CHECK(f.take(true) == "arm lights 0.025");
        f.asked = 0x07;
        f.fire(d, wh::Timer::lights);
        CHECK(f.take(true) == lights + "07 00 00 00 01 | arm lights 0.025");
        f.fire(d, wh::Timer::lights);
        CHECK(f.take().empty());                                    // held: nothing
        f.asked = 0x1F;
        f.fire(d, wh::Timer::lights);
        CHECK(f.take() == lights + "1f 00 00 00 01");
        f.asked = -1;
        f.fire(d, wh::Timer::lights);
        f.fire(d, wh::Timer::lights);
        CHECK(f.take() == lights + "00 00 00 00 01");               // none asked: off, once
        f.asked = 0x03;
        f.fire(d, wh::Timer::lights);
        f.take();
        d.setOn(false);
        CHECK(f.take(true) == "listen none | " + off + " | " + lights + "00 00 00 00 01 | disarm lights");
        CHECK_FALSE(f.armed[1]);
        d.setOn(true);                                              // read again: the lights asked go at once
        CHECK(f.take(true) == range + " | " + spring + " | listen g29 | " + lights + "03 00 00 00 01 | arm lights 0.025");
        d.removed(g29);
        CHECK(f.take(true) == "listen none | disarm lights");      // unplugged: nothing sent to it
        f.asked = 0;
        d.added(g29, wh::Kind::g29);                                // none lit assumed: nothing sent for 0
        CHECK(f.take(true) == range + " | " + spring + " | listen g29 | arm lights 0.025");
        f.failing = 0xf812;
        f.asked = 0x01;
        f.fire(d, wh::Timer::lights);
        f.fire(d, wh::Timer::lights);
        CHECK(f.take() == lights + "01 00 00 00 01");               // tried once
        CHECK(d.status() == "its shift lights were not set (0xe00002bc)");
        f.failing = 0;
        f.asked = 0x02;
        f.fire(d, wh::Timer::lights);
        CHECK(f.take() == lights + "02 00 00 00 01");               // tried again on a change
    }

    // QUIT
    d.quit();
    CHECK(f.take(true) == "listen none | " + off + " | g29: f8 12 00 00 00 00 01 | disarm lights");
    f.asked = 0x1F;
    f.fire(d, wh::Timer::lights);
    d.value(g29, hid(kDesk, kZ, 255, 0, 255, 21));
    d.added(compat, wh::Kind::compat);
    d.setOn(false);
    CHECK(f.take(true).empty());                                    // after it, nothing

    wh::Driver idle(f, false);
    idle.added(g29, wh::Kind::g29);
    CHECK(f.take(true).empty());                                    // off: open, nothing read or sent
    CHECK_FALSE(idle.reading());
    idle.quit();
    CHECK(f.take(true).empty());
}

TEST_CASE("g29: its state into the driver: the pedals beside a gamepad's, paddles, the start key, the shifter, let go",
          "[shell][input][g29]")
{
    DriverSettings manual;
    manual.autoClutch = false;                                      // the H-pattern lever is the shifter's
    tee::input::Driver driver(manual);
    wh::Input input;
    const EngineSeen seen{};
    wh::State s;

    // the pedals: each the larger of the wheel's and a gamepad's
    s.v14[1] = 16383;                                               // throttle down
    s.v14[3] = 8192;                                                // clutch half way
    driver.setAnalog(0.2f, 0.7f, 0.0f);                             // a gamepad: a little throttle, more brake
    input.apply(s, true, driver);
    const tee::ControlsFrame& f1 = driver.advance(0.01f, seen);
    CHECK_THAT(f1.throttle, WithinAbs(1.0, 1e-6));
    CHECK_THAT(f1.brake, WithinAbs(0.7, 1e-6));
    CHECK_THAT(f1.clutch, WithinAbs(8192.0 / 16383.0, 1e-4));
    CHECK(f1.steer == 0.0f);                                        // the wheel's steering drives nothing

    // the paddles: a shift each press; the shifter: the lever
    const int ups = f1.shiftUp, downs = f1.shiftDown;
    s.buttons = 1u << wh::kBitRightPaddle;
    s.gear = 3;
    input.apply(s, true, driver);
    const tee::ControlsFrame& f2 = driver.advance(0.01f, seen);
    CHECK(f2.shiftUp == ups + 1);
    CHECK(f2.lever == 3);
    s.buttons = 0;
    input.apply(s, true, driver);
    driver.advance(0.01f, seen);
    s.buttons = 1u << wh::kBitLeftPaddle;
    s.gear = -1;
    input.apply(s, true, driver);
    const tee::ControlsFrame& f3 = driver.advance(0.01f, seen);
    CHECK(f3.shiftDown == downs + 1);
    CHECK(f3.shiftUp == ups + 1);                                   // the paddle let go: no second count
    CHECK(f3.lever == -1);
    s.gear = 0;
    input.apply(s, true, driver);
    CHECK(driver.advance(0.01f, seen).lever == 0);                   // out of every gate: neutral

    // the PS button: the start key, held
    s.buttons = 1u << wh::kBitPs;
    input.apply(s, true, driver);
    CHECK(driver.advance(0.01f, seen).ignition == 2);

    // reading stops: everything the wheel held let go (the gamepad's stays); the start key's crank of a stopped engine
    // runs out by itself (the Driver's rule: at most 2 s), not held by the wheel
    input.apply(s, false, driver);
    for (int i = 0; i < 60; ++i)                                    // 3 s in frames (advance takes a frame's step)
        driver.advance(0.05f, seen);
    const tee::ControlsFrame& f4 = driver.advance(0.01f, seen);
    CHECK(f4.ignition != 2);
    CHECK_THAT(f4.throttle, WithinAbs(0.2, 1e-6));
    CHECK(f4.clutch == 0.0f);
}

TEST_CASE("g29: the buttons' bindings: the defaults, kept and read back, a bad pair left out", "[shell][input][g29]")
{
    const wh::Buttons d = wh::defaultButtons();
    const std::string saved = wh::saveButtons(d);
    CHECK(saved.rfind("2:4=shiftup,5=shiftdown,17=start,18=e.rowup,", 0) == 0);
    std::string notes;
    CHECK(wh::loadButtons("", notes) == d);
    CHECK(wh::loadButtons(saved, notes) == d);
    CHECK(notes.empty());
    CHECK(wh::saveButtons({}) == "2:");
    CHECK(wh::loadButtons("2:", notes).empty());                          // every button unbound, as the owner left it

    const wh::Buttons b = wh::loadButtons("2:0=splitter,99=range,3=gear1,x=neutral,2=handbrake,,21=enginebrake,1=e.undo,"
                                  "6=e.nothing",
                                  notes);
    REQUIRE(b.size() == 4);
    CHECK(b[0] == wh::drive(0, Action::splitter));
    CHECK(b[1] == wh::drive(2, Action::handbrake));
    CHECK(b[2] == wh::drive(21, Action::engineBrake));
    CHECK(b[3] == wh::editOn(1, wh::EditCommand::undo));
    CHECK(notes.find("99=range") != std::string::npos);            // outside the mask
    CHECK(notes.find("3=gear1") != std::string::npos);             // not a button's (the shifter's)
    CHECK(notes.find("x=neutral") != std::string::npos);
    CHECK(notes.find("6=e.nothing") != std::string::npos);         // no such edit command
}

TEST_CASE("g29: a saved binding from before editing keeps every pair and gains the edit defaults on free buttons",
          "[shell][input][g29]")
{
    // the owner moved the start key to cross (the edit default's activate) and bound the splitter to the d-pad's up
    std::string notes;
    const wh::Buttons b = wh::loadButtons("4=shiftup,5=shiftdown,0=start,18=splitter", notes);
    CHECK(notes.empty());
    CHECK(std::find(b.begin(), b.end(), wh::drive(0, Action::start)) != b.end());
    CHECK(std::find(b.begin(), b.end(), wh::drive(18, Action::splitter)) != b.end());
    // cross and the d-pad's up stay theirs: no edit lands on a button the owner bound
    for (const wh::Binding& x : b)
        if (x.bit == 0 || x.bit == 18)
            CHECK_FALSE(x.isEdit());
    // the other edit defaults arrive; activate stays on the dial's return; the row down still on the d-pad
    CHECK(std::find(b.begin(), b.end(), wh::editOn(16, wh::EditCommand::activate)) != b.end());
    CHECK(std::find(b.begin(), b.end(), wh::editOn(20, wh::EditCommand::rowDown)) != b.end());
    CHECK(std::find(b.begin(), b.end(), wh::editOn(1, wh::EditCommand::undo)) != b.end());
    // PS was the start key's: the owner moved it, so PS is free (nothing edits with it)
    for (const wh::Binding& x : b)
        CHECK(x.bit != wh::kBitPs);
    // saved again it is the new format, read back exactly
    const std::string again = wh::saveButtons(b);
    CHECK(again.rfind("2:", 0) == 0);
    CHECK(wh::loadButtons(again, notes) == b);
    // the owner's SHARE: a binding of theirs is kept, and SHARE is no longer the modifier (Input's rule)
    const wh::Buttons shared = wh::loadButtons("8=handbrake", notes);
    CHECK(std::find(shared.begin(), shared.end(), wh::drive(8, Action::handbrake)) != shared.end());
}

TEST_CASE("g29: learning a button: the next press binds it; a held one waits; it moves; time runs out",
          "[shell][input][g29]")
{
    wh::Buttons buttons = wh::drivingDefaults();
    wh::ButtonLearn learn;
    wh::State s;
    const wh::Target splitter{ Action::splitter, std::nullopt }, start{ Action::start, std::nullopt };

    // the cross held as learning begins: it is not the answer until let go and pressed again
    s.buttons = 1u << 0;
    learn.begin(splitter, s);
    CHECK(learn.active());
    CHECK_FALSE(learn.watch(s, 0.1, buttons));
    s.buttons = 0;
    CHECK_FALSE(learn.watch(s, 0.1, buttons));
    s.buttons = 1u << 0;
    CHECK(learn.watch(s, 0.1, buttons));
    CHECK_FALSE(learn.active());
    CHECK(buttons.back() == wh::drive(0, Action::splitter));
    CHECK(buttons.size() == 4);

    // the right paddle for the start key: the paddle loses shift up, and PS loses the start key
    s.buttons = 0;
    learn.begin(start, s);
    s.buttons = 1u << wh::kBitRightPaddle;
    CHECK(learn.watch(s, 0.1, buttons));
    CHECK(wh::saveButtons(buttons) == "2:5=shiftdown,0=splitter,4=start");

    // an edit command: it may stay on other buttons (activate on two), the learned button loses its own
    wh::Buttons edits = { wh::editOn(16, wh::EditCommand::activate), wh::drive(1, Action::handbrake) };
    s.buttons = 0;
    learn.begin({ Action::none, wh::EditCommand::activate }, s);
    s.buttons = 1u << 1;
    CHECK(learn.watch(s, 0.1, edits));
    CHECK(wh::saveButtons(edits) == "2:16=e.activate,1=e.activate");

    // SHARE, the modifier while unbound, is never the answer
    s.buttons = 0;
    learn.begin({ Action::none, wh::EditCommand::undo }, s);
    s.buttons = 1u << wh::kBitShare;
    CHECK_FALSE(learn.watch(s, 0.1, edits));
    CHECK(learn.active());
    learn.cancel();

    // nothing pressed: it ends with nothing after its seconds
    s.buttons = 0;
    learn.begin({ Action::neutral, std::nullopt }, s);
    for (int i = 0; i < 99 && learn.active(); ++i)
        CHECK_FALSE(learn.watch(s, 0.25, buttons));
    CHECK_FALSE(learn.active());
    CHECK(wh::saveButtons(buttons) == "2:5=shiftdown,0=splitter,4=start");
}

TEST_CASE("g29: bound buttons drive their actions; a learn listening holds them back; new bindings let go",
          "[shell][input][g29]")
{
    DriverSettings instant;
    instant.handbrake.rise = instant.handbrake.fall = 0.0f;           // the handbrake follows its key at once
    tee::input::Driver driver(instant);
    wh::Input input;
    const EngineSeen seen{};
    wh::State s;
    std::string notes;
    input.setButtons(wh::loadButtons("2:0=splitter,1=handbrake", notes));
    input.apply(s, true, driver);                                   // (the frame that takes the new bindings)
    const int ups = driver.advance(0.01f, seen).shiftUp;

    // the cross toggles the splitter at each press; the square holds the handbrake; the right paddle no longer shifts
    s.buttons = 1u << 0 | 1u << 1 | 1u << wh::kBitRightPaddle;
    input.apply(s, true, driver);
    const tee::ControlsFrame& f1 = driver.advance(0.01f, seen);
    CHECK(f1.shiftUp == ups);
    CHECK(f1.handbrake == 1.0f);
    CHECK(driver.latchedButtons() == kButtonSplitter);

    // a learn listening: the buttons do nothing (the square's hold is let go, the cross's press is the learn's)
    s.buttons = 0;
    input.apply(s, true, driver, false);
    s.buttons = 1u << 0 | 1u << 1;
    input.apply(s, true, driver, false);
    CHECK(driver.advance(0.01f, seen).handbrake == 0.0f);
    CHECK(driver.latchedButtons() == kButtonSplitter);
    // the learn over, the buttons still held count only once pressed again
    input.apply(s, true, driver);
    CHECK(driver.advance(0.01f, seen).handbrake == 0.0f);
    CHECK(driver.latchedButtons() == kButtonSplitter);
    s.buttons = 0;
    input.apply(s, true, driver);
    s.buttons = 1u << 0 | 1u << 1;
    input.apply(s, true, driver);
    CHECK(driver.advance(0.01f, seen).handbrake == 1.0f);
    CHECK(driver.latchedButtons() == 0);

    // new bindings: what the old ones held is let go at once
    input.setButtons(wh::defaultButtons());
    input.apply(s, true, driver);
    CHECK(driver.advance(0.01f, seen).handbrake == 0.0f);
}

TEST_CASE("g29: each edit default gives its command once a press; driving buttons give none", "[shell][input][g29]")
{
    tee::input::Driver driver{ DriverSettings{} };
    wh::Input input;
    wh::State s;
    input.apply(s, true, driver, true, 0.016);
    for (const wh::Binding& b : wh::editDefaults())
    {
        INFO(wh::kBits[b.bit].name);
        s.buttons = 1u << b.bit;
        input.apply(s, true, driver, true, 0.016);
        REQUIRE(input.edits().size() == 1);
        CHECK(input.edits()[0] == wh::Edit{ *b.edit, false });
        // held one more frame: no second command (the row keys repeat only after kRepeatAfter)
        input.apply(s, true, driver, true, 0.016);
        CHECK(input.edits().empty());
        s.buttons = 0;
        input.apply(s, true, driver, true, 0.016);
        CHECK(input.edits().empty());
    }
    // the driving defaults and R3: no edit
    for (const int bit : { wh::kBitRightPaddle, wh::kBitLeftPaddle, wh::kBitPs, 10 })
    {
        s.buttons = 1u << bit;
        input.apply(s, true, driver, true, 0.016);
        CHECK(input.edits().empty());
        s.buttons = 0;
        input.apply(s, true, driver, true, 0.016);
    }
}

TEST_CASE("g29: the d-pad's rows repeat while held; SHARE makes a command fine; a learn or no reading gives none",
          "[shell][input][g29]")
{
    tee::input::Driver driver{ DriverSettings{} };
    wh::Input input;
    wh::State s;
    const double dt = 0.01;
    input.apply(s, true, driver, true, dt);

    // the d-pad down held 1 s: one at the press, the first repeat at 0.35 s, then 12 a second
    int downBit = -1;
    for (const wh::Binding& b : wh::editDefaults())
        if (b.edit == wh::EditCommand::rowDown)
            downBit = b.bit;
    REQUIRE(downBit >= 0);
    s.buttons = 1u << downBit;
    std::vector<double> at;
    for (int f = 0; f <= 100; ++f)
    {
        input.apply(s, true, driver, true, dt);
        for (std::size_t k = 0; k < input.edits().size(); ++k)
            at.push_back(f * dt);
        CHECK(input.edits().size() <= 1);                            // at most one a frame
    }
    REQUIRE(at.size() >= 2);
    CHECK(at[0] == 0.0);
    CHECK(at[1] >= wh::kRepeatAfter - 1e-9);
    CHECK(at[1] < wh::kRepeatAfter + 2 * dt);
    // (1.0 - 0.35) s at 12 a second: 7 or 8 repeats after the first
    CHECK(at.size() >= 1 + 1 + 7);
    CHECK(at.size() <= 1 + 1 + 9);
    // the dial does not repeat: held, one command
    s.buttons = 0;
    input.apply(s, true, driver, true, dt);
    int dialUp = -1;
    for (const wh::Binding& b : wh::editDefaults())
        if (b.edit == wh::EditCommand::valueUp)
            dialUp = b.bit;
    s.buttons = 1u << dialUp;
    int n = 0;
    for (int f = 0; f < 100; ++f)
    {
        input.apply(s, true, driver, true, dt);
        n += static_cast<int>(input.edits().size());
    }
    CHECK(n == 1);

    // SHARE held: the dial's nudge is fine
    s.buttons = 1u << wh::kBitShare;
    input.apply(s, true, driver, true, dt);
    CHECK(input.edits().empty());                                    // SHARE itself commands nothing
    s.buttons = 1u << wh::kBitShare | 1u << dialUp;
    input.apply(s, true, driver, true, dt);
    REQUIRE(input.edits().size() == 1);
    CHECK(input.edits()[0] == wh::Edit{ wh::EditCommand::valueUp, true });

    // SHARE bound by the owner: theirs, not a modifier
    std::string notes;
    input.setButtons(wh::loadButtons("2:8=handbrake,14=e.valueup", notes));
    s.buttons = 0;
    input.apply(s, true, driver, true, dt);
    s.buttons = 1u << wh::kBitShare;
    input.apply(s, true, driver, true, dt);
    s.buttons = 1u << wh::kBitShare | 1u << 14;
    input.apply(s, true, driver, true, dt);
    REQUIRE(input.edits().size() == 1);
    CHECK_FALSE(input.edits()[0].fine);

    // a learn listening, or the wheel not read: nothing
    input.setButtons(wh::defaultButtons());
    s.buttons = 0;
    input.apply(s, true, driver, true, dt);
    s.buttons = 1u << dialUp;
    input.apply(s, true, driver, false, dt);
    CHECK(input.edits().empty());
    s.buttons = 0;
    input.apply(s, false, driver, true, dt);
    s.buttons = 1u << dialUp;
    input.apply(s, false, driver, true, dt);
    CHECK(input.edits().empty());
}

TEST_CASE("g29: a button the owner bound to driving stays driving: its press drives, and no edit", "[shell][input][g29]")
{
    DriverSettings instant;
    instant.handbrake.rise = instant.handbrake.fall = 0.0f;
    tee::input::Driver driver(instant);
    wh::Input input;
    const EngineSeen seen{};
    std::string notes;
    // the pre-edit format: the owner's handbrake on cross (an edit default's button) and on the d-pad's up
    input.setButtons(wh::loadButtons("4=shiftup,5=shiftdown,17=start,0=handbrake", notes));
    wh::State s;
    input.apply(s, true, driver, true, 0.016);
    s.buttons = 1u << 0;
    input.apply(s, true, driver, true, 0.016);
    CHECK(input.edits().empty());
    CHECK(driver.advance(0.01f, seen).handbrake == 1.0f);
    // the dial's return still activates (the default kept on a free button)
    s.buttons = 1u << 16;
    input.apply(s, true, driver, true, 0.016);
    REQUIRE(input.edits().size() == 1);
    CHECK(input.edits()[0].command == wh::EditCommand::activate);
}
