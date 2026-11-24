// tests/sim/core/EnergyExplore.cpp: the port gas's temperature after a start, printed (hidden: "[.tport]", "[.tportf]").
// The balance itself is a test: EnergyTest.cpp.
#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

TEST_CASE("tPort over time", "[.tport]")
{
    for (int four : {0, 1})
    {
        tee::EnginePlayer::Options options;
        options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors;
        auto p = tee::EnginePlayer::create(options, errors);
        REQUIRE(p);
        tee::EngineBuild b;
        std::string warnings;
        REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "default.json", b, errors, warnings));
        tee::applyBuild(*p, b);
        if (four)
        {
            REQUIRE(p->setSpec("engine.layout", 5, errors));
            REQUIRE(p->setSpec("engine.displacement", 0.002, errors));
        }
        std::vector<float> l(512), r(512);
        auto& ctl = p->controls();
        auto rd = [&](const char* path) { double v = 0; std::string e; p->read(path, v, e); return v; };
        ctl.ignition = 2;
        for (int i = 0; i < 94; ++i) p->process(l.data(), r.data(), 512);
        ctl.ignition = 1;
        ctl.throttle = 1.0f; ctl.mode = 2; ctl.dynoSpeed = float(4000 / 9.5492966);
        std::printf("%s, cold start, then full throttle on the dyno at 4000 rpm:\n", four ? "2 L inline four" : "default V8");
        for (int s = 1; s <= 12; ++s)
        {
            for (int i = 0; i < 94; ++i) p->process(l.data(), r.data(), 512);
            std::printf("  t %2d s: tPort %4.0f K, %5.0f rpm, coolant %.0f K\n", s, rd("exh.tPort"), rd("crank.rpm"), rd("heat.tCool"));
        }
    }
}

TEST_CASE("tPort on F's turbo four", "[.tportf]")
{
    for (int model : {1, 0})
    {
        tee::EnginePlayer::Options options;
        options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors;
        auto p = tee::EnginePlayer::create(options, errors);
        REQUIRE(p);
        tee::EngineBuild b;
        std::string warnings;
        REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "boost" / "turbo-four.json", b, errors, warnings));
        tee::applyBuild(*p, b);
        REQUIRE(p->setSpec("boost.model", model, errors));
        std::vector<float> l(512), r(512);
        auto& ctl = p->controls();
        auto rd = [&](const char* path) { double v = 0; std::string e; p->read(path, v, e); return v; };
        ctl.ignition = 2;
        for (int i = 0; i < 94; ++i) p->process(l.data(), r.data(), 512);
        ctl.ignition = 1;
        ctl.throttle = 1.0f; ctl.mode = 2; ctl.dynoSpeed = float(5500 / 9.5492966);
        std::printf("turbo-four, boost.model %d, full throttle on the dyno at 5500 rpm:\n", model);
        for (int s = 1; s <= 8; ++s)
        {
            for (int i = 0; i < 94; ++i) p->process(l.data(), r.data(), 512);
            std::printf("  t %d s: tPort %4.0f K, %5.0f rpm, boost %.0f kPa, BMEP %.1f bar, cyl1 t %.0f K\n", s, rd("exh.tPort"), rd("crank.rpm"),
                        rd("tel.boost.pressure") / 1000, rd("tel.engine.bmep") / 1e5, rd("cyl[0].t"));
        }
    }
}
