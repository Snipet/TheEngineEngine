// tests/sim/sound/EngineTest.cpp: the voices in the whole simulator (sim/engine.fm through card C's Scenario), each
// heard as the difference between a run with it and the same run without it (every voice draws its own noise, so
// switching one leaves the others' samples as they were).
#include "Rig.h"
#include "tee/test/Scenario.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
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

TEST_CASE("engine: on the bench no gearbox (the whine's switch changes nothing, to the bit), the alternator's line at "
          "100.8 a turn, and the fuel pump's prime at key-on before the first crank", "[sound][engine]")
{
    // (one test, so that the simulator is compiled once for all of them: the fast tier's budget)
    {
        auto run = [](double whineDb) {
            tee::test::Scenario s = base(0);
            s.at(0.0).spec("voices.whine", whineDb).spec("voices.axle", whineDb).spec("voices.rattle", whineDb);
            s.at(2.5).holdSpeed(300.0);
            return s.run(3.5);
        };
        const auto on = run(0.0), off = run(-60.0);
        const auto d = diff(on, off, 0.0, 3.5);
        double m = 0.0;
        for (float v : d)
            m = std::max(m, static_cast<double>(std::fabs(v)));
        CHECK(m == 0.0);
    }
    {
        auto run = [](double beltDb) {
            tee::test::Scenario s = base(0);
            s.at(0.0).spec("voices.belt", beltDb);
            s.at(2.5).holdSpeed(300.0);
            return s.run(4.0);
        };
        const auto on = run(12.0), off = run(-60.0);
        const double hz = on.tel("engine.speed").during(3.5, 4.0).mean() / kTwoPi;
        const auto d = diff(on, off, 3.5, 4.0);
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
    {
        auto run = [](double model, double pumpDb) {
            tee::test::Scenario s("v8");
            s.rate(96000.0);
            s.at(0.0).mode(0).throttle(0.0).ignition(tee::On).spec("voices.model", model).spec("voices.pump", pumpDb);
            s.at(1.0).ignition(tee::Start);
            return s.run(1.1);
        };
        const auto ref = run(0.0, 0.0), on = run(1.0, 0.0), off = run(1.0, -60.0);
        CHECK(ref.audio().during(0.0, 0.95).rms() == 0.0);           // the reference renders nothing before the start
        CHECK(on.tel("engine.state").during(0.0, 0.95).max() == 0.0);
        const double prime = tee::test::sound::rms(diff(on, off, 0.1, 0.9));
        INFO("prime " << prime);
        CHECK(prime > 0.0);
        CHECK(on.audio().during(0.1, 0.9).rms() > off.audio().during(0.1, 0.9).rms());
    }
}


TEST_CASE("engine: driven in third the gearbox whines at the constant mesh's 19 teeth; in neutral it does not", "[sound][engine][nightly]")
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

TEST_CASE("engine: the starter is package M's while it cranks, and a reduction starter's differs from a direct one", "[sound][engine][nightly]")
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


TEST_CASE("engine: card F's wastegate chatters at full load on the dyno; an external one vented to the air is louder", "[sound][engine][nightly]")
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

TEST_CASE("engine: a naked twin's chain whines at its sprocket's teeth and its lash clunks on and off the throttle", "[sound][engine][bike][nightly]")
{
    auto run = [](double db) {
        tee::test::Scenario s("bike-naked-twin");
        s.rate(96000.0);
        s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On).spec("voices.model", 1.0)
            .spec("voices.chain", db).spec("voices.lashLevel", db).spec("voices.dryClutch", -60.0)
            .spec("car.frontSprocket", 16.0);
        s.at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On);
        s.at(2.5).shiftUp();
        s.at(2.8).throttle(0.4).ramp(0.2);
        s.at(5.0).shiftUp();
        s.at(6.0).throttle(0.0).ramp(0.05);
        s.at(6.6).throttle(0.5).ramp(0.05);
        s.at(7.2).throttle(0.0).ramp(0.05);
        return s.run(8.0);
    };
    const auto on = run(6.0), off = run(-60.0);
    REQUIRE(on.ok());
    INFO("lash closings " << on.tel("car.chainLash").last() << ", gear " << on.tel("car.gear").last());
    CHECK(on.tel("car.chainLash").last() > 0.0);
    const auto d = diff(on, off, 4.5, 4.75);
    const double w = on.tel("car.sprocketSpeed").during(4.5, 4.75).mean();
    const double f = w / kTwoPi * 16.0;     // (the front sprocket, set above)
    double best = 0.0, bestA = -1.0;
    for (double g = f * 0.97; g <= f * 1.03; g += f * 0.001)
        if (const double a = tee::test::sound::line(d, g); a > bestA)
        {
            bestA = a;
            best = g;
        }
    INFO("sprocket " << w << " rad/s: expected " << f << ", found " << best);
    CHECK(std::fabs(best - f) < 0.02 * f);
    CHECK(tee::test::sound::rms(diff(on, off, 5.9, 7.9)) > 0.0);
}

