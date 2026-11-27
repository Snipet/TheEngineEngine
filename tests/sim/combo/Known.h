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
     "3.21 to 3.80 against 2.85 (small four), 4.32 against 4.03 (V8); since c035fe75; a sometimes row (the lead, 26 Nov 07:30): 12 of 17 copies over at 293.15 +- 0.15 K on fb32d4fc (lowest 2.25, median 3.20, highest 7.09); the over draws fall at the upshift's return, the first burning firing back about one revolution after the cut ends (8.52 to 8.78 s, 6.73 s; one plain click at 8.73 s); under the nightly bound (twice the reference's lowest of its 5 copies) 2.82: 13 of 17"},
    {"X-031", "2026-11-25", "the steward (the ECU's downshift)", "+fuel.model+ecu.model", "v8", "pull", "click",
     "a firing unlike its neighbours as the spark returns after the V8's downshift at 11.0 s, ecu.model and fuel.model "
     "on: excess 4.73 to 5.17 against 4.03 (13.4 step before c035fe75); back with 98d0dc4d's revert for the bake (the "
     "lead's ruling, 26 Nov 03:18), re-read on the reverted tree: fuel+ecu 4.73, custom+fuel+ecu 5.17 at 11.02 s; a sometimes row (the lead, 26 Nov 07:30): the downshift click, 1 of 17 copies over at 293.15 +- 0.15 K on fb32d4fc (lowest 1.83, median 3.66, highest 4.78 over the whole pull); the pull's upshift cuts over in 5 more, which are X-030's check; in the downshift's own window (10.95 to 11.20 s) 2 of 15 over on the steward's nudges; under the nightly bound (twice the reference's lowest of its 5 copies) 3.92: 7 of 17"},
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
    // With 98d0dc4d reverted for the bake (the lead's ruling, 26 Nov 03:18), re-read on that tree (sim f23337cc): the rally
    // five's row back, which the idle cure had ended; the pull row above is not met there. (The turbo six's tour lift,
    // valve+boost+ecu, left on 26 Nov: 0 of 17 with the combo builds' lab cams stated equal to their dials; it measured the
    // lab's default cam against the dial's.)
    {"X-041", "2026-11-26", "the steward (the idle valve) and G (the limiter's bangs)", "as built",
     "inline-five-turbo-rally", "tour", "click",
     "an X-041 row on main (b557c4a1) already: 3.92 at 8.74 s there, and its dpWave -350 kPa on the limiter; on the "
     "bake's tree (sim f23337cc) 3.08 against 3.00 at 6.48 s, the spark-cut limiter at full throttle (733 rad/s, "
     "ecu.flags 1029), bangs burning near (1.9 MW within 50 ms); 2.14 at 6.51 s with 98d0dc4d: the idle valve's rate "
     "term acting far off idle (the lead's note to the steward); a sometimes row (the lead, 26 Nov 07:30): 1 of 17 copies over at 293.15 +- 0.15 K on fb32d4fc (lowest 1.72, median 2.15, highest 3.08); the over draw is the 293.15 K one itself, at 6.48 s; under the nightly bound (twice the reference's lowest of its 5 copies) 2.48: 5 of 17"},
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
     "against 2.97 (c40f0411 moved the hunt's phase onto it); a sometimes row (the lead, 26 Nov 07:30): 11 of 17 copies over at 293.15 +- 0.15 K on fb32d4fc (lowest 2.26, median 3.13, highest 4.15); the over draws fall at the start and its catches, 2.71 to 4.79 s; under the nightly bound (twice the reference's lowest of its 5 copies) 2.96: 11 of 17"},
    // X-044 (the lead's ruling, 26 Nov 01:08; with 98d0dc4d reverted for the bake, as it was before it: 1 phase in 6)
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "engine.custom+fuel.model+ecu.model", "small-four",
     "pull", "click",
     "engine.custom + fuel.model + ecu.model without exhaust.model, the small four's pull at the downshift (11.02 s): the "
     "fuel returning whole on the blip's first firings after the overrun cut; 1 phase in 6 without 98d0dc4d (reverted "
     "for the bake: phase 0 reads 1.75 against 2.85, the triple's phase), 3 in 6 with it (3.48 at phase 0); green with "
     "every switch on; a sometimes row (the lead, 26 Nov 07:30): 11 of 17 copies over at 293.15 +- 0.15 K on fb32d4fc (lowest 1.53, median 3.35, highest 4.57); the over draws fall at the 11.02 s downshift (10) and one upshift cut (8.63 s); its phase-0 pass (1.75 at 293.15 K) was a draw; in the downshift's own window 14 of 15 over on the steward's nudges; under the nightly bound (twice the reference's lowest of its 5 copies) 2.82: 11 of 17"},
    {"X-045", "2026-11-26", "the integrator (engine.fm; the next window, the lead's ruling)", "boost.model", "default",
     "keyfields", "keyfields-exhaust",
     "boost.model or exhaust.model written while stopped, and the application's own load, keep tel.exhaust.model 0 and "
     "the model-0 exhaust configured until the key (the exhaust is built at the key; holdModel() takes the switch with "
     "no configure); they agree after it: nothing keys an engine without it, only what a stopped engine reports"},
    {"X-045", "2026-11-26", "the integrator (engine.fm; the next window, the lead's ruling)", "exhaust.model", "default",
     "keyfields", "keyfields-exhaust", "as boost.model's: the stopped engine reports the last exhaust until the key"},
    // The nightly bound's newly red rows (the lead's ruling, 26 Nov 09:30): 17 copies at 293.15 +- 0.15 K on 207c1a62's
    // tree, against the nightly bound (twice the turbo six pull reference's lowest of its 5 copies, 1.34: 2.67) and
    // today's single draw (3.37); exact combinations, no "+" pattern. With per-event windows (part B) the rows at "its
    // shifts" become per-event and may split.
    {"X-043", "2026-11-26", "none: kept (the lead's ruling; N1 diagnosed it, the steward worded it)", "intake.model+exhaust.model+car.model", "turbo-six", "pull",
     "click", "X-043 on the pull, the start's catch at 2.72 s: under the nightly bound 2.67, 7 of 17 copies over; under 3.37, "
     "4 of 17; lowest 1.24, median 2.01, highest 4.15"},
    {"X-043", "2026-11-26", "none: kept (the lead's ruling; N1 diagnosed it, the steward worded it)", "engine.custom+intake.model+exhaust.model", "turbo-six",
     "pull", "click", "X-043's pairing with a custom engine, the start at 3.37 s: under the nightly bound 2.67, 3 of 17 over; "
     "under 3.37, 0 of 17; lowest 1.38, median 1.67, highest 2.84"},
    {"X-048", "2026-11-26", "N2 (boost.model)", "boost.model", "turbo-six", "pull", "click",
     "the turbo six's pull at its shifts with boost.model on, the mechanism not read yet (the reference's lowest copy on "
     "this scenario 1.34): under the nightly bound 2.67, 2 of 17; under 3.37, 0 of 17; lowest 1.55, median 1.90, highest 2.72; at 8.75 and 11.31 s"},
    {"X-048", "2026-11-26", "N2 (boost.model)", "boost.model+exhaust.model", "turbo-six", "pull", "click",
     "the turbo six's pull at its shifts with boost.model on, the mechanism not read yet (the reference's lowest copy on "
     "this scenario 1.34): under the nightly bound 2.67, as boost.model alone, the same figures: 2 of 17; under 3.37, 0 of 17"},
    {"X-048", "2026-11-26", "N2 (boost.model), N1 named (the valve)", "valve.model+boost.model", "turbo-six", "pull", "click",
     "the turbo six's pull at its shifts with boost.model on, the mechanism not read yet: 2 of 17 copies over the nightly bound 2.68 on the turbo six with its lab cams stated equal to its dial (26 Nov, N1's lift-matched law); lowest 1.56, median 2.23, highest 2.86; under 3.37, 0 of 17; the model's, not the cam's (4 of 17 on the dial build, the lab's default cam)"},
    {"X-048", "2026-11-26", "N2 (boost.model), N1 named (the valve)", "valve.model+boost.model+voices.model", "turbo-six", "pull", "click",
     "the turbo six's pull at its shifts with boost.model on, the mechanism not read yet: as valve.model+boost.model, on the lift-matched turbo six: 2 of 17 over 2.68; lowest 1.56, median 2.23, highest 2.85"},
    {"X-048", "2026-11-26", "N2 (boost.model), N1 named (the valve)", "valve.model+boost.model+exhaust.model", "turbo-six", "pull", "click",
     "the turbo six's pull at its shifts with boost.model on, the mechanism not read yet: as valve.model+boost.model, on the lift-matched turbo six: 2 of 17 over 2.68; lowest 1.56, median 2.23, highest 2.86"},
    {"X-048", "2026-11-26", "N2 (boost.model), N1 named (the valve)", "engine.wallHeat+valve.model+boost.model", "turbo-six-mild", "pull", "click",
     "the turbo six's pull at its shifts with boost.model on, the mechanism not read yet: on the mild twin (a 231 degree cam, its lab cams equal to its dial): 2 of 17 copies over its nightly bound 3.04; lowest 1.47, median 1.86, highest 3.64; on the turbo six's own wild cam, lift-matched, 0 of 17 (14 of 17 on the dial build: the lab's default cam against the dial's)"},
    {"X-048", "2026-11-26", "N2 (boost.model), N1 named (the valve)", "valve.model+boost.model+fuel.model", "turbo-six-mild", "pull", "click",
     "the turbo six's pull at its shifts with boost.model on, the mechanism not read yet: on the mild twin: 3 of 17 over 3.04; lowest 1.64, median 2.15, highest 4.06; on the turbo six's wild cam, lift-matched, 0 of 17 (8 of 17 on the dial build)"},
    // Part B's newly red rows (the lead's ruling, 26 Nov 11:11): 17 copies at 293.15 +- 0.15 K per EVENT (the worst window
    // of that event in each copy), on the merged tip of 26 Nov 10:15, against the nightly bound; exact combinations, each
    // with its event
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "engine.wallHeat+fuel.model+ecu.model", "small-four", "pull", "click",
     "the fuel's return after an overrun cut with ecu.model on (the steward's item 8; the lead's ruling, 26 Nov 11:11), a row part B's per-event judge brought in: the small four's 3-2 downshift: 5 of 17 copies over the nightly bound 2.82; lowest 1.56, median 2.15, highest 3.91", "shift 3-2"},
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "fuel.model+ecu.model+car.model", "small-four", "pull", "click",
     "the fuel's return after an overrun cut with ecu.model on (the steward's item 8; the lead's ruling, 26 Nov 11:11), a row part B's per-event judge brought in: the small four's 3-2 downshift: 2 of 17 over 2.82; lowest 1.60, median 2.27, highest 4.74", "shift 3-2"},
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "fuel.model+ecu.model+voices.model", "small-four", "pull", "click",
     "the fuel's return after an overrun cut with ecu.model on (the steward's item 8; the lead's ruling, 26 Nov 11:11), a row part B's per-event judge brought in: the small four's 3-2 downshift: 5 of 17 over 2.82; lowest 1.56, median 2.14, highest 3.93", "shift 3-2"},
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "valve.model+fuel.model+ecu.model", "v8", "tour", "click",
     "the fuel's return after an overrun cut with ecu.model on (the steward's item 8; the lead's ruling, 26 Nov 11:11), a row part B's per-event judge brought in: the V8's tour, the overrun cut ending at 9.46 s: 2 of 17 over the nightly bound 3.94; lowest 1.55, median 2.16, highest 4.13", "cut"},
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut), N2 named (boost)", "engine.custom+boost.model+ecu.model", "turbo-six", "pull", "click",
     "the fuel's return after an overrun cut with ecu.model on (the steward's item 8; the lead's ruling, 26 Nov 11:11), a row part B's per-event judge brought in: the turbo six's 3-2 downshift: 8 of 17 over the nightly bound 2.67; lowest 1.63, median 2.53, highest 3.53", "shift 3-2"},
    {"X-052", "2026-11-26", "the steward (ecu.model's limiter)", "valve.model+fuel.model+ecu.model", "v8", "tour", "click",
     "at the limiter, a row part B's per-event judge brought in; the mechanism not read; X-051's stand-in (the exhaust valve's flow solve run to convergence) to be read on it (the lead's ruling, 26 Nov 11:11): 6 of 17 copies over the nightly bound 3.94; lowest 2.84, median 3.82, highest 5.85", "limiter"},
    {"X-052", "2026-11-26", "the steward (ecu.model's limiter)", "engine.wallHeat+ecu.model+car.model", "turbo-six", "tour", "click",
     "at the limiter, a row part B's per-event judge brought in; the mechanism not read; X-051's stand-in (the exhaust valve's flow solve run to convergence) to be read on it (the lead's ruling, 26 Nov 11:11): 3 of 17 over 2.97; lowest 2.17, median 2.59, highest 3.20", "limiter"},
    {"X-052", "2026-11-26", "G (the reference ECU's limiter: a train of cuts, X-051 the suspect)", "intake.model+exhaust.model+voices.model", "turbo-six", "tour", "click",
     "at the limiter, a row part B's per-event judge brought in; the mechanism not read; X-051's stand-in (the exhaust valve's flow solve run to convergence) to be read on it (the lead's ruling, 26 Nov 11:11): 1 of 17 over 2.97; lowest 1.96, median 2.28, highest 3.03", "limiter"},
    {"X-053", "2026-11-26", "the integrator (a switch taken while running)", "flip engine.wallHeat", "turbo-six", "flip", "click",
     "a model switched while running, the click in the 0.3 s after the switch is thrown (part B's flip window; the lead's ruling, 26 Nov 11:11), the turbo six's pull, flips at 5.0, 6.5, 8.0, 9.5 s: 3 of 17 copies over the nightly bound 2.67; lowest 1.65, median 2.08, highest 3.27", "flip"},
    {"X-053", "2026-11-26", "the integrator (a switch taken while running)", "flip exhaust.model", "turbo-six", "flip", "click",
     "a model switched while running, the click in the 0.3 s after the switch is thrown (part B's flip window; the lead's ruling, 26 Nov 11:11), the turbo six's pull, flips at 5.0, 6.5, 8.0, 9.5 s: 10 of 17 over 2.67; lowest 2.05, median 2.84, highest 3.89", "flip"},
    // The mild twin's new rows (the corrected corpus's nightly, 26 Nov; the lead's ruling, 14:25): 17 copies at 293.15 +-
    // 0.15 K per event against the twin's nightly bound 3.04; exact rows with their events. Most are 1 of 17 on one build,
    // the weakest kind of entry there is, kept because none is waved off
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "engine.wallHeat+fuel.model+ecu.model", "turbo-six-mild", "pull", "click",
     "the fuel's return after an overrun cut with ecu.model on, on the mild twin's 3-2 downshift: 1 of 17 over 3.04; lowest 1.65, median 2.26, highest 3.47", "shift 3-2"},
    {"X-044", "2026-11-26", "the steward (the ECU's overrun cut)", "fuel.model+ecu.model+voices.model", "turbo-six-mild", "pull", "click",
     "the fuel's return after an overrun cut with ecu.model on, on the mild twin's 3-2 downshift: 1 of 17 over 3.04; lowest 1.66, median 2.38, highest 3.70", "shift 3-2"},
    {"X-030", "2026-11-26", "the steward (the ECU's shift cut)", "engine.wallHeat+fuel.model+ecu.model", "turbo-six-mild", "pull", "click",
     "the first firing back after the new ECU's shift cut, on the mild twin's 1-2 upshift, a plain click there: 1 of 17 over 3.04; lowest 1.81, median 2.30, highest 3.56; without car.model the cut there is the 110 ms one (X-047), so item 16 is expected to move it", "shift 1-2"},
    {"X-030", "2026-11-26", "the steward (the ECU's shift cut)", "fuel.model+ecu.model+voices.model", "turbo-six-mild", "pull", "click",
     "the first firing back after the new ECU's shift cut, on the mild twin's 1-2 upshift, a plain click there: 1 of 17 over 3.04; lowest 1.82, median 2.31, highest 3.59; without car.model the cut there is the 110 ms one (X-047), so item 16 is expected to move it", "shift 1-2"},
    {"X-048", "2026-11-26", "N2 (boost.model)", "intake.model+boost.model+fuel.model", "turbo-six-mild", "pull", "click",
     "boost.model at the shifts, on the mild twin's 1-2 upshift: 4 of 17 over 3.04; lowest 1.69, median 2.27, highest 3.94", "shift 1-2"},
    {"X-048", "2026-11-26", "N2 (boost.model)", "boost.model+sound.model+voices.model", "turbo-six-mild", "pull", "click",
     "boost.model at the shifts, on the mild twin's 2-3 upshift: 1 of 17 over 3.04; lowest 1.34, median 1.64, highest 3.18", "shift 2-3"},
    {"X-048", "2026-11-26", "N2 (boost.model)", "boost.model+exhaust.model+sound.model", "turbo-six-mild", "pull", "click",
     "boost.model at the shifts, on the mild twin's 2-3 upshift: 1 of 17 over 3.04; lowest 1.34, median 1.66, highest 3.14", "shift 2-3"},
    {"X-048", "2026-11-26", "N2 (boost.model)", "boost.model+exhaust.model+voices.model", "turbo-six-mild", "pull", "click",
     "boost.model at the shifts, on the mild twin's 2-3 upshift: 3 of 17 over 3.04; lowest 1.86, median 2.42, highest 3.60", "shift 2-3"},
    // Shipped builds as built on the pull (the lead's ruling, 26 Nov 15:00): they run the map ECU, so the upshift's cut is
    // X-047's over-long one and the steward's item 16 is expected to move them; 17 copies at 293.15 +- 0.15 K per event
    // against each build's own reference's nightly bound; each event twice, as a click and as a click-shift-cut (the same
    // window's worst step, named by whether it lies at the cut: a copy reads either), on 8ec06fd1 (the tip of 26 Nov afternoon).
    // EACH PAIR IS ONE ROW, NOT TWO FINDINGS (the lead's ruling, 26 Nov 15:26): one build, one scenario, one named event,
    // the same excess against the same bound under either name, and the spread counted a copy over under either; a cure
    // (item 16 on the real code) is 0 of 17 under BOTH names, and the pair leaves together
    {"X-048", "2026-11-26", "N2 (boost.model), the steward named (X-047's cut, item 16)", "as built", "three-cylinder-10-turbo", "pull", "click",
     "boost at the shifts, on a shipped build as built: the three-cylinder 1.0 turbo's 1-2 upshift: 6 of 17 over its nightly bound 2.84; lowest 1.26, median 2.62, highest 3.98", "shift 1-2"},
    {"X-048", "2026-11-26", "N2 (boost.model), the steward named (X-047's cut, item 16)", "as built", "three-cylinder-10-turbo", "pull", "click",
     "boost at the shifts, on a shipped build as built: the three-cylinder 1.0 turbo's 2-3 upshift: 8 of 17 over its nightly bound 2.84; lowest 2.32, median 2.83, highest 3.56", "shift 2-3"},
    {"X-048", "2026-11-26", "N2 (boost.model), the steward named (X-047's cut, item 16)", "as built", "kei-660-triple-turbo", "pull", "click",
     "boost at the shifts, on a shipped build as built: the kei 660's 1-2 upshift: 2 of 17 over its nightly bound 2.77; lowest 1.53, median 2.08, highest 2.82", "shift 1-2"},
    {"X-048", "2026-11-26", "N2 (boost.model), the steward named (X-047's cut, item 16)", "as built", "three-cylinder-10-turbo", "pull", "click-shift-cut",
     "as its click row, where the worst step lies at the upshift's cut (judge's click-shift-cut; the excess decides there too): the three-cylinder 1.0 turbo's 1-2 upshift: 6 of 17 over its nightly bound 2.84; lowest 1.26, median 2.62, highest 3.98", "shift 1-2"},
    {"X-048", "2026-11-26", "N2 (boost.model), the steward named (X-047's cut, item 16)", "as built", "three-cylinder-10-turbo", "pull", "click-shift-cut",
     "as its click row, where the worst step lies at the upshift's cut (judge's click-shift-cut; the excess decides there too): the three-cylinder 1.0 turbo's 2-3 upshift: 8 of 17 over its nightly bound 2.84; lowest 2.32, median 2.83, highest 3.56", "shift 2-3"},
    {"X-048", "2026-11-26", "N2 (boost.model), the steward named (X-047's cut, item 16)", "as built", "kei-660-triple-turbo", "pull", "click-shift-cut",
     "as its click row, where the worst step lies at the upshift's cut (judge's click-shift-cut; the excess decides there too): the kei 660's 1-2 upshift: 2 of 17 over its nightly bound 2.77; lowest 1.53, median 2.08, highest 2.82", "shift 1-2"},
    // X-055, the flat-plane twin-turbo V8 as built on the pull (the lead's ruling, 26 Nov 18:11; the shipped survey,
    // docs/measurements/combo/2026-11-26-shipped-pull.md): 17 copies at 293.15 +- 0.15 K per event against its own
    // reference's nightly bound 2.33 (that reference's lowest copy 1.17), the same on the tip and on item 16's cut probe
    {"X-055", "2026-11-26", "the steward (as built it runs the map ECU: its limiter)", "as built", "flat-plane-v8-39-twin-turbo", "pull", "click",
     "the flat-plane twin-turbo V8 as built on the pull, at its limiter: 7 of 17 over its nightly bound 2.33; lowest 1.87, median 2.17, highest 2.63. A TAIL: the limiter is a hard all-cylinder fuel cut bouncing near 25 Hz, about forty episodes a run, each with its own first firing back (mostly 1.0 to 1.5, about one in ten near 2; the steward's read), and the row is the worst of those forty a copy, at a different episode each copy (5.70, 6.30, 7.55, 8.11, 9.92 s), against a reference whose lowest copy is 1.17", "limiter"},
    {"X-055", "2026-11-26", "the integrator (the dual clutch's shift; N2 named for the boost)", "as built", "flat-plane-v8-39-twin-turbo", "pull", "click",
     "the flat-plane twin-turbo V8 as built on the pull, its 1-2 upshift on the dual clutch (no shift cut: item 16 does not reach it): 1 of 17 over 2.33 (2.66 at +0.01 K, 6.882 s); lowest 1.23, median 1.61", "shift 1-2"},
    {"X-055", "2026-11-26", "the integrator (the dual clutch's shift; N2 named for the boost)", "as built", "flat-plane-v8-39-twin-turbo", "pull", "click-shift-cut",
     "as its click row (one row under two names, as the shipped pairs): the 1-2 upshift, 1 of 17 over 2.33", "shift 1-2"},
    {"X-055", "2026-11-26", "not known (no owner read yet)", "as built", "flat-plane-v8-39-twin-turbo", "pull", "click",
     "the flat-plane twin-turbo V8 as built on the pull, in the rest (no event): 1 of 17 over 2.33 (2.76 at +0.03 K, 4.532 s); lowest 1.45, median 1.77. The owner is not known: nobody has read it", "the rest"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
