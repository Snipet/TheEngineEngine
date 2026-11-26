// tests/sim/calibration/na/StartTest.cpp: card N1's hold on a start on the engine dyno, as card N2's harness makes one
// (the key on, then a second of start, held warm, the dyno at 1300 rpm), on diesels and a boosted petrol engine, not
// only the naturally aspirated ones (the lead, 17:59: the calibration's tests ran green while every diesel was dead).
// The ECU went from cranking to running only above 550 rpm (the reference's petrol figure): a 15 L tractor cranks to
// some 540 in the starter's second, already firing, and any 1 % change (the charge's heating, 5fdc5ad) left it off.
// Under ecu.model 1 it runs above 0.75 of its own idle when that is lower (sim/ecu/ecu.fm).
#include "Player.h"
#include "tee/EnginePlayer.h"
#include "tee/Warm.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <string>

namespace
{
    // The mean brake torque over the last second of 6 s, and the state then
    void startOnDyno(const std::string& slug, double& torque, int& state)
    {
        struct
        {
            double sampleRate = 48000.0;
        } options;
        std::string errors, warnings;
        tee::EnginePlayer* p = &n1::player(options.sampleRate);   // (one compile a process: Player.h)
        tee::EngineBuild b;
        REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json"), b,
                               errors, warnings));
        tee::applyBuild(*p, b);
        auto& c = p->controls();
        c.mode = 2;
        c.dynoSpeed = float(1300.0 * M_PI / 30.0);
        const int block = 512;
        float l[block], r[block];
        double sum = 0.0;
        int n = 0;
        for (int i = 0; i < int(6.0 * options.sampleRate / block); ++i)
        {
            for (const tee::HeldValue& h : tee::warmEngine())
                p->instance().write(p->schema(), h.path, h.value, errors);
            const double t = i * block / options.sampleRate;
            c.ignition = t < 0.1 ? 1 : (t < 1.1 ? 2 : 1);
            c.throttle = t > 3.0 ? 1.0f : 0.0f;
            p->process(l, r, block);
            if (t >= 5.0)
                sum += p->telemetry(std::size_t(p->telemetryIndex("engine.torque"))), ++n;
        }
        torque = sum / n;
        state = int(p->telemetry(std::size_t(p->telemetryIndex("engine.state"))));
    }
} // namespace

namespace
{
    void startsAndPulls(std::initializer_list<const char*> slugs)
    {
        for (const char* slug : slugs)
        {
            double torque = 0.0;
            int state = 0;
            startOnDyno(slug, torque, state);
            std::printf("%s: state %d, %.0f N m at 1300 rpm, full throttle\n", slug, state, torque);
            CHECK(state == 2);
            CHECK(torque > 0.0);
        }
    }
} // namespace

// (the fast tier: the heaviest diesel, whose start the 550 rpm threshold left to chance, and the boosted petrol engine;
// a section of FastTest.cpp's one case: "a start on the engine dyno: the 15 L diesel and a boosted engine fire and pull")
void n1::dynoStartTractorAndTurbo()
{
    startsAndPulls({"tractor-15-inline-six", "turbo-four-20-gdi"});
}

TEST_CASE("a start on the engine dyno: diesels and a boosted engine fire and pull", "[nightly][calibration][na]")
{
    for (const char* slug : {"tractor-15-inline-six", "pickup-diesel-67-inline-six", "turbo-four-20-gdi", "economy-four-15"})
    {
        double torque = 0.0;
        int state = 0;
        startOnDyno(slug, torque, state);
        std::printf("%s: state %d, %.0f N m at 1300 rpm, full throttle\n", slug, state, torque);
        CHECK(state == 2);
        CHECK(torque > 0.0);
    }
}

// (a section of FastTest.cpp's one case: "a start on the engine dyno: the 2.0 common-rail diesel fires and pulls")
void n1::dynoStartCommonRail()
{
    // (it cranks to some 490 rpm in the starter's second, firing, and goes off as the key returns: the key's release
    // while cranking was Engine's: since card D's release rule, a firing engine let go from its start runs)
    double torque = 0.0;
    int state = 0;
    startOnDyno("diesel-20-common-rail", torque, state);
    CHECK(state == 2);
    CHECK(torque > 0.0);
}