TEST_CASE("engine: a bike's own sequential box whines at its constant mesh's teeth on its input shaft; no final drive's", "[sound][engine][bike][nightly]")
{
    auto run = [](double db) {
        tee::test::Scenario s("bike-naked-twin");
        s.rate(96000.0);
        s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On).spec("voices.model", 1.0)
            .spec("voices.whine", db).spec("voices.axle", 0.0).spec("voices.chain", -60.0).spec("voices.lashLevel", -60.0)
            .spec("voices.dryClutch", -60.0).spec("voices.rattle", -60.0);
        s.at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On);
        s.at(2.5).shiftUp();
        s.at(2.8).throttle(0.3).ramp(0.2);
        return s.run(5.0);
    };
    const auto on = run(6.0), off = run(-60.0);
    REQUIRE(on.ok());
    // the input shaft with the clutch in: the crank over the primary ratio (bike-naked-twin's)
    const double crankHz = on.tel("engine.speed").during(4.5, 4.75).mean() / kTwoPi;
    double primary = 1.7;
    {
        std::ifstream in(std::string(TEE_SOURCE_DIR) + "/content/builds/bike-naked-twin.json");
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const auto at = text.find("\"primaryRatio\"");
        if (at != std::string::npos)
            primary = std::stod(text.substr(text.find(':', at) + 1));
    }
    const double f = 19.0 * crankHz / primary;
    const auto d = diff(on, off, 4.5, 4.75);
    double best = 0.0, bestA = -1.0;
    for (double g = f * 0.97; g <= f * 1.03; g += f * 0.001)
        if (const double a = tee::test::sound::line(d, g); a > bestA)
        {
            bestA = a;
            best = g;
        }
    INFO("crank " << crankHz << " Hz, primary " << primary << ": expected " << f << ", found " << best);
    CHECK(std::fabs(best - f) < 0.02 * f);
    // the final drive is the chain: switching the axle's whine changes nothing
    const auto axleOff = [] {
        tee::test::Scenario s("bike-naked-twin");
        s.rate(96000.0);
        s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On).spec("voices.model", 1.0)
            .spec("voices.whine", 6.0).spec("voices.axle", -60.0).spec("voices.chain", -60.0).spec("voices.lashLevel", -60.0)
            .spec("voices.dryClutch", -60.0).spec("voices.rattle", -60.0);
        s.at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On);
        s.at(2.5).shiftUp();
        s.at(2.8).throttle(0.3).ramp(0.2);
        return s.run(5.0);
    }();
    CHECK(tee::test::sound::rms(diff(on, axleOff, 0.0, 5.0)) == 0.0);
}

TEST_CASE("engine: voices switched on with the key or after it end in the same idle, and the pump primes either way",
          "[sound][engine]")
{
    // the two routes to "voices on, key on" end alike (X, 20:02; card D's c93c3391 renders from the voices' rise
    // too): voices with the key, or the key first and the voices 0.3 s later. The idle at 2.2 to 2.6 s
    // (the cold V8's, after a start at 1 s; the ECU's cold idle target glides meanwhile, so a later toggle needs
    // the glide to have settled by then) within 1 rad/s; before the fix 188.75 against 181.65. And the pump
    // primes in the late route too, before the first crank.
    auto run = [](double voicesAt, double pumpDb) {
        tee::test::Scenario s("v8");
        s.at(0.0).mode(0).throttle(0.0).ignition(tee::On).spec("voices.model", voicesAt <= 0.0 ? 1.0 : 0.0)
            .spec("voices.pump", pumpDb);
        if (voicesAt > 0.0)
            s.at(voicesAt).spec("voices.model", 1.0);
        s.at(1.0).ignition(tee::Start).until(2.0).ignition(tee::On);
        return s.run(2.6);
    };
    const auto first = run(0.0, 0.0);
    const double idleFirst = first.tel("engine.speed").during(2.2, 2.6).mean();
    const auto lateOn = run(0.3, 0.0), lateQuiet = run(0.3, -60.0);
    const double idleLate = lateOn.tel("engine.speed").during(2.2, 2.6).mean();
    INFO("voices with the key: " << idleFirst << " rad/s; 0.3 s after: " << idleLate);
    CHECK(std::fabs(idleLate - idleFirst) < 1.0);
    CHECK(tee::test::sound::rms(diff(lateOn, lateQuiet, 0.4, 0.95)) > 0.0);
}

