# Bike scenarios to listen to (card B2)

Each file is a `tee-scenario` naming its build; play one with
`build-dev/bin/tee-render --build <its build> --scenario tests/sim/bike/listen/<file>.json --out out/<file>.wav`.
Its `description` ends with what a person should hear. Nobody has listened to any of them yet; each was rendered
here (audio finite, peak below 1) and its telemetry checked against the description.

| File | Build | What to hear |
|---|---|---|
| bike-quickshift-12000 | bike-superbike | three quickshifts at about 12,000 rpm: 50 ms gaps, no clutch |
| bike-autoblip | bike-superbike | three auto-blipped downshifts from fourth |
| bike-crossplane-vs-screamer | bike-superbike | the crossplane's 270-180-90-180 beat, then the screamer's even four at the same 9,000 rpm (bench) |
| bike-v4-bigbang | engines/bike-v4-bigbang | the V4's twin-pulse firing through the range |
| bike-vtwin45-idle | bike-cruiser | the 45-degree twin's lope at idle, two blips |
| bike-600-16000 | bike-supersport-600 | the 600 on its 16,000 rpm limiter |
| bike-chain-clunk | bike-naked-twin | the chain's clunk at each throttle change in first |
| bike-downshift-no-slipper | bike-superbike | a clumsy downshift without a slipper (rear slip to -0.44); again with car.slipper 1 (-0.03) |
