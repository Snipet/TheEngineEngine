# Two-stroke

A crankcase-scavenged two-stroke of one to four cylinders, each with its own expansion chamber: ports in the cylinder
wall, the crankcase as the pump, a reed valve, rotary disc or piston port, and the tuned pipe.

It runs when Cycle (`engine.cycle`) is set to Two-stroke on a custom engine. The four-stroke cylinder model is not
used then. There is no "Reference" two-stroke: the original simulator had none. Five two-stroke engines are shipped: a 125
motocross single, a 250 road twin, a 500 piston-port triple, a 100 cc kart engine with a rotary disc, and a 50 cc
utility engine.

## What is simulated

- **Ports opened by the piston.** Each port opens when the piston crown passes its top edge. Its window is a
  rectangle with rounded top corners; its open area grows as the crown goes lower. A **power valve** lowers the
  exhaust port's top edge below a set rpm and raises it above another.
- **The crankcase** as a second volume pumped by the underside of the piston, fed through a reed (a sprung petal that
  opens with the pressure drop), a rotary disc (a window of crank angle) or a piston port, behind the carburettor.
- **Scavenging**: the fresh charge pushes out and mixes with the burned gas, with a share short-circuiting straight
  out of the exhaust. A returning wave from the pipe can push the last gas back in.
- **The burn** at the spark, with misfires when the charge is too diluted; unburned charge stays for the next cycle.
  This makes the engine "four-stroke" at light throttle by itself, as real two-strokes do.
- **The expansion chamber**: header, diffuser, belly, baffle cone and stinger as stepped pipes, with an optional
  silencer. The diffuser's suction helps pull charge through the transfers; the baffle cone's returning pulse plugs
  the port before it closes. The tuned rpm follows the pipe's length (within about 5 % of the returning wave's timing)
  and rises as the pipe heats.

Measured on the pipe: torque rises 42 % in 1000 rpm as the engine comes "on the pipe". The power valve adds 37 % of
torque at 4000 rpm and nothing in the band.

## What is left out

- **The inertia of the gas in the inlet tract and the transfer ducts**, which on a racing engine pushes 10 to 20 %
  more charge through than the crankcase alone. This is the main reason the output is low (below).
- A shared chamber for several cylinders, a boost port, auxiliary exhaust ports, oil smoke.

## What the numbers mean

| Setting | Meaning | Typical |
|---|---|---|
| Exhaust port open (`engine.tsExhaustDur`) | Crank degrees the exhaust port is uncovered; longer gives power higher up | 175 to 200 on racing engines |
| Transfer ports open | Degrees the transfers are uncovered | 120 to 135 |
| Crankcase compression | The crankcase's volume ratio | 1.3 to 1.5 |
| Inlet | Reed, disc or piston port | |
| Header, Diffuser, Belly, Baffle cone, Stinger | The pipe's sections; total length sets the tuned rpm | |
| Power valve closed / Opens from / Fully open at | The valve's lowered timing and its rpm range | |
| Scavenging: mixing, Short-circuited share | How the fresh charge and burned gas mix | 0.55; 10 % |

## Known gaps today

- **Output is low for racing engines**: the 125 makes 11.6 kW against some 28 kW for its class (41 %), the 250 twin
  72 %, the 500 triple 65 %, the kart 46 %; the 50 cc utility engine 82 %. Each peaks within 10 % of its class's rpm.
  The missing inlet and transfer inertia (above) is the main cause.
- At idle every build misfires on 80 to 84 % of its cycles, nearer "eight-stroking" than four.
- The pipe's packing passes slightly more than it takes at low frequency; a broadband gain works around it.
- Purity, delivery ratio, trapping and the power valve's position are not reported yet.
- Nobody has listened to it yet.
