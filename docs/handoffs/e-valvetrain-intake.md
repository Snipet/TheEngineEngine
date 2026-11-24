# Card E: valvetrain lab and induction: standalone handoff
Branch `claude/awesome-gates-ntdb58`. K3a has landed and K3b has not, so this covers the standalone half only. The models are in `docs/models/valvetrain.md` and `intake.md`.
**Done, standalone.** `tee-tests-e` runs 14 cases and all pass. It is built by `-DCMAKE_PROJECT_TheEngineEngine_INCLUDE=tests/sim/valvetrain/standalone.cmake`.
1. `cam generator: ...` (mild, hot and race cams): seat duration, 1 mm duration and centreline each within 1 deg, lift within 1 %, no negative lift, acceleration continuous. A free valve is also tested. Asked for the reference's own cam (231/212 deg), the generator returns the reference's constants (p 1.25, k 3.99).
2. `area against lift`: the area is 0.73 of the curtain at low lift and 0.4 of the valve area at d/4. Four valves beat two. At its defaults the lab gives the reference V8's areas within 0.5 %. A small-block head flows 232 CFM at 0.5 in.
3. `phaser:`: a step moves at the set rate, slower at 50 C. It stays parked when cold or without oil pressure, and never leaves its range.
4. `two stages:`, `a cam edited while the engine runs:` and `deactivation:`: every change is made with the valve shut, and no step is larger than the tables' own slopes. A stage change reaches every cylinder within a cycle.
5. `float:`: the float speed rises with the spring and falls with mass and with a sharper cam. A 280 deg cam on a 300 lb/in spring floats at 7000 rpm.
6. `air box:`: predicted 75.5, 53.4 and 43.6 Hz; measured 75.5, 53.4 and 43.5 Hz. The default is 90 Hz, and the box is passive.
Also tested: `ITB:` (passive at every opening), `runner of two lengths:`, and `the cam presets:` (10 builds in `content/builds/cams/`).
**Cost:** the lab adds 16 ns per 96 kHz sample to a V8, with two phasers and two stages on. That is about 0.005 R, against R ≈ 3400 ns from `tee-bench --quick` on this machine. FMinus proves its control path nonblocking.
**Not done:** tests 7 to 13 (integration), waiting for K3b's `Valvetrain`, `Intake` and `Cylinder` modules. Remaining steps:
(a) Append `ValveLabSpec` (without `rev`) to `ValveSpec`, and `IntakeLabSpec` to `IntakeSpec`. Then delete both `lab-spec.fm` files.
(b) `Valvetrain`: model 0 is the reference's `valves()`, bit for bit; model 1 is `ValveLab`.
(c) `Intake`: plenum and runner fields, `RunnerLength`, an `ItbEnd` per runner, and `AirBox` in place of the box filter in `Extras`. Upstream pressure comes from F.
(d) `tel.valve`, `tel.intake`, and the Scope lift arrays (`liftIn` and `liftEx` lookups to add).
(e) Tests 7 to 13 on `EnginePlayer`.
**Seams I need:**
- `Cylinder::step(...)` and `exhaustFlowAt` / `intakeFlowAt` should take the valve areas in m^2 from `Valvetrain::areaEx(c, a)` (main plus auxiliary event) and `areaIn(c, a)`. For model 0 these return `aE * areaAt(a, evo, edur)`. The event angles the cylinder detects come from `evo(c)`, `evc(c)`, `ivo(c)` and `ivc(c)`, all already in `ValveLab`.
- `Combustion` asks `active(c)` at intake closing.
- `control(d16, omega, redline, load, pOil, tOil, float* angles, inTarget, exTarget, liftTarget)`. Oil pressure (gauge Pa) and temperature (K) come from `Heat`. The targets come from H: phaser targets in crank degrees from parked, lift command 0 to 1, and -1 for "use the lab's schedule".
- `plateOpen()` tells `Ecu` / `Intake` to leave the throttle open when lift is the load control.
- `clatter` (0 to 1) drives the valve clicks in `Extras`.
**Wanted outside my paths:** `tests/CMakeLists.txt` should glob `tests/sim/**` recursively.
**FMinus:** no issues.
**To listen to, once integrated:** `hot-street` at idle, `two-stage` through 5500 rpm, `cylinder-deactivation` at 2000 rpm, and an ITB build.
