# What a turbine is matched to: a design for the lead's ruling (N2, 25 Nov; nothing built, no number chosen)

The lead's window item 5: the rotary's joined pair, rule (c), the turbine's rated match and the 2.0 at 1750 are one
question. This note answers it from sources and from what is measured, and names the choices that are the lead's.

## What the sources say a match is

- **Compressor** (P's section 18; Woollenweber, Vitesco and BorgWarner): "avoid the surge area at torque peak and stay
  in as high an efficiency area as possible at rated speed and load". About 10 % right of the surge line. The full-load
  curve lies inside the map, and the rated point under the speed limit (Woollenweber's example at 94 %).
- **Turbine:** no source gives a turbine rule as such. What the sources fix is its consequences:
  - the target boost is made where the build's torque curve says it is (the torque peak's start: VW's 2.0 "from 1,750
    rpm", the X15's "starting at 950 rpm", P's section 20);
  - rated flow passes at a back pressure the engine tolerates (p3/p2 about 0.90-0.93 without EGR at peak torque, a
    patent's estimate; 1.05 at a Cummins cruise point, measured; section 18).
- **The sequential pair** (sections 16 and 23): the second turbo joins at about 4500 rpm at full throttle. Both turbos
  are HT-12s. Boost is 0.7 bar on the primary, dips to about 0.55 at the join, then holds 0.7 on the pair to the
  redline.

## What is measured (the docs beside this one)

| finding | where |
|---|---|
| At rated every diesel's turbine runs at its efficiency peak (BSR 0.64-0.70). Matching on its own efficiency changes nothing. | rated-nozzle-q2.md |
| The rated balance closes within 1-7 % once p4 (117-137 kPa measured, not 101) and T3 (803-911 K, not 900) are in. Today's 1.247 p2 stands in for both, by accident. | rated-nozzle-q2.md |
| p4 has a fitted form: Δp = a·Q + b·ρQ² (first.json's fit, 2.2 kPa rms). `XBuild::pConv` gives it in the running engine. | G, exhaust.md |
| Rule (c) sits on the choke flag. The cycle's peak phi over the mean is 1.001-1.023, so a mean of 0.96 clears every build measured (labelled judgement). | c-margin.md |
| The 2.0 at 1750: 244.8 N m against VW's 320 (Table P). The back-pressure limit holds its vanes; at the stop it read 260. | package-final.md |
| The rotary's pair is starved by its transfer, and its turbines are too big for the pair's flow (45 kPa with the gate shut). | rotary-pair.md |
| A feed-forward from the 1.247 broke the truck at 1300 (vanes past the boost's maximum). | vgt-feedforward.md |

## The design proposed

**1. The compressor: (b) as built, checked at rated.** The wheel is (b)'s, at the torque peak with the surge margin.
At rated, rule (c)'s check runs on the mean phi with the measured pulsation's margin (0.96, labelled). Where no wheel
meets both ends, the build is flagged (U3's mismatch telemetry), not compromised by a chosen rule.

**2. The turbine's open area: from the rated balance, with the engine's own p4 and T3.**
- Compressor power at rated (the map's efficiency at that point) equals turbine power. The turbine law at its BSR is
  its peak there, as measured.
- Expansion runs to p4 from the build's exhaust (pConv at the rated flow, or G's two-term fit per build), with T3 from
  the engine at rated.
- p3 falls out of the balance, and the open area passes the rated flow at that p3 (the match's choked form).
- **No 1.247, no 900/1150 K.**

**3. The low end: a check, not a second match.**
- With the vanes at their stop (a VGT) or the gate shut (a wastegate), does the balance make the target at the torque
  peak's start (the 2.0's 1750, the tractor's 950-1000)?
- If not, the build is flagged with the shortfall. The vane stop stays the build's (vaneMin), not fitted to the row.
- The 2.0's 1750 is this check's first case. Its vanes are held open by the back-pressure limit, so the check reports
  whether the limit or the stop binds.

**4. The sequential pair.**
- The single is matched to make the target at the join's low side.
- The pair is matched with each turbine passing half the flow at the target from the join (4500) to the redline.
- The transfer is an unrestricted path (the housing's own inlet).
- The equal split stays labelled judgement (two HT-12s).

**5. The VGT loop's feed-forward (research):** once p3/p2 at each point comes from the same balance, the feed-forward
can read the steady opening from it, not from the 1.247. Not before.

## The choices that are the lead's

- **(i) Where p4 and T3 come from at sizing.** The machine is sized before the engine runs. Either:
  - **(a)** one quasi-static pass at load: match, run a short rated point, re-match once;
  - **(b)** per-build fitted coefficients (G's a, b; a T3 at rated) stored in the build;
  - **(c)** a class rule (p4 and T3 tabled by engine class from the measured builds), labelled judgement.
  - My preference is (a), since it uses nothing the build does not have. It costs a second or two at load per boosted
    build, and the steady state must come out the same on a second load.
- **(ii) What a build that cannot meet both ends gets.** A flag and its shortfall (my preference), or a compromise
  rule. The tractor at 202 kPa on one wheel (tractor-map-reading.md) is such a case.
- **(iii) Whether the low-end check may move the vane stop.** My preference is no: the stop is the build's, and a
  shortfall is reported.

## What I expect each part to change (predictions, for the ruling)

- **The diesels at rated:** little. The 1.247 was within 4 % on three of four. The truck's sets move most (its 1.18).
- **The rotary above the join:** from 0.45-0.53 bar to the cited 0.7, torque at 5000 from 233 towards 294 N m.
- **The 2.0 at 1750:** the check reports. My expectation is that the back-pressure limit, not the turbine's size,
  binds there (260 N m at the stop against 320), so the row stays a miss with a named cause until the limit's 1.4 is
  sourced.
- **The petrol turbos:** a few % at their top. Their 1150 K is replaced by their own T3.
