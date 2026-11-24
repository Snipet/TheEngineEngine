// tests/ReferenceTest.cpp: the reference engine, played by tee::RefPlayer, held to what it should do. These are the
// baseline the port is compared with (docs/plan/p0-split.md): what they measure of the reference today is written in
// docs/measurements/p0-bench.md.
//
// Nobody can listen here, so each test claims numbers: samples equal or apart by so many dB, a speed within so many
// rpm, a spectrum's strongest line at a frequency. The tolerances are stated where they are used, each beside what
// was measured when it was set.
#include "support/Audio.h"
#include "support/Ref.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <sstream>

using namespace tee;
using namespace tee::test;

namespace
{
    // The reference's files as they are on disk, for a test that compiles an edited copy.
    Program::Files referenceFiles()
    {
        Program::Files files;
        for (const char* name : {"program.fm", "host.fm", "engine.fm", "cylinder.fm", "pipes.fm", "car.fm", "layouts.fm",
                                 "sounds.fm", "hall.fm", "biquad.fm", "onepole.fm"})
        {
            std::ifstream in(std::filesystem::path(TEE_SOURCE_SIM_DIR) / name, std::ios::binary);
            REQUIRE(in.good());
            std::ostringstream text;
            text << in.rdbuf();
            files[name] = text.str();
        }
        return files;
    }

    // A render is sound: every sample finite and inside full scale, the engine ran, and every number the panel was
    // shown is finite.
    void checkSound(const RenderResult& result)
    {
        CHECK(allFinite(result.audio.left));
        CHECK(allFinite(result.audio.right));
        // the reference ends in a soft ceiling, 0.7 + 0.3 tanh: a sample never reaches 1
        CHECK(result.peak < 1.0);
        CHECK(result.peak > 0.05);
        CHECK(result.soundingFrames > 0);
        CHECK(allFinite(result.rows));
        const int rpm = result.column("showRpm");
        REQUIRE(rpm >= 0);
        float fastest = 0.0f;
        for (std::size_t r = 0; r < result.rowCount(); ++r)
            fastest = std::max(fastest, result.at(r, static_cast<std::size_t>(rpm)));
        CHECK(fastest > 500.0f);        // it ran
        CHECK(fastest < 20000.0f);      // and did not run away
    }
} // namespace

TEST_CASE("the reference compiles with no error and no warning", "[ref][compile]")
{
    const auto player = refPlayer();
    CHECK(player->program().warnings().empty());
    // the eleven files, each read once
    CHECK(player->program().filesRead().size() == 11);
    CHECK(player->program().filesRead().front() == "program.fm");

    // the contract: 32 dials, the panel's twelve show fields
    CHECK(player->dials().size() == 32);
    CHECK(player->dialIndex("engine") == 0);
    CHECK(player->dialIndex("ambient") == 31);
    CHECK(player->showNames().size() == 12);
    CHECK(player->showIndex("showRpm") >= 0);
    CHECK(player->showIndex("showLamps") >= 0);

    // content/ref/presets/defaults.json is the program's own defaults
    const Preset defaults = preset("defaults");
    REQUIRE(defaults.dials.size() == player->dials().size());
    for (std::size_t k = 0; k < defaults.dials.size(); ++k)
    {
        CHECK(defaults.dials[k].first == player->dials()[k].name);
        CHECK(defaults.dials[k].second == Catch::Approx(player->dials()[k].defaultValue));
        CHECK(player->dial(k) == Catch::Approx(player->dials()[k].defaultValue));
    }
    // and every preset names only dials the program has
    for (const char* name : {"v8", "v10", "large-diesel", "bench-v12"})
    {
        const Preset p = preset(name);
        CHECK(p.dials.size() == 32);
        for (const auto& dial : p.dials)
            CHECK(player->dialIndex(dial.first) >= 0);
    }
}

