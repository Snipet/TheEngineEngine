# H: combustion, fuels and the ECU (standalone; integration waits for K3b)

Card H ([w1-h-combustion-ecu.md](../plan/w1-h-combustion-ecu.md)), branch `claude/awesome-gates-ntdb58`. Models and
sources: [combustion.md](../models/combustion.md), [ecu.md](../models/ecu.md). Run: configure with
`-DCMAKE_PROJECT_TheEngineEngine_INCLUDE=$PWD/tests/sim/ecu/standalone.cmake`, then `ctest -R '^h\.'` (19 pass).

**Done, standalone** (`sim/combustion/`, `sim/ecu/`; tests in `tests/sim/combustion/`, `tests/sim/ecu/`):
- H1 fuel energy: each row's LHV/AFR, 2.93 to 3.08 MJ a kg of air (hydrogen 3.50, untested row); mixture: heat's top at lambda 0.86 to 0.92, unburned, misfire limits, cooling.
- H2 knock (Livengood-Wu over Douaud-Eyzat, tables within 1 %): boost, advance, hotter air, lower octane, higher compression each sooner; tau(PRF 90, 40 atm, 800 K) 2.7 ms (shock tubes: 1 to 5). On the made-up trace regular is knock-limited 16 degrees short of best torque: too far; `fuel.knockDelay` is calibrated against the real cylinder at integration.
- H3 maps: on K3a's `Map16`; exact at 256 breakpoints, bilinear between; a baked map reads its formula back bit for bit.
- H4 on a one-line engine: fuel cut 6342-6564, spark cut same, soft 6451-6480, throttle 6357-6465 (redline 6500); launch 3952-4000 at 4000; warm idle within 0.1 rpm, cold 971 down to 750 in ten minutes; the key cranks, runs, stalls, runs down.
- Parts of 9 and 10: bangs by fuel account (4x the fuel, 4x burned, bigger pulses; none with no fuel, air or heat); traction holds a toy wheel at 0.101 for 0.10. Diesel fields (identity at defaults), tuner readings, model 1's charge, settings' attributes.

**Cost**: a knock tick 9.7 ns (load ~3); every 4 samples a V8 costs at most 19 ns a sample, ~5 on average. R here: 2722 ns (`tee-bench`, v8 free-revs), so 0.7 % of R at worst against the 0.06 R budget.

**Waits for K3b** (in order): append `FuelSettings` to `spec.fuel` and `EcuSettings` to `spec.ecu`, `tel.ecu` fields; in the carved `Combustion`, `fuel.model 1` calls `Charge1.make` at each intake closing and ticks `EndGas` every 4 samples; in `Ecu`, `ecu.model 1` fills `EcuIn` and runs `Ecu1`, `fire(c)` per firing; model 0 kept bit for bit (null test, 5); then 6 to 11, calibrating `knockDelay` in 8.

**Seams I need** (names to K3b's style; what they return):
- K3b/D: `float Cylinder::burned()` (the burn's share, `xPrev`), and Combustion allowed to set `qTot`, `burnDur`, `cBurn`, `knock` at intake closing as `charge()` does.
- E: reads `float Ecu::camTarget(int cam)` (0 intake advance, 1 exhaust retard, crank degrees), `float Ecu::liftCommand()`.
- F: gives `float Boost::oxygenShare()` (charge's oxygen over air's; 1 without nitrous); reads `float Ecu::boostTarget()` (Pa over the air), `bool Ecu::antiLag()`, `bool Ecu::nitrous()`. The boost loop (gate, vanes) is F's, toward my target.
- G: gives `void Exhaust::pop(int bank, float amplitude)` (Pa, as `Pop::fire`'s `a`) and `float Exhaust::gasTemperature(int bank)` (K, where bangs light).
- I: gives `float Car::drivenSlip()`, `float Car::speed()` (m/s), `int Car::gear()`, `int Car::upshifts()` (a counter), `float Car::cutRequest()` (s of dog-box cut), `float Car::blipRequest()` (plate 0 to 1).

**Changes asked outside my paths**:
1. `ctl.bakeMaps` (int, a press counter) in `spec/controls.fm`; and in `spec/scope.fm`: `int bakeCount; int bakedNx[7]; int bakedNy[7]; float bakedX[7][16]; float bakedY[7][16]; float baked[7][256];`.
2. `tests/CMakeLists.txt`: glob `tests/sim/**/*.cpp` too (then `standalone.cmake` goes).
3. `spec/tables.fm` `Map16::at` (and `Curve16::at`): blend `a * (1 - f) + b * f`; `a + (b - a) * f` misses 1 of 256 last breakpoints by a rounding (`[!mayfail]` test). `EcuMaps::exactAt` works around it.
4. K3b: name carved files apart from mine (`fuels`, `mixture`, `knock`, `diesel`, `watch`, `charge1`, `settings`; `maps`, `limiter`, `idle`, `state`, `launch`, `knockctl`, `overrun`, `traction`, `ecu1`).

**Found**: no FMinus issue. The reference's idle gains limit-cycle (plus or minus 75 to 110 rpm) when the torque lags 270 degrees or more on the one-line engine; they hold at 180. Listen to nothing yet: no model-1 sound exists until integration.
