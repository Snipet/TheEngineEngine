// app/Script.h: a script of inputs for a run with no person at it: what the keyboard, the pointer and the dials do,
// when, with pictures to take and things that must be true on the way. tee-headless runs one (and the window does,
// with --script, in its own real time). A line an event: seconds from the start, a verb, its arguments; `#` starts
// a comment.
//
//   0.20  key return down          a key by Keys.h's name (w, s, up, down, e, q, n, space, return, lshift, 1 to 6)
//   0.50  key return up
//   2.00  tap e                    a key down, and up 0.1 s later (tap e 0.3: after 0.3 s)
//   2.50  set size 6.2             a parameter by its id
//   3.00  pointer move 40 300      the pointer, in the Panel's logical px: move, down, up, click
//   3.10  wheel 40 300 1           the wheel over a point: dy
//   4.00  shot out/idle.png        the frame as it stands, written as a PNG
//   4.00  expect engine.state == 2 what must hold then (>, <, >=, <=, ==, !=), of a telemetry value by its name
//                                  (Sources.h), of `rpm` (engine.speed in turns a minute), of a parameter (`param
//                                  size`), or of the output since the last such check: `peak` (0 to 1) and `rms`
//   9.00  print                    a line of telemetry on the standard output
//  10.00  end                      the run ends here
//
// A check that fails is counted and said, and the run goes on; the program's exit code says whether any did.
#pragma once

#include <string>
#include <vector>

namespace tee::app
{
    struct ScriptEvent
    {
        enum class Kind { key, set, pointer, wheel, shot, expect, print, end };
        double time = 0.0;
        Kind kind = Kind::print;
        int line = 0;               // in the file, for a message
        std::string name;           // key: its name; set: the id; pointer: move, down, up, click; shot: the file;
                                    // expect: what is checked
        std::string op;             // expect: the comparison
        double a = 0.0, b = 0.0, c = 0.0;   // key: a = 1 down, 0 up; set: a the value; pointer: a, b the point; wheel: a, b
                                            // the point, c dy; expect: a the number
    };

    struct Script
    {
        std::vector<ScriptEvent> events;    // by time; those of one time in the file's order
        double end = -1.0;                  // the time of its `end`, or of its last event; negative: empty
    };

    // Reads a script's text. False, and why (with the line), for a line it cannot read.
    bool parseScript(const std::string& text, Script& out, std::string& why);
    bool loadScript(const std::string& path, Script& out, std::string& why);

    // The script a render runs when none is given: the start key, a rev, and the start key held to switch off, laid
    // over `seconds`.
    Script defaultScript(double seconds);
}
