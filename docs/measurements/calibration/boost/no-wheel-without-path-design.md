# No wheel without a path: the design (26 Nov, the next window's first item; N2 with the integrator, to the lead)

On paper, before any code or number. The fault and its four rows are in dead-housing.md. The builder's side is the
integrator's (docs/handoffs/d-no-wheel-without-path.md, written by the integrator; its gist is restated here where
the design leans on it).

## What exists

- **The builder** (`XBuild::resolve`, slots.fm) makes one collector a bank unless a build draws more
  (`exhaust.primaryTo`). Each collector ends in `eng.hmap[bank*2 + group]`, filled before resolve from
  `Boost::housingOf(bank, group, banks)`. So group 1 exists only where a build draws it.
- **Boost** gives turbo t the housings 2t (and 2t+1 for a twin scroll). Each turbine's nozzle is solved from its own
  housing against its downpipe's end (`Machine::downpipe`), and the gate likewise. A housing no pipe reaches holds
  ambient air, so its turbine stands (the four rows: shafts at 0).
- **A precedent in the machine:** a twin scroll's two housings are one source at their mean state against the
  downpipe, and the flow is then shared between them by each one's own orifice law. Several nozzles on one gas state
  is already how the machine works.

## (ii) The guarantee: a turbine whose housing no collector reaches draws from its fed sibling's

1. **The builder says, after resolve, which housings a collector reaches:** one call, `Boost::housingsFed(mask)`
   (bit h). It is known when the engine is built, not after a run, as the integrator offered (point 2), and it is
   called at every configure, as `exhaustSides` is.
2. **Boost keeps a map, `feedOf[t]`:** the housing turbine t's nozzle and gate draw from. It is 2t by default. Where
   2t is unfed, it is the fed housing of the same bank's other turbo in the arrangement (a twin on one bank:
   turbo 0's; a quad: the bank's first). Its volume becomes the sum of the two (a common manifold's volume is the
   housings' together), and the unfed housing is left out of the solve.
3. **The split needs no new number.** The nozzles that share a housing are solved against their downpipe ends from
   one housing pressure and temperature, each by its own orifice law. That is what two turbines on one manifold
   do: one plenum, one pressure, flow by area. Each turbine's work comes from its own share. The books close
   because the shared housing books every nozzle's outflow (`mOutPipe` summed).
4. **Arrangements that share by design are untouched:** sequential (the transfer valve) and compound (high stage
   into low) keep their own wiring. `feedOf` acts only on twin and quad, the layouts whose wheels are meant to be
   fed in parallel. Boost tells the builder this (the integrator's point 4) through `housingOf`, which already
   returns one housing for those.
5. **Bit 128 stays, with a narrower meaning:** a turbine with no path at all, which (ii) makes impossible. It
   should then read 0 everywhere; its test changes to say so.

What it does not do: it builds no second collector, so a quad on a cross-plane V8 runs two turbines per collector.
That is a real arrangement (a common manifold), not a dead one.

## (i) The refinement: where an even split exists, the builder splits

- Where the layout table's firing order gives an even split per bank (the integrator's point 3: the inline four
  1-4 / 2-3, the flat-plane V8, the V12), the builder draws two collectors a bank for twin or quad, as its custom
  rule already does by firing order. There, each turbine has its own collector and (ii) never acts.
- **Default or choice:** a default, because a parallel pair on an engine that can split is drawn split. I
  suspect this from the production twins I know of (straight sixes 1-3 / 4-6), but I have no source in hand for the
  inline four or the V12 yet. A build's own `exhaust.primaryTo` keeps the last word, as today.
- Elsewhere (the cross-plane V8, the V6, the V10, the flat six, the odd inline engines, the flat four), (ii) holds
  the floor.

## Order and who

1. (ii) first: N2's in boost/ (`feedOf`, the volume, the nozzle and gate solve), plus the integrator's one call in
   the exhaust (`housingsFed`). It alone ends every dead housing.
2. (i) after it: the integrator's, the custom rule's table twin.
3. The four rows re-read after each step.

## Predictions for when it is built (before any code)

1. After (ii), every boosted archetype as built is unchanged to the bit: no housing of theirs is unfed. The null test
   is unchanged: the reference has no machine.
2. The default V8 as Quad: four shafts within 2 % of each other a bank, torque within 5 % of its Twin's (699 N m at
   5000 rpm), against 598 now.
3. first.json as Quad (two collectors after the integrator's split): four shafts turning, torque within 10 % of its
   Twin's 302 N m, against 164 now.
4. Bit 128 reads 0 on all of the above.
5. After (i), the V8 (cross-plane) is unchanged; a flat-plane V8 as Quad moves by under 3 % against (ii) alone.

## Open, for the lead

- **A source for the common-manifold parallel pair:** I take its physics (one plenum, flow by area) as the model.
  If a measured case of two turbines sharing one log manifold turns up, the split is checked against it.
- **The housing volume when shared:** the sum (item 2). The alternative, keeping each housing's own volume behind a
  short pipe, adds a number with no source.
