// tests/sim/combo/DecisionsTest.cpp: package X's "the same switches reach the same engine, whichever way they were
// set". What a person does between runs: the key on, a switch turned on while the engine is stopped, the start, with
// no layout or size change between (so nothing configures again; D's 492e525 sequence). For every model switch:
//   - "the build with it off, written on while stopped" reaches the engine "the build with it on" reaches;
//   - a build with every switch on, loaded while stopped over the same block with every switch off, reaches the
//     engine that build reaches loaded fresh.
// The engine reached: the idle (8 to 9 s) and brake torque on the engine dyno at 3000 rpm, full throttle, each within
// 2 %. Fast tier: the default block (content/builds/default.json); nightly: the V8 and the turbo six. And the absolute
// check at the end (smoothInSize), which does fail before f89d40c.
// What it does not catch: the hole D closed in f89d40c. On the tree before it every route through the host gives the
// same 444 N m with fuel.model on (the switch in the build file, written while stopped, or written with a size change
// so a configure runs: measured with tee-render), so no two host routes differ there; D's Bench test reads the
// decisions themselves. This holds the routes together from here on.
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace combo;

namespace
{
    struct Reached
    {
        double idle = 0.0, torque = 0.0;
    };

    using Values = std::vector<std::pair<std::string, double>>;

    std::filesystem::path blockFile(const std::string& block)
    {
        return block.empty() ? std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/default.json"
                             : std::filesystem::path(buildPath(block));
    }
    std::string blockName(const std::string& block) { return block.empty() ? "default block" : block; }

    // The block's build file with `values` written into it (so the program's first init sees them: a Scenario's
    // write at t = 0 lands after that init, as a person's does), under out/combo/decisions/.
    std::string buildWith(const std::string& block, const Values& values, const std::string& tag)
    {
        std::ifstream in(blockFile(block));
        nlohmann::json j = nlohmann::json::parse(in);
        for (const auto& [path, value] : values)
        {
            const auto dot = path.find('.');
            nlohmann::json& f = j["spec"][path.substr(0, dot)][path.substr(dot + 1)];
            if (f.is_boolean())
                f = value != 0.0;
            else
                f = value;
        }
        const std::filesystem::path dir = std::filesystem::path("out") / "combo" / "decisions";
        std::filesystem::create_directories(dir);
        const std::filesystem::path file =
            dir / ((block.empty() ? std::string("default") : block) + "-" + tag + ".json");
        std::ofstream(file) << j.dump(2) << "\n";
        return std::filesystem::absolute(file).string();
    }

    // D's sequence (content/scenarios/core/stopped-switch.json): the key on; `late` written at 0.1 s, the engine
    // stopped, with no layout or size change (so nothing configures again); the start at 0.2 s; idle; the engine dyno
    // at 3000 rpm, full throttle. The idle is read from 8 s: an event of the key differs on the two paths (voices.model
    // primes the fuel pump as the key turns on, so a switch turned on after the key misses it), and its idle meets the
    // other path's by then (96.0 against 91.1 rad/s at 2.2 to 3 s, 90.8 both at 8 to 12 s, measured).
    Reached reach(const std::string& build, const Values& late)
    {
        tee::test::Scenario s(build);
        s.at(0.0).control("mode", 0).lever(0).clutch(0).throttle(0).ignition(tee::On);
        s.at(0.1);
        for (const auto& [path, value] : late)
            s.spec(path, value);
        s.at(0.2).ignition(tee::Start);
        s.at(1.8).ignition(tee::On);
        s.at(9.0).mode(2).dyno(314.16);   // MODE_DYNO_ENGINE at 3000 rpm
        s.at(9.2).throttle(1.0).ramp(0.3);
        const tee::test::Run r = s.run(12.0);
        REQUIRE(r.ok());
        Reached out;
        out.idle = r.tel("engine.speed").during(8.0, 9.0).mean();
        out.torque = r.tel("engine.torque").during(10.5, 12.0).mean();
        return out;
    }

    bool same(const Reached& a, const Reached& b)
    {
        return std::fabs(a.torque - b.torque) <= 0.02 * std::max(std::fabs(b.torque), 50.0) &&
               std::fabs(a.idle - b.idle) <= 0.02 * std::max(b.idle, 30.0);
    }

    // The switches whose index is `part` mod `parts` (a case each third: each under 30 s).
    void eachSwitch(const std::string& block, std::size_t part = 0, std::size_t parts = 1)
    {
        const auto& sw = switches();
        REQUIRE(!sw.empty());
        for (std::size_t i = part; i < sw.size(); i += parts)
        {
            const Switch& w = sw[i];
            const double on = w.on;
            std::string tag = w.path;
            std::replace(tag.begin(), tag.end(), '.', '-');
            const Reached fresh = reach(buildWith(block, {{w.path, on}}, tag), {});
            const Reached turned = reach(buildWith(block, {{w.path, 0.0}}, tag + "-off"), {{w.path, on}});
            std::printf("decisions | %-14s | %-16s | at start: idle %6.1f, %7.1f N m | turned on while stopped: idle "
                        "%6.1f, %7.1f N m\n",
                        blockName(block).c_str(), w.path.c_str(), fresh.idle, fresh.torque,
                        turned.idle, turned.torque);
            INFO(w.path << " on " << blockName(block) << ": at start " << fresh.torque
                        << " N m, idle " << fresh.idle << "; turned on while stopped " << turned.torque << " N m, idle "
                        << turned.idle);
            CHECK(same(turned, fresh));
        }
    }

