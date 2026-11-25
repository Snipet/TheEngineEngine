# The knock-limit table on the naturally aspirated archetypes (table L's second pass)

Run 2026-11-25-0620 UTC by `content/builds/archetypes/knockpass.py` on the tools at e43e228b, the builds as committed in 1efa3282 (the naturally aspirated ones unchanged since). Each build baked in the app's order (`CalibrationBakeService`: `tee-dyno --bake-spark --save`; on a phased build `--bake-cam --save` on that, then `--bake-spark --save` again) and measured against its cams-only form: the engine dyno held at full throttle from 2 s at 0.35 to 0.95 of the redline, the means from 8 to 10 s; warm idle (p-idle-warm, 8 to 14 s). C's proof's conditions: retard under 3 deg, torque within 2 %, no more advance asked; and N1's 0.5 % floor.

**Findings.** Every build baked and idles as before (speed, spread and manifold unchanged). The knock retard stays under 0.6 deg everywhere and no build asks more advance (a little less low down on several). The torque moves only where the build is phased and the cam is re-baked on the spark: **over 2 %** the economy four (+2.9 % at 2300 rpm) and the 3.5 V6 (+3.1 % at 2350) and the flat-plane 4.5 (-2.7 % at 8550); over 0.5 % the four-cam 5.0 (-1.2, -1.8 %), the S54 (+0.8), the V12 (+0.8), the flat-plane (-0.9 at 7200) and the economy four and V6 at their other points. The unphased builds move under 0.2 %. So the spark table itself is near-neutral on these engines (none is knock-limited at full load); the moves are the cam's, baked on the spark-applied build where table L's maps were baked without it (cams.py; the app's order bakes it on the spark).

| archetype | rpm | torque off / on N m | change % | retard off / on | ask off / on | marks |
|---|---|---|---|---|---|---|
| air-cooled-flat-six-36 | 2350 | 340.6 / 340.0 | -0.2 | 0.0 / 0.0 | 14.1 / 13.2 |  |
| air-cooled-flat-six-36 | 3350 | 329.6 / 329.5 | -0.0 | 0.0 / 0.0 | 17.0 / 17.0 |  |
| air-cooled-flat-six-36 | 4350 | 360.3 / 360.7 | +0.1 | 0.0 / 0.0 | 19.3 / 19.3 |  |
| air-cooled-flat-six-36 | 5350 | 366.9 / 367.4 | +0.1 | 0.0 / 0.0 | 21.2 / 21.2 |  |
| air-cooled-flat-six-36 | 6350 | 326.4 / 326.6 | +0.0 | 0.0 / 0.0 | 23.0 / 23.0 |  |
| big-block-74-pushrod-v8 | 1950 | 547.4 / 547.4 | +0.0 | 0.0 / 0.0 | 13.1 / 13.1 |  |
| big-block-74-pushrod-v8 | 2800 | 564.8 / 564.8 | +0.0 | 0.0 / 0.0 | 15.8 / 15.8 |  |
| big-block-74-pushrod-v8 | 3650 | 554.1 / 554.1 | +0.0 | 0.0 / 0.0 | 18.0 / 18.0 |  |
| big-block-74-pushrod-v8 | 4500 | 530.0 / 530.0 | +0.0 | 0.0 / 0.0 | 19.9 / 19.9 |  |
| big-block-74-pushrod-v8 | 5300 | 470.0 / 470.0 | +0.0 | 0.0 / 0.0 | 21.5 / 21.5 |  |
| big-bore-70-pushrod-v8 | 2450 | 580.1 / 580.1 | +0.0 | 0.0 / 0.0 | 12.9 / 12.9 |  |
| big-bore-70-pushrod-v8 | 3500 | 613.3 / 613.3 | +0.0 | 0.0 / 0.0 | 15.5 / 15.5 |  |
| big-bore-70-pushrod-v8 | 4550 | 584.3 / 584.3 | +0.0 | 0.0 / 0.0 | 17.7 / 17.7 |  |
| big-bore-70-pushrod-v8 | 5600 | 598.8 / 598.8 | +0.0 | 0.0 / 0.0 | 19.5 / 19.5 |  |
| big-bore-70-pushrod-v8 | 6650 | 580.5 / 580.5 | +0.0 | 0.0 / 0.0 | 21.2 / 21.2 |  |
| economy-four-15 | 2300 | 131.6 / 135.4 | +2.9 | 0.5 / 0.6 | 11.5 / 11.5 | torque > 2 %, > 0.5 % |
| economy-four-15 | 3300 | 147.2 / 146.8 | -0.3 | 0.0 / 0.0 | 13.9 / 13.9 |  |
| economy-four-15 | 4300 | 138.4 / 138.4 | +0.0 | 0.0 / 0.0 | 15.9 / 15.9 |  |
| economy-four-15 | 5300 | 146.3 / 145.0 | -0.9 | 0.0 / 0.0 | 17.6 / 17.6 | > 0.5 % |
| economy-four-15 | 6250 | 131.0 / 128.9 | -1.6 | 0.0 / 0.0 | 19.0 / 19.0 | > 0.5 % |
| flat-plane-v8-45 | 3150 | 397.4 / 397.2 | -0.1 | 0.0 / 0.0 | 14.8 / 14.8 |  |
| flat-plane-v8-45 | 4500 | 439.3 / 439.8 | +0.1 | 0.0 / 0.0 | 17.7 / 17.7 |  |
| flat-plane-v8-45 | 5850 | 470.1 / 468.9 | -0.3 | 0.0 / 0.0 | 20.0 / 20.0 |  |
| flat-plane-v8-45 | 7200 | 449.0 / 444.8 | -0.9 | 0.0 / 0.0 | 22.0 / 22.0 | > 0.5 % |
| flat-plane-v8-45 | 8550 | 380.0 / 369.7 | -2.7 | 0.0 / 0.0 | 23.8 / 23.8 | torque > 2 %, > 0.5 % |
| four-cam-v8-50 | 2600 | 452.7 / 454.0 | +0.3 | 0.0 / 0.0 | 12.6 / 12.0 |  |
| four-cam-v8-50 | 3750 | 486.2 / 485.9 | -0.1 | 0.0 / 0.0 | 15.2 / 15.2 |  |
| four-cam-v8-50 | 4850 | 468.7 / 463.1 | -1.2 | 0.0 / 0.0 | 17.3 / 17.3 | > 0.5 % |
| four-cam-v8-50 | 6000 | 489.0 / 480.1 | -1.8 | 0.0 / 0.0 | 19.2 / 19.2 | > 0.5 % |
| four-cam-v8-50 | 7100 | 462.3 / 462.3 | +0.0 | 0.0 / 0.0 | 20.8 / 20.8 |  |
| hot-hatch-16-16v | 2500 | 138.1 / 138.1 | +0.0 | 0.0 / 0.0 | 14.3 / 14.3 |  |
| hot-hatch-16-16v | 3600 | 134.1 / 134.1 | +0.0 | 0.0 / 0.0 | 17.1 / 17.1 |  |
| hot-hatch-16-16v | 4700 | 149.9 / 149.9 | +0.0 | 0.0 / 0.0 | 19.4 / 19.4 |  |
| hot-hatch-16-16v | 5750 | 148.6 / 148.6 | +0.0 | 0.0 / 0.0 | 21.3 / 21.3 |  |
| hot-hatch-16-16v | 6850 | 147.8 / 147.8 | +0.0 | 0.0 / 0.0 | 23.0 / 23.0 |  |
| pushrod-v6-38 | 2000 | 334.9 / 334.9 | +0.0 | 0.0 / 0.0 | 13.0 / 13.0 |  |
| pushrod-v6-38 | 2850 | 339.0 / 339.0 | +0.0 | 0.0 / 0.0 | 15.6 / 15.6 |  |
| pushrod-v6-38 | 3700 | 316.8 / 316.8 | +0.0 | 0.0 / 0.0 | 17.8 / 17.8 |  |
| pushrod-v6-38 | 4550 | 311.6 / 311.6 | +0.0 | 0.0 / 0.0 | 19.7 / 19.7 |  |
| pushrod-v6-38 | 5400 | 254.1 / 254.1 | +0.0 | 0.0 / 0.0 | 21.3 / 21.3 |  |
| rotary-twin-na-side-port | 3150 | 210.7 / 210.7 | +0.0 | 0.0 / 0.0 | 15.1 / 15.1 |  |
| rotary-twin-na-side-port | 4500 | 219.4 / 219.4 | +0.0 | 0.0 / 0.0 | 18.0 / 18.0 |  |
| rotary-twin-na-side-port | 5850 | 221.6 / 221.6 | +0.0 | 0.0 / 0.0 | 20.4 / 20.4 |  |
| rotary-twin-na-side-port | 7200 | 221.9 / 221.9 | +0.0 | 0.0 / 0.0 | 22.6 / 22.6 |  |
| rotary-twin-na-side-port | 8550 | 225.1 / 225.1 | +0.0 | 0.0 / 0.0 | 24.5 / 24.5 |  |
| small-block-57-carb | 1950 | 442.7 / 442.7 | +0.0 | 0.0 / 0.0 | 13.7 / 13.7 |  |
| small-block-57-carb | 2750 | 445.8 / 445.8 | +0.0 | 0.0 / 0.0 | 16.4 / 16.4 |  |
| small-block-57-carb | 3600 | 429.4 / 429.4 | +0.0 | 0.0 / 0.0 | 18.6 / 18.6 |  |
| small-block-57-carb | 4400 | 407.7 / 407.7 | +0.0 | 0.0 / 0.0 | 20.5 / 20.5 |  |
| small-block-57-carb | 5250 | 366.5 / 366.5 | +0.0 | 0.0 / 0.0 | 22.2 / 22.2 |  |
| straight-six-32-high-rev | 2800 | 309.4 / 311.9 | +0.8 | 0.0 / 0.0 | 12.9 / 12.5 | > 0.5 % |
| straight-six-32-high-rev | 4000 | 294.1 / 294.0 | -0.0 | 0.0 / 0.0 | 15.6 / 15.6 |  |
| straight-six-32-high-rev | 5200 | 289.2 / 289.2 | +0.0 | 0.0 / 0.0 | 17.8 / 17.8 |  |
| straight-six-32-high-rev | 6400 | 318.9 / 319.4 | +0.1 | 0.0 / 0.0 | 19.7 / 19.7 |  |
| straight-six-32-high-rev | 7600 | 289.3 / 288.0 | -0.5 | 0.0 / 0.0 | 21.3 / 21.3 |  |
| twin-cam-roadster-18 | 2450 | 169.2 / 169.0 | -0.1 | 0.0 / 0.0 | 13.2 / 12.6 |  |
| twin-cam-roadster-18 | 3500 | 160.8 / 160.6 | -0.1 | 0.0 / 0.0 | 15.8 / 15.8 |  |
| twin-cam-roadster-18 | 4550 | 177.7 / 177.9 | +0.1 | 0.0 / 0.0 | 17.9 / 17.9 |  |
| twin-cam-roadster-18 | 5600 | 170.0 / 169.8 | -0.1 | 0.0 / 0.0 | 19.8 / 19.8 |  |
| twin-cam-roadster-18 | 6650 | 160.5 / 160.5 | -0.0 | 0.0 / 0.0 | 21.4 / 21.4 |  |
| two-stage-cam-20 | 3150 | 194.1 / 194.0 | -0.0 | 0.0 / 0.0 | 14.4 / 14.4 |  |
| two-stage-cam-20 | 4500 | 186.3 / 186.6 | +0.1 | 0.0 / 0.0 | 17.1 / 17.1 |  |
| two-stage-cam-20 | 5850 | 180.5 / 180.4 | -0.0 | 0.0 / 0.0 | 19.4 / 19.4 |  |
| two-stage-cam-20 | 7200 | 189.7 / 189.5 | -0.1 | 0.0 / 0.0 | 21.4 / 21.4 |  |
| two-stage-cam-20 | 8550 | 191.3 / 191.2 | -0.0 | 0.0 / 0.0 | 23.1 / 23.1 |  |
| v10-52 | 3050 | 444.3 / 443.1 | -0.3 | 0.0 / 0.0 | 12.7 / 12.4 |  |
| v10-52 | 4350 | 445.1 / 444.5 | -0.1 | 0.0 / 0.0 | 15.3 / 15.3 |  |
| v10-52 | 5650 | 520.1 / 519.9 | -0.0 | 0.0 / 0.0 | 17.4 / 17.4 |  |
| v10-52 | 6950 | 506.9 / 504.6 | -0.5 | 0.0 / 0.0 | 19.3 / 19.3 |  |
| v10-52 | 8250 | 435.1 / 434.4 | -0.2 | 0.0 / 0.0 | 20.9 / 20.9 |  |
| v12-65 | 3100 | 589.3 / 588.9 | -0.1 | 0.0 / 0.0 | 14.7 / 14.7 |  |
| v12-65 | 4450 | 543.8 / 548.3 | +0.8 | 0.0 / 0.0 | 17.6 / 17.6 | > 0.5 % |
| v12-65 | 5800 | 705.6 / 706.2 | +0.1 | 0.0 / 0.0 | 20.0 / 20.0 |  |
| v12-65 | 7100 | 684.7 / 683.5 | -0.2 | 0.0 / 0.0 | 22.0 / 22.0 |  |
| v12-65 | 8450 | 628.8 / 629.5 | +0.1 | 0.0 / 0.0 | 23.8 / 23.8 |  |
| v6-35-na | 2350 | 330.6 / 340.7 | +3.1 | 0.6 / 0.2 | 13.4 / 13.4 | torque > 2 %, > 0.5 % |
| v6-35-na | 3350 | 331.2 / 328.2 | -0.9 | 0.0 / 0.0 | 16.1 / 16.1 | > 0.5 % |
| v6-35-na | 4350 | 327.9 / 327.9 | +0.0 | 0.0 / 0.0 | 18.3 / 18.3 |  |
| v6-35-na | 5350 | 307.7 / 307.7 | +0.0 | 0.0 / 0.0 | 20.2 / 20.2 |  |
| v6-35-na | 6350 | 271.5 / 271.5 | +0.0 | 0.0 / 0.0 | 21.8 / 21.8 |  |
| vr6-28 | 2250 | 245.9 / 245.9 | +0.0 | 0.0 / 0.0 | 11.8 / 11.8 |  |
| vr6-28 | 3250 | 242.8 / 242.8 | +0.0 | 0.0 / 0.0 | 14.5 / 14.5 |  |
| vr6-28 | 4200 | 243.1 / 243.1 | +0.0 | 0.0 / 0.0 | 16.5 / 16.5 |  |
| vr6-28 | 5200 | 224.9 / 224.9 | +0.0 | 0.0 / 0.0 | 18.3 / 18.3 |  |
| vr6-28 | 6150 | 189.8 / 189.8 | +0.0 | 0.0 / 0.0 | 19.8 / 19.8 |  |

| archetype | idle off: rpm, sd, kPa | idle on: rpm, sd, kPa | bake |
|---|---|---|---|
| air-cooled-flat-six-36 | 850, 5.5, 35.6 | 850, 5.2, 35.4 | spark: ok |
| big-block-74-pushrod-v8 | 650, 4.9, 57.7 | 650, 4.9, 57.7 | spark: ok |
| big-bore-70-pushrod-v8 | 750, 3.8, 38.9 | 750, 3.8, 38.9 | spark: ok |
| economy-four-15 | 750, 6.2, 29.7 | 750, 6.2, 29.7 | spark: ok; cam: ok idle with the map: warm 750 rpm (asked 750), sd 6.1; cold 994 rpm, sd 4.7, running; spark again: ok |
| flat-plane-v8-45 | 1000, 2.5, 44.0 | 1000, 2.5, 43.9 | spark: ok; cam: ok idle with the map: warm 1000 rpm (asked 1000), sd 2.4; cold 1243 rpm, sd 3.1, running; spark again: ok |
| four-cam-v8-50 | 700, 3.1, 34.7 | 700, 3.1, 34.7 | spark: ok; cam: ok idle with the map: warm 700 rpm (asked 700), sd 3.1; cold 947 rpm, sd 3.3, running; spark again: ok |
| hot-hatch-16-16v | 850, 5.6, 32.3 | 850, 5.6, 32.3 | spark: ok |
| pushrod-v6-38 | 650, 5.1, 29.7 | 650, 5.1, 29.7 | spark: ok |
| rotary-twin-na-side-port | 800, 6.2, 26.0 | 800, 6.2, 26.0 | spark: ok |
| small-block-57-carb | 700, 2.7, 39.1 | 700, 2.7, 39.1 | spark: ok |
| straight-six-32-high-rev | 850, 3.1, 37.5 | 850, 3.1, 37.5 | spark: ok; cam: ok idle with the map: warm 850 rpm (asked 850), sd 3.0; cold 1096 rpm, sd 3.1, running; spark again: ok |
| twin-cam-roadster-18 | 850, 5.1, 28.9 | 850, 5.1, 28.7 | spark: ok |
| two-stage-cam-20 | 900, 5.4, 30.5 | 900, 5.3, 30.3 | spark: ok |
| v10-52 | 900, 1.6, 34.9 | 900, 1.6, 34.6 | spark: ok; cam: ok idle with the map: warm 900 rpm (asked 900), sd 1.6; cold 1143 rpm, sd 3.2, running; spark again: ok |
| v12-65 | 900, 1.2, 43.1 | 900, 1.3, 42.8 | spark: ok; cam: ok idle with the map: warm 900 rpm (asked 900), sd 1.4; cold 1144 rpm, sd 2.2, running; spark again: ok |
| v6-35-na | 700, 4.8, 27.7 | 700, 4.7, 27.8 | spark: ok; cam: ok idle with the map: warm 700 rpm (asked 700), sd 4.7; cold 946 rpm, sd 3.8, running; spark again: ok |
| vr6-28 | 750, 3.9, 29.0 | 750, 3.9, 29.0 | spark: ok |
