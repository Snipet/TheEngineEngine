# The valvetrain lab (card E)

*`sim/valvetrain/`. `valve.model = 0` is the reference's cams (one dial for both cams' duration and their
separation, one tabulated curve, areas as fixed shares of the piston's); `valve.model = 1` is this lab, its fields
the ones after `cam` in `sim/spec/valve.fm`, wired through `Valvetrain` (`sim/valvetrain/valvetrain.fm`).*

Every number below says where it comes from: **measured** (a test measures it), **published** (a source), or
**judgement** (chosen; a person with better data should change it).

## What the cylinder sees

One effective area (m^2) for its intake and one for its exhaust each sample, each a table lookup:
`ValveLab::areaIn(c, a)`, `areaEx(c, a)` (main event plus the auxiliary one). The tables are 129 points over each
event, as the reference's `kCamArea`, and are rebuilt at the 16-sample rate, one table a tick at most (a queue). A
phaser moves only where a table starts (`AreaTable::place`). Cost: some 13 ns a sample more than the reference's
areas for a V8 with two phasers and two stages (**measured**, `cost:` test, this container; 0.004 R against R of
some 3400 ns).

## The cam generator (`cam.fm`)

A cam is asked as a cam card gives it: duration at the seat, at 1 mm, lift, centreline, fatness.

    lift(u) = L sin(pi u)^p (1 - exp(-k^2 sin^2(pi u))) / (1 - exp(-k^2)),  u = s / span, 0 to 1

