# Card H: combustion, fuels and the ECU

*Wave 1. Repository: TheEngineEngine, integration branch `claude/awesome-gates-ntdb58`. Read
[01-rules.md](01-rules.md), [02-contracts.md](02-contracts.md), `sim/README.md` (K3's: `Combustion`, `Ecu`), then
the reference: `charge()` and `chargeDiesel()` in `sim/ref/engine.fm` with every comment (the dilution's
recursion, the knock rule, Wolfer's delay, Watson's share), `control16()` (the driver, the idle valve, the spark's
trim, the limiter, the overrun cut, the state machine), `Pop`, and the burn in `sim/ref/cylinder.fm`'s `step`.*

**OWNS**: `sim/combustion/**`, `sim/ecu/**`, `sim/spec/fuel.fm`, `sim/spec/ecu.fm`, the `tel.ecu` section,
`tests/sim/combustion/**`, `tests/sim/ecu/**`, `content/builds/tunes/**`, `docs/models/combustion.md`,
`docs/models/ecu.md`.

**Budget**: 0.06 R a sample (the knock integral is the only per-sample work; everything else is per cycle or per
16 samples).

**Start standalone**: the fuel table, the maps' interpolation, the knock integral on a made-up pressure trace,
the limiter and idle logic against a one-line engine model.

## What the reference does

Gasoline at a fixed mixture or a diesel; the heat from what of the charge can burn; a spark from a formula of
speed and load; knock when the compression passes a limit that moves with boost, timing and heat; a hard fuel-cut
limiter; an overrun cut with pops by chance. All of it stays as `fuel.model = 0` and `ecu.model = 0`, the
defaults.

## Combustion and fuels

**A fuel is a row of a `const` table**: stoichiometric air-fuel ratio, lower heating value, latent heat, an
octane-like number for the knock rule, a flame-speed factor, lean and rich limits. Rows: pump gasoline in three
grades, race gasoline, E85, methanol, diesel; propane and hydrogen as rows marked untested. Give each number's
source in `docs/models/combustion.md`.

**Mixture.** The ECU asks a lambda. Fuel is the air's oxygen over lambda and the stoichiometric ratio (the
oxygen share is card F's when nitrous flows, else air's); the heat is what the scarcer of fuel and oxygen
allows; what is left unburned leaves with the exhaust (its mass is what overrun and limiter bangs are made of,
below). Lambda moves the burn's length (fastest a little rich), the heat's completeness near the limits (past
them, misfires by chance), the charge's cooling as the fuel evaporates (port or direct injection as how much of
it happens in the cylinder) and so the trapped mass and the knock margin.

**Knock from the end gas.** From intake closing to the burn's end, integrate `dt / tau(p, Tu)` (Livengood and
Wu), `tau` by Douaud and Eyzat's fit with the fuel's octane number, `Tu` the unburned gas's temperature squeezed
isentropically from its state at intake closing by the cylinder's own pressure. The integral reaching 1 before
the flame has finished is a knock, as hard as the unburned share then; the reference's consequences follow (the
ping's event, the heat it scrubs, the ECU's retard). Evaluate it every few samples with tables for its power and
exponential, not per sample with `expf`. Reaching 1 before the spark is pre-ignition, as the reference handles.

