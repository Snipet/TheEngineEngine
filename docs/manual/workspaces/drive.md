# DRIVE

The car being driven: round gauges, the car on its road, each wheel's slip, and the drag strip's tree and time slip.
Put the cockpit's mode on **drive** and use the keys (below). There is no dyno ribbon here.

## What you see

**Instruments** (top):

| Instrument | What it shows |
|---|---|
| Tachometer | Engine speed, numerals in thousands of rpm (X1000 RPM), with the number in digits |
| Speedometer | Road speed in your units, in digits too |
| BOOST | Boost over the outside air, or VACUUM when the manifold is below it |
| LAMBDA | The mixture as the ECU's sensor reads it; LAMBDA LEAN as a warning |
| COOLANT, OIL, OIL PRESSURE | Coolant and oil temperatures (COOLANT HOT as a warning) and oil pressure |
| GEAR | The gear engaged (N for neutral) |
| THROTTLE, BRAKE, CLUTCH | The three pedals as bars, whatever is pressing them (keys, the mouse on the cockpit's bars, a gamepad) |

**The car** (bottom left): the car from the side on its road. It squats as it pulls and dives as it brakes, its
wheels turn at their own speeds, and the road's marks pass under it. Under it, **WHEEL SLIP** for FL, FR, RL and RR as
bars, with SPIN when a wheel spins. A motorcycle shows WHEELIE, STOPPIE and WHEELIE CONTROL.

**The strip** (bottom right): the tree (STAGED, three AMBER, GREEN, RED, each lamp with its word) and the **TIME
SLIP**: REACTION, 60 FT, 330 FT, 1/8 MILE, 1/8 SPEED, 1000 FT, 1/4 MILE, TRAP SPEED. "NO STRIP YET" until a run has
been staged.

Typical values: a family car's quarter mile is 15 to 17 s; a strong sports car's 12 to 13 s. Wheel slip of a tyre
pulling hard on dry asphalt is about 5 to 15 %; much more is wheelspin.

## What you can change

- **The road** (click the scene): Air temperature (`env.temperature`), Air pressure, Surface (Dry, Damp, Wet, Snow,
  Prepared strip), Surface, left side (for a split road), Grade.
- **The car or the tree** (click it): the car's settings, as on [CAR](car.md).

**Driving keys** (they work in every workspace):

| Key | Does |
|---|---|
| Return | Start: the starter cranks until the engine runs. Hold for a second to switch off. |
| Up or W | Throttle (eases down in 0.3 s, back in 0.18 s) |
| Down or S | Brake |
| E or Page Up | Shift up (from neutral into first) |
| Q or Page Down | Shift down |
| N, or 1 to 6 | Neutral, or straight to that gear |
| Left Shift | Clutch (needed only with the automatic clutch off) |

The automatic clutch is on: E and Q work like a paddle-shift car's paddles, the car pulls away by itself when you
press the throttle in first, and it does not stall. A gamepad works too (right trigger throttle, left trigger brake,
bumpers to shift). See [Controls](../controls.md).

## Things to try

1. **Wheelspin.** Load "Supercharged 6.2 L V8", shift into first and floor it. Watch the rear slip bars show SPIN,
   the car squat and the rpm flare. Try second gear, then lift a little to find where the tyres grip. Listen for the
   tyres.
2. **A wet road.** Set Surface to Wet and repeat. The same throttle spins the wheels sooner; on CAR the grip curve
   shows the lower peak.
3. **A hill.** Set Grade to 10 % and pull away in first, then in third. The engine labours in the tall gear;
   with the automatic clutch on it does not stall.
4. **A hot day on a turbo.** On "3.0 L straight six, twin turbo", set Air temperature to 35 C and compare BOOST,
   LAMBDA and acceleration with a 15 C day. Then lower Air pressure from 101 kPa (sea level) to about 80 kPa (some 2000 m up)
   and see how much boost the turbos still make.
5. **Shift by number.** At 50 km/h in fourth, press 2: the gearbox shifts down one gear at a time and refuses a
   downshift that would take the engine past its redline. Listen to each downshift.

## What is not there yet

- Nothing in the window stages a drag run yet, so the tree and the time slip stay empty ("NO STRIP YET").
- The automatic clutch has no switch in the window, so the clutch key (Left Shift) and an H-pattern lever cannot be
  used yet.
- Steering is not read (there is no cornering in the model), a wheel and pedals are not read yet, and the gamepad
  code has not been tried with a pad in hand.
- The Space key (handbrake) does nothing yet.
