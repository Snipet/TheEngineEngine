# The model's compressor map, as built (for the published-map comparison the lead has a research agent reading)

The three compressors the lead named, as F's matching builds them on the tip (the design-point rule, designShare
0.85; the truck as D2's case: boost.model 1, single, VGT, gateSpring 220 kPa). Read from the running machine
(boost.mach.comp[0]); the lines computed from F's own formulas (sim/boost/compressor.fm), air at 101325 Pa and
288-300 K. "Surge" is the flag's line (0.8 of the knee's share on each speed line), "island" the peak-efficiency
flow (phiB), "choke" the speed line's choke flow; M is the tip Mach number of the speed line through that point
(560 m/s at the tip limit: M 1.61 to 1.65).

| compressor (as built) | inducer mm | PR | surge flow kg/s (M) | island flow kg/s | choke flow kg/s | island/surge | choke/surge | eta near surge | at tip limit? |
|---|---|---|---|---|---|---|---|---|---|
| turbo-four-20-gdi | 44 | 1.5 | 0.082 (0.83) | 0.152 | 0.262 | 1.85 | 3.18 | 0.65 | |
| turbo-four-20-gdi | 44 | 2.0 | 0.100 (1.11) | 0.184 | 0.262 | 1.85 | 2.64 | 0.66 | choke beyond (M 1.86 > 1.65) |
| turbo-four-20-gdi | 44 | 2.5 | 0.105 (1.30) | 0.190 | 0.262 | 1.81 | 2.50 | 0.66 | choke beyond (M 2.18 > 1.65) |
| turbo-four-20-gdi | 44 | 3.0 | 0.105 (1.44) | 0.190 | 0.262 | 1.81 | 2.50 | 0.64 | choke beyond (M 2.42 > 1.65) |
| diesel-20-common-rail | 38 | 1.5 | 0.062 (0.83) | 0.114 | 0.197 | 1.85 | 3.18 | 0.65 | |
| diesel-20-common-rail | 38 | 2.0 | 0.074 (1.11) | 0.138 | 0.197 | 1.85 | 2.64 | 0.66 | choke beyond (M 1.86 > 1.65) |
| diesel-20-common-rail | 38 | 2.5 | 0.079 (1.30) | 0.142 | 0.197 | 1.81 | 2.50 | 0.66 | choke beyond (M 2.18 > 1.65) |
| diesel-20-common-rail | 38 | 3.0 | 0.079 (1.44) | 0.142 | 0.197 | 1.81 | 2.50 | 0.64 | choke beyond (M 2.42 > 1.65) |
| truck-15l-six | 88 | 1.5 | 0.320 (0.83) | 0.591 | 1.019 | 1.85 | 3.18 | 0.65 | |
| truck-15l-six | 88 | 2.0 | 0.386 (1.11) | 0.714 | 1.019 | 1.85 | 2.64 | 0.66 | choke beyond (M 1.86 > 1.61) |
| truck-15l-six | 88 | 2.5 | 0.408 (1.30) | 0.739 | 1.019 | 1.81 | 2.50 | 0.66 | choke beyond (M 2.18 > 1.61) |
| truck-15l-six | 88 | 3.0 | 0.408 (1.44) | 0.739 | 1.019 | 1.81 | 2.50 | 0.64 | choke beyond (M 2.42 > 1.61) |

## What sets each (the constants to set from the published maps)

| quantity | constant (compressor.fm / machine.fm) | value now | basis |
|---|---|---|---|
| choke flow's scale | mc = 0.45 rho01 a01 A_ind arWide | 0.45 | judgement (a choked eye's flow coefficient) |
| the inducer's size | A_ind so that the full choke is 1.15 x the design flow (machine.fm, aAnn) | 1.15 | judgement |
| choke flow against speed | x (0.3 + 0.7 min(M / 1.2, 1)) | 0.3, 1.2 | judgement: **saturates at M 1.2**, so every line above PR ~1.8 chokes at one flow, where real maps' choke grows with speed |
| the knee (the surge line's place on a speed line) | phiK, a share of the choke flow | 0.5 | judgement |
| the surge flag's line | 0.8 x phiK | 0.8 | judgement |
| the island | phiB | 0.725 | judgement |
| the island's width in flow | eta = etaM - 0.9 (phi - phiB)^2 | 0.9 | judgement |
| efficiency against tip speed | etaM = etaMax - 0.25 (M - 1.1)^2 | 0.25, 1.1 | judgement (N3's "Mach term") |
| peak efficiency | etaMax (spec compEta) | 0.76 | the spec's default |
| pressure rise | tau_knee = 0.45 U^2 / (cp T), tau0 0.88, tauC 0.35 of it | 0.45, 0.88, 0.35 | judgement (a slip factor and losses) |
| the tip limit | omegaMax = 560 m/s / r_tip | 560 | judgement |
| A/R's widening | arWide = (A/R / 0.6)^0.25 | 0.25 | judgement |

## Read

The model's shape is the same for every wheel: island at 1.8 to 1.85 times the surge flow and choke at 2.5 to 3.2
times it at ratios 1.5 to 3.0. Choke saturates at the tip Mach of 1.2 (above PR ~1.8 every line chokes at one
flow). At PR 2 the surge flow is 0.38 of the full choke flow (the truck: 0.386 against 1.019 kg/s). Recalled, to be
replaced by the agent's table: published truck and diesel wheels put the surge line at PR 2 nearer 0.25 to 0.30 of
choke, and a ported shroud lower still. If the table confirms it, the knee (phiK), the surge locus (0.8) and the
choke's growth with speed (the 1.2 saturation) are the three constants that move.