TEST_CASE("start, process and stop are proven nonblocking; init and end are not, and need not be", "[ref][nonblocking]")
{
    // The player calls each audio method through a function marked [[clang::nonblocking]], appended to the program:
    // FMinus compiled it, so it has checked that the method, and everything it reaches (Shared's keyDown, render,
    // configure, every pipe and cylinder), cannot reach malloc, calloc or free.
    const auto player = refPlayer();
    const fminus::Program& fm = player->program().fminus();
    for (const char* function : {"tee_ref_start", "tee_ref_process", "tee_ref_stop"})
    {
        INFO(function);
        CHECK(fm.nonBlocking(function));
        CHECK(fm.nonAllocating(function));
    }
    // What is not: nothing in the reference carries the mark itself, so FMinus says so of each method by name; and
    // Shared's init and end allocate and free (the pipes' delay lines, the resampler's table), off the audio thread.
    CHECK_FALSE(fm.nonBlocking("Voice::process"));
    CHECK_FALSE(fm.nonBlocking("Shared::init"));
    CHECK_FALSE(fm.nonBlocking("Shared::end"));

    // The proof is a real one: the same program with an allocation where process reaches it does not compile.
    Program::Files files = referenceFiles();
    const std::string marker = "        readDials();\n        shared->render(left, right, n);\n";
    const std::size_t at = files["program.fm"].find(marker);
    REQUIRE(at != std::string::npos);
    files["program.fm"].insert(at, "        float* leak = calloc(4, sizeof(float));\n        leak[0] = 0.0f;\n");
    Program::Options options;
    options.appended = RefPlayer::shims();
    std::string errors;
    const auto broken = Program::fromFiles(files, "program.fm", options, errors);
    INFO(errors);
    CHECK(broken == nullptr);
    CHECK(errors.find("nonblocking") != std::string::npos);
    CHECK(errors.find("calloc") != std::string::npos);

    // and the unedited files compile that way too, as RefPlayer compiles them
    files = referenceFiles();
    const auto whole = Program::fromFiles(files, "program.fm", options, errors);
    INFO(errors);
    CHECK(whole != nullptr);
}

TEST_CASE("cold-start-idle renders finite and within full scale", "[ref][scenario]")
{
    const auto player = refPlayer();
    const RenderResult result = rendered(*player, preset("v8"), scenario("cold-start-idle"));
    checkSound(result);
    // cranked, then running to the end: at 20 F it idles fast, above a warm engine's idle
    const std::size_t lamps = static_cast<std::size_t>(result.column("showLamps"));
    const std::size_t rpm = static_cast<std::size_t>(result.column("showRpm"));
    bool cranked = false;
    for (std::size_t r = 0; r < result.rowCount(); ++r)
        cranked = cranked || stateOf(result.at(r, lamps)) == 1;
    CHECK(cranked);
    CHECK(stateOf(result.at(result.rowCount() - 1, lamps)) == 2);
    CHECK(result.at(result.rowCount() - 1, rpm) > 900.0f);
}

TEST_CASE("free-revs renders finite and within full scale, for every preset", "[ref][scenario]")
{
    const auto player = refPlayer();
    for (const char* name : {"defaults", "v8", "v10", "large-diesel", "bench-v12"})
    {
        INFO(name);
        checkSound(rendered(*player, preset(name), scenario("free-revs")));
    }
}

TEST_CASE("drive-gears renders finite and within full scale, and the car drives", "[ref][scenario]")
{
    const auto player = refPlayer();
    const RenderResult result = rendered(*player, preset("v8"), scenario("drive-gears"));
    checkSound(result);
    const std::size_t gear = static_cast<std::size_t>(result.column("showGear"));
    const std::size_t kmh = static_cast<std::size_t>(result.column("showKmh"));
    float topGear = 0.0f;
    float topSpeed = 0.0f;
    for (std::size_t r = 0; r < result.rowCount(); ++r)
    {
        topGear = std::max(topGear, result.at(r, gear));
        topSpeed = std::max(topSpeed, result.at(r, kmh));
    }
    CHECK(topGear == 4.0f);                 // four paddle shifts up
    CHECK(topSpeed > 150.0f);               // measured 218 km/h: a 6.2 L V8 in 1500 kg, flat out for 15 s
    CHECK(topSpeed < 300.0f);
    CHECK(result.at(result.rowCount() - 1, gear) == 0.0f);          // back down to neutral
    CHECK(result.at(result.rowCount() - 1, kmh) < 1.0f);            // and stopped
}

