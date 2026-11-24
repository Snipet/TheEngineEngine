# The induction (card E)

*`sim/intake/`. `intake.model = 0` is the reference's: one throttle, a plenum of 0.8 times the displacement, runners
of 0.30 m (a bike's 0.16) opening into it through an open end's reflection, the air box a resonant filter (90 Hz,
Q 2.2) on the throttle's flow whose rate of change is the intake's sound. `intake.model = 1` is this card's. The new
parts (`airbox.fm`, `itb.fm`, `runner.fm`) are wired through `Intake` (`intake.fm`); their fields are
`sim/spec/intake.fm`'s.*

**measured**: a test measures it; **published**: a source; **judgement**: chosen.

## The air box (`airbox.fm`)

A Helmholtz resonator: the box's air is the spring, the snorkel's air the mass.

    V / (rho c^2) dp/dt = qn - qt          rho Le / A  dqn/dt = -p - R qn

- `qn`: the snorkel's flow in; `qt`: the throttle's out; `p`: the box's pressure over the air outside, which the
  throttle then draws from (so a small box and a thin snorkel starve a big engine).
- `qn / qt` is a second-order low-pass at `f0 = c / 2 pi sqrt(A / (V Le))`: the same filter the reference rings the
  throttle's flow through, now from a volume and a neck. **Measured** (test `air box: its resonance ...`): the
  neck's flow passes -90 degrees of phase within 1 % of `f0` for 10 L and 20 L boxes and two snorkels (75.5, 53.4,
  43.6 Hz predicted; 75.5, 53.4, 43.5 measured).
- `Le` = the neck plus 0.85 r (a flanged end inside the box) and 0.61 r (the unflanged mouth) (**published**: the
  classical end corrections, Rayleigh's and Levine and Schwinger's).
- `R` gives the reference's Q of 2.2 (**judgement**: the filter's and the neck's losses together; a real paper
  element's resistance grows with the flow, left out).
- Left at 0: the box twice the displacement (**judgement**; car air boxes are some 5 to 15 L), the snorkel's bore
  1.3 times the throttle's (**judgement**), its length set for the reference's 90 Hz (**measured**: 90 Hz within 1 %).
- Solved by the trapezoidal rule on the 2 by 2 system, closed form each sample: A-stable; **measured** passive (test
  `air box: passive`: from a steady draw, its stored energy never rises over a second once the throttle stops).
- Its sound: the snorkel's flow's rate of change (a small open end radiates as a monopole), at the reference's scale
  (0.02 times dq/dt).
- Left out: the box's own acoustic modes (a 10 L box's first some 500 Hz up).

## A throttle a runner (`itb.fm`)

Each runner's far end is an open pipe's end, its trumpet in the box or the air, with a plate in it.

- The plate: an orifice, `q = K sgn(dp) sqrt|dp|`, `K = Cd A sqrt(2 / rho)`, Cd 0.8 (**judgement**), quasi-steady.
  Solved with the runner in closed form: the end's pressure is `2a - z q`, so `x + z K sgn(x) sqrt|x| = 2a - P`, a
  quadratic in `sqrt|x|`; the wave back is `a - z q`.
- Wide open this is an ideal open end (`P - a`); the open end's radiation (the reference's own end filter, Silva et
  al.'s fit to an unflanged pipe, by the bilinear transform, in air on both sides) adds what an ideal end leaves out,
  in the share the plate is open.
- **Measured** (test `ITB:`): with noise of 3 kPa in, the energy back never exceeds the energy in, at any moment, at
  openings from shut to 1.2 times the runner; a 100 Hz tone comes back inverted (correlation under -0.9) wide open;
  shut, more than 95 % of the noise's energy comes back (a closed end).
- The plate's area law and size: the reference's (`aThrottle (plate^1.8 + 0.0006)`) for one body; for ITBs each
  plate the runner's own bore when the size is left at 0 (**judgement**: real ITBs are sized so). Idle air through
  a common rail: the reference's bypass shared between the runners.
- Its sound: each trumpet's flow's rate of change, much louder than a box's snorkel (the claim of test 11 is to be
  measured integrated).

## Runners and plenum (`runner.fm`)

- Length and bore as fields (0: the reference's 0.30 m and its bore from the bore). A second length and the speed it
  takes over at (3 % hysteresis): the length glides with an 80 ms time constant (the reference's smoothing of its
  dials), as the waveguide is built to change length smoothly. **Measured** (test `runner of two lengths:`): it
  switches past its speed, holds inside the hysteresis, and never moves more than 1 % of the change a tick.
- The tuning peaks are the waveguide's own; test 12 (integrated) is to hold the torque peak's move to what the
  acoustics predict within 20 %.
- The plenum's volume (0: 0.8 of the displacement, as the reference).

## Not done here, and why

Everything that needs `Intake` (K3b's carve of `pipes()` and `sampleTick`'s intake): the runners' ends with ITBs in
the order of work, the plenum and runner fields, the air box in place of `Extras`' filter, upstream from card F's
last volume, the sources to `Listen`, `tel.intake` (manifold pressure and its ripple). The handoff lists the seams.

## Integrated (intake.model 1 in the engine)

- ITBs (test 11), the default V8 at 4000 rpm wide open: the trumpets in the air radiate +31.2 dB against the single
  throttle's snorkel; the idle's manifold ripple 23.4 kPa against 2.3 (**measured**). What Intake radiates (`sound`)
  is not yet mixed: Extras still rings its own box filter (a change wave 2 or the lead must make).
- The runner's length (test 12, the four at 1500 rpm): the intake port rings at 115 Hz with 0.75 m (its quarter wave
  with the end correction: 113 Hz) and 135 Hz with 0.45 m (185 Hz predicted): the direction is right, the card's 20 %
  is not met (the shorter runner's ring is pulled toward the cycle's strong low orders), and the torque curves'
  peaks are too ragged to read a tuning peak from.
- The single throttle is solved with its plenum (one Newton step on the end pressure), as F asked.
