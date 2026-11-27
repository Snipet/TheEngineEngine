// tests/shell/GearboxDrivingTest.cpp: every gearbox kind driven by hand through the application's own path: a shipped
// build of each kind opened as GARAGE opens it (files::BuildFile), the keys and the wheel's paddles into the Controls'
// Driver, its frame into the simulator (SimAdapter over the shared program), the gear read back from the car's
// telemetry. The owner's fault, 27 Nov ("shifting in any form/input does not work for any transmission other than a
// manual"): the automatic, the CVT and the eighteen-speed stayed in neutral with the automatic clutch on, which zeroed
// the lever they read (13 shipped builds). Per kind: from a standstill the shift-up key gives a forward gear (the
// automatic's and the CVT's D; the eighteen-speed's first, with its clutch down, as its box has no synchromesh); the
// wheel's right paddle the same, its shifter out of every gate, and the gear stays. The reference's vehicle (car.model
// 0, which reads no `gearbox`) is driven exactly as before.
#include "files/BuildFiles.h"
#include "input/Controls.h"
#include "input/Keys.h"
#include "input/g29/G29Input.h"
#include "support/SharedSim.h"
#include "tee/Build.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>

#include <string>
#include <vector>

using namespace tee;

namespace
{
    struct Shipped
    {
        const char* build;      // under content/builds
        const char* kind;
        bool clutch;            // its gear goes in at rest with the clutch down (the eighteen-speed)
    };
    // a shipped build of every kind (the lead's count, 27 Nov: Automatic 7, Dual clutch 7, Eighteen-speed 5, Sequential 1,
    // CVT 1, the rest Manual)
    constexpr Shipped kShipped[] = {
        { "cars/rwd-v8-coupe.json", "manual", false },
        { "archetypes/inline-five-turbo-rally.json", "sequential", false },
        { "cars/awd-turbo-six.json", "dual clutch", false },
        { "diesel-pickup-tuned.json", "automatic", false },
        { "archetypes/v6-35-na.json", "automatic", false },
        { "archetypes/economy-four-15.json", "CVT", false },
        { "truck-15l-six.json", "eighteen-speed", true },
        { "archetypes/tractor-15-inline-six.json", "eighteen-speed", true },
    };

    // The simulator over a build, the engine started, driven as the window does.
    struct Car
    {
        std::unique_ptr<player::SimAdapter> sim;
        std::unique_ptr<input::Controls> controls;
        int gear = -1, state = -1;
        std::vector<float> l = std::vector<float>(800), r = std::vector<float>(800);

        // `build`: a shipped build's file (empty: the simulator's defaults); `edit` then changes the build before the start
        template <class Edit>
        Car(const std::string& build, Edit edit)
        {
            std::string errors;
            sim = test::sharedAdapter(errors);
            INFO(errors);
            REQUIRE(sim != nullptr);
            sim->prepare(48000.0, 800);
            REQUIRE(sim->waitReady(600.0));
            sim->frame();
            if (!build.empty())
            {
                files::BuildFile doc(sim->build());
                std::vector<std::string> warnings;
                std::string error;
                const bool opened = doc.open(std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / build,
                                             BuildPart::Whole, warnings, error);
                INFO(build << ": " << error);
                REQUIRE(opened);
            }
            edit(sim->build());
            sim->frame();
            controls = std::make_unique<input::Controls>(*sim, *sim);
            gear = sim->findValue(telemetry::kGear);
            state = sim->findValue(telemetry::kState);
            REQUIRE(gear >= 0);
            frames(10);
            controls->key(input::key::kReturn, true);
            frames(60);
            controls->key(input::key::kReturn, false);
            frames(90);
            REQUIRE(sim->value(state) == 2.0);                       // running
        }
        explicit Car(const std::string& build) : Car(build, [](Build&) {}) {}

        // n frames of a sixtieth: the controls, then the audio that carries them
        void frames(int n)
        {
            for (int i = 0; i < n; ++i)
            {
                controls->tick(1.0f / 60.0f);
                sim->frame();
                sim->process(l.data(), r.data(), 800);
            }
        }

        int geared() const { return static_cast<int>(sim->value(gear)); }
    };
}

TEST_CASE("gearbox: each shipped kind takes a forward gear from a standstill by the shift-up key",
          "[shell][sim][sharedsim]")
{
    for (const Shipped& k : kShipped)
    {
        INFO(k.build << " (" << k.kind << ")");
        Car car(k.build);
        CHECK(car.geared() == 0);                                     // at rest in neutral
        // the status line says what the box asks of the driver: on an eighteen-speed, the clutch is theirs
        CHECK((car.controls->services().status().find("the clutch is yours") != std::string::npos) == k.clutch);
        CHECK((car.controls->services().status().find("start in L (key 5)") != std::string::npos) == k.clutch);
        if (k.clutch)
        {
            car.controls->key(input::key::kLeftShift, true);
            car.frames(30);
        }
        car.controls->key(input::key::kE, true);
        car.frames(6);
        car.controls->key(input::key::kE, false);
        car.frames(30);
        CHECK(car.geared() >= 1);
        if (k.clutch)
        {
            car.controls->key(input::key::kLeftShift, false);
            car.frames(30);
        }
        car.frames(60);
        CHECK(car.geared() >= 1);                                     // and it stays in
    }
}

