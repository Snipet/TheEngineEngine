// tests/sim/combo/Known.h: the known failures (Combo.h, Known): each is reported by the test that meets it, and does
// not turn the shared branch red. A finding goes here the day it is reported to its owner (with that date and the
// owner), and leaves the day it is fixed: its test then passes and holds the fix. Included once, by Combo.cpp.

const Known kKnown[] = {
    // id, date, owner, combo, build, scenario, check, what
    {"X-011", "2026-11-24", "I (via the lead): standstill", "ecu.model+car.model", "v8", "wet-traction-on", "click",
     "traction control from a standstill on a wet road: the slip not held (only a slipping clutch can at walking pace); "
     "its cuts bang at the shift"},
    {"X-016", "2026-11-24", "Q (cost; the heavy case of 02-contracts section 9)", "random 1", "fuzz", "random", "cost",
     "random Spec seed 1 is a four-rotor rotary (engine.custom, rotors 4: 12 faces on 4 shared ports, each solved twice "
     "a sample): 3.2 to 4.6 R; reported, not required in real time (Q: Economy or TEE_FAST)"},
    {"X-016", "2026-11-24", "Q (cost)", "corners 1", "fuzz", "corners", "cost",
     "the ranges' corners seed 1: 2.1 to 4.9 R (Q measured 2.1 to 2.4 on a quieter host); reported as the heavy case"},
    {"X-017", "2026-11-24", "N1 (calibration)", "*", "*", "*", "tel-range:engine.chargeTemperature",
     "tel.engine.chargeTemperature reads 0 K until the first charge (about 0.5 s): it should read the air's"},
    {"X-018", "2026-11-24", "H (via the lead)", "ecu.model+car.model", "v8", "wet-rolling-traction-off", "click",
     "the new ECU's shift cut on a manual's paddle upshift at part throttle steps 31 against the reference's pops at 24"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
