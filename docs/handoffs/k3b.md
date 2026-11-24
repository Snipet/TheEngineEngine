# K3b: the reference split into modules (handoff, 2026-11-24)

*Written by the lead. The split's agent finished the carve and `sim/README.md` (c7c6bb7, 13:20 UTC) and was stopped
by an accidental interrupt before writing this file; its working copy was clean and pushed. What follows is what
the lead verified at the branch tip (45474be), and what the agent's commits and README say. Branch
`claude/awesome-gates-ntdb58`.*

**Done.** `sim/core/monolith.fm` is gone (8620241). `sim/engine.fm` is `Engine`: the contract's blocks, the order of
work, the fade. Carved, one commit a module, the null test after each: `Listen` and `Extras`, `Car`, `Heat`, `Boost`,
`Ecu`, `Combustion`, `Valvetrain`, `Intake`, `Exhaust` (with `Duct`, `Valve`, `Pop`), `Mechanics`, `Crank`; `Noise`
and the filters in `sim/lib/`. The arrays of 12 are `MAX_CYL` and `MAX_PORTS` (16), the seventeen layouts unchanged
(2a709d1). The Scope: cylinder 1's pressure, volume and both lifts by degree, and eight pipe pressures, while
`ctl.scopeOn` (0e423d7).

**The seam document is [`sim/README.md`](../../sim/README.md)**: every module's responsibility, state, functions with
exact signatures and when each is called; the order of work per sample and per 16; `configure`; "Adding a
subsystem"; what the Controls do; the Telemetry and the Scope as they are; the null test. Each package keeps it
true for its module from here.

**Verified by the lead at 45474be** (this machine, the dev preset, other agents building beside it):
- the whole suite: 272 of 272 tests pass (`ctest`, 408 s), the null test among them: every build of
  `content/builds` on every scenario of `tests/sim/scenarios`, every layout with a wild cam, two other rates and
  blocks, each pair identical to the bit (the test's own claim; it passed);
- the cost against the reference, `tee-bench --engine` (ns a host sample, the least of two runs, machine under
  load, so read the ratio): v8 free-revs 1.018, v8 drive-car 1.026, large-diesel free-revs 1.011, large-diesel
  drive-car 1.060; all four 1.028. The split costs about 3 % over the reference.

**Not verified by the lead**: the hour-long soak of card step 12 (the suite runs the short one); the bench within
5 % of the reference on a quiet machine; anything on macOS or Windows beyond CI's builds.

**Left as the reference has it** (see the README's list): the cylinder (`core/cylinder.fm`, unchanged); one lift table
for all valves; the key interface inside `Ecu` that `Engine::ignition` drives; the order of the noise draws, which
is part of the sound; `handbrake`, `steer`, `buttons`, `dynoSpeed`, `dynoRate`, `strip`, `bakeMaps` not read yet;
the dyno modes do nothing yet.

**Order of integration** (the lead's, 13:33): card D is the integrator and owns `sim/engine.fm`: first the hooks
of `docs/plan/integration-queue.md` as calls that do nothing at the Spec's defaults, the fields asked of the lead,
one model switch a package (0: the reference to the bit), and one name where two packages differed. E, H, F, G and
I integrate beside it, each in its own module and Spec section, the null test green before every push.

**Open from the split**: `host/include/tee/EnginePlayer.h` declares its own `struct tee::Build` beside the
document's (card C's new session renames it). `tee-render`'s `--engine` path is as the agent left it.

**For a person**: nothing new to hear: by the null test the split sounds exactly as the reference does.
