// tests/shell/ControlsTest.cpp: the driver's rules (shell/input/Driver.h) and the Controls over a stand-in engine.
#include "input/Controls.h"
#include "input/Driver.h"
#include "input/Keys.h"
#include "player/TonePlayer.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace tee;
using namespace tee::input;
using Catch::Approx;

namespace
{
    // dt seconds on, in frames of a sixtieth
    const ControlsFrame& run(Driver& d, double seconds, const EngineSeen& seen = {})
    {
        const int frames = static_cast<int>(seconds * 60.0 + 0.5);
        for (int i = 0; i < frames; ++i)
            d.advance(1.0f / 60.0f, seen);
        return d.frame();
    }
}

TEST_CASE("shell: the default bindings are the ones docs/controls.md lists", "[shell][input]")
{
    const std::vector<Binding> b = defaultBindings();
    CHECK(actionOf(b, key::kUp) == Action::throttle);
    CHECK(actionOf(b, key::kW) == Action::throttle);
    CHECK(actionOf(b, key::kDown) == Action::brake);
    CHECK(actionOf(b, key::kS) == Action::brake);
    CHECK(actionOf(b, key::kLeftShift) == Action::clutch);
    CHECK(actionOf(b, key::kE) == Action::shiftUp);
    CHECK(actionOf(b, key::kPageUp) == Action::shiftUp);
    CHECK(actionOf(b, key::kQ) == Action::shiftDown);
    CHECK(actionOf(b, key::kPageDown) == Action::shiftDown);
    CHECK(actionOf(b, key::kN) == Action::neutral);
    CHECK(actionOf(b, key::kSpace) == Action::handbrake);
    CHECK(actionOf(b, key::kReturn) == Action::start);
    CHECK(actionOf(b, key::k1) == Action::gear1);
    CHECK(actionOf(b, key::k6) == Action::gear6);
    CHECK(actionOf(b, key::kA) == Action::none);
    // every bound key has a name, and the names read back
    for (const Binding& binding : b)
    {
        CHECK_FALSE(keyName(binding.key).empty());
        CHECK(keyFromName(keyName(binding.key)) == binding.key);
        CHECK(actionFromName(actionName(binding.action)) == binding.action);
    }
    CHECK(keyFromName("no such key") == 0);
}

TEST_CASE("shell: a pedal eases down while its key is held and back when it is let go", "[shell][input]")
{
    DriverSettings s;
    s.throttle = { 0.30f, 0.18f };
    Driver d(s);
    d.set(Source::keyboard, Action::throttle, true);
    CHECK(run(d, 0.15).throttle == Approx(0.5).margin(0.03));
    CHECK(run(d, 0.20).throttle == 1.0f);
    d.set(Source::keyboard, Action::throttle, true);        // a repeat changes nothing
    CHECK(run(d, 0.05).throttle == 1.0f);
    d.set(Source::keyboard, Action::throttle, false);
    CHECK(run(d, 0.09).throttle == Approx(0.5).margin(0.04));
    CHECK(run(d, 0.20).throttle == 0.0f);

    // a long frame does not slam it
    d.set(Source::keyboard, Action::brake, true);
    d.advance(5.0f, {});
    CHECK(d.frame().brake == 1.0f);
    d.set(Source::keyboard, Action::brake, false);
    d.advance(0.01f, {});
    CHECK(d.frame().brake < 1.0f);
    CHECK(d.frame().brake > 0.8f);
}

TEST_CASE("shell: two keys of one action, and two sources, each let go of their own", "[shell][input]")
{
    Driver d;
    d.set(Source::keyboard, Action::throttle, true);
    d.set(Source::gamepad, Action::throttle, true);
    run(d, 0.5);
    d.set(Source::keyboard, Action::throttle, false);
    CHECK(run(d, 0.5).throttle == 1.0f);                    // the gamepad still holds it
    d.release(Source::gamepad);
    CHECK(run(d, 0.5).throttle == 0.0f);
}

TEST_CASE("shell: a gamepad's triggers and the Panel's strip are analogue, and the largest wins", "[shell][input]")
{
    Driver d;
    d.setAnalog(0.4f, 0.25f, -0.5f);
    CHECK(run(d, 0.02).throttle == Approx(0.4f));
    CHECK(d.frame().brake == Approx(0.25f));
    CHECK(d.frame().steer == Approx(-0.5f));
    d.setStrip(0.7f, true);
    CHECK(run(d, 0.02).throttle == Approx(0.7f));
    d.setStrip(0.7f, false);
    CHECK(run(d, 0.02).throttle == Approx(0.4f));
    d.setAnalog(7.0f, -3.0f, 9.0f);                         // out of range: held to it
    CHECK(run(d, 0.02).throttle == 1.0f);
    CHECK(d.frame().brake == 0.0f);
    CHECK(d.frame().steer == 1.0f);
}

