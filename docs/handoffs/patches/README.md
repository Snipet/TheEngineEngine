n2-b.patch: N2's (b) series as five code commits (format-patch). Base: 1fa216c0. Apply with `git am docs/handoffs/patches/n2-b.patch` on 1fa216c0 or a later branch tip.
What it carries: (b), the torque-peak compressor sizing with its charge terms; the vane surge guard removed (and its LayoutTest case); a VGT's matched nozzle passing the whole design flow, its stop scaled to the same shut area.
Not final at the rated end (the lead's ruling pending); for D2's brake retune. Pushed as docs because code is frozen; it goes in through the package window, not from here.

n1-pops-seam.patch: N1's half of the pops' seam (sim/ecu/overrun.fm's FuelAccount under Pop::burnPipe, combustion.fm's account calls, and the quick test and probe in tests/sim/ecu/IntegratedTest.cpp), a plain diff against 856a4ee4. For G's review before the code push (docs/handoffs/g-exhaust.md, "The pops' seam"); not applied from here.

d-bike-synced.patch: card D's (the integrator's) patch for B2, the lead's ruling of 25 Nov: bike.fm's launch helper gets the
vehicle model's "a clutch that has synchronised stays closed" (gearbox.fm, `synced`): once locked in a gear (the slip
under 0.5 rad/s, the lever out) the clutch is held closed by the output's speed through the gear, opening only as that
nears the idle (50 to 250 rpm over it), until a shift, neutral, the lever pulled or a stall. Box::control takes the
lever's engagement as a new last argument. Measured with it applied (bike suite, non-nightly): one printed number moves,
"engine lash ... last closing 4.62 -> 4.66 rad/s" (the overrun half of the on-off cycle no longer slips the clutch at
low revs); every other print and every case's assertion count identical. The output's speed carries the rear tyre's
slip, where the vehicle model reads the road's: B2's choice. A plain diff against the tip of 25 Nov 14:40; not applied.
