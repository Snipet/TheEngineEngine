# The rule for changes made for speed

*Package Q, 2026-11-24, as the lead decided it (14:30 and 14:58 UTC). Every change made to make the simulator cheaper,
in any `sim/` module, by anyone, follows this. The tool is `docs/measurements/tools/qbench` (build it with
`docs/measurements/tools/build.sh` after a `build-dev` build).*

## 1. Exact first

A change that leaves the output identical to the bit needs no switch and no listening: coefficients recomputed only
when their inputs move, a loop split so that its transcendental functions vectorise (FMinus's `Math::Vector` gives a
vector lane the scalar's bits), a table where the reference already reads one, a branch for a case whose arithmetic
is the same. It is proved by the null test on a freshly merged tree (`build-dev/bin/tee-tests "[null]"`: 59 pairs,
each `depth L inf R inf`) and the owning package's own tests (`tee-tests-exhaust`, `-boost`, `-core`, `-vehicle` ...).
It is committed by whoever proved it, one commit a change, the message saying "bit-exact" and the saving, with a line
to the owner's session. Do all the exact changes there are before the first switched one.

## 2. A saving is measured, not seen once

This container's host is shared: the same render can run 5 to 60 % slower from one run to the next, for several runs
together. A saving is believed when it holds with **at least 7 runs** a side (qbench alternates A and B and keeps the
least of each), **with A and B swapped** as well, and in the **processor-time ratio** qbench prints beside the wall-time
one. (A first measurement with 5 runs credited a change with 8 % that, measured this way, saved nothing.)

## 3. Not exact: judged against the engine's own sensitivity

The engine is chaotic: a change in the last bit of any number makes two renders part company at the first sound, and
the difference sample for sample then reads about 0 dB under the signal however good the change. So a change that is
not exact is judged against an **ensemble floor**: for each pair of the null test (each build and scenario), at least 8
renders of the unchanged program, each perturbed below anything physical (the outside air by 1e-4 K, seeded nudges of
the same size to other inputs), give for each measure a spread:

- third-octave band levels, 25 Hz to 20 kHz, over the whole scenario and per quarter second;
- each quarter second's level;
- the engine's speed trace (rpm apart, worst and mean), the idle speed and its lope's period;
- the steady dyno points' torque (tee-dyno);
- the clicks (the valvetrain's and the injectors' level).

A change **passes a measure** when its difference from the unchanged program lies within the ensemble's 95th
percentile on every pair. The **long-term** measures (the whole scenario's bands, the steady dyno torque) must agree
much more tightly than the fast ones; one perturbed render already shows what to expect: the whole scenario's bands
within 0.14 to 0.77 dB of each other, the speed's mean within 3 to 43 rpm, the torque within tenths of a percent. A
change that moves those beyond the ensemble is a different engine, chaos or not.

qbench reports, for an A/B pair that is not exact: how long the two stayed within -80 dB of each other, the worst
third-octave band difference over the scenario, the worst quarter-second level difference, the speed apart. The
ensemble is run as 8 A/B pairs of the unchanged program against perturbed copies (a perturbed copy is a copy of `sim/`
with one constant nudged, as `core/heat.fm`'s `tAmb + 0.0001f`); the spread is taken over those.

## 4. One switch for the whole program

A change that is not exact never replaces the exact code: it sits beside it behind **one switch for the whole
program**, `TEE_FAST`, a compile-time macro (FMinus's preprocessor: `#ifdef TEE_FAST`; the host passes it in
`tee::Program::Options::defines`). Undefined, the default and the null test's, the program is the reference's. The
application defines it only for changes that passed part 3 **and** that the owner has listened to as named
before-and-after pairs, loudness-matched (the whole-program tester's listening pack). Keep the switched changes few,
each worth it.

**Behind `TEE_FAST` today**: the valves' solve in two passes instead of three (`exhaust/duct.fm`, 42c3dd5): 10.3 % of
R (7 runs, both orders); third-octave bands 0.7 to 1.6 dB apart, beyond one perturbed render's 0.14 to 0.77, so it
does **not** pass part 3 as measured so far and waits for the ensemble and the owner's ears.

## 5. Reproduce

```
docs/measurements/tools/build.sh
build-dev/q/qbench --a <sim copy> --b <sim copy> --builds v8,large-diesel --scenarios free-revs,drive-car \
    --repeat 7 --seconds 8                                   # A against B
build-dev/q/qbench --a sim --b sim --benv "TEE_Q_DEFINES=TEE_FAST" --repeat 7   # the fast switch on against off
```