TEST_CASE("shell: the start key says start while held, on when let go, and off after a second on a running engine",
          "[shell][input]")
{
    Driver d;
    EngineSeen running;
    running.running = true;
    CHECK(run(d, 0.1).ignition == 0);
    d.set(Source::keyboard, Action::start, true);
    CHECK(run(d, 0.3).ignition == 2);
    d.set(Source::keyboard, Action::start, false);
    // let go before the engine runs: the starter goes on cranking by itself until it does
    CHECK(run(d, 0.5).ignition == 2);
    CHECK(run(d, 0.1, running).ignition == 1);
    // and gives up after crankMax (2 s) on an engine that never catches: on, not cranking
    Driver dead;
    dead.set(Source::keyboard, Action::start, true);
    dead.set(Source::keyboard, Action::start, false);
    CHECK(run(dead, 1.5).ignition == 2);
    CHECK(run(dead, 0.6).ignition == 1);

    // a short press on a running engine changes nothing
    d.set(Source::keyboard, Action::start, true);
    CHECK(run(d, 0.3, running).ignition == 2);
    d.set(Source::keyboard, Action::start, false);
    CHECK(run(d, 0.1, running).ignition == 1);
    // held a second: it says start for as long as the key is down, then off
    d.set(Source::keyboard, Action::start, true);
    CHECK(run(d, 1.2, running).ignition == 2);
    d.set(Source::keyboard, Action::start, false);
    CHECK(run(d, 0.1, {}).ignition == 0);
    // held a second on an engine that was not running when the key went down: the starter, and then on
    d.set(Source::keyboard, Action::start, true);
    run(d, 1.5, {});
    d.set(Source::keyboard, Action::start, false);
    CHECK(run(d, 0.1, running).ignition == 1);
}

TEST_CASE("shell: a tap of the start key shorter than a frame still says start for a moment", "[shell][input]")
{
    // (on a running engine, where the starter does not crank on by itself: only startLeast)
    EngineSeen running;
    running.running = true;
    Driver d;
    d.set(Source::keyboard, Action::start, true);
    d.set(Source::keyboard, Action::start, false);          // down and up between two frames
    CHECK(d.advance(1.0f / 60.0f, running).ignition == 2);
    CHECK(run(d, 0.1, running).ignition == 2);              // for startLeast seconds
    CHECK(run(d, 0.1, running).ignition == 1);
    // and a slow frame (the window drawing at ten a second) sees it too
    d.set(Source::keyboard, Action::start, true);
    d.set(Source::keyboard, Action::start, false);
    CHECK(d.advance(0.1f, running).ignition == 2);
    CHECK(d.advance(0.1f, running).ignition == 2);
    CHECK(d.advance(0.1f, running).ignition == 2);
    CHECK(d.advance(0.1f, running).ignition == 1);
    // a late first frame (the window opening: a long dt, all of it before the press) does not use the tap up
    Driver late;
    late.set(Source::keyboard, Action::start, true);
    late.set(Source::keyboard, Action::start, false);
    CHECK(late.advance(1.2f, running).ignition == 2);
    CHECK(late.advance(1.0f / 60.0f, running).ignition == 2);
    CHECK(run(late, 0.1, running).ignition == 2);
    CHECK(run(late, 0.1, running).ignition == 1);
    // on a stopped engine a tap cranks it until it runs (found on DRIVE: the window's first focus event let the key
    // go a moment after it went down, and 0.15 s of starter did not start the simulator)
    Driver tap;
    tap.set(Source::keyboard, Action::start, true);
    tap.set(Source::keyboard, Action::start, false);
    CHECK(tap.advance(0.25f, {}).ignition == 2);
    CHECK(run(tap, 1.0).ignition == 2);
    CHECK(run(tap, 0.1, running).ignition == 1);
}

TEST_CASE("shell: the shift keys count presses, once a press", "[shell][input]")
{
    Driver d;
    d.set(Source::keyboard, Action::shiftUp, true);
    d.set(Source::keyboard, Action::shiftUp, true);         // the keyboard's repeat
    CHECK(run(d, 0.1).shiftUp == 1);
    d.set(Source::keyboard, Action::shiftUp, false);
    d.set(Source::keyboard, Action::shiftUp, true);
    d.set(Source::keyboard, Action::shiftDown, true);
    CHECK(run(d, 0.1).shiftUp == 2);
    CHECK(d.frame().shiftDown == 1);
}

