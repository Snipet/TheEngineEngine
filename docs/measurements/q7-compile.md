# Q7: where the compile goes, and how to compile less

*Package Q, 2026-11-24, 16:40 UTC, at the branch tip of 16:20 (every package but G's builder landed). Compile = qbench's
"create": `EnginePlayer::create` and the machine code, wall time on this container (Xeon 2.10 GHz, 4 cores, the host
shared: about +-0.3 s between two compiles of the same program in one process).*

## The size and the time

| Program | Machine code | Compile (front end + machine code) |
|---|---:|---:|
| The reference (`sim/ref`, K2) | 158 kB | 1.4 s |
| `sim/` at the split (c5636c5) | 166 kB | 1.1 to 1.2 s |
| `sim/` now | **466 kB** | **3.4 to 4.2 s** (front end 0.11 to 0.14 s; LLVM's optimiser and code generator the rest) |

The front end is nothing; LLVM's pipeline at -O2 is all of it, in proportion to the code.

## By package (machine code, FMinus's code ranges by module, grouped by the module's directory)

| Package | kB | Share | The largest modules |
|---|---:|---:|---|
| `vehicle/` (I, B2) | 61 | 13.3 % | Vehicle 19.6, Car 15.0, Bike 12.7, VehGearbox 4.5 |
| `boost/` (F) | 59 | 12.8 % | BoostMachine 31.5, Boost 14.2, BoostSound 6.0 |
| `sound/` (M, Extras) | 57 | 12.4 % | Extras 14.5, Voices 9.7, BikeVoice 5.8, GearVoice 5.3, StarterVoice 4.9 |
| `engine.fm` (D) | 53 | 11.5 % | Engine 52.7 (38 functions) |
| `listen/` (L) | 50 | 10.8 % | Listen 27.9, Scene 16.0, Hall 5.2 |
| `exhaust/` (G, the reference's model) | 48 | 10.5 % | Exhaust 35.5, Duct 9.9 |
| `valvetrain/` (E) | 36 | 7.7 % | ValveLab 16.7, Valvetrain 6.1, CamShape 5.9 |
| `core/` (D) | 27 | 6.0 % | Cylinder 7.6, LayoutGeom 3.8, ShakeByOrder 3.6 |
| `combustion/` (H) | 26 | 5.8 % | Combustion 18.9, HeavyDiesel 4.1 |
| `ecu/` (H) | 24 | 5.3 % | Ecu 8.9, Ecu1 5.5 |
| `intake/`, `lib/`, the rest | 18 | 3.9 % | |

**The three largest: the vehicle, the boost machine, the mechanical voices** (with Extras), each about 60 kB; the
listener's and the engine's next. G's builder will add its own when it lands.

## FMinus's levers, again, for the compile

- **-O1**: 3.0 against 3.3 s (8 % less), plays as -O2 (q2). Not worth a change.
- **-O0**: **0.30 s** (428 kB), and plays at **2.6 to 2.8 times** -O2's cost (7 runs, drive-car): the V8 4974 ns, the
  twin-turbo V8 with every switch on **7721 ns: about 4.0 R**, which at the owner's slower figure (R = 40 % of a core)
  is 160 %: it **cannot keep up** in real time on that machine with the typical build, nor (at 2 R) at 48 kHz inside.
  The -O0 first sound is not viable as things stand; strike it from the proposals.
- **Knob versions and `param`**: the simulator still has no `param` (`grep -rn "^\s*param " sim`: none), so the
  knob-version cap multiplies nothing. Nothing to gain there.

## Defining a switched-off model out: the numbers first

FMinus gives every module method external linkage (`define void @"Module::method"`), so a model switched off in the
build is still compiled whole: LLVM cannot drop a function the host might look up. To see what compiling it not at all
would give, a copy of `sim/` in which every method of the boost machine (`BoostMachine`, its compressor, turbine,
housings, volumes, ends, shaft, supercharger, sound), the listener's model 1 (`Scene`, `SceneView`, `Place`,
`CabinBoom`) and the mechanical voices (`Voices` and its nine voices; Extras kept) has an empty body
(`docs/measurements/tools/stub.py`; their fields kept so the callers still compile: an approximation of `#ifdef`
around them):

| | Code | Compile | Cost on V8 and Large Diesel (free-revs, drive-car; 7 runs; the models off in both) |
|---|---:|---:|---:|
| Everything compiled | 466 kB | 3.51 s | 1 |
| Those three defined out | 358 kB (-23 %) | 2.79 s (-0.72 s, -20 %) | 0.999 (wall), 1.000 (processor time): **no change** |

So: **about 6.7 ms of compile a kB of machine code**, and a switched-off model costs nothing at run time (a predicted
branch; the hot loop is no faster without it). Defining out every model a build does not use (for a naturally
aspirated reference build: the boost machine, the voices, the listener's model 1, the vehicle's model 1 and the bike,
the valvetrain's lab, H's maps and heavy diesel: some 200 kB) would take the compile **from about 3.5 s to about
2.1 s**, and save no processor time while playing. The price: a model's switch becomes a recompile and a crossfade
(2 to 3.5 s before the new model sounds), and every combination of defines is a different program for the tests
(the null test would need running per combination the application can produce, or the defines restricted to whole
models that X's combo tests already flip).

## What would help more, from FMinus (for the owner of the compiler; not bugs)

1. **Compiling in parallel.** All of the 3.5 s is LLVM's pipeline on one thread over one module. Split per FMinus
   module (or per file) and given to ORC's concurrent compile on the machine's cores, the 4-core container would
   compile in about 1 to 1.3 s, the owner's 8 to 10 cores faster still, with the program unchanged and nothing for the
   packages to do. This is the largest lever on the launch.
2. **Compiling on first call** (ORC's lazy reexports): a function compiled when first called. A switched-off model would
   never be compiled, with no defines and no recompile on a switch (the first call to a newly switched model's code
   compiles it, off the audio thread only if the host warms it: this needs care for `nonblocking`).
3. **A cache of the machine code** keyed by the source and options (ORC's object cache): a second launch of an
   unchanged program loads in milliseconds. Most launches are of an unchanged program.

## Recommendation

1. Ask FMinus's owner for (1) parallel compilation, and (3) the object cache: together they make the launch cost a
   non-issue without any package changing its code.
2. Until then, defines per model are worth 1.4 s at launch for a reference-like build and nothing at run time; do
   them only if the launch must come down before FMinus can, and only for whole models (boost machine, voices,
   listener model 1, vehicle model 1).
3. -O0 first sound: no (4 R with the typical build).

## Reproduce

```
build-dev/q/qbench --a sim --b sim --sizes 1 --builds v8 --scenarios free-revs --repeat 1 --seconds 0.5
cp -r sim /tmp/sim-stub && python3 docs/measurements/tools/stub.py /tmp/sim-stub BoostMachine,...,Voices
build-dev/q/qbench --a sim --b /tmp/sim-stub --builds v8,large-diesel --scenarios free-revs,drive-car --repeat 7 --seconds 8
build-dev/q/qbench --a sim --b sim --benv TEE_Q_OPT=0 --builds <every-switch build>,v8 --scenarios drive-car --repeat 3
```
