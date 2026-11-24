// tests/sim/core/LayoutTest.cpp: card D's test 3, firing intervals from the geometry; and every preset's as its file
// says; the reference's seventeen exactly as sim/ref/layouts.fm has them.
#include "Presets.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace tee::test::core;

namespace
{
    struct Derived
    {
        std::vector<double> fire;
        std::vector<int> order;          // cylinder numbers from 1
        std::vector<double> intervals;
        int ok = 0;
    };

    Derived derive(Rig& rig, const nlohmann::json& doc)
    {
        applyLayout(rig, doc);
        rig.build(0.09, 0.09, 0.15, 10.0, 0.2);
        Derived d;
        const int n = int(rig.get("lay.n"));
        for (int c = 0; c < n; ++c)
        {
            d.fire.push_back(rig.get("lay.fire[" + std::to_string(c) + "]"));
            d.order.push_back(int(rig.get("lay.order[" + std::to_string(c) + "]")) + 1);
            d.intervals.push_back(rig.get("lay.interval[" + std::to_string(c) + "]"));
        }
        d.ok = int(rig.get("lay.ok"));
        return d;
    }

    // Each bank's intervals, from its first cylinder's firing
    std::vector<double> bankIntervals(const nlohmann::json& doc, const Derived& d, int bank)
    {
        std::vector<double> a;
        const auto& banks = doc["spec"]["engine"]["bank"];
        for (std::size_t c = 0; c < d.fire.size(); ++c)
            if (banks[c].get<int>() == bank)
                a.push_back(d.fire[c]);
        std::sort(a.begin(), a.end());
        std::vector<double> out;
        for (std::size_t i = 0; i < a.size(); ++i)
            out.push_back(i + 1 < a.size() ? a[i + 1] - a[i] : 720.0 - a[i] + a[0]);
        return out;
    }

    bool sameCyclic(std::vector<double> a, const std::vector<double>& b)
    {
        for (std::size_t k = 0; k < a.size(); ++k)
        {
            bool same = a.size() == b.size();
            for (std::size_t i = 0; same && i < a.size(); ++i)
                same = std::abs(a[(i + k) % a.size()] - b[i]) < 0.01;
            if (same)
                return true;
        }
        return false;
    }

    void requireIntervals(const std::string& name, const std::vector<int>& order, const std::vector<double>& intervals)
    {
        Rig rig;
        const auto doc = loadPreset(name);
        const Derived d = derive(rig, doc);
        INFO(name);
        CHECK(d.ok == 1);
        CHECK(d.order == order);
        REQUIRE(d.intervals.size() == intervals.size());
        for (std::size_t i = 0; i < intervals.size(); ++i)
            CHECK(std::abs(d.intervals[i] - intervals[i]) < 0.01);
    }
} // namespace

TEST_CASE("firing intervals from geometry: the card's table of known engines", "[core][layout]")
{
    requireIntervals("ref-05-inline-4", {1, 3, 4, 2}, {180, 180, 180, 180});
    requireIntervals("ref-12-v8-cross", {1, 8, 4, 3, 6, 5, 7, 2}, {90, 90, 90, 90, 90, 90, 90, 90});
    requireIntervals("v6-90-common", {1, 6, 5, 4, 3, 2}, {150, 90, 150, 90, 150, 90});
    requireIntervals("v6-90-split", {1, 6, 5, 4, 3, 2}, {120, 120, 120, 120, 120, 120});
    requireIntervals("ref-03-vtwin-45", {1, 2}, {315, 405});
    requireIntervals("ref-02-twin-270", {1, 2}, {270, 450});
    requireIntervals("triple-tplane", {1, 3, 2}, {180, 270, 270});
    requireIntervals("ref-14-v10-72", {1, 6, 5, 10, 2, 7, 3, 8, 4, 9}, {72, 72, 72, 72, 72, 72, 72, 72, 72, 72});

    // the crossplane V8's banks: each 180-90-180-270 (as a cycle)
    Rig rig;
    const auto doc = loadPreset("ref-12-v8-cross");
    const Derived d = derive(rig, doc);
    CHECK(sameCyclic(bankIntervals(doc, d, 0), {180, 90, 180, 270}));
    CHECK(sameCyclic(bankIntervals(doc, d, 1), {180, 90, 180, 270}));
}

TEST_CASE("every layout preset fires as its file says", "[core][layout][presets]")
{
    const auto names = presetNames();
    REQUIRE(names.size() >= 33);
    for (const auto& name : names)
    {
        const auto doc = loadPreset(name);
        const auto& expect = doc["expect"];
        requireIntervals(name, expect["firingOrder"].get<std::vector<int>>(), expect["firingIntervals"].get<std::vector<double>>());
    }
}

TEST_CASE("the reference's seventeen layouts' firing angles, exactly", "[core][layout][presets]")
{
    // sim/ref/layouts.fm's table (fa), as tests/sim/core/ref_firing.json copies it
    const auto table = nlohmann::json::parse(readFile(std::filesystem::path(TEE_SOURCE_DIR) / "tests" / "sim" / "core" / "ref_firing.json"));
    int seen = 0;
    for (const auto& name : presetNames())
    {
        const auto doc = loadPreset(name);
        const auto& engine = doc["spec"]["engine"];
        if (!engine.contains("layout"))
            continue;
        const int k = engine["layout"].get<int>();
        Rig rig;
        const Derived d = derive(rig, doc);
        INFO(name);
        for (std::size_t c = 0; c < d.fire.size(); ++c)
            CHECK(d.fire[c] == table[k][c].get<double>());
        seen += 1;
    }
    CHECK(seen == 17);
}

TEST_CASE("a layout whose cylinders fire together is flagged", "[core][layout]")
{
    Rig rig;
    auto doc = loadPreset("ref-01-twin-360");
    doc["spec"]["engine"]["fireRev"] = {0, 0};
    const Derived d = derive(rig, doc);
    CHECK(d.ok == 0);
    CHECK(rig.get("lay.same") == 1);
}
