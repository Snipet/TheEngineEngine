# The 15 L tractor's low-speed end (the lead's queue; finding first)

`archetypes/tractor-15-inline-six`, full load, held 30 s, approached from 800 and from 1300 rpm, the last 10 s; the
compressor's point against its knee from its telemetry by the map's own formulas (compressor-map-model.md).

| rpm | N m (from 800 / 1300) | boost kPa | vanes | flag duty | phi / knee | PR |
|---|---|---|---|---|---|---|
| 900 | 627 / 629 | 11.4 / 11.6 | 0.90 | 0.09 | 0.75 | 1.12 |
| 1000 | 651 / 653 | 13.4 / 13.7 | 0.99-1.00 | 0.09-0.23 | 0.82 | 1.15 |
| 1058 | 1502 / 1502 | 50.3 / 50.1 | 1.00 | 0.49 | 0.82 | 1.52 |
| 1061 | 1720 / 1726 | 58.0 / 58.7 | 1.00 | 0.48 | 0.81 | 1.60 |
| 1100 | 1910 / 1931 | 75.5 / 77.4 | 1.00 | 0.45 | 0.83 | 1.78 |

- No path dependence in these holds (C's knife's edge at 1058/1061 not reproduced); a cliff, 651 -> 1502 -> 1910 N m.
- **Below it**: the compressor steady (block by block 0.136-0.146 kg/s at 1000, no reversal), its ratio pinned at the
  surge flag's own gate (1.143-1.153 against "pr > 1.15"), flagged a fifth of the time, the guard holding the vanes
  open: the same self-reference as the refuted cap (a guard deciding on a flag the vanes' own ratio decides).
- **Above it**: a deep surge cycle passing for the good state (1100: -0.11 to +0.59 kg/s, 58 reversals in 200 blocks).
- **Under both**: the matched compressor is too big for this engine's low-speed flow: 0.75-0.83 of the knee at every
  speed from 900 to 1100; at 1100 (PR 1.8) some 0.23 of choke where the published surge share at that ratio is about
  0.27.

## (a) the guard on what the compressor does, measured alone and held (the lead's ruling)

The guard opening the vanes on the flow reversing or its swing about its 50 ms mean beyond 0.3 of that mean, not on
the flag (`guard-on-flow.patch` beside this file; not in the tree). Measured:

| | the tip | (a) |
|---|---|---|
| tractor 900 | 627 / 629 N m, 11.4 kPa | 628 / 638, 11.8-12.7 |
| tractor 1000 | 651 / 653, 13.4 kPa steady | 676 / 688, 18-20, **a pulse 14-29 kPa on some 3 s** |
| tractor 1058 / 1100 / 1300 | 1502 / 1910-1931 / 2759 | 1495-1505 / 1918-1936 / 2763 |
| truck 1300 (GateTest's sets, settled) | 1546 / 1562 | 1543 / 1556 |
| 2.0 common-rail 1500 / 1800 / 2000 | 144 / 156 / 171 | 143 / 157 / **200** (25 -> 38 kPa) |
| the other take-ups | | within 1 % |

It removes the self-reference and gains the 2.0 at 2000; it trades the tractor's steady hole at 1000 for an audible
pulse: with the vanes closing, the compressor really oscillates there. The wheel cannot hold more ratio at 1000
whatever the guard does; 1100 still surges. Held, to be judged with (b).

## (b) inputs, waiting on the rule

- P, sources §18: Woollenweber (US 5,025,629): the compressor "matched to the engine to avoid the surge area at torque
  peak" and efficient at rated; Vitesco (US 10,900,497): the full-load curve "completely within the usable compressor
  characteristic map", the margins the vehicle maker's; BorgWarner's engineers (enginelabs.com): "10-percent or more to
  the right of the surge line" in flow, not tied to an engine point; when both ends cannot be met: a ported shroud
  (Holset US 6,540,480: up to 15 % more surge margin), a falling boost target at low speed (Garrett), a wider map. No
  stated margin at a heavy diesel's torque peak was found.
- D2 and P on the turbine side: the tractor's exhaust port at 1.40 / 1.52 of the intake port at 1300 / 1600 rpm
  (PMEP 1.9 / 2.6 bar; BSFC 203 g/kWh against a 15 L's published 177-195); a Cummins 15 L at cruise (OSTI 1474075,
  1130 rpm, 1437 N m) at about 1.05, P's derivation; no source for peak torque or rated.
