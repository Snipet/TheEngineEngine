n2-b.patch: N2's (b) series as five code commits (format-patch). Base: 1fa216c0. Apply with `git am docs/handoffs/patches/n2-b.patch` on 1fa216c0 or a later branch tip.
What it carries: (b), the torque-peak compressor sizing with its charge terms; the vane surge guard removed (and its LayoutTest case); a VGT's matched nozzle passing the whole design flow, its stop scaled to the same shut area.
Not final at the rated end (the lead's ruling pending); for D2's brake retune. Pushed as docs because code is frozen; it goes in through the package window, not from here.

n1-pops-seam.patch: N1's half of the pops' seam (sim/ecu/overrun.fm's FuelAccount under Pop::burnPipe, combustion.fm's account calls, and the quick test and probe in tests/sim/ecu/IntegratedTest.cpp), a plain diff against 856a4ee4. For G's review before the code push (docs/handoffs/g-exhaust.md, "The pops' seam"); not applied from here.
