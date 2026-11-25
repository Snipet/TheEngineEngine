# A turbo's run-down after the key (finding; the cure waits its turn)

Since ef2160fd the simulator steps on while a turbo's shaft turns over 50 rad/s after the key (Boost::spinning()):
the turbo four (first.json, about 160,000 rpm at full load) keeps it awake some 23 s, the 15 L truck on GateTest's sets
(about 54,000 rpm) 79 to 89 s.

**Against a measured rotor** (P's sources §19: Gritsenko et al. 2023, a truck-class K27-145, oil 50-90 C at 0.1-0.4 MPa:
from 25,000 rpm 10.3-14.8 s, from 50,000 28.1-34.4 s, from 75,000 38.6-47.1 s), the model's run-down has the wrong shape
at both ends (measured locally, the bearings' loss scaled as d^2 (today), d^3 and d^4 of the compressor's diameter):

| | 50k -> 25k rpm | 25k rpm -> stop (< 478) | 50k -> stop |
|---|---|---|---|
| truck, d^2 (today) | about 2 s | 87.1 s | 89.1 s |
| truck, d^3 | about 2 s | 60.6 s | 62.5 s |
| truck, d^4 | about 2 s | 38.7 s | 40.6 s |
| K27-145, measured | about 18 s | 10.3-14.8 s | 28.1-34.4 s |
| turbo four, d^2 / d^3 / d^4 | | 21.3 / 23.6 / 25.8 s | |

- At speed the stopped rotor slows far too fast (its aerodynamic load on a stopped engine's pipes too strong).
- At low speed far too slowly: shaft.fm's loss scales as d^2 against an inertia as d^5, so the viscous time constant
  J/c grows as d^3 (IS20 11 s, the four's 20 s, the truck's 127 s); a purely viscous fit to 10-15 s from 25,000 would
  need some 40 times today's coefficient, about 1.9 kW at 25,000 rpm, more than any bearing loss found.
- The source points at the cause: the run-down lengthens with an autonomous oil supply (18.6 -> 22.8 s, a KAMAZ turbo)
  and shortens with more oil pressure: a real bearing loses its oil when the engine stops; the model's bearing has no
  oil state, its loss the same running or stopped.

**Proposed, for its turn** (the lead's order: after the rotary's cure, the tractor's matching and the rotary's joined
pair): a starved bearing's friction once the engine stops, sized from the K27's last 25,000 rpm; the stopped rotor's
aerodynamic load checked against its first 25,000. Both judgement on one measured turbo, stated as that.
