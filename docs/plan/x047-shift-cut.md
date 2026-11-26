# X-047: the shift's cut, a design (next window's item 16)

The steward writes this page; the integrator's reads appear under the integrator's name, as the lead passes them on.
Status: a design, nothing in `sim/` until the window can take a bake (the lead's ruling, 26 Nov 07:52).

## The fault, as measured (26 Nov, the steward; X's spread instrument, 15 copies 293.15 + k x 0.001 K)

- **One flag.** On the small four's 8.5 s upshift (fuel+ecu+car, 16-sample blocks) the spark cut runs 8.5003 to
  8.7103 s, 210 ms. There is no fuel cut and no overrun cut, and the pedal stays at 1.000. On the V8 (fuel+ecu) it
  runs 107 ms. The reference cuts for 53 to 64 ms: its fuel cut, read from the gas torque's dip.
- **Two numbers nobody designed together.**
  - The vehicle hands a number for as long as its own shift window lasts (about 60 ms). `car.fm` `cutRequest()`
    returns 0.05 s without car.model; with car.model 1 it returns `m1.spec.shiftTime`, card I's whole shift time,
    default 0.15 s (`spec/car.fm:91`).
  - The ECU sets its countdown to that number again on every step it is asked
    (`launch.fm:80`: `dogLeft = dog > 0 ? dog : ...`).
  - So the cut is the window plus the number.
- **What it does.** A spark cut pumps fresh charge through. Each cylinder's burned share at intake closing falls from
  0.11 to 0.00 by the second intake inside the cut, and its trapped charge rises from 385 to about 465 mg (+21 %).
  The first firing back burns that full, purged charge. Against the median of the four firings after it, it reads:

  | | the four | the V8 |
  |---|---:|---:|
  | the source tree | 1.66 | 2.01 |
  | the reference | 1.16 | 1.51 |

  The judge then reads it against neighbours taken partly over the cut's silence: X-030 is over in 12 of 15 copies,
  the V8's upshift row in 4 of 15.
- **Probe copies** (`sim f23337cc` with one change each, never committed):

  | probe | the cut (four / V8) | first-back ratio (four / V8) | X-030 / V8 row over |
  |---|---|---|---|
  | the countdown set once at the request's start | 149 / 43 ms | 1.52 / 1.81 | 6 / 2 of 15 |
  | the cut held only while the vehicle asks | 53 / 53 ms | 1.30 / 1.80 | 4 / 0 of 15 |

- **Struck as causes:**
  - the reinstatement's share: 12 of 15 with it at 0;
  - the retard: the ratio is unchanged without it, though the row's rate halves, unexplained;
  - an overrun cut joining in: no flag;
  - the wall film: `fuel.model` has none (`combustion/mixture.fm`, "leaves out wall wetting").

## 1. The contract at the seam

The seam's words (`ecu.fm:476`, "seconds of cut a dog box asks") read as "seconds from the request's start", but no
asker has ever behaved that way: each asks for its whole window. There are three consistent forms:

- **(a) Seconds from the start**, latched at the rising edge; each asker hands its own cut's length. This keeps two
  numbers that must agree: the vehicle's window and its handed seconds.
- **(b) The engine is cut while the vehicle asks, and no longer.** The number is at most a floor, against an asker
  that asks for one step only, or a plan for the reinstatement.
- **(c) The seconds remaining**, counting down to nothing at the window's end. Under (c) `launch.fm:80` is already
  right, and only the askers change.

The steward agrees with the lead's lean, (b) with the number as a floor:

- The cut exists to unload the dogs for the window the gearbox model runs, so one owner of its length (the vehicle's
  own flag) cannot disagree with itself.
- The floor answers (b)'s one weakness, an asker that asks for a single step: such a cut lasts the floor, about one
  firing interval at least, and no longer.
- On the probe, (b) gives the reference's 53 ms on both builds.

To be refuted by the integrator's read of the askers (2), not by preference.

## 2. Every asker (the integrator's read: pending, the lead passes it on)

To fill, sample by sample, for each asker: what it returns, for how long, and the cut it wants. Seen so far in
`car.fm` by the steward:

- the bike: `quickshiftCut` while `shiftCut`;
- model 1: `shiftTime` while `cutWanted()`;
- otherwise: 0.05 s while `shiftCut`.

Still to be read by the integrator:

- the reference's paddle robot;
- model 1's robotised box;
- the dog box, up and down, and the blip;
- the truck's automated box (150 ms);
- a launch or flat shift through the same seam.

## 3. Sources for a cut's length

- Tuners report timed ignition shift cuts of 40 to 60 ms ("40-60 mS typically for our cars"), with a short cut just
  long enough for the dogs to disengage, then 20 to 30 degrees of retard ramped out over 50 to 100 ms
  ([HP Academy forum](https://www.hpacademy.com/forum/reflashing/show/gear-shift-with-ignition-cut)). Haltech's
  torque-reduction notes make the same pairing
  ([Haltech](https://support.haltech.com/portal/en/kb/articles/torque-reduction)).
- The code's own bike quickshifter: `quickshiftCut` defaults to 0.05 s, range 0.02 to 0.12 (`spec/car.fm:186`).
- Card I's 0.15 s is a shift's time, about three times a cut.
- Still to find (the integrator's list): a quickshifter maker's manual, a motorsport ECU's flat-shift notes, and a
  dual clutch's torque phase (which does not cut at all).

## 4. Acceptance, stated before code

- Each kind's cut equals what that kind wants within one control step, measured on `ecu.flags`.
- Model 0 unchanged to the bit.
- Every archetype's tables reproduced by the bake: no table reads a shift.
- The V8's upshift row 0 of 17, and the four's X-030 stated as a rate against its 12 of 17, on X's spread instrument.
- The downshift's blip unchanged, or its change predicted.
- N1's dips test and P's idle gate untouched.

## 5. What it does not cure

On the probe with the cut at the reference's length, the four's first firing back still reads 1.30 against the
reference's 1.16, and 4 of 15 copies remain over. A cut of any length purges the residual by its second intake. What
is left is the burn of a purged, full charge (real engines do it too: a torque step on recovery, sourced; its size is
not), together with the judge's window straddling the cut, which is X's to restate. The owner's ears decide the
size, on the six clips shown on 26 Nov.