TEST_CASE("shell: with the automatic clutch a gear key shifts until the gearbox is there", "[shell][input]")
{
    Driver d;
    REQUIRE(d.settings().autoClutch);
    EngineSeen seen;
    seen.running = true;
    seen.gearKnown = true;
    seen.gear = 0;
    d.set(Source::keyboard, Action::gear3, true);
    d.set(Source::keyboard, Action::gear3, false);
    // a gearbox that takes each press
    int last = 0;
    for (int i = 0; i < 240; ++i)
    {
        const ControlsFrame& f = d.advance(1.0f / 60.0f, seen);
        if (f.shiftUp != last)
        {
            last = f.shiftUp;
            ++seen.gear;
        }
        CHECK(f.lever == 0);                                // the lever stays in neutral: the gearbox's own clutch
    }
    CHECK(seen.gear == 3);
    CHECK(d.frame().shiftUp == 3);
    CHECK(d.frame().shiftDown == 0);

    // neutral from third: three presses down
    d.press(Action::neutral);
    int lastDown = 0;
    for (int i = 0; i < 240; ++i)
    {
        const ControlsFrame& f = d.advance(1.0f / 60.0f, seen);
        if (f.shiftDown != lastDown)
        {
            lastDown = f.shiftDown;
            --seen.gear;
        }
    }
    CHECK(seen.gear == 0);
    CHECK(d.frame().shiftDown == 3);

    // a gearbox that refuses: the key gives up after its tries
    d.requestGear(5);
    for (int i = 0; i < 1200; ++i)
        d.advance(1.0f / 60.0f, seen);
    CHECK(d.frame().shiftUp == 3 + d.settings().seekTries);
    // and with no gearbox to ask, a gear key does nothing
    const int before = d.frame().shiftUp;
    d.requestGear(2);
    run(d, 2.0, {});
    CHECK(d.frame().shiftUp == before);
}

TEST_CASE("shell: without the automatic clutch a gear key moves the lever", "[shell][input]")
{
    DriverSettings s;
    s.autoClutch = false;
    Driver d(s);
    d.set(Source::keyboard, Action::gear2, true);
    CHECK(run(d, 0.1).lever == 2);
    CHECK(d.frame().shiftUp == 0);
    d.press(Action::neutral);
    CHECK(run(d, 0.1).lever == 0);
    d.set(Source::keyboard, Action::clutch, true);
    CHECK(run(d, 0.5).clutch == 1.0f);
}

TEST_CASE("shell: the Controls drive a stand-in engine from keys and from a Panel's controls", "[shell][input]")
{
    player::TonePlayer tone;
    tone.prepare(48000.0, 256);
    Controls controls(tone, tone);
    const TelemetrySource& tel = tone;
    const int state = tel.findValue(telemetry::kState);
    const int speed = tel.findValue(telemetry::kSpeed);
    const int gear = tel.findValue(telemetry::kGear);
    REQUIRE(state >= 0);
    REQUIRE(speed >= 0);
    REQUIRE(gear >= 0);
    std::vector<float> l(800), r(800);
    const auto frames = [&](int n) {
        for (int i = 0; i < n; ++i)
        {
            controls.tick(1.0f / 60.0f);
            tone.process(l.data(), r.data(), 800);
        }
    };

    CHECK_FALSE(controls.key(key::kA, true));               // not bound: not taken
    CHECK(controls.key(key::kReturn, true));
    frames(12);
    CHECK(controls.key(key::kReturn, false));
    frames(60);
    CHECK(tel.value(state) == 2.0);
    const double idle = tel.value(speed);
    CHECK(idle > 50.0);                                     // rad/s

    // the Panel's strip is the throttle
    const AppServices services = controls.services();
    PanelControls p;
    p.throttle = 1.0f;
    services.panelControls(p);
    frames(60);
    CHECK(tel.value(speed) > 3.0 * idle);
    CHECK(services.controls().throttle == 1.0f);
    p.throttle = -1.0f;
    p.shiftUp = 2;                                          // two presses of its shift
    services.panelControls(p);
    frames(150);
    CHECK(tel.value(speed) < 1.5 * idle);
    CHECK(tel.value(gear) == 2.0);
    p.leverSet = true;                                      // its gear control: fourth
    p.lever = 4;
    services.panelControls(p);
    frames(120);
    CHECK(tel.value(gear) == 4.0);

    // a text field has the keyboard: no key drives
    controls.key(key::kW, true);
    controls.setTyping(true);
    frames(30);
    CHECK(controls.frame().throttle == 0.0f);
    CHECK_FALSE(controls.key(key::kW, true));
    controls.setTyping(false);

    // the Panel's start button, held a second: off
    p.startHeld = true;
    services.panelControls(p);
    frames(80);
    p.startHeld = false;
    services.panelControls(p);
    frames(60);
    CHECK(tel.value(state) == 0.0);
    CHECK(controls.frame().ignition == 0);

    controls.setStatus("a line");
    CHECK(services.status() == "a line");
}