    void loadedOver(const std::string& block)
    {
        std::vector<std::pair<std::string, double>> off, on;
        for (const Switch& w : switches())
        {
            off.emplace_back(w.path, 0.0);
            on.emplace_back(w.path, w.on);
        }
        const Reached fresh = reach(buildWith(block, on, "all"), {});
        const Reached loaded = reach(buildWith(block, off, "none"), on);
        std::printf("decisions | %-14s | every switch     | fresh: idle %6.1f, %7.1f N m | loaded over every switch off: "
                    "idle %6.1f, %7.1f N m\n",
                    blockName(block).c_str(), fresh.idle, fresh.torque, loaded.idle,
                    loaded.torque);
        INFO("every switch on " << blockName(block) << ": fresh " << fresh.torque
                                << " N m; loaded over every switch off " << loaded.torque << " N m");
        CHECK(same(loaded, fresh));
    }
} // namespace

TEST_CASE("decisions: each switch turned on while stopped reaches the engine it reaches at start: the default block 1 of 3",
          "[fast][combo][decisions]")
{
    eachSwitch("", 0, 3);
}

TEST_CASE("decisions: each switch turned on while stopped reaches the engine it reaches at start: the default block 2 of 3",
          "[fast][combo][decisions]")
{
    eachSwitch("", 1, 3);
}

TEST_CASE("decisions: each switch turned on while stopped reaches the engine it reaches at start: the default block 3 of 3",
          "[fast][combo][decisions]")
{
    eachSwitch("", 2, 3);
}

TEST_CASE("decisions: every switch loaded over every switch off while stopped reaches the fresh engine: the default block",
          "[fast][combo][decisions]")
{
    loadedOver("");
}

TEST_CASE("decisions: each switch turned on while stopped and every switch loaded over: the V8 and the turbo six",
          "[nightly][decisions]")
{
    for (const char* b : {"v8", "turbo-six"})
    {
        eachSwitch(b);
        loadedOver(b);
    }
}

namespace
{
    // The absolute check (the lead's, 20:02): the default block's size is the one the program inits with, so a decision
    // the switch makes only when the size changes (a configure) is missed there and nowhere else. Torque is smooth in
    // size: with a switch on, the default block's (5.76 L) lies on the line between the same block at 5.0 and 6.2 L
    // (each a configure), within 3 %. On f89d40c~1, fuel.model on: 444.5 N m against the line's 533.8 (463.2 at 5.0 L,
    // 574.6 at 6.2 L); on the tip 505.0 against 505.2. One cc larger does not configure: 444.5 both on the old tree.
    void smoothInSize(const std::string& path, double on)
    {
        const double sizes[3] = {0.0050, 0.00576, 0.0062};
        double torque[3] = {};
        for (int i = 0; i < 3; ++i)
        {
            Values v = {{path, on}};
            if (i != 1)
                v.emplace_back("engine.displacement", sizes[i]);
            std::string tag = path + "-size" + std::to_string(i);
            std::replace(tag.begin(), tag.end(), '.', '-');
            torque[i] = reach(buildWith("", v, tag), {}).torque;
        }
        const double line = torque[0] + (torque[2] - torque[0]) * (sizes[1] - sizes[0]) / (sizes[2] - sizes[0]);
        std::printf("decisions | size line      | %-16s | 5.0 L %7.1f, 5.76 L %7.1f, 6.2 L %7.1f N m: the line at 5.76 L "
                    "%7.1f (%+.1f %%)\n",
                    path.c_str(), torque[0], torque[1], torque[2], line, 100.0 * (torque[1] / line - 1.0));
        INFO(path << " on: the default block makes " << torque[1] << " N m; the line between 5.0 L (" << torque[0]
                  << ") and 6.2 L (" << torque[2] << ") gives " << line);
        CHECK(std::fabs(torque[1] - line) <= 0.03 * std::fabs(line));
    }
} // namespace

// The fast form: the two switches whose decision f89d40c fixed on the default block (measured on f89d40c~1: fuel.model
// -16.7 %, engine.wallHeat -17.0 %; boost.model -0.1 %, so it is the nightly's). The nightly: every switch (the tip:
// within 1.1 %, voices.model's prime the largest).
TEST_CASE("decisions: with fuel.model or engine.wallHeat on the default block's torque lies on its size line",
          "[fast][combo][decisions]")
{
    for (const char* p : {"fuel.model", "engine.wallHeat"})
        smoothInSize(p, 1.0);
}

TEST_CASE("decisions: with each switch on the default block's torque lies on its size line", "[nightly][decisions]")
{
    for (const Switch& w : switches())
        smoothInSize(w.path, w.on);
}
