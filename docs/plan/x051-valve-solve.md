# X-051: the valve's flow solve, the design (next window's item 17)

Card G writes this page (the owner of `sim/exhaust/duct.fm`, where `Valve::flow` lives). Status: THE DESIGN, RULED (the lead, 26 Nov 12:33).
Item 17 takes its place in the next window in the form measured below: the warm-started secant on the one
continuous equation, the test at 1e-4 relative, the bracket kept with bisection, cold at each opening, reversal and
reset, behind `spec.exhaust.valveSolve` with today's three passes kept verbatim for the null. Its place in the
window's order is the lead's. **Corrected 26 Nov 17:40:** the reason given at 12:33 for putting it first ("it moves
every reference and every bound") is false of this design. With a model off, each valve runs today's code to the bit,
so the combo's every-switch-off references and every nightly bound stay as they are (below, "What moves").
It needs, as every sim change does, a bake to follow (P), and the owner's
two lines in the cylinder (the switch's hand-over; `wOn = 0` in `Cylinder::reset`), the integrator's. Nothing in
`sim/` until the window can take a bake. The lead's ruling of 26 Nov 10:35 set its
direction: one continuous equation, a bracketed root with a convergence test, both branches, a hidden switch.

## 1. The fault's reach (26 Nov; the lead's ruling of 11:46: item 17 stays a design and drops behind items 16 and 15)

**The instrument.** A counter in a scratch tree's `Valve::flow` (exhaust side): a sample whose returned flow (q g)
reverses direction twice running, each step more than 25 % of its size and more than 0.15 m^3/s. That is the chatter's
signature, summed over the cylinders. Checked on the combo turbo six's pull (exhaust.model 1): it finds the relights
after the 2nd and 3rd shifts (6.5 s: 8 to 18; 8.5 s: 4 to 6), with the reference valves and with the lab's.

**On what ships** (every model on: the lab's valves, the boost machine, the map ECU), as built:

| build | combo cold start (253 K, through the catch) | combo pull | lug: up to 4th at 0.25 pedal, then full pedal 9.5-15 s |
|---|---|---|---|
| turbo-four-20-gdi | 0 | 0 | 0 (from 163 rad/s) |
| kei-660-triple-turbo | 0 | **4** (9.03-9.12 s, after its 3rd shift) | 0 (from 179) |
| straight-six-twin-turbo-30 | 0 | 0 | 0 (from 170) |
| three-cylinder-10-turbo | 0 | 0 | 0 (from 179) |
| v6-35-twin-turbo-pickup | 0 | 0 | 0, but not lugged: it held about 620 rad/s |
| boxer-four-turbo-unequal | 0 | not run | 0 (from 161) |
| inline-five-turbo-rally | 0 | not run | 0 (from 265) |
| flat-plane-v8-39-twin-turbo | 0 | not run | 0 (from 308) |
| twin-turbo-hot-vee-v8-40 | 0 | not run | 0, but not lugged: it held about 700 rad/s |
| first.json, default.json | 0, 0 | 0, 0 | not run |

So on shipped builds X-051 is nearly absent: one event train on one build. The rows it cures (X-049, X-043) are
reference-model combinations on the combo fixtures.

**What stays true:**
- **(i) A real numerical fault, on every build.** The running solve is about 5 % unconverged on every build (the
  median spread between its last two passes on the turbo six), so every reading and every table sits on it.
- **(ii) An owner can make the combinations that chatter.** All nine model selectors are shown in the interface, none
  hidden: boost ("Model": Reference, Machine), car ("Vehicle model"), ECU ("ECU model"), exhaust ("Exhaust model"),
  fuel ("Combustion model"), intake ("Induction model"), sound ("Listener"), valve ("Valvetrain model"), voices
  ("Mechanical voices"), each Reference against its new model. A shipped build with boost and the ECU set back to
  Reference, keeping the Builder exhaust, is a click away from the fixtures' combinations.
- **(iii) A running regime, not only a first pulse.** The combo turbo six (exhaust.model 1, the reference turbo) also
  chatters through 1st gear at about 2000 rpm (210 to 230 rad/s) as the pedal goes from 0.5 to full (3.5 to 4.7 s of
  the pull, 17 to 89 counted a half second). Captured at 3.95 to 4.01 s:
  - the wave arriving at the valve as it opens is a deep rarefaction, -37 to -61 kPa (the primaries' return at low
    speed); at its deepest the linear pipe pressure 101325 + 2 b is below zero and the 8000 Pa floor holds it;
  - the cylinder is at 171 to 464 kPa, the solve's passes spread by 61 % (the median at the jumps);
  - the same corner as the relight, reached in steady running.
  None of the shipped builds reached it on the lug above. Two were not lugged (they held their speed), and the
  lug ran in 4th only, so this is a probe, not a proof. The test below guards it.

## The fault, as measured (26 Nov; X-049's probes, G and N1)

- **Where:** `Valve::flow` (sim/exhaust/duct.fm), called each sample for each cylinder's exhaust valve (`vE`) and
  intake valve (`vI`) by `sim/core/cylinder.fm` (and the two-stroke's `cylinder2.fm`). The reference has its own copy
  (`sim/ref/pipes.fm`), untouched.
- **What:** out of the cylinder it runs **three fixed passes** of a fixed-point iteration (`kValvePasses`, two under
  TEE_FAST). After them a choke test (`rr < rc`, 0.5457 for the exhaust) replaces the result with a closed-form choked
  flow. On a cylinder's first pulse into pipes a cut has emptied (the arriving wave -57 kPa, the pipe about 60 kPa
  absolute, the cylinder 450 to 900 kPa) the iteration is a slowly damped oscillation (passes: 1.67, 0.27, 0.75,
  0.38, 0.59, 0.43 ... m^3/s, converging near 0.49). Its third pass's ratio wanders across 0.5457 from sample to
  sample, so the result jumps between the open branch (0.75 x g at its cap 1.6, about 1.2) and the choked one (about
  0.6): 29 jumps of more than 0.2 m^3/s per sample in 10 ms, none in running (largest step 0.026).
- **The same at every rate:** 48, 96 and 192 kHz inside (the solve is algebraic each sample).
- **Converged** (30 passes, replayed offline on the captured inputs and on a scratch tree), every such sample lands
  smoothly on 0.64 to 0.70 m^3/s, choked, the level of the running pulses.
- **Not a first pulse only:** a scratch counter (a returned flow reversing twice running by more than 25 % and
  0.15 m^3/s) also finds sustained chatter in the combo turbo six's 1st gear at low speed under load (3.5 to 4.7 s of
  the pull, 17 to 89 a half second), with the reference valves and with the lab.
- **On shipped builds** (every model on: the lab's valves, the boost machine, the map ECU) the same counter finds
  none in the combo cold start (253 K, through the catch) on all nine turbo archetypes, first.json and default.json;
  on the combo pull none, except the kei 660 triple turbo (4 events, 9.03 to 9.12 s, after its third shift). The rows
  it cures are reference-model combinations on the combo fixtures. Why the shipped builds escape is not shown (the
  machine's housing and the map ECU not leaving the deep rarefaction is a guess).

## The equation (both branches)

The law the code already states (its comments, duct.fm `flow`): through an orifice of effective area kd A, from the
upstream gas at (p0, T0) to the downstream pressure p, the volume flow at the pipe's pressure satisfies

    q p_pipe = kd A p0 sqrt(R T0) Psi(p / p0),   Psi(r) = t(r) sqrt(1 - r)

with `t` the table `kPsiE`/`kPsiI`. **The table already holds Psi flat below the critical ratio** (kPsiE[0] =
0.66726 = psi*, and kPsiE[1] = psi* / sqrt(1 - 1/64)), so the iteration and the choked closed form are two attempts
at one equation. The pipe answers the flow with its wave: p_pipe = pq + z g(q) q, with pq = max(101325 + 2 b, 8000)
and g(q) = min(1 + gk q, 1.6) (`gk` = 0.59 z / (1.4 x 101325)).

- **Out of the cylinder** (pc >= pq): find q >= 0 with

      F(q) = q pp(q) - kd A pc sqrt(R tc) Psi(pp(q) / pc) = 0,   pp(q) = pq + z g(q) q

  pp rises with q (g q is non-decreasing, g capped), and Psi does not rise with its ratio, so **F is strictly
  increasing**. F(0) = -kd A pc sqrt(R tc) Psi(pq / pc) < 0. At q_hi = kd A pc sqrt(R tc) psi* / pq, F(q_hi) >= 0,
  because pp >= pq and Psi <= psi*. So **one root in [0, q_hi]**. `kd` (the blowdown's jet: 0.9 below rc to 1.0 at
  rc + 0.15, on r0 = pq / pc) is fixed for the solve, as now.
- **Into the cylinder** (pq > pc): find qi >= 0 with

      G(qi) = qi - A sqrt(R tp) Psi(pc / pp(qi)),
      pp(qi) = max(pq - z g(qi) qi, 1000),   g = max(1 - gk qi, 0.5)

  which is the code's own law: its k = ci t(rr) / sqrt(pp) and qi^2 = k^2 (pp - pc) reduce to qi = ci t(r)
  sqrt(1 - pc / pp) = ci Psi(pc / pp), ci = A sqrt(R tp). As qi grows, pp falls (g qi is
  non-decreasing: its slope 1 - 2 gk qi is positive below the cap and the cap makes it linear), the drive pp - pc
  falls with it, and the law's flow falls: **G strictly increasing**, G(0) < 0. The upper end: the law's flow at pp
  = pq. One root.
- **What is returned:** q (or -qi), with `g` and `r` taken at the root (as `qEa = qE g` and the cylinder's mass flow
  read them).

## The solve: a bracket and a test, never a count

- **The form, measured (26 Nov; the lead's ruling of 12:18 set the target):** a **warm-started secant** in the
  bracket. The first guess is the last sample's root when the valve flowed the same way, with the last solve's secant
  slope as the first step's slope. The bracket's top is free on a warm start (C psi* / pq, as Psi <= psi*); a cold
  start (each opening, each reversal, after a reset) takes the tighter C Psi(r0) / pq (one lookup) and starts at
  the bracket's middle. Each step is a secant between the last two evaluated points. Whenever a step would leave the
  open bracket, it bisects instead. No slope of Psi, no division by one.
- **The test:** stop when |F| < 1e-6 C (into: |G| < 1e-6 of the bracket's top), **testing F first and keeping the
  evaluated q**, or when the last step is under 1e-6 m^3/s + 1e-4 q. At most 24 evaluations: the bisection
  fallback bounds it, and a counter of solves that ran out goes to telemetry, to read 0 (a test holds it there).
- **A named hazard: the root hit exactly.** In float32 a step can land on the root exactly (F = 0.0). In the first
  scratch build the test came after the bracket update: the root became the bracket's top, the next step "left" the
  open bracket, and the solve bisected away from the root. The |F| test then stopped on the bisected point, not the
  evaluated one: half the flow, alternating sample to sample, worse than today (16,435 counted in 1.4 s). Duct.fm's
  old note ("a Newton iteration on the compressible law chattered where the drop is tiny") may be the same thing.
  **Its test:** a case whose root is hit exactly, found for the form that is built. For the warm Newton, my float32
  emulation found one in the captured stream: pc 206546 Pa, tc 1215.93 K, b -34549.3 Pa, z 339833, area 3.38782e-4
  m^2, started from q 0.3, lands F on 0.0 at q 0.20369. The solve must return 0.20369, not 0.10184. The secant's own
  case is found the same way, a search of the captured stream for F = 0.0, before its code is accepted.
- **Evaluations measured** (the combo turbo six's pull, exhaust.model 1; one evaluation = one Psi lookup, the cold
  top's counted):

  | window | warm Newton (exact slope, 1e-5) | warm secant (1e-4) |
  |---|---|---|
  | start 0.2-2.0 s | 2.60 (95th 6) | 2.53 (95th 5) |
  | low gear 3.5-4.7 s | 2.66 (6) | 2.61 (5) |
  | steady 2nd 5.0-6.4 s | 2.41 (3) | 2.29 (3) |
  | relight 6.45-6.70 s | 2.38 (3) | 2.29 (3) |
  | relight 8.45-8.70 s | 2.37 (3) | 2.26 (3) |
  | steady 3rd 9.0-10.4 s | 2.39 (3) | 2.27 (3) |
  | lift, downshift, stop 12-14 s | 3.81 (10) | 3.57 (9) |
  | the whole pull | | 2.68 (7) |

  The chatter counter reads 0 in every window with either form (today: 243 on the same run).
- **The memory and correctness:** the answer does not depend on the memory, because the bracket holds from any
  start; only the number of evaluations does. The same run twice is identical to the bit. A reset clears it (`wOn =
  0` in `Cylinder::reset`: the owner's line). A reload carries it with the state by field name, and a model switch
  leaves a valid root as the start: either way the same root to the tolerance. A reversal (the remembered root's sign
  against the branch) is a cold start, so a remembered root never sits on the wrong side of the new bracket.
- **The edges:**
  - **lift near zero:** `area <= 1e-9` returns 0 as now. Small areas make q_hi small and the bracket tight; nothing
    divides by the area.
  - **pc near pq:** F(0) and q_hi go to 0 together; the root goes to 0 continuously. The branch choice stays pc >= pq,
    and both branches give 0 there, so a flow crossing zero is continuous.
  - **reverse flow:** the into-cylinder branch, as above.
  - **the floors:** 8000 Pa on pq and 1000 Pa on the into-branch's pp stay inside pp(q); with them F and G are
    non-decreasing, not strictly, and the bracket still holds.
  - **the floor in running, measured** (the lead's note of 11:53). In the low-gear regime (§1 iii, 3.95 to 4.01 s)
    the 8000 Pa floor binds in 30 of 1389 flowing samples (2 %), and 2 of the 9 jumps fall on a floored sample. At
    the relight (6.555 to 6.615 s) it binds in 6 of 1917 (0.3 %), 2 of 39 jumps. So the chatter is the solve's,
    not the floor's. Where it binds, though, the linear wave puts the pipe's pressure below zero (101325 + 2 b down
    to -38 kPa), the wave model beyond its range. **A converged root there is converged to the clamp, not to the
    gas.** The converged solve cures the chatter; it does not cure that. Flagged here as its own question: the
    pipes' linear rarefaction (a real one cannot go below some fraction of the gas's pressure; the simple wave's
    nonlinearity limits it). A counter of floored solves goes beside the chatter counter, so its rate is known
    before anyone designs for it.
  - **a chamber that empties within the sample:** not this solve's (the cylinder's step holds its own guard).

## The switch, and the line that is not mine

- **The switches: each valve follows its own side's model** (the lead's ruling, 26 Nov 17:35).
  - `vE` converges where `exhaust.model != 0`, under a hidden `spec.exhaust.valveSolve` (G's field).
  - `vI` converges where `intake.model != 0`, under a hidden `spec.intake.valveSolve` (N1's field, to be added in the
    window; the code is G's).
  - Each defaults to the converged solve under its new model, and runs the old passes with its model off. A build
    with the new exhaust and the reference intake keeps the reference's intake valve to the bit.
  - With a valve's model off, or its switch 0, `Valve::flow` runs today's code **unchanged, to the bit**: the old
    passes are kept verbatim as one function, and the new solve sits beside them.
  - One `Valve` type, its instances told apart by a per-instance flag the cylinder sets from its side's model and
    switch: nothing awkward. The instances in `sim/`: the core cylinder's `vE` and `vI`, and the two-stroke's
    `cylinder2.fm` `vE` (card T's, which keeps the old passes unless T hands it a flag). The exhaust's `vLaw`
    (slots.fm) uses only the table. The boost machine's ends are its own type (`BoostEnd`, gas.fm), untouched.
- **The identity tests pin their own switches:** test 2 ("model 1 at its defaults is model 0 in the engine, every
  layout", tests/sim/exhaust/EngineTest.cpp) sets `exhaust.valveSolve 0` beside portHeat 0, flowClose 0 and portMean
  0; the intake's own "model 1 at its defaults is model 0" test sets `intake.valveSolve 0` (N1's).
- **The one line I do not write:** the cylinder (`sim/core/cylinder.fm`, the integrator's) must hand the switch to
  its valves (e.g. `vE.converge = ...; vI.converge = ...` where the cylinder is set from the Spec), and so must the
  two-stroke's `cylinder2.fm` (card T's) if it is to share it. Named here as its owner's line. I write it only if
  my permission check allows it at the time, with a dated note in the owner's handoff.
## The cost

- **The ruler** (the lead's of 12:18): the solve's inputs captured sample by sample, cylinder 1's exhaust and intake
  valves, 0.17 s windows of the combo turbo six's pull, replayed outside the engine. The replay is C++ in float32,
  built -O2 -ffp-contract=off, written as the FMinus code. Its form of today's code reproduces the engine's captured
  flows to 6.3e-6 m^3/s in every window, both valves. Nanoseconds per call, the median of 15 batches (each of 20
  repeats of the stream), the batch range in brackets:

  | window | today: 3 passes | warm Newton 1e-5 | warm Newton 1e-4 | warm secant 1e-5 | warm secant 1e-4 |
  |---|---|---|---|---|---|
  | running (steady 2nd, 3rd, both relights) | 50.5 (48.6-60.7) | 51.4 | 47.4 | 41.9 | **38.3 (37.8-39.4)** |
  | start | 38.6 (32.5-55.6) | 53.8 | 49.9 | 45.3 | 43.0 |
  | low gear | 39.7 (38.9-48.1) | 55.3 | 51.7 | 50.0 | 55.0 (47.2-66.0) |
  | lift, downshift, stop | 52.0 (51.6-57.2) | 82.4 | 79.2 | 74.3 | 67.4 |

  - **On the running windows** the secant at 1e-4 costs 24 % less than today's three passes. The secant alone buys
    18 % (51.4 to 41.9), the looser test alone 8 % (51.4 to 47.4).
  - **Where flows reverse often** (start, low gear, the stop) it costs 11 to 39 % more per call; the cold starts are
    most of it.
  - **The looser test's price in the answer:** against a tight solve (1e-7), the largest difference in the returned
    flow is 2.2e-6 m^3/s in running, 3.0e-4 at the start, 8.9e-5 in low gear and 5.6e-4 at the stop: relative 2e-5
    to 9e-5. Today's three passes differ from it by up to 0.56 m^3/s (relative 0.82) at the chatter.
- **The whole program** (`tee-fuzz --play turbo-six pull none`, % of a core, nine alternated runs, counters
  stripped): the tip 15.1, 16.6, 16.3, 15.8, 16.4, 16.3, 16.2, 14.9, 14.8 (median 16.2); the warm secant 15.4, 15.2,
  15.9, 15.3, 15.4, 15.7, 15.1, 15.6, 16.1 (median 15.4). Pairwise median **-0.5 points (-3 %)**.
- **Against the target** (per call on the running windows no more than today's; the whole pull no more than 2 %
  over): **met**, on one container. The earlier forms for the record:
  - 30 passes: 55 % of a core against 15 %;
  - the warm Newton as first built: a median +1.0 point over five runs, its noise a point.
  Q reads the secant on Q's hosts.
- TEE_FAST keeps its 2 fixed passes.

## What moves that nobody asked to move

- **Every running sound.** The running solve is unconverged too: about 5 % between its last two passes (the median
  over cylinder 1's samples on the turbo six's pull). Converging it moves every build's flow a little, and with it
  every wave, every table and every pinned value that reads the exhaust's sound.
- **Predicted band moves, before code** (third-octave levels of a fixed few seconds, the converged solve against
  today's, read on the real code):
  - **default.json** (no chatter found), a free rev: every band within +-0.5 dB.
  - **kei-660-triple-turbo** (chatter only after its third shift), the pull's 7 to 11 s: within +-0.5 dB, except
    the 2 to 8 kHz bands over its third shift's 9.0 to 9.2 s, down 1 to 3 dB.
  - **the combo turbo six with exhaust.model 1**, the pull's 1st gear under load (3.5 to 4.7 s): the bands above
    4 kHz down 3 dB or more (the sustained chatter is a sample-rate buzz).
- **Clips for the owner's ears** (26 Nov, before code, on the 30-pass stand-in): the kei 660 triple turbo, as
  shipped against converged, the same seconds at one level, not normalised: a cold start (0 to 6 s), a full-throttle
  upshift (8.0 to 10.5 s), a steady 2nd-gear mid load (7 to 11 s). Their RMS levels agree within 0.2 dB. The
  waveforms part 0.27 s into the start (the first valve openings) and then drift, so only the ear can compare them.
- **What does NOT move (corrected 26 Nov 17:40; from the code):**
  - the combo's every-switch-off references, and so **every nightly bound**: with exhaust.model and intake.model off,
    both valves run today's code to the bit;
  - every case with both of those models off, which must read **the same to the bit**: X-030 (fuel+ecu+car), X-031
    (fuel+ecu), X-044 (custom+fuel+ecu), X-048 (wallHeat+valve+boost), X-041's tour lift (valve+boost+ecu), and
    X-052's wallHeat+ecu+car and the V8's valve+fuel+ecu.
- **What moves:** a case with exhaust.model on (its `vE`) or intake.model on (its `vI`), read **against today's
  unchanged bounds**. Among them: X-043's two rows (intake+exhaust+car), X-052's valve+intake+car and
  intake+exhaust+voices, X-049's, and every shipped build, which runs every model, in its own cases and tables.
- **How to read the stand-in tables below:** my 30-pass and secant scratch trees changed the solve for every run,
  every case and every reference. So their "own bound" columns, and any bound computed on them (the steward's V8
  "after item 17" reading against 2.58), describe a tree that will not exist. **The columns to read are "over X's
  bound".** Even those overstate the change for a case with only one of the two models on: the scratch trees
  converged both valves everywhere.
- **The list of rows whose verdict changes** is part of the change: each entered or released by X on the spread,
  never by a single draw.

## The real code: predictions before its first run (27 Nov, G; a local worktree, nothing pushed)

The window's form (`flowPasses` verbatim, `flowConv` the secant, `flow` choosing by the cylinder's flag; the switch
`spec.exhaust.valveSolve`; the instruments `tel.exhaust.valveChatter`, `valveFloored`, `valveRanOut`; the owner's
lines in engine.fm `applyModels` and `writeTelemetry` and in cylinder.fm `reset`; the intake's half not yet: `vI`
keeps the passes until N1's field). Before its first run:
1. ref.null (every case) and test 2 (its 3 layouts; 17 nightly) identical to the bit.
2. The hazard's case (`the valve's converged solve keeps a root it hits exactly`) passes.
3. The combo turbo six's pull, 0 to 5 s, the builder: chatter about 270 on the passes (the scratch counter's 76 over
   the start and 198 in the low gear) and 0 converged; with exhaust.model 0 the switch not heard.
4. The nine shipped boosted builds (nightly): chatter 0 on the cold start, the pull and the lug. "Unfinished" may not
   be 0: the scratch trees' histogram had solves of 15 or more evaluations.
5. The other exhaust cases under the builder move within their bounds; a red one is explained, never loosened.

## The real code, as measured (27 Nov, G; local commit 7f7219ea on dfd86fcd, nothing pushed under sim/)

**Against the predictions above:** 1 held (ref.null and test 2 to the bit; with exhaust.model 0 the switch is not
heard). 2 held. 3: chatter 228 on the passes (predicted about 270), 0 converged. 4: chatter 0 on all nine shipped
boosted builds' start, pull and lug. "Unfinished" first read 1 to 49 a run, mostly on the lugs. 5: two cases went
red on the first tier (below).

**Two changes to the solve after its first run** (neither loosens a test):
- **The bracket-halving safeguard.** A secant step is taken only while the bracket has at least halved in two steps;
  otherwise bisect. Found on the turbo six's into-branch at a 58 Pa drop, where the secant wandered for 24
  evaluations. It is likely what duct.fm's old note about a Newton that chattered at a tiny drop meant.
- **The tolerance scaled to the valve.** The bracket and step tests are now `1e-4 x` the valve's own scale (its
  choked flow at pq: C psi*/pq out, ci psi* in) `+ 1e-4 q`, where they were a fixed `1e-6 m^3/s + 1e-4 q`. At a
  trickle (a tiny drop on a lug) a fixed 1e-6 sat under float32's resolution of the root, and the solve bisected to
  24 evaluations. After both changes, "unfinished" reads 0 on every nightly run and on the turbo six.
- **What an unfinished solve returns:** its last point, strictly inside the kept bracket. The safeguard halves that
  bracket at least every two evaluations, so after 24 its error is under about 2^-11 of the top, roughly 5e-4 of the
  valve's choked flow, on one sample. Today's passes are wrong by up to 98 % on a chattering sample. Neither the
  null (the switch is gated by the model) nor an ear could tell one such sample. The counter is there to keep it at 0.

**The tier:** 580 of 582 in 7 min 26 s, from the scratchpad worktree, -j4. The two reds are the window's known
ones. host.sim.bakes: the knock key mixes `sim->sourceHash()`, so any sim change stales it until P bakes.
assistant.sim: its socket path under the long worktree path is 118 characters, over the 108 limit, which is
environmental. The tier of record is one whole run from a short path.

**The two moves, each spread over 51 nudges of the air (+-0.1625 K) on both trees** (predictions written first, in
the scratchpad's x049/pred-two-moves.txt):

| | the tip | item 17 |
|---|---|---|
| the rally five's lift, Burned - Carved (median) | +0.99 dB | +0.54 dB |
| copies under the case's -2 dB | 2 of 51 (worst -3.28) | 2 of 51 (worst -3.95) |
| Carved body / Burned body (median, dBFS) | -26.42 / -25.34 | -25.88 / -25.31 |
| the flat-plane V8 at 8700 rpm, torque (mean, sd) | 396.50 (1.69) N m | 392.28 (1.28) N m |
| its standard error over 0.005 of the torque | 6 of 51 | 16 of 51 |

- **The lift is a sometimes thing on both trees**, 2 of 51 each, so it is a row with a count, not item 17's. The
  converged valve does not move the bangs' body (Burned -25.34 against -25.31). It raises the Carved body by
  0.5 dB, which narrows the margin. The owner's default ("burned in the pipe") reads as it did. My prediction A held
  on 51 copies; on the first 17 the tip had none under -2.
- **The V8 moves in both its torque and its noise. My prediction B was wrong.** Its passes never chatter at
  8700 rpm (counter 0, both switches), but the pipe floor binds: 728 floored solves a point on the passes, 458
  converged.
  - **The torque** (-1.1 %, about 2.5 sd) is a correction. With the passes kept wherever the floor binds, the torque
    still reads 392.35 N m. So it comes from ordinary samples, where the passes sit about 1 % off the equation's root.
  - **The noise tail** is the floor's. With the passes kept where the floor binds, it reads 7 of 51, the tip's rate.
    So it comes from the converged root landing on the 8000 Pa clamp: the linear wave beyond its range.
  - **The passes on floored samples are no cure.** With that gate the turbo six's pull reads 5 chatters
    (4321 floored on the passes, 66 converged) and the twin-turbo six's pull 1. The nightly and the chatter case go
    red.
- **So item 17 as built cures the chatter everywhere,** but triples how often the V8's top point fails the
  predictor's settle floor (6 to 16 of 51). That case is one draw; it passes at the case's own draw on both trees
  today. The honest cure for the tail is the pipes' linear rarefaction (§ the edges, "the floor in running"), not
  this solve. That is a design of its own, not this window's.

**The cost:** on the replay, with the real tolerance, 40.9 against 44.6 ns a call (-8 %), 1.67 evaluations a call,
0 out of evaluations. The whole program (tee-fuzz --play turbo-six pull, 9 alternated runs, tip against item 17's
tree): +0.7 % in median, pairs from -1.3 to +1.0 points, within the noise.

## Acceptance, written before code

1. **The null, to the bit,** with both models off or both switches off: ref.null (every preset, the layouts, the
   rates and blocks) and test 2 on every layout (the nightly's 17).
2. **The rows, by the evidence rule** (X's spread.sh, 17 copies, the two-tree mode, the source against the new code):
   - **cured, read per event** (X's part B), against today's unchanged bounds: X-043's event, the start's catch (2.7
     to 4.8 s), at 0 of 17 on both its rows (intake+exhaust+car, the turbo six's tour and pull), and whatever the turbo
     six's mild-cam twin (X's, a dial equal to the lab's default cam) shows. On the secant's scratch tree, against X's
     bounds: the pull 7 -> 0 of 17 (2.67); the tour 11 -> 1 (2.96). The tour's one is **another event** (dT -0.05,
     3.09 at 6.801 s, in the full-throttle stretch, judged "none"): it is spread per event on the real code against
     today's 2.96, and entered only if that spread shows a copy over;
   - **untouched, to the bit** (both models off in the case): X-030, X-031, X-044, X-048, X-041's tour lift, and X-052's
     wallHeat+ecu+car and V8 valve+fuel+ecu. Any move in one of them is a fault of the gate;
   - **moved, read against today's bounds:** the rally five as built (every model on; 1 over 3.00 today), and X-052's
     valve+intake+car and intake+exhaust+voices (on X's corrected corpus, per event, today: 0, and 1 at the limiter at
     3.03; on the secant's scratch tree: 0, and the limiter's worst 2.99, one copy over 2.97).
   - X-049 is not evidence: on X's corrected corpus (the turbo six's lab cams stated equal to its dial) it reads 0 of
     17 on the tip too (N1, 26 Nov). Its valve rows were comparing the lab's default 231 degree cam with the dial's
     300. The chatter itself (§1, the relight and the low-gear captures) was measured with valve.model off, the
     reference valves on the dial's cam, and stands.
3. **The instrument kept, the absence guarded** (the lead's ruling of 11:46, whatever is decided about the solve):
   - the chatter counter (§1) as a hidden telemetry field, `tel.exhaust.valveChatter` (a running count), written by
     the cylinders' valves;
   - a default-tier test that every shipped boosted build reads 0 on a cold start, a pull and a lug (the lug driven
     into its high gear at low speed, so that it really lugs, which my scratch lug did not do for two builds);
   - with the converged solve, the counter of solves that ran out of evaluations reads 0 on all 31 archetypes'
     validation runs and every default-tier scenario.
4. **The bake's gate,** warm and cold on all 31; **Table P2** re-read (the archetypes' cited rows, the two columns,
   every row crossing an edge listed with its direction and size); **Q's cost** on Q's hosts, on the real code in the window.
5. **The owner's ears** on the clips and on the real code's, before the bake.

## The rows as measured on the stand-in (26 Nov; spread.sh, 17 copies, one hand, tip against the 30-pass tree)

| row | tip: over (own) | 30-pass: over (own) | case median tip -> 30 | reference median tip -> 30 |
|---|---|---|---|---|
| X-049 (valve+exhaust+sound, pull) | 2 | 0 | 2.30 -> 1.52 | 1.64 -> 1.90 |
| X-043 tour (intake+exhaust+car) | 11 (11) | 0 (0) | 3.13 -> 2.38 | 1.80 -> 2.21 |
| X-043 pull | 7 (7) | 0 (0) | 2.01 -> 1.45 | 1.64 -> 1.90 |
| X-048 (wallHeat+valve+boost, pull) | 14 (14) | 8 (3) | 2.85 -> 2.65 | 1.64 -> 1.90 |
| X-031 (v8 pull fuel+ecu) | 7 (7) | 2 (14) | 3.66 -> 3.06 | 2.09 -> 1.61 |
| X-030 (small-four pull fuel+ecu+car) | 13 (13) | 9 (9) | 3.20 -> 3.25 | 1.48 -> 1.53 |
| X-044 (small-four custom+fuel+ecu) | 11 (11) | 12 (12) | 3.35 -> 3.30 | 1.48 -> 1.53 |
| X-041 tour lift (valve+boost+ecu) | 3 (3) | 3 (0) | 1.87 -> 1.91 | 1.80 -> 2.21 |
| X-041 rally five, as built (over 3.00) | 1 | 1 | 2.15 -> 2.26 | (the case) |
| control: v8 pull exhaust.model | 0 | 0 | 2.28 -> 1.78 | 2.09 -> 1.61 |
| background: large-diesel tour exhaust.model | 0 | 0 | 1.59 -> 1.66 | 1.60 -> 1.81 |
| background: turbo-v8-dual pull exhaust.model | 0 | 0 | 2.56 -> 2.20 | 2.76 -> 2.43 |

Counts against X's nightly bounds; in brackets, the tree's own (twice its reference's lowest of the five copies).
The predictions written before these runs (G's scratchpad, PRED-rows.md) were wrong on three rows: X-043 (predicted
to stay, cured), X-044 and the rally five (predicted to fall, unchanged).
