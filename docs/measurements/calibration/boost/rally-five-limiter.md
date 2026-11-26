# The rally five's boost on its limiter, on G's hot gas (26 Nov; the next window's item 1, N2's half)

The build: archetypes/inline-five-turbo-rally (wastegate, target 130 kPa, spring 80 kPa, spark-cut limiter at
7000 rpm, anti-lag on). The bench, flat out on the limiter from 2 s after the start, 3 s to settle, then 2 s one row a
512-sample block (10.7 ms). The tree: the branch at 04439bc4 (G's 5941904e, the burns' gas hot, and e6ae3a8b).
The case: tests/sim/calibration/boost/LimiterMeasure.cpp (hidden, `[.][measure]`; PROBE_BANG, PROBE_TARGET,
PROBE_ANTILAG, PROBE_ROWS).

## Predictions (written before any run)

1. Burned: the boost swings some 0.9 to 1.8 bar on a 0.3 to 0.5 s period, locked to the limiter's rpm cycle.
2. Carved: some 1.05 to 1.36 (G's reading).
3. The spring alone (target 0): the swing keeps at least 80 % of its size, so the loop is not its cause.
4. Anti-lag off: the swing mostly gone.
5. The gate at full lift at the peaks; the exhaust's books near 1.
6. So boost/ needs nothing.

## Measured

| run | boost, bar gauge | mean | period | rpm | books |
|---|---|---|---|---|---|
| Burned | 0.853 to 1.897 | 1.399 | 0.440 s (4 cycles) | 6839 to 7072 | 1.008 |
| Carved | 1.032 to 1.376 | 1.223 | 0.413 s (4) | 6834 to 7051 | 1.012 |
| Burned, the spring alone | 0.726 to 1.163 | 0.940 | 0.348 s (5) | 6840 to 7041 | 1.001 |
| Burned, `ecu.antiLag.on` 0 | the same as Burned, to the digit | | | | |

One cycle of Burned, every third block:

| t s | boost bar | shaft rad/s | gate lift | housing bar, K |
|---|---|---|---|---|
| 5.395 | 1.059 | 11222 | 0.232 | 2.99, 2499 |
| 5.459 | 1.282 | 13059 | 0.273 | 5.69, 2500 |
| 5.523 | 1.565 | 14718 | 0.814 | 5.77, 2500 |
| 5.587 | 1.744 | 14601 | 0.962 | 3.85, 2144 |
| 5.651 | 1.634 | 13135 | 0.992 | 4.34, 2426 |
| 5.715 | 1.348 | 11429 | 0.996 | 6.85, 2477 |
| 5.779 | 1.030 | 10381 | 0.434 | 1.62, 2343 |
| 5.843 | 1.079 | 11530 | 0.093 | 9.60, 2500 |

## What it shows

1. **The swing is a limit cycle of the gate and its loop, not the limiter's rhythm.** The rpm bounces block to block
   (some 20 ms), the boost on 0.41 to 0.44 s. The gate travels its whole stroke each cycle (0.05 to 0.999): it shuts
   below the target (1.3 bar), the shaft spins up 10,400 to 15,000 rad/s in some 0.15 s, the boost passes the
   target by 0.5 bar while the gate is still opening, and falls with the gate held fully open to under the target
   again.
2. **It was there before the hot gas.** Carved cycles on the same period at a third of the amplitude (0.34 bar). The
   hot gas (the housing at 2,100 to 2,500 K, 2 to 9.6 bar, against Carved's 633 K) gives the turbine more drive with the gate
   shut (estimated, not measured as such: about twice, from the shaft's spin-up), and the same loop's swing grows with it to 1.04 bar.
3. **The loop makes most of it.** The spring alone (no duty, the gate on its spring, the bled signal and the exhaust's
   push) cycles too, but 0.44 bar on 0.35 s, about the mean 0.94 bar: the push of the hot housing on the poppet opens
   the gate early. The loop (feed-forward, P 0.15, integral 1 a second) more than doubles the swing.
4. **Anti-lag changes nothing here:** flat out on the limiter the run is the same to the digit with `ecu.antiLag.on` 0.
   Suspected, not checked: it acts only on a lift.
5. **The books close:** 1.001 to 1.012, every run. The hot gas's turbine side is consistent: nothing in boost/ to
   change for G's (a).

**The predictions:** 1 the range right, the mechanism wrong (not locked to the limiter); 2 right; 3 wrong (42 %, not
80 %: the loop is most of it); 4 wrong (the anti-lag switch changes nothing at full throttle); 5 right; 6 right for (a) (the hot gas
needs nothing from boost/), but the loop has a fault of its own.

## The fault, and where it goes

A wastegate loop that cycles its gate end to end on a limiter: its gains (0.15, 1 a second) were set against the
turbo four's plant (machine.fm's comment: 0.5 cycled at 4500 rpm), and a turbine with twice the drive per unit of gate
crosses them. The cure is a design: a gain scheduled on the plant's gain (the turbine's drive per unit of gate lift),
or the feed-forward's push taken from a filtered housing pressure rather than the sample's (it reads 1.3 to 9.6 bar
block to block here), measured on every wastegate build before and after. It changes every wastegate build's loop, so
not before the freeze: the next window's, a design first, to the lead.
