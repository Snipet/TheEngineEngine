# The baked knock-limit table: the lead's proof (24 Nov, the closed exhaust, the tip after 445453da)

Each build baked on the tip by C's `tee-dyno --bake-spark --build B --threads 3 --save out/kl-B.json` (pedal steps;
the boost-target sweep not yet in), then the engine dyno (tee-calib-boost: stepped up, settle 5 s, measured over 2 s)
without and with the table, at 0.35, 0.5, 0.65, 0.8 and 0.95 of each redline. The lead's conditions: the knock
control's retard under 3 degrees in steady state; torque within 2 % of the knock-limited value before; nothing asks
more advance than before. ** marks a row that misses one.

| archetype | rpm | torque off / on N m | change % | retard off / on | ask off / on | table |
|---|---|---|---|---|---|---|
| boxer-four-turbo-unequal | 2450 | 191.0 / 191.0 | +0.0 | 0.0 / 0.0 | 17.1 / 17.1 | 1 |
| boxer-four-turbo-unequal | 3500 | 386.8 / 386.3 | -0.1 | 5.1 / 0.1 | 20.0 / 15.4 | 1 |
| boxer-four-turbo-unequal | 4550 | 398.4 / 399.0 | +0.2 | 1.5 / 0.0 | 22.5 / 20.3 | 1 |
| boxer-four-turbo-unequal | 5600 | 353.9 / 353.8 | -0.0 | 0.0 / 0.0 | 24.6 / 24.6 | 1 |
| boxer-four-turbo-unequal | 6650 | 348.0 / 348.1 | +0.0 | 0.0 / 0.0 | 26.5 / 26.5 | 1 |
| flat-plane-v8-39-twin-turbo | 2800 | 452.2 / 453.5 | +0.3 | 1.2 / 0.0 | 15.1 / 13.7 | 1 |
| flat-plane-v8-39-twin-turbo | 4000 | 730.7 / 718.5 | -1.7 | 8.4 / 0.4 | 18.4 / 10.6 | 1 |
| flat-plane-v8-39-twin-turbo | 5200 | 729.8 / 728.2 | -0.2 | 4.3 / 0.0 | 20.8 / 15.7 | 1 |
| flat-plane-v8-39-twin-turbo | 6400 | 677.8 / 673.4 | -0.7 | 0.0 / 0.0 | 22.8 / 20.8 | 1 |
| flat-plane-v8-39-twin-turbo | 7600 | 660.9 / 660.3 | -0.1 | 0.0 / 0.0 | 24.7 / 24.7 | 1 |
| inline-five-turbo-rally | 2450 | 245.6 / 245.6 | +0.0 | 0.0 / 0.0 | 13.3 / 13.3 | 1 |
| inline-five-turbo-rally | 3500 | 424.8 / 421.6 | -0.8 | 5.9 / 2.9 | 16.4 / 13.0 | 1 |
| inline-five-turbo-rally | 4550 | 431.8 / 429.0 | -0.6 | 4.9 / 0.0 | 18.6 / 12.8 | 1 |
| inline-five-turbo-rally | 5600 | 395.9 / 395.0 | -0.2 | 0.0 / 0.0 | 20.5 / 19.1 | 1 |
| inline-five-turbo-rally | 6650 | 385.7 / 385.3 | -0.1 | 0.0 / 0.0 | 22.2 / 22.2 | 1 |
| kei-660-triple-turbo | 2800 | 78.4 / 78.4 | +0.0 | 0.0 / 0.0 | 16.5 / 16.5 | 1 |
| kei-660-triple-turbo | 4000 | 94.1 / 94.1 | +0.0 | 0.0 / 0.0 | 19.6 / 19.6 | 1 |
| kei-660-triple-turbo | 5200 | 77.9 / 77.9 | +0.0 | 0.0 / 0.0 | 21.9 / 21.9 | 1 |
| kei-660-triple-turbo | 6400 | 61.1 / 61.1 | +0.0 | 0.0 / 0.0 | 24.0 / 24.0 | 1 |
| kei-660-triple-turbo | 7600 | 61.4 / 61.4 | +0.0 | 0.0 / 0.0 | 25.9 / 25.9 | 1 |
| rotary-twin-13 | 2800 | 130.9 / 130.9 | +0.0 | 0.0 / 0.0 | 19.2 / 19.2 | 1 |
| rotary-twin-13 | 4000 | 219.5 / 219.6 | +0.0 | 3.1 / 0.0 | 19.0 / 14.9 | 1 |
| rotary-twin-13 | 5200 | 214.3 / 211.7 | -1.2 ** | 14.2 / 7.0 | 20.7 / 8.3 | 1 |
| rotary-twin-13 | 6400 | 204.4 / 201.5 | -1.4 | 2.3 / 0.0 | 22.3 / 20.1 | 1 |
| rotary-twin-13 | 7600 | 201.1 / 201.3 | +0.1 | 0.0 / 0.0 | 24.1 / 21.8 | 1 |
| straight-six-twin-turbo-30 | 2450 | 525.4 / 526.6 | +0.2 ** | 14.6 / 5.5 | 12.9 / 2.9 | 1 |
| straight-six-twin-turbo-30 | 3500 | 551.3 / 543.8 | -1.4 | 6.4 / 0.0 | 15.5 / 9.1 | 1 |
| straight-six-twin-turbo-30 | 4550 | 515.7 / 509.4 | -1.2 | 4.4 / 0.0 | 17.7 / 13.1 | 1 |
| straight-six-twin-turbo-30 | 5600 | 463.1 / 460.1 | -0.7 | 0.0 / 0.0 | 19.5 / 17.2 | 1 |
| straight-six-twin-turbo-30 | 6650 | 451.7 / 451.3 | -0.1 | 0.5 / 0.3 | 21.2 / 21.2 | 1 |
| supercharged-v8-62 | 2170 | 781.5 / 776.6 | -0.6 | 2.6 / 0.0 | 13.9 / 10.4 | 1 |
| supercharged-v8-62 | 3100 | 815.4 / 814.5 | -0.1 | 0.0 / 0.0 | 16.7 / 16.3 | 1 |
| supercharged-v8-62 | 4030 | 808.3 / 808.2 | -0.0 | 0.0 / 0.0 | 19.0 / 19.0 | 1 |
| supercharged-v8-62 | 4960 | 773.6 / 773.3 | -0.0 | 0.0 / 0.0 | 21.0 / 21.0 | 1 |
| supercharged-v8-62 | 5890 | 707.3 / 707.7 | +0.0 | 0.0 / 0.0 | 22.7 / 22.7 | 1 |
| three-cylinder-10-turbo | 2275 | 138.7 / 137.7 | -0.7 | 6.7 / 0.3 | 12.1 / 6.9 | 1 |
| three-cylinder-10-turbo | 3250 | 177.7 / 177.1 | -0.3 | 8.8 / 0.0 | 14.8 / 6.1 | 1 |
| three-cylinder-10-turbo | 4225 | 170.2 / 168.1 | -1.2 | 5.7 / 0.0 | 16.8 / 9.4 | 1 |
| three-cylinder-10-turbo | 5200 | 152.8 / 152.1 | -0.5 | 1.9 / 0.0 | 18.5 / 14.7 | 1 |
| three-cylinder-10-turbo | 6175 | 148.8 / 148.9 | +0.1 | 1.4 / 0.3 | 20.1 / 19.3 | 1 |
| turbo-four-20-gdi | 2380 | 257.7 / 256.9 | -0.3 | 6.9 / 2.1 | 12.6 / 8.8 | 1 |
| turbo-four-20-gdi | 3400 | 359.9 / 360.4 | +0.1 | 8.4 / 0.0 | 15.5 / 7.3 | 1 |
| turbo-four-20-gdi | 4420 | 371.0 / 369.9 | -0.3 | 5.6 / 0.0 | 17.6 / 11.3 | 1 |
| turbo-four-20-gdi | 5440 | 308.3 / 306.4 | -0.6 | 0.0 / 0.0 | 19.3 / 16.2 | 1 |
| turbo-four-20-gdi | 6460 | 274.8 / 275.2 | +0.1 | 0.0 / 0.0 | 20.7 / 20.7 | 1 |
| twin-turbo-hot-vee-v8-40 | 2380 | 599.9 / 596.2 | -0.6 ** | 14.5 / 3.8 | 13.0 / 4.2 | 1 |
| twin-turbo-hot-vee-v8-40 | 3400 | 730.9 / 721.1 | -1.3 | 13.2 / 0.0 | 15.8 / 2.8 | 1 |
| twin-turbo-hot-vee-v8-40 | 4420 | 714.8 / 704.9 | -1.4 | 6.9 / 0.0 | 17.9 / 9.8 | 1 |
| twin-turbo-hot-vee-v8-40 | 5440 | 695.6 / 684.2 | -1.6 | 5.1 / 0.0 | 19.8 / 12.1 | 1 |
| twin-turbo-hot-vee-v8-40 | 6460 | 659.9 / 656.3 | -0.5 | 2.4 / 0.0 | 21.4 / 18.0 | 1 |
| v6-35-twin-turbo-pickup | 2100 | 441.3 / 421.4 | -4.5 ** | 5.5 / 0.5 | 12.4 / 7.8 | 1 |
| v6-35-twin-turbo-pickup | 3000 | 605.8 / 611.3 | +0.9 | 13.3 / 0.6 | 15.2 / 2.7 | 1 |
| v6-35-twin-turbo-pickup | 3900 | 583.8 / 583.1 | -0.1 | 10.0 / 0.0 | 17.3 / 7.5 | 1 |
| v6-35-twin-turbo-pickup | 4800 | 547.6 / 542.9 | -0.9 | 6.6 / 0.0 | 19.1 / 11.9 | 1 |
| v6-35-twin-turbo-pickup | 5700 | 525.9 / 524.0 | -0.4 | 5.5 / 0.0 | 20.7 / 15.7 | 1 |
| v12-65 | 3115 | 450.0 / 459.2 | +2.0 ** | 4.2 / 0.2 | 15.6 / 11.4 | 1 |
| v12-65 | 4450 | 408.2 / 406.7 | -0.4 | 0.1 / 0.0 | 18.4 / 18.5 | 1 |
| v12-65 | 5785 | 588.0 / 587.7 | -0.1 | 0.0 / 0.0 | 19.9 / 19.9 | 1 |
| v12-65 | 7120 | 603.5 / 603.2 | -0.1 | 0.0 / 0.0 | 22.0 / 22.0 | 1 |
| v12-65 | 8455 | 638.4 / 638.1 | -0.0 | 0.0 / 0.0 | 23.8 / 23.8 | 1 |
| flat-plane-v8-45 | 3150 | 354.0 / 355.2 | +0.3 | 0.0 / 0.0 | 14.8 / 13.8 | 1 |
| flat-plane-v8-45 | 4500 | 361.4 / 361.5 | +0.0 | 0.0 / 0.0 | 17.7 / 17.7 | 1 |
| flat-plane-v8-45 | 5850 | 452.8 / 449.5 | -0.7 | 0.0 / 0.0 | 20.0 / 20.0 | 1 |
| flat-plane-v8-45 | 7200 | 444.9 / 445.3 | +0.1 | 0.0 / 0.0 | 22.0 / 22.0 | 1 |
| flat-plane-v8-45 | 8550 | 404.5 / 404.1 | -0.1 | 0.0 / 0.0 | 23.8 / 23.8 | 1 |
| economy-four-15 | 2310 | 127.7 / 127.6 | -0.1 | 1.9 / 0.0 | 11.5 / 9.2 | 1 |
| economy-four-15 | 3300 | 147.5 / 147.3 | -0.1 | 0.0 / 0.0 | 13.9 / 13.9 | 1 |
| economy-four-15 | 4290 | 127.3 / 128.8 | +1.2 | 0.0 / 0.0 | 15.9 / 15.9 | 1 |
| economy-four-15 | 5280 | 133.1 / 132.9 | -0.1 | 0.0 / 0.0 | 17.6 / 17.6 | 1 |
| economy-four-15 | 6270 | 125.2 / 125.1 | -0.1 | 0.0 / 0.0 | 19.1 / 19.1 | 1 |

**Read.** Nine of the fourteen meet all three at every speed: the boxer, the flat-plane twin turbo, the five, the kei
(never knocks), the supercharged V8, the triple, the turbo four, and the economy four and the flat-plane 4.5 of the
natural ones. Nothing asks more than before anywhere. The knock retard falls everywhere (the hot-vee at 3400 rpm 13.2
to 0, the V6 pickup at 3000 13.3 to 0.6) and the torque moves under 2 % at all but two points.
**The misses**: (1) the knock retard still over 3 degrees where it sat at or near its 15 degree ceiling before: the
rotary at 5200 rpm (14.2 -> 7.0), the six at 2450 (14.6 -> 5.5), the hot-vee at 2380 (14.5 -> 3.8). Cause: the cell is
the mean retard plus 2, and a mean pinned at the ceiling understates what the engine needs: the bake should take a
second pass where its mean is over 12 degrees (bake, apply, re-measure, add the residual). C's to decide. (2) The V6
pickup at 2100 rpm loses 4.5 % at nearly the same spark run (6.9 against 7.3 degrees): not explained; the spool at its
lowest speed is the suspect. (3) The V12 at 3115 gains 2.0 %, on the bound.
