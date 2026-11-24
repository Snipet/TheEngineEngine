# Phase 0, part K3: the new contract, and the split

*Serial, after K2 ([p0-keel.md](p0-keel.md)) can render the reference. One agent. Repository: TheEngineEngine,
integration branch `claude/awesome-gates-ntdb58` (fetch and merge before every push). Read
[01-rules.md](01-rules.md) and [02-contracts.md](02-contracts.md) whole: this card makes that file's simulator
side real. Every sim package of wave 1 waits for this card's second half, so push each step as it passes.*

**OWNS**: `sim/**` except `sim/ref/**` (read-only, the baseline); `tests/sim/**`; `content/builds/**`;
`sim/README.md`; and in `host/` and `tools/` only: `host/include/tee/EnginePlayer.h`, `host/src/EnginePlayer.cpp`
and the `--engine` path of `tee-render` and `tee-bench`.

## The one test that rules this card

**The null test.** For each of the reference's presets and each scenario that drives it by pedals (CAR and TRUCK
modes, and free revs by the throttle in neutral), the new program's output and the reference's, rendered in the
same process from the same inputs, differ by nothing, or by less than -100 dB under the signal where a step
changed the order of floating-point work (say which step and why). `tee::test::nullDepth` is K2's. Every step
below ends with the null test green and a push. A step that cannot keep it green is undone and thought about
again; the model is not "improved" in this card, however tempting a fix looks. Note what you would fix in
`sim/README.md`, under "Left as the reference has it".

## K3a: the contract over the monolith (push within the first hour or two)

1. `sim/spec/`: `limits.fm`, `tables.fm` (`Curve16`, `Map16` and their interpolating reads, with tests of their
   own), `spec.fm` and one file a section holding the first Spec of 02-contracts.md §2's table (every field with
   its attributes: the reference's ranges and defaults, converted to SI where the table says so), `controls.fm`,
   `telemetry.fm`, `scope.fm`.
2. `sim/engine.fm`: `module Engine` with `spec`, `ctl`, `tel`, `scope`, and inside it the reference's `Shared`
   **copied unchanged** into `sim/core/monolith.fm` (its includes copied beside it), driven through an adapter:
   each 16 samples the Spec's sections whose `rev` moved are written into the monolith's `Set` (units converted
   back), `ctl` into the pedals, lever and buttons; after a block the `show*` values are unpacked into `tel`.
   `init`, `process` (nonblocking: fix what stops it being so, and say what you changed), `end`.
3. **What replaces the keys.** `ctl.ignition`: start cranks while held and the engine is not running; on runs;
   off stops it (the reference's state 3, running down). The engine never switches itself off. `ctl.mode`
   `MODE_BENCH` is the reference with no vehicle and the throttle from `ctl.throttle` (its mod-wheel path);
   `holdSpeed` above 0 is its key governor aimed at that speed. `MODE_DRIVE` is its CAR or TRUCK by
   `spec.car.kind`. `shiftUp` and `shiftDown` are its paddles. The dyno modes do nothing yet. Write the exact
   mapping, and how each null-test scenario is expressed both ways, in `sim/README.md`.
4. `tee::EnginePlayer` (a `Program`, an `Instance` of `Engine`, direct writes: single-threaded, for tools and
   tests; card C's `SimHost` is the threaded one) and `tee-render --engine`, `tee-bench --engine`.
5. `content/builds/default.json` (every default: the reference's Init V8) and the reference's three presets
   converted, in 02-contracts.md §8's format.
6. Tests: the null test; defaults equal the reference's defaults; a build file round-trips; `Engine::process` is
   nonblocking; the cost is within 2 % of the reference's (`tee-bench`).

**Push, and tell the lead: the new contract works.** Card C and the interface's binding start from here.

## K3b: the split (push module by module)

7. Carve `monolith.fm` into the modules of 02-contracts.md §6, **one at a time, the null test after each**, in
   an order that keeps each step small: `Listen` (views, distance, room, compressor, limiter, resampler) and
   `Extras`; `Car`; `Heat`; the forced induction out of `Extras` and the pipes into `Boost`; `Ecu` (state, driver,
   idle, limiter, cuts) and `Combustion` (`charge`, `chargeDiesel`); `Valvetrain` (the events and the area
   table, which the cylinder then reads through the valvetrain: one table for all, as now); `Intake` (runners,
   plenum, throttle); `Exhaust` (headers to tails, `node`, the junction losses, pops, flow noise); `Mechanics`
   (friction, the starter, the bench load) and `Crank`. `Engine` keeps the order of work and the fade.
8. Each module gets `setup`, `reset`, `apply(spec section)`, `control` and its per-sample functions
   (02-contracts.md §6), a header comment in the reference's manner (move the reference's own explanations with
   the code they explain: they are the most valuable text in the project), and reads its Spec section directly:
   the `Set` adapter of step 2 goes away piece by piece.
9. The arrays of 12 become `MAX_CYL` and `MAX_PORTS` (16); layouts stay the reference's seventeen. No behaviour
   changes for the layouts that exist.
10. `sim/README.md`: for every module its responsibility, its state in a sentence, **its functions with their
    exact signatures and when each is called**, what it reads from which other module, and the list "Left as the
    reference has it". This file is what cards D to I are written against; after this card it is theirs to keep
    true, each for its module.
11. Scope: cylinder 1's pressure, volume and the two valve lifts by degree of its last whole cycle, filled when
    `ctl.scopeOn` is set, costing nothing when it is not.
12. Tests besides the null test: each module's `reset` leaves it as `init` did (a stop and a start give the same
    samples as a fresh instance, where the reference does); an hour of random pedals and dials stays finite and
    bounded (run it shorter in `ctest`, the full hour by hand, and say which you ran); the bench within 5 % of the
    reference.

**Done when** `monolith.fm` is gone, the null test is green for every preset and scenario, and `sim/README.md`
would let someone who never saw the reference add a subsystem in the right place.

## Not in this card

Any new physics, any new parameter beyond the first Spec, any change of sound. The threaded host (card C).
