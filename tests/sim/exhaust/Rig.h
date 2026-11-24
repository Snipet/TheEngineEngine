// tests/sim/exhaust/Rig.h: the exhaust builder's harness (harness.fm) as the tests reach it, and what they measure.
//
// The harness is compiled once per run (some seconds) and each test makes its own instance of it: the builder and the
// reference's exhaust fitted to one of the reference's layouts, driven by flows the test makes up (blowdown pulses at
// the layout's firing angles, a steady flow, an impulse). Spectra are Goertzel's at the frequencies asked for.
#pragma once

#include "tee/Denormals.h"
#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace tee::test::exhaust
{
    constexpr int kPorts = 16;
    constexpr double kRate = 96000.0;

    inline tee::Program& program()
    {
        static std::unique_ptr<tee::Program> p = [] {
            std::string errors;
            tee::Program::Options options;
            auto made = tee::Program::fromDirectory(TEE_SOURCE_DIR, "tests/sim/exhaust/harness.fm", options, errors);
            INFO(errors);
            REQUIRE(made != nullptr);
            REQUIRE(made->link(errors));
            return made;
        }();
        return *p;
    }

    class Rig
    {
    public:
        explicit Rig(int layout)
        {
            tee::Program& p = program();
            std::string error;
            schema_ = tee::Schema::of(p.fminus(), "XTest", error);
            INFO(error);
            REQUIRE(schema_ != nullptr);
            const auto* module = p.fminus().module("XTest");
            REQUIRE(module != nullptr);
            inst_ = std::make_unique<tee::Instance>(*module);
            init_ = p.function<void(void*, float)>("XTest::init", error);
            end_ = p.function<void(void*)>("XTest::end", error);
            engine_ = p.function<void(void*, int)>("XTest::engine", error);
            configure_ = p.function<void(void*)>("XTest::configure", error);
            noise_ = p.function<void(void*, float)>("XTest::noise", error);
            dials_ = p.function<void(void*, float, float, float, float)>("XTest::dials", error);
            runB_ = p.function<void(void*, float*, float*, float*, int)>("XTest::runB", error);
            runR_ = p.function<void(void*, float*, float*, float*, int)>("XTest::runR", error);
            runE_ = p.function<void(void*, float*, float*, int)>("XTest::runE", error);
            flowIn_ = p.function<float(void*)>("XTest::flowIn", error);
            flowOut_ = p.function<float(void*)>("XTest::flowOut", error);
            ends_ = p.function<int(void*)>("XTest::ends", error);
            fire_ = p.function<float(void*, int)>("XTest::fireAngle", error);
            portOf_ = p.function<int(void*, int)>("XTest::portOf", error);
            cylinders_ = p.function<int(void*)>("XTest::cylinders", error);
            pMuf_ = p.function<float(void*, int)>("XTest::pMuf", error);
            pConv_ = p.function<float(void*, int)>("XTest::pConv", error);
            valve_ = p.function<float(void*, int)>("XTest::valve", error);
            INFO(error);
            REQUIRE(valve_ != nullptr);
            tee::ScopedFlushDenormals flush;
            init_(inst_->data(), static_cast<float>(kRate));
            engine_(inst_->data(), layout);
        }

        ~Rig()
        {
            tee::ScopedFlushDenormals flush;
            end_(inst_->data());
        }

        Rig(const Rig&) = delete;
        Rig& operator=(const Rig&) = delete;

        void set(const std::string& path, double value)
        {
            std::string error;
            const bool ok = inst_->write(*schema_, path, value, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
        }

        double get(const std::string& path) const
        {
            std::string error;
            double v = 0.0;
            const bool ok = inst_->read(*schema_, path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
            return v;
        }

        void noise(double k) { noise_(inst_->data(), static_cast<float>(k)); }

        void rebuild()
        {
            std::string error;
            auto* fn = program().function<void(void*)>("XTest::rebuild", error);
            INFO(error);
            REQUIRE(fn != nullptr);
            tee::ScopedFlushDenormals flush;
            fn(inst_->data());
        }

        // Any method of XTest's taking two floats, or none and giving one
        void call(const std::string& name, float a, float b)
        {
            std::string error;
            auto* fn = program().function<void(void*, float, float)>(name, error);
            INFO(error);
            REQUIRE(fn != nullptr);
            fn(inst_->data(), a, b);
        }
        double callf(const std::string& name)
        {
            std::string error;
            auto* fn = program().function<float(void*)>(name, error);
            INFO(error);
            REQUIRE(fn != nullptr);
            return fn(inst_->data());
        }
        void dials(float hdr, float pipe, float xp, float muf) { dials_(inst_->data(), hdr, pipe, xp, muf); }
        void configure()
        {
            tee::ScopedFlushDenormals flush;
            configure_(inst_->data());
        }

        // Runs the builder (or the reference's exhaust) on q (rows of kPorts flows), out into left and right
        void run(std::vector<float>& q, std::vector<float>& left, std::vector<float>& right, bool reference = false)
        {
            const int n = static_cast<int>(q.size() / kPorts);
            left.assign(static_cast<std::size_t>(n), 0.0f);
            right.assign(static_cast<std::size_t>(n), 0.0f);
            tee::ScopedFlushDenormals flush;
            const int block = 512;
            for (int t = 0; t < n; t += block)
            {
                const int m = std::min(block, n - t);
                auto fn = reference ? runR_ : runB_;
                fn(inst_->data(), q.data() + static_cast<std::size_t>(t) * kPorts, left.data() + t, right.data() + t, m);
                // (the harness sums a block's flows in floats; the run's sums are kept here, in doubles)
                if (!reference)
                {
                    inSum_ += flowIn_(inst_->data());
                    outSum_ += flowOut_(inst_->data());
                }
            }
        }

        // The waves in flight in the builder's pipes after the last block (up to a constant): runs one sample a block
        double energyAfter(std::vector<float>& q, std::vector<double>& energy)
        {
            const int n = static_cast<int>(q.size() / kPorts);
            energy.assign(static_cast<std::size_t>(n), 0.0);
            std::vector<float> e(static_cast<std::size_t>(n), 0.0f);
            tee::ScopedFlushDenormals flush;
            runE_(inst_->data(), q.data(), e.data(), n);
            double peak = 0.0;
            for (int t = 0; t < n; ++t)
            {
                energy[static_cast<std::size_t>(t)] = e[static_cast<std::size_t>(t)];
                peak = std::max(peak, energy[static_cast<std::size_t>(t)]);
            }
            return peak;
        }

        // The flow in at the ports and out at the ends (m^3/s samples) since the last call
        double flowIn()
        {
            const double v = inSum_ + flowIn_(inst_->data());
            inSum_ = 0.0;
            return v;
        }
        double flowOut()
        {
            const double v = outSum_ + flowOut_(inst_->data());
            outSum_ = 0.0;
            return v;
        }
        int ends() { return ends_(inst_->data()); }
        int cylinders() { return cylinders_(inst_->data()); }
        float fireAngle(int c) { return fire_(inst_->data(), c); }
        int portOf(int c) { return portOf_(inst_->data(), c); }
        double pMuf(int s) { return pMuf_(inst_->data(), s); }
        double pConv(int s) { return pConv_(inst_->data(), s); }
        double valve(int s) { return valve_(inst_->data(), s); }

    private:
        std::unique_ptr<tee::Schema> schema_;
        double inSum_ = 0.0;
        double outSum_ = 0.0;
        std::unique_ptr<tee::Instance> inst_;
        void (*init_)(void*, float) = nullptr;
        void (*end_)(void*) = nullptr;
        void (*engine_)(void*, int) = nullptr;
        void (*configure_)(void*) = nullptr;
        void (*noise_)(void*, float) = nullptr;
        void (*dials_)(void*, float, float, float, float) = nullptr;
        void (*runB_)(void*, float*, float*, float*, int) = nullptr;
        void (*runR_)(void*, float*, float*, float*, int) = nullptr;
        void (*runE_)(void*, float*, float*, int) = nullptr;
        float (*flowIn_)(void*) = nullptr;
        float (*flowOut_)(void*) = nullptr;
        int (*ends_)(void*) = nullptr;
        float (*fire_)(void*, int) = nullptr;
        int (*portOf_)(void*, int) = nullptr;
        int (*cylinders_)(void*) = nullptr;
        float (*pMuf_)(void*, int) = nullptr;
        float (*pConv_)(void*, int) = nullptr;
        float (*valve_)(void*, int) = nullptr;
    };

    // Made-up exhaust flows: each cylinder's blowdown at its firing angle (exhaust opening 130 degrees after its
    // firing, 240 degrees open: a sharp pulse a fifth of the way in, the piston's displacement after it), peak m^3/s,
    // and a steady flow on every port besides. seconds at rpm.
    inline std::vector<float> pulses(Rig& rig, double rpm, double seconds, double peak, double steady = 0.0)
    {
        const int n = static_cast<int>(seconds * kRate);
        std::vector<float> q(static_cast<std::size_t>(n) * kPorts, 0.0f);
        const int cyl = rig.cylinders();
        const double degPerSample = rpm / 60.0 * 360.0 / kRate;
        for (int t = 0; t < n; ++t)
        {
            const double theta = std::fmod(t * degPerSample, 720.0);
            for (int c = 0; c < cyl; ++c)
            {
                double a = theta - rig.fireAngle(c) - 130.0;
                a = std::fmod(a + 1440.0, 720.0);
                if (a < 240.0)
                {
                    const double x = a / 240.0;
                    const double blow = std::exp(-((x - 0.12) / 0.07) * ((x - 0.12) / 0.07));
                    const double disp = std::sin(3.14159265358979 * x);
                    q[static_cast<std::size_t>(t) * kPorts + rig.portOf(c)] +=
                        static_cast<float>(peak * (0.75 * blow + 0.25 * disp));
                }
            }
            if (steady != 0.0)
            {
                for (int p = 0; p < kPorts; ++p)
                    q[static_cast<std::size_t>(t) * kPorts + p] += static_cast<float>(steady);
            }
        }
        return q;
    }

    // The flows of the ports in use, held at `each` m^3/s
    inline std::vector<float> steadyFlow(int ports, double seconds, double each)
    {
        const int n = static_cast<int>(seconds * kRate);
        std::vector<float> q(static_cast<std::size_t>(n) * kPorts, 0.0f);
        for (int t = 0; t < n; ++t)
            for (int p = 0; p < ports; ++p)
                q[static_cast<std::size_t>(t) * kPorts + p] = static_cast<float>(each);
        return q;
    }

    // Goertzel's power at f over x[from, from + len): the mean square of that component
    inline double tone(const std::vector<float>& x, double f, std::size_t from, std::size_t len)
    {
        const double w = 2.0 * 3.14159265358979 * f / kRate;
        const double cw = 2.0 * std::cos(w);
        double s1 = 0.0;
        double s2 = 0.0;
        for (std::size_t i = 0; i < len; ++i)
        {
            // a Hann window: the lines between the bins do not leak into each other
            const double h = 0.5 - 0.5 * std::cos(2.0 * 3.14159265358979 * static_cast<double>(i) / static_cast<double>(len));
            const double s = x[from + i] * h + cw * s1 - s2;
            s2 = s1;
            s1 = s;
        }
        const double p = s1 * s1 + s2 * s2 - cw * s1 * s2;
        return p / (static_cast<double>(len) * static_cast<double>(len)) * 2.0 / 0.375 * 2.0;
    }

    inline double rms(const std::vector<float>& x, std::size_t from, std::size_t len)
    {
        double s = 0.0;
        for (std::size_t i = from; i < from + len; ++i)
            s += static_cast<double>(x[i]) * x[i];
        return std::sqrt(s / static_cast<double>(len));
    }

    inline double db(double ratio) { return 10.0 * std::log10(std::max(ratio, 1e-30)); }

    // The power above f (Hz): x high-passed by a two-pole Butterworth at f (bilinear), its mean square
    inline double powerAbove(const std::vector<float>& x, double f, std::size_t from, std::size_t len)
    {
        const double k = std::tan(3.14159265358979 * f / kRate);
        const double q = 0.70710678;
        const double norm = 1.0 / (1.0 + k / q + k * k);
        const double b0 = norm;
        const double b1 = -2.0 * norm;
        const double b2 = norm;
        const double a1 = 2.0 * (k * k - 1.0) * norm;
        const double a2 = (1.0 - k / q + k * k) * norm;
        double z1 = 0.0;
        double z2 = 0.0;
        double s = 0.0;
        for (std::size_t i = 0; i < from + len; ++i)
        {
            const double in = x[i];
            const double y = b0 * in + z1;
            z1 = b1 * in - a1 * y + z2;
            z2 = b2 * in - a2 * y;
            if (i >= from)
                s += y * y;
        }
        return s / static_cast<double>(len);
    }
} // namespace tee::test::exhaust