**The diesel** stays the reference's, with fields for what it fixes: injection timing, a pilot injection (it
lowers the premixed share, so the clatter), rail pressure (the diffusion burn's rate), and the smoke limit with
an over-fuel allowance (a smoke number in telemetry).

**What a tuner reads**: the angle of half the burn (it should sit near 8 degrees after the top at best torque),
peak pressure and its angle, exhaust gas temperature, lambda as a sensor sees it (delayed), injector duty from an
injector size, fuel flow.

## The ECU

**Maps** (`Map16`, axes speed and load; load is manifold pressure over the air's, or the pedal for throttles with
no plenum): spark advance, lambda, boost target (speed by gear), each phaser's angle (card E reads them), the
pedal's map to the plate, the lift command. Each map has `auto` (default on): the map's output is then the
reference's formula, so off-is-reference holds and no build ever lacks a tune. **Baking**: on a request
(`ctl` counter, the lead adds it) the formulas' values at the map's breakpoints are written to a Scope array,
which the host copies into the build as an editable map: the starting point a tuner edits.

**Loops**: knock control (retard a knock, creep back, per cylinder; can be switched off, at the user's risk);
boost control (card F's gate or vanes toward the target; agree with F where the loop lives); idle (speed target
warm and cold, by air and spark, the reference's).

**Limiters** (`limiter.kind`): fuel cut (the reference's); spark cut (the charge goes out unburned and lights in
the pipe); soft (retard, then cut cylinders progressively over the last few hundred rpm); throttle (drive-by-wire
closes). The speed is a field (0: the layout's).

**Launch**: with the launch button held, or the clutch down at a standstill and the pedal floored, hold a set
speed by cutting and retarding spark (which also heats the turbine); let go as the clutch comes up. Flat-shift:
the same cut through a clutch upshift with the pedal down. A dog box's cut and a downshift's blip are requests
from card I that you carry out.

**Overrun**: when the cut comes in and goes out (the reference's); `overrun.fuel` (how much is injected anyway)
and `overrun.retard`. **Bangs by account, not by chance**: unburned fuel that reaches the exhaust adds to a store
for its bank; it lights when the gas there is hot enough and there is oxygen, more readily the more there is; the
pulse card G's node receives (the reference's `Pop`) is as large as the fuel that burned. The reference's dice
stay behind `ecu.model = 0`.

**Anti-lag and nitrous**: the switching (windows by speed, pedal, gear), the retard and the extra fuel; card F
burns it and flows it.

**Traction control**: a slip target; torque taken out by retard and cylinder cuts while the driven wheels (card I
gives their slip) exceed it. A pit limiter by the same means.

**State**: the ignition key (K3's mapping, now yours), cranking fuel, the cold start's enrichment and fast idle,
the reference's stall and run-down. The driver for `ctl.holdSpeed` (the reference's key governor) is yours.

## Telemetry

Spark advance asked and delivered, knock by cylinder and the retard in force, lambda asked and seen, injector
duty, fuel flow, exhaust temperature, the half-burn angle, peak pressure and its angle; flags for the limiter,
the cuts, launch, anti-lag, traction control; the idle valve; the ECU's state.

## Tests

Standalone:
1. Energy released a kilogram of air by fuel at stoichiometric (near 2.9 to 3.1 MJ, each against its row).
2. The knock integral on made-up traces: more boost, more advance, hotter air and lower octane each bring it
   sooner; its value at a known operating point within the literature's range (state the case).
3. `Map16` reads its breakpoints exactly and interpolates between; a baked map reproduces the formula at every
   breakpoint.
4. Each limiter holds its speed within a stated band on a one-line engine; launch holds its speed; idle holds
   within 30 rpm warm and comes down from its cold fast idle.

Integrated:
5. `model = 0`: the null test.
6. A spark sweep at a fixed speed and load: torque against advance has one maximum, and the half-burn angle there
   is between 6 and 12 degrees after the top.
7. A lambda sweep: torque peaks between 0.85 and 0.92; exhaust temperature near 1.0 to 1.05.
8. E85 takes more advance than regular before it knocks; methanol more boost.
9. A spark-cut limiter and a rich overrun make bangs whose pulse sizes follow the fuel sent; a full cut makes
   none.
10. Traction control holds a spinning car's slip near its target (with card I's car, when it has landed).
11. Ten minutes of random pedals through every limiter kind: finite, bounded, the engine never runs away.

## Not in this card

Valve events and phaser mechanics (card E). The turbo and what burns in its housing (card F). The pipes that
carry a bang (card G). Gearbox logic (card I). Carburettors and wall-wetting transients (later).
