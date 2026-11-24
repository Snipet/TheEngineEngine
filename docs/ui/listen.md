# LISTEN: where the car is heard from

*Package U5. Code: `ui/views/listen/`. Tests: `tests/ui/views/ListenTests.cpp`. The model: package L's
(`docs/models/listen.md`, `docs/handoffs/l-listener.md`). Pictures: the software rasteriser's.*

**The plan (FROM ABOVE)**: the car drawn to the build's `car.wheelbase` and `car.track` (a bike when `car.kind` is
Motorcycle), its front marked in words; every sound source a dot at its place in the car's frame (x right, y forward
from the rear bumper), its size its level at the listener, its name and level beside it (sources at one place share a
label: "ENGINE, TURBO"); a silent source hollow and "silent", an unknown level "-". The listener: a point in the accent
with its facing and a handle that turns it, rings of distance (1, 2, 5, 10, 20, 30 m) numbered where there is room,
its camera in words. Drag the listener and the camera becomes FREE at the place it is dropped (`sound.listenerX`,
`listenerY`, camera 7, one undo step); drag the handle to turn it (`sound.facing`, 5 degree steps). The place drawn
round the car: the tunnel's walls 9 m apart, the garage's 6 by 7 m box, the street's facades (4 m to the left, 14 m
across), the field's tree line said in words (34 m off), the hall said to be the reference's room. Trackside: the road
along the car's line, the car's distance past the listener. A scale bar (1, 2, 5 or 10 m).

**The bar**: CAMERA (Tailpipe, Rear, Side, Engine bay, Driver, Trackside, Chase, Free) and PLACE (Hall, Field,
Street, Tunnel, Garage), one click each, lit while chosen. With the reference's listener (`sound.model` 0) the
reference's five VIEWs instead (`sound.view`), said in words.

**WHAT IS HEARD**: the loudness trim, the mix's level (a bar, -60 to 0 dB), the limiter's gain reduction (a bar to 12 dB,
amber, a lamp and LIMITING or NOT LIMITING), the ceiling's count, the range, the Doppler ratio; trackside the car's
distance past the listener; in the driver's seat THE CABIN: the windows (also a handle on the car's side in the plan:
down opens it) and the four ways in with their levels.

The inspector shows the `sound` section's generated pages (SCOPE no longer does).

## Names

| Read | On the branch (L, e377835) | Without it |
|---|---|---|
| Spec `sound.model`, `camera` (8: the last Free), `place`, `enginePlace`, `windows`, `distance`, `view`, `listenerX`, `listenerY`, `facing` (degrees from forward toward the right); `car.wheelbase`, `car.track`, `car.kind` | yes | the button does nothing; no drag |
| tel `sound.srcX[i]`, `srcY[i]`, `srcLevel[i]` (dB, -200 silent; tailpipe left, right, intake mouth, bay, bay carrying, tyres, wind, gearbox, axle, tank), `listenerX`, `listenerY`, `facing`, `range`, `doppler`, `roadPosition`, `loudness`, `mixLevel`, `limiter`, `ceilingParts`; `car.liftAngle` (a bike's pitch) | yes | the model's documented places, said in words; a dash |
| tel `sound.pathFloor`, `pathFirewall`, `pathGlass`, `pathStructure` (dB) | not offered yet (asked of L) | the four ways in words, no numbers |

The rear axle is drawn 0.85 m forward of the rear bumper (a bike's 0.3 m): no field says it yet. A bike is drawn
shortened by its lift about the rear tyre; the sources' places, from L, are already turned.

The demo hook (`ListenDemo.cpp`, `TEE_UI_DEMO(listen, listenDemo)`) adds L's fields and makes the telemetry up.

Tests: `listen.stage` (+`.dpi1`, `listen.paper`), `listen.framestage` (the frame's stage at 1100 x 700 and 1600 x 1000,
both densities, every camera, no glyph off its panel or over another), `listen.cameras` (+ goldens `listen.driver.open`,
`listen.trackside`, `listen.garage`), `listen.drag` (+`listen.free`), `listen.reference`, `listen.absent`, `listen.rest`.
