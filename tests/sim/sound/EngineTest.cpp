// tests/sim/sound/EngineTest.cpp: the voices in the whole simulator (sim/engine.fm through card C's Scenario), each
// heard as the difference between a run with it and the same run without it (every voice draws its own noise, so
// switching one leaves the others' samples as they were).
#include "Rig.h"
#include "tee/test/Scenario.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace
{
    constexpr double kTwoPi = 6.28318530717958647692;

    tee::test::Scenario base(int mode)
    {
        tee::test::Scenario s("v8");
        s.rate(96000.0);
        s.at(0.0).mode(mode).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On).spec("voices.model", 1.0);
        s.at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On);
        return s;
    }

    std::vector<float> diff(const tee::test::Run& a, const tee::test::Run& b, double from, double to)
    {
        const auto x = a.audio().during(from, to).samples();
        const auto y = b.audio().during(from, to).samples();
        std::vector<float> d(std::min(x.size(), y.size()));
        for (std::size_t i = 0; i < d.size(); ++i)
            d[i] = x[i] - y[i];
        return d;
    }
}

TEST_CASE("engine: on the bench there is no gearbox: the whine's switch changes nothing, to the bit", "[sound][engine]")
{
    auto run = [](double whineDb) {
        tee::test::Scenario s = base(0);
        s.at(0.0).spec("voices.whine", whineDb).spec("voices.axle", whineDb).spec("voices.rattle", whineDb);
        s.at(3.0).holdSpeed(300.0);
        return s.run(5.0);
    };
    const auto on = run(0.0), off = run(-60.0);
    const auto d = diff(on, off, 0.0, 5.0);
    double m = 0.0;
    for (float v : d)
        m = std::max(m, static_cast<double>(std::fabs(v)));
    CHECK(m == 0.0);
}

TEST_CASE("engine: the alternator's line at 100.8 a turn of the crank, held at speed on the bench", "[sound][engine]")
{
    auto run = [](double beltDb) {
        tee::test::Scenario s = base(0);
        s.at(0.0).spec("voices.belt", beltDb);
        s.at(2.5).holdSpeed(300.0);
        return s.run(5.0);
    };
    const auto on = run(12.0), off = run(-60.0);
    const double hz = on.tel("engine.speed").during(4.0, 5.0).mean() / kTwoPi;
    const auto d = diff(on, off, 4.0, 5.0);
    const double f = 100.8 * hz;
    double best = 0.0, bestA = -1.0;
    for (double g = f * 0.98; g <= f * 1.02; g += f * 0.0005)
        if (const double a = tee::test::sound::line(d, g); a > bestA)
        {
            bestA = a;
            best = g;
        }
    INFO("crank " << hz << " Hz: expected " << f << ", found " << best);
    CHECK(std::fabs(best - f) < 0.01 * f);
    CHECK(tee::test::sound::rms(d) > 0.0);
}

TEST_CASE("engine: driven in third the gearbox whines at the constant mesh's 19 teeth; in neutral it does not", "[sound][engine]")
{
    auto run = [](double whineDb, int lever) {
        tee::test::Scenario s = base(1);
        s.at(0.0).spec("voices.whine", whineDb).spec("voices.rattle", -60.0);
        s.at(2.5).clutch(1.0).lever(lever).at(3.0).clutch(0.0).ramp(1.0).throttle(0.15).ramp(0.5);
        return s.run(16.0);
    };
    const auto on = run(12.0, 3), off = run(-60.0, 3);
    const double hz = on.tel("engine.speed").during(15.75, 16.0).mean() / kTwoPi;
    INFO("gear " << on.tel("car.gear").last() << ", engine " << hz << " Hz, " << on.tel("engine.speed").during(15.75, 16.0).min() / kTwoPi << " to " << on.tel("engine.speed").during(15.75, 16.0).max() / kTwoPi);
    REQUIRE(on.tel("car.gear").last() == 3.0);
    const auto d = diff(on, off, 15.75, 16.0);
    const double f = 19.0 * hz;
    double best = 0.0, bestA = -1.0;
    for (double g = f * 0.97; g <= f * 1.03; g += f * 0.001)
        if (const double a = tee::test::sound::line(d, g); a > bestA)
        {
            bestA = a;
            best = g;
        }
    INFO("expected " << f << ", found " << best);
    CHECK(std::fabs(best - f) < 0.02 * f);
    // in neutral, the clutch in: the constant mesh turns, unloaded: far quieter
    const auto nOn = run(12.0, 0), nOff = run(-60.0, 0);
    CHECK(tee::test::sound::rms(diff(nOn, nOff, 15.75, 16.0)) < 0.3 * tee::test::sound::rms(d));
}

