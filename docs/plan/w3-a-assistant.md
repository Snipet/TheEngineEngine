# W3-A: the assistant (Claude in the app). Abstract plan, reviewed

The owner's ask (25 Nov): "A Claude chat integration using either an API key from Anthropic or Claude Desktop via MCP.
Look at the TestingPlayground repo for this logic, as that repo contains most of the code for this already."

Status: **abstract plan, reviewed once, nothing built.** The lead wrote it from a read-only map of TestingPlayground at
47bac40 and this tree at d0a7330e; an independent reviewer then read it against this tree's headers and rules
(25 Nov) and broke several of its first claims. This is the version after that review; what changed is marked
**(review)**. The card's detailed plan follows from it.

## What it is

Claude can read the build on the bench, change it, run it and say what it found. Two ways in, one tool table:

1. **From Claude Desktop, over MCP.** Claude Desktop drives the running app. No key in the app, no network from the app.
2. **In the app, by the owner's Anthropic API key.** A chat panel; a turn streams from `api.anthropic.com`.

Both are **off until the owner turns them on** (a switch "Allow Claude Desktop"; a saved key). Off, the app listens on
nothing and connects to nothing. **(review: the first draft said "opens no connection" while planning a listener.)**

## What TestingPlayground gives, and what it does not

| Piece (TestingPlayground) | For this app |
|---|---|
| `ChatPort.h`: the contract between the chat and its view (plain C++; the view polls a revision counter) | taken in shape |
| `ChatView`, `ChatItems`, `ChatStyle`, `Markdown` (FunkGui, no JUCE) | ported into ui/ against this app's kit, listed in a SEED file |
| `claude::Transport` (post / get with a cancel flag), the libcurl transport, `SseParser` | taken |
| `MessageBuilder`, request building, the turn loop, retries, stop reasons | **the mechanism rewritten on nlohmann/json** (there it is `juce::var`, in a 6,800-line file bound to that app's tools) |
| The MCP bridge executable (stdio JSON-RPC), the private socket to the app, `McpLink`, the `.mcpb` bundle | same design, rewritten without juce_core, **with a peer check it does not have (review)** |
| `ChatStore` (the key sealed in a file by a key derived from the machine id) | **not taken as the default (review):** the machine id is world-readable, and it needs a crypto library this tree may not add as a system dependency |
| `FakeClaude.h`: a scripted transport replaying recorded streams | the way every test here talks to "the API" |
| Its tools, system prompt, planner, other providers | not taken |

Its `docs/chat-audit.md` (84 findings on this code) is read before any piece is reused; STORE-1, UI-4, TURN-1, TURN-3
and DOCS-2 become tests or rules here.

## Shape

- **`assistant/`, its own library** (C++20, as the tree is): no SDL, no FunkGui, no simulator.
- **The seam is the document, not the simulator's command ring (review).** `Build`, the build files and library, the
  driver and the predictor's service belong to the frame's thread and have no lock. So the first thing built is **an
  inbox**: the assistant's worker posts a request, the frame drains it in `beforeTick` beside the script's events, and
  a future carries the answer back. `AssistantHost` is that inbox's interface. What is already safe from a worker
  (`SimHost::telemetry` and `readSpec`; a dyno or a predicted curve on a private copy of the build with a cancel
  flag) is called directly.
- **A table of tools before any tool is written (review):** for each, the host call behind it, its thread, how it is
  cancelled, its longest seconds. A tool with no host call behind it today is a named change in another package's
  paths, asked of the lead, not improvised.

### The tools, first version

- read: the status (build's name, running state, a telemetry snapshot); a field or a section by its Schema entry
  (label, help, unit, range, value); the build's Schema-known fields as JSON; the library's names; a dyno run or a
  predicted curve on a private copy (never the ribbon's own request, which a second request would cancel).
- act: set scalar fields (a batch as **one undo step**); load a build from the library; save as a **new** name.
- **Cut from the first version (review):** tables and curves, pictures, the sound, the validation rows, holding a
  speed, starting and stopping the engine, compaction, the model picker, chat persistence, the Windows link.

### What a tool may never do, and what it must ask

- **Never:** remove a build; take or return a path (library names only); change quality, the audio device, the
  recorder or the settings; return a whole-window picture; touch a hidden field, `rev` or `ctl`.
- **Asked of the owner in the app first:** anything that makes sound or moves the live engine (when those tools
  come); loading over unsaved changes.
- **Rejected, not clamped (review):** a value outside its Schema range is refused and the stored value returned.
  Today `Fields` clamps silently; the assistant's path does not use that.
- **Undo that is true (review):** today `Build::undo()` pops whatever is on top, a batch landing mid-drag joins the
  open gesture, a load moves the file and the clean mark that undo does not restore, and save-as overwrites silently.
  So: `Build` gives each step an id (a contract change, the lead's); a card's Undo is live only while its step is on
  top; a batch waits while a gesture is open; save-as refuses an existing name; load asks when the build is dirty.
- **What tools return is content, not instruction.** Only Schema-known values; a build's notes only on request,
  capped and labelled as the file's text; errors without paths. Over MCP the app is otherwise a way to inject text
  into a session that may hold the owner's mail and files.
- **Limits on a turn:** rounds, seconds and tokens, each capped; one driver at a time (the chat or Claude Desktop).

### The MCP path (first)

A small executable beside the app is the MCP server Claude Desktop starts (from a `.mcpb` the build packs). It forwards
over a Unix-domain socket in a private folder to the running app. **The listener exists only while "Allow Claude
Desktop" is on; the app checks the peer's user (`getpeereid` / `SO_PEERCRED`); every change made over MCP shows in the
app as a card with Undo; macOS and Linux first, no Windows link in this version (review).**

### The key path (second)

- **The key (review, the default reversed):** the system's store first (the Keychain on macOS, libsecret on Linux;
  Windows with its phase), refusing rather than falling back to a weaker file. Typed into a dialog, shown masked, never
  logged, never on a command line, never in a build file, a session, a fixture or a commit.
- **The transport (review):** libcurl from the macOS SDK and from the system on Linux (one package line, the lead's);
  on Windows WinHTTP or nothing in this version. FMinus's own curl build is not usable here (archives by URL, built at
  configure, no CA store). A loopback-server test proves the transport without leaving the machine.
- **The request**, per the current Claude API reference, each line checked against it again by the implementer and
  none from memory. The detailed plan pins: how an input is validated before its tool runs (and whether small inputs
  take strict schemas instead of eager streaming); that nothing runs after `max_tokens` or a refusal; that on cancel
  the partial message is dropped whole and every tool call already appended gets a result; that changing state rides
  in the newest user message only, the system prompt byte-stable for a chat's life; what a fallback strips; attempts
  and a ceiling on `retry-after`; errors arriving mid-stream.
- **The model:** the reference's current default, held in one place. Whether a model id may stand in this repository
  at all is the owner's (01-rules.md §"No model names"), asked below.
- **Why the Messages API with our own loop, not Managed Agents:** the tools act on the running app's own state on the
  owner's machine with the owner's key, C++ has no official SDK, and the loop exists in TestingPlayground.

## Proof

- Every test uses the scripted transport or a loopback server. **No test leaves the machine; CI never holds a key.**
- The inbox and the tools are proved on `tee-headless`'s simulated clock with a real host, not a mock.
- The MCP path: the bridge and the link over a socket in a directory the test made; a peer of another user refused.
- macOS: built by CI. **A real Claude Desktop and a real key are tried only by the owner on their Mac**; until then
  every hand-off says "never run against Claude Desktop" or "never run against the API".

## Phases, each landing green by itself

1. **A1: the inbox, `AssistantHost` over the document, the tool table and its tools, on tee-headless.** No network, no
   key, no view. Needs from the lead: the root CMake entry, the step id on `Build`, the SEED list.
2. **A2: the MCP bridge, the link, the switch, the cards with Undo, the `.mcpb`.** (review: before the key chat. It
   needs no curl, no key store, no stream parser and no chat view, and Claude Desktop's own prompts keep a person in
   the loop.)
3. **A3: the key chat:** the transport, the key store and dialog, the turn loop, the chat panel in U1's frame.
4. **A4: what was cut,** each on its own: pictures, the sound, the live engine, tables, Windows.

## The owner's to answer (defaults taken otherwise)

1. Claude may **change** the build, every change one undoable step. (Otherwise: read and explain only.)
2. **Claude Desktop first, the in-app key chat second.** (The reviewer's order; the first draft had it the other way.)
3. The key in the **system's keychain**, not TestingPlayground's sealed file.
4. Claude never starts the engine or overwrites a saved build without being asked in the app.
5. A default model id written in the app's code: allowed? (The repository's rule against model names was written
   for commit messages; the key chat needs one constant.)
6. Windows after macOS and Linux.
