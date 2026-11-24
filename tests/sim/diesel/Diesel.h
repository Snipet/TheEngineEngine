// tests/sim/diesel/Diesel.h: card D2's harness: the whole simulator (sim/engine.fm through tee::EnginePlayer) with a
// build of content/builds, on the bench, the engine dyno or the road, and what the tests measure from it.
#pragma once

#include "sim/Twin.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <complex>
#include <memory>
#include <string>
#include <vector>

namespace d2
{
    using tee::EnginePlayer;

    struct Engine
    {
        std::unique_ptr<EnginePlayer> player;
        std::vector<float> l = std::vector<float>(512);
        std::vector<float> r = std::vector<float>(512);
        std::vector<float> rec;     // the left channel while recording
        bool recording = false;
        double rate = 48000.0;

        explicit Engine(const std::string& build = "", double rateIn = 48000.0)
        {
            rate = rateIn;
            player = tee::test::enginePlayer(rate);
            player->setSpecDefaults();
            if (!build.empty())
                tee::applyBuild(*player, tee::test::engineBuild(*player, build));
        }

        void spec(const std::string& path, double v)
        {
            std::string error;
            const bool ok = player->setSpec(path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
        }

        double get(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            const bool ok = player->read(path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
            return v;
        }

        double tel(const std::string& name) const { return get("tel." + name); }
        tee::EnginePlayer::Controls& ctl() { return player->controls(); }

        // seconds of sound; false once it stops or writes something not finite or past full scale
        bool run(double seconds)
        {
            const int calls = static_cast<int>(seconds * rate / 512.0 + 0.5);
            for (int k = 0; k < calls; ++k)
            {
                player->process(l.data(), r.data(), 512);
                for (int i = 0; i < 512; ++i)
                    if (!std::isfinite(l[i]) || std::fabs(l[i]) > 1.0f)
                        return false;
                if (recording)
                    rec.insert(rec.end(), l.begin(), l.end());
            }
            return !player->stopped();
        }

        // the mean of a telemetry field over some seconds, read each call
        double mean(const std::string& name, double seconds)
        {
            const int calls = static_cast<int>(seconds * rate / 512.0 + 0.5);
            double sum = 0.0;
            for (int k = 0; k < calls; ++k)
            {
                REQUIRE(run(512.0 / rate));
                sum += tel(name);
            }
            return sum / calls;
        }

        // started on the bench and running
        void start(double crank = 2.0, double settle = 2.0)
        {
            auto& c = ctl();
            c.mode = EnginePlayer::kModeBench;
            c.ignition = EnginePlayer::kIgnitionStart;
            REQUIRE(run(crank));
            c.ignition = EnginePlayer::kIgnitionOn;
            REQUIRE(run(settle));
        }

        // the engine dyno holds the crank at rpm
        void dyno(double rpm, float throttle)
        {
            auto& c = ctl();
            c.mode = 2;
            c.dynoSpeed = static_cast<float>(rpm * 0.10471976);
            c.dynoRate = 0.0f;
            c.throttle = throttle;
        }
    };

    // The level (dB) of a signal's spectrum near a frequency: the most of a Hann-windowed DFT's bins within +-tol Hz
    inline double lineDb(const std::vector<float>& x, double rate, double hz, double tol)
    {
        const size_t n = x.size();
        double best = 0.0;
        const double df = rate / static_cast<double>(n);
        for (double f = hz - tol; f <= hz + tol; f += df * 0.5)
        {
            std::complex<double> s(0.0, 0.0);
            for (size_t i = 0; i < n; ++i)
            {
                const double w = 0.5 - 0.5 * std::cos(6.283185307179586 * static_cast<double>(i) / static_cast<double>(n - 1));
                const double ph = -6.283185307179586 * f * static_cast<double>(i) / rate;
                s += w * x[i] * std::complex<double>(std::cos(ph), std::sin(ph));
            }
            best = std::max(best, std::abs(s) / static_cast<double>(n));
        }
        return 20.0 * std::log10(best + 1e-12);
    }

    inline double rmsDb(const std::vector<float>& x)
    {
        double s = 0.0;
        for (float v : x)
            s += static_cast<double>(v) * v;
        return 10.0 * std::log10(s / std::max<size_t>(x.size(), 1) + 1e-20);
    }
} // namespace d2
