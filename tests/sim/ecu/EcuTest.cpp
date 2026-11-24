// tests/sim/ecu/EcuTest.cpp: card H's standalone tests 3 and 4: the maps and their baking; each limiter, launch
// control and the idle on a one-line engine (oneline.fm), through the model-1 ECU the simulator will run.
#include "sim/ecu/FmHarness.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

using tee::test::h::Fm;

namespace
{
    enum
    {
        MAP_SPARK,
        MAP_LAMBDA,
        MAP_BOOST,
        MAP_CAM_IN,
        MAP_CAM_EX,
        MAP_PEDAL
    };

    Fm engine()
    {
        Fm fm = Fm::load("ecu/oneline.fm", "OneLine");
        fm.call<void>("OneLine::setup");
        fm.set("ecu.maps.tune.burnK", 1.0);
        fm.set("ecu.maps.tune.maxLoad", 1.0);
        fm.set("ecu.maps.tune.gears", 6);
        return fm;
    }

    std::string at(const std::string& field, int i) { return field + "[" + std::to_string(i) + "]"; }

    struct Band
    {
        double low, high, mean;
    };

    // Runs `settle` seconds, then measures `seconds`
    Band hold(Fm& fm, double settle, double seconds)
    {
        fm.call<void>("OneLine::run", static_cast<float>(settle));
        fm.call<void>("OneLine::measure");
        fm.call<void>("OneLine::run", static_cast<float>(seconds));
        return {fm.get("rpmMin"), fm.get("rpmMax"), fm.call<float>("OneLine::mean")};
    }
} // namespace

TEST_CASE("H3: a map reads its breakpoints exactly and interpolates between", "[h][ecu][maps]")
{
    Fm fm = engine();
    const std::string map = "set.maps.spark";
    auto xOf = [](int i) { return 800.0 + 350.0 * i + 12.0 * i * i; };     // uneven axes
    auto yOf = [](int j) { return 0.15 + 0.05 * j + 0.002 * j * j; };
    auto zOf = [](int i, int j) { return 10.0 + 1.7 * i - 0.9 * j + 0.13 * i * j; };
    for (int i = 0; i < 16; ++i)
    {
        fm.set(at(map + ".x", i), xOf(i));
        fm.set(at(map + ".y", i), yOf(i));
        for (int j = 0; j < 16; ++j)
            fm.set(map + ".z[" + std::to_string(i) + "][" + std::to_string(j) + "]", zOf(i, j));
    }
    fm.set(map + ".nx", 16);
    fm.set(map + ".ny", 16);
    fm.set("set.maps.sparkAuto", 0);
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j)
        {
            const float z = static_cast<float>(zOf(i, j));
            CHECK(fm.call<float>("OneLine::mapAt", MAP_SPARK, static_cast<float>(xOf(i)), static_cast<float>(yOf(j))) == z);
        }
    // between: bilinear (z is bilinear in i and j, and x, y are smooth in them, so check against the corners)
    for (int i = 0; i < 15; ++i)
        for (int j = 0; j < 15; ++j)
        {
            const double fx = 0.3, fy = 0.65;
            const double u = xOf(i) + fx * (xOf(i + 1) - xOf(i));
            const double v = yOf(j) + fy * (yOf(j + 1) - yOf(j));
            const double want = (1 - fx) * ((1 - fy) * zOf(i, j) + fy * zOf(i, j + 1)) + fx * ((1 - fy) * zOf(i + 1, j) + fy * zOf(i + 1, j + 1));
            CHECK(fm.call<float>("OneLine::mapAt", MAP_SPARK, static_cast<float>(u), static_cast<float>(v)) == Catch::Approx(want).epsilon(1e-5));
        }
    // beyond its ends: flat
    CHECK(fm.call<float>("OneLine::mapAt", MAP_SPARK, 100.0f, 0.0f) == static_cast<float>(zOf(0, 0)));
    CHECK(fm.call<float>("OneLine::mapAt", MAP_SPARK, 20000.0f, 9.0f) == static_cast<float>(zOf(15, 15)));
    // a map of fewer breakpoints uses only those
    fm.set(map + ".nx", 3);
    fm.set(map + ".ny", 2);
    CHECK(fm.call<float>("OneLine::mapAt", MAP_SPARK, 20000.0f, 9.0f) == static_cast<float>(zOf(2, 1)));
}

