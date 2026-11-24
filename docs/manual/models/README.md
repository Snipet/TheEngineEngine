# How each part is modelled

The sound and the numbers come from a physical model, one part of the machine at a time. Each page says what is
simulated, what is left out, what the main readings and settings mean with typical real-world values, and what is
known to be wrong today.

Most parts have a model switch. **Reference** is the original simulator's behaviour, kept so the two can be
compared; the other choice is the new model. The shipped archetype builds have every new model switched on.

| Page | What it covers | Switch |
|---|---|---|
| [Mechanics](mechanics.md) | Layout and firing, piston motion, moving masses, shaking, friction, work figures, the engine dyno | Custom engine; Wall heat |
| [Valvetrain](valvetrain.md) | Cams, lift to flow area, flow bench, phasers, two stages, continuous lift, deactivation, valve float | Valvetrain model |
| [Intake](intake.md) | Air box, throttles, plenum, runners and their tuning | Induction model |
| [Forced induction](boost.md) | Compressor maps, turbines, shafts, intercooler, wastegate, blow-off valve, superchargers, anti-lag, nitrous | Model (boost) |
| [Exhaust](exhaust.md) | Pipes as wave guides, headers, collectors, crossovers, resonators, mufflers, bypass valves | Exhaust model |
| [Combustion and fuels](combustion.md) | Fuels, mixture, heat release, burn speed, charge cooling, knock, diesel injection | Combustion model |
| [The ECU](ecu.md) | Maps, idle, limiters, knock control, overrun and pops, launch, traction, anti-lag, nitrous, the key | ECU model |
| [The vehicle](vehicle.md) | Clutch, gearboxes, differentials, shafts, tyres, load transfer, aero, brakes, chassis dyno, drag strip | Vehicle model |
| [Mechanical voices](voices.md) | Gear whine and rattle, belts, fans, fuel pump, injectors, lifters, wastegate, starter | Mechanical voices |
| [Two-stroke](twostroke.md) | Ports, crankcase pumping, scavenging, expansion chambers, power valve | Cycle |

Problems that cross several parts, in short:

- Engines make less power than they should at high piston speed ([Mechanics](mechanics.md)).
- Exhaust port gas runs 1150 to 1330 K at full load where 900 to 1100 K is usual ([Combustion](combustion.md)).
- The exhaust builder's settings are kept but not yet run by the engine ([Exhaust](exhaust.md)).
- The warm idle lopes high on builds with hot cams ([The ECU](ecu.md)).
- The compression brake has no switch yet ([Valvetrain](valvetrain.md)).

See also the [workspaces](../workspaces/README.md) and the [glossary](../glossary.md).
