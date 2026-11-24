// ui/views/scope/ScopeDemo.cpp: SCOPE's own demo data (U1's hook, TEE_UI_DEMO in register.h): what the bench reads
// (sim/spec/telemetry.fm, EcuTel): the half-burn angle, later under load; the knocks counted and the knock retard by
// cylinder, cylinders 3 and 6 knocking a little near full load at speed. TUNE's hook makes the knock names too, its
// own way: a rig applies only its own package's hooks (U1's DemoHooks), so each stage's goldens stand on its own.
#include "app/Demo.h"

#include <algorithm>
#include <cmath>
#include <string>


namespace tee::ui
{
    void scopeDemo(DemoExtras& x)
    {
        if (x.engine == nullptr)
            return;
        x.engine->addTelemetry("ecu.halfBurn", [](const DemoEngine& e) { return 6.0 + 5.0 * e.throttle(); });
        const auto hard = [](const DemoEngine& e) {
            return std::clamp((e.throttle() - 0.8) / 0.2, 0.0, 1.0) * std::clamp((e.rpm() - 4000.0) / 2000.0, 0.0, 1.0);
        };
        for (int c = 0; c < 8; ++c)
            x.engine->addTelemetry("ecu.knockByCylinder[" + std::to_string(c) + "]", [c, hard](const DemoEngine& e) {
                return c == 2 ? 2.5 * hard(e) : c == 5 ? 1.0 * hard(e) : 0.0;
            });
        x.engine->addTelemetry("ecu.knock", [hard](const DemoEngine& e) { return std::floor(12.0 * hard(e)); });
    }
}