namespace
{
    // A signal's energy in the octave about fc (fc / sqrt 2 to fc sqrt 2), or A-weighted over all (fc 0), in dB on
    // tee::measure's scale
    double bandDb(const tee::measure::Spectrum& s, double fc)
    {
        double e = 0.0;
        for (std::size_t k = 1; k < s.db.size(); ++k)
        {
            const double f = (double)k * s.binHz;
            double w = 1.0;
            if (fc > 0.0)
            {
                if (f < fc / std::sqrt(2.0) || f >= fc * std::sqrt(2.0))
                    continue;
            }
            else
            {
                // IEC 61672's A weighting
                const double f2 = f * f;
                const double ra = 12194.0 * 12194.0 * f2 * f2
                                  / ((f2 + 20.6 * 20.6) * std::sqrt((f2 + 107.7 * 107.7) * (f2 + 737.9 * 737.9)) * (f2 + 12194.0 * 12194.0));
                w = ra * ra * std::pow(10.0, 0.2);
            }
            e += w * std::pow(10.0, s.db[k] / 10.0);
        }
        return 10.0 * std::log10(e + 1.0e-30);
    }
}

TEST_CASE("engine: levels at idle, each voice alone against the exhaust, A-weighted and in octaves from 250 Hz to 8 kHz",
          "[sound][engine][nightly]")
{
    // The owner's 21:32 report (first.json: a 2.0 litre turbo four, direct injection, an electric fan): what each voice
    // is at its source beside the idling exhaust. Heard through the reference's listener (spec.sound.model 0: no
    // places, no distance; card L carries the path to the ear), each voice alone (the others at -60 dB) less the same
    // run with every voice silent: the voices' sum cancels exactly, and what is left, the reference's mix, is the
    // exhaust's above all. The fan switched at 300 K and the air at 301 K (the coolant starts at the air's) so it runs
    // from the start, as at a warm idle (its sound does not depend on the coolant; the air 13 K warmer than first.json's
    // moves the exhaust's level by its density, some 4 %). The bounds are judgement (docs/models/voices.md, "Levels at idle"):
    //   pump        an in-tank pump is made to go unheard in a quiet cabin: 40 dB(A) and 25 dB in every octave under
    //   fan         an electric fan is heard in front of an idling car: 15 to 35 dB(A) under, its highs falling (12 dB
    //               under at 4 and 8 kHz)
    //   injectors   a direct injector's tick is the one heard beside the car: 25 to 45 dB(A) under, within 25 dB in
    //               the 4 kHz octave
    //   belt        the alternator and the pumps at idle: 45 dB(A) under
    const char* levels[] = {"voices.pump", "voices.fanLevel", "voices.injectors", "voices.belt", "voices.whine",
                            "voices.axle", "voices.rattle", "voices.timingLevel", "voices.valveLevel", "voices.gate"};
    auto run = [&](const char* alone) {
        tee::test::Scenario s("first");
        s.rate(48000.0);
        s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On).spec("voices.model", 1.0)
            .spec("sound.model", 0.0).spec("voices.fanOn", 300.0).spec("env.temperature", 301.0);
        for (const char* l : levels)
            s.spec(l, alone != nullptr && std::string(l) == alone ? 0.0 : -60.0);
        s.at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On);
        return s.run(5.5);
    };
    const auto quiet = run(nullptr);
    INFO(quiet.error());
    REQUIRE(quiet.ok());
    const auto ref = quiet.audio().during(3.5, 5.5).spectrum();
    const double fcs[] = {0.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0};
    auto against = [&](const char* alone, double* out) {
        const auto on = run(alone);
        const auto d = diff(on, quiet, 3.5, 5.5);
        const auto s = tee::measure::spectrum(d, 48000.0);
        for (int i = 0; i < 7; ++i)
            out[i] = bandDb(s, fcs[i]) - bandDb(ref, fcs[i]);
        UNSCOPED_INFO(alone << ": A " << out[0] << ", 250 " << out[1] << ", 500 " << out[2] << ", 1k " << out[3] << ", 2k "
                            << out[4] << ", 4k " << out[5] << ", 8k " << out[6] << " dB");
    };
    double pump[7], fan[7], inj[7], belt[7];
    against("voices.pump", pump);
    against("voices.fanLevel", fan);
    against("voices.injectors", inj);
    against("voices.belt", belt);
    CHECK(pump[0] < -40.0);
    for (int i = 1; i < 7; ++i)
        CHECK(pump[i] < -25.0);
    CHECK(fan[0] < -15.0);
    CHECK(fan[0] > -35.0);
    CHECK(fan[5] < -12.0);
    CHECK(fan[6] < -12.0);
    CHECK(inj[0] < -25.0);
    CHECK(inj[0] > -45.0);
    CHECK(inj[5] > -25.0);
    CHECK(belt[0] < -45.0);
}
