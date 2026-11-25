# Published compressor maps, read into numbers

Read 2026-11-24, 23:40 to 23:52 UTC, by a research agent of the lead's; one map spot-checked by the lead against
the image (the GT2860RS: surge at pressure ratios 1.5, 2.0 and 2.5 and the 60 % contour at 2.0 agree with the table
within 0.3 lb/min). Why: on the model's one family of compressor map the surge line lies under the engine's operating
line at low speed (N2, the 2.0 common-rail at 1800 rpm: the surge-line ratio at 0.04 kg/s is about 1.1), and no one
match holds both ends of the turbo four (N3, `causes.md` B-3). The question this answers: how wide is a real map.

**Basis of every figure:** *read from image* unless marked *(stated)* for the maker's own text or *(computed)*.
Nothing here is from memory. Flows are corrected flow in lb/min with kg/s beside them (x 0.00756, computed). The
pressure ratio is as each map labels it; neither maker says total-to-total on the map. Reading uncertainty: about
+-0.5 lb/min for surge and choke on the Garrett and EFR maps, +-2 lb/min for the peak-efficiency flow, +-2 to 4 lb/min
on the S400; ratios +-0.03 to 0.05. The images are not kept in the repository; the links are.

## The answer in one line

Surge flow over choke flow (choke taken at the last efficiency contour drawn): about **0.22 to 0.26 at a pressure
ratio of 1.5, 0.26 to 0.34 at 2.0, 0.32 to 0.42 at 2.5, 0.41 to 0.47 at 3.0**. Measured against where the speed lines
go vertical (the EFR map alone shows it) it is lower still: 0.15 at 1.5. The peak-efficiency ridge runs from about 0.55
of choke at 1.5 to about 0.75 at 3.0. Near the surge line the small wheels still make 61 to 69 % against peaks of 72
to 76 %.

## The maps

1. **Garrett GT2860RS**: <https://www.garrettmotion.com/racing-and-performance/performance-catalog/turbo/gt2860rs/>.
   *(stated)* inducer 47 mm, exducer 60 mm, compressor A/R 0.60. Highest speed line 143,971 rpm: tip speed 452 m/s
   *(computed)*. Peak efficiency 76 %. The page does not say whether it has a ported shroud.
2. **Garrett GTX2860R Gen II**: the same catalogue, `/gtx2860r-gen-ii/`. *(stated)* inducer 46 mm, exducer 60 mm,
   "improved ported shroud design for surge resistance". 185,000 rpm: 581 m/s. Peak 73 %.
3. **Garrett GT2560R**: the same catalogue, `/gt2560r/`. *(stated)* inducer 46 mm, exducer 60 mm, A/R 0.80.
   144,864 rpm: 455 m/s. Peak 75 %. The map only just reaches a ratio of 2.5.
4. **BorgWarner EFR 6258**:
   <https://www.borgwarner.com/docs/default-source/iam/boosting-technologies/efr-6258-a.pdf>. *(stated)* inducer
   49 mm, exducer 62 mm, A/R 0.64 or 0.85; a recirculation valve; no ported shroud stated. Drawn as vectors, so it
   reads precisely. 168 krpm: 545 m/s. Peak 0.76. Its speed lines go vertical at 44.0 lb/min (168 krpm).