TEST_CASE("H3: spec/tables.fm's Map16::at reads every breakpoint exactly", "[h][ecu][maps]")
{
    // (Map16::at blends a (1 - f) + b f: b exactly at f = 1, a map's last breakpoint; a + (b - a) f was a rounding off)
    Fm fm = engine();
    const std::string map = "set.maps.spark";
    for (int i = 0; i < 16; ++i)
    {
        fm.set(at(map + ".x", i), 800.0 + 350.0 * i);
        fm.set(at(map + ".y", i), 0.15 + 0.05 * i);
        for (int j = 0; j < 16; ++j)
            fm.set(map + ".z[" + std::to_string(i) + "][" + std::to_string(j) + "]", 10.0 + 1.7 * i - 0.9 * j + 0.13 * i * j);
    }
    fm.set(map + ".nx", 16);
    fm.set(map + ".ny", 16);
    int off = 0;
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j)
        {
            const float want = static_cast<float>(fm.get(map + ".z[" + std::to_string(i) + "][" + std::to_string(j) + "]"));
            off += fm.call<float>("OneLine::tableAt", static_cast<float>(800.0 + 350.0 * i), static_cast<float>(0.15 + 0.05 * j)) != want ? 1 : 0;
        }
    WARN(off << " of 256 breakpoints read off by Map16::at");
    CHECK(off == 0);
}

TEST_CASE("H3: a baked map reproduces its formula at every breakpoint", "[h][ecu][maps]")
{
    Fm fm = engine();
    fm.call<void>("OneLine::bake");
    for (int which : {MAP_SPARK, MAP_LAMBDA, MAP_BOOST, MAP_PEDAL})
    {
        fm.call<void>("OneLine::adopt", which);
        const int nx = static_cast<int>(fm.get("ecu.maps.bakedNx[" + std::to_string(which) + "]"));
        const int ny = static_cast<int>(fm.get("ecu.maps.bakedNy[" + std::to_string(which) + "]"));
        CHECK(nx == 16);
        CHECK(ny == (which == MAP_BOOST ? 6 : 16));
        int checked = 0;
        for (int i = 0; i < nx; ++i)
            for (int j = 0; j < ny; ++j)
            {
                const float u = static_cast<float>(fm.get("ecu.maps.bakedX[" + std::to_string(which) + "][" + std::to_string(i) + "]"));
                const float v = static_cast<float>(fm.get("ecu.maps.bakedY[" + std::to_string(which) + "][" + std::to_string(j) + "]"));
                INFO("map " << which << " at " << u << ", " << v);
                CHECK(fm.call<float>("OneLine::mapAt", which, u, v) == fm.call<float>("OneLine::formula", which, u, v));
                checked += 1;
            }
        CHECK(checked == nx * ny);
    }
    // and the spark's formula is the reference's: 12 + 22 sqrt(rpm / red) - 8 (load - 0.4) at a square engine
    CHECK(fm.call<float>("OneLine::formula", MAP_SPARK, 3250.0f * 0.10471976f, 0.4f) == Catch::Approx(12.0 + 22.0 * std::sqrt(0.5)).epsilon(1e-5));
}

