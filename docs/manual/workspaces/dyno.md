# DYNO

The dyno ribbon made large: torque and power against rpm, the build's predicted curve, and dyno runs you start,
watch, name, overlay and export. There is no ribbon in this workspace; this is the ribbon.

## What you see

- **The big plot.** Torque on the left axis and power on the right, against engine speed, in your units. Torque is
  a heavy line, power a light one (the legend says so). The peaks are marked with discs, and the chosen curve's peaks
  carry their numbers and also appear large at the top right.
- **The cursor.** Move the pointer over the plot: a line at that rpm and a card reading every shown curve there
  (torque, power and the third trace).
- **The third trace.** A band under the plot on the same rpm axis: none, boost, lambda or spark advance (buttons in
  its title row). A quantity no shown run recorded is a dash and a sentence naming what is missing, never a line.
- **Curves and runs.** The predicted curve (the build as it is now: solid, in the accent colour), the curve before
  your last edit (grey, short dashes), then each run in turn, each with its own colour and dash pattern, so no two are
  told apart by colour alone. Each row has a box to show or hide it, a sample of its two lines, its name and its
  peaks. When there are too many, the newest are listed with a count of those left out.
- **A run.** The plan's box: engine or chassis dyno; a sweep at a rate, or steps every 500 rpm each held for a dwell;
  from and to; pedal; the gear for a chassis run. START RUN and STOP. While a run goes it draws itself, the span swept
  so far shaded, and the plan's box says where it is in words with a bar. When it lands it is chosen and its peaks
  are written.

The predicted curve is the engine dyno run by the program in the background on a copy of the build, a moment after
each edit: first a rough curve, then a finer one.

Typical values: a naturally aspirated petrol engine at its torque peak makes about 90 to 110 N m per litre (a BMEP
of 11 to 14 bar); a modern turbo petrol engine 150 to 200 N m per litre.

## What you can change

- **A row** chooses it: the readouts at the top, the NAME field and the export follow it.
- **A row's box** shows or hides that curve.
- **NAME** renames a run (Enter keeps it). The predicted and before curves cannot be renamed.
- **SAVE CSV** writes the chosen curve to a file (or **COPY CSV** to the clipboard), with rpm, torque in N m, power
  in kW and the third trace's quantity.
- **The band's buttons**: none, boost, lambda, advance.
- **The plan**: engine or chassis, sweep or steps, rate, dwell, from, to, pedal, gear. The plan is not part of the
  build; it stays with this workspace.

## Things to try

1. **Before and after.** On "Flat-plane 4.5 L V8", run an engine sweep from 2000 rpm to the redline at full pedal and
   name it "stock". On AIR, lengthen the runners by 10 cm, run again and name it "long runners". Show both and move the
   cursor across the band where they differ.
2. **Predicted against measured.** Compare a finished run with the predicted curve. They should lie close; a run at a
   slow rate or in steps lets temperatures settle and may differ a little from a fast sweep.
3. **Watch lambda.** Choose the lambda band, run a sweep on "2.0 L turbo four, direct injection", and see how rich the
   ECU runs under boost compared with low rpm.
4. **Part throttle.** Run the same sweep with the pedal at 0.5 and overlay it on the full-pedal run. Note where the
   two part company: a half-open throttle restricts the flow more as rpm rises.
5. **Steps against a sweep.** Run the same range in steps of 500 rpm with a dwell of a few seconds, and compare with a
   fast sweep: the steps hold each point long enough for the turbo and the exhaust to settle.

## What is not there yet

- A **chassis** run: the chassis dyno is not yet a driven mode in the running engine, and the gear in the plan is not
  passed on to it yet. Use engine runs for now.
- On the running engine, a run in progress may not draw itself point by point (the connection that hands over a run's
  points as it goes is still missing); the finished run appears when it lands.
