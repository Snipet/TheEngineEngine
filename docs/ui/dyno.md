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
- **A run.** The plan (engine or chassis; from, to, rate, pedal; gear for a chassis run), START RUN and STOP. While a
  run goes the plot shades the span swept so far (from the plan's rate) and the plan's box says, in words, where it
  is and how long it should take; when it lands it is chosen and its peaks are said.

## What is clickable

A row (chooses it: the header's readouts, the NAME field and COPY CSV follow it); a row's box (shows or hides it);
NAME (renames a run: Enter keeps it; the predicted and before curves are not renamable); COPY CSV (the chosen curve as
`speed_rad_s,rpm,torque_N_m,power_kW,<fields>` to the clipboard); the band's four buttons; the plan's buttons and
steppers.

## What is derived

Nothing is invented: every number is a `DynoTrace`'s. The swept span of a run in progress is estimated from the plan
(the source gives no partial trace yet). The plan is this workspace's state, not a Spec field, so it is on the stage
and `sections()` is empty.

Tests: `ui.dyno.stage`, `ui.dyno.large` (with `dyno.paper`), `ui.dyno.run`, `ui.dyno.absent`, `ui.dyno.sheet`.