TEST_CASE("truck-pull-away renders finite and within full scale, and the truck pulls away", "[ref][scenario]")
{
    const auto player = refPlayer();
    const RenderResult result = rendered(*player, preset("large-diesel"), scenario("truck-pull-away"));
    checkSound(result);
    const std::size_t gear = static_cast<std::size_t>(result.column("showGear"));
    const std::size_t kmh = static_cast<std::size_t>(result.column("showKmh"));
    const std::size_t rpm = static_cast<std::size_t>(result.column("showRpm"));
    const std::size_t red = static_cast<std::size_t>(result.column("showRed"));
    float topGear = 0.0f;
    float topSpeed = 0.0f;
    for (std::size_t r = 0; r < result.rowCount(); ++r)
    {
        topGear = std::max(topGear, result.at(r, gear));
        topSpeed = std::max(topSpeed, result.at(r, kmh));
        // a diesel's governor: it never runs far past its (low) redline
        CHECK(result.at(r, rpm) < result.at(r, red) + 250.0f);
    }
    CHECK(topGear == 10.0f);                // ten steps up the eighteen
    CHECK(topSpeed > 30.0f);                // measured 46 km/h, 30 t
    CHECK(topSpeed < 90.0f);
}

TEST_CASE("the same samples whatever the block size", "[ref][blocks]")
{
    // 12 s of the V8's free revs (the start, idle, two blips, onto the limiter) at 48 kHz, where the engine's 96 kHz
    // goes through its resampler, and 6 s at 96 kHz, where it does not: calls of 64, 333 and 512 samples give the
    // same samples, bit for bit (measured: no difference at all, at 44.1 kHz and with calls of 1024 either).
    const Preset p = preset("v8");
    const Scenario s = scenario("free-revs");
    for (const double rate : {48000.0, 96000.0})
    {
        INFO("rate " << rate);
        const auto player = refPlayer(rate);
        const double seconds = rate == 48000.0 ? 12.0 : 6.0;
        const RenderResult reference = rendered(*player, p, s, 512, seconds);
        CHECK(reference.peak > 0.1);
        for (const int block : {64, 333})
        {
            INFO("block " << block);
            const RenderResult other = rendered(*player, p, s, block, seconds);
            REQUIRE(other.audio.left.size() == reference.audio.left.size());
            CHECK(maxDifference(reference.audio.left, other.audio.left) == 0.0);
            CHECK(maxDifference(reference.audio.right, other.audio.right) == 0.0);
        }
    }
}

TEST_CASE("with the checks the reference plays the same samples, and a budget too small stops it", "[ref][checks]")
{
    // A development build (the loop budget and the bounds checks on) computes what the release build does, bit for
    // bit, as FMinus promises of a call that stays within its budget and its arrays: 6 s of the V8's free revs.
    const Preset p = preset("v8");
    const Scenario s = scenario("free-revs");
    const auto plain = refPlayer(48000.0, false);
    const auto checked = refPlayer(48000.0, true);
    const RenderResult a = rendered(*plain, p, s, 512, 6.0);
    const RenderResult b = rendered(*checked, p, s, 512, 6.0);
    CHECK(a.peak > 0.1);
    CHECK(maxDifference(a.audio.left, b.audio.left) == 0.0);
    CHECK(maxDifference(a.audio.right, b.audio.right) == 0.0);

    // And the budget is really counted: process, called with ten loop iterations to spend, returns early and says so
    // (the player gives a call 2^32 iterations a second of its samples, which nothing that plays comes near).
    checked->reset();
    checked->start(36.0f, 1.0f);
    REQUIRE_FALSE(checked->stopped());
    std::string errors;
    auto* process = checked->program().function<bool(void*, float*, float*, int)>("tee_ref_process", errors);
    REQUIRE(process != nullptr);
    fminus::Program& fm = checked->program().fminus();
    float left[64] = {};
    float right[64] = {};
    fm.setLoopBudget(10);
    (void)process(checked->voice().data(), left, right, 64);
    CHECK(fm.loopBudgetExceeded());
    CHECK_FALSE(fm.boundsViolation().has_value());
}

