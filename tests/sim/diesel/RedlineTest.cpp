// tests/sim/diesel/RedlineTest.cpp: card D2, X-057: a governed build's redline is a speed its engine reaches. The
// vehicle reads its shift points and its auto final drive off the redline; a governed diesel never passes its governed
// high idle, so a redline above it is a speed no gear is ever left at.
#include "Diesel.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using d2::Engine;

TEST_CASE("diesel builds: the tuned pickup in D at full pedal leaves first", "[diesel][builds]")
{
    // X-057 (the lead's, 27 Nov): diesel-pickup-tuned (automatic, six gears) stayed in first for 40 s at full pedal: its
    // governor held 2226 rpm, its automatic's full-pedal upshift waited for 2775 (0.85 of the reference's 3265 rpm redline
    // for 6.7 L, which the build did not state). With its rated speed the class's (2900 rpm), its redline at its high
    // idle and the archetype's drivetrain: the 1-2 at 37 km/h after 4.6 s here (the archetype pickup: 36 km/h, 4.8 s)
    Engine e("diesel-pickup-tuned");
    e.spec("env.temperature", 300.0);
    e.spec("env.pressure", 101325.0);
    e.start();
    auto& c = e.ctl();
    c.mode = 1;
    c.lever = 0;
    c.throttle = 0.0f;
    c.brake = 0.0f;
    REQUIRE(e.run(1.0));
    c.lever = 1;    // D
    REQUIRE(e.run(0.5));
    c.throttle = 1.0f;
    double at = -1.0, v = 0.0, rpm = 0.0;
    for (int i = 0; i < 200 && at < 0.0; ++i)
    {
        REQUIRE(e.run(0.1));
        v = e.tel("car.speed") * 3.6;
        if (e.tel("car.gear") >= 2.0)
            at = 0.1 * (i + 1);
        rpm = e.tel("engine.speed") / 0.10471976;
    }
    std::printf("diesel-pickup-tuned in D, floored: second gear %s%.1f s%s (%.0f km/h); the engine at %.0f rpm, the redline %.0f\n",
                at < 0.0 ? "not reached in " : "after ", at < 0.0 ? 20.0 : at, at < 0.0 ? "" : "", v,
                rpm, e.tel("engine.redline") / 0.10471976);
    CHECK(at > 0.0);
    CHECK(at < 8.0);
}

namespace
{
    // the shipped builds whose ECU states a diesel governor (content/builds and its archetypes; a part's tune is not a build)
    std::vector<std::string> governedBuilds()
    {
        namespace fs = std::filesystem;
        const fs::path root = fs::path(TEE_SOURCE_DIR) / "content" / "builds";
        std::vector<std::string> out;
        for (const auto& entry : fs::recursive_directory_iterator(root))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".json")
                continue;
            const fs::path rel = fs::relative(entry.path(), root);
            const std::string first = rel.begin()->string();
            if (first != "archetypes" && rel.has_parent_path())
                continue;
            std::ifstream in(entry.path());
            std::stringstream ss;
            ss << in.rdbuf();
            const std::string text = ss.str();
            if (text.find("\"governor\": \"Electronic\"") == std::string::npos &&
                text.find("\"governor\": \"Mechanical\"") == std::string::npos)
                continue;
            std::string name = rel.generic_string();
            name = name.substr(0, name.size() - 5);
            out.push_back(name);
        }
        std::sort(out.begin(), out.end());
        return out;
    }
} // namespace

namespace
{
    // a governed build's redline against its governed high idle (rpm both), read as the vehicle reads it
    void redlineAgainstHighIdle(const std::string& name, double& red, double& high)
    {
        Engine e(name);
        e.spec("env.temperature", 300.0);
        e.spec("env.pressure", 101325.0);
        e.start();
        REQUIRE(e.run(0.2));
        red = e.tel("engine.redline") / 0.10471976;   // (rad/s, as the vehicle reads it)
        high = e.get("hd.highIdle") / 0.10471976;
        std::printf("%-40s redline %5.0f rpm, governed high idle %5.0f rpm\n", name.c_str(), red, high);
    }

    bool shiftsByRedline(const std::string& name)
    {
        // an automatic shifts by its redline, and auto gearing derives the final drive from it
        std::ifstream in(std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / (name + ".json"));
        std::stringstream ss;
        ss << in.rdbuf();
        const std::string text = ss.str();
        return text.find("\"gearbox\": \"Automatic\"") != std::string::npos || text.find("\"autoGearing\": true") != std::string::npos;
    }
} // namespace

TEST_CASE("diesel builds: every shipped governed build that shifts by its redline has it at or under its governed high idle", "[diesel][builds]")
{
    // The redline the vehicle reads (tel.engine.redline: an automatic's shift points, auto gearing's final drive) against
    // the speed the governor lets the engine reach at no load and full pedal (heavy.fm's highIdle). A build states its
    // redline at its high idle (the archetypes' rule: ecu.ratedSpeed x (1 + droop)), so the next governed automatic
    // cannot ship on a redline it never reaches (X-057). 0.5 % of slack for a stated figure's rounding
    int n = 0;
    for (const std::string& name : governedBuilds())
    {
        if (!shiftsByRedline(name))
            continue;
        double red = 0.0, high = 0.0;
        redlineAgainstHighIdle(name, red, high);
        INFO(name);
        CHECK(high > 0.0);
        CHECK(red <= 1.005 * high);
        ++n;
    }
    CHECK(n >= 2);   // (the pickup archetype and the tuned pickup, 27 Nov)
}

TEST_CASE("diesel builds, full: every shipped governed build's redline is at or under its governed high idle", "[diesel][builds][!mayfail]")
{
    // (marked [!mayfail], the lead's rule for a known finding: on the reference ECU (ecu.model 0) a redline is floored at
    // idle + 1500 rpm (ecu.fm redlineFor), above the governed high idle of diesel-v8-truck (2014 rpm against some 2324),
    // diesel-mechanical-classic (2268 against 2355) and diesel-marine-v12 (1890 against 2208): no build file can state
    // theirs. They shift by hand or not at all, so nothing is stuck; the cure is the window's item 19 (a governed engine's
    // redline is its governed high idle, in redlineFor, sim/), and with it this case goes plain)
    for (const std::string& name : governedBuilds())
    {
        double red = 0.0, high = 0.0;
        redlineAgainstHighIdle(name, red, high);
        INFO(name);
        CHECK(high > 0.0);
        CHECK(red <= 1.005 * high);
    }
}
