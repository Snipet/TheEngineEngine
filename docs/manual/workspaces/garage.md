# GARAGE

The library of builds. A build has three parts that load separately: the **engine** (its block, valves, intake,
boost, exhaust and fuel), the **car**, and the **tune** (the ECU). GARAGE shows whole builds and parts alone, and lets
you combine them.

## What you see

**The cards.** One card for every build and every part in the library:

| On the card | What it means |
|---|---|
| Name | The build's or part's name, e.g. "3.0 L straight six, twin turbo". |
| WHOLE BUILD, ENGINE, CAR or TUNE | What the card holds, in a frame. |
| SHIPPED, YOURS or CURRENT | Came with the program, saved by you, or the build now loaded. The current one also has a bar down its edge, and `* UNSAVED` while you have changed it. |
| Sentence | The engine in one line ("3.0 L straight six, twin turbo, E85"); for a car or a tune, its first values. |
| Peak torque and power | With the rpm each occurs at, in your units. |
| Small curve | The power curve. Every card uses the same scale, so a taller curve really is more power. |

Above the cards: a **search** field (it matches the name, the sentence and the notes as you type), **SORT** (by name,
power, displacement or number of cylinders) and **SHOW** (all, builds, engines, cars or tunes). The mouse wheel
scrolls the cards.

**The card's column**, at the right of the stage, stands in for the inspector here: the chosen card's name, part,
sentence and notes, and the buttons for what you can do with it. A line over the buttons says what the last action
did (green), or why it could not (amber).

**COMPARE** puts the current build and another on one plot: torque as heavy lines, power as light ones, the current
build solid and the other dashed, with a legend. Under it, every setting that differs between the two, grouped by
section, with both values.

## What you can change

| The card chosen | Buttons |
|---|---|
| Another whole build | LOAD THIS BUILD; ITS ENGINE (into your current car); ITS CAR (under your current engine); ITS TUNE (on your current engine); COMPARE; DUPLICATE; DELETE (your own builds only, then CONFIRM DELETE or KEEP IT) |
| A part alone | PUT THIS ENGINE IN, USE THIS CAR, or TRY THIS TUNE |
| The current build | SAVE (your own builds) and REVERT; SAVE AS (type a name); NOTES (type them); DUPLICATE |

Each load is one undo step, so Ctrl+Z (Cmd+Z on a Mac) takes it back. Loading a part replaces that whole part: an
engine part sets the engine's sections and puts anything it does not mention back to its default.

The same can be done from the keyboard and the File menu: Ctrl+N (new build), Ctrl+O (open), Ctrl+S (save),
Ctrl+Shift+S (save as), and F2 for the File menu, which also loads or saves one part on its own. A build still
unsaved when the window closes is kept as "Unsaved changes" and opens next time.

The thirty shipped archetypes are listed in [The shipped builds](../builds.md); the same engines, cars and tunes are
also in the library as parts alone.

## Things to try

1. **Same car, different engine.** Load "1.6 L 16-valve hot hatch". Choose the "Rotary twin" card and press ITS
   ENGINE. The hatch now carries the rotary; the dyno ribbon and the card's sentence change. Ctrl+Z puts the four back.
2. **Flat-plane against crossplane.** Load "Flat-plane 4.5 L V8", choose "Four-cam 5.0 L V8" and press COMPARE. The
   plot shows where each makes its torque; the list shows how much else differs (bore, stroke, cams, redline). Then
   open both on ENGINE and compare their firing rings.
3. **Sort by power.** SHOW builds, SORT by power. Compare where the turbocharged builds sit against the large
   naturally aspirated ones, then SORT by displacement and see how the order changes.
4. **Try a tune alone.** With "2.0 L turbo four, direct injection" loaded, choose another build's tune and press
   TRY THIS TUNE (or ITS TUNE). Look at TUNE to see what changed. REVERT or Ctrl+Z returns to the saved tune.
5. **Keep your own.** Change something on VALVES, then SAVE AS with a new name. The card appears marked YOURS; DELETE
   is offered only for those.