TEST_CASE("shell: a Panel's clutch and handbrake bars, the frame's mode, the mouse's clutch", "[shell][input]")
{
    player::TonePlayer tone;
    tone.prepare(48000.0, 256);
    Controls controls(tone, tone);
    const AppServices services = controls.services();
    PanelControls p;
    p.clutch = 0.6f;
    p.handbrake = 1.0f;
    services.panelControls(p);
    controls.tick(1.0f / 60.0f);
    CHECK(controls.frame().clutch == Approx(0.6f));
    CHECK(controls.frame().handbrake == 1.0f);
    p.clutch = -1.0f;                   // let go: the pedals are the keys' again (nothing)
    p.handbrake = -1.0f;
    services.panelControls(p);
    controls.tick(1.0f / 60.0f);
    CHECK(controls.frame().clutch == 0.0f);
    CHECK(controls.frame().handbrake == 0.0f);

    // the frame's own clutch bar (FrameServices::clutch) and the mode
    controls.driver().setClutchStrip(0.3f, true);
    controls.driver().setMode(2);
    controls.tick(1.0f / 60.0f);
    CHECK(controls.frame().clutch == Approx(0.3f));
    CHECK(controls.frame().mode == 2);
    controls.driver().setClutchStrip(0.0f, false);
    controls.tick(1.0f / 60.0f);
    CHECK(controls.frame().clutch == 0.0f);
}

TEST_CASE("shell: the start key does nothing while the engine is not ready (its compile), and works after",
          "[shell][input]")
{
    Driver d;
    d.setStartEnabled(false);
    d.set(Source::keyboard, Action::start, true);
    CHECK(run(d, 0.3).ignition == 0);
    d.set(Source::keyboard, Action::start, false);
    CHECK(run(d, 2.5).ignition == 0);          // nothing latched to crank later
    d.setStartEnabled(true);
    CHECK(run(d, 0.1).ignition == 0);
    d.set(Source::keyboard, Action::start, true);
    d.set(Source::keyboard, Action::start, false);
    CHECK(run(d, 0.1).ignition == 2);
}

TEST_CASE("shell: a truck's switches latch: Z the splitter, X the range, J the engine brake (ctl.buttons)", "[shell][input]")
{
    Driver d;
    CHECK(run(d, 0.05).buttons == 0);
    d.set(Source::keyboard, Action::splitter, true);
    d.set(Source::keyboard, Action::splitter, false);
    CHECK(run(d, 0.05).buttons == kButtonSplitter);
    d.set(Source::keyboard, Action::engineBrake, true);
    CHECK(run(d, 0.5).buttons == (kButtonSplitter | kButtonEngineBrake));   // held: still one press
    d.set(Source::keyboard, Action::engineBrake, false);
    d.set(Source::keyboard, Action::range, true);
    d.set(Source::keyboard, Action::range, false);
    CHECK(run(d, 0.05).buttons == (kButtonSplitter | kButtonEngineBrake | kButtonRange));
    d.set(Source::keyboard, Action::splitter, true);                    // pressed again: off
    d.set(Source::keyboard, Action::splitter, false);
    CHECK(run(d, 0.05).buttons == (kButtonEngineBrake | kButtonRange));
    // the default keys
    CHECK(actionOf(defaultBindings(), key::kZ) == Action::splitter);
    CHECK(actionOf(defaultBindings(), key::kX) == Action::range);
    CHECK(actionOf(defaultBindings(), key::kJ) == Action::engineBrake);
    CHECK(actionFromName("enginebrake") == Action::engineBrake);
}

