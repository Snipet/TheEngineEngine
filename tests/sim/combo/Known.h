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
    // The soak's reds of 25 Nov (e37feb24's run; bisected over sim trees with TEE_SIM_DIR). None is a pop: each reads
    // the same with ecu.pops 0 (tee-fuzz --play ... --set ecu.pops=0). Closed by the click check's restatement (the
    // lead's ruling, 17:24: a click is a step its neighbouring firing periods lack; Combo.cpp, judge), as a firing
    // train, not a click: X-033 (the large diesel's custom engine on boost: excess 1.35 to 2.59 against 3.19), X-034
    // (every switch on the large diesel: 1.17 against 3.19 and 3.22), X-035 (its custom engine without boost: 1.86 to
    // 2.29 against 3.19). Not a loosened bound: the restated check still sees X-014 and X-015 on their old tree.
    {"X-030", "2026-11-25", "the steward (the ECU's shift cut)", "+fuel.model+ecu.model", "*", "pull", "click-shift-cut",
     "a firing unlike its neighbours as the spark returns after an upshift's cut, ecu.model 1 under fuel.model 1: excess "
     "3.21 to 3.80 against 2.85 (small four), 4.32 against 4.03 (V8); since c035fe75"},
    {"X-031", "2026-11-25", "the steward (the ECU's downshift)", "+fuel.model+ecu.model", "v8", "pull", "click",
     "a firing unlike its neighbours as the spark returns after the V8's downshift at 11.0 s, ecu.model and fuel.model "
     "on: excess 4.73 to 5.17 against 4.03 (13.4 step before c035fe75)"},
    {"X-032", "2026-11-25", "N1 (the idle valve)", "exhaust.model+fuel.model+ecu.model", "turbo-six", "tour", "click",
     "a firing unlike its neighbours as the turbo six lifts at 8.5 s (exhaust, fuel and ecu on): excess 3.05 against "
     "2.97, narrowly; from 576c1c6e, the idle valve's integral"},
    {"X-036", "2026-11-25", "D (engine.custom) and the steward (car.model)", "+engine.custom+car.model", "turbo-v8-dual",
     "pull", "dc", "the turbo V8 dual's pull with a custom engine and car.model: the running mean 0.0057 to 0.0069 of "
     "full scale against 0.005, since the first soak (never green)"},
    {"X-037", "2026-11-25", "N1 (the valve)", "as built", "pickup-diesel-67-inline-six", "tour", "tel-range:valve.dpOverlap",
     "valve.dpOverlap -393835 Pa at 9.41 s against +-300000, since 2f86a47a"},
    // The fuzz under stable draws (1abb05a0): X-029 (a rotary's 0.6 mm primary, from the old draws' random 245) is held
    // by its build alone, content/builds/combo/findings/x029-rotary-tiny-primary.json
    {"X-038", "2026-11-25", "G (the builder's exhaust; N2 found it, 16:17)", "random 167", "fuzz", "random", "*",
     "boost.model 1 with a 0.07 mm tailpipe diverges (findings/x038-boost-tiny-tailpipe.json)"},
    {"X-038", "2026-11-25", "G (the builder's exhaust; N2 found it, 16:17)", "random 298", "fuzz", "random", "*",
     "boost.model 1 with a 0.011 mm primary diverges (findings/x038-boost-tiny-primary.json)"},
    {"X-039", "2026-11-25", "N2 (the supercharger)", "corners 205", "fuzz", "corners", "*",
     "a supercharged 50 cc single, blow-off kind 2, diverges at 0.27 s (findings/x039-single-blower-blowoff.json)"},
    {"X-040", "2026-11-25", "L (intake.model)", "random 251", "fuzz", "random", "*",
     "intake.model 1 with a 0.025 mm runner bore: intake.box non-finite at 0.26 s (findings/x040-intake-tiny-runner.json)"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
