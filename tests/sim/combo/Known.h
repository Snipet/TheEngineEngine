// tests/sim/combo/Known.h: the known failures (Combo.h, Known): each is reported by the test that meets it, and does
// not turn the shared branch red. A finding goes here the day it is reported to its owner (with that date and the
// owner), and leaves the day it is fixed: its test then passes and holds the fix. Included once, by Combo.cpp.

const Known kKnown[] = {
    // id, date, owner, combo, build, scenario, check, what
    {"X-010", "2026-11-24", "N2 (boost, for card F)", "as built", "tractor-15-inline-six", "tour", "click",
     "boost.model 1 on the tractor diesel: steps of 52 to 57 after the blip (9.3 without boost; 32 before 2db13dd)"},
    {"X-010", "2026-11-24", "N2 (boost, for card F)", "as built", "pickup-diesel-67-inline-six", "tour", "click",
     "the same on the pickup diesel: 26.7 with boost, 19.8 without"},
    {"X-011", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "wet-traction-on", "click",
     "traction control on a wet road holds the revs, not the slip (it stays at 1); its spark cut bangs at the shift"},
    {"X-012", "2026-11-24", "T (two-strokes)", "*", "*", "*", "tel-range:engine.tsPipeTemperature",
     "tel.engine.tsPipeTemperature reads 0 K on every four-stroke (no pipe): it should read a physical temperature"},
    {"X-010", "2026-11-24", "N2 (boost, for card F)", "as built", "tractor-15-inline-six", "tour", "dc",
     "the tractor's running mean at 0.0050 of full scale with boost on"},
    {"X-014", "2026-11-24", "N2 and G (boost into the exhaust builder)", "as built", "straight-six-twin-turbo-30", "tour",
     "click", "boost.model 1 with exhaust.model 1: steps of 38 to 39 at the throttle's onset and the blip (8 without either)"},
    {"X-015", "2026-11-24", "N2 and G (boost into the exhaust builder)", "+engine.custom+valve.model+boost.model+exhaust.model",
     "large-diesel", "*", "click",
     "the large diesel with the custom engine, the valvetrain, boost and the exhaust builder: clicks of 42 to 66 every "
     "0.4 s at full load"},
    {"X-015", "2026-11-24", "N2 and G (boost into the exhaust builder)", "+engine.custom+valve.model+boost.model+exhaust.model",
     "v8", "load", "click", "the same while the large diesel is loaded over the V8 with every switch on"},
    {"X-016", "2026-11-24", "Q (cost)", "random 1", "fuzz", "random", "cost",
     "random Spec seed 1 costs 4.6 R since the exhaust builder runs (1.29 R before)"},
    {"X-016", "2026-11-24", "Q (cost)", "corners 1", "fuzz", "corners", "cost",
     "the ranges' corners seed 1 costs 4.9 R since the exhaust builder runs (0.76 R before)"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
