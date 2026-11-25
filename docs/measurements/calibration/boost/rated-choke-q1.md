# Research Q1: why the engine flags choke where (c) says the wheel clears. Predictions, 25 Nov ~15:30, before any run
Case: the tractor, 69.7 mm (c at 0.94), rated 1900 rpm on the taper (202 kPa gauge, PR 2.99 on 101.3 kPa).
The rule's check: phi = mr / (0.506 * 1.204 * 343.2 * A * line(m)), standard air, p1 = 101.3 kPa, T1 = 293 K,
PR = 2.99, mr from VE 0.884 (the Heywood form) and the charge at compEta - 0.09 through icEffect 0.75.
The engine's flag: phi = mdot / mc(p1, T1) at the inlet as it is, PR = p2 / p1 as it runs.
1. The boost: the engine runs over the taper (215 measured in the c94 row, not 202): +4 % PR, a higher line, more
   flow. Prediction: the largest term, 4-6 % of phi.
2. The inlet: p1 under 101.3 kPa (the filter's drop) and T1 over 293 K raise the flow's corrected share. Prediction
   2-4 % in phi together; T1 within 10 K of 293.
3. The flow: the engine's air flow at rated within 5 % of the rule's mr (the VE form and the charge temperature
   roughly right); its sign unknown.
4. The line: at the same PR the rule's m and the engine's Mach agree within 2 % (the same laws).
Sum: the rule's phi ~0.90-0.93 against the engine's 0.977; the boost's overshoot and the inlet carry most of it.

## Measured (n2-b-local 668c642e with (c) at 0.94 added, scratch; the tractor, 1900 rpm, the taper's 202 kPa)

The rule's point is the shipped build's (target 260, tapered to 202 at the rated 1900 rpm). The engine's is the cell
with the target set to 202 and the wheel held at (c)'s 69.7 mm. Rows are means. "First 16 s" covers the cell's first 16
one-second rows after the step from 1600 rpm, as every earlier (c) table read it. "Steady" is the last 30 of 60.

| | flow kg/s (corr.) | PR (p2/p1) | T1 K | p1 kPa | phi | tip Mach | charge K | boost kPa | vanes | flags (mean) | N m |
|---|---|---|---|---|---|---|---|---|---|---|---|
| the rule's | 0.674 (std) | 2.989 | 293.15 | 101.33 | 0.980 | 1.508 | 327.6 | 202 | - | - | - |
| engine, first 16 s | 0.698 | 3.304 | 288.1 | 98.89 | 0.979 | 1.607 | 331.1 | 215 | 0.76-0.81 | 2.88 | 2345 |
| engine, steady | 0.678 | 3.160 | 288.1 | 99.02 | 0.972 | 1.537 | 328.2 | 202 (190-216) | 1.00 | 0.58 | 2422 |
| (b)'s 72.4 mm, steady | 0.690 | 3.213 | 288.1 | 98.95 | 0.938 | 1.487 | 328.0 | 206 | 1.00 | 0 | 2426 |

The flag's input is the engine's phi = mdot / mc(p1, T1), checked each step against 0.98; the rows hold its means.

**Correction (vgt-windup.md):** the steady rows' wander (190-216 kPa) was the harness pressing the heat-settle every
window. Run free, the boost holds 208.5 +/- 0.5 kPa on 72.4 mm, so the margin must cover the cycle's pulsation, not a
wander.

**What it shows:**
1. **The rule puts the wheel on the flag by construction.** "The smallest wheel that clears" stops at phi = 0.980,
   with no margin. Any excursion sets the flag. Within the steady rows, phi's mean of 0.972 crosses 0.98 with the
   cycle's pulsation and with the boost's wander. The wander is 190-216 kPa, a slow swing with the vanes held fully
   open, so the loop has no authority left downward.
2. **The 16-row readings were the VGT loop's transient, not rated.** After the spool, the loop's integral (rate 1·e a
   second, bound +1.0) unwinds at some 5 % a second. For about 20 s it holds the vanes at 0.76-0.81 and the boost 13 kPa
   over the target, at PR 3.30 and Mach 1.61, with the choke flag in most rows. Steady, (c)'s 69.7 mm makes 2422 N m,
   within 0.2 % of (b)'s 2426. The earlier (c) row (2345 N m, flags 2.88; b-in-the-matching's research table) is
   withdrawn: (c) at 0.94 does not cost the tractor's rated torque. Its flag at the mean is 0.58, against (b)'s 0.
3. **The rule's point misses two drops,** the filter's (p1 99.0 kPa, 2.3 %) and the cooler's (the outlet's PR 3.16 is
   about 3 % over the boost's 303/99). Together the engine's PR is 5.7 % over the rule's. The higher line's larger
   choke flow (the line factor grows to M 1.63) makes up for it, so the mean phi ends under the rule's.
4. **T1 is 288 K, not 293** (the ambient the cell holds), 1.7 % on the corrected flow, the other way.
5. **The flow and VE agree.** The engine's corrected flow is 0.6 % over the rule's; veManifold is 0.899 against the
   rule's 0.884.

**The predictions:**
- 1 (overshoot the largest term): right in the first 16 s, and wrong at steady, where there is none.
- 2: p1 right (-2.3 %); T1 the wrong sign (288 against 293).
- 3 and 4: right.
- The sum: wrong. The cause is zero margin plus the wander and the pulsation, not a mis-stated point.

I did not find the lead's 164 kPa in any of my rows. The nearest is (c) on the double-tapered target (202 set, the
rule tapering it again to 153): 67.2 mm, boost 193, flags 2.86 in the first 16 s.

## The package cells, first 16 s against steady (the package tree, n22/sim-pkg)

Only two cells move by more than 1 %:
- **The 2.0 at 1750:** 252 -> 266 N m (boost 91 -> 101 kPa, shaft 0.592 -> 0.608), -17 % against VW's 320 (the table
  said -21 %).
- **The truck's sets at 1300:** 2653 -> 2674 N m.

Every other cell is within 0.7 %. The flags are unchanged: the truck at 2100 stays at 1.97.
