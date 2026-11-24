# Card D2's listening scenarios

Engine scenarios (`tee-engine-scenario/1`) for the owner's listening pack, each naming its build (`content/builds/`) and
its air (300 K, 101325 Pa, but the cold one). Render, e.g.:
`tee-render --engine --build truck-15l-six --scenario tests/sim/diesel/listen/d2-jake-muffled.json`.

| File | What to listen for |
|---|---|
| d2-jake-muffled | the compression brake at 1900 rpm as on a grade: off, then Low, Medium, High (2, 4, 6 cylinders): the bark at the firing frequency (95 Hz) growing by stages; its fall in pitch as the speed drops from 14 s |
| d2-jake-straight | the same, straight-piped: the sharp edge of each release (measured: the harmonics of the firing frequency some 25 dB over motoring) |
| d2-jake-flap | High, then the exhaust flap as well (3 bar), then the flap alone: a duller drone |
| d2-cold-mechanical | a mechanical-pump six's cold start at 253 K and its cold idle: the clatter, the governor's slow hunt (tens of rpm) |
| d2-pilot | full load at 1200 rpm: the combustion clatter, then softer with a 4 % pilot (rate of rise 11 to 5 bar/deg measured), then with later injection |
| d2-turbo-whistle | a big single turbo's slow spool from 5 s (59 kPa after 2 s measured), then variable vanes' quick one from 15 s |
| d2-eighteen-pull | the 36 t rig on the eighteen-speed: away in 3 on the clutch, the splitter to 4 at 14 s (a lever shift needs a closed-loop foot: EighteenTest.cpp) |
| d2-pull-away | the 36 t rig pulling away and up the paddles' gears; a 6 % grade from 40 s (the shifts are timed, not driven: some may fall at odd revs) |

Not mixed yet, so not in these: the eighteen-speed's grind and the air system's purge and spring brakes
(`sim/vehicle/roadranger.fm`, `truckair.fm`, tested alone). Nobody here has heard any of it.
