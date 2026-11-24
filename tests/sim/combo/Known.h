// tests/sim/combo/Known.h: the known failures (Combo.h, Known): each is reported by the test that meets it, and does
// not turn the shared branch red. A finding goes here the day it is reported to its owner (with that date and the
// owner), and leaves the day it is fixed: its test then passes and holds the fix. Included once, by Combo.cpp.

const Known kKnown[] = {
    // id, date, owner, combo, build, scenario, check, what
    {"X-002", "2026-11-24", "X (reducing)", "load, all on", "v8", "load", "tel-range:engine.bmep",
     "a build loaded over another with every switch on: tel.engine.bmep reaches -2.08 MPa (-20.8 bar), past the "
     "-20 bar bound for a pressure; being reduced to its package before it is reported"},
    {"X-003", "2026-11-24", "H (combustion and ECU)", "+fuel.model+ecu.model", "*", "cold", "*",
     "fuel.model 1 with ecu.model 1: no start at -20 C in 4 s of cranking (10 s neither); starts at -10 C in 0.64 s; "
     "each alone starts at -20 C (content/builds/combo/findings/x003-h-cold.json)"},
    {"X-004", "2026-11-24", "L / the final mix (via the lead)", "*", "turbo-six", "launch", "guard",
     "the output is not limited to full scale: the turbo six's held launch peaks at 0.89 to 0.92 FS in every "
     "combination and passes 1.0 with eight switches on; the guard silences it (findings/x004-six-launch.json)"},
    {"X-004", "2026-11-24", "L / the final mix (via the lead)", "*", "turbo-six", "launch", "moves",
     "the same run: after the guard's restart the engine is off, so the car does not move"},
    {"X-004", "2026-11-24", "L / the final mix (via the lead)", "*", "turbo-six", "launch", "stall",
     "the same run: after the guard's restart the engine is off"},
    {"X-005", "2026-11-24", "Q (sim/boost/)", "*", "*", "*", "tel-range:boost.intercoolerOut",
     "tel.boost.intercoolerOut reads 0 K when no machine is in the path (boost.model 0, or no boost): it should read "
     "the charge air's temperature"},
    {"X-005", "2026-11-24", "Q (sim/boost/)", "*", "*", "*", "tel-range:boost.throttleIn",
     "tel.boost.throttleIn reads 0 K when no machine is in the path, as intercoolerOut"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
