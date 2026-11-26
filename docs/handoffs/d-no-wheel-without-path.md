# No wheel without a path: what the exhaust builder can offer Boost (D, 26 Nov; notes for N2, a design to the lead)

The lead's next-window item, with N2 (docs/measurements/calibration/boost/dead-housing.md): (ii) as the guarantee, wheels
beyond the built collectors share their gas, so no dial can make a dead housing; (i) as the refinement where an even
split exists. These notes are what the builder side holds today, read from the code. Nothing under sim/ is changed.

## Where the collector count is decided

- **Ports to collectors:** `XBuild::resolve()` (sim/exhaust/slots.fm). Each port's primary goes to
  `spec.exhaust.primaryTo[i]` when the build sets it (0 to 9, or -1 an open stack). Otherwise it goes to `autoPort(i)`:
  collector 0 for topology 0, else the port's bank (`eng.bank[i]`), so one collector a bank.
- **Collectors to sides:** `spec.exhaust.collectorTo[c]` when set (a side, or a merge into another collector, two
  levels at most); otherwise side `c & 1` with two sides, else side 0.
- **Collectors to housings:** in `resolve()`, under `eng.useHousings`, each collector in use and going straight to a
  side counts in its bank's order (`nbk[bk]`; the bank is the bank of the last port that feeds it). It takes
  `eng.hmap[bk * 2 + g]` with g = 0 for the bank's first collector and 1 after. `hmap` is filled before the build by
  `Exhaust` (exhaust.fm ~1420) from `Boost::housingOf(bank, group, banks)`, where `banks` is 2 when any port is on
  bank 1.
- **The topology and the banks come from the layout:** `lay.topo` and `lay.bank` (sim/core/layouts.fm's tables;
  `customLayout()` for a custom engine). Since 823e45cd, 9c107de6 and e0bdc69f, a custom one-bank engine splits its
  ports into two "banks" (firing order, every other cylinder) when its table splits or its twins sit a bank each.
  Under topology 1 those two halves are two collectors into one pipe: a twin on one bank then works through
  `banks > 1`, not through group 1.

So today a bank never gets a second collector by default. Group 1 exists only when a build draws it (primaryTo), and
`housingOf`'s group-1 housings (a twin on one bank, without the split above; every quad) take gas only then.

## What the builder could offer

1. **The counts Boost needs to know (for (ii), the guarantee):** after `resolve()` the builder knows, for each bank,
   how many collectors go straight to a side (`nbk[0]`, `nbk[1]`) and which housing each took. Two ways to hand that
   over:
   - Before the map: `Exhaust` calls `housingOf(bank, group, banks, groupsInBank)` with the bank's count, so Boost can
     give a bank with one collector all of its wheels on that collector (one collector, two or four housings: the
     "sequential" form's junction, parallel). Today `hmap` is filled before `resolve()` runs, so the count has to come
     from a first resolve or from the same rule (`autoPort`/primaryTo) evaluated early.
   - After the map: the builder reports, per housing, how many collectors feed it (0 = dead), and Boost re-maps or
     shares. That's the same information bit 128 reads from the shaft today, but known at the build, not after a run.
   Either way, the junction for "one collector, two housings" is the builder's to make (a node with the collector's
   pipe and both housings' inlets, as `jMuf` makes a muffler's): I can describe where it goes in `shape()` for N2's
   design.
2. **An even split per bank where one exists (for (i), the refinement):** the same rule as `splitEvenly()` in
   engine.fm, applied per bank to the table engines, with `autoPort` assigning a port to collector `2 * bank + half`
   instead of `bank`. Computed from the layout tables' firing angles (each bank's cylinders in firing order, every
   other one to each half, both halves firing evenly):

   | layout | cylinders a bank | a bank fires evenly | an even split a bank |
   |---|---|---|---|
   | Inline 4 | 4 | yes | yes (1-4 / 2-3) |
   | Inline 6 (table: two halves, topo 1) | 3 a half | yes | no further (odd) |
   | Flat 4 (topo 1) | 2 | no | no |
   | Crossplane I4 | 4 | no | no |
   | Inline 3, Inline 5 | 3, 5 | yes | no (odd) |
   | V6, Flat 6 | 3 | yes | no (odd) |
   | V8 crossplane | 4 | no | no |
   | V8 flat-plane | 4 | yes | yes |
   | V10 | 5 | yes | no (odd) |
   | V12 | 6 | yes | yes |
   | singles and twins | 1 to 2 | | no |

   So (i) helps an inline four with twins, a flat-plane V8 or a V12 with quads. A cross-plane V8 (the default) with
   quads has no even split a bank, so its turbos 1 and 3 need (ii). That matches the dead-housing table: the default V8
   as Quad reads bit 128.
3. **What I'd ask Boost to tell the builder:** how many wheels each bank wants (1 or 2), so the builder splits only
   when asked (as e0bdc69f reads the arrangement from the Spec today). And which arrangement shares a collector by
   design (sequential, compound), so the builder never splits those.

## Open, for N2's design

- (ii) where wheels share one collector: the gas divides by each housing's admittance. Is that what a real parallel
  pair on a common manifold does? A source is N2's to find.
- Whether (i) should be the default for a table engine with quads at all, or only a dial (a person's header choice):
  a real flat-plane V8 with quad turbos is rare; the split changes the sound (two 2-into-1 a bank).
- The rotary: sequential, one collector by design (N2, 26 Nov); nothing for the builder.