TEST_CASE("the same sound at 44.1, 48 and 96 kHz, resampled, to a stated tolerance", "[ref][rates]")
{
    // The engine runs at 96 kHz whatever the host's rate, so the three renders are one simulation heard through
    // three converters, as long as the player's inputs reach it at the same moments. They do not quite: at 48 and
    // 44.1 kHz the engine runs some 50 of its samples ahead of the output (its resampler's look-ahead), so a pedal
    // moved at one time on the clock reaches the engine half a millisecond earlier in the engine's time than at
    // 96 kHz, and the gas is chaotic enough that the waveforms part company a few seconds after the first such
    // movement, while staying the same sound. So the tolerance is in two parts:
    //   - up to the first pedal movement (the start and the idle, 0.3 to 3.9 s): sample for sample, resampled to
    //     48 kHz, the difference at least 30 dB under the signal (measured: 96 kHz 55 dB, 44.1 kHz 41 dB, both the
    //     converters' own differences near 20 kHz);
    //   - over the whole scenario: each second's level within 1.5 dB (measured 0.3 at 96 kHz, 0.001 at 44.1), and
    //     on the limiter each octave from 31 Hz to 16 kHz within 2 dB (measured 0.3 and 0.0001).
    const Preset p = preset("v8");
    const Scenario s = scenario("free-revs");
    const auto at48 = refPlayer(48000.0);
    const RenderResult base = rendered(*at48, p, s);
    const std::size_t second = 48000;
    const std::size_t spectrum = std::size_t{1} << 17;       // 2.7 s
    const std::vector<double> basePower = powerSpectrum(base.audio.left, 9 * second, spectrum);
    for (const double rate : {44100.0, 96000.0})
    {
        INFO("rate " << rate);
        const auto player = refPlayer(rate);
        const RenderResult other = rendered(*player, p, s);
        CHECK(other.audio.left.size() == static_cast<std::size_t>(s.seconds * rate + 0.5));
        const std::vector<float> left = resample(other.audio.left, rate, 48000.0);
        const std::vector<float> right = resample(other.audio.right, rate, 48000.0);
        REQUIRE(left.size() >= 23 * second);

        const auto slice = [&](const std::vector<float>& x) {
            return std::vector<float>(x.begin() + static_cast<long>(0.3 * 48000.0), x.begin() + static_cast<long>(3.9 * 48000.0));
        };
        const double depthLeft = nullDepth(slice(base.audio.left), slice(left));
        const double depthRight = nullDepth(slice(base.audio.right), slice(right));
        WARN("at " << rate << " Hz, 0.3 to 3.9 s: the difference is " << depthLeft << " dB (left) and " << depthRight
                   << " dB (right) under the signal");
        CHECK(depthLeft > 30.0);
        CHECK(depthRight > 30.0);

        double worstLevel = 0.0;
        for (std::size_t t = 1; t + 1 < 20; ++t)             // the engine is running from 1 s to 20 s
            worstLevel = std::max(worstLevel, std::fabs(levelDb(left, t * second, (t + 1) * second)
                                                        - levelDb(base.audio.left, t * second, (t + 1) * second)));
        WARN("at " << rate << " Hz: a second's level differs by at most " << worstLevel << " dB");
        CHECK(worstLevel < 1.5);

        const std::vector<double> power = powerSpectrum(left, 9 * second, spectrum);
        double worstBand = 0.0;
        for (double low = 31.25; low < 16000.0; low *= 2.0)
            worstBand = std::max(worstBand, std::fabs(bandDb(power, 48000.0, low, 2.0 * low)
                                                      - bandDb(basePower, 48000.0, low, 2.0 * low)));
        WARN("at " << rate << " Hz, on the limiter: an octave's level differs by at most " << worstBand << " dB");
        CHECK(worstBand < 2.0);
    }
}

