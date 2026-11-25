# Card A: the assistant. Hand-off, A1 and A2 (25 Nov)

Branch `claude/awesome-gates-ntdb58`; plan [w3-a-assistant.md](../plan/w3-a-assistant.md), detail
[w3-a-detailed.md](../plan/w3-a-detailed.md). **Never run against Claude Desktop** until the owner has tried the steps
below on their Mac: nobody in the fleet can.

**Done** (tests/assistant: `assistant.sim`, one process, ~25 s CPU; the inbox cases discovered; shell's settings test):
- A1: `Inbox` (bounded, never blocks a worker; an abandoned job never runs; close answers all); nine tools; `Turn`
  limits (24 calls, 300 s, 32 KB, 256 KB: judgement); one driver at a time; the curves yield to the ribbon and the bake.
  On a real simulator, document and library at 60 simulated frames a second: a batch is one undo step; out of range
  refused with the stored value; hidden, rev, ctl, tel, scope unreachable; no path in or out; a load over unsaved
  changes asks (refused with no one to ask); save-as never overwrites; a batch waits for a held knob.
- A2: `tee-mcp` (the MCP server), `McpLink` (0700 folder, each peer's user checked, calls in order, cancel), `Cards`
  (Undo live only while its step is on top), `Session`, `TheEngineEngine.mcpb` (target `tee-mcpb`). End to end with
  the real bridge as a child process: initialize, honest annotations, a change with its card and Undo, a cancel never
  answered, switch off/on (list_changed), another user's peer, an open folder and a second app refused, shutdown
  mid-call under 2 s. Switch off (the default) costs nothing: no thread, no socket, no folder.
- Wiring (the lead's yes, 11:58): `Kept` (app/Offline.cpp) holds the `Session`, destroyed first, drained by the frame
  with the app's busy flag; app/Main.cpp turns the link on when settings say `"allowClaudeDesktop": true` (never in a
  machine's run) and puts Claude Desktop's line on the status line; shell/settings has the key (default false).

**What Claude Desktop can do today, and what the owner sees.** Read the build, its fields, the library; change scalar
fields (each batch one step the app's Undo takes back); load a build only when the open one is saved; save under a new
name; run the two curves on a copy. The owner sees the dials move, the title's star, and the status line "Claude
Desktop connected, N changes" ("working: set_fields" during a call). No card is drawn until U1's strip lands.

**Not done:** U1's switch row, cards strip and yes/no dialog (after the push); A3 (the key chat); Windows (no link).
**Outside my paths:** host/ Build step ids (e260d2b); the root CMake line; the wiring above; all ruled yes.
**Not run here:** macOS (CI only) and Claude Desktop. The MCP protocol versions and the manifest's shape are
TestingPlayground's at 47bac40.

**For the owner (macOS):**
1. Build the target `tee-mcpb`; the extension is `build-dev/TheEngineEngine.mcpb`. Double-click it to install.
2. Until the app has its switch, set `"allowClaudeDesktop": true` by hand in
   `~/Library/Application Support/TheEngineEngine/settings.json`, then open TheEngineEngine (its console says
   "Claude Desktop: allowed").
3. Ask Claude "What engine is open?", then "Raise the idle a little": the dial moves, the status line counts it, the
   app's Undo takes it back. Set the key back to false and reopen: Claude says the app is not reachable.
4. If anything fails, send `~/Library/Logs/Claude/mcp*.log`.
