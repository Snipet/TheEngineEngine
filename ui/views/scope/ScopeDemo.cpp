// ui/views/scope/ScopeDemo.cpp: see ScopeDemo.h. Made-up numbers, plausible in shape: no clock, no randomness.
#include "views/scope/ScopeDemo.h"

#include "views/scope/Spectrum.h"

#include <algorithm>
#include <cmath>

namespace tee::ui::scope
{
    int ScopeDemo::find(std::string_view name) const
    {
        if (name == "portIntake")
            return kIntake;
        if (name == "portExhaust")
            return kExhaust;
        return inner_->find(name);
    }

    int ScopeDemo::read(int handle, float* out, int max)
    {
        if (handle != kIntake && handle != kExhaust)
            return inner_->read(handle, out, max);
        a_.assign(1080, 0.0f);
        b_.assign(1080, 0.0f);
        const int n = inner_->read(inner_->find("pressure"), a_.data(), 1080);
        const int lift = inner_->read(inner_->find(handle == kIntake ? "liftIntake" : "liftExhaust"), b_.data(), 1080);
        if (n <= 0 || lift != n)
            return 0;
        // The manifold's pressure: the cylinder's during the intake stroke's middle (the demo's cylinder reads it there).
        const double manifold = a_[static_cast<std::size_t>(std::min(n - 1, n * 5 / 8))];
        // Where the valve opens: the first degree its lift rises from nothing.
        int open = 0;
        for (int i = 0; i < n; ++i)
            if (b_[static_cast<std::size_t>(i)] > 0.01f && b_[static_cast<std::size_t>((i + n - 1) % n)] <= 0.01f)
                open = i;
        const int m = std::min(n, max);
        for (int i = 0; i < m; ++i)
        {
            const double since = std::fmod(static_cast<double>(i - open + n), static_cast<double>(n));   // degrees
            const double cyl = a_[static_cast<std::size_t>(i)];
            const double l = b_[static_cast<std::size_t>(i)];
            double p;
            if (handle == kExhaust)
            {
                // The blowdown pulse as the valve cracks, then its reflection back from the collector as a rarefaction
                // (the scavenging dip) about 140 degrees on, and smaller echoes after it.
                const double pulse = std::max(0.0, std::min(cyl, 600e3) - 101325.0) * 0.35 * l;
                const double echo = 38e3 * std::exp(-since / 260.0) * std::sin(2.0 * kPi * since / 190.0 + 0.6);
                p = 104e3 + pulse + echo;
            }
            else
            {
                // The runner pulled down while the valve is open, then the rebound that rams the charge in late.
                const double suction = -(101325.0 - std::min(manifold, 101325.0) + 18e3) * l;
                const double ram = 22e3 * std::exp(-since / 300.0) * std::sin(2.0 * kPi * since / 230.0 - 1.2);
                p = std::max(10e3, manifold + suction + ram);
            }
            out[i] = static_cast<float>(p);
        }
        return m;
    }

    void scopeDemoTelemetry(DemoEngine& engine)
    {
        engine.set("ecu.advance", 26.0);
    }
}