5. **BorgWarner AirWerks S400SX3** (a heavy-truck frame; a performance wheel, not an OEM truck's):
   <https://www.borgwarner.com/docs/default-source/iam/boosting-technologies/s400sx3.pdf>. *(stated)* inducer
   74.56 mm, exducer 100.17 mm, "compressor cover recirculation grooves". 105,079 rpm: 551 m/s. Peak 0.78.
   **Doubtful:** its flow axis is labelled unevenly and its ratio labels look truncated; read to +-2 to 4 lb/min.

Garrett's maps cut their speed lines at the map's edge; Garrett *(stated, Turbo Tech 103)*: the "choke line is
typically defined by the point where the efficiency drops below 58%".

## Readings (lb/min / kg/s)

| Map | PR | Surge | Peak-efficiency flow (eff.) | Choke at the last contour | Choke where speed lines go vertical | Surge/choke | Peak/choke |
|---|---|---|---|---|---|---|---|
| GT2860RS | 1.5 | 7.4 / .056 | 17.0 / .129 (76 %) | 30.0 / .227 | same | 0.25 | 0.57 |
| | 2.0 | 10.9 / .082 | 25.0 / .189 (75 %) | 33.1 / .250 (60 %) | 35.9 / .271 (map edge) | 0.33 (0.30) | 0.76 (0.70) |
| | 2.5 | 13.7 / .104 | 29.6 / .224 (72 %) | 32.6 / .246 | same | 0.42 | 0.91 |
| GTX2860R Gen II | 1.5 | 6.3 / .048 | 16.0 / .121 (~72 %) | 28.8 / .218 | same | 0.22 | 0.56 |
| | 2.0 | 9.8 / .074 | 24.2 / .183 (73 %) | 38.4 / .290 | same | 0.26 | 0.63 |
| | 2.5 | 13.8 / .104 | 26.1 / .197 (73 %) | 42.1 / .318 | same | 0.33 | 0.62 |
| | 3.0 | 16.4 / .124 | 31.0 / .234 (~72 %) | 39.6 / .299 | same | 0.41 | 0.78 |
| GT2560R | 1.5 | 6.8 / .051 | 17.1 / .129 (~75 %) | 28.1 / .212 | same | 0.24 | 0.61 |
| | 2.0 | 10.4 / .079 | 25.2 / .191 (72 %) | 34.3 / .259 | same | 0.30 | 0.73 |
| | 2.5 | 17.4 / .132 | ~29 / .22 | 31.2 / .236 | same | 0.56 (map top, doubtful) | ~0.93 |
| EFR 6258 | 1.5 | 6.7 / .051 | 15.2 / .115 (~0.70) | 29.7 / .225 | 44.0 / .333 | 0.23 (0.15) | 0.51 (0.35) |
| | 2.0 | 10.0 / .076 | 26 / .197 (0.76) | 38.7 / .293 | 44.0 / .333 | 0.26 (0.23) | 0.67 (0.59) |
| | 2.5 | 13.3 / .101 | 28.6 / .216 (~0.74) | 41.1 / .311 | 43.7 / .330 | 0.32 (0.30) | 0.70 (0.65) |
| | 3.0 | 19.0 / .144 | 31.5 / .238 (~0.74) | 41.5 / .314 | 42.9 / .324 | 0.46 (0.44) | 0.76 (0.73) |
| S400SX3 | 1.5 | 22 / .166 | 46 / .348 (0.78) | 86 / .650 | none drawn | 0.26 | 0.53 |
| | 2.0 | 32.2 / .243 | 59 / .446 (0.78) | 95.6 / .723 | none drawn | 0.34 | 0.62 |
| | 2.5 | 39.5 / .299 | 69.5 / .525 (~0.76) | 99.5 / .752 | none drawn | 0.40 | 0.70 |
| | 3.0 | 47.3 / .358 | 76 / .575 (~0.75) | 101.7 / .769 | ~102.7 | 0.47 | 0.75 |

Bracketed ratios use the choke where the speed lines go vertical.

**Efficiency just right of surge** (at surge flow plus a tenth of the choke flow; +-4 points; contours counted inward
from the labelled peak island, so approximate):

| Map | PR 1.5 | PR 2.0 |
|---|---|---|
| GT2860RS | ~69 % | ~62 % (60 to 66) |
| GTX2860R Gen II | ~62 % | ~63 % |
| GT2560R | ~69 % | ~61 % |
| EFR 6258 | ~0.65 | ~0.65 |
| S400SX3 | ~0.72 | ~0.73 |

## By class (computed from the table; choke at the last contour)

| Class | PR 1.5: surge/choke, peak/choke | PR 2.0 | PR 2.5 | PR 3.0 |
|---|---|---|---|---|
| Small petrol (four maps) | 0.22 to 0.25, 0.51 to 0.61 | 0.26 to 0.33, 0.63 to 0.76 | 0.32 to 0.42, 0.62 to 0.91 | 0.41 to 0.46, 0.76 to 0.78 |
| Heavy-truck frame (S400SX3) | 0.26, 0.53 | 0.34, 0.62 | 0.40, 0.70 | 0.47, 0.75 |
| 2 L diesel VGT | no map found | | | |

Surge flow climbs with the ratio (about x 1.45 from 1.5 to 2.0); choke at the last contour also climbs to a ratio of 2
to 2.5, so the ratio of the two rises with it. On the EFR map the physical choke stays at 43 to 44 lb/min.

## What a ported shroud changes

- Garrett, Turbo Tech 103 *(stated)*: it "functions to move the surge line further to the left ... allows a larger
  compressor to be used" and "usually has a minor negative impact on compressor efficiency". Its figure 3 overlays one
  map with and without: surge at about 0.64 of choke without and 0.48 with, in the upper middle of the map (read from a
  258 x 231 pixel image, +-0.08, low confidence).
- US 7,407,364 (Honeywell; a two-stage compressor) *(stated)*: at a ratio of 3.5 surge falls from about 68 to 47
  lb/min with choke unchanged at 124: surge/choke 0.55 to 0.38 *(computed)*; "about 45 percent" more range; the surge
  line "about the same" below its knee (a ratio of about 2.3).
- Of the five maps the GTX2860R is the one stated to be ported; it and the S400 (recirculation grooves) have no
  unported twin, so the shroud's share in their width cannot be separated.

## Not found

No published map for a 2 L diesel's VGT compressor (Garrett's catalogue pages for those models answered 404), none
for a Holset, none for the IS20 or IS38 (P looked separately), no map as a table or CSV, and no textbook or SAE
statement of typical map width against pressure ratio. Fetches refused and not retried: core.ac.uk, tara.tcd.ie,
eprints.soton.ac.uk, image-ppubs.uspto.gov, spiral.imperial.ac.uk.

## What follows for the model (the lead's reading; N2's to carry out)

The model's map is to be written in these same terms (surge, peak and choke flow at ratios 1.5 to 3.0 and the
efficiency near surge, for the turbo four's, the 2.0 common-rail's and the truck's compressors as built) and its shape
constants set so that it lies inside these ranges, each constant citing this file. A diesel VGT's compressor has no
published map here: until one is found it takes the small-petrol and truck-frame ranges, marked as that. The surge cap
on the boost target (built, held) goes in with the corrected map.
