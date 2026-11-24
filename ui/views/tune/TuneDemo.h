// ui/views/tune/TuneDemo.h: what the TUNE workspace's tests and pictures add to U1's demo (ui/app/Demo.h) until its
// hook for a workspace's own demo data is there: a plausible spark map in the document, and the ECU's live values
// the demo's telemetry does not have yet (lambda asked and measured, knock, advance, the ECU's flags), as functions
// of the script's time. Kept here, in one place, so they move into the hook in a minute.
#pragma once

#include "app/Demo.h"
#include "views/dyno/Chart.h"

#include <algorithm>
#include <cmath>

namespace tee::ui::tunedemo
{
    // A spark map of 16 speeds by 12 loads a tuner would recognise: advance rising with speed, falling with load, the
    // knock-limited dip at peak torque under boost.
    inline void fillDocument(FieldDoc& doc)
    {
        const int f = doc.find("ecu.spark");
        if (f < 0)
            return;
        const FieldInfo& info = doc.info(f);
        Map16 m;
        m.nx = 16;
        m.ny = 12;
        for (int i = 0; i < m.nx; ++i)
            m.x[static_cast<std::size_t>(i)] = std::round((800.0 + 450.0 * i) / 50.0) * 50.0 * chart::kRpm;
        for (int j = 0; j < m.ny; ++j)
            m.y[static_cast<std::size_t>(j)] = std::round((20e3 + 20e3 * j) / 1000.0) * 1000.0;
        for (int j = 0; j < m.ny; ++j)
            for (int i = 0; i < m.nx; ++i)
            {
                const double rpm = m.x[static_cast<std::size_t>(i)] / chart::kRpm;
                const double kpa = m.y[static_cast<std::size_t>(j)] / 1000.0;
                const double base = 10.0 + 26.0 * (1.0 - std::exp(-rpm / 2200.0));
                const double load = (kpa - 20.0) / 220.0;
                const double dip = 4.0 * load * std::exp(-std::pow((rpm - 4200.0) / 1300.0, 2.0));
                const double v = base - 20.0 * load * load - 6.0 * load - dip;
                m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] = std::clamp(std::round(v * 2.0) / 2.0, info.min, info.max);
            }
        doc.beginGesture();
        doc.setMap(f, m);
        doc.endGesture();
    }

    // The ECU's live values at the script's time t (seconds; the demo's pull is 2 to 6 s, the limiter 6 to 6.4).
    inline void setLive(DemoEngine& e, double t)
    {
        const double rpm = e.value(e.findValue("engine.speed")) / chart::kRpm;
        const double plate = e.value(e.findValue("engine.throttle"));
        const double target = plate > 0.8 ? (rpm > 4000.0 ? 0.82 : 0.86) : 1.0;
        e.set("ecu.lambda", target);
        e.set("exhaust.lambda", target + 0.025 * std::sin(t * 9.0) + (plate > 0.8 ? 0.02 : 0.0));
        const bool knocking = rpm > 4300.0 && rpm < 5600.0 && plate > 0.9;
        e.set("ecu.knock", knocking ? 0.35 + 0.3 * std::fabs(std::sin(t * 23.0)) : 0.0);
        e.set("ecu.knockRetard", knocking ? std::min(4.5, (t - 4.6) * 3.0 + 1.5) : (rpm > 5600.0 && plate > 0.9 ? 1.5 : 0.0));
        e.set("ecu.spark", (plate > 0.8 ? 20.0 : 32.0) + rpm / 1000.0 - (knocking ? 3.0 : 0.0));
        const bool limiter = t >= 6.0 && t < 6.4;
        const bool overrun = t >= 6.6 && t < 7.4;
        e.set("ecu.flags", (limiter ? 1 + 2 : 0) + (overrun ? 8 + 2 : 0) + (knocking ? 1024 : 0));
    }
}
