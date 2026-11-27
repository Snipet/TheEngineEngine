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
    CHECK(car.controls->driver().gearbox() == input::Gearbox::shifts);
    const int ups = car.controls->driver().frame().shiftUp;
    car.controls->key(input::key::kE, true);
    car.frames(6);
    car.controls->key(input::key::kE, false);
    car.frames(30);
    CHECK(car.controls->driver().frame().shiftUp == ups + 1);        // a press counted, as today
    CHECK(car.controls->driver().frame().lever == 0);                 // the lever not moved (the automatic clutch)
    CHECK(car.geared() == 1);                                         // the reference's car: first gear
}
