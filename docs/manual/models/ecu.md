# The ECU

The engine control unit: maps of spark, fuel, boost and cam phase; the idle; the rev limiter; launch control and
flat-shifting; the overrun cut and the pops it lets through; knock control per cylinder; traction control, the pit
limiter, anti-lag and nitrous; and the key (cranking, running, stalling, switching off).

ECU model (`ecu.model`): **Maps** is the model below, used by the shipped archetypes. **Reference** is the original
simulator's ECU (no maps, one knock retard for every cylinder, pops by chance), kept so the two can be compared.

## What is simulated

- **Seven maps**, 16 by 16, against rpm and load: spark advance, lambda, boost target (against gear), intake and
  exhaust cam phase, pedal to throttle plate, and the lift command. Each can follow a **formula** (the default) or
  its **table**. BAKE writes the formula into the table to start from.
- **The idle**: an idle valve with integral and proportional control and a spark trim, a higher idle when cold
  falling as the coolant warms, a dashpot on closing.
- **Limiters**: fuel cut, spark cut (the fuel still goes in and burns in the exhaust), soft (spark retard, then an
  even pattern of cut firings), and throttle (closing the plate just under the limit). Tested within 6342 to 6564 rpm
  for a 6500 rpm limit.
- **Knock control per cylinder**: each knock takes spark back from that cylinder, which is then given back slowly.
- **The overrun**: fuel cut when the pedal is up above idle; an adjustable share of firings still fuelled and burned
  late, so unburned fuel reaches the exhaust.
- **Pops and bangs by account**: unburned fuel and air leaving the cylinders are tracked per bank; when there is
  enough burnable mixture and the pipe is hot, it lights and drives a pressure pulse in the exhaust. A clean overrun
  makes none; a spark-cut limiter makes many.
- **Launch control and flat-shift**: holds a set rpm with retarded spark and cut sparks. The dog box's ignition cut on
  an upshift and the throttle blip on a downshift, as the gearbox asks.
- **Traction control**: retards the spark, then cuts cylinders, to hold the driven wheels near a slip target. A pit
  limiter. **Anti-lag** and **nitrous** switching.
- **The key**: cranking, catching, a richer mixture to start and while cold, stalling under load, running down.

## What is left out

- Closed-loop fuel trims learning over time; sensors failing; emissions control.
- The diesel's smoke limit is computed in the combustion model but not reported (reads 0).

## What the numbers mean

| Setting or reading | Meaning | Typical |
|---|---|---|
| Spark advance | Degrees before top centre the spark fires | 10 to 40 depending on rpm and load |
| Lambda asked / Lambda | The map's target and the sensor's reading | 1.0 part load, 0.85 to 0.90 full load |
| Load | Manifold pressure over outside air (the maps' vertical axis) | 0.2 to 0.3 idle, 1.0 wide open NA, above 1 boosted |
| Limiter (kind), Limiter speed | How and where revs are cut | |
| Overrun fuel | Share of overrun firings still fuelled | 0 clean; 0.1 to 0.4 for pops |
| Retard a knock, Most retard, Recovery | Degrees taken per knock, the most taken, degrees given back per second | 1.5, 15, 2 |
| Idle speed, Cold fast idle | Warm idle (0 uses the layout's) and the extra when cold | 600 to 900 rpm warm |
| Slip target | Traction control's wheel slip | About 0.10 |
| Knocks, Knock retard by cylinder | Since the start; per cylinder now | |
| ECU flags | Limiter, fuel cut, spark cut, overrun, launch, flat-shift, anti-lag, nitrous, traction, pit, knock (TUNE's lamps) | |

## Known gaps today

- **The warm idle lopes high on some builds.** The original simulator's V8 with its race cam idles at about 1200 rpm,
  wandering between 900 and 1500 (the cam's dilution holds it up), where 600 to 900 is usual. The newest work holds the
  default build at 700 rpm within 20, and a V8 with a mild or medium cam at 700 or 835 rpm; hot cams still idle high.
- The bangs' lighting threshold was set when the port gas was read as cooler than it is; it will be checked again now
  that the gas is known to run 1150 to 1330 K.
- Launch control cannot be armed from the window yet (no launch button, and the automatic clutch has no switch).
- Traction control's test on the full car is still running.
- With this ECU and the new combustion model both on, an engine does not start at -40 C.
