# A turbo the exhaust leaves without gas: the Aspiration dial reaches one (26 Nov)

The integrator's probe found the rotary's second wheel without gas. That turned out to be a sequential pair below its
join, by design: the transfer valve feeds housing 2, and turbo 1 turns at 10,000 to 10,600 rad/s from 5500 rpm. The
lead then asked whether a dial alone could reach a dead housing. It can.

## Measured (the branch at 87fefb56 with mismatch bit 128; engine dyno, 5000 rpm, full throttle, 3 s, the last 0.5 s)

Predicted before the runs: first.json as Twin leaves turbo 1 without gas, the default V8 as Quad leaves turbos 1
and 3, and both read bit 128. Both right; the four with Quad was worse than predicted.

| build, Aspiration | N m | shaft rad/s, turbos 0 / 1 / 2 / 3 | turbine 0 inlet | bit 128 |
|---|---|---|---|---|
| first.json (the table's inline four, one collector), Twin turbos | 231 | 14,381 / 0 | 2.72 bar abs | set |
| first.json, Quad turbos | **6** | 17,327 / 0 / 0 / 0 | 5.40 bar abs | set |
| the default V8, Twin turbos | 700 | 8,523 / 8,535 | 2.01 bar abs | clear |
| the default V8, Quad turbos | 598 | 19,585 / 0 / 19,639 / 0 | 3.33 bar abs | set |

Every one of the 31 boosted archetypes, as built, reads bit 128 clear.

**Re-read after the integrator's firing-order split (e0bdc69f) landed:** first.json is a custom engine, and its
headers now split for twin turbos.
- first.json as Twin feeds both wheels: 302 N m, shafts 13,369 / 13,329, bit 128 clear.
- first.json as Quad: 164 N m, shafts 14,528 / 0 / 14,531 / 0, bit 128 set.
- The default V8 rows are unchanged: Twin 699 N m and clear; Quad 598 N m, turbos 1 and 3 standing, set.

So Quad still reaches a dead housing on both. The test holds the V8: Quad reads 128 with turbos 1 and 3 standing,
Twin reads 0.

**Why:** `Boost::housingOf(bank, group, banks)` gives a twin on one bank the front group's collector to turbo 0 and the
rear group's to turbo 1, and a quad the same on each bank. A layout whose exhaust builds one collector a bank (the
table's inline four; the V8's one 4-into-1 a bank) builds no group 1, so that housing takes no pipe. With Quad on the
four, all the engine's gas goes to turbine 0, sized for a quarter of it: 5.4 bar ahead of it and the engine choked.

## Ruled (the lead, 26 Nov)

- **This window:** bit 128 (telemetry only) and its test (calibration.boost.warn): the default V8 as Quad reads it
  with turbos 1 and 3 standing; as Twin it does not. The AIR workspace words it for the owner (U1).
- **The next window's first item (N2 with the integrator, a design to the lead first):**
  - (ii) as the guarantee: wheels beyond the built collectors share their gas, so no dial can make a dead housing;
  - (i) as the refinement where an even split exists: the integrator's custom split (02388191) extended to table
    engines;
  - each from what such engines really do.
