// ui/views/tune/TuneDemo.cpp: TUNE's own demo data (U1's hook, ui/app/Demo.h): a lambda map with its formula switch,
// as the integrated Spec has (ecu.maps.*), and the ECU's live values the demo lacks (lambda asked and sensed, the
// knock count and each cylinder's retard, the load, the ECU's flags), as functions of the script.
#include "app/Demo.h"
#include "views/dyno/Chart.h"

#include <algorithm>
#include <cmath>
#include <string>

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
            // The intake cam's phase map (degrees advanced) and its switch: on, the formula (phasers at 0).
            FieldInfo ci = m;
            ci.path = "ecu.maps.camIn";
            ci.min = -10.0;
            ci.max = 50.0;
            ci.def = 0.0;
            ci.label = "Intake cam";
            ci.unit = "deg";
            ci.decimals = 1;
            x.fields->push_back(ci);
            FieldInfo cia = a;
            cia.path = "ecu.maps.camInAuto";
            cia.label = "Intake cam: formula";
            x.fields->push_back(cia);
            // Card N2's knock-limit table (degrees off the best-torque spark), not baked yet.
            FieldInfo k = m;
            k.path = "ecu.maps.knockLimit";
            k.min = 0.0;
            k.max = 20.0;
            k.def = 0.0;
            k.label = "Knock limit";
            k.unit = "deg";
            k.decimals = 1;
            x.fields->push_back(k);
            FieldInfo ka = a;
            ka.path = "ecu.maps.knockLimitAuto";
            ka.label = "Knock limit: not baked";
            x.fields->push_back(ka);
        }
        if (x.engine == nullptr)
            return;
        DemoEngine& e = *x.engine;
        // The names of the integrated ECU (docs/handoffs/h-combustion-ecu.md): lambdaTarget asked, lambda the sensor,
        // knock a count, knockByCylinder[16] each cylinder's retard, load the maps' y axis, flags ECU_FLAG_*.
        const auto target = [](const DemoEngine& d) { return d.throttle() > 0.8 ? (d.rpm() > 4000.0 ? 0.82 : 0.86) : 1.0; };
        const auto knocking = [](const DemoEngine& d) { return d.rpm() > 4300.0 && d.rpm() < 5600.0 && d.throttle() > 0.9; };
        e.addTelemetry("ecu.lambdaTarget", target);
        e.addTelemetry("ecu.lambda", [target](const DemoEngine& d) {
            return target(d) + 0.025 * std::sin(d.time() * 9.0) + (d.throttle() > 0.8 ? 0.02 : 0.0);
        });
        // The knocks so far: 25 a second through the pull's knock-limited stretch (4.4 to 5.2 s of the script).
        e.addTelemetry("ecu.knock", [](const DemoEngine& d) {
            return std::floor(25.0 * std::clamp(d.time() - 4.4, 0.0, 0.8));
        });
        for (int k = 0; k < 16; ++k)
            e.addTelemetry("ecu.knockByCylinder[" + std::to_string(k) + "]", [k](const DemoEngine& d) {
                // Pulled through the knock stretch (4.4 to 5.2 s), most on cylinders 3 and 6, given back after it.
                const double t = d.time();
                if (t < 4.4 || t > 7.0)
                    return 0.0;
                const double most = k == 2 || k == 5 ? 3.0 : k == 6 ? 1.5 : 0.4;
                return most * std::min(1.0, (t - 4.4) / 0.4) * (t > 5.2 ? std::exp(-(t - 5.2) * 1.2) : 1.0);
            });
        e.addTelemetry("ecu.load", [](const DemoEngine& d) {
            const double boost = std::min(1.0, std::max(0.0, (d.rpm() - 2600.0) / 2400.0) * d.throttle()) * d.field("boost.level", 80e3);
            return (28e3 + d.throttle() * (101e3 - 28e3) + boost) / 101325.0;
        });
        e.addTelemetry("ecu.flags", [knocking](const DemoEngine& d) {
            const double t = d.time();
            const bool limiter = t >= 6.0 && t < 6.4, overrun = t >= 6.6 && t < 7.4;
            return static_cast<double>((limiter ? 1 + 2 : 0) + (overrun ? 8 + 2 : 0) + (knocking(d) ? 1024 : 0));
        });
    }
}
