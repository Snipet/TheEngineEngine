// ui/views/scope/ScopeDemo.cpp: SCOPE's own demo data (U1's hook, TEE_UI_DEMO in register.h): what the bench reads
// that no other demo makes: the half-burn angle (sim/spec/telemetry.fm, EcuTel), later under load. The knock names
// (ecu.knock, ecu.knockByCylinder[i]) are TUNE's demo's: one name, one maker.
#include "app/Demo.h"


namespace tee::ui
{
    void scopeDemo(DemoExtras& x)
    {
        if (x.engine == nullptr)
            return;
        x.engine->addTelemetry("ecu.halfBurn", [](const DemoEngine& e) { return 6.0 + 5.0 * e.throttle(); });
    }
}
