# W3-A, phase A1: the inbox and the tools. Detailed plan

*Card A (the assistant). Follows [w3-a-assistant.md](w3-a-assistant.md), the reviewed abstract plan; where they differ,
that file wins and this one is corrected. A1 has no network, no key and no view.*

## Where it lives

- `assistant/`: the library `tee_assistant` (`Tee::assistant`), C++20, on `Tee::host`, `Tee::shell_files` (the
  document on disk and the library of builds) and nlohmann/json. No SDL, no FunkGui, no simulator code of its own.
  Headers `assistant/include/tee/assistant/*.h`, sources `assistant/src/`.
- `tests/assistant/`: `tee-tests-assistant`, added from `assistant/CMakeLists.txt` when `TEE_BUILD_TESTS` is on, so the
  root needs one line: `add_subdirectory(assistant)` after `add_subdirectory(shell)` (the lead's).
- The step id on `Build` (below) is the lead's; prepared as a diff, pushed on the lead's word.

## The inbox

`Build`, `BuildFile` and `BuildLibrary` belong to the frame's thread and have no lock. Everything a tool does to them is
a **job** posted by the assistant's worker (the MCP link's thread in A2, the chat's in A3) and run by the frame.

- **Post (the worker).** `Inbox::post(job)` puts the job in a bounded queue and returns a `Ticket`: a future of the
  job's reply. It never blocks. **A full queue** (32 jobs) answers at once, "the app is busy; try again", without
  queueing: a worker never waits on a frame that is not draining.
- **Drain (the frame).** `Inbox::drain()` is called once a frame in `beforeTick`, beside the script's events, and runs
  at most 8 jobs a frame in the order posted; the rest wait for the next frame, so the frame never stalls behind the
  assistant. Each job is short (no disk beyond the library's own reads and one save, no simulation).
- **Answer (the worker).** The worker waits on the ticket for at most the tool's limit. Each ticket has a state:
  *waiting*, *running*, *done*, *abandoned*. The frame moves waiting to running before it runs a job; a worker whose
  wait timed out moves waiting to abandoned and answers "timed out, nothing was changed". If the frame got there first
  (running), the worker waits for the job to finish (it is one frame's work) and reports what it did. So a reply is
  never wrong about whether the build changed.
- **Asking the owner.** A job never waits on a person. A tool that must ask (a load over unsaved changes) posts a job
  that registers the question with the app's `Asker` (U1's card in A2; a scripted answer in tests) and returns its
  future; the worker waits on that (at most 60 s), then posts the act itself, which first checks the build is as it
  was asked about (its hash and dirty flag): changed meanwhile, refused, "the build changed while you were asked".
- **Shutdown.** The owner of the link or chat stops its worker first (cancelling the tool running: its cancel flag).
  Then `Inbox::close()` on the frame: new posts are refused ("the app is closing") and every queued job is answered the
  same without running. Then the document goes. The inbox outlives every worker that posts to it.
- **Rebinding.** A quality change rebinds the document to new Fields. Jobs read `build.fields()` when they run, never a
  pointer kept from an earlier frame.

`AssistantHost` is the interface the tools see: each method posts a job (or reads what is safe from any thread) and
returns a reply. `DocumentHost` is it over the app's `BuildFile`, `BuildLibrary`, the `SimHost` and the `Sim` (for
the private copies).

## The tools, first version

Each tool's input is validated against its schema before it runs (unknown keys refused). Replies are JSON of
Schema-known values only; errors are the assistant's own words, never a path or a library error passed through.

| Tool | Host call behind it | Thread | Cancelled by | Longest |
|---|---|---|---|---|
| `get_status` | `Build::name`, `BuildFile::dirty`, `SimAdapter::ready`, `SimHost::telemetry` (engine speed, state, gear, manifold, boost: the gauges' Schema-known fields) | job | its ticket's timeout | 2 s |
| `describe_field` {path} | `Fields::find`: label, help, unit or quantity, range, choices; `Build::get` | job | timeout | 2 s |
| `get_build` {section?} | `Fields::all` (Spec, not hidden, not `rev`, numbers, switches, choices; tables named but not given), `Build::get` | job | timeout | 2 s |
| `list_builds` | `BuildLibrary::list`: name, title, part, layout, displacement, shipped or own. Never a file | job | timeout | 2 s |
| `set_fields` {changes: [{path, value or choice}]} | `beginGesture`, `Build::set` / `setChoice` each, `endGesture`: one step; returns the step id | job, which **waits** (re-posted next frame, at most 5 s) while `Build::gestureOpen()` | timeout | 5 s |
| `load_build` {name} | `BuildLibrary::load(BuildFile&, name)`; asks the owner first when `BuildFile::dirty()` | job, the ask, job | timeout; a declined or unanswered ask refuses | 60 s with an ask, 2 s without |
| `save_build_as` {name} | `BuildLibrary::validName`, `BuildLibrary::summary` (any build of that name: refused), `BuildLibrary::save` | job | timeout | 2 s |
| `predict_curve` | a job copies the document's values into a private `Build` over the `Sim`'s Fields; then `predictCurve(sim, copy, options, cancel)` on the worker (never `PredictService`: that is the ribbon's, and a second request would cancel it) | job, then worker | an atomic flag: MCP cancel, turn stopped, shutdown | 20 s |
| `run_dyno` {from, to rpm} | the same copy; `runDyno(sim, copy, plan, cancel)`, a sweep, plan bounded (from and to within idle and redline) | job, then worker | the flag | 60 s |

**`set_fields` is all or nothing.** Every change is checked first: the path names a Spec field (with or without
"spec."), not hidden, not `rev`, not a table or array; a number within its `fminus::range` and finite; an integer whole;
a choice by its name or index. One bad change refuses the batch: the reply names it, with its range and **the stored
value**; nothing is set (never clamped). A good batch is one undo step.

**Never** (each a test): remove a build; take or return a path (a name with a separator, `..`, a leading dot or a drive
is refused; no reply holds the library's directories); change quality, the audio device, the recorder or the settings
(no tool exists for them); a picture; a hidden field, `rev`, anything under `ctl`, `tel` or `scope` written.

**Asked of the owner:** a load over unsaved changes. (Sound and the live engine: no tool yet.)

## The curves yield to the app's own (the lead's ruling, 25 Nov)

`predict_curve` and `run_dyno` run the simulator flat out on worker threads while the audio thread plays. The app's
own heavy work already shares: the calibration bake waits between its points while the ribbon's curve is wanted
(`CalibrationBakeService::yieldTo`, `PredictService::busyFlag`). The assistant joins at the bottom of that order:
`Host::othersBusy` (the app gives it the ribbon's busy flag, or the bake's `state()` Baking or Checking; both read
from any thread) is polled before a curve starts and between its points (`PredictOptions::onPoint`; the dyno's
progress callback). So the assistant's run never starts beside them, and pauses within a point (one a worker) when they
start; nothing in the app ever waits on the assistant, so there is no cycle to deadlock. Still busy when the tool's
limit runs out: refused, "the app is busy with its own curve or calibration". Tested with a real `PredictService` as
the ribbon and a flag as the bake.

## Turn limits and one driver

- **One driver at a time** (TestingPlayground's TURN-3). A `Driver` lock: the chat or Claude Desktop holds it for a
  turn (a Desktop call is a turn of one round); the other is refused at once, "Claude Desktop is using the build".
- **A turn's limits** (judgement, not measurement: the lead's word), each a constant in one place, checked before every tool call: at most 24 tool calls, 300 s of
  the frame's clock, 256 KB of tool results in all and 32 KB in one (a larger result is cut and says so). In A3 the
  tokens of the request join them. The clock is injected: the tests run it on the simulated frames.

## Proof (A1)

`tests/assistant/`, one process with one compiled simulator (the shared-Sim pattern of tests/shell), a real
`SimAdapter`, `BuildFile` and `BuildLibrary` (shipped: content/builds; the user's: a directory the test made), and a
frame loop at 60 frames a second that drains the inbox, calls `frame()` and plays the frame's audio, as tee-headless
does; the tools called from a worker thread. Properties: a batch is one undo step and undo restores every value; an
out-of-range value is refused and the stored value returned, nothing changed; a hidden field and `rev` and `ctl` are
unreachable; no reply holds a path and no tool takes one; a load over a dirty build asks (yes loads, no and silence
refuse); save-as refuses an existing name, shipped or own; the turn limits hold; a full inbox refuses at once; a
closed inbox answers every waiting job; a job abandoned by its timeout never runs; a batch waits for an open gesture.

## The change asked of the lead (host/)

`Build` gives each undo step an id: `undoStep()` (the one on top, 0 none), `redoStep()`, and `gestureOpen()`. Ids are
counted from 1, never reused, kept through undo and redo, dropped with the steps on a rebind that drops them. Append
only: 20 lines in `Build.h` and `Build.cpp`, no signature changed.
