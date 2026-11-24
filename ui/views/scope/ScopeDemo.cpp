// ui/views/scope/ScopeDemo.cpp: SCOPE's own demo data (U1's hook, TEE_UI_DEMO in register.h): what the bench reads
// that U1's demo does not make. The half-burn angle and the knock retard by cylinder (sim/spec/telemetry.fm, EcuTel),
// made up from the script: the burn later under load; cylinders 3 and 6 knocking a little near full load at speed.
// (ecu.knock is TUNE's demo's: one name, one maker.)
#include "app/Demo.h"

#include <algorithm>
#include <string>

namespace tee::ui
{
    void scopeDemo(DemoExtras& x)
    {
        if (x.engine == nullptr)
            return;
        x.engine->addTelemetry("ecu.halfBurn", [](const DemoEngine& e) { return 6.0 + 5.0 * e.throttle(); });
        for (int c = 0; c < 8; ++c)
            x.engine->addTelemetry("ecu.knockByCylinder[" + std::to_string(c) + "]", [c](const DemoEngine& e) {
                const double hard = std::clamp((e.throttle() - 0.8) / 0.2, 0.0, 1.0) * std::clamp((e.rpm() - 4000.0) / 2000.0, 0.0, 1.0);
                return c == 2 ? 2.5 * hard : c == 5 ? 1.0 * hard : 0.0;
            });
    }
}
