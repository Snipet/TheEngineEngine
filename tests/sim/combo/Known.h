// tests/sim/combo/Known.h: the known failures (Combo.h, Known): each is reported by the test that meets it, and does
// not turn the shared branch red. A finding goes here the day it is reported to its owner (with that date and the
// owner), and leaves the day it is fixed: its test then passes and holds the fix. Included once, by Combo.cpp.

const Known kKnown[] = {
    // id, date, owner, combo, build, scenario, check, what
    {"X-016", "2026-11-24", "Q (cost; the heavy case of 02-contracts section 9)", "random 1", "fuzz", "random", "cost",
     "random Spec seed 1 is a four-rotor rotary (engine.custom, rotors 4: 12 faces on 4 shared ports, each solved twice "
     "a sample): 3.2 to 4.6 R; reported, not required in real time (Q: Economy or TEE_FAST)"},
    {"X-016", "2026-11-24", "Q (cost)", "corners 1", "fuzz", "corners", "cost",
     "the ranges' corners seed 1: 2.1 to 4.9 R (Q measured 2.1 to 2.4 on a quieter host); reported as the heavy case"},
    {"X-020", "2026-11-24", "N2 (boost, for card F)", "load, all on", "v8", "load", "tel-finite",
     "tel.boost.efficiency[1] goes non-finite from the moment the twin-turbo large diesel is loaded over a running build"},
    {"X-023", "2026-11-24", "G (exhaust)", "*", "*", "*", "tel-range:exhaust.headerThermocouple",
     "tel.exhaust.headerThermocouple reads 0 K in the first block (f703892)"},
    {"X-023", "2026-11-24", "G (exhaust)", "*", "*", "*", "tel-range:exhaust.portThermocouple",
     "tel.exhaust.portThermocouple reads 0 K in the first block (f703892)"},
    {"X-022", "2026-11-24", "N2 and E (via the lead)", "corners 3", "fuzz", "corners", "guard",
     "the ranges' corners seed 3 (findings/x022-supercharger-itb-small-charge.json): a charge volume at its minimum into "
     "ITBs diverges; the engine's guard puts it at rest (FLAG_FAULT)"},
    {"X-022", "2026-11-24", "N2 and E (via the lead)", "corners 3", "fuzz", "corners", "tel-finite",
     "the same run: torque, bmep, shake, rock and supercharger power non-finite from the divergence"},
    {"X-022", "2026-11-24", "N2 and E (via the lead)", "corners 3", "fuzz", "corners", "silent",
     "the same run: silent after the guard's reset"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
