# Combustion and fuels

What happens to the charge once the intake valve shuts: how much fuel goes in for the air, how much heat it releases,
how fast it burns, how the end gas knocks, and the diesel's injection.

Combustion model (`fuel.model`): **Fuels** is the model below. **Reference** is the original simulator's charge (one
petrol and one diesel, knock by a simple rule), kept so the two can be compared.

## What is simulated

- **A table of fuels**: regular, midgrade and premium E10 pump petrol, race petrol, E85, methanol and diesel (propane
  and hydrogen are listed but untested). Each has its stoichiometric air-fuel ratio, heating value, heat of
  evaporation, octane numbers and flame speed, from standard references.
- **The mixture**: the fuel for the trapped air at the lambda the ECU asks; the heat released, which peaks about 10 %
  rich, as in real engines; unburned fuel left over when rich (it can burn later in the exhaust); misfires near the
  lean and rich limits.
- **Burn speed** follows the fuel's flame speed at that mixture: alcohols burn faster, a lean or very rich mixture
  slower.
- **Charge cooling** by the fuel's evaporation: more with direct injection, much more with alcohols.
- **Knock**: the unburned end gas's chemistry is tracked through each cycle from its pressure and temperature
  (Livengood and Wu's integral over Douaud and Eyzat's delay). If it reaches ignition before the flame finishes, the
  rest of the charge lights at once: a knock, whose size is the share left unburned. Before the spark, it is
  pre-ignition. More boost, more advance, hotter intake air, more compression and lower octane each bring it sooner.
- **The diesel's injection**: start of injection, a pilot share, rail pressure and over-fuelling (with a smoke
  number), over the original simulator's diesel burn.
- **What a tuner reads**: peak pressure and its angle, the half-burn angle, a wide-band lambda sensor with its delay,
  injector duty, fuel flow.

## What is left out

- Two-stage (cool-flame) ignition, hot spots, where in the chamber the knock starts.
- Exhaust gas left in the cylinder affecting the knock delay.
- Emissions.

## What the numbers mean

| Setting or reading | Meaning | Typical |
|---|---|---|
| Fuel (`fuel.grade`) | The fuel's row | |
| Direct injection | Fuel sprayed into the cylinder: more charge cooling | |
| Knock delay (`fuel.knockDelay`) | Scales the end gas's ignition delay; the calibration | Default 3 |
| Injector size | Flow of one injector; sets the duty | |
| Lambda | Actual air-fuel ratio over the stoichiometric one | 1.0 part load; 0.85 to 0.90 full load petrol; diesel 1.3 to 3 |
| Knocks, knock retard | Count of knocks, and spark taken back by the ECU | 0 is healthy |
| Half burned | Crank angle where half the charge has burned | 8 to 10 degrees after top centre at best torque in real engines |
| Peak pressure, Peak at | Highest cylinder pressure and its angle | 50 to 80 bar NA petrol, 12 to 16 degrees after top centre |
| Injector duty | Share of the cycle the injector is open | Under 0.85 |

Stoichiometric ratios: petrol about 14.6 (E10 14.0), E85 9.8, methanol 6.5, diesel 14.5. Octane (research / motor):
regular E10 91/83, premium 96/86, E85 108/89.

## Known gaps today

- **Best torque comes with half the charge burned at 0.3 degrees after top centre**, where engines show 6 to 12. The
  cylinder loses too little heat to its walls near the top; the Woschni wall model is meant to fix this.
- The exhaust temperature peaks at lambda 0.90, not at 1.0 to 1.05 as in engines.
- **Exhaust port gas runs 1150 to 1330 K** at full load where 900 to 1100 K is usual.
- The heat kept by the charge still includes the original simulator's allowance for wall losses, so wall heat is
  counted twice when Woschni walls are on; the fix is agreed and pending.
- The knock delay is calibrated against one engine (the original simulator's 10:1 V8 is at its knock edge on regular,
  clean on premium).
- With both this model and the ECU's on, an engine does not start in air at -40 C (253 K and below).