TEST_CASE("engine: the starter is package M's while it cranks, and a reduction starter's differs from a direct one", "[sound][engine]")
{
    auto run = [](int kind) {
        tee::test::Scenario s = base(0);
        s.at(0.0).spec("voices.starter", static_cast<double>(kind));
        return s.run(2.5);
    };
    const auto red = run(1), direct = run(0);
    CHECK(red.audio().finite());
    CHECK(tee::test::sound::rms(diff(red, direct, 0.3, 0.6)) > 0.0);
    CHECK(red.audio().peak() <= 1.0f);
}

TEST_CASE("engine: the fuel pump's prime is heard at key-on, before the first crank", "[sound][engine]")
{
    auto run = [](double model, double pumpDb) {
        tee::test::Scenario s("v8");
        s.rate(96000.0);
        s.at(0.0).mode(0).throttle(0.0).ignition(tee::On).spec("voices.model", model).spec("voices.pump", pumpDb);
        s.at(1.0).ignition(tee::Start).until(2.5).ignition(tee::On);
        return s.run(3.0);
    };
    const auto ref = run(0.0, 0.0), on = run(1.0, 0.0), off = run(1.0, -60.0);
    CHECK(ref.audio().during(0.0, 0.95).rms() == 0.0);           // the reference renders nothing before the start
    CHECK(on.tel("engine.state").during(0.0, 0.95).max() == 0.0);
    const double prime = tee::test::sound::rms(diff(on, off, 0.1, 0.9));
    INFO("prime " << prime);
    CHECK(prime > 0.0);
    CHECK(on.audio().during(0.1, 0.9).rms() > off.audio().during(0.1, 0.9).rms());
}

TEST_CASE("engine: card F's wastegate chatters at full load on the dyno; an external one vented to the air is louder", "[sound][engine]")
{
    auto run = [](double gateDb, double external, double toAir) {
        tee::test::Scenario s("builds/boost/turbo-four.json");
        s.rate(96000.0);
        s.at(0.0).mode(0).throttle(0.0).ignition(tee::On).spec("voices.model", 1.0).spec("voices.gate", gateDb)
            .spec("boost.gate", external).spec("voices.gateDump", toAir);
        s.at(0.25).ignition(tee::Start).until(1.5).ignition(tee::On);
        s.at(3.0).mode(2).dyno(5500.0 * 0.10471976).throttle(1.0);
        return s.run(10.0);
    };
    const auto on = run(0.0, 0.0, 0.0), off = run(-60.0, 0.0, 0.0);
    INFO(on.error());
    REQUIRE(on.ok());
    // idling before the pull the gate is shut: the gate's voice adds nothing
    CHECK(tee::test::sound::rms(diff(on, off, 2.0, 2.9)) == 0.0);
    const double chatter = tee::test::sound::rms(diff(on, off, 9.0, 10.0));
    INFO("boost " << on.tel("boost.pressure").during(9.0, 10.0).mean() << " Pa, speed " << on.tel("engine.speed").last() << ", state " << on.tel("engine.state").last() << ", seconds " << on.audio().seconds() << "; chatter " << chatter);
    CHECK(chatter > 0.0);
    const auto ext = run(0.0, 1.0, 1.0), extOff = run(-60.0, 1.0, 1.0);
    const double dump = tee::test::sound::rms(diff(ext, extOff, 9.0, 10.0));
    INFO("dump " << dump);
    CHECK(dump > 2.0 * chatter);
}