TEST_CASE("gearbox: each shipped kind takes a forward gear from a standstill by the wheel's paddle, and keeps it",
          "[shell][sim][sharedsim]")
{
    for (const Shipped& k : kShipped)
    {
        INFO(k.build << " (" << k.kind << ")");
        Car car(k.build);
        input::g29::Input wheel;
        input::g29::State s;                                          // the shifter out of every gate from the first frame
        if (k.clutch)
            s.v14[3] = input::g29::kMost;                             // the clutch pedal down
        const auto frame = [&] {
            wheel.apply(s, true, car.controls->driver());
            car.frames(1);
        };
        for (int i = 0; i < 30; ++i)
            frame();
        s.buttons = 1u << input::g29::kBitRightPaddle;
        for (int i = 0; i < 6; ++i)
            frame();
        s.buttons = 0;
        for (int i = 0; i < 30; ++i)
            frame();
        CHECK(car.geared() >= 1);
        s.v14[3] = 0;                                                 // the clutch up
        for (int i = 0; i < 120; ++i)
            frame();
        CHECK(car.geared() >= 1);                                     // still in, frame after frame
        if (std::string(k.kind) == "automatic" || std::string(k.kind) == "CVT")
            CHECK(car.controls->driver().frame().lever == 1);         // D held: the shifter out of every gate asks nothing
    }
}

TEST_CASE("gearbox: the reference's vehicle (car.model 0) is driven as before, whatever its gearbox field says",
          "[shell][sim][sharedsim]")
{
    // the simulator's defaults with the gearbox field at Automatic: model 0 reads no gearbox, so the presses are shifts
    Car car("", [](Build& b) {
        REQUIRE(b.set("spec.car.model", 0));
        REQUIRE(b.set("spec.car.gearbox", 3));
    });
    CHECK(car.controls->driver().gearbox() == input::Gearbox::manual);
    const int ups = car.controls->driver().frame().shiftUp;
    car.controls->key(input::key::kE, true);
    car.frames(6);
    car.controls->key(input::key::kE, false);
    car.frames(30);
    CHECK(car.controls->driver().frame().shiftUp == ups + 1);        // a press counted, as today
    CHECK(car.controls->driver().frame().lever == 0);                 // the lever not moved (the automatic clutch)
    CHECK(car.geared() == 1);                                         // the reference's car: first gear
}

TEST_CASE("gearbox: the automatic clutch's line per kind, and a manual with it off and keys only says which key the clutch is",
          "[shell][sim][sharedsim]")
{
    const struct
    {
        const char* build;
        const char* says;
    } lines[] = {
        { "cars/rwd-v8-coupe.json", "Manual: the shifter is the lever" },
        { "archetypes/inline-five-turbo-rally.json", "Sequential: it works its own clutch" },
        { "cars/awd-turbo-six.json", "Dual clutch: it works its own clutch" },
        { "diesel-pickup-tuned.json", "Automatic: its selector" },
        { "archetypes/economy-four-15.json", "CVT: its selector" },
        { "truck-15l-six.json", "Eighteen-speed: the lever is the shifter" },
    };
    for (const auto& l : lines)
    {
        INFO(l.build);
        Car car(l.build);
        CHECK(car.controls->clutchLine().find(l.says) != std::string::npos);
        CHECK(car.controls->clutchLine().size() <= 90);                 // the SETTINGS row's small line
    }
    // the manual with the automatic clutch off: the line says so, and with keys only the status line names the key
    Car car("cars/rwd-v8-coupe.json");
    input::DriverSettings off = car.controls->driver().settings();
    off.autoClutch = false;
    car.controls->driver().setSettings(off);
    car.frames(2);
    CHECK(car.controls->clutchLine().find("the clutch is yours") != std::string::npos);   // Yours
    CHECK(car.controls->services().status().find("clutch: yours (Left Shift)") != std::string::npos);
    // a wheel's pedals read: its clutch pedal is there, nothing to say
    car.controls->driver().setWheel(0.0f, 0.0f, 0.0f);
    CHECK(car.controls->services().status().find("clutch: yours") == std::string::npos);
    // the H-lever with the clutch down: a gear key puts it in first
    car.controls->driver().release(input::Source::wheel);
    car.controls->key(input::key::kLeftShift, true);
    car.frames(30);
    car.controls->driver().requestGear(1);
    car.frames(30);
    CHECK(car.geared() == 1);
}

namespace
{
    // A G29 on the car: its pedals and shifter injected a frame at a time, as RacingWheel::state() gives them.
    struct Wheel
    {
        Car& car;
        input::g29::Input input;
        input::g29::State s;
        explicit Wheel(Car& c, bool clutchAxis = true) : car(c) { input.setClutchAxis(clutchAxis); }
        void frames(int n)
        {
            for (int i = 0; i < n; ++i)
            {
                input.apply(s, true, car.controls->driver());
                car.frames(1);
            }
        }
        void clutch(bool down) { s.v14[3] = down ? input::g29::kMost : 0; }
        // frames until car.gear is `g` (at most `most`): the count, or -1
        int until(int g, int most)
        {
            for (int f = 0; f < most; ++f)
            {
                if (car.geared() == g)
                    return f;
                frames(1);
            }
            return -1;
        }
    };

