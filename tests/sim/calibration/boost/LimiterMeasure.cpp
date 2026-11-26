// The rally five on the bench, flat out on its limiter (spark cut, anti-lag on): the boost's swing block by block,
// with the shaft, the gate, the housing and the exhaust's books beside it. Hidden ([.][measure]); environment:
// PROBE_BANG sets exhaust.bang (0 Carved, 1 Burned), PROBE_TARGET boost.target (Pa; 0 the spring alone),
// PROBE_ANTILAG ecu.antiLag.on. Card N2's half of the next window's item 1 (docs/measurements/calibration/boost/).
#include "tee/Build.h"
#include "tee/EnginePlayer.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::unique_ptr<tee::EnginePlayer> limiterPlayer()
    {
        tee::EnginePlayer::Options options;
        options.directory = std::string(TEE_SOURCE_DIR) + "/sim";
        options.sampleRate = 48000.0;
        std::string errors;
        auto player = tee::EnginePlayer::create(options, errors);
        if (!player)
            throw std::runtime_error("sim/engine.fm does not compile:\n" + errors);
        return player;
    }

    tee::EngineBuild limiterBuild(const tee::EnginePlayer& player, const std::string& name)
    {
        tee::EngineBuild build;
        std::string error, warnings;
        if (!tee::loadBuild(player, std::string(TEE_SOURCE_DIR) + "/content/builds/" + name + ".json", build, error, warnings))
            throw std::runtime_error(error);
        return build;
    }
} // namespace

TEST_CASE("measure: the rally five's boost on its limiter, block by block", "[.][measure]")
{
    auto player = limiterPlayer();
    tee::applyBuild(*player, limiterBuild(*player, "archetypes/inline-five-turbo-rally"));
    auto set = [&](const char* path, const char* env) {
        if (const char* v = std::getenv(env))
        {
            const int i = player->specIndex(path);
            REQUIRE(i >= 0);
            player->setSpec(static_cast<std::size_t>(i), std::atof(v), 0);
        }
    };
    set("exhaust.bang", "PROBE_BANG");
    set("boost.target", "PROBE_TARGET");
    set("ecu.antiLag.on", "PROBE_ANTILAG");
    auto rd = [&](const std::string& path) {
        double v = 0.0;
        std::string error;
        if (!player->read(path, v, error))
        {
            std::printf("(%s: %s)\n", path.c_str(), error.c_str());
            return -1.0;
        }
        return v;
    };
    std::vector<float> l(512), r(512);
    auto run = [&](double s) {
        for (int b = 0; b < static_cast<int>(s * 48000.0 / 512.0 + 0.5); ++b)
            player->process(l.data(), r.data(), 512);
    };
    player->controls().mode = tee::EnginePlayer::kModeBench;
    player->controls().ignition = tee::EnginePlayer::kIgnitionStart;
    run(1.5);
    player->controls().ignition = tee::EnginePlayer::kIgnitionOn;
    run(0.5);
    player->controls().throttle = 1.0f;
    run(3.0);
    // 2 s, one row a block (10.7 ms)
    std::vector<double> boost, rpm;
    double books = 0.0;
    int nb = 0;
    for (int b = 0; b < 188; ++b)
    {
        player->process(l.data(), r.data(), 512);
        const double p = rd("tel.boost.pressure") / 1e5;
        boost.push_back(p);
        rpm.push_back(rd("crank.rpm"));
        const double br = rd("exh.bookRatio");
        if (br > 0.0)
        {
            books += br;
            ++nb;
        }
        if (std::getenv("PROBE_ROWS"))
            std::printf("%.3f s rpm %.0f boost %.3f bar | shaft %.0f rad/s gate %.3f | housing %.2f bar %.0f K | heat %.0f kW | eflags %.0f bflags %.0f\n",
                        5.0 + (b + 1) * 512.0 / 48000.0, rpm.back(), p, rd("boost.mach.shaft[0].omega"), rd("boost.mach.turb[0].lift"),
                        rd("boost.mach.house[0].p") / 1e5, rd("boost.mach.house[0].t"), rd("exh.b.bangRate") / 1e3,
                        rd("tel.engine.flags"), rd("tel.boost.flags"));
    }
    // the swing: its range, and its period from the boost's upward crossings of its mean
    const auto [lo, hi] = std::minmax_element(boost.begin(), boost.end());
    double mean = 0.0;
    for (double v : boost)
        mean += v;
    mean /= static_cast<double>(boost.size());
    std::vector<int> up;
    for (std::size_t i = 1; i < boost.size(); ++i)
        if (boost[i - 1] < mean && boost[i] >= mean)
            up.push_back(static_cast<int>(i));
    const double period = up.size() > 1 ? (up.back() - up.front()) * 512.0 / 48000.0 / static_cast<double>(up.size() - 1) : 0.0;
    const auto [rlo, rhi] = std::minmax_element(rpm.begin(), rpm.end());
    std::printf("boost %.3f to %.3f bar (mean %.3f), %zu cycles, period %.3f s | rpm %.0f to %.0f | books %.3f\n", *lo, *hi, mean,
                up.size() > 0 ? up.size() - 1 : 0, period, *rlo, *rhi, nb > 0 ? books / nb : -1.0);
}