TEST_CASE("H4: each limiter holds its speed on a one-line engine", "[h][ecu][limiter]")
{
    // Free revs, the pedal floored, warm, the layout's redline of 6500 rpm. The bands: what each kind is for
    struct Kind
    {
        int kind;
        const char* name;
        double below, above;
    };
    const Kind kinds[] = {{0, "fuel cut", 300.0, 150.0},
                          {1, "spark cut", 300.0, 150.0},
                          {2, "soft", 300.0, 50.0},
                          {3, "throttle", 200.0, 50.0}};
    for (const Kind& k : kinds)
    {
        Fm fm = engine();
        fm.call<void>("OneLine::running");
        fm.set("set.limiter.kind", k.kind);
        fm.set("in.pedal", 1.0);
        const Band b = hold(fm, 2.0, 4.0);
        WARN(k.name << ": " << b.low << " to " << b.high << " rpm, mean " << b.mean);
        CHECK(b.low > 6500.0 - k.below);
        CHECK(b.high < 6500.0 + k.above);
        if (k.kind == 1)
            CHECK(fm.get("cutSpark") > 0.0);
        if (k.kind == 0)
            CHECK(fm.get("cutFuel") > 0.0);
    }
    // a limiter's own speed, under the redline
    Fm fm = engine();
    fm.call<void>("OneLine::running");
    fm.set("set.limiter.speed", 5000.0 / 9.5492966);
    fm.set("in.pedal", 1.0);
    const Band b = hold(fm, 2.0, 2.0);
    CHECK(b.low > 4700.0);
    CHECK(b.high < 5150.0);
}

TEST_CASE("H4: launch control holds its speed", "[h][ecu][launch]")
{
    Fm fm = engine();
    fm.call<void>("OneLine::running");
    fm.set("set.launch.on", 1);
    fm.set("in.clutch", 1.0);
    fm.set("in.carSpeed", 0.0);
    fm.set("in.pedal", 1.0);
    const Band b = hold(fm, 1.5, 3.0);
    WARN("launch at 4000: " << b.low << " to " << b.high << " rpm, mean " << b.mean);
    CHECK(b.low > 3800.0);
    CHECK(b.high < 4150.0);
    CHECK((static_cast<int>(fm.get("ecu.flags")) & 16) != 0);
    // the clutch comes up: it lets go, and the engine runs on to the limiter
    fm.set("in.clutch", 0.0);
    const Band free = hold(fm, 1.0, 1.0);
    CHECK((static_cast<int>(fm.get("ecu.flags")) & 16) == 0);
    CHECK(free.high > 6000.0);
}

TEST_CASE("H4: the idle holds within 30 rpm warm and comes down from its cold fast idle", "[h][ecu][idle]")
{
    {
        Fm fm = engine();
        fm.call<void>("OneLine::running");
        const Band b = hold(fm, 20.0, 10.0);
        const double target = fm.get("ecu.idle.target");
        WARN("warm idle: target " << target << ", " << b.low << " to " << b.high);
        CHECK(target == Catch::Approx(750.0));
        CHECK(b.low > target - 30.0);
        CHECK(b.high < target + 30.0);
    }
    {
        Fm fm = engine();
        fm.call<void>("OneLine::running");
        fm.set("coolant", 290.0);
        fm.set("warmTau", 100.0);
        const Band cold = hold(fm, 4.0, 2.0);
        const double coldTarget = fm.get("ecu.idle.target");
        const Band warm = hold(fm, 600.0, 10.0);
        WARN("cold idle: " << cold.mean << " rpm (target " << coldTarget << "); after ten minutes " << warm.mean);
        CHECK(coldTarget > 950.0);
        CHECK(cold.mean > coldTarget - 60.0);
        CHECK(warm.low > 720.0);
        CHECK(warm.high < 780.0);
    }
}

