# X-047: the shift's cut, the design (next window's item 16)

The steward writes this page; the integrator's reads appear under the integrator's name, as the lead passes them on.
Status: THE DESIGN, ruled (the lead, 26 Nov 07:54: the contract (b), plain; §1). Nothing in `sim/` until the
window can take a bake (26 Nov 07:52).

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

**Ruled (the lead, 26 Nov 07:54): (b), plain.** Under the new models the engine is cut while the vehicle asks,
and no longer. The number's size is not read: above zero means "asking".

- **No floor.** A floor would hold model 1's cut at its 0.15 s, so the fault would stay, and the read below shows no
  one-step asker for a floor to guard against.
- **The guard instead:** a test that every asker's window lasts at least two ECU steps, so that a future one-step
  asker fails a test, not a shift.
- **The result:** each kind gets what its own flag wants (60 ms, the truck's 150 ms, the bike's `quickshiftCut` as
  set).
- **Scope:** the whole change is `launch.fm`, the steward's. The vehicle's numbers (0.05; `shiftTime`) are left as
  they are, and are no longer lengths. Renaming or removing them is the integrator's, later, and not part of this item.
- **Model 0** unchanged to the bit.

## 2. Every asker through `cutRequest()` (the integrator's read, passed on by the lead, 26 Nov 07:54)

Stepped every 16 samples; file:line as the integrator gave them.

| asker | when it asks | what it hands | the cut it wants |
|---|---|---|---|
| without car.model (the reference's paddle robot; `car.fm:780-801`) | an upshift to gear 2 or above, `shiftCut` while `shiftT < 0.06 x tk` | 0.05 (`car.fm:853`), every step | 60 ms; 150 ms on the truck (drive 2, tk 2.5) |
| car.model 1, manual in paddle mode | an upshift, while t < 0.06 (`gearbox.fm:232`) | `shiftTime` (default 0.15, range 0.05 to 1), every step (`car.fm:852`) | 60 ms |
| car.model 1, sequential (the dog box) | an upshift, while t < 0.06 (`:268`); the dogs out at once and the next in at 25 ms | `shiftTime`, every step | 60 ms |
| the bike | an upshift with the quickshifter, while t < `cutTime` (`bike.fm:231`), `cutTime` = `quickshiftCut` (`bike.fm:468`; `bspec.fm:66`, default 0.05, range 0.02 to 0.12) | `quickshiftCut`, every step (`car.fm:850`) | `quickshiftCut` |

No cut is asked:

- car.model 1, manual: a downshift (the blip is a speed, while t < 0.21) and the H-pattern lever (`:199`).
- car.model 1, sequential: a downshift (`:279`; the blip while t < 0.1).
- The dual clutch (`:294`): the torque is handed over across `shiftTime`, with a blip on a downshift.
- The automatic and the CVT (`:336`), and the truck's Roadranger and its air shift.
- The paddle robot's downshift, which raises the pedal to the new gear's speed (`car.fm:792-796`).
- The bike's downshift (the auto-blipper).
- A launch or a flat shift (`launch.fm`'s own).

No asker asks for one step only. The shortest window is the bike's floor, 0.02 s: three or more steps at 48 kHz.

**A second user-visible effect.** The bike's field is the cut's own length, and the seam hands it while that same
length runs, so today the bike gets twice its setting: 50 ms becomes about 100. Under (b) it gets its setting.

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

- Each kind's cut equals what that kind wants within one control step, measured on `ecu.flags`: 60 ms (the paddle
  robot; model 1's manual and sequential), 150 ms (the truck), the bike's `quickshiftCut` as set (no longer doubled).
- The guard test: every asker's window lasts at least two ECU steps.
- Model 0 unchanged to the bit.
- Every archetype's tables reproduced by the bake: no table reads a shift.
- The V8's upshift row 0 of 17, and the four's X-030 stated as a rate against its 12 of 17, on X's spread instrument.
- The downshift's blip unchanged, or its change predicted.
- N1's dips test and P's idle gate untouched.

**Read after item 17 (the lead, 26 Nov 12:33; this replaces the 10:36 order).** G's valve solve (item 17, X-051:
a warm-started secant on one continuous equation, 38.3 ns a call against today's 50.5, the chatter counter at 0)
comes first among the window's sim changes that move readings, because it moves every reference and every nightly
bound. So item 17 lands first; then item 16, with its predictions restated on that tree before its code: the V8's
upshift row and the four's rate. The 53 ms probe's figures above were taken on today's valve: they are a guide, not
the acceptance. The clips with both fixes, shown on 26 Nov, stand as what the window is meant to deliver. X-044 is
not carried by X-051 and stays with item 8, whole.

**First evidence on shipped builds (26 Nov 15:32; X's per-event spread, 17 copies at 293.15 +- 0.15 K; probe trees
only).** The three shipped boosted builds run as built on the pull. Their upshifts are X-048's rows, and their cut is
this fault's: 210 ms on the flags, 60 ms with the cut held only while the vehicle asks.

| build, event | tip | the cut probe | both fixes (with item 17's valve) |
|---|---|---|---|
| the three-cylinder 1.0 turbo, the 1-2 | 6 of 17 (bound 2.84) | 0 of 17 (2.84) | 0 of 17 (3.51) |
| the three-cylinder 1.0 turbo, the 2-3 | 8 of 17 | 0 of 17 | 0 of 17 |
| the kei 660, the 1-2 | 2 of 17 (2.77) | 0 of 17 (2.77) | 0 of 17 |
| the boxer four | 0 of 17 everywhere | 0 of 17 everywhere | 0 of 17 everywhere |

- The three-cylinder's medians at its upshifts fall from 2.62 and 2.83 to 1.26 and 1.30.
- Its first firing back over the next four falls from 2.65 and 2.82 to 1.12 and 1.07: level with the next firings.
- The steward's prediction (0 to 1 of 17 on both trees) was met, at 0.
- By the evidence rule, the rows are cured on the cut-probe tree. They stay in Known.h until this item's change is on
  the branch and the nightly reads them on the real code.

**The V8 (fuel+ecu) on the cut probe (X, 26 Nov 17:27; 17 copies at +-0.15 K, per event, the nightly bound 3.91,
which the probe does not move):**

| upshift | tip | the cut probe |
|---|---|---|
| the 1-2 | 4 of 17 (highest 4.78) | 1 of 17 (4.03, at dT -0.03 K) |
| the 2-3 | 4 of 17 (4.66) | 0 of 17 (3.35) |
| the 3-2 | 1 of 17 (4.73) | 0 of 17 (3.29) |

Not CURED as a row: one copy stays over at the 1-2. The steward's worst-of-run reading (0 of 15 at +-0.007 K) is
consistent with that: a narrower nudge. On G's valve stand-in, the reference is quieter and the bound falls to 2.58,
and the same row reads 11, 11 and 6 of 17. So item 16's V8 acceptance is read on the real code after item 17,
against that tree's own bound, and is not assumed from these figures.

## 5. What it does not cure

On the probe with the cut at the reference's length, the four's first firing back still reads 1.30 against the
reference's 1.16, and 4 of 15 copies remain over. A cut of any length purges the residual by its second intake. What
is left is the burn of a purged, full charge (real engines do it too: a torque step on recovery, sourced; its size is
not), together with the judge's window straddling the cut, which is X's to restate. The owner's ears decide the
size, on the six clips shown on 26 Nov.
