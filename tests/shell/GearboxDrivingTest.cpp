// tests/shell/GearboxDrivingTest.cpp: every gearbox kind driven by hand through the application's own path: the
// keys and the wheel's paddles into the Controls' Driver, its frame into the simulator (SimAdapter over the shared
// program), the gear read back from the car's telemetry. The owner's fault, 27 Nov ("shifting in any form/input does
// not work for any transmission other than a manual"): the automatic, the CVT and the eighteen-speed stayed in neutral
// with the automatic clutch on, which zeroed the lever they read. Per kind: from a standstill the shift-up key gives a
// forward gear (the automatic's and the CVT's D); the wheel's right paddle the same.
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
    struct Kind
    {
        int gearbox;
        const char* name;
    };
    // sim/vehicle/vspec.fm's VEH_*
    constexpr Kind kKinds[] = { { 0, "manual" },    { 1, "sequential" }, { 2, "dual clutch" },
                                { 3, "automatic" }, { 4, "CVT" },        { 5, "eighteen-speed" } };

    // The simulator with the vehicle model on and a gearbox kind, the engine started, driven as the window does.
    struct Car
    {
        std::unique_ptr<player::SimAdapter> sim;
        std::unique_ptr<input::Controls> controls;
        int gear = -1, state = -1;
        std::vector<float> l = std::vector<float>(800), r = std::vector<float>(800);

        explicit Car(int gearbox)
        {
            std::string errors;
            sim = test::sharedAdapter(errors);
            INFO(errors);
            REQUIRE(sim != nullptr);
            sim->prepare(48000.0, 800);
            REQUIRE(sim->waitReady(600.0));
            sim->frame();
            REQUIRE(sim->build().set("spec.car.model", 1));
            REQUIRE(sim->build().set("spec.car.gearbox", gearbox));
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

TEST_CASE("gearbox: each kind takes a forward gear from a standstill by the shift-up key", "[shell][sim][sharedsim]")
{
    for (const Kind& k : kKinds)
    {
        INFO(k.name);
        Car car(k.gearbox);
        CHECK(car.geared() == 0);                                     // at rest in neutral
        // the eighteen-speed's main box has no synchromesh: at rest its gear goes in with the clutch down (its key)
        const bool clutch = k.gearbox == 5;
        if (clutch)
        {
            car.controls->key(input::key::kLeftShift, true);
            car.frames(30);
        }
        car.controls->key(input::key::kE, true);
        car.frames(6);
        car.controls->key(input::key::kE, false);
        car.frames(30);
        CHECK(car.geared() >= 1);
        if (clutch)
        {
            car.controls->key(input::key::kLeftShift, false);
            car.frames(30);
            CHECK(car.geared() >= 1);                                 // still in, the clutch let up
        }
    }
}

TEST_CASE("gearbox: each kind takes a forward gear from a standstill by the wheel's right paddle", "[shell][sim][sharedsim]")
{
    for (const Kind& k : kKinds)
    {
        INFO(k.name);
        Car car(k.gearbox);
        input::g29::Input wheel;
        input::g29::State s;
        if (k.gearbox == 5)
            s.v14[3] = input::g29::kMost;                             // the eighteen-speed: the clutch pedal down
        wheel.apply(s, true, car.controls->driver());
        car.frames(30);
        s.buttons = 1u << input::g29::kBitRightPaddle;
        wheel.apply(s, true, car.controls->driver());
        car.frames(6);
        s.buttons = 0;
        wheel.apply(s, true, car.controls->driver());
        car.frames(30);
        CHECK(car.geared() >= 1);
    }
}