TEST_CASE("H4: the key cranks, runs, stalls and runs down", "[h][ecu][state]")
{
    Fm fm = engine();
    fm.set("coolant", 290.0);
    fm.set("in.key", 2);
    fm.call<void>("OneLine::run", 0.05f);
    CHECK(fm.get("ecu.key.state") == 1.0);
    fm.call<void>("OneLine::run", 3.0f);
    CHECK(fm.get("ecu.key.state") == 2.0);
    fm.set("in.key", 1);
    fm.call<void>("OneLine::run", 3.0f);
    CHECK(fm.get("ecu.key.state") == 2.0);
    CHECK(fm.get("ecu.speed.avg") > 700.0);
    // a load the idle cannot carry stalls it
    fm.set("load", 400.0);
    fm.call<void>("OneLine::run", 2.0f);
    CHECK(fm.get("ecu.key.state") != 2.0);
    fm.set("load", 0.0);
    fm.call<void>("OneLine::run", 3.0f);
    CHECK(fm.get("ecu.key.state") == 0.0);
    // started again, then the key off: it runs down to rest
    fm.set("in.key", 2);
    fm.call<void>("OneLine::run", 3.0f);
    fm.set("in.key", 1);
    fm.call<void>("OneLine::run", 2.0f);
    CHECK(fm.get("ecu.key.state") == 2.0);
    fm.set("in.key", 0);
    fm.call<void>("OneLine::run", 0.1f);
    CHECK(fm.get("ecu.key.state") == 3.0);
    fm.call<void>("OneLine::run", 5.0f);
    CHECK(fm.get("ecu.key.state") == 0.0);
    CHECK(fm.get("omega") < 2.0);
}

TEST_CASE("H9 (standalone part): the bangs follow the fuel sent, and no fuel makes none", "[h][ecu][overrun]")
{
    Fm fm = engine();
    // 3000 rpm, a hot pipe (1100 K), each firing letting 2 mg of fuel and plenty of air (a spark cut's whole charge
    // would be some 40 mg: this is an overrun's few percent)
    const double little = fm.call<float>("OneLine::bangs", 2.0e-6f, 0.5e-3f, 1100.0f, 3000.0f, 2.0f);
    const double littleBig = fm.get("biggest");
    const int littleBangs = static_cast<int>(fm.get("ecu.account.bangs"));
    const double much = fm.call<float>("OneLine::bangs", 8.0e-6f, 0.5e-3f, 1100.0f, 3000.0f, 2.0f);
    const double muchBig = fm.get("biggest");
    const int muchBangs = static_cast<int>(fm.get("ecu.account.bangs"));
    const double sent = 2.0 * 3000.0 / 60.0 * 4.0;                      // firings in 2 s
    WARN("2 mg a firing: " << littleBangs << " bangs, " << little * 1e6 / sent << " mg burned a firing, the largest "
                           << littleBig << " Pa; 8 mg: " << muchBangs << " bangs, " << much * 1e6 / sent
                           << " mg a firing, the largest " << muchBig << " Pa");
    CHECK(littleBangs > 0);
    CHECK(much > 3.0 * little);                  // four times the fuel: about four times what burns
    CHECK(much < 5.0 * little);
    CHECK(muchBig > littleBig);                  // and larger pulses
    CHECK(little <= 2.0e-6 * sent);              // never more than was sent
    // no fuel (a full fuel cut): none; no air (a rich spark cut into a pipe with no oxygen): none; a cold pipe (600 K): none
    CHECK(fm.call<float>("OneLine::bangs", 0.0f, 0.5e-3f, 1100.0f, 3000.0f, 2.0f) == 0.0f);
    CHECK(fm.call<float>("OneLine::bangs", 8.0e-6f, 0.0f, 1100.0f, 3000.0f, 2.0f) == 0.0f);
    CHECK(fm.call<float>("OneLine::bangs", 8.0e-6f, 0.5e-3f, 600.0f, 3000.0f, 2.0f) == 0.0f);
}

TEST_CASE("H10 (standalone part): traction control holds a toy wheel's slip near its target", "[h][ecu][traction]")
{
    Fm fm = engine();
    fm.set("set.traction.on", 1);
    // twice the torque the grip takes: unchecked, the slip would settle at 0.4
    const double held = fm.call<float>("OneLine::traction", 2.0f, 4.0f);
    WARN("slip held at " << held << " against a target of 0.1");
    CHECK(held > 0.07);
    CHECK(held < 0.13);
    fm.set("set.traction.on", 0);
    CHECK(fm.call<float>("OneLine::traction", 2.0f, 4.0f) == Catch::Approx(0.4).epsilon(0.01));
}
