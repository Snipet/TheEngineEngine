// tests/sim/core/EnergyExplore.cpp: one cylinder's energy balance over cycles, from the model's own state, sample by
// sample (hidden: "[.energy]"). The default V8 on the engine dyno, full throttle, warm.
#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
    float gammaAt(double tk) { return float(std::fmin(std::fmax(1.39 - 7.5e-5 * (tk - 300.0), 1.24), 1.40)); }
    double cpAt(double tk) { const double g = gammaAt(tk); return 287.0 * g / (g - 1.0); }
}

TEST_CASE("one cylinder's energy balance", "[.energy]")
{
    tee::EnginePlayer::Options options;
    options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 96000.0;
    std::string errors;
    auto p = tee::EnginePlayer::create(options, errors);
    INFO(errors);
    REQUIRE(p);
    tee::EngineBuild b;
    std::string warnings;
    REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "default.json", b, errors, warnings));
    tee::applyBuild(*p, b);
    std::vector<float> l(1024), r(1024);
    auto play = [&](double s, int block) { const int n = int(s * 96000 / block); for (int i = 0; i < n; ++i) p->process(l.data(), r.data(), block); };
    auto& ctl = p->controls();
    auto rd = [&](const char* path) { double v = 0; std::string e; p->read(path, v, e); return v; };
    auto wr = [&](const char* path, double v) { std::string e; p->instance().write(p->schema(), path, v, e); };
    ctl.ignition = 2; play(1.0, 1024); ctl.ignition = 1; play(1.0, 1024);
    wr("heat.tCool", 363); wr("heat.tOil", 373); wr("heat.tSurf", 450);
    ctl.mode = 2; ctl.throttle = 1.0f; ctl.dynoSpeed = float(4000.0 / 9.5492966);
    play(3.0, 1024);
    const double dt = 1.0 / 96000.0;
    double fuel = 0, work = 0, wall = 0, hOut = 0, hIn = 0, mOut = 0, tmOut = 0, mIn = 0, tEvoSum = 0, du = 0;
    int cycles = 0, nEvo = 0;
    double lastAng = rd("cyl[0].ang"), lastV = rd("cyl[0].v"), lastP = rd("cyl[0].p");
    double u0 = 0;
    bool counting = false;
    double tPortSum = 0, tPortN = 0;
    for (int i = 0; i < 96000 * 1; ++i)
    {
        p->process(l.data(), r.data(), 1);
        const double ang = rd("cyl[0].ang"), pc = rd("cyl[0].p"), t = rd("cyl[0].t"), v = rd("cyl[0].v");
        const double u = pc * v / (gammaAt(t) - 1.0);
        if (ang < lastAng)
        {
            if (counting)
            {
                cycles += 1;
                du += u - u0;
            }
            counting = true;
            u0 = u;
            if (cycles >= 8)
                break;
        }
        if (counting)
        {
            work += 0.5 * (pc + lastP) * (v - lastV);
            const double hA = rd("cyl[0].hA"), tw = rd("cyl[0].tw");
            wall += hA * (t - tw) * dt;
            const double mE = rd("cyl[0].mE");
            if (mE > 0) { hOut += cpAt(t) * t * mE * dt; mOut += mE * dt; tmOut += t * mE * dt; }
            else hOut += cpAt(800) * 800 * mE * dt;     // (back from the header: the pipe's gas, roughly)
            const double qI = rd("cyl[0].qI");
            if (qI < 0)
            {
                const double pp = rd("intk.pPlenum"), ta = rd("intk.tAir");
                const double mi = -qI * pp / (287.0 * ta);
                hIn += cpAt(ta) * ta * mi * dt;
                mIn += mi * dt;
            }
            else
            {
                const double mo = qI * rd("intk.pPlenum") / (287.0 * t);
                hOut += cpAt(t) * t * mo * dt;
            }
            if (rd("cyl[0].evoNow") != 0) { tEvoSum += t; nEvo += 1; }
            if (rd("cyl[0].sparkNow") != 0) fuel += rd("cyl[0].qTot");
            tPortSum += rd("exh.tPort"); tPortN += 1;
        }
        lastAng = ang; lastV = v; lastP = pc;
    }
    const double n = cycles;
    std::printf("default V8, 4000 rpm, full throttle, warm; cylinder 1 over %d cycles, per cycle (J):\n", cycles);
    std::printf("  heat released by the burn (qTot at the spark) %.1f\n", fuel / n);
    std::printf("  indicated work (p dV, net)                     %.1f  (%.0f %%)\n", work / n, 100 * work / fuel);
    std::printf("  heat to the walls                              %.1f  (%.0f %%)\n", wall / n, 100 * wall / fuel);
    std::printf("  enthalpy out (exhaust and intake backflow)     %.1f\n", hOut / n);
    std::printf("  enthalpy in (intake)                           %.1f\n", hIn / n);
    std::printf("  net enthalpy out                               %.1f  (%.0f %%)\n", (hOut - hIn) / n, 100 * (hOut - hIn) / fuel);
    std::printf("  internal energy change over the cycles         %.2f\n", du / n);
    std::printf("  sum (work + wall + net out + du) / heat        %.3f\n", (work + wall + hOut - hIn + du) / fuel);
    std::printf("  mass out the exhaust %.1f mg, in %.1f mg; mass-weighted temperature out %.0f K; at the exhaust's opening %.0f K; tPort %.0f K\n",
                mOut / n * 1e6, mIn / n * 1e6, tmOut / mOut, tEvoSum / std::max(nEvo, 1), tPortSum / tPortN);
}

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
