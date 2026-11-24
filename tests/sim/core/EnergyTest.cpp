// tests/sim/core/EnergyTest.cpp: one cylinder's energy balance over cycles, from the model's own state sample by
// sample, with bounds, so it cannot drift unnoticed as packages land; and the port gas's temperature at full load
// (docs/models/mechanics.md). The engine on the dyno at 4000 rpm, full throttle, warm (oil 100 C).
#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
    double gammaAt(double tk) { return std::fmin(std::fmax(1.39 - 7.5e-5 * (tk - 300.0), 1.24), 1.40); }
    double cpAt(double tk) { const double g = gammaAt(tk); return 287.0 * g / (g - 1.0); }

    struct Balance
    {
        double heat = 0, work = 0, wall = 0, netOut = 0, du = 0, tOut = 0, tEvo = 0, tPort = 0;
        int cycles = 0;
    };

    // The build at 4000 rpm on the dyno, full throttle, warm; cylinder 1 over 8 cycles (per cycle, J)
    Balance balance(const std::string& build, double displacement = 0.0, int layout = -1)
    {
        tee::EnginePlayer::Options options;
        options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 96000.0;      // one engine sample a call
        std::string errors;
        auto p = tee::EnginePlayer::create(options, errors);
        INFO(errors);
        REQUIRE(p);
        tee::EngineBuild b;
        std::string warnings;
        REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / (build + ".json"), b, errors, warnings));
        tee::applyBuild(*p, b);
        if (layout >= 0)
            REQUIRE(p->setSpec("engine.layout", layout, errors));
        if (displacement > 0.0)
            REQUIRE(p->setSpec("engine.displacement", displacement, errors));
        std::vector<float> l(1024), r(1024);
        auto play = [&](double s) { const int n = int(s * 96000 / 1024); for (int i = 0; i < n; ++i) p->process(l.data(), r.data(), 1024); };
        auto rd = [&](const char* path) { double v = 0; std::string e; REQUIRE(p->read(path, v, e)); return v; };
        auto wr = [&](const char* path, double v) { std::string e; REQUIRE(p->instance().write(p->schema(), path, v, e)); };
        auto& ctl = p->controls();
        ctl.ignition = 2;
        play(1.0);
        ctl.ignition = 1;
        play(1.0);
        wr("heat.tCool", 363);
        wr("heat.tOil", 373);
        wr("heat.tSurf", 450);
        ctl.mode = 2;
        ctl.throttle = 1.0f;
        ctl.dynoSpeed = float(4000.0 / 9.5492966);
        play(8.0);
        Balance out;
        const double dt = 1.0 / 96000.0;
        double hOut = 0, hIn = 0, mOut = 0, tmOut = 0, tEvoSum = 0, u0 = 0;
        int nEvo = 0;
        bool counting = false;
        double lastAng = rd("cyl[0].ang"), lastV = rd("cyl[0].v"), lastP = rd("cyl[0].p");
        for (int i = 0; i < 96000; ++i)
        {
            p->process(l.data(), r.data(), 1);
            const double ang = rd("cyl[0].ang"), pc = rd("cyl[0].p"), t = rd("cyl[0].t"), v = rd("cyl[0].v");
            const double u = pc * v / (gammaAt(t) - 1.0);
            if (ang < lastAng)
            {
                if (counting)
                {
                    out.cycles += 1;
                    out.du += u - u0;
                }
                counting = true;
                u0 = u;
                if (out.cycles >= 8)
                    break;
            }
            if (counting)
            {
                out.work += 0.5 * (pc + lastP) * (v - lastV);
                out.wall += rd("cyl[0].hA") * (t - rd("cyl[0].tw")) * dt;
                const double mE = rd("cyl[0].mE");
                if (mE > 0)
                {
                    hOut += cpAt(t) * t * mE * dt;
                    mOut += mE * dt;
                    tmOut += t * mE * dt;
                }
                else
                    hOut += cpAt(800) * 800 * mE * dt;     // (back from the header: its gas, roughly)
                const double qI = rd("cyl[0].qI");
                const double pp = rd("intk.pPlenum"), ta = rd("intk.tAir");
                if (qI < 0)
                    hIn += cpAt(ta) * ta * (-qI * pp / (287.0 * ta)) * dt;
                else
                    hOut += cpAt(t) * t * (qI * pp / (287.0 * t)) * dt;
                if (rd("cyl[0].evoNow") != 0)
                {
                    tEvoSum += t;
                    nEvo += 1;
                }
                if (rd("cyl[0].sparkNow") != 0)
                    out.heat += rd("cyl[0].qTot");
            }
            lastAng = ang;
            lastV = v;
            lastP = pc;
        }
        const double n = out.cycles;
        out.heat /= n;
        out.work /= n;
        out.wall /= n;
        out.netOut = (hOut - hIn) / n;
        out.du /= n;
        out.tOut = tmOut / mOut;
        out.tEvo = tEvoSum / std::max(nEvo, 1);
        out.tPort = rd("exh.tPort");
        return out;
    }

    void check(const char* name, const Balance& b)
    {
        const double sum = (b.work + b.wall + b.netOut + b.du) / b.heat;
        std::printf("%s, 4000 rpm, full throttle, warm, cylinder 1 over %d cycles: burn's heat %.0f J; work %.0f %%, walls %.0f %%, "
                    "net enthalpy out %.0f %%, sum %.3f; the gas out %.0f K (mass-weighted), at the exhaust's opening %.0f K; "
                    "tPort %.0f K\n",
                    name, b.cycles, b.heat, 100 * b.work / b.heat, 100 * b.wall / b.heat, 100 * b.netOut / b.heat, sum, b.tOut,
                    b.tEvo, b.tPort);
        CHECK(b.cycles == 8);
        // the balance closes (the intake's enthalpy is estimated from its volume flow at the plenum's state: a few %)
        CHECK(sum > 0.95);
        CHECK(sum < 1.10);
        // the shares of the burn's heat (qTot: already the reference's 80 % of the fuel's at full load): work 35 to 55 %,
        // the walls 5 to 25 %, the exhaust 35 to 60 %
        CHECK(b.work / b.heat > 0.35);
        CHECK(b.work / b.heat < 0.55);
        CHECK(b.wall / b.heat > 0.05);
        CHECK(b.wall / b.heat < 0.25);
        CHECK(b.netOut / b.heat > 0.35);
        CHECK(b.netOut / b.heat < 0.60);
        // the port gas at full load: a petrol engine's 900 to 1100 K, and the reference's own up to some 1350 K
        // (docs/models/mechanics.md: its wall share is low; the 20 % it takes from the burn is taken up front)
        CHECK(b.tPort > 900.0);
        CHECK(b.tPort < 1400.0);
        CHECK(std::abs(b.tPort - b.tOut) < 0.1 * b.tOut);
    }
} // namespace

TEST_CASE("energy: the default V8's cylinder balances, and its port gas is hot at full load", "[core][engine][energy]")
{
    check("default V8", balance("default"));
}

TEST_CASE("energy: a 2 L inline four's cylinder balances, and its port gas is hot at full load", "[core][engine][energy]")
{
    check("2 L inline four", balance("default", 0.002, 5));
}
