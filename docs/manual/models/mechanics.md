# Mechanics

The engine's moving parts: how the cylinders are laid out and fire, the piston's motion, the masses that go up and
down, the shaking they cause, friction, the work each cycle does, and the engine dyno.

These are used when Custom engine (`engine.custom`) is on, as it is in every shipped archetype. With it off the
engine is the original simulator's, the **Reference**, which is kept so the two can be compared.

## What is simulated

- **Layout as geometry.** Each cylinder has a bank, a crank pin, a revolution it fires on and a place along the crank.
  The firing angles, the firing order and the intervals follow from that, as on a real crank. Thirty-three presets
  are checked against the published orders (a crossplane V8's 1-8-4-3-6-5-7-2 with each bank 180-90-180-270, a 90
  degree V6 with shared pins, a 45 degree V-twin, a triple). A rotary fires each rotor's three faces.
- **The piston's motion**: the exact slider-crank, so the piston moves faster near the top than near the bottom, as a
  real one does with a rod of finite length.
- **Moving masses.** The piston, pin, rings and a third of the rod go up and down. Because they speed up and slow
  down twice a turn, the crank feels a torque that rises with the square of rpm, and its effective inertia changes
  round the turn. The crank's speed is stepped so that energy is kept exactly.
- **Shaking.** First- and second-order shaking forces and rocking moments, and balance shafts that cancel them. The
  results agree with the textbook: a straight six is balanced, a flat-plane four shakes at twice crank speed, a boxer
  four has only a small rocking moment.
- **Friction** by part (rings and skirts, bearings, valvetrain, accessories), in the standard form that grows with
  peak pressure, piston speed and its square (Chen and Flynn). The alternator and the air conditioner take their load.
- **Work and efficiency figures**, measured from the pressure in each cylinder: IMEP, pumping (PMEP), friction (FMEP),
  BMEP, volumetric efficiency, peak pressure.
- **Heat to the cylinder walls.** Wall heat (`engine.wallHeat`): Reference (one coefficient by speed) or Woschni (the
  standard correlation, worked out for each cylinder from its own pressure and temperature).
- **The engine dyno**: an absorber that holds a set rpm or sweeps at a rate, letting the firing ripple through as a
  real dyno does. It holds its speed within about 10 rpm at full throttle.

## What is left out

- The counterweights are assumed to balance the rotating masses; shaking above the second order.
- Shaking is computed and reported but not yet heard.
- The dyno's own inertia and limits.
- The friction coefficients are fitted to the original simulator's friction curve, not measured on a real engine.

## What the numbers mean

| Reading or setting | Meaning | Typical |
|---|---|---|
| Bore, Stroke, Rod length | The cylinder's size and the rod's centre-to-centre length | Rod/stroke 1.5 to 2.0 |
| Compression | Volume at the bottom over volume at the top | 9 to 13 petrol, 15 to 18 diesel |
| Mean piston speed | Twice the stroke times revolutions per second | 20 to 26 m/s at a road petrol engine's redline |
| Redline (`engine.redlineSpeed`) | Left at 0, set from a piston speed of 25 m/s (13.5 for a diesel) | |
| Piston, pin and rings; Rod | The moving masses; left at 0, estimated from the bore | 0.45 kg at 86 mm bore |
| Crank and flywheel | The rotating inertia: lighter revs faster and stalls more easily | |
| Torque, Power | At the crank, after friction | |
| IMEP, PMEP, FMEP, BMEP | Pressures that compare engines of any size: the gas's work, the pumping loss, friction, and what is left at the crank | BMEP at the torque peak: 10 to 13 bar NA petrol, 18 to 25 bar turbo |
| Volumetric efficiency | Air trapped against the cylinder's swept volume of outside air | 0.85 to 0.95 NA; over 1 with a tuned intake or boost |
| Inertia torque, Shaking force, Rocking moment | From the moving masses, at the speed now | |

## Known gaps today

- **Engines make less power than they should at high piston speed.** A 998 cc superbike four makes 59 kW at 13,500
  rpm where 125 to 145 kW is expected, and a 6.2 L V8 257 kW where 300 to 330 kW is expected. The torque per unit of
  air falls with rpm: friction at high piston speed and the burn's fixed length in degrees take most of it. Being
  worked on.
- **Exhaust gas at the ports runs hot**: 1150 to 1330 K at full load where 900 to 1100 K is usual (part of the reason
  is that a thermocouple reads downstream of the port; part is the heat balance, still being checked).
- Woschni wall heat is meant to be switched on together with the new combustion model's full burn; that pairing
  is still being settled.
- Switching a model on or off while the engine runs is not yet faded: expect a click.
