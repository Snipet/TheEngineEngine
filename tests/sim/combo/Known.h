// tests/sim/combo/Known.h: the known failures (Combo.h, Known): each is reported by the test that meets it, and does
// not turn the shared branch red. A finding goes here the day it is reported to its owner (with that date and the
// owner), and leaves the day it is fixed: its test then passes and holds the fix. Included once, by Combo.cpp.

const Known kKnown[] = {
    // id, date, owner, combo, build, scenario, check, what
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
    {"X-006", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "dog-box", "click",
     "the new ECU's shift cut and spark-cut limiter step the output 2 to 3 times the reference at the same moments "
     "(content/scenarios/combo/findings/x006-*.json)"},
    {"X-006", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "launch-control", "click",
     "the new ECU's shift cut and spark-cut limiter step the output 2 to 3 times the reference at the same moments "
     "(content/scenarios/combo/findings/x006-*.json)"},
    {"X-006", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "wet-traction-off", "click",
     "the new ECU's shift cut and spark-cut limiter step the output 2 to 3 times the reference at the same moments "
     "(content/scenarios/combo/findings/x006-*.json)"},
    {"X-006", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "wet-traction-on", "click",
     "the new ECU's shift cut and spark-cut limiter step the output 2 to 3 times the reference at the same moments "
     "(content/scenarios/combo/findings/x006-*.json)"},
    {"X-006", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "limiter-1", "click",
     "the new ECU's shift cut and spark-cut limiter step the output 2 to 3 times the reference at the same moments "
     "(content/scenarios/combo/findings/x006-*.json)"},
    {"X-007", "2026-11-24", "D and I (via the lead)", "random 2", "fuzz", "random", "guard",
     "random Spec seed 2, reduced to 8 fields (findings/x007-heavy-piston.json): a custom single with an 18 kg "
     "piston and the new vehicle's automatic runs away; without the checks process hangs (or, earlier, SIGSEGV)"},
    {"X-008", "2026-11-24", "X (reducing)", "corners 3", "fuzz", "corners", "guard",
     "the ranges' corners, seed 3: a block beyond full scale or not finite; the guard restarts it"},
    {"X-006", "2026-11-24", "H (via the lead)", "+fuel.model+ecu.model", "*", "tour", "click",
     "the new ECU's cut sends a whole charge to the pipe and its bang is five times too loud (H's fix, not yet on "
     "the branch at 15:45): with fuel.model and ecu.model both on the limiter's steps reach 22 against 11"},
    {"X-006", "2026-11-24", "H (via the lead)", "+fuel.model+ecu.model", "*", "pull", "click",
     "the new ECU's cut sends a whole charge to the pipe and its bang is five times too loud (H's fix, not yet on "
     "the branch at 15:45): with fuel.model and ecu.model both on the limiter's steps reach 22 against 11"},
    {"X-006", "2026-11-24", "H (via the lead)", "+fuel.model+ecu.model", "*", "launch", "click",
     "the new ECU's cut sends a whole charge to the pipe and its bang is five times too loud (H's fix, not yet on "
     "the branch at 15:45): with fuel.model and ecu.model both on the limiter's steps reach 22 against 11"},
    {"X-006", "2026-11-24", "H (via the lead)", "+fuel.model+ecu.model", "*", "cold", "click",
     "the new ECU's cut sends a whole charge to the pipe and its bang is five times too loud (H's fix, not yet on "
     "the branch at 15:45): with fuel.model and ecu.model both on the limiter's steps reach 22 against 11"},
    {"X-006", "2026-11-24", "H (via the lead)", "as built", "*", "tour", "click",
     "the archetype corpus has fuel.model and ecu.model on: with both, the output steps 24 to 41 at the throttle's "
     "onset and the blip (8 without either)"},
    {"X-009", "2026-11-24", "D (via the lead)", "as built", "*", "tour", "tel-range:engine.veManifold",
     "tel.engine.veManifold reads -5.8 on the lift after the limiter (straight-six-32-high-rev)"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
