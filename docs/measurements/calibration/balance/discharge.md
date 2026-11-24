# Port quality by class of head, from bench flow (card N3, taken over from N1; started 20:15 by `date -u`)

*What `valve.portIn` (card E's `sim/valvetrain/area.fm`) should be for each class of head, read off published flow-bench
figures. Every archetype uses the defaults, 1.35 intake and 1.5 exhaust (N1's causes §B). Not yet complete: the
two-valve pushrod rows have figures; the overhead-cam and four-valve rows do not yet.*

## The conversion

The model's effective area of n intake valves of diameter d at lift L is
`A = n q 0.4 (π d² / 4) tanh(6.8 L / d) / tanh(1.7)`: a discharge coefficient on the valve's area of **0.4 q at L = d / 4**
(0.392 q at 0.23 d, 0.406 q at 0.27 d). A flow bench at 28 inches of water (6.97 kPa, air at 1.2 kg/m³) passes at most
`Q_ideal = (π d² / 4) sqrt(2 Δp / ρ)`: 463 cfm through a 2.00 in valve, 561 through 2.20 in. So
`q = (Q / Q_ideal) / 0.4` at a quarter of the diameter's lift.

## Two-valve pushrod heads (GM's LS family: figures from forum and blog postings, different benches)

| Head | Intake valve | Flow at L = d/4, cfm at 28 in | Cd on valve area | q |
|---|---|---|---|---|
| LS1, 241 casting (cathedral, 1997-2000) | 2.00 in | 219 at 0.500 in | 0.47 | **1.18** |
| LS6, 243 casting (cathedral, 2001-) | 2.00 in | 251 at 0.500 in | 0.54 | **1.36** |
| LS7 (rectangular, CNC ports) | 2.20 in | about 370 at 0.600 in (0.27 d; test pressure not stated) | 0.66 | **1.6** (unconfirmed) |

Sources: LS1TECH's "Stock LS7 flow info" and "Any ProComp Motorsports LS1 head users?" threads (the 241's and 243's
tables at 0.100 to 0.600 in), LS Engine DIY's head guides; to be checked against a maker's bench sheet. No stock
Vortec or L98 (older small-block) table was found; those heads flow less than the LS1's (a smaller port, a 1.94 in
valve), so a carburettor-era small block sits at or under q 1.18.

What follows for the archetypes (proposed to P, not yet run as a table):
- the 5.7 small block (carburettor-like) and the 3.8 V6 (a 1990s pushrod): portIn 1.15 to 1.2, not 1.35;
- the 7.0 pushrod V8 (LS7): 1.6;
- the 7.4 big block: an older rectangular port, not looked up.

Measured sensitivity on the flat six (present tip, 4750 rpm): portIn 1.35 → 1.15 takes 3 % of the torque at the
runner's tune, 7 % at 6050 rpm (causes.md N3-3's table).

## Still to do

Two-valve overhead cam (the air-cooled flat six, the VR6) and four-valve heads (production and tuned): per-valve
figures from bench sheets; and the exhaust's quality (portEx 1.5) from the same sources.