TEST_CASE("shell: an automatic's or a CVT's selector: R, N, D on the lever whatever the clutch; out of D only at a crawl",
          "[shell][input]")
{
    Driver d;                                                       // the automatic clutch on (the default)
    d.setGearbox(Gearbox::selector);
    EngineSeen still;                                               // at rest
    EngineSeen moving;
    moving.kmh = 40.0f;
    const auto step = [&](const EngineSeen& seen) { return d.advance(0.1f, seen).lever; };
    CHECK(step(still) == 0);                                        // N
    d.press(Action::shiftUp);
    CHECK(step(still) == 1);                                        // D, and held there frame after frame
    CHECK(step(still) == 1);
    d.press(Action::shiftUp);
    CHECK(step(still) == 1);                                        // (D is the top)
    // at speed a shift down does nothing (no neutral hoping for a kickdown); at a crawl D to N to R
    CHECK(step(moving) == 1);
    d.press(Action::shiftDown);
    CHECK(step(moving) == 1);
    CHECK(step(still) == 1);
    d.press(Action::shiftDown);
    CHECK(step(still) == 0);
    d.press(Action::shiftDown);
    CHECK(step(still) == -1);                                       // R (the box takes it only at a crawl)
    d.press(Action::shiftUp);
    CHECK(step(still) == 0);                                        // R to N
    // the keys: 1 to 6 D, N N at any speed, reverse R; no seek (no shift presses)
    const int ups = d.frame().shiftUp;
    d.press(Action::gear3);
    CHECK(step(moving) == 1);
    d.press(Action::neutral);
    CHECK(step(moving) == 0);                                       // N at speed: explicit
    d.press(Action::gear1);
    step(moving);
    d.press(Action::reverse);
    CHECK(step(still) == -1);
    for (int i = 0; i < 20; ++i)
        step(still);
    CHECK(d.frame().shiftUp == ups);                                // nothing seeks on a selector
    // another box: back to neutral and the presses' own rules (the lever 0 with the automatic clutch)
    d.setGearbox(Gearbox::shifts);
    d.press(Action::shiftUp);
    CHECK(step(still) == 0);
    CHECK(d.frame().shiftUp == ups + 1);
}

TEST_CASE("shell: the eighteen-speed: keys to the lever's positions, a shift the next of its eighteen, Z and X after it",
          "[shell][input]")
{
    Driver d;                                                       // the automatic clutch on: the lever passes
    d.setGearbox(Gearbox::eighteen);
    EngineSeen seen;
    const auto frame = [&] { return d.advance(0.1f, seen); };
    d.press(Action::gear3);
    CHECK(frame().lever == 3);
    d.press(Action::gear5);
    CHECK(frame().lever == 5);                                      // the L slot
    d.press(Action::gear6);
    CHECK(frame().lever == 5);                                      // 6: nothing
    d.press(Action::reverse);
    CHECK(frame().lever == -1);
    d.press(Action::neutral);
    CHECK(frame().lever == 0);
    // the shifts: from N the first gear (L, low range, low split), then each of the eighteen in order
    const auto gearOf = [&](const ControlsFrame& f) {
        const int r = (f.buttons & kButtonRange) != 0, s = (f.buttons & kButtonSplitter) != 0;
        if (f.lever == 5)
            return r == 0 ? 1 + s : 0;
        if (f.lever < 1 || f.lever > 4)
            return 0;
        return r == 0 ? 1 + 2 * f.lever + s : 9 + 2 * f.lever + s;
    };
    d.press(Action::shiftDown);
    CHECK(gearOf(frame()) == 0);                                    // nothing below neutral
    for (int g = 1; g <= 18; ++g)
    {
        d.press(Action::shiftUp);
        CHECK(gearOf(frame()) == g);
    }
    d.press(Action::shiftUp);
    CHECK(gearOf(frame()) == 18);                                   // refused at the top
    for (int g = 17; g >= 1; --g)
    {
        d.press(Action::shiftDown);
        CHECK(gearOf(frame()) == g);
    }
    d.press(Action::shiftDown);
    CHECK(gearOf(frame()) == 1);                                    // refused at the bottom
    // Z (the splitter) and X (the range) toggle from where the press left them: up to 12 (lever 1, high, split)
    for (int g = 2; g <= 12; ++g)
        d.press(Action::shiftUp);
    CHECK(gearOf(frame()) == 12);
    d.press(Action::splitter);
    CHECK(gearOf(frame()) == 11);
    d.press(Action::range);
    CHECK(gearOf(frame()) == 3);                                    // low range, low split, lever 1
    // nothing works the clutch: the pedal stays where the driver has it
    CHECK(frame().clutch == 0.0f);
    d.setGearbox(Gearbox::shifts);
    CHECK(frame().lever == 0);
}
