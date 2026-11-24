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
    {"X-026", "2026-11-24", "N2 (boost)", "corners 178", "fuzz", "corners", "*",
     "a supercharged 50 cc engine on boost.model 1 goes non-finite and the guard restarts it (content/builds/combo/findings/x026-single-big-blower.json)"},
    {"X-026", "2026-11-24", "N2 (boost)", "corners 217", "fuzz", "corners", "*",
     "a supercharged 50 cc engine on boost.model 1 goes non-finite and the guard restarts it (content/builds/combo/findings/x026-redline.json)"},
    {"X-026", "2026-11-24", "N2 (boost)", "corners 237", "fuzz", "corners", "*",
     "a supercharged 50 cc engine on boost.model 1 goes non-finite and the guard restarts it (content/builds/combo/findings/x026-redline.json)"},
    {"X-026", "2026-11-24", "N2 (boost)", "corners 282", "fuzz", "corners", "*",
     "a supercharged 50 cc engine on boost.model 1 goes non-finite and the guard restarts it (content/builds/combo/findings/x026-single-small-pipe.json)"},
    {"X-026", "2026-11-24", "N2 (boost)", "corners 298", "fuzz", "corners", "*",
     "a supercharged 50 cc engine on boost.model 1 goes non-finite and the guard restarts it (content/builds/combo/findings/x026-redline.json)"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
