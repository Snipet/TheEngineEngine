// tests/sim/vehicle/SoakTest.cpp: card I's standalone test 7: a car standing on a 10 % grade on its brakes for a
// minute neither creeps nor rings; ten minutes of random pedals, levers, shifts and settings stay finite and bounded.
// And the model's cost a sample, for the handoff.
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

using namespace tee::vehicle;

TEST_CASE("hill: on a 10 % grade on the brakes for a minute: no creep, no ringing", "[vehicle][soak]")
{
    for (int drive = 0; drive < 3; ++drive)
    {
        for (double grade : {0.1, -0.1})
        {
            INFO("drive " << drive << ", grade " << grade);
            Rig rig;
            rig.set("car.spec.drive", drive);
            rig.set("car.env.grade", grade);
            rig.settle();
            rig.set("brake", 0.5);
            rig.seconds(2.0);
            const double x0 = rig.get("car.x");
            rig.clear();
            rig.seconds(58.0);
            const double creep = rig.get("car.x") - x0;
            const double swing = rig.get("xMax") - rig.get("xMin");
            std::printf("hill (drive %d, grade %+.1f): settled at %.3f mm, then moved %.6f mm in 58 s (range %.6f mm), "
                        "speed now %.2e m/s\n",
                        drive, grade, 1000.0 * x0, 1000.0 * creep, 1000.0 * swing, rig.get("car.v"));
            CHECK(std::fabs(x0) < 0.005);           // it leans on its tyres by millimetres
            CHECK(std::fabs(creep) < 1.0e-5);       // and stays there
            CHECK(swing < 1.0e-5);
            CHECK(std::fabs(rig.get("car.v")) < 1.0e-6);
            CHECK(rig.get("finite") == 1.0);
        }
    }
}

TEST_CASE("soak: ten minutes of random pedals, shifts and settings: finite and bounded", "[vehicle][soak]")
{
    double samples = 0.0;
    double seconds = 0.0;
    for (int box = 0; box < 5; ++box)
    {
        INFO("gearbox " << box);
        Rig rig;
        rig.set("car.spec.gearbox", box);
        rig.set("car.spec.drive", box % 3);
        rig.set("car.spec.tyre", (box * 3) % 10);
        rig.set("car.spec.rearPreload", box == 1 ? 80.0 : 0.0);
        rig.set("car.spec.frontKSpeed", box == 2 ? 20.0 : 0.0);
        rig.set("car.spec.centreLocked", box == 4 ? 1 : 0);
        rig.set("car.spec.liveAxle", box == 3 ? 1 : 0);
        rig.set("car.spec.clA", 1.0);
        rig.settle();
        int up = 0, down = 0, strip = 0;
        const auto t0 = std::chrono::steady_clock::now();
        for (int step = 0; step < 4 * 120; ++step)       // two minutes a gearbox, a change every quarter second
        {
            const float r = rig.random();
            rig.set("throttle", rig.random() < 0.3f ? 0.0 : rig.random());
            rig.set("brake", rig.random() < 0.8f ? 0.0 : rig.random());
            rig.set("clutch", rig.random() < 0.7f ? 0.0 : rig.random());
            rig.set("handbrake", rig.random() < 0.95f ? 0.0 : rig.random());
            rig.set("buttons", rig.random() < 0.95f ? 0 : 1);
            if (r < 0.1f)
                rig.set("lever", static_cast<int>(rig.random() * 8.0f) - 1);
            else if (r < 0.2f)
                rig.set("up", ++up);
            else if (r < 0.28f)
                rig.set("down", ++down);
            else if (r < 0.29f)
                rig.set("strip", ++strip);
            else if (r < 0.3f)
            {
                rig.set("car.env.surface", static_cast<int>(rig.random() * 5.0f));
                rig.set("car.env.surfaceLeft", static_cast<int>(rig.random() * 6.0f) - 1);
                rig.settle();
            }
            else if (r < 0.31f)
                rig.set("mode", rig.random() < 0.5f ? DriveMode : DynoChassis);
            // a stalled engine is started again, as a driver would
            if (rig.get("running") == 0.0)
            {
                rig.set("running", 1);
                rig.set("w", 85.0);
            }
            rig.seconds(0.25);
            REQUIRE(rig.get("finite") == 1.0);
        }
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        samples += rig.get("wn");
        seconds += s;
        std::printf("soak (gearbox %d): speed up to %.1f m/s, crank %.1f to %.1f rad/s, shafts %.0f to %.0f N m, road "
                    "peak %.3f, tyres up to %.0f K\n",
                    box, rig.get("vMax"), rig.get("wMin"), rig.get("wMax"), rig.get("shaftMin"), rig.get("shaftMax"),
                    rig.get("roadPeak"), rig.get("car.tyre[2].temp"));
        CHECK(rig.get("finite") == 1.0);
        CHECK(rig.get("vMax") < 120.0);
        CHECK(rig.get("wMax") < 1.2 * 680.0);
        CHECK(rig.get("wMin") >= 0.0);
        CHECK(std::fabs(rig.get("shaftMin")) < 1.0e5);
        CHECK(std::fabs(rig.get("shaftMax")) < 1.0e5);
        CHECK(rig.get("roadPeak") < 10.0);
        for (int i = 0; i < 4; ++i)
            CHECK(rig.get("car.tyre[" + std::to_string(i) + "].temp") < 700.0);
    }
    std::printf("soak: %.0f s simulated at 96 kHz in %.1f s: %.1f ns a sample, the rig's made-up engine included\n",
                samples / 96000.0, seconds, 1e9 * seconds / samples);
}
