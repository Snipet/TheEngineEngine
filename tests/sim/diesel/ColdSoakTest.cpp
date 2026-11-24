// tests/sim/diesel/ColdSoakTest.cpp: card D2: cold starts of the truck six at 253 K and 233 K (the whole-program tester
// found a petrol V8 that would not start at 253 K with the new fuel and ECU models together: the diesel checked), and a
// soak: random pedals and switches, every D2 field moving, finite and bounded.
#include "Diesel.h"

#include <cstdio>
#include <random>

using d2::Engine;

namespace
{
    // seconds of cranking until it runs (state 2) and stays running two seconds; -1: it did not within the limit
    double startAt(double kelvin, int governor, int fuelModel, int ecuModel, double limit)
    {
        Engine e("truck-15l-six");
        e.spec("env.temperature", kelvin);
        e.spec("env.pressure", 101325.0);
        e.spec("ecu.governor", governor);
        e.spec("fuel.model", fuelModel);
        e.spec("ecu.model", ecuModel);
        auto& c = e.ctl();
        c.mode = 0;
        c.throttle = 0.0f;
        c.ignition = 2;
        double t = 0.0;
        while (t < limit && e.tel("engine.state") != 2.0)
        {
            REQUIRE(e.run(0.1));
            t += 0.1;
        }
        c.ignition = 1;
        if (e.tel("engine.state") != 2.0)
            return -1.0;
        REQUIRE(e.run(2.0));
        return e.tel("engine.state") == 2.0 ? t : -1.0;
    }
} // namespace

TEST_CASE("diesel cold: the truck six starts at 253 K with every model, and says what it does at 233 K", "[diesel][cold]")
{
    for (int g : {0, 1, 2})
        for (int m : {0, 1})
        {
            const double at253 = startAt(253.15, g, m, m, 15.0);
            const double at233 = startAt(233.15, g, m, m, 15.0);
            std::printf("governor %d, fuel and ecu models %d: runs after %.1f s cranking at 253 K; at 233 K %s%.1f s\n", g, m, at253,
                        at233 < 0.0 ? "no start in 15 s " : "", at233 < 0.0 ? 15.0 : at233);
            CHECK(at253 > 0.0);
            CHECK(at253 < 10.0);
        }
}

TEST_CASE("diesel soak: random pedals, brakes and governors for a while stay finite and bounded", "[diesel][soak]")
{
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    Engine e("truck-15l-six");
    e.start();
    auto& c = e.ctl();
    double peak = 0.0;
    for (int step = 0; step < 80; ++step)
    {
        c.throttle = static_cast<float>(u(rng) < 0.3 ? 0.0 : u(rng));
        c.mode = u(rng) < 0.5 ? 2 : 0;
        c.dynoSpeed = static_cast<float>((700.0 + 1600.0 * u(rng)) * 0.10471976);
        e.spec("ecu.brakeSwitch", static_cast<int>(u(rng) * 4.0));
        e.spec("ecu.exBrakeSwitch", u(rng) < 0.5 ? 0 : 1);
        e.spec("ecu.exhaustBrake", static_cast<int>(u(rng) * 3.0));
        e.spec("ecu.governor", static_cast<int>(u(rng) * 3.0));
        e.spec("ecu.afc", u(rng) < 0.3 ? 0.0 : 0.8);
        e.spec("valve.auxOpen", 680.0 + 30.0 * u(rng));
        if (e.tel("engine.state") == 0.0)
        {
            c.ignition = 2;
            REQUIRE(e.run(1.5));
            c.ignition = 1;
        }
        REQUIRE(e.run(0.5));
        peak = std::max(peak, e.tel("sound.peak"));
        REQUIRE(std::isfinite(e.tel("engine.speed")));
        CHECK(e.tel("engine.speed") < 400.0);
        CHECK(e.tel("ecu.backPressure") < 600001.0);
    }
    std::printf("soak: 40 s of random pedals, switches and governors: peak %.2f\n", peak);
    CHECK(peak <= 1.0);
}
