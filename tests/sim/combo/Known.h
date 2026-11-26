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
    {"X-041", "2026-11-25", "G (Burned in the pipe; the window's item 1)", "valve.model+boost.model+ecu.model", "turbo-six",
     "tour", "click",
     "a lift (throttle 0.015 to 0.004) at 9.76 s, nothing burning within 50 ms (bangHeat and flame 0): excess 3.23 "
     "against 2.97; Bangs Carved 2.10; from 9b678980 (9e71f6c7's Burned meeting the boost series, each alone passes); "
     "G: the turbine housing booked the burns' volume at the collector's cold temperature (fix 301ba06c, local)"},
    {"X-041", "2026-11-25", "G (Burned in the pipe; the window's item 1)", "as built", "inline-five-turbo-rally", "tour",
     "click",
     "the lift's blowdown front steepened in the pipes at 8.74 s with anti-lag (G), nothing burning within 50 ms: excess "
     "3.92 against 3.00; Bangs Carved 2.01, anti-lag off 1.48, pops 0 unchanged; from 9b678980"},
    {"X-041", "2026-11-25", "G and N2 (the window's item 1)", "as built", "inline-five-turbo-rally", "tour",
     "tel-range:valve.dpWave",
     "Burned in the pipe, anti-lag and spark cut: the bangs' mean pressure ahead of the turbine overstated (G's words): "
     "valve.dpWave -349747 Pa at 8.47 s against +-300000 on the limiter (bangHeat 2 to 3 MW); Bangs Carved stays inside"},
    {"X-042", "2026-11-25", "N2 with the integrator (the turbine's match under engine.custom; the window's item 3)",
     "+engine.custom+valve.model+boost.model", "large-diesel", "tour", "tel-range:valve.dpWave",
     "the exhaust 337 to 350 kPa over the intake at full throttle, 2480 rpm, the charge at 111.7 kPa (every switch on; "
     "engine.custom+valve.model+boost.model): N2, engine.custom's one collector leaves the rear turbo unfed and the "
     "front's compressor chokes; shown from 9aa2acac (the map's lower choke flow), valve+boost or custom+boost pass"},
    {"X-043", "2026-11-25", "N1 (the intake's plenum)", "intake.model+exhaust.model+car.model", "turbo-six", "tour", "click",
     "the start's catch at 3.39 s (121 to 237 rad/s in 40 ms): excess 3.27 against 2.97; from c40f0411 (the plenum "
     "gives up what the valves took: 3.02), unseen until every triple shard ran"},
    {"X-044", "2026-11-26", "the steward (the idle valve)", "engine.custom+fuel.model+ecu.model", "small-four", "pull",
     "click",
     "a firing unlike its neighbours as the spark returns after the small four's downshift at 11.02 s: excess 3.48 "
     "against 2.85; from 98d0dc4d (the idle valve's proportional part on the speed alone: 1.75 at its parent), the "
     "cure that ended X-031 on the V8 (intake+fuel+ecu 2.77 at 11.03 s, under)"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