    void setMode(Car& car, input::ClutchMode m)
    {
        input::DriverSettings s = car.controls->driver().settings();
        input::setClutchMode(s, m);
        car.controls->driver().setSettings(s);
    }
}

TEST_CASE("gearbox: a G29's H-shifter on a manual is the lever at once by default; Automatic seeks (the owner's delay)",
          "[shell][sim][sharedsim]")
{
    int took[2] = { -1, -1 };
    for (int i = 0; i < 2; ++i)
    {
        Car car("cars/rwd-v8-coupe.json");
        setMode(car, i == 0 ? input::ClutchMode::shifter : input::ClutchMode::automatic);
        Wheel wheel(car);
        wheel.clutch(true);
        wheel.frames(30);
        wheel.s.gear = 1;                                           // the shifter into first's gate
        took[i] = wheel.until(1, 300);
        INFO((i == 0 ? "shifter" : "automatic") << ": frames from the gate to car.gear 1: " << took[i]);
        CHECK(took[i] >= 0);
        // second, through neutral
        wheel.s.gear = 0;
        wheel.frames(20);
        wheel.s.gear = 2;
        const int second = wheel.until(2, 300);
        INFO("to second: " << second);
        CHECK(second >= 0);
        std::printf("  gate to gear, %s: first %d frames, second %d frames (60 a second)\n",
                    i == 0 ? "the shifter's lever" : "automatic (sought)", took[i], second);
    }
    CHECK(took[0] < took[1]);                                       // the lever is quicker than the seek
    CHECK(took[0] <= 3);                                            // within a few frames: no delay of its own
}

TEST_CASE("gearbox: with the shifter's lever a manual stalls when the clutch is let up in first at rest with no throttle",
          "[shell][sim][sharedsim]")
{
    Car car("cars/rwd-v8-coupe.json");
    Wheel wheel(car);
    wheel.clutch(true);
    wheel.frames(30);
    wheel.s.gear = 1;
    wheel.frames(30);
    REQUIRE(car.geared() == 1);
    wheel.clutch(false);                                            // let up at once, no throttle
    bool stalled = false;
    for (int f = 0; f < 240 && !stalled; ++f)
    {
        wheel.frames(1);
        stalled = car.sim->value(car.state) != 2.0;
    }
    std::printf("  clutch let up in first at rest, no throttle: %s\n", stalled ? "stalled" : "still running after 4 s");
    CHECK(stalled);
}

TEST_CASE("gearbox: the shifter's edges: in a gate before the start, a gate refused at speed, a wheel with no clutch pedal",
          "[shell][sim][sharedsim]")
{
    {
        // in a gate before the start: in gear, and the status line asks for the clutch (nothing hidden)
        Car car("cars/rwd-v8-coupe.json");
        Wheel wheel(car);
        wheel.clutch(true);
        wheel.frames(10);
        wheel.s.gear = 1;
        wheel.frames(20);
        wheel.clutch(false);
        wheel.frames(10);
        INFO(car.controls->services().status());
        CHECK((car.geared() == 1 || car.sim->value(car.state) != 2.0));
        if (car.sim->value(car.state) == 2.0 && car.geared() == 1)
            CHECK(car.controls->services().status().find("in gear (the shifter): the clutch") != std::string::npos);
    }
    {
        // a gate taken at speed with the clutch up: the box refuses it, and the status line says it did not go in
        Car car("cars/rwd-v8-coupe.json");
        Wheel wheel(car);
        wheel.clutch(true);
        wheel.frames(20);
        wheel.s.gear = 1;
        wheel.frames(10);
        wheel.s.v14[1] = input::g29::kMost / 3;                      // some throttle
        for (int k = 0; k < 40; ++k)                                 // the clutch let up over two thirds of a second
        {
            wheel.s.v14[3] = input::g29::kMost - k * input::g29::kMost / 40;
            wheel.frames(1);
        }
        wheel.clutch(false);
        wheel.frames(120);
        REQUIRE(car.sim->value(car.state) == 2.0);
        wheel.s.v14[1] = 0;
        wheel.s.gear = 0;
        wheel.frames(10);
        wheel.s.gear = 4;                                            // fourth, the clutch up, the revs far off
        bool said = false;
        for (int f = 0; f < 30 && !said; ++f)
        {
            wheel.frames(1);
            said = car.controls->services().status().find("did not go in") != std::string::npos;
        }
        INFO("gear " << car.geared());
        CHECK(said);
        CHECK(car.geared() != 4);
    }
    {
        // a wheel with no clutch pedal: the gate sought (the automatic's way), and said
        Car car("cars/rwd-v8-coupe.json");
        Wheel wheel(car, false);
        wheel.s.gear = 1;
        CHECK(wheel.until(1, 300) >= 0);
        CHECK(car.controls->services().status().find("no clutch pedal") != std::string::npos);
    }
}
