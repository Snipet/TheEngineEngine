// tests/sim/combo/Known.h: the known failures (Combo.h, Known): each is reported by the test that meets it, and does
// not turn the shared branch red. A finding goes here the day it is reported to its owner (with that date and the
// owner), and leaves the day it is fixed: its test then passes and holds the fix. Included once, by Combo.cpp.

const Known kKnown[] = {
    // id, date, owner, combo, build, scenario, check, what
    {"X-002", "2026-11-24", "X (reducing)", "load, all on", "v8", "load", "tel-range",
     "a build loaded over another with every switch on: tel.engine.bmep reaches -2.08 MPa (-20.8 bar), past the "
     "-20 bar bound for a pressure; being reduced to its package before it is reported"},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},   // the end (an array is never empty)
};
