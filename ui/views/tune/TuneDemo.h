// ui/views/tune/TuneDemo.h: what TUNE's tests and pictures put in the demo document: a plausible spark map (U1's demo
// hook adds fields and telemetry, not a table's values). The live values are TuneDemo.cpp's hook.
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
}
