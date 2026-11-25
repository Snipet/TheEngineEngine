# DYNO (`ui/views/dyno/`)

*Package U4. The big version of the dyno ribbon; no ribbon here.*

## What the stage shows

- **The big plot.** Torque (left axis) and power (right axis) against engine speed, in the user's units, five shared
  grid divisions, each axis's step the round one that holds its peak. Torque is a heavy stroke, power a light one
  (the legend says so). The peaks are discs; the lead curve's carry their numbers. The lead curve is the selected
  row, else the predicted one; its peaks are also the big readouts at the top right.
- **The cursor.** Over the plot or the band: a line at the pointer's speed and a card reading every shown curve there
  (torque, power and the third quantity), drawn low in the plot where the curves leave room.
- **The third trace.** A band under the plot sharing its speed axis: none, boost, lambda or advance (buttons in its
  title row). A quantity no shown run recorded is a dash and a sentence naming the telemetry field, never a line.
- **Curves and runs.** Predicted (the build as it is: accent, solid), before the last edit (grey, short dash), then
  each run (cold, green, amber, hot, ... with long dash, dot-dash, dash, dots in turn): colour and dash differ, never
  colour alone. Each row has a box to show or hide it, a sample of its two strokes, its name and its peaks. The newest
  runs are listed when they do not all fit, with a count of those left out.
- **A run.** The plan (engine or chassis; a sweep at a rate, or steps every 500 rpm each held for a dwell; from, to,
  pedal; the gear for a chassis run), START RUN and STOP. While a run goes it draws itself from the runner's points
  (`DynoSource::runSoFar()`) in the stroke it will keep, the span swept so far shaded, and the plan's box says where it
  is in words with a bar; when it lands it is chosen and its peaks are said.

- **Exhaust walls.** A run says whether its exhaust walls were settled at each step (`DynoTrace::settled`, card C's
  `DynoCurve::settled`, the lead's ruling 1220ca3): "walls settled" or "cold walls" (amber too) at the right of its
  name, and the same on the first line of its CSV (`# exhaust cold walls`). When the runs shown are of both kinds a
  note over the plot says so ("MIXED: COLD AND SETTLED WALLS"): a cold-wall step reads a few percent low, so part of
  their difference is not the engine's. A run that does not say (a sweep, a prediction) shows nothing. A stepped run's plan has SETTLE (lit: settle the
  walls at each step, card C's default; `DynoRequest::settleExhaust`).

- **Knock-limited points.** Where card C says knock control's retard reached half a degree (`DynoTrace::knock`,
  `retardMin`, `retardMax`; that point's torque is a mean over the retard's sawtooth): an amber ring on the torque
  curve and a tick under the plot, "KNOCK-LIMITED" in the legend, and under the curve's line in the cursor's card its
  words ("knock-limited, retard 5 to 9 deg"). The CSV adds `knock_limited,retard_min_deg,retard_max_deg`. Points are
  read by speed, so card C's upward stepped runs need nothing else.

- **Rev-limited points.** A point the rev limiter acted on (`DynoTrace::limited`, card C's `PredictPoint::limited`) is
  the cut's torque, no measure of the engine: the curve breaks there and the point is a dim ring alone, "LIMITER, NOT
  MEASURED" in the legend (or between the axis titles when the legend's line is full); the peaks leave it out; the CSV
  adds `rev_limited`.

## What is clickable

A row (chooses it: the header's readouts, the NAME field and COPY CSV follow it); a row's box (shows or hides it);
NAME (renames a run: Enter keeps it; the predicted and before curves are not renamable); SAVE CSV through the frame's save service, or COPY CSV to the clipboard
without one (the chosen curve as `speed_rad_s,rpm,torque_N_m,power_kW,<fields>`); the band's four buttons; the plan's buttons and
steppers.

## What is derived

Nothing is invented: every number is a `DynoTrace`'s. The plan is this workspace's state, not a Spec field, so it is on the stage
and `sections()` is empty.

Tests: `ui.dyno.stage` (goldens dyno.stage, .large, .paper), `ui.dyno.run`, `ui.dyno.absent`, `ui.dyno.text`, `ui.dyno.walls`, `ui.dyno.knock`; nightly: `ui.dyno.sheet.pictures`, `ui.dyno.framestages.pictures`.

## The fuel

A run is named with the grade it ran on ("Engine 2, Regular (87)") when the build has `fuel.grade` under the Fuels
model, so runs on two fuels stay side by side in the list (held for the session: DYNO keeps runs until quit, it does
not save them). When the predicted curve and the one before the last edit were made on different grades, their rows
say so ("Predicted, Race", "Before, Premium (91)"); the grade is the one in the build when a curve came in. Test:
`ui.dyno.fuel`.
