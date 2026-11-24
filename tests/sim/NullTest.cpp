// tests/sim/NullTest.cpp: the null test (docs/plan/p0-split.md): for each of the reference's presets (as builds) and
// each scenario that drives it by pedals, the simulator's output and the reference's, rendered in this process from the
// same inputs (Twin.h), differ by nothing, or by less than -100 dB under the signal.
#include "Twin.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>

namespace
{
    using namespace tee;

    struct Players
    {
        std::unique_ptr<EnginePlayer> engine;
        std::unique_ptr<RefPlayer> ref;
    };

    Players& players(double rate)
    {
        static std::map<double, Players> made;
        Players& p = made[rate];
        if (!p.engine)
        {
            p.engine = test::enginePlayer(rate);
            p.ref = test::refPlayer(rate);
        }
        return p;
    }

    // Plays one pair, prints its depth, and checks it (with `over`: these Spec values over the build's)
    void nullOf(const std::string& build, const std::string& scenario, double rate, int block, double seconds = 0.0,
                const std::map<std::string, double>& over = {})
    {
        Players& p = players(rate);
        Build b = test::engineBuild(*p.engine, build);
        for (const auto& [path, value] : over)
            b.values[path] = {value};
        const test::Null r = test::twin(*p.engine, *p.ref, b, test::engineScenario(scenario), block, seconds);
        std::printf("null %-13s %-10s %6.0f Hz /%4d: depth L %s R %s dB, worst difference %.3g, peak %.3f, %d start(s)\n",
                    build.c_str(), scenario.c_str(), rate, block,
                    std::isinf(r.depthLeft) ? "inf" : std::to_string(r.depthLeft).c_str(),
                    std::isinf(r.depthRight) ? "inf" : std::to_string(r.depthRight).c_str(), r.worst, r.peak, r.twin.keyDowns);
        INFO(build << " / " << scenario << " at " << rate << " Hz in blocks of " << block << ": depth " << r.depthLeft
                   << " and " << r.depthRight << " dB, worst " << r.worst);
        CHECK(r.peak > 0.05);          // it made a sound, so the comparison means something
        CHECK(r.twin.keyDowns >= 1);
        CHECK(r.depthLeft >= 100.0);
        CHECK(r.depthRight >= 100.0);
    }
} // namespace

TEST_CASE("null: every preset: free revs in neutral", "[sim][null]")
{
    for (const char* build : {"default", "v8", "v10", "large-diesel"})
        nullOf(build, "free-revs", 48000.0, 512);
}

TEST_CASE("null: every preset: a car through the gears on the paddles", "[sim][null]")
{
    for (const char* build : {"default", "v8", "v10", "large-diesel"})
        nullOf(build, "drive-car", 48000.0, 512);
}

TEST_CASE("null: every preset: a truck pulling away", "[sim][null]")
{
    for (const char* build : {"default", "v8", "v10", "large-diesel"})
        nullOf(build, "truck", 48000.0, 512);
}

TEST_CASE("null: every preset: on the bench by the throttle", "[sim][null]")
{
    for (const char* build : {"default", "v8", "v10", "large-diesel"})
        nullOf(build, "bench", 48000.0, 512);
}

TEST_CASE("null: every preset: the five views", "[sim][null]")
{
    for (const char* build : {"default", "v8", "v10", "large-diesel"})
        nullOf(build, "views", 48000.0, 512);
}

TEST_CASE("null: every preset: a start in its own air", "[sim][null]")
{
    for (const char* build : {"default", "v8", "v10", "large-diesel"})
        nullOf(build, "cold-start", 48000.0, 512);
}

TEST_CASE("null: every preset: forced induction", "[sim][null]")
{
    for (const char* build : {"default", "v8", "v10", "large-diesel"})
    {
        nullOf(build, "supercharged", 48000.0, 512);
        nullOf(build, "quad-turbo", 48000.0, 512);
    }
}

TEST_CASE("null: every layout, a wild cam, free revs", "[sim][null][layouts]")
{
    for (int layout = 0; layout <= 16; ++layout)
    {
        std::printf("layout %d: ", layout);
        nullOf("default", "free-revs", 48000.0, 512, 8.0,
               {{"engine.layout", layout}, {"valve.cam", 0.9}, {"engine.displacement", 0.002 + 0.0005 * layout}});
    }
}

TEST_CASE("null: other rates and blocks", "[sim][null]")
{
    nullOf("v8", "free-revs", 44100.0, 333, 8.0);
    nullOf("default", "drive-car", 96000.0, 64, 8.0);
}

TEST_CASE("null: quick, a pair of each kind", "[sim][null][quick]")
{
    nullOf("default", "free-revs", 48000.0, 512, 8.0);
    nullOf("large-diesel", "truck", 48000.0, 512, 8.0);
    nullOf("v10", "drive-car", 48000.0, 512, 8.0);
    nullOf("v8", "bench", 48000.0, 512, 8.0);
    nullOf("default", "views", 48000.0, 512);
    nullOf("v8", "cold-start", 48000.0, 512);
    nullOf("default", "supercharged", 48000.0, 512);
    nullOf("v10", "quad-turbo", 48000.0, 512);
}
