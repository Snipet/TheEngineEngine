# Card A: Claude in the app. Hand-off (25 Nov; brought up to date 27 Nov)

The owner's page is **docs/manual/claude.md** (installing, allowing, the key, what is sent where, what to send
when it fails, what to watch for). This page is the fleet's.

**Never run against Claude Desktop, and never run against the API with a real key: nobody in the fleet can. The owner's
try on their Mac is the first.**

## For the owner: turning Claude Desktop on (macOS)
1. Build the target `tee-mcpb`, then double-click `build-dev/TheEngineEngine.mcpb`: Claude Desktop installs the
   extension "TheEngineEngine". (One downloaded from CI is quarantined: first run
   `xattr -d com.apple.quarantine TheEngineEngine.mcpb`, or after a refusal System Settings, Privacy & Security,
   "Open Anyway".)
2. In the app's SETTINGS, the CLAUDE DESKTOP row: Allow (the choice is kept; the console says "Claude Desktop:
   allowed" at the next start). Off, the app listens on nothing.
3. Ask Claude "What engine is open?", then "Raise the idle a little".
4. If anything fails, send `~/Library/Logs/Claude/mcp*.log`. Its "tee-mcp" lines name the folder the extension
   looked in ("the app is not reachable at .../TheEngineEngine-mcp/app.sock: ..."); `tee-mcp --where`, run in a
   Terminal, prints the folder it would use.

**Extension 0.2.1 (27 Nov): reinstall it.** 0.1.0 looked for the app in a folder taken from its environment, which
Claude Desktop does not pass: with the app open and the switch on it still said "not open". 0.2.0 and later find the folder
from the system (the per-user temporary folder), the same from both ends. Build `tee-mcpb` again and double-click the
new `TheEngineEngine.mcpb` (Claude Desktop replaces an installed extension only with a newer version), and use an app
built from the same commit. (0.2.1 also tells Claude what the app really shows: each change is one step of the
app's Undo, counted on the status line; no card is drawn yet.)

**What you will see.** Claude reads the build and the library, changes fields (each batch one step the app's Undo takes
back whole), loads a build only when the open one is saved, saves under a new name (never over one), and runs the two
curves on a private copy. In the app the dials move, the title gains its star, and the status line says "Claude
Desktop connected, N changes" ("working: set_fields" during a call). No card is drawn yet (Session::cards() has no
caller outside assistant/): Claude is told only of the Undo and the status line.

**The key chat**: Chat opens it in the inspector's column; Key (or "Add a key" in the empty chat) saves the key in the
system's store; Enter sends, Escape stops; New chat starts again; Claude's thinking shows as a dim summary.

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
  `removeKey`) and the yes/no dialog for `SessionParts::ask` (AppFrame::ask, with the withdrawn flag of the 27 Nov
  review) are drawn; the cards are not.
- B: `std::signal(SIGPIPE, SIG_IGN)` in app/Main.cpp's and app/Headless.cpp's main (the library raises none itself).
- J2: an Enter-only multi-line TextField and an error colour in the kit's Look (on the branch at 330c98f0: the chat
  moves to them in a later change).

## Held for the next update (27 Nov, the review's items; committed, waiting for the freeze to lift)
- Item 10: the status line becomes "N changes made", counting field-change cards only. **It does not fall when the
  owner undoes one in the app, on purpose.** Telling whether a card's step still stands needs the build's undo stack
  read below its top: Build gives only undoStep()/redoStep() (the tops), and host/ is not card A's. Do not "fix" it by
  comparing step ids: a step undone and then built over leaves a larger id on top, and the count would lie.
- Items 3 (the withdrawn question; every tool within 55 s), 4 (New chat; the too-long 400), 6, 7 (the history byte
  for byte), 8, 9, 10, and the Keychain read off the frame's thread; the extension then 0.2.2.
