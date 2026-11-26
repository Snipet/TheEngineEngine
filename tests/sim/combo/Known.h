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
    {"X-036", "2026-11-25", "D (engine.custom) and the steward (car.model)", "+engine.custom+car.model", "turbo-v8-dual",
     "pull", "dc", "the turbo V8 dual's pull with a custom engine and car.model: the running mean 0.0057 to 0.0069 of "
     "full scale against 0.005, since the first soak (never green)"},
    {"X-036", "2026-11-25", "D (engine.custom) and the steward (car.model)", "random 103", "fuzz", "random", "dc",
     "a free-valve turbo with electric assist, car.model on a 28 % downhill: the running mean 0.0062 against 0.005; "
     "each of its 11 fields is needed; the turbo V8 dual's kind as far as shown (car.model and a turbo), the same "
     "mechanism not shown (findings/x036-random103.json)"},
    {"X-039", "2026-11-25", "N2 (the supercharger)", "corners 205", "fuzz", "corners", "*",
     "a supercharged 50 cc single, blow-off kind 2, diverges at 0.27 s (findings/x039-single-blower-blowoff.json)"},
    {"X-036", "2026-11-25", "the steward and the integrator (the steward's word, 25 Nov)", "all", "large-diesel", "dyno",
     "dc", "the large diesel on the engine dyno with every switch on: the running mean 0.0051 against 0.005 (the bake's "
     "soak, 223f4301); X-036's kind by the steward"},
    // The bake's soak (223f4301, job 139542192123), bisected over sim trees with tee-fuzz --play and TEE_SIM_DIR. Retired
    // by it: X-029 (G's bore floor), X-032 (the turbo six's lift: excess 1.84 against 2.97), X-037 (N1's b07522fc),
    // X-038 (random 167 and 298 run clean), X-040 (random 251 runs clean).
    // X-041's first three rows (the turbo six's tour lift, the rally five's lift and its dpWave on the limiter) retired on
    // G's 5941904e (the housing books the burns' gas hot), which moves the turbo six's lift onto the pull:
    {"X-041", "2026-11-26", "N2 (the blow-off and the gate loop; the lead's ruling, 26 Nov 02:22: the next window)",
     "valve.model+boost.model+ecu.model", "turbo-six", "pull", "click",
     "the blow-off valve opening on the lift at 10.588 s (0.006 to 0.93 in 20 ms) against a compressor ratio of 1.94 "
     "(1.81 before 5941904e), the turbo spooled about 10 % harder by the burns' drive (G's measurement): excess 3.69 "
     "against 3.37; nothing burning, the housing's gas unchanged"},
    {"X-042", "2026-11-25", "N2 with the integrator (the turbine's match under engine.custom; the window's item 3)",
     "+engine.custom+valve.model+boost.model", "large-diesel", "tour", "tel-range:valve.dpWave",
     "the exhaust 337 to 350 kPa over the intake at full throttle, 2480 rpm, the charge at 111.7 kPa (every switch on; "
     "engine.custom+valve.model+boost.model): N2, engine.custom's one collector leaves the rear turbo unfed and the "
     "front's compressor chokes; shown from 9aa2acac (the map's lower choke flow), valve+boost or custom+boost pass"},
    // X-043 stays for good (the lead's ruling, 26 Nov 00:34): a pairing nothing ships.
    {"X-043", "2026-11-25", "none: kept (the lead's ruling; N1 diagnosed it, the steward worded it)",
     "intake.model+exhaust.model+car.model", "turbo-six", "tour", "click",
     "the reference idle valve's look-ahead on the speed's rate, over intake.model's plenum and the reference turbo's "
     "spool at a shut throttle, on the fixture's cam-0.9999 lope; with ecu.model on it falls to the lope's own size "
     "(316 against 299 rpm sd), and with ecu.model a near stall at the catch, 13 rpm at 0.96 s after a 2030 rpm flare; "
     "no shipped build runs the reference idle, none carries such a cam. The start's catch at 3.39 s: excess 3.27 "
     "against 2.97 (c40f0411 moved the hunt's phase onto it)"},
    // X-044 stays (the lead's ruling, 26 Nov 01:08): nothing is cured for it this window.
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "engine.custom+fuel.model+ecu.model", "small-four",
     "pull", "click",
     "engine.custom + fuel.model + ecu.model without exhaust.model, the small four's pull at the downshift (11.02 s): the "
     "fuel returning whole on the blip's first firings after the overrun cut; 1 phase in 6 before 98d0dc4d, 3 in 6 after "
     "it (that cure moves which phases meet the return; the idle valve is not in it); green with every switch on (excess "
     "1.92 against 2.85 on the small four, 2.35 at most against 4.03 on the V8). Phase 0: 1.75 before, 3.48 after"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
