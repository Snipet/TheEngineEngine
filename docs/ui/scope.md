# SCOPE: the instrument bench

*Package U5. Code: `ui/views/scope/`. Tests: `tests/ui/views/ScopeTests.cpp`. Pictures: the software rasteriser's.*

Six plots, everything live, read only while the workspace is shown (its `step()` runs only then), all held by
**FREEZE**. Every plot has the kit's hover cursor: a line and each trace's value at it, in the title line.

| Plot | What it shows | Reads |
|---|---|---|
| CYLINDER PRESSURE | Cylinder 1 over its cycle (axis ticked at TDC and BDC), in crank degrees after firing top centre (found as the higher-pressure of the two smallest volumes). The valves' open periods as bands (INTAKE, EXHAUST) with IVO/IVC/EVO/EVC in numbers, the spark (dashed), half the charge burned (dotted), the peak marked; peak and angle as numbers in the bar; under it the knock retard by cylinder (amber with its degrees while it retards) and the knocks counted | Scope `pressure`, `volume`, `liftIntake`, `liftExhaust`, `degrees`; telemetry `ecu.advance`, `ecu.halfBurn`, `ecu.knockByCylinder[i]`, `ecu.knock` |
| P-V LOOP | The same cycle against volume, linear or log-log (**LOG P-V**); EVO, IVC and the spark as points | Scope `pressure`, `volume` |
| PORTS | Intake (solid, cold) and exhaust (dashed, hot) port pressures over the cycle, with the valves' bands and the air outside | Scope `portIntake`, `portExhaust` |
| OUTPUT WAVE | The output's last whole cycles (about 50 ms), its end at cylinder 1's cycle start so it stands still at a steady speed; on its own rising edge when there is no crank angle | `outputTail`; telemetry `engine.speed`, `engine.crank`, `sound.rate` |
| SPECTRUM | A 4096-point FFT (Hann, power smoothed over 0.12 s, analysed at most 30 times a second) in dB against log frequency; the firing frequency (the order of cylinders per revolution) and its multiples marked and numbered | `outputTail`; `engine.speed`; the layout's cylinders |
| SPEED MAP | The spectrum against engine speed (a Campbell diagram), 72 x 56 cells filled as the engine sweeps (brighter is louder, a scale beside the title); the firing orders as slanted dashed lines; the speed now in the accent. **CLEAR MAP** empties it | as SPECTRUM; `engine.redline` sets its range |

The inspector is the listener (`sound` section's generated page) until it has a workspace of its own.

**Absent data** draws an empty plot with words saying what is missing, and a dash for a number; nothing is made up
(`scope.absent`). Names read that the simulator does not write yet: Scope `portIntake`, `portExhaust`; telemetry
`ecu.advance`, `sound.rate` (48000 is assumed until then). U1's demo has all four, so the tests and pictures show them.

**At rest** (frozen, or the output unchanged) `moving()` is false and `drawKey()` stands (`scope.rest`, `scope.freeze`).

Tests: `scope.stage` (+`.dpi1`), `scope.large` (+`.dpi1`, `scope.paper`), `scope.campbell`, `scope.logpv`,
`scope.absent`, `scope.fft`, `scope.freeze`, `scope.rest`.
