# Intake

The induction from the air outside to the intake valves: the air box and its snorkel, the throttle (or one throttle
per cylinder), the plenum and the runners.

Induction model (`intake.model`): **Lab** is the model below. **Reference** is the original simulator's intake (one
throttle, a plenum of 0.8 times the displacement, 0.30 m runners, the air box as a fixed resonant filter), kept so
the two can be compared.

## What is simulated

- **The air box as a Helmholtz resonator**: the box's air is the spring, the snorkel's air the mass. A small box and a
  thin snorkel resonate lower and can starve a big engine of air. Its resonance is measured within 1 % of the formula.
  The snorkel's mouth radiates the intake's sound.
- **Runners as pipes** in which pressure waves travel. A runner rings at a frequency set by its length and the
  cylinder's volume; when that ring meets the intake event, the cylinder fills better (the "ram" or tuning effect).
  Each length gives a main bump in volumetric efficiency and a lower one. Measured on a 2.0 L four: volumetric
  efficiency reaches 1.14 at the 0.60 m runner's tuned rpm; the bumps move with length as the acoustics predict
  within a few percent.
- **A two-length manifold**: a second length taking over above a set rpm, gliding over 80 ms.
- **Individual throttle bodies**: each runner's open end with its own plate, each trumpet radiating to the air. With
  trumpets in the open the intake is 31 dB louder than a single throttle's snorkel, and the manifold pressure pulses
  far more at idle (23 kPa against 2).
- **The plenum** as a volume, its pressure and its pulsing ("ripple") reported.
- Upstream, a turbo or supercharger's outlet when the build has one (see [Forced induction](boost.md)).

## What is left out

- The air box's own acoustic modes (a 10 L box's start at some 500 Hz).
- A paper filter whose resistance grows with flow (one fixed loss is used).
- Individual throttles share the original simulator's idle bypass; no idle control was tuned for them.

## What the numbers mean

| Setting or reading | Meaning | Typical |
|---|---|---|
| Runner length (`intake.runnerLength`) | Longer tunes lower in the rev range; 0 uses 0.30 m | 0.2 to 0.5 m on road engines |
| Runner bore | 0 sizes it from the cylinder bore | |
| Second runner length, Second length from | A variable manifold's other length and the rpm it switches at | |
| Plenum volume | 0 uses 0.8 times the displacement | 0.8 to 1.5 times displacement |
| Air box volume, Snorkel length and bore | Set the box's resonance and how freely it breathes | Car air boxes 5 to 15 L |
| Throttle | One body and a plenum, or a body a runner | |
| Manifold (reading) | Absolute pressure after the throttle | About 30 kPa at idle, near 100 wide open |
| Manifold ripple | Peak-to-peak pulsing over a cycle | Small with a plenum, large with ITBs |
| Air box (reading) | The box's pressure against the air outside: below 0 means the box restricts | |
| RAM NEAR (on AIR) | The rpm of the runner's main and lower tuning bumps, worked out from length and cylinder | |

## Known gaps today

- The breathing is now in the right range (volumetric efficiency over 1 at a tuned speed), but engines still make
  less power than they should at high rpm: see [Mechanics](mechanics.md).
- Changing the Induction model while running is applied on the next tick, with a click.
