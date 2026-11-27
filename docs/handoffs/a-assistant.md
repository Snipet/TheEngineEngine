# Card A: Claude in the app. Hand-off (25 Nov)

**Never run against Claude Desktop, and never run against the API with a real key: nobody in the fleet can. The owner's
try on their Mac is the first.**

## For the owner: turning Claude Desktop on (macOS)
1. Build the target `tee-mcpb`, then double-click `build-dev/TheEngineEngine.mcpb`: Claude Desktop installs the
   extension "TheEngineEngine". (One downloaded from CI is quarantined: first run
   `xattr -d com.apple.quarantine TheEngineEngine.mcpb`, or after a refusal System Settings, Privacy & Security,
   "Open Anyway".)
2. In the app, turn on "Allow Claude Desktop" (U1's switch row; until it is drawn, set `"allowClaudeDesktop": true`
   in `~/Library/Application Support/TheEngineEngine/settings.json` and open the app: its console says "Claude
   Desktop: allowed"). Off, the app listens on nothing.
3. Ask Claude "What engine is open?", then "Raise the idle a little".
4. If anything fails, send `~/Library/Logs/Claude/mcp*.log`. Its "tee-mcp" lines name the folder the extension
   looked in ("the app is not reachable at .../TheEngineEngine-mcp/app.sock: ..."); `tee-mcp --where`, run in a
   Terminal, prints the folder it would use.

**Extension 0.2.0 (27 Nov): reinstall it.** 0.1.0 looked for the app in a folder taken from its environment, which
Claude Desktop does not pass: with the app open and the switch on it still said "not open". 0.2.0 finds the folder
from the system (the per-user temporary folder), the same from both ends. Build `tee-mcpb` again and double-click the
new `TheEngineEngine.mcpb` (Claude Desktop replaces an installed extension only with a newer version), and use an app
built from the same commit.

**What you will see.** Claude reads the build and the library, changes fields (each batch one step the app's Undo takes
back whole), loads a build only when the open one is saved, saves under a new name (never over one), and runs the two
curves on a private copy. In the app the dials move, the title gains its star, and the status line says "Claude
Desktop connected, N changes" ("working: set_fields" during a call). Cards with their own Undo come with U1's strip.

**The key chat** (once U1 mounts its panel and key dialog): save your key in the dialog (the Keychain; macOS asks
once); Enter sends, Escape stops; Claude's thinking shows as a dim summary while it works.

## What has never been run
Claude Desktop itself; the real API with a real key; the Keychain; macOS by hand (CI builds and tests it); Windows (no
link and no key chat in this version: both say so in words). MCP: tee-mcp speaks 2024-11-05 to 2025-11-25 (checked
against the 2025-11-25 specification); to a client of 2026-07-28 it answers `server/discover` with -32601, which that
revision reads as "legacy, use initialize": a client of both eras works, one of 2026-07-28 alone does not. The manifest
is checked against the MCPB reference 0.3 and parsed by a test.

## For the fleet
- Code: `assistant/` (Tee::assistant, tee-mcp, tee-mcpb, Tee::assistant_view), tests in `tests/assistant/`
  (`assistant.sim` one process ~30 s CPU; the rest discovered). Plan: docs/plan/w3-a-assistant.md, w3-a-detailed.md.
- U1: the switch row (`Session::allowDesktop`, `desktopAllowed`, `statusNote`, `desktop()`), the cards
  (`Session::cards()`), the chat (`view::ChatView` over `Session::chat()`; the key dialog: `saveKey`, `keyMask`,
  `removeKey`), a yes/no dialog for `SessionParts::ask` (until then a load over unsaved changes is refused).
- B: `std::signal(SIGPIPE, SIG_IGN)` in app/Main.cpp's and app/Headless.cpp's main (the library raises none itself).
- J2: an Enter-only multi-line TextField and an error colour in the kit's Look (on the branch at 330c98f0: the chat
  moves to them in a later change).
