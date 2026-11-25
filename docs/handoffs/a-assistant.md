# Card A: the assistant. Hand-off, A1, A2 and A3 (25 Nov)

Branch `claude/awesome-gates-ntdb58`; plan [w3-a-assistant.md](../plan/w3-a-assistant.md), detail
[w3-a-detailed.md](../plan/w3-a-detailed.md). **Never run against Claude Desktop** until the owner has tried the steps
below on their Mac: nobody in the fleet can. **Never run against the API** with a real key until the owner has.

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
- A3, the key chat: `Transport` (libcurl; https only, verified, no redirect, cancellable; loopback-tested), `Chat`
  (the Messages API loop, checked against the reference; 10 cases on scripted streams), `KeyStore` (Keychain, Secret
  Service, else none; masked), the panel `ChatView` (assistant/view, TestingPlayground's markdown ported: SEED.tsv; 6
  view cases). `Session::chat()`, `saveKey`, `keyMask`, `removeKey` are what U1 mounts and its key dialog calls.

**What Claude Desktop can do today, and what the owner sees.** Read the build, its fields, the library; change scalar
fields (each batch one step the app's Undo takes back); load a build only when the open one is saved; save under a new
name; run the two curves on a copy. The owner sees the dials move, the title's star, and the status line "Claude
Desktop connected, N changes" ("working: set_fields" during a call). No card is drawn until U1's strip lands.

**Not done:** U1's switch row, cards strip, yes/no dialog, the chat panel's place and the key dialog (after the push);
Windows (no link, no key chat). Linux needs `libcurl4-openssl-dev libsecret-1-dev` (asked of the lead).
**Outside my paths** (all ruled yes): host/ Build step ids (e260d2b); the root CMake line; the wiring. **Not run
here:** macOS (CI only), Claude Desktop, the API. MCP versions and the manifest's shape: TestingPlayground's at 47bac40.

**For the owner (macOS):**
1. Build the target `tee-mcpb`; double-click `build-dev/TheEngineEngine.mcpb` to install it in Claude Desktop.
2. Until the app has its switch, set `"allowClaudeDesktop": true` in `~/Library/Application Support/TheEngineEngine/
   settings.json` and open the app (its console says "Claude Desktop: allowed"). Ask Claude "What engine is open?",
   then "Raise the idle a little": the dial moves, the status line counts it, Undo takes it back. Failures: send
   `~/Library/Logs/Claude/mcp*.log`.
3. The key chat, once U1 has mounted its panel and key dialog: save your key (Keychain; macOS asks once), ask the same,
   stop a long answer with Escape; Keychain Access shows "TheEngineEngine" and no file holds the key.
