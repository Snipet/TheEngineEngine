// tests/sim/calibration/na/BoostInletTest.cpp: card N1, acting for F (the boost machine's model-on path, the lead's
// 21:12 and 21:25), holds X's machine-far-larger-than-its-engine findings. N2's 9a714998 solves the orifices that end in
// the inlet with its stiffness (X-024 and most of X-026); the last, corner seed 298, was a displacement blower's
// clearance leak running back into its charge pipe entered as air at 0 K (sim/boost/machine.fm, card N1): the pipe cooled
// to its floor and went non-finite while cranking. Held: at rest nothing flows and nothing moves; from the key, idle and
// blips without the guard's fault, everything finite, the charge pipe under a megapascal.
#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace
{
    void machineHolds(const char* finding, double seconds)
    {
        CAPTURE(finding);
        tee::EnginePlayer::Options options;
        options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors, warnings;
        auto p = tee::EnginePlayer::create(options, errors);
        INFO(errors);
        REQUIRE(p != nullptr);
        tee::EngineBuild b;
        REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "combo" / "findings" /
                                       (std::string(finding) + ".json"),
                               b, errors, warnings));
        tee::applyBuild(*p, b);
        auto read = [&](const char* path) {
            double v = 0.0;
            std::string e;
            return p->read(path, v, e) ? v : std::nan("");
        };
        auto& c = p->controls();
        const int block = 256;
        float l[block], r[block];
        // at rest: the key off for half a second; the charge path stays at the outside's pressure, nothing flows
        double most = 0.0;
        for (int i = 0; i < int(0.5 * options.sampleRate / block); ++i)
        {
            p->process(l, r, block);
            most = std::fmax(most, std::fabs(read("boost.mach.v1.p") - read("boost.mach.v2.p")));
        }
        INFO("at rest: the charge pipe's and the plenum side's pressures differ at most " << most << " Pa, the cooler's flow "
             << read("boost.mach.mIc") << " kg/s");
        CHECK(most < 1.0);
        CHECK(std::fabs(read("boost.mach.mIc")) < 1.0e-6);
        const int flags = p->telemetryIndex("engine.flags");
        int faults = 0, bad = 0;
        double top = 0.0, least = 1.0e9;
        for (int i = 0; i < int(seconds * options.sampleRate / block); ++i)
        {
            const double t = i * block / options.sampleRate;
            c.ignition = t < 1.0 ? 2 : 1;
            c.throttle = t > 1.5 && std::fmod(t, 1.0) < 0.4 ? 1.0f : 0.0f;
            p->process(l, r, block);
            if ((int(p->telemetry(std::size_t(flags))) & 4) != 0)
                ++faults;
            for (int k = 0; k < block; ++k)
                if (!std::isfinite(l[k]) || !std::isfinite(r[k]))
                    ++bad;
            top = std::fmax(top, read("boost.mach.v1.p"));
            if (t > 1.0)
                least = std::fmin(least, read("boost.mach.v2.p"));
        }
        std::printf("%s: faults %d, non-finite samples %d, the charge pipe's most %.0f kPa, before the throttle the least %.1f kPa\n",
                    finding, faults, bad, top * 1e-3, least * 1e-3);
        CHECK(faults == 0);
        CHECK(bad == 0);
        CHECK(top < 1.0e6);
        // (the volume before the throttle never drawn toward vacuum: with intk.pBox read every 16 samples, X-022's blips
        // pulled it to 0 kPa (195 faults at f69bf016; at 92fe75ae none, by luck); read each sample (D's b7e64fa0) its
        // least is some 48 kPa, the other builds' 98. 20 kPa: judgement, between)
        CHECK(least > 20000.0);
    }
} // namespace

// (the fast tier, the lead's 21:51: the three builds that have traded faults today in one case, so that a fix for one
// cannot undo another: X-022's throttles a runner behind a tiny charge pipe, X-024's huge inducer, X-026's 50 cc blower)
TEST_CASE("X-022, X-024 and X-026 together: tiny charge paths start, idle and blip without a fault or a vacuum", "[calibration][na]")
{
    for (const char* f : {"x022-supercharger-itb-small-charge", "x024-tiny-engine-huge-inducer", "x026-single-small-pipe"})
        machineHolds(f, 3.0);
}

TEST_CASE("X-024 and X-026: every reduced build holds (the full form)", "[nightly][calibration][na]")
{
    for (const char* f : {"x024-tiny-engine-huge-inducer", "x026-redline", "x026-single-big-blower", "x026-single-small-pipe",
                          "x026-tiny-engine-supercharger", "x026-seed298-leak-back", "x022-supercharger-itb-small-charge"})
        machineHolds(f, 6.0);
}
