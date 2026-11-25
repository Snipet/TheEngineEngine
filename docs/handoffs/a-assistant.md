# Card A: the assistant (Claude in the app). Hand-off, A1 and A2 (25 Nov)

Branch `claude/awesome-gates-ntdb58`. Plan: [w3-a-assistant.md](../plan/w3-a-assistant.md), detail:
[w3-a-detailed.md](../plan/w3-a-detailed.md). **Never run against Claude Desktop** until the owner has tried the steps
below on their Mac; nobody in the fleet can.

**Done (tests/assistant, `assistant.sim` one process ~20 s CPU, and the inbox cases discovered):**
- A1, the inbox and the tools: `Inbox` (bounded, never blocks the worker, abandoned jobs never run, close answers all);
  nine tools (`get_status`, `describe_field`, `get_build`, `list_builds`, `set_fields`, `load_build`, `save_build_as`,
  `predict_curve`, `run_dyno`); `Turn` limits (24 calls, 300 s, 32 KB, 256 KB: judgement) and one driver at a time.
  Proved on a real simulator, document and library at 60 simulated frames a second: a batch is one undo step; out of
  range refused with the stored value; hidden, rev, ctl, tel, scope unreachable; no path in or out; load over unsaved
  changes asks; save-as never overwrites (any case, shipped too); a batch waits for a held knob; the curves yield to the
  ribbon's and the bake's and never deadlock.
- A2, Claude Desktop's path: `tee-mcp` (the MCP server, stdio JSON-RPC, nlohmann/json only), `McpLink` (the app's end:
  private folder 0700, peer's user checked, calls in order as Claude Desktop's turns, cancel), `Cards` (a card a change,
  its Undo live only while its step is on top), `Session` (what the app holds), `TheEngineEngine.mcpb` (target
  `tee-mcpb`). Proved end to end with the real bridge as a child process: initialize, tools/list with honest
  annotations, a change with its card and Undo, a cancel never answered, the switch off and on (list_changed), another
  user's peer closed unanswered, a folder open to others refused, a second app refused, shutdown mid-call in < 2 s.

**Not done:** the frame's wiring (app/Offline.cpp `Kept`, app/Main.cpp, app/CMakeLists.txt: B's, sent to the lead); the
switch's place in the settings file (B); the switch, the cards with Undo and the owner's question dialog in the
frame (U1); A3 (the key chat), not started. Windows: no link (the switch refuses, in words).

**Changes outside my paths:** `host/` Build step ids (e260d2b, ruled yes, held for the freeze); the root CMake line
(in d97da7b, ruled yes); the wiring above.

**Not run here:** macOS (CI builds it: `getpeereid`, `$TMPDIR`, `SO_NOSIGPIPE` paths); Claude Desktop itself. The MCP
protocol versions and the `.mcpb` manifest's shape are TestingPlayground's at 47bac40, which Claude Desktop has run.

**For the owner, once the wiring is in (macOS):**
1. Build (`cmake --workflow --preset dev-verify`); `build-dev/TheEngineEngine.mcpb` is the extension.
2. Double-click it (or Claude Desktop, Settings, Extensions, install from file); Claude Desktop lists TheEngineEngine.
3. Open TheEngineEngine, turn on "Allow Claude Desktop". Ask Claude: "What engine is open?", then "Raise the idle a
   little", and see the card with its Undo; Undo it. Ask it to load another build with unsaved changes: the app asks you.
4. Turn the switch off: Claude says the app is not reachable. Report what Claude Desktop's MCP log
   (`~/Library/Logs/Claude/mcp*.log`) says if anything fails.
