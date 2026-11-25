# (c)'s margin: the cycle's peak phi over its mean at rated. Predictions, 25 Nov ~16:20, before any run
The compressor sees the engine's intake pulses through the charge volume and the cooler, so its flow pulses less than
the turbine's. The peak over the mean grows as the pulses get fewer and larger (fewer cylinders, lower speed) and
shrinks with the charge volume.
- tractor and truck's sets (six, 1900-2100 rpm, 20 L pipes): peak/mean 1.01-1.02;
- pickup (six, 2800): 1.01-1.02;
- 2.0 (four, 4000, smaller pipes against its size): 1.02-1.04.
The free-run wander is about 0 (vgt-windup.md), so the margin is the pulsation alone: (c) would clear the flag at a
mean phi of 0.98 / (peak/mean), 0.94-0.97.

## Measured (the package tree plus scratch instrumentation; rated, free run, the last 10 of 20 s)

phi = mdot / mc, read every sample, over every turbo. "Window peak" is the highest phi in each 0.12 s window (at least
one engine cycle here), averaged. "Max" is the highest over the 10 s. "Choke share" is the share of the control steps
with the choke flag set, as a 1 s running mean.

| build | rpm | wheel | mean phi | window peak | max | peak/mean | max/mean | choke share |
|---|---|---|---|---|---|---|---|---|
| tractor (b) | 1900 | 72.4 | 0.9400 | 0.9414 | 0.9421 | 1.0014 | 1.0022 | 0 |
| tractor (c) | 1900 | 69.7 | 0.9768 | 0.9815 | 0.9920 | 1.0048 | 1.0156 | 0.41 |
| truck's sets | 2100 | 69.8 | 0.9655 | 0.9757 | 0.9879 | 1.0106 | 1.0232 | 0.38 |
| pickup | 2800 | 64.7 | 0.8044 | 0.8065 | 0.8078 | 1.0027 | 1.0042 | 0 |
| 2.0 | 4000 | 38.4 | 0.9024 | 0.9117 | 0.9140 | 1.0103 | 1.0129 | 0 |

**What it shows:**
1. **The flag fires on the instantaneous phi.** On (c)'s 69.7 mm the mean is 0.977, under the rule's 0.980, while 41 %
   of the steps are flagged on the cycle's peaks. The truck likewise: 0.966 mean, 38 %.
2. **The pulsation is small: 0.1-1.1 % peak over mean, 0.2-2.3 % at the 10 s maximum.** It is largest where the
   compressor works closest to choke (the truck, the 69.7 mm tractor) and on the four-cylinder 2.0. It is smallest on
   (b)'s tractor and the pickup, which run far from the flag.
3. **The model has no rule for the compressor's pulsation** from the cylinder count and the volumes. The charge volume
   and the cooler damp it, and the result depends on the operating point. So by the lead's terms a margin would be the
   measured ratio, labelled as judgement: by the max, 1.016-1.023 on the heavy sixes near choke and 1.013 on the four.
4. **For (c),** clearing the flag at the peak means a mean phi of 0.98 / 1.023 = 0.958 on the sixes and 0.98 / 1.013 =
   0.967 on the four. A single labelled 0.96 covers every build measured.

**The predictions:**
- sixes 1.01-1.02: the measured window peak is 1.001-1.011, at or under the range; by the max, 1.002-1.023, in it.
- the four 1.02-1.04: wrong, it is 1.010-1.013.
- the wander about 0: right.
- (c) clearing at 0.94-0.97 mean: right (0.958-0.967).
