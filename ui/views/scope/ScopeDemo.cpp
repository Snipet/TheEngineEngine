// ui/views/scope/ScopeDemo.cpp: SCOPE's own demo data (U1's hook, TEE_UI_DEMO in register.h): what the bench reads
// (sim/spec/telemetry.fm, EcuTel): the half-burn angle, later under load. The knock names are TUNE's hook's (one name,
// one maker: U1's demo refuses two); SCOPE's tests take TUNE's hook with their own.
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
    }
}
