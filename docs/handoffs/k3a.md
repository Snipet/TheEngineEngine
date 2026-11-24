# K3a: the new contract over the monolith (landed)

Package K3a of [p0-split.md](../plan/p0-split.md), branch `claude/awesome-gates-ntdb58`, commits dc45620 (sim), 1e8bdfe
(host, tools), 7a58532 (tests, builds). K3b (the split) follows module by module.

- **sim/engine.fm**: `module Engine { Spec spec; Controls ctl; Telemetry tel; Scope scope; init; process
  [[clang::nonblocking]]; end }`, the blocks in `sim/spec/` (the first Spec: the reference's dials by section, SI for
  the displacement and the air; `limits.fm`, `tables.fm` with `Curve16`/`Map16`). Inside, the reference's `Shared`
  unchanged in `sim/core/monolith.fm`, driven by an adapter. [sim/README.md](../../sim/README.md) has every field, the
  mapping from Controls to the reference's keys, and how the null test gives both programs the same inputs.
- **host**: `tee::EnginePlayer` (single-threaded: Spec by path with range and rev, Controls struct, Telemetry by
  path), builds (`tee-build` JSON), engine scenarios (`tee-engine-scenario/1`), `tee::playTwin` (simulator and
  reference side by side). All in `host/include/tee/EnginePlayer.h`.
- **Render**: `build-dev/bin/tee-render --engine --build v8 --scenario free-revs` (builds in `content/builds`,
  scenarios in `tests/sim/scenarios`; `--engine --help`, `--engine --defaults`, `--engine --list`).
  Bench: `build-dev/bin/tee-bench --engine`.
- **Null test** (`tee-tests "[null]"`): 4 builds x 4 scenarios at 48 kHz/512, plus 44.1 kHz/333 and 96 kHz/64:
  every one identical to the bit. Cost: 1.011 of the reference (dev build, 2 builds x 2 scenarios, least of 2).