- The shape is the reference's (`cylinder.fm`, `lift`: `sin^1.25` with a ramp `1 - exp(-e^2)`, `e = 4 min(ph, pi -
  ph)`), with `sin(ph)` in place of the distance to the nearer end so the curve is smooth everywhere: near the seat
  it rises as `u^(p+2)`, so velocity and acceleration are 0 there and the acceleration is continuous (**measured**:
  test `cam generator: what was asked ...`, the acceleration's change halves with the step).
- `p = 2 - 1.5 fat` (**judgement**: fat 0.5 is the reference's 1.25).
- "At the seat" is at 0.006 in, 0.15 mm (**published**: SAE J604's checking height for advertised duration).
- `k` is solved (bisection on log k) so the 1 mm duration over the seat's is what was asked; then the span (the
  degrees with any lift) so the seat duration is. The ramp can only move the effective power between `p` and
  `p + 2`, so for `p = 1.25` and 10 mm of lift the 1 mm duration can be 82 % to 92 % of the seat's; outside that the
  nearest is made and `reached` is 0. Real cams' 0.050 in durations sit 35 to 50 degrees inside their advertised
  ones (**published**, cam makers' cards), inside that span.
- The reference's own V8 cam (dial 0.35: a span of 250.6 degrees) measures 231 at the seat and 212 at 1 mm for 10 mm
  of lift; asked for those, the generator makes `p = 1.25, k = 3.99`, span 250.5: the reference's constants back
  (**measured**). Those are `spec.valve`'s defaults.
- Measured back from the generated curve (0.01 degree steps), for a mild, a hot and a race cam: seat and 1 mm
  durations within 1 degree, lift within 1 %, centreline within 1 degree; no negative lift (**measured**).
- A free valve: open and close given outright, flanks of `freeFlank` degrees (smootherstep, acceleration continuous),
  a dwell at full lift between. Late and early intake closing are presets (`content/builds/cams/`).
- Left out: asymmetric lobes, lash.

## From lift to area (`area.fm`)

    A(L) = n q 0.4 (pi d^2 / 4) tanh(6.8 L / d) / tanh(1.7)

- The form is the reference's `flowArea` (`tanh(1.7 l)`, `l` the lift over a quarter of the diameter), given the
  geometry: 0.73 of the curtain `pi d L` low down, 0.63 at a tenth of the diameter (the reference's comment: "about
  0.6"), 0.4 of the valves' area at a quarter, flat beyond (**measured**, test `area against lift`).
- Diameters left at 0, from the bore (heads' published sizes as recalled, not checked here; **judgement** for the
  rounding): two valves,
  intake 0.48 and exhaust 0.39 of the bore (a small-block Chevrolet's 1.94 and 1.50 in on a 4.00 in bore: 0.485 and
  0.375; an LS's 2.00 and 1.55 on 3.90: 0.51, 0.40); four valves 0.39 and 0.33 (Honda K20 35 and 30 mm on 86: 0.41,
  0.35; Toyota 2JZ 33 and 29 on 86: 0.38, 0.34); three (two intakes) 0.37 and 0.44, five (three intakes) 0.32 and
  0.33 (**judgement**: few heads to go by).
- Port quality `q` 1 is the reference's coefficients (Cd 0.4 at a quarter of the diameter). The defaults, 1.35 for
  the intake and 1.5 for the exhaust, give the reference V8's peak areas (12 % and 9 % of the piston's) with the
  default valves and 10 mm of lift, within 0.5 % (**measured**, test `area against lift`), so the lab at its
  defaults breathes as the reference does. On the flow bench (below) a small-block's two-valve head comes out at
  172 CFM at 0.500 in at `q` 1 and 232 at 1.35 (**measured**); real stock small-block heads flow some 190 to 230 and
  good aftermarket ones 250 to 300 (flow-bench figures as recalled, not checked against a source here).

## Phasers (`phaser.fm`)

- Phase is crank degrees from the parked stop: the intake advances, the exhaust retards (**published**: most vane
  phasers' arrangement).
- Slew: `rate` (200 crank degrees a second warm, at full pressure: **judgement**) times the oil pressure's share between 70 and 200 kPa (**judgement**)
  times 0.25 to 1 as the oil warms from 40 to 80 C (**judgement**). Parked below 40 C or 70 kPa: back to 0 at half
  its rate (the cam's torque and the lock pin).
- Clamped to its range at every tick (**measured**: test `phaser:`, a step of 40 degrees reaches 20 at 0.1 s and 40
  at 0.2 s warm; at 50 C less than 0.6 of that; parked colder or without pressure; never outside).
- An engine whose ECU gives no target: `Phaser::schedule` (intake advanced at load from 12 % to 50 % of the
  redline, none at the top or idle; exhaust retarded at part load in the middle): **judgement**, a placeholder for
  card H's maps.

## Two stages

The stage wants B above `stageSpeed`, A again below it less `stageHyst`, only with oil above 100 kPa and above 60 C
(**judgement**). Each cylinder takes the new profile, intake and exhaust apart, at the first tick where its old and
its new table both give no area at its angle, so the change needs no crossfade and goes round the engine in a cycle
(**measured**: test `two stages:`, every sample's step of area within what the tables' own slopes allow, with a
phaser moving meanwhile; the last cylinder changed within 720 degrees and a tick of the want).

## Continuous lift

The intake only (**published**: BMW Valvetronic, Toyota Valvematic, Nissan VVEL are intake systems). Five tables
prebuilt from `liftMin` to full lift, each the cam's own shape with its lift times `s` and its span times
`0.4 + 0.6 s` (**judgement**), centreline kept (the phaser moves it); the table in use is a blend of the two either
side, made on the tick the command moves. With `liftThrottle` the command is the load (the pedal); the plate is then
to be left open (a seam to `Intake`, below).

## Deactivation

A cylinder in `deacMask` rests while the speed is in `[deacLow, deacHigh]`, the load under `deacLoad` and the oil
warm (leaves at 5 % outside the window, or 0.05 over the load). It is taken out, and put back, in its power stroke:
both valves shut, its exhaust next. Its exhaust then stays shut (the burned gas is its spring), its intake too, and
`active(c)` is false, so `Combustion` gives it no fuel (**published**: GM's Active Fuel Management traps exhaust gas
this way). **Measured**: test `deactivation:`, the chosen four rest for a whole cycle with no valve of theirs
opening, every change made with the valve shut, all eight back under load.

## Valve float (`float.fm`)

    w_float = min over the nose of sqrt((F0 + k L(th)) / (m |d2L/dth2|))

- The effective mass: the valve's own (left at 0: 0.10 kg at 49 mm as the diameter's 2.5th power, **judgement**
  from steel valves' masses) plus the train's by kind: pushrod 0.20 kg, rocker 0.08, bucket 0.06, finger 0.045
  (**judgement**, from published valvetrain masses). Pneumatic never floats.
- A small-block's street cam (280 at the seat, 0.48 in lift) on 300 lb/in over 120 lb seat pressure with a 0.12 kg
  valve, pushrod: 7000 rpm (**measured**). Cam makers recommend springs of that order for cams of that size run to
  some 6000 to 6800 rpm: the right neighbourhood, but recalled, not checked against a source here.
- Rises with the spring and preload, falls with the mass and with a sharper cam (**measured**, test `float:`).
- Past it (`s = (w / w_float)^2 - 1 > 0`): a tail of area after the event, bounces of 0.15 s of the peak area (at
  most 0.3) over 10 + 30 s degrees (at most 40), and `clatter` for the sound (**judgement**, all three). No damage.
  The power falling over must come out of the gas model (test 13, integrated).

## The flow bench (`bench.fm`)

`Q = A(L) sqrt(2 dp / rho)`, dp 28 in of water (6.97 kPa), rho 1.204 kg/m^3; lifts 0.050 to 0.800 in, in m^3/s
(CFM is times 2118.88). Computed when the Spec changes.

## Not done here, and why

- Wiring into the cylinder, the ECU's targets, the scope's lift arrays, telemetry: wait for K3b (the handoff lists
  the seam).
- The lab is for piston engines; a rotary keeps the reference's ports.

## Integrated (valve.model 1 in the engine; tests/sim/valvetrain/IntegratedTest.cpp, engine dyno, wide open)

- Phasing (test 8), a 280-degree cam, intake centreline 95 against 125: the default V8 +13.8 % at 1500 rpm, -0.9 % at
  6000, peaks 3877 and 4113 rpm; the four +6.1 % and -8.3 %, peaks 2724 and 3335 rpm (**measured**).
- Two stages (test 9) on the four: the curves cross at 5500 rpm; through the change the gas torque does not dip, and
  the output's level steps 2.8 dB there against 9.8 dB at most between two firings elsewhere in the sweep.
- Deactivation (test 10), the V8 at 2000 rpm and 90 N m: the manifold 45.9 kPa on eight, 69.7 on four; the
  half-firing line from -17.1 to +6.9 dB against the firing line.
- Float (test 13): the bounce's size and length (judgement) were raised so the power falls past the float speed: on the
  four, 0 % at 0.85 times it, -0.8 % at 1.1, -4.5 % at 1.25; finite at twice it. A real valvetrain falls harder.
- Cost: `tee-bench --engine`, free-revs and drive-car: the simulator over the reference 1.04 at the defaults, 1.07 to
  1.11 with the lab (phasers, two stages, springs, a two-length runner): some +0.03 to +0.07 R, on a shared machine.
