# The G29 racing wheel, as TestingPlayground has it: reference files for the port

*The lead, 25 Nov. The owner: "I have a Logitech G29 controller with a manual shifter attached. I should not need
any external software for it to work, as TestingPlayground was working without any new installed software/drivers
(lights included)." These are the files that did that, copied unchanged from the owner's public repository
Snipet/TestingPlayground at commit `47bac40327ed9e8e59928500de9f3beb63fe8636`, for card B to port into `shell/input/`
(the MIDI taken out, the values into the `Driver`). Nothing here is compiled or tested by this repository, and each file carries `.txt` after its name so that no
lint, glob or editor takes it for this repository's source (its bytes are unchanged: the sha256 is the original's); a file
ported from one of these gets a SEED.tsv row from the table below (`docs/plan/01-rules.md`).*

| File here (beneath this folder) | Path in TestingPlayground | sha256 of the file as it was there |
|---|---|---|
| `Source/dsp/Events.h.txt` | `Source/dsp/Events.h` | `65c766914aeedab6ea8b94fc3796704adc934ec66a0f58ffa6e00b934390a08c` |
| `Source/plugin/WheelDriver.cpp.txt` | `Source/plugin/WheelDriver.cpp` | `47ac50899336f8a14c209fcb47eaf8ab4506a9ee93fcd13f1a4404fba3c59fce` |
| `Source/plugin/WheelDriver.h.txt` | `Source/plugin/WheelDriver.h` | `be00f1e21c7297320e9f58158a3698dacaaa06bc029273788b0741e44b8cad16` |
| `Source/plugin/WheelEvdev.h.txt` | `Source/plugin/WheelEvdev.h` | `fe6088b9d6f97cb6c6db10d5b2bb44a6643f99ea27a9c87bba433843e7656033` |
| `Source/plugin/WheelMap.cpp.txt` | `Source/plugin/WheelMap.cpp` | `7a034d9f0c869686a1d3be639c13b7971b2c23c0443171a573b9d469606486e7` |
| `Source/plugin/WheelMap.h.txt` | `Source/plugin/WheelMap.h` | `e8d2cbad0cb6e83d2e372e404a0d39040723456c834c802d79ffe714462c1c77` |
| `Source/plugin/standalone/RacingWheel.cpp.txt` | `Source/plugin/standalone/RacingWheel.cpp` | `a2650f468276f8db6184901768861a2a38202fd3004ed124a51471bb028bfb44` |
| `Source/plugin/standalone/RacingWheel.h.txt` | `Source/plugin/standalone/RacingWheel.h` | `acf44f9476584f486cd3c3ea45f290f97b22c911d5e7a7ed95d7c8196f0daf19` |
| `Source/plugin/standalone/RacingWheelLinux.cpp.txt` | `Source/plugin/standalone/RacingWheelLinux.cpp` | `eea555253ea47eeb5ff297e25e3507c74ae3b92831c331c1ea32d65700e2643b` |
| `Source/plugin/standalone/RacingWheelNone.cpp.txt` | `Source/plugin/standalone/RacingWheelNone.cpp` | `ae8730aa57d14ec8d4a274c6aea06e105a3920521b7d5a8fb1f6386d0c4941af` |
| `Tools/probes/plugin/wheel.cpp.txt` | `Tools/probes/plugin/wheel.cpp` | `2055155822d04a0ce78f8c112e87d88ff214072effc67833e43634283ffff6f9` |
| `Tools/wheel-dump/main.cpp.txt` | `Tools/wheel-dump/main.cpp` | `a0a3cc3b446ad65ea6c17f614a5c8e380bbbef8f7d419c6a7b52bfd3a7617d91` |

Excerpts (not whole files, so no sha256):

- `README-a-racing-wheel.txt`: the section "A racing wheel" of its `README.md` (lines 1640 to 1804 there): what the owner sees and does, what was checked on the device.
- `CMakeLists-excerpts.txt`: which body each platform compiles, the two frameworks the IOKit body links, the `wheel-dump` target.
- `Devices-shiftLights-excerpt.txt`: `Source/plugin/standalone/Devices.cpp` lines 85 to 100, where the shift lights' mask is taken from the program.
