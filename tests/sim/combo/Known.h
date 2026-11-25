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
    // the same with ecu.pops 0 (tee-fuzz --play ... --set ecu.pops=0).
    {"X-029", "2026-11-25", "G (exhaust.model) and T (the rotary)", "random 245", "fuzz", "random", "*",
     "a one-rotor custom rotary on exhaust.model 1 with a 0.6 mm first primary diverges at 0.31 s and is restarted "
     "(content/builds/combo/findings/x029-rotary-tiny-primary.json)"},
    {"X-030", "2026-11-25", "the steward (the ECU's shift cut)", "+fuel.model+ecu.model", "*", "pull", "click-shift-cut",
     "ecu.model 1 under fuel.model 1 at an upshift's cut: 20.8 to 23.1 against 19.6 or 20.7 (not ruled: fuel.model on); "
     "since c035fe75 (the V8 13.4 before it, 38.5 at it)"},
    {"X-031", "2026-11-25", "the steward (the ECU's downshift)", "+fuel.model+ecu.model", "v8", "pull", "click",
     "the V8's downshift at 11.0 s with ecu.model and fuel.model: 26.3 to 27.2 at 11.02 s against 19.6 (13.4 before "
     "c035fe75); fuel+ecu+car 20.1 at 6.449 s"},
    {"X-032", "2026-11-25", "N1 (the idle valve)", "exhaust.model+fuel.model+ecu.model", "turbo-six", "tour", "click",
     "the turbo six's lift at 8.5 s: 22.2 at 8.676 s against 22.2 (11.3 before 576c1c6e, the idle valve's integral)"},
    {"X-034", "2026-11-25", "N1 (the valve's effective area)", "all", "large-diesel", "*", "click",
     "every switch on the large diesel: pull 15.7 at 13.24 s against 15.4, launch 15.5 at 11.45 s against 14.4; from "
     "68e683f1 (the lesser of port and curtain), as is valve+fuel+ecu on the V8's pull (21.6)"},
    {"X-033", "2026-11-25", "N2 (the boost's design point)", "+engine.custom+boost.model", "large-diesel", "pull", "*",
     "the large diesel with a custom engine and boost.model: 16.7 to 19.9 against 15.4 (at the upshifts 3.6, 6.6, 8.6 "
     "s); 5c85736b alone takes custom+boost+car from 13.8 to 17.2"},
    {"X-035", "2026-11-25", "D (engine.custom) and D2 (the diesel)", "+engine.custom", "large-diesel", "pull", "click",
     "a custom engine on the large diesel without boost.model: 15.6 to 19.3 against 15.4 since the first soak (never "
     "green); custom+intake+fuel red again from 3a253ce6 (the combustion's start spread)"},
    {"X-035", "2026-11-25", "D2 (the diesel)", "engine.wallHeat+ecu.model+voices.model", "large-diesel", "pull", "click",
     "15.5 at 4.84 s against 15.4, since 2f86a47a"},
    {"X-035", "2026-11-25", "D2 (the diesel)", "intake.model+fuel.model+car.model", "large-diesel", "pull", "click",
     "15.7 at 4.93 s against 15.4, from 3a253ce6 (the combustion's start-of-combustion spread at its cited sigma)"},
    {"X-036", "2026-11-25", "D (engine.custom) and the steward (car.model)", "+engine.custom+car.model", "turbo-v8-dual",
     "pull", "dc", "the turbo V8 dual's pull with a custom engine and car.model: the running mean 0.0057 to 0.0069 of "
     "full scale against 0.005, since the first soak (never green)"},
    {"X-037", "2026-11-25", "N1 (the valve)", "as built", "pickup-diesel-67-inline-six", "tour", "tel-range:valve.dpOverlap",
     "valve.dpOverlap -393835 Pa at 9.41 s against +-300000, since 2f86a47a"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