TEST_CASE("the engine starts, idles near its idle speed, reaches its limiter flat out in neutral, and stops",
          "[ref][life]")
{
    // The defaults (the reference's Init V8, a mild cam: its idle is steady, where the presets' big cams lope) on
    // free-revs: in the car, in neutral.
    const auto player = refPlayer();
    // What the engine itself holds as its idle speed while it idles (Shared's idleRpm: its layout's 700 rpm and what
    // an engine not yet warm adds), read through the schema after the scenario's first 3.9 s, before any pedal moves.
    std::string error;
    const auto idleField = player->sharedSchema().field<float>("idleRpm", error);
    REQUIRE(idleField.valid());
    (void)rendered(*player, preset("defaults"), scenario("free-revs"), 512, 3.9);
    const double idle = player->shared().get(idleField);
    CHECK(idle == Catch::Approx(880.0).margin(40.0));

    const RenderResult result = rendered(*player, preset("defaults"), scenario("free-revs"));
    checkSound(result);
    const std::size_t lamps = static_cast<std::size_t>(result.column("showLamps"));
    const std::size_t rpm = static_cast<std::size_t>(result.column("showRpm"));
    const std::size_t red = static_cast<std::size_t>(result.column("showRed"));
    const double redline = result.at(result.rowCount() - 1, red);
    CHECK(redline == Catch::Approx(6500.0).margin(1.0));

    bool cranked = false, ran = false, limited = false, ranDown = false;
    double idleSum = 0.0, idleLow = 1.0e9, idleHigh = 0.0, fastest = 0.0, onLimiterLow = 1.0e9;
    int idleRows = 0;
    for (std::size_t r = 0; r < result.rowCount(); ++r)
    {
        const double t = result.rowTime[r];
        const double speed = result.at(r, rpm);
        const int state = stateOf(result.at(r, lamps));
        cranked = cranked || state == 1;
        ran = ran || state == 2;
        ranDown = ranDown || state == 3;
        limited = limited || (t > 8.0 && t < 14.0 && limiterOf(result.at(r, lamps)));
        fastest = std::max(fastest, speed);
        if (t > 2.5 && t < 3.9)
        {
            idleSum += speed;
            idleLow = std::min(idleLow, speed);
            idleHigh = std::max(idleHigh, speed);
            ++idleRows;
        }
        if (t > 10.0 && t < 13.5)
            onLimiterLow = std::min(onLimiterLow, speed);
    }
    // it starts: the starter cranks it (the key at 0.25 s), then it runs
    CHECK(cranked);
    CHECK(ran);
    // it idles near its idle speed: the mean within 5 %, every reading within 15 % (measured: mean 880 of 880, 828
    // to 928)
    REQUIRE(idleRows > 0);
    WARN("idle: " << idleSum / idleRows << " rpm mean of " << idle << ", " << idleLow << " to " << idleHigh);
    CHECK(idleSum / idleRows == Catch::Approx(idle).epsilon(0.05));
    CHECK(idleLow > 0.85 * idle);
    CHECK(idleHigh < 1.15 * idle);
    // flat out in neutral it reaches its limiter and stays on it: the fuel cut holds it within 150 rpm above the
    // redline and 400 below (measured: 6557 at most, 6305 at least)
    WARN("limiter: fastest " << fastest << " rpm, slowest while held flat out " << onLimiterLow << ", redline " << redline);
    CHECK(limited);
    CHECK(fastest > redline - 50.0);
    CHECK(fastest < redline + 150.0);
    CHECK(onLimiterLow > redline - 400.0);
    // and it stops: the start key held a second switches it off, it runs down, and the voice falls silent for good
    CHECK(ranDown);
    CHECK(stateOf(result.at(result.rowCount() - 1, lamps)) == 0);
    CHECK(result.at(result.rowCount() - 1, rpm) == 0.0f);
    CHECK_FALSE(player->sounding());
    const std::size_t n = result.audio.left.size();
    CHECK(peak(std::vector<float>(result.audio.left.end() - 24000, result.audio.left.end())) == 0.0);
    CHECK(n == static_cast<std::size_t>(24 * 48000));
}

