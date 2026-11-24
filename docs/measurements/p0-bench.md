# Phase 0: what the reference costs, and what it does

*Part K2 of the keel (docs/plan/p0-keel.md), 2026-11-24. Everything here was measured by `tee-bench` and by the tests
of `tests/ReferenceTest.cpp` on the reference engine (`sim/ref/`, FMinus at `5b665d7`, LLVM 23.1.3), played by
`tee::RefPlayer`: one voice, the dials as knobs, release options unless a line says otherwise. Run them again with
`build-dev/bin/tee-bench` and `ctest --preset dev -V -R "strongest|same sound|idles"`.*

## The number the budgets hang on

**R, the reference V8's cost in this container: about 2.3 microseconds a sample of the engine's 96 kHz**, which is
**22 to 23 % of one core** when it plays in real time (48 kHz out, blocks of 512). The reference's heaviest layout,
the V12, costs 3.1 to 3.2 microseconds: 30 to 31 %.

| Engine | ns a 96 kHz sample | of a core, in real time |
|---|---:|---:|
| V8 preset (crossplane V8, 6.2 L) | 2265 to 2408 | 21.7 to 23.1 % |
| The defaults (the same layout, 5.76 L, a mild cam) | 2321 to 2449 | 22.3 to 23.5 % |
| V10 preset | 2755 to 2890 | 26.4 to 27.7 % |
| Bench V12 (the V10's dials, twelve cylinders: ours, not the reference's) | 3120 to 3226 | 29.9 to 31.0 % |
| Large Diesel preset (an inline six, twin turbos) | 1639 to 1801 | 15.7 to 17.3 % |

The range is across the four scenarios. Cost follows the cylinders and the pipes they bring, not what the engine is
doing: idling costs what the limiter costs.

## The machine, and how far to trust it

A cloud container: Intel Xeon at 2.10 GHz (family 6, model 207: Emerald Rapids), 4 cores without
hyper-threading, 16 GB, Ubuntu 24.04, clang 18.1.3. **It was shared**: other sessions were compiling on the same
four cores the whole time (the load average was between 7 and 10), so the costs are the thread's own processor time,
not the clock's, and each is the least of three runs. Even so a busy machine slows the code itself (the caches are
shared): a first look on the same machine an hour earlier, before the others got going, read 2276 ns for the V8 and
3199 for the V12 by the clock. Read the figures as good to about 5 %; the "slowest call" column is the clock's and
says more about the neighbours than about the engine.

The same on CI's hosted runners, which are quiet (`tee-bench --quick`: free-revs, one run, from the logs of runs
45392135054 and 45394648192; this container's row is the table's below):

| Machine | V8, ns | of a core | V12, ns | of a core | compile at -O2 |
|---|---:|---:|---:|---:|---:|
| This container (Xeon, 2.10 GHz) | 2282 | 21.9 % | 3226 | 31.0 % | 1.38 s |
| GitHub Linux runner (AMD EPYC 7763) | 2272 | 21.8 % | 2987 | 28.7 % | 1.36 s |
| GitHub Linux runner (AMD EPYC 9V74) | 2380 | 22.8 % | 3155 | 30.3 % | 1.36 s |
| GitHub Windows runner (AMD EPYC, family 25) | 2432 | 23.4 % | 3238 | 31.1 % | 1.70 s |
| GitHub macOS runner (Apple M1, virtual, 3 cores) | 2574 | 24.7 % | 2975 | 28.6 % | 1.75 s |

**Against the user's Mac.** The plugin's load meter reads 30 to 40 % of a core for this program at 512 samples on an
Apple silicon Mac. That is not the same measurement and not the same machine, and nothing here pretends otherwise:
the meter times the plugin's whole audio callback, which plays the engine through a loop budget in parts of 16
samples, smooths its dials into streams and does its own bookkeeping round it; this is the engine's `process` alone.
What can be said: the only Apple silicon this ran on, CI's virtual M1, takes 25 % of a core for the V8 where this
container takes 22 %, and 29 % for the V12 where this takes 31 %: the two machines are within about a tenth of each
other for this code, either way, so a budget set in this container's R is about the same budget on an M1. The
user's Mac is very likely faster than a virtual M1, and its 30 to 40 % then says the plugin's own share is
substantial; only a run of `tee-bench` on that Mac settles it (the CI artefact `tee-macos-arm64` has it:
`./tee-bench --quick`).

## The whole table

Machine: Intel(R) Xeon(R) Processor @ 2.10GHz, 4 logical cores, Linux x86-64. FMinus 0.1.0 on LLVM 23.1.3. tee-bench 0.1.0, the least of 3 runs.

### Compiling the reference

Seconds of the compiling thread's processor time for `program.fm` and the ten files it includes (5,173
lines): FMinus's `compile()`, then the machine code (the optimiser and the code generator, which FMinus runs at
the first look-up of a function; with the checks it runs them inside `compile()`).

