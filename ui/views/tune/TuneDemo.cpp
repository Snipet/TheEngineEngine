// ui/views/tune/TuneDemo.cpp: TUNE's own demo data (U1's hook, ui/app/Demo.h): a lambda map with its formula switch,
// as the integrated Spec will have (sim/ecu/settings.fm's EcuMapsSpec), and the ECU's live values the demo lacks
// (lambda asked and measured, knock, the ECU's flags), as functions of the script.
#include "app/Demo.h"
#include "views/dyno/Chart.h"

#include <cmath>

namespace tee::ui
{
    void tuneDemo(DemoExtras& x)
    {
        if (x.fields != nullptr)
        {
            FieldInfo m;
            m.path = "ecu.maps.lambda";
            m.kind = FieldKind::map;
            m.min = 0.6;
            m.max = 1.2;
            m.def = 0.9;
            m.label = "Lambda";
            m.group = "Maps";
            m.decimals = 2;
            m.axes = { Quantity::rotation, Quantity::ratio };
            m.axisLo = { 800.0 * chart::kRpm, 0.2 };
            m.axisHi = { 7500.0 * chart::kRpm, 2.4 };
            x.fields->push_back(m);
            FieldInfo a;
            a.path = "ecu.maps.lambdaAuto";
            a.kind = FieldKind::toggle;
            a.min = 0.0;
            a.max = 1.0;
            a.def = 1.0;
            a.label = "Lambda: formula";
            a.group = "Maps";
            x.fields->push_back(a);
        }
        if (x.engine == nullptr)
            return;
        DemoEngine& e = *x.engine;
        const auto target = [](const DemoEngine& d) { return d.throttle() > 0.8 ? (d.rpm() > 4000.0 ? 0.82 : 0.86) : 1.0; };
        const auto knocking = [](const DemoEngine& d) { return d.rpm() > 4300.0 && d.rpm() < 5600.0 && d.throttle() > 0.9; };
        e.addTelemetry("ecu.lambda", target);
        e.addTelemetry("exhaust.lambda", [target](const DemoEngine& d) {
            return target(d) + 0.025 * std::sin(d.time() * 9.0) + (d.throttle() > 0.8 ? 0.02 : 0.0);
        });
        e.addTelemetry("ecu.knock", [knocking](const DemoEngine& d) {
            return knocking(d) ? 0.35 + 0.3 * std::fabs(std::sin(d.time() * 23.0)) : 0.0;
        });
        e.addTelemetry("ecu.flags", [knocking](const DemoEngine& d) {
            const double t = d.time();
            const bool limiter = t >= 6.0 && t < 6.4, overrun = t >= 6.6 && t < 7.4;
            return static_cast<double>((limiter ? 1 + 2 : 0) + (overrun ? 8 + 2 : 0) + (knocking(d) ? 1024 : 0));
        });
    }
}
