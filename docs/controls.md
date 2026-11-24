# Controls

*What drives the engine in TheEngineEngine, as built (shell/input/). A wheel and pedals are not read yet.*

Everything below ends in one `ControlsFrame` a frame (shell/include/tee/app/Sources.h), which is all the engine
sees: the keys, a gamepad and the Panel's own controls are merged, and for each pedal the largest wins.

## Keyboard

Keys are read by their place on the board, not by the letter printed on them, so W, A, S, D are where a hand
expects them on any layout.

| Key | What it does |
|---|---|
| Up, or W | The throttle. It eases down while the key is held (0.30 s for the whole travel) and back up when it is let go (0.18 s). |
| Down, or S | The brake, the same way (0.20 s down, 0.12 s up). |
| Left Shift | The clutch pedal (0.10 s down, 0.30 s up). Only needed with the automatic clutch off. |
| Space | The handbrake. The reference engine's car has none: the key does nothing yet. |
| Return, or keypad Enter | The start key (below). |
| E, or Page Up | A shift up. |
| Q, or Page Down | A shift down. |
| N | Neutral. |
| 1 to 6 | That gear. |

**The start key.** Press it and the starter cranks the engine; it goes on cranking by itself until the engine
runs. Hold it for a second while the engine runs and the engine is switched off. A tap is enough to start: a press
shorter than a frame is still held for 0.15 s.

**The gears.** The automatic clutch is on (there is no switch for it in the window yet). With it, E and Q are the
paddles of a paddle-shift car: the gearbox works its own clutch, pulls away by itself when the throttle goes down in
first, and never stalls. A gear key (1 to 6, N) presses the shift up or down for you, once every 0.3 s, until the
gearbox is in that gear; it gives up after twelve presses (a downshift the gearbox refuses because it would take the
engine past its redline). With the automatic clutch off, a gear key moves an H-pattern lever and the clutch is yours
(Left Shift): the Driver has this path and its test, and nothing in the window turns it on yet.

The times are `tee::input::DriverSettings`; they move to the settings file with the bindings in the shell's
second slice.

## Mouse

The Panel's own controls reach the same frame: its **START** button is the start key (hold it a second to switch
off), its **throttle strip** is the throttle while it is pressed and dragged, and its gear control asks for a gear
as the keys do. In the plain Panel (`--panel plain`) the strip is the tall bar at the left: press and drag it, and it
springs back when let go; the wheel over it sets a throttle that stays where it is put, until it is wheeled back to
nothing or the strip is pressed.

A knob turns by a drag up or down, or by the wheel over it. In the plain Panel, Shift makes the drag fine and a
double click puts a dial back at its default.

## Gamepad

Through SDL's gamepad interface, so any pad SDL knows. Analogue, with no easing.

| Control | What it does |
|---|---|
| Right trigger | The throttle |
| Left trigger | The brake |
| Left stick, left and right | Steering (nothing reads it yet) |
| Right bumper, or A (the bottom face button) | A shift up |
| Left bumper, or X (the left face button) | A shift down |
| Start | The start key |

The first pad plugged in is the one read; when it is unplugged, the next. Nothing here has been tried with a pad
in hand: the code was written against SDL's documentation and compiled.

## A wheel and pedals

Not yet. `shell/input/Wheel.h` is the seam it will arrive through (the shell's second slice).

## What the reference engine does with them

The application plays the reference engine (sim/ref) for now, and starts it in **DRIVE CAR**: in a car, in neutral,
so the pedal is the throttle and the engine idles until it is switched off. The `drive` dial (DRIVE, on the panel's
SOUND & DRIVE page) changes that:

- **CAR** and **TRUCK**: as above. The start key cranks only in neutral, or with the clutch pedal down.
- **KEYS** and **PITCH** (the reference as a synthesiser plays it): the engine runs from the start key until the
  start key is held for a second, and then for as long again as the IDLE FOR dial says (the reference switches an
  engine off that nobody is holding a key for). The throttle is the reference's "throttle by hand". There is no
  gearbox, so the shifts and the gear keys do nothing.

## For a machine

`tee-headless` (and the window with `--script`) reads the same keys from a script, by name: `w`, `s`, `up`, `down`,
`e`, `q`, `n`, `space`, `return`, `enter`, `lshift`, `pageup`, `pagedown`, `1` to `6`. `app/Script.h` has the
format; `tests/shell/scripts/` has scripts that start, rev, drive and stop the engine.
