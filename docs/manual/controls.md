# Controls

*Taken from [docs/controls.md](../controls.md), which describes the controls as built. When the two differ, that
file is right.*

The keyboard, a gamepad and the cockpit's own on-screen controls all drive the same engine at once: for each pedal
the largest wins, so you can hold the throttle on the keyboard and brake with the pad.

## Keyboard

Keys are read by their place on the board, not their letter, so W, A, S, D sit under the left hand on any layout.

| Key | What it does |
|---|---|
| Up arrow, or W | Throttle. It eases down while held (0.3 s to the floor) and back up when let go (0.18 s) |
| Down arrow, or S | Brake, the same way (0.2 s down, 0.12 s up) |
| Left Shift | Clutch pedal. Only needed with the automatic clutch off |
| Space | Handbrake |
| Return, or keypad Enter | The key: a press cranks until the engine runs (two seconds at most; press again if it did not catch). Held a second while running: off |
| E, or Page Up | Shift up |
| Q, or Page Down | Shift down |
| N | Neutral |
| 1 to 6 | Straight to that gear (the shifts are made for you, one every 0.3 s; a downshift that would over-rev is refused) |

The automatic clutch is on: E and Q work like a paddle-shift car's paddles, the car pulls away by itself when you
press the throttle in first, and it does not stall.

## Files and sound

On a Mac, Cmd in place of Ctrl.

| Key | What it does |
|---|---|
| Ctrl+N | A new build, every setting at its default (asks first if you have unsaved changes) |
| Ctrl+O | Open a build |
| Ctrl+S, Ctrl+Shift+S | Save, save as |
| F2 | The File menu: recent files, the library, and loading or saving one part (engine, car, tune) alone |
| F9 | The Audio menu: the output device, its rate and buffer, and the recorder (writes what you hear to a WAV file) |

A build with unsaved changes when the window closes is kept as "Unsaved changes" and opens next time.

## Mouse

The cockpit's START, its mode (bench, dyno, drive) and its three pedal bars (press and drag) work as the keys do. A
knob turns by dragging up or down or by the mouse wheel; Shift makes the drag fine, a double click puts it back to its
default.

## Gamepad

Any pad SDL knows. Analogue, with no easing.

| Control | What it does |
|---|---|
| Right trigger | Throttle |
| Left trigger | Brake |
| Left stick, left and right | Steering (nothing reads it yet) |
| Right bumper, or A | Shift up |
| Left bumper, or X | Shift down |
| Start | The key |

The first pad plugged in is the one read. Nobody has tried a pad with these mappings yet: they follow SDL's
documentation.

## Wheel and pedals

Not yet. The wheel's mapping and force feedback are coming in a later version.
