# Getting started

Ten minutes from opening the application to watching a torque curve move because you changed a cam.

## 1. Pick a build

The application opens in **GARAGE**: the library of builds. A *build* is a whole machine in three parts that can be
swapped separately: the **engine** (block, valves, intake, boost, exhaust, fuel), the **car** (mass, tyres,
gearbox, aerodynamics) and the **tune** (the ECU's maps and settings).

The thirty archetypes are a good place to start: each is a kind of engine you will recognise in the car it usually
lives in, with its real dimensions. Try **"Small-block 5.7 L, carburettor-like"** first: a simple, forgiving engine.
Click its card, then **LOAD THIS BUILD**.

Want the same engine in another car? Open the car you want, then load only the **engine** part of another build
(the parts are listed apart in GARAGE, and under File > Load part).

## 2. Start it

Press **Return** (or the cockpit's START). The starter cranks until the engine catches, about a second for a warm
engine, longer from cold. Hold Return for a second to switch it off again.

Listen to the idle. A hot cam idles lumpy and high; a stock cam idles smoothly. The tachometer and the SCOPE show the
idle speed wandering a little: that is real, a warm idle wanders some 20 to 50 rpm.

## 3. Rev it

**Up arrow** (or W) is the throttle. Tap it and the engine blips; hold it and it climbs to the limiter, which
bounces it off the redline. Let go and listen to the overrun: the ECU cuts the fuel, and any it lets through can pop
in the exhaust (TUNE > Pops).

## 4. Drive it

The car starts in neutral with the clutch worked for you. **E** shifts up (into first from neutral), **Q** down, **N**
to neutral, **1** to **6** straight to a gear. Floor it in first: on street tyres a strong engine spins its tyres;
the DRIVE workspace shows each tyre's slip. **Down arrow** (or S) brakes.

An automatic or a CVT shifts by itself once it is in drive.

## 5. Look at the dyno

Open **DYNO**. The ribbon at the top shows the build's torque and power curves: worked out in the background from
steady full-throttle points on the engine dyno, ready within a second or two of any change. Read off:

- **peak torque** and where it comes: a long-stroke pushrod V8 peaks low, a short-stroke four-valve engine high;
- **peak power**, which is torque times speed: it comes later than the torque peak;
- **BMEP** (torque made comparable across sizes): 10 to 13 bar for a good naturally aspirated engine, 18 to 25
  turbocharged (see the [glossary](glossary.md)).

## 6. Change a cam and watch the curve move

Open **VALVES**. Select the intake cam and raise its **duration** (say from 214 to 240 degrees at 1 mm) and its
**lift** a millimetre. Go back to DYNO: within a couple of seconds the new curve is drawn over the old one. A longer
cam moves the torque peak up the rev range and makes more power at the top, and gives up torque low down. Start the
engine again: the idle is lumpier, because the longer overlap lets exhaust back into the intake at low speed.

Then try the other way round: close the **lobe separation** (move the intake centreline earlier). The peak gets
sharper and moves down.

## 7. Where next

- [Workspaces](workspaces/README.md): a page for each of the ten screens.
- [How each part is simulated](models/README.md): what is modelled, what is left out, and the known gaps.
- [Controls](controls.md): the keyboard, a gamepad, and (later) a wheel.
- [Glossary](glossary.md): BMEP, VE, overlap, lambda, knock and the rest.
- [Every setting](parameters.md): the full table, generated from the simulator itself.
- [How right is it?](validation.md): each archetype measured against published figures, with the misses.

**Air.** The archetypes stand in standard-day air (15 C, 101.3 kPa). Colder air is denser and makes more power: the
CAR workspace's environment page has the air's temperature, pressure and the road's surface.
