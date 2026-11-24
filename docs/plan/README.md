# The plan

Stage 1 is one file; stage 2 is the rest of this folder. An implementation agent reads the rules, the contracts if
its card says so, and its own card. Nothing else is required reading.

| File | What it is | Who reads it |
|---|---|---|
| [00-abstract-plan.md](00-abstract-plan.md) | The research and the abstract plan: the reference, the platform, the architecture, the features, the risks, the decisions | Anyone who wants the why |
| [01-rules.md](01-rules.md) | Rules for every package: what you may change, branches, building in the cloud, code, proof, the handoff | Every agent, first |
| [02-contracts.md](02-contracts.md) | The Spec, Controls, Telemetry and Scope; attributes; the simulator's modules and order of work; the host's interface; files; budgets | Every agent touching `sim/`, `host/` or `ui/` |
| [STATUS.md](STATUS.md) | What is running, what has landed, what is blocked | The lead keeps it; the user reads it |
| [checkpoints.md](checkpoints.md) | What the user runs on a Mac and on Windows, and when | The user |

**Phase 0** (serial):

| Card | Delivers |
|---|---|
| [p0-keel.md](p0-keel.md) (K1, K2) | The repository, the build, the cloud bootstrap, CI; `tee_host`; the reference playing to a WAV; the measurements |
| [p0-split.md](p0-split.md) (K3) | The new contract over the reference (K3a), then the reference split into modules (K3b) |

**Wave 1** (parallel; A, B and J start at once, C after K2, I, F, E and H standalone at once and integrated after
K3b, D and G after K3b):

| Card | Package | Owns |
|---|---|---|
| [w1-a-funkgui-desktop.md](w1-a-funkgui-desktop.md) | FunkGui without JUCE: `FunkGui::gpu` split, `DesktopHost` on SDL3, menus, CI | FunkGui |
| [w1-b-shell.md](w1-b-shell.md) | The application: audio device, keyboard, mouse and gamepad, the loop, headless, artefacts | `shell/`, `app/` |
| [w1-c-host-tools.md](w1-c-host-tools.md) | Schema, `SimHost`, the build document, reload, the dyno runner, tools, scenarios, validation | `host/`, `tools/` |
| [w1-d-mechanics-layouts.md](w1-d-mechanics-layouts.md) | Layouts as data, moving masses, friction, the engine dyno; the integrator | `sim/core/`, `sim/engine.fm` |
| [w1-e-valvetrain-intake.md](w1-e-valvetrain-intake.md) | The valvetrain lab; the induction | `sim/valvetrain/`, `sim/intake/` |
| [w1-f-boost.md](w1-f-boost.md) | Turbos and superchargers as machines in the gas path | `sim/boost/` |
| [w1-g-exhaust.md](w1-g-exhaust.md) | The exhaust builder | `sim/exhaust/` |
| [w1-h-combustion-ecu.md](w1-h-combustion-ecu.md) | Fuels, mixture, knock; the ECU's maps, limiters, launch, overrun | `sim/combustion/`, `sim/ecu/` |
| [w1-i-vehicle.md](w1-i-vehicle.md) | The straight-line car: tyres, loads, drive, gearboxes, the strip | `sim/vehicle/` |
| [w1-j-ui-kit.md](w1-j-ui-kit.md) | The interface kit: TestingPlayground's panel code, build-time shaders, widgets, the binding, the frame | `ui/`, `shaders/` |

**Gate**: checkpoint M2, integration, the user's ears.

**Wave 2**: [w2-cards.md](w2-cards.md), in outline until the gate: performance (early), the listener and the
cabin, mechanical voices, two-strokes, heavy diesels, motorcycles, the views (two packages), content and the
manual, the wheel, release.

## How a package is run

Each package is one agent in a cloud session (or, for the first few, a local agent on the lead's machine), on
the integration branch `claude/awesome-gates-ntdb58`, touching only its card's paths. It is given: this
repository, the two public repositories it may read (FunkGui, FMinus; TestingPlayground where its card says),
its card's name, and the lead's session to report to. It ends with the handoff of 01-rules.md §6.
