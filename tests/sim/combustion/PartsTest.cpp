// tests/sim/combustion/PartsTest.cpp: the diesel's fields at their defaults leave the reference's numbers; a pilot,
// the rail and the over-fuel move what they should; the tuner's readings (parts.fm).
#include "sim/ecu/FmHarness.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using tee::test::h::Fm;

TEST_CASE("H: the diesel's fields: the reference at their defaults", "[h][combustion][diesel]")
{
    Fm fm = Fm::load("combustion/parts.fm", "Parts");
    fm.call<void>("Parts::setup");
    CHECK(fm.call<float>("Parts::phi", 1.0f) == 0.75f);
    CHECK(fm.call<float>("Parts::phi", 0.4f) == 0.75f * 0.4f);
    CHECK(fm.call<float>("Parts::premixed", 0.5f) == 0.5f);
    CHECK(fm.call<float>("Parts::diffusion", 50.0f) == 50.0f);
    // a pilot takes most of the premixed share away; a higher rail mixes faster; over-fuel smokes
    fm.set("diesel.pilot", 0.05);
    CHECK(fm.call<float>("Parts::premixed", 0.5f) == Catch::Approx(0.15));
    fm.set("diesel.rail", 2000.0);
    CHECK(fm.call<float>("Parts::diffusion", 50.0f) == Catch::Approx(50.0 / std::sqrt(2.0)));
    fm.call<float>("Parts::phi", 1.0f);
    const double clean = fm.get("diesel.smoke");
    fm.set("diesel.overFuel", 0.2);
    CHECK(fm.call<float>("Parts::phi", 1.0f) == Catch::Approx(0.9));
    CHECK(fm.get("diesel.smoke") > clean + 3.0);
}

TEST_CASE("H: the tuner's readings: peak, half burn, lambda seen late, injector duty", "[h][combustion][watch]")
{
    Fm fm = Fm::load("combustion/parts.fm", "Parts");
    fm.call<void>("Parts::setup");
    CHECK(fm.call<float>("Parts::cycle", 14.0f, 40.0f) == 14.0f);
    CHECK(fm.get("watch.lastHalf") == 20.0);
    CHECK(fm.get("watch.lastPeak") == Catch::Approx(61.0 * 101325.0));
    // the sensor: a step to 0.85 seen after the firings' trip and its own 80 ms
    CHECK(fm.call<float>("Parts::sense", 1.0f, 3000.0f, 1.0f, 8) == Catch::Approx(1.0));
    const double soon = fm.call<float>("Parts::sense", 0.85f, 3000.0f, 0.03f, 8);
    const double later = fm.call<float>("Parts::sense", 0.85f, 3000.0f, 1.0f, 8);
    CHECK(soon > 0.95);
    CHECK(later == Catch::Approx(0.85).epsilon(1e-3));
    // 40 mg a firing through a 3.7e-3 kg/s injector (some 300 cc/min) at 3000 rpm: 10.8 ms + 0.5 ms open of a 40 ms cycle
    const double duty = fm.call<float>("Parts::inject", 4.0e-5f, 3.7e-3f, 3000.0f, 1.0f);
    CHECK(duty == Catch::Approx((4.0e-5 / 3.7e-3 + 0.0005) / 0.04).epsilon(1e-3));
    CHECK(fm.get("meter.flow") == Catch::Approx(4.0e-5 * 200.0).epsilon(0.02));
}

namespace
{
    // Every number or switch under `prefix` has its default and its label, every number its range (02-contracts.md
    // §3); a map's own breakpoints and values are its table's (they take none)
    void checkAttributes(const Fm& fm, const std::string& prefix)
    {
        int seen = 0;
        fm.schema->forEach([&](const std::string& path, const tee::Schema::Node& node, std::size_t) {
            if (path.rfind(prefix, 0) != 0)
                return;
            const std::string rest = path.substr(prefix.size());
            if (rest.find("maps.") != std::string::npos && rest.find('.', rest.find("maps.") + 5) != std::string::npos)
                return;
            INFO(path);
            seen += 1;
            CHECK(node.attribute("fminus::default") != nullptr);
            CHECK(node.attribute("ui::label") != nullptr);
            if (node.scalar != tee::Scalar::Bool)
                CHECK(node.attribute("fminus::range") != nullptr);
        });
        CHECK(seen > 8);
    }
} // namespace

TEST_CASE("H: every setting has its range, its default and its label", "[h][combustion][ecu][schema]")
{
    checkAttributes(Fm::load("combustion/parts.fm", "Parts"), "settings.");
    checkAttributes(Fm::load("ecu/oneline.fm", "OneLine"), "set.");
}
