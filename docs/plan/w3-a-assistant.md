# W3-A: the assistant (a Claude chat in the app). Abstract plan, for review

The owner's ask (25 Nov): "A Claude chat integration using either an API key from Anthropic or Claude Desktop via MCP.
Look at the TestingPlayground repo for this logic, as that repo contains most of the code for this already."

Status: **abstract plan, not reviewed, nothing built.** The lead wrote it from a read-only map of TestingPlayground at
47bac40 (a local agent's report, 25 Nov) and from this tree at d0a7330e. A detailed plan and one implementation card
follow its review.

## What it is

A chat in TheEngineEngine where Claude can read the build on the bench, change it, run it and measure it, and say what
it found. Two ways in, one tool table:

1. **In the app, by the owner's Anthropic API key.** A chat panel; a turn streams from `api.anthropic.com`.
2. **From Claude Desktop, over MCP.** Claude Desktop drives the running app with the same tools.

With no key saved and no Claude Desktop connected the feature does nothing and opens no connection.

## What TestingPlayground gives, and what it does not

| Piece (TestingPlayground) | For this app |
|---|---|
| `ChatPort.h`: the contract between the chat and its view (plain C++; the view polls a revision counter) | taken in shape |
| `ChatView`, `ChatItems`, `ChatStyle`, `Markdown` (FunkGui, no JUCE) | ported into ui/ against this app's kit |
| `claude::Transport` (post / get with a cancel flag), the libcurl transport, `SseParser` | taken; libcurl on every platform here |
| `MessageBuilder`, request building, the turn loop, retries, stop reasons (`ClaudeApi`, `ChatAgent`) | **the mechanism rewritten on nlohmann/json** (there it is all `juce::var`, and `ChatAgent.cpp` is 6,800 lines bound to that app's tools) |
| The MCP bridge executable (stdio JSON-RPC), the private Unix-domain socket to the app, `McpLink`, the `.mcpb` bundle | same design, rewritten without juce_core |
| `ChatStore` (the key in an encrypted file, 0600; libsecret on Linux) | same scheme, rewritten without `juce::File` and JUCE's device id |
| `FakeClaude.h`: a scripted transport replaying recorded streams | the way every test here talks to "the API" |
| Its tools, its system prompt, its planner, its other providers | not taken: this app's own tools and prompt; one provider |

`docs/chat-audit.md` there (84 findings on exactly this code, most since fixed) is read before any piece is reused.

## Shape

- **`assistant/`, its own library `tee_assistant`**: no SDL, no FunkGui, no simulator. It holds the transport, the
  stream parser, the conversation (kept in the Messages API's own shape), the turn loop on one worker thread, the key
  store, and the tool table. It reaches the app only through one interface, `AssistantHost`, which the app implements
  over host/ (`Commands.h`, `Schema.h`, `Fields.h`, `Predict.h`, `Dyno.h`, `Telemetry.h`, `Validation.h`, `Render.h`).
  The audio thread never sees it.
- **The tools** (one table; the MCP path offers the same ones less `ask_user`):
  - read: the status (build, running state, a telemetry snapshot); a field or a section by its Schema entry (label,
    help, unit, range, value); the build as JSON; the garage's list; the predicted curve and the last dyno run; the
    validation rows; a picture of a workspace.
  - act: set fields (a batch, each checked against its Schema range, **one undo step**, shown in the chat as a card with
    Undo); load a build; save as (into the user's library only); start, stop, hold a speed and a pedal; run the dyno.
  - later (its own phase): measure the sound (a render's level and spectrum, as numbers and a picture).
  - never: a shell, a file outside the build library, a host other than `api.anthropic.com`.
- **The request**, per the current Claude API reference (read 25 Nov; the implementer verifies each line against it
  again, and model ids, the version header and beta names never come from memory): `POST /v1/messages`, streamed;
  the default model `claude-opus-5-5`, the list from `GET /v1/models`, the owner's choice kept; adaptive thinking
  with its summary shown; the effort set explicitly; client tools with `eager_input_streaming`, **each input
  validated here against its schema before it runs**; `tool_choice` auto only; a turn's tool results in one message,
  a failed one marked as an error; the assistant's content appended unchanged; the tools and the system prompt
  stable and cached, the changing state after them; the server-side fallback on; `refusal`, `pause_turn` and
  `max_tokens` each handled; retries on 429 and 5xx by `retry-after`; no redirect followed; compaction for a long chat.
- **Why the Messages API with our own loop, not Managed Agents:** the tools act on the running app's own state on the
  owner's machine with the owner's key, C++ has no official SDK (so raw HTTP), and the loop exists in TestingPlayground.
- **MCP**: a small executable beside the app is the MCP server Claude Desktop starts (from a `.mcpb` the build packs);
  it forwards over a socket in a private folder to the running app, which runs the same tool table. No TCP port.
- **The key**: typed into a dialog, shown masked, kept as TestingPlayground keeps it. Never logged, never in a build
  file, a session, a test fixture or a commit. The dialog says in one sentence what a chat sends to Anthropic (the
  conversation, the build's fields, what the tools return) and that it is sent only when the owner uses the chat.
- **The view**: a chat panel in U1's frame (its place is U1's), with a Stop button and the usage of each chat.
- **The system prompt**: this app's own, built from the Schema's help text so it stays true: what the bench is, the
  units, to change little and say what it changed, to tell measured from assumed.

## Proof

- Every test uses the scripted transport: request shapes, the loop, retries, a refusal, a cancel mid-stream, an input
  that fails its schema, a tool that throws. **No test opens a connection off the machine**, and CI never holds a key.
- The MCP path: the bridge and the app's link over a socket pair in a directory the test made.
- The key store: a round trip; a wrong machine id refuses.
- macOS and Windows: built by CI. **A real key and a real Claude Desktop are tried only by the owner on their Mac**;
  until then the hand-off says "never run against the API".

## Phases, each landing green by itself

1. A1: `tee_assistant` with the scripted transport, the tool table over `AssistantHost`, its tests. No view, no network.
2. A2: the libcurl transport, the key store and its dialog, the chat panel.
3. A3: the MCP bridge, the link, the `.mcpb`.
4. A4: the sound tools.

## Open, for the detailed plan

- libcurl per platform: the macOS SDK's; on Linux and Windows either the system's or the pinned build FMinus's web
  library already makes (`cmake/FMinusWeb.cmake`, Schannel on Windows, never run there). The transport interface keeps
  the choice in one file.
- The device id without JUCE (machine-id; the platform UUID on macOS; MachineGuid on Windows).
- Which host calls are safe from the assistant's worker, and which must be queued to the frame.

## The owner's to overrule (defaults taken otherwise)

1. Claude may **change** the build, every change undoable. (Otherwise: read and explain only.)
2. The default model is `claude-opus-5-5`, with a picker.
3. The key in an encrypted file as in TestingPlayground; the system keychain later.
4. One provider (Anthropic); TestingPlayground's other providers and its planner are left out.
