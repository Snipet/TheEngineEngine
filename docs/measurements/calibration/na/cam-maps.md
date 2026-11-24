# Intake cam maps at full load, from the dyno (card N1)

*The best intake advance (crank degrees from parked, as `ecu.maps.camIn` takes it) at seven speeds, for each phased NA archetype. Measured on the engine dyno at full throttle, warm, with the phaser held still and the intake centreline swept from parked to the whole range in 10-degree steps (`--set valve.inPhaseRange=0 --set valve.inCentre=...`; Woschni walls off, 51959de's simulator). One run a point: the engine is chaotic and neighbouring advances within 1 to 2 % of each other trade places, so smooth these along the speed before using them. For P's generator: the full-load row of `camIn`; the part-load rows are not measured here.*

## economy-four-15

Parked centre 112 deg, range 40 deg, redline 6600 rpm.

| rpm | 1500 | 2306 | 3112 | 3918 | 4724 | 5530 | 6336 |
|---|---|---|---|---|---|---|---|
| Best advance, deg | 20 | 40 | 30 | 30 | 30 | 20 | 10 |
| Torque there, N m | 123 | 137 | 133 | 143 | 147 | 145 | 148 |
| Torque parked, N m | 119 | 125 | 130 | 132 | 139 | 138 | 145 |

## flat-plane-v8-45

Parked centre 106 deg, range 50 deg, redline 9000 rpm.

| rpm | 1500 | 2690 | 3880 | 5070 | 6260 | 7450 | 8640 |
|---|---|---|---|---|---|---|---|
| Best advance, deg | 0 | 30 | 20 | 10 | 20 | 10 | 20 |
| Torque there, N m | 261 | 393 | 424 | 406 | 395 | 351 | 327 |
| Torque parked, N m | 261 | 339 | 389 | 393 | 382 | 348 | 321 |

## four-cam-v8-50

Parked centre 108 deg, range 50 deg, redline 7500 rpm.

| rpm | 1500 | 2450 | 3400 | 4350 | 5300 | 6250 | 7200 |
|---|---|---|---|---|---|---|---|
| Best advance, deg | 50 | 50 | 20 | 10 | 0 | 30 | 0 |
| Torque there, N m | 422 | 446 | 472 | 449 | 455 | 459 | 456 |
| Torque parked, N m | 382 | 423 | 457 | 446 | 455 | 434 | 456 |

## straight-six-32-high-rev

Parked centre 106 deg, range 60 deg, redline 8000 rpm.

| rpm | 1500 | 2530 | 3560 | 4590 | 5620 | 6650 | 7680 |
|---|---|---|---|---|---|---|---|
| Best advance, deg | 30 | 10 | 30 | 20 | 10 | 0 | 0 |
| Torque there, N m | 254 | 288 | 266 | 283 | 273 | 262 | 246 |
| Torque parked, N m | 238 | 275 | 256 | 269 | 270 | 262 | 246 |

## v10-52

Parked centre 108 deg, range 42 deg, redline 8700 rpm.

| rpm | 1500 | 2642 | 3784 | 4926 | 6068 | 7210 | 8352 |
|---|---|---|---|---|---|---|---|
| Best advance, deg | 10 | 10 | 30 | 10 | 10 | 20 | 0 |
| Torque there, N m | 425 | 419 | 477 | 430 | 387 | 422 | 409 |
| Torque parked, N m | 406 | 416 | 444 | 426 | 383 | 412 | 409 |

## v12-65

Parked centre 106 deg, range 50 deg, redline 8900 rpm.

| rpm | 1500 | 2674 | 3848 | 5022 | 6196 | 7370 | 8544 |
|---|---|---|---|---|---|---|---|
| Best advance, deg | 20 | 0 | 20 | 10 | 0 | 20 | 0 |
| Torque there, N m | 501 | 537 | 587 | 564 | 466 | 539 | 558 |
| Torque parked, N m | 447 | 537 | 545 | 554 | 466 | 515 | 558 |

## v6-35-na

Parked centre 110 deg, range 40 deg, redline 6700 rpm.

| rpm | 1500 | 2322 | 3144 | 3966 | 4788 | 5610 | 6432 |
|---|---|---|---|---|---|---|---|
| Best advance, deg | 40 | 30 | 20 | 20 | 30 | 10 | 10 |
| Torque there, N m | 311 | 335 | 309 | 329 | 315 | 307 | 309 |
| Torque parked, N m | 285 | 303 | 299 | 313 | 301 | 302 | 307 |