TEST_CASE("the strongest low line at a held speed sits where the layout's firing predicts", "[ref][firing]")
{
    // The defaults, played from the keys: middle C asks for a speed half way up the layout's range, and the engine
    // holds it. From 7 s, over 2.7 s, the left channel's strongest line between 15 Hz and 1.2 kHz is compared with
    // what the layout predicts from the speed the panel showed over the same stretch.
    //
    // A layout that fires evenly predicts its firing frequency: cylinders / 2 a turn of the crank. A V6 and a flat
    // six send each bank down its own pipe, each firing every 240 degrees: half the engine's firing frequency.
    // Measured, each within 0.01 %; the test allows 0.5 %.
    struct Case
    {
        int layout;
        const char* name;
        int cylinders;
        double ofFiring;     // the line predicted, over the firing frequency
    };
    const Case cases[] = {
        {1, "TWIN 360", 2, 1.0}, {4, "INLINE 3", 3, 1.0}, {5, "INLINE 4", 4, 1.0}, {6, "FLAT 4", 4, 1.0},
        {10, "INLINE 6", 6, 1.0}, {14, "V10", 10, 1.0},   {15, "V12", 12, 1.0},    {9, "V6", 6, 0.5},
        {11, "FLAT 6", 6, 0.5},
    };
    // The crossplane V8 fires evenly, but each bank's four pulses come 180, 90, 180 and 270 degrees apart: its lines
    // stand on every multiple of the cycle's frequency (half the crank's), and the strongest is not the firing
    // frequency (measured: the third, 1.5 times the crank's speed: the burble).
    const auto player = refPlayer();
    const int rpmColumn = player->showIndex("showRpm");
    REQUIRE(rpmColumn >= 0);
    const int rate = 48000;
    const int total = 10 * rate;
    const std::size_t from = static_cast<std::size_t>(7 * rate);
    const std::size_t size = std::size_t{1} << 17;
    std::vector<float> left(static_cast<std::size_t>(total));
    std::vector<float> right(static_cast<std::size_t>(total));
    const auto hold = [&](int layout) {
        player->reset();
        player->setDial("engine", layout);
        player->setDial("hold", 60.0);
        player->start(60.0f, 0.8f);
        double sum = 0.0;
        int rows = 0;
        for (int at = 0; at < total; at += 512)
        {
            const int n = std::min(512, total - at);
            player->process(left.data() + at, right.data() + at, n);
            if (static_cast<std::size_t>(at) >= from && static_cast<std::size_t>(at) < from + size)
            {
                sum += player->show(static_cast<std::size_t>(rpmColumn));
                ++rows;
            }
        }
        return sum / rows;       // the mean speed over the analysed stretch, rpm
    };
    for (const Case& c : cases)
    {
        INFO(c.name);
        const double rpm = hold(c.layout);
        const double firing = rpm / 60.0 * c.cylinders / 2.0;
        const Line line = strongestLine(powerSpectrum(left, from, size), rate, 15.0, 1200.0);
        WARN(c.name << ": " << rpm << " rpm, firing " << firing << " Hz, strongest line " << line.hz << " Hz ("
                    << line.hz / firing << " of it)");
        CHECK(rpm > 2000.0);
        CHECK(line.hz == Catch::Approx(c.ofFiring * firing).epsilon(0.005));
    }
    {
        INFO("V8 CROSS");
        const double rpm = hold(12);
        const double cycle = rpm / 120.0;
        const Line line = strongestLine(powerSpectrum(left, from, size), rate, 15.0, 1200.0);
        const double harmonic = line.hz / cycle;
        WARN("V8 CROSS: " << rpm << " rpm, cycle " << cycle << " Hz, strongest line " << line.hz << " Hz (harmonic "
                          << harmonic << " of the cycle; the firing frequency is the 8th)");
        CHECK(std::fabs(harmonic - std::round(harmonic)) < 0.03);
        CHECK(std::round(harmonic) < 8.0);
    }
}
