// tests/sim/diesel/EighteenTest.cpp: card D2's eighteen-speed (sim/vehicle/roadranger.fm) through card I's vehicle
// (spec.car.gearbox "Eighteen-speed") in the whole engine: the 36 t truck six, 300 K, 101325 Pa.
#include "Diesel.h"

#include <cstdio>

using d2::Engine;

namespace
{
    const double kRatio[18] = {14.40, 12.29, 8.56, 7.30, 6.05, 5.16, 4.38, 3.74, 3.20, 2.73, 2.29, 1.95, 1.62, 1.38, 1.17, 1.00, 0.86, 0.73};

    void driving(Engine& e)
    {
        e.start();
        auto& c = e.ctl();
        c.mode = 1;
        c.lever = 0;
        c.clutch = 1.0f;
        c.throttle = 0.0f;
        REQUIRE(e.run(1.0));
    }

    // In neutral, clutch up: the pedal worked to bring the crank to rpm (a driver's foot: proportional and integral),
    // until it has been within `tol` rpm for a tenth of a second or the time is up; `want` recomputed as the truck rolls
    template <typename Want>
    void revsTo(Engine& e, Want want, double tol, double seconds)
    {
        double base = 0.05;
        int inside = 0;
        for (int i = 0; i < static_cast<int>(seconds / 0.01) && inside < 10; ++i)
        {
            const double err = want() - e.tel("engine.speed") / 0.10471976;
            base = std::min(std::max(base + 0.00015 * err, 0.0), 0.6);
            e.ctl().throttle = static_cast<float>(std::min(std::max(base + 0.0015 * err, 0.0), 1.0));
            REQUIRE(e.run(0.01));
            inside = std::fabs(err) < tol ? inside + 1 : 0;
        }
    }
} // namespace

TEST_CASE("truck eighteen-speed: every gear selected at rest through the vehicle", "[diesel][truck][eighteen]")
{
    // at rest, the clutch floored (its brake stops the input shaft): the lever, the range and the splitter give 1 to 18
    Engine e("truck-15l-six");
    driving(e);
    int seen = 0;
    for (int range : {0, 1})
        for (int lever : {5, 1, 2, 3, 4})
        {
            if (range == 1 && lever == 5)
                continue;
            for (int split : {0, 1})
            {
                const int expect = lever == 5 ? 1 + split : (range == 0 ? 1 + 2 * lever + split : 9 + 2 * lever + split);
                e.ctl().lever = 0;
                REQUIRE(e.run(0.1));
                e.ctl().buttons = (split ? 16 : 0) + (range ? 32 : 0);
                REQUIRE(e.run(0.05));
                e.ctl().lever = lever;
                REQUIRE(e.run(0.2));
                CHECK(e.tel("car.gear") == expect);
                if (e.tel("car.gear") == expect)
                    ++seen;
            }
        }
    CHECK(seen == 18);
}

TEST_CASE("truck eighteen-speed: rolling, a gear floated in at matched revs goes in silently; 300 rpm off it grinds and is refused",
          "[diesel][truck][eighteen][sound]")
{
    Engine e("truck-15l-six");
    e.spec("sound.model", 1);
    driving(e);
    // away in 3 (lever 1, low split), the clutch let in, then up to some 1500 rpm
    e.ctl().lever = 1;
    REQUIRE(e.run(0.3));
    REQUIRE(e.tel("car.gear") == 3.0);
    e.ctl().throttle = 0.5f;
    for (int i = 0; i < 30; ++i)
    {
        e.ctl().clutch = std::max(0.0f, 1.0f - 0.04f * static_cast<float>(i));
        REQUIRE(e.run(0.1));
    }
    e.ctl().throttle = 1.0f;
    for (int i = 0; i < 200 && e.tel("engine.speed") / 0.10471976 < 1500.0; ++i)
        REQUIRE(e.run(0.05));
    const double v = e.tel("car.speed");
    std::printf("in 3 at %.0f rpm, %.1f km/h\n", e.tel("engine.speed") / 0.10471976, v * 3.6);
    REQUIRE(v > 1.0);
    // a float into 5 (lever 2): off the pedal, out to neutral, the revs to 5's at this road speed, the lever in
    const double fd = e.get("car.m1.box.fd"), r = e.get("car.m1.radius");     // (the vehicle's own: 3.55, 0.5)
    e.ctl().throttle = 0.0f;
    e.ctl().lever = 0;
    REQUIRE(e.run(0.3));
    auto want5 = [&] { return e.tel("car.speed") / r * fd * kRatio[4] / 0.10471976; };
    revsTo(e, want5, 25.0, 3.0);
    std::printf("before the lever: crank %.0f rpm, 5 wants %.0f rpm, shaft %.1f rad/s, output %.2f rad/s\n", e.tel("engine.speed") / 0.10471976,
                e.tel("car.speed") / r * fd * kRatio[4] / 0.10471976, e.get("car.m1.rr.wShaft"), e.tel("car.speed") / r * fd);
    const double grinds0 = e.get("car.m1.rr.grinds");
    e.recording = true;
    e.ctl().lever = 2;
    for (int i = 0; i < 20 && e.tel("car.gear") != 5.0; ++i)
        REQUIRE(e.run(0.01));
    std::printf("float into 5: gear %.0f, grinds %.0f -> %.0f\n", e.tel("car.gear"), grinds0, e.get("car.m1.rr.grinds"));
    CHECK(e.tel("car.gear") == 5.0);
    CHECK(e.get("car.m1.rr.grinds") == grinds0);
    // a miss: out to neutral, the revs 300 rpm over 7's (lever 3), the lever pushed in and held
    e.ctl().throttle = 0.0f;
    e.ctl().lever = 0;
    REQUIRE(e.run(0.3));
    const double grinds1 = e.get("car.m1.rr.grinds");
    auto want7 = [&] { return e.tel("car.speed") / r * fd * kRatio[6] / 0.10471976 + 300.0; };
    revsTo(e, want7, 25.0, 3.0);
    e.ctl().lever = 3;
    int refused = 0;
    for (int i = 0; i < 30; ++i)
    {
        REQUIRE(e.run(0.01));
        if (e.tel("car.shift") == 3.0)
            ++refused;
    }
    std::printf("300 rpm off 7: gear %.0f, refused in %d of 30 looks, grinds %.0f, grind now %.2f\n", e.tel("car.gear"), refused,
                e.get("car.m1.rr.grinds"), e.get("car.m1.rr.grinding"));
    CHECK(e.tel("car.gear") == 0.0);
    CHECK(refused > 10);
    CHECK(e.get("car.m1.rr.grinds") > grinds1);
}
