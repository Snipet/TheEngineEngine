# X-051: the valve's flow solve, the design (next window's item 17)

Card G writes this page (the owner of `sim/exhaust/duct.fm`, where `Valve::flow` lives). Status: THE DESIGN, a DRAFT for the
lead's ruling (26 Nov; the cost and X-052's rows still to be added); nothing in `sim/` until the window can take a bake. The lead's ruling of 26 Nov 10:35 set its
direction: one continuous equation, a bracketed root with a convergence test, both branches, a hidden switch.

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

- **Method:** Illinois (regula falsi with the stale end's value halved) on the bracket above. It keeps the bracket,
  so it cannot leave it or oscillate as the fixed point does. Its first guess is the old first pass (which keeps
  running flows close to today's on the first step).
- **The test:** stop when the bracket's width or the last step is under 1e-5 m^3/s + 1e-4 q, or F's magnitude is
  under 1e-4 of kd A pc sqrt(R tc) psi*. At most 24 evaluations: if none of those holds there, return the midpoint and
  count it (a telemetry counter, to be zero on every build; a test holds it there).
- **Expected:** 4 to 8 evaluations in running, more on the edge cases. Each evaluation is one `tOf`, one sqrt and
  a few multiplies, as one of today's passes.
- **The edges:**
  - **lift near zero:** `area <= 1e-9` returns 0 as now. Small areas make q_hi small and the bracket tight; nothing
    divides by the area.
  - **pc near pq:** F(0) and q_hi go to 0 together; the root goes to 0 continuously. The branch choice stays pc >= pq,
    and both branches give 0 there, so a flow crossing zero is continuous.
  - **reverse flow:** the into-cylinder branch, as above.
  - **the floors:** 8000 Pa on pq and 1000 Pa on the into-branch's pp stay inside pp(q); with them F and G are
    non-decreasing, not strictly, and the bracket still holds.
  - **a chamber that empties within the sample:** not this solve's (the cylinder's step holds its own guard).

## The switch, and the line that is not mine

- **The switch:** a hidden Spec field, `spec.exhaust.valveSolve` (0 Passes: the reference's three fixed passes; 1
  Converged: the default), acting where `exhaust.model != 0`, as `portMean` does (docs/models/exhaust.md, "Which mean
  the port's gas should be"). With exhaust.model 0, or valveSolve 0, `Valve::flow` runs today's code **unchanged, to
  the bit**: the old passes are kept verbatim as one function and the new solve sits beside them.
- **Test 2** ("model 1 at its defaults is model 0 in the engine, every layout", tests/sim/exhaust/EngineTest.cpp) sets
  `exhaust.valveSolve 0` beside portHeat 0, flowClose 0 and portMean 0.
- **The one line I do not write:** the cylinder (`sim/core/cylinder.fm`, the integrator's) must hand the switch to
  its valves (e.g. `vE.converge = ...; vI.converge = ...` where the cylinder is set from the Spec), and so must the
  two-stroke's `cylinder2.fm` (card T's) if it is to share it. Named here as its owner's line. I write it only if
  my permission check allows it at the time, with a dated note in the owner's handoff.
- **The intake valve:** `vI` runs the same code. Whether it converges under `exhaust.model`'s switch or under the
  intake's own (N1's) is the lead's to say; the design works either way. X-051's measurements are all exhaust-side.

## The cost

- **Measured:** pending. The 30-pass stand-in against the tip, the whole program's cost on one run, is to be read
  on an idle host (X-052's two-tree spreads hold the CPUs as this draft is committed) and added here.
- **Expected for the real solve:** 4 to 8 evaluations against today's 3, about 2 to 3 times the valve's arithmetic.
  The valve is a small part of a sample's work, so the whole program's share is to be measured by Q on Q's hosts
  before the window's code is accepted. TEE_FAST keeps its 2 fixed passes (its own note already counts the passes).

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
- **Every click reading and every bound.** On the stand-in the controls' medians move 0.1 to 0.5 both ways (the v8
  pull exhaust.model case 2.28 to 1.78, its reference 2.09 to 1.61; the turbo V8 dual 2.56 to 2.20; the large diesel
  1.59 to 1.66). Every reference moves, so **every nightly bound is recomputed on the new tree**. The list of rows
  whose verdict changes is part of the change: each entered or released by X on the spread, never by a single draw.

## Acceptance, written before code

1. **The null, to the bit,** with the switch off and with exhaust.model 0: ref.null (every preset, the layouts, the
   rates and blocks) and test 2 on every layout (the nightly's 17).
2. **The rows, by the evidence rule** (X's spread.sh, 17 copies, the two-tree mode, the source against the new code):
   - **cured, 0 of 17:** X-049 (valve+exhaust+sound, turbo six pull) and X-043, both rows (intake+exhaust+car, the
     turbo six's tour and pull);
   - **rates beside today's, no verdict:** X-030 (small four pull fuel+ecu+car: 13 of 17 today, 9 on the stand-in) and
     X-048 (turbo six pull wallHeat+valve+boost: 14 today, 8 on the stand-in against X's bound, 3 against its own);
   - **expected NOT to move, a move to be explained:** X-044 (11 and 12), X-041's tour lift (3 and 3) and the rally
     five (1 over 3.00 on both);
   - **read against both bounds:** X-031 (7 of 17 today; on the stand-in 2 against X's 3.92, 14 against its own 2.58,
     its reference having fallen more than it did);
   - **X-052's limiter rows:** as read in the two-tree mode (below, when in).
3. **No unconverged solve:** the solve's counter (above) zero on all 31 archetypes' validation runs and every
   scenario in the default tier.
4. **The bake's gate,** warm and cold on all 31; **Table P2** re-read (the archetypes' cited rows, the two columns,
   every row crossing an edge listed with its direction and size); **Q's cost** on Q's hosts.
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