| Level | compile() | machine code | together | code bytes | with checks, together | code bytes |
|---|---:|---:|---:|---:|---:|---:|
| -O0 | 0.05 | 0.06 | 0.10 | 129065 | 0.14 | 212093 |
| -O1 | 0.05 | 1.19 | 1.24 | 154267 | 1.39 | 159277 |
| -O2 | 0.04 | 1.33 | 1.38 | 158406 | 1.37 | 160698 |

### Playing it

-O2, 48000 Hz, blocks of 512, over the calls in which the engine ran.
"ns" is the thread's processor time for one sample of the engine's own 96 kHz rate; "real time" is that
time over the audio's duration (100 % is one core, whole); "slowest" is the slowest single call's wall time
over its own duration, which a busy machine lengthens. "With checks" is a development build: the loop budget
and the bounds checks on.

| Preset | Scenario | ns | real time | slowest | with checks: ns | real time | slowest |
|---|---|---:|---:|---:|---:|---:|---:|
| bench-v12 | cold-start-idle | 3120 | 29.9 % | 157 % | 2791 | 26.8 % | 120 % |
| bench-v12 | drive-gears | 3185 | 30.6 % | 211 % | 2761 | 26.5 % | 216 % |
| bench-v12 | free-revs | 3226 | 31.0 % | 220 % | 2958 | 28.4 % | 267 % |
| bench-v12 | truck-pull-away | 3134 | 30.1 % | 189 % | 3244 | 31.1 % | 304 % |
| defaults | cold-start-idle | 2449 | 23.5 % | 182 % | 2007 | 19.3 % | 96 % |
| defaults | drive-gears | 2321 | 22.3 % | 151 % | 2108 | 20.2 % | 248 % |
| defaults | free-revs | 2433 | 23.4 % | 297 % | 1954 | 18.8 % | 294 % |
| defaults | truck-pull-away | 2401 | 23.0 % | 453 % | 2099 | 20.2 % | 344 % |
| large-diesel | cold-start-idle | 1747 | 16.8 % | 167 % | 1704 | 16.4 % | 196 % |
| large-diesel | drive-gears | 1756 | 16.9 % | 198 % | 1798 | 17.3 % | 278 % |
| large-diesel | free-revs | 1639 | 15.7 % | 177 % | 1723 | 16.5 % | 201 % |
| large-diesel | truck-pull-away | 1801 | 17.3 % | 245 % | 1693 | 16.3 % | 176 % |
| v10 | cold-start-idle | 2779 | 26.7 % | 148 % | 2500 | 24.0 % | 242 % |
| v10 | drive-gears | 2890 | 27.7 % | 292 % | 2525 | 24.2 % | 286 % |
| v10 | free-revs | 2755 | 26.4 % | 232 % | 2400 | 23.0 % | 106 % |
| v10 | truck-pull-away | 2782 | 26.7 % | 226 % | 2611 | 25.1 % | 147 % |
| v8 | cold-start-idle | 2265 | 21.7 % | 69 % | 2006 | 19.3 % | 61 % |
| v8 | drive-gears | 2396 | 23.0 % | 183 % | 2083 | 20.0 % | 188 % |
| v8 | free-revs | 2282 | 21.9 % | 104 % | 2066 | 19.8 % | 109 % |
| v8 | truck-pull-away | 2408 | 23.1 % | 149 % | 2183 | 21.0 % | 166 % |

How busy the machine was: the wall time of those calls ran 55 % over their processor time, on average (0 % on an idle machine).

What the table adds to the summary:

- **The checks cost nothing here.** A development build (the loop budget and the bounds checks on) came out about
  10 % cheaper than the release build on this machine, and 0 to 5 % dearer on CI's runners: inside the noise either
  way. The plan's "a third or more" for bounds checks does not hold for this program (its machine code grows 1.4 %
  with them at -O2, 64 % at -O0: the optimiser proves nearly every access). The test `[ref][checks]` shows the two
  builds give the same samples, bit for bit, and that the budget is really counted. So a development build can keep
  the checks on at no cost in speed.
- **Compile time**: 1.4 s at -O2 for the 5,173 lines, nearly all of it LLVM's optimiser and code generator (the
  front end is 0.04 s). -O1 saves a tenth of that and plays at -O2's cost (22.7 % for the V8, one run). -O0 compiles
  in 0.1 s and plays at 62 % of a core, 2.8 times -O2's cost: slow, but it does play, which makes it a candidate for
  the first seconds after a reload while -O2 compiles behind it. A cold start pays the 1.4 s once.
- The machine code is 158 kB. `tee-render`, which carries the JIT, is 126 MB with debug information and 61 MB
  stripped (LLVM, linked statically).

## What the reference does (the tests' measurements)

**It compiles with no error and no warning**, and FMinus proves `start`, `process` and `stop` nonblocking through the
functions `RefPlayer` calls them by: nothing they reach can reach `malloc`, `calloc` or `free` (an allocation put
where `process` reaches it stops the compile, with the test). Not proven, and not meant to be: `Shared::init` and
`Shared::end`, which take and give back the pipes' delay lines and the resampler's table. Nothing in the reference
carries `[[clang::nonblocking]]` itself.

