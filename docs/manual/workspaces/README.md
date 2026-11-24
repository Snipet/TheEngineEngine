# The workspaces

The window has the same frame everywhere: the **top bar** (the build's name, with a star while it has unsaved
changes; the engine in one sentence; the simulator's load; the output's level; units and settings), the **rail** of
workspaces down the left, the **stage** in the middle (a picture of that part of the machine, drawn from the build
and moving with the engine), the **inspector** on the right (the settings of whatever you clicked on the stage), the
**dyno ribbon** under the inspector, and the **cockpit** along the bottom.

- Clicking a part on the stage selects it; its settings appear in the inspector. Every setting is in an inspector:
  nothing can be changed only by dragging a picture.
- The **dyno ribbon** (every workspace but DYNO and DRIVE) shows the predicted torque and power of the build as it
  stands. A moment after each edit (a rough curve in about half a second, a finer one shortly after) the new curve
  appears with the one before the edit ghosted behind it. It folds away.
- The **cockpit** is always there: START, the mode (bench, dyno, drive), an rpm bar with the redline and shift
  lights, gear, road speed, boost or vacuum, lambda, coolant and oil, and the three pedals as bars you can drag.
  The keys of the [controls](../controls.md) work in every workspace unless a text field has the caret.
- A value the simulator does not supply yet is drawn as a dash, never made up.
- Undo and redo work on every edit; a drag on the stage is one undo step.

| Page | What it is for |
|---|---|
| [GARAGE](garage.md) | The library: open, compare, mix and save builds and their parts. |
| [ENGINE](engine.md) | The block from above, the firing order, one cylinder in section, the pressure-volume loop. |
| [VALVES](valves.md) | The cam editor: drag lift, duration and centreline; timing ring, flow bench, phasers, springs. |
| [AIR](air.md) | The charge path from filter to runners, the compressor map, wastegate and blow-off valve. |
| [EXHAUST](exhaust.md) | The exhaust system as built, tinted by heat, with pressure pulses running along it. |
| [TUNE](tune.md) | The ECU's maps as tables, the live cell, lambda and knock traces, limiter and launch. |
| [CAR](car.md) | Mass and axle loads, tyre grip curve, driveline, gearing chart. |
| [DYNO](dyno.md) | Full-size torque and power curves, dyno runs you start, name, overlay and export. |
| [DRIVE](drive.md) | Round gauges, the car on its road, wheel slip, the drag strip's tree and time slip. |
| [SCOPE](scope.md) | Cylinder pressure, P-V loop, port pressures, the output wave, spectrum and speed map. |

See also the [glossary](../glossary.md) and the pages on [how each part is modelled](../models/README.md).
