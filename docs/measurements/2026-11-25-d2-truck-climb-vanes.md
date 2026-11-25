# The truck's climb at 1300 rpm: smoke-limited, held by the vanes (D2, 25 Nov)

For card N2 (the vane loop) and the lead. One render on the tip of 25 Nov ~03:00, before N2's compressor commit:
`truck-15l-six` with N2's GateTest sets (`boost.model` 1, `kind` 1, `layout` 1, `vanes` 1, `gateSpring` 220000), 300 K,
101325 Pa; the engine dyno at 800 rpm floored for 1 s, `exhaust.heatSettle` and `boost.heatSettle` pressed, 3 s more,
then held at 1300 rpm floored for 30 s.

**The fuel.** The rack is full (1.00) throughout; the boost compensator (`ecu.afc` 0.8, D2's) caps what is injected at
an equivalence ratio of 0.80 (AFR some 18, lambda 1.25): what burned read 0.80 every second, smoke 1.8 FSN. The
"20.18 g/s, AFR 7" read earlier was `tel.ecu.fuelFlow` reporting the rack's ask, not the fuel (fixed: it reports the
injected fuel since this date); `tel.ecu.lambda` read 0 because the truck runs `ecu.model` 0 (it now reads the charge's).

**What holds the climb: the vanes.** From 6 s the vanes sit at 1.00 (wide) at some 11 kPa of boost, the turbine inlet
120 to 128 kPa absolute at 813 to 860 K, the torque ~880 N m, for the rest of the 30 s:

| t (s) | boost (kPa) | vanes | T3 (K) | p3 (kPa abs) | torque (N m) |
|---|---|---|---|---|---|
| 1 | 4.2 | 0.30 | 705 | 148 | 419 |
| 3 | 16.4 | 0.30 | 761 | 159 | 361 |
| 6 | 8.7 | 1.00 | 813 | 123 | 888 |
| 15 | 11.6 | 1.00 | 842 | 126 | 872 |
| 30 | 11.4 | 1.00 | 847 | 120 | 885 |

A guess, not proven: the back-pressure limit (machine.fm's bpOpen, integrating the turbine inlet over the charge's
pressure past bpLimit 1.4) latches at low boost, where that ratio is naturally above 1.4.

**Without the smoke limit** (`ecu.afc` 0, the tuner's removed stop): it burns at phi 1.0, T3 rises to some 950-1030 K,
the climb is faster: 15 kPa at 6 s, 26.5 at 15, 39.9 at 30 (vanes still 1.00), torque to 1540 N m, smoke 10 FSN. So
over-fuelling does not slow the climb in the model; the smoke limit costs some (as in life).
