# Card N2: the boosted engines, measured

Every table here is made by `build-dev/bin/tee-calib-boost` (tests/sim/calibration/boost/harness/main.cpp): the build
on the engine dyno (MODE_DYNO_ENGINE, full throttle, the engine held warm as `tee::runDyno` holds it), 48 kHz, 8 s of
warm-up, then at each speed 5 s settling and 1 s measured. Telemetry is the mean over the 1 s window, block by block;
the port pressures are cylinder 1's `scope.portExhaust` and `scope.portIntake` averaged over each whole cycle in the
window, then over the cycles (means over cycles, never a waveform). Each file names the commit it was measured on.
Runs are deterministic: the same build and commit give the same numbers.

Files: `backpressure.md` (the first question: what the cylinder sees at its exhaust port under boost).
- `gamma.md`: the boosted ledger across D's gamma change (3990ee47, a845b07f), what moved more than 3 %.
