# The quarter mile, one change at a time

*Run 24 Nov from 20:38 to 22:15 UTC, at `6932a88` (`/tmp` worktree; tools built there): the lead's five run changes of 19:36, each measured alone against a baseline, then all together. 15 archetypes: those that missed the quarter mile's time or trap speed in table G, plus three near it. Times and traps by `content/validation/archetypes/quarter_mile.py` (renders of the car's own drag run, the upshifts placed by iterated renders), judged against today's targets (`content/validation/archetypes/*.json`; bands from Hale's rule with each kind's published power). Bold: outside its time or trap band.*

The runs:
- **Q0**: table G's own drag scenarios (upshifts at fixed times placed on the old launch, the pedal floored at the green from idle), tyres at the air's temperature; builds without launch control, the rotary's clutch or the V12's limited-slip.
- **Q1**: upshifts placed again, each where the next gear's wheel torque at the same road speed is the greater, from the engine's predicted full-load curve (97 % of the redline where it never is).
- **Q2**: Q1 with the NA rotary's clutch sized at 1.5 times its measured peak (350 N m) instead of the car's estimate.
- **Q3**: Q1 with the revs held at the green (the brake held, the pedal floored 1.5 s before; the launch button where the car has launch control, now every dual clutch and the rally car).
- **Q4**: Q1 on warm tyres (card C's `warmTyres`: each tyre starts in the middle of its window).
- **Q5**: Q1 with the V12's limited-slip rear (100 N m preload, torque bias 0.5).
- **QA**: all of them (the timed runs as now written).

| Archetype | Should be | Q0 | Q1 | Q2 | Q3 | Q4 | Q5 | QA |
|---|---|---|---|---|---|---|---|---|
| air-cooled-flat-six-36 | 12.92 to 15.63 s, 148 to 171 km/h | **13.29 s, 173** | **13.12 s, 175** |  | **13.41 s, 174** | **13.12 s, 175** |  | **13.41 s, 174** |
| big-block-74-pushrod-v8 | 13.53 to 16.38 s, 142 to 163 km/h | **16.90 s, 152** | **16.90 s, 152** |  | **17.23 s, 151** | **17.06 s, 152** |  | **17.40 s, 151** |
| big-bore-70-pushrod-v8 | 10.34 to 12.52 s, 185 to 213 km/h | 12.24 s, 202 | 12.09 s, 207 |  | 11.93 s, 208 | 11.55 s, 209 |  | 11.39 s, 210 |
| flat-plane-v8-39-twin-turbo | 9.57 to 11.59 s, 200 to 231 km/h | 11.40 s, 225 | 11.28 s, 226 |  | 11.12 s, 225 | 10.73 s, 229 |  | 10.81 s, 226 |
| flat-plane-v8-45 | 10.07 to 12.19 s, 190 to 219 km/h | 12.14 s, 201 | 12.01 s, 202 |  | 11.75 s, 201 | 11.70 s, 203 |  | 11.46 s, 202 |
| four-cam-v8-50 | 11.48 to 13.89 s, 167 to 193 km/h | 13.38 s, 183 | 13.08 s, 187 |  | 12.95 s, 187 | 12.96 s, 188 |  | 12.84 s, 188 |
| pickup-diesel-67-inline-six | 14.92 to 18.07 s, 129 to 148 km/h | **22.59 s, 104** | **22.59 s, 104** |  | **23.46 s, 103** | **22.59 s, 104** |  | **23.46 s, 103** |
| rotary-twin-13 | 12.28 to 14.87 s, 156 to 180 km/h | 14.43 s, 163 | **14.64 s, 139** |  | **14.69 s, 139** | **16.15 s, 126** |  | **14.52 s, 139** |
| rotary-twin-na-side-port | 12.78 to 15.47 s, 150 to 173 km/h | **19.85 s, 98** | **18.78 s, 102** | 13.77 s, 170 | **18.19 s, 104** | **18.91 s, 102** |  | 13.49 s, 170 |
| supercharged-v8-62 | 10.94 to 13.24 s, 175 to 202 km/h | 12.98 s, 191 | 12.77 s, 197 |  | 12.73 s, 197 | 12.72 s, 197 |  | 12.68 s, 197 |
| two-stage-cam-20 | 12.52 to 15.15 s, 153 to 176 km/h | **15.41 s, 148** | **15.43 s, 146** |  | **14.96 s, 146** | **15.43 s, 146** |  | **14.95 s, 146** |
| v12-65 | 9.64 to 11.66 s, 199 to 229 km/h | **12.10 s, 212** | **14.03 s, 169** |  | **14.43 s, 168** | **13.20 s, 173** | **13.95 s, 169** | **13.70 s, 170** |
| diesel-20-common-rail | 15.37 to 18.61 s, 125 to 144 km/h | 17.10 s, 143 | **15.78 s, 147** |  | **15.51 s, 147** | **15.78 s, 147** |  | **15.51 s, 147** |
| small-block-57-carb | 14.71 to 17.81 s, 130 to 150 km/h | **14.70 s, 161** | **14.65 s, 161** |  | **14.58 s, 161** | **14.65 s, 161** |  | **14.58 s, 161** |
| twin-cam-roadster-18 | 14.98 to 18.14 s, 128 to 147 km/h | **15.72 s, 148** | **15.70 s, 148** |  | **15.31 s, 148** | **15.70 s, 148** |  | **15.31 s, 148** |
| *missed (of those run)* | | 8 | 10 | 0 | 10 | 10 | 1 | 9 |

## What each change did

- **The clutch (Q2)** is the NA rotary's whole miss: 18.78 s at 102 km/h to 13.77 s at 170 (both in): the car's own clutch estimate slipped under the engine's real torque after the launch. One change, one car, both rows in.
- **Upshifts from the power curve (Q1)** help the cars whose curves are smooth (the 3.9, the 4.5, the four-cam, the 7.0, the supercharged V8: 0.1 to 0.3 s each) and hurt two badly: the V12 (12.10 to 14.03 s, trap 212 to 169 km/h) and the twin-turbo rotary (trap 163 to 139). Their predicted curves carry the model's low tooth (table I: the V12's torque peak at 2049 rpm against 5500 to 7000), and a shift rule that believes the curve shifts too early. That is a model finding (N1's: the curves' sharpest points), not the run's; the method is how a real timed run is shifted, and it stays.
- **The revs held (Q3)** gain 0.1 to 0.5 s on most cars (the 4.5 12.01 to 11.75, the roadster 15.70 to 15.31, the two-stage 15.43 to 14.96) and cost the automatics 0.3 to 0.9 s (the big block 16.90 to 17.23, the pickup 22.59 to 23.46): a brake-torque launch against these converters bogs them; card I's.
- **Warm tyres (Q4)** gain where grip limits: the V12 14.03 to 13.20, the 3.9 11.28 to 10.73, the 7.0 12.09 to 11.55; little elsewhere. (They were measured after card I's start fix, so they show the window, not the 205 K start.)
- **The limited-slip rear (Q5)**: the V12 14.03 to 13.95, small: its open rear was not what held it.
- **All together (QA)**: 9 of these 15 miss, against 8 in G's runs. What remains is not the run:
  - **trap over its band** on the flat six, the small block, the roadster and the 2.0 diesel: their engines over-deliver (table I: the flat six 228 kW for 180 to 195, the small block 194 for 120 to 160): N3's;
  - **the pickup diesel** 23.5 s at 103 km/h: its automatic's launch and its torque converter (card I) and the heavy truck tyres;
  - **the V12 and the twin-turbo rotary**: their curves' low teeth through the shift rule (N1), and the rotary's boost collapse at its power peak (N2);
  - **the two-stage and the big block**: their engines under their bands (the two-stage's top end, N3-4; the big block's automatic launch).

The timed runs in `content/validation/scenarios/qm/` are QA's method, regenerated on each table's pin (`quarter_mile.py --shift power --launch prerev --tyres warm`).
