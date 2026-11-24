# D: mechanics, layouts (standalone part done; integration waits for K3b)

Package D, branch `claude/awesome-gates-ntdb58`, commits 20e0785, 72d2d53, 8f228c4, 0362088, ac89906. Cloud session.
Model, sources, judgement: [docs/models/mechanics.md](../models/mechanics.md).

**Done** (new files only; nothing K3 creates was touched): `sim/core/{mechspec,layout,kinematics,inertia,shake,
friction,work,dyno}.fm`, 33 presets in `content/builds/engines/` (the reference's 17 plus the card's 16), and
`tee-tests-core` (`tests/sim/core/`, 16 tests, all pass):
- test 2: `the slider-crank's volume, slope and acceleration match the closed form...` (lambda 2.8 to 4.5, < 1e-6 r);
  `the size and bore/stroke dials and the bore and stroke are one setting` (+ redline by piston speed)
- test 3: `firing intervals from geometry: the card's table`, `every layout preset fires as its file says`,
  `the reference's seventeen layouts' firing angles, exactly`, `a layout whose cylinders fire together is flagged`
- test 4: `the inertia torque does no work over a cycle...`, `one cylinder's inertia torque is the closed form's, and
  goes as the speed squared`, soak `a crank with its moving masses turning freely keeps its energy (20 s)`
- test 5: `shaking by order: the known engines`, `balance shafts cancel the order they claim, and only that`
- test 6 (standalone half): `the dyno holds a set speed within 10 rpm...` (mean within 0.01 rpm; absorber = brake
  torque to 0.1 %), `the dyno sweeps at its rate`
- also: Otto-cycle work integral vs closed form (0.2 %), friction by part (0.95 to 2.35 bar, 700 to 6500 rpm)

**Not done**: tests 1, 6 (in the engine), 7, 8, 9, the integrator's job, Spec fields into `sim/spec/engine.fm`,
`tel.engine` fields, `sim/README.md`: all wait for K3b, as the card says.

**Measurements**: R = 1942 ns (tee-bench --quick, reference V8, this container). Inertia torque + crank step + work
integral: 69 ns a sample for 8 cylinders (0.036 R; budget 0.06 R), 117 ns for 16. `tee-tests-core` runs in 0.6 s.
Whole suite (115 tests, mine included) passes at merge 67b5877.

**Finding for K3/the Crank**: with a varying inertia J(theta), an explicit step of dw/dt loses energy (a free single
-15 % in 20 s). `MovingMasses::advance` steps the kinetic energy exactly instead (2e-5 in 20 s). With `custom` on the
crank must use it; with it off the reference's step stays.

**Change wanted outside my paths**: `tests/CMakeLists.txt` needs `add_subdirectory(sim/core)` (or a loop over
`tests/sim/*/CMakeLists.txt`); until then `tee-tests-core` is not built by the presets (I add the line locally,
uncommitted). Cards E, H, I each improvised their own hook (`standalone.cmake`, `hook.cmake`): one rule for all would
help.

**FMinus issues**: none met.

**Judgement, not measurement**: friction coefficients (fitted to the reference's curve), default piston/rod masses,
the V4/W pin phasing, places along the crank, 25 m/s piston-speed redline, dyno loop 15 rad/s. Presets' `"expect"`
key is outside the spec (02-contracts §8 keeps unknown keys).

**To look at**: nothing audible yet (standalone). `python3 tests/sim/core/make_presets.py` prints every preset's order.