**Block size.** Calls of 64, 333, 512 and 1024 samples give the same samples, bit for bit, at 44.1, 48 and 96 kHz
(the V8 on free-revs, 12 s).

**Sample rate.** The engine runs at 96 kHz whatever the host's rate. Renders at 44.1 and 96 kHz, resampled to 48 kHz
and compared with the 48 kHz render (the V8 on free-revs, left and right):

| | 44.1 kHz | 96 kHz | The test allows |
|---|---:|---:|---:|
| Start and idle (0.3 to 3.9 s), the difference under the signal | 40.6, 41.1 dB | 55.3, 55.7 dB | at least 30 dB |
| Any second's level, 1 to 20 s | within 0.001 dB | within 0.30 dB | 1.5 dB |
| Any octave's level on the limiter, 31 Hz to 16 kHz | within 0.0001 dB | within 0.28 dB | 2 dB |

At 44.1 and 48 kHz the engine sees its inputs at the same moments and stays in step for the whole 24 s; the 40 dB is
the two converters' own difference near 20 kHz. At 96 kHz the inputs reach the engine half a millisecond later in
its own time (there is no resampler looking ahead), and after the first pedal movement the waveforms part company,
the sound staying the same: docs/formats.md says why.

**It starts, idles, sits on its limiter and stops** (the defaults on free-revs): the starter cranks it and it fires;
at idle it turns 880 rpm in the mean against the 880 its own `idleRpm` holds (828 to 928); flat out in neutral it
reaches 6557 rpm against a redline of 6500 and stays above 6305; the start key held a second switches it off, it
runs down, and a second later the voice is silent for good. The presets' big cams (CAM at 0.9999) lope instead, as
they are meant to: the V8 preset idles between 1000 and 1600 rpm round an `idleRpm` of 1227.

**Driven**: the V8 in a 1500 kg car reaches 218 km/h in fourth, flat out for 15 s from rest, and comes back down the
gears to a stop; the Large Diesel pulls a 30 t truck to 46 km/h in its tenth gear in half a minute, held at its
governed 2500 to 2650 rpm in each.

**The strongest low line** (15 Hz to 1.2 kHz, left channel) with a key holding a speed, against the firing frequency
(cylinders / 2 a turn of the crank), on the defaults with the layout changed:

| Layout | Speed held, rpm | Firing, Hz | Strongest line, Hz | Over the firing frequency |
|---|---:|---:|---:|---|
| TWIN 360 | 5355 | 89.26 | 89.26 | 1.0000 |
| INLINE 3 | 3988 | 99.70 | 99.70 | 1.0000 |
| INLINE 4 | 3963 | 132.10 | 132.10 | 1.0000 |
| FLAT 4 | 3840 | 128.01 | 128.02 | 1.0001 |
| INLINE 6 | 3889 | 194.47 | 194.49 | 1.0001 |
| V10 | 4744 | 395.35 | 395.36 | 1.0000 |
| V12 | 4721 | 472.08 | 472.11 | 1.0001 |
| V6 | 3817 | 190.83 | 95.42 | 0.5000: each bank fires every 240 degrees into its own pipe |
| FLAT 6 | 4696 | 234.80 | 117.40 | 0.5000: the same |
| V8 CROSS | 3645 | 243.00 | 91.13 | 0.3750: the third harmonic of the cycle, 1.5 times the crank: the burble |

The test holds the first nine to 0.5 %, and the crossplane V8 to a harmonic of its cycle below the firing
frequency. Looked at once and not held by a test: a single's strongest line was twice its firing frequency, an inline
five's three times, a flat-plane V8's 1.5 times (the third harmonic of a bank), and the rotary's did not sit on a
multiple of anything this simple (0.93 of two pulses a turn of the shaft): someone who knows the rotary's model
should say whether that is right.

## Building

| What | Time |
|---|---:|
| `scripts/bootstrap-cloud.sh`, a container that had most of the apt packages | 24 s |
| The same, run again | 1.6 s |
| First build of the keel alone (FMinus, Catch2, the host, the tools, the tests; 203 steps, 3 jobs, an empty compiler cache) | 2 min 42 s |
| A fresh checkout elsewhere on the machine, the compiler cache warm: configure | 54 s |
| ... and build `tee-render`, `tee-bench`, `tee-tests` (208 of 211 compiles from the cache) | 30 s |
| The whole tree as it stood two hours later (SDL3, the shell, the interface, the application's tests: 673 steps), a clean tree, the cache warm | 7 min 49 s |
| CI, the keel alone, no caches: Linux, macOS, Windows | 3 min 17 s, 2 min 4 s, 4 min 8 s |
| CI, the tree an hour later, caches warm (99 % hits): Linux, macOS, Windows | 3 min 47 s, 3 min, 4 min 20 s |

All but CI's on the shared machine described above, so on a quiet one they are shorter.
