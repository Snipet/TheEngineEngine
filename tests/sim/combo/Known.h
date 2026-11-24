// tests/sim/combo/Known.h: the known failures (Combo.h, Known): each is reported by the test that meets it, and does
// not turn the shared branch red. A finding goes here the day it is reported to its owner (with that date and the
// owner), and leaves the day it is fixed: its test then passes and holds the fix. Included once, by Combo.cpp.

const Known kKnown[] = {
    // id, date, owner, combo, build, scenario, check, what
    {"X-009", "2026-11-24", "D (via the lead)", "as built", "*", "tour", "tel-range:engine.veManifold",
     "tel.engine.veManifold reads -5.8 on the lift after the limiter (straight-six-32-high-rev)"},
    {"X-010", "2026-11-24", "Q (sim/boost/; via the lead)", "as built", "tractor-15-inline-six", "tour", "click",
     "boost.model 1 on the tractor diesel: a step of 32 at 10.78 s after the blip (9.3 without boost): surge's onset?"},
    {"X-010", "2026-11-24", "Q (sim/boost/; via the lead)", "as built", "pickup-diesel-67-inline-six", "tour", "click",
     "the same on the pickup diesel: 29.5 with boost, 19.8 without"},
    {"X-011", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "wet-traction-on", "click",
     "traction control on a wet road holds the revs, not the slip (it stays at 1); its spark cut bangs at the shift"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
