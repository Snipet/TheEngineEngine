# Claude and TheEngineEngine

Claude can work on the build open in the app in two ways:

- **from Claude Desktop**, through a small extension that talks to the running app over a private link on this
  computer;
- **from the chat in the app**, which talks to Anthropic's API with your own API key.

Either way Claude reaches only the open build and the build library, through the same nine tools, and every change
it makes is yours to undo. macOS and Linux; Windows has neither in this version.

## Claude Desktop

### Installing the extension

1. Build the target `tee-mcpb` (or download `TheEngineEngine.mcpb` from CI) and double-click
   `TheEngineEngine.mcpb`. Claude Desktop installs the extension "TheEngineEngine".
   - A file downloaded from CI is quarantined by macOS. Run `xattr -d com.apple.quarantine TheEngineEngine.mcpb`
     first, or after a refusal open System Settings, Privacy & Security, and choose "Open Anyway".
2. Use an extension and an app built from the same commit. Claude Desktop replaces an installed extension only with
   a newer version, so after an update install the new `.mcpb` again. **Version 0.2.3 or later is needed**: 0.1.0
   looked for the app in the wrong folder and always said the app was not open, and 0.2.3 ends every call in time.

### Allowing it in the app

In SETTINGS, the CLAUDE DESKTOP row: **Allow**. Off (the default), the app listens on nothing. The choice is kept for
the next launch.

The status line says what is happening: "Claude Desktop allowed" (waiting), "Claude Desktop connected, 3 changes made"
(the batches of field changes Claude has made, not its loads or saves; it does not fall when you undo one in the app),
"Claude Desktop working: set_fields" while a call runs.

Then ask Claude, in Claude Desktop, "What engine is open?" or "Raise the idle a little".

## The chat in the app

1. Open the chat with **Chat** (the inspector's column becomes the chat).
2. Save your Anthropic API key with **Key**, or with **Add a key** in the empty chat. The key is kept in the
   system's own store: the Keychain on macOS, the Secret Service on Linux. It is never written to a file, a log or the
   settings, and the dialog shows only its last four characters.
3. Type, and press Enter to send. Escape, or the Stop button, stops Claude's answer. While Claude works you see a
   dim summary of its thinking and a row for each tool it uses.

**New chat** (at the top of the panel, once there is a conversation) starts again from nothing. A long conversation
is sent whole each time; once it is longer than Claude can read, the chat says so, and your last message goes back
into the box.

## What Claude can do, and what it cannot

It can:

- read the open build: its sections, any field's value, label, unit and range;
- change fields, in SI units, within each field's range (a value outside it is refused, never clamped). Each batch
  of changes is **one step of the app's own Undo**, and while you hold a control Claude waits for you to let go;
- list the builds GARAGE shows and load one. If the open build has unsaved changes, the app asks you first (**Load**
  or **Keep**; Keep has the focus). If you do not answer within 40 seconds, the question leaves the screen by
  itself and nothing is loaded;
- save the open build under a new name, which never overwrites a build;
- predict the torque curve, or run a dyno sweep, on a private copy (the engine you hear is not touched).

It cannot:

- change tables or arrays (cam profiles, maps) in this version;
- delete, rename or overwrite a build;
- read or write any other file, or anything outside the app;
- change the app's own settings, the audio, or the controls.

## What is sent where

- **Claude Desktop:** the app and the extension talk only on this computer, through a socket in a private folder of
  your user's (no other user can connect). What the tools return becomes part of your conversation in Claude Desktop,
  which Claude Desktop sends to Anthropic as it does any conversation. That includes the build's values and names,
  the gauges and the curves.
- **The chat:** the app sends to `api.anthropic.com`, over HTTPS:
  - your messages;
  - a line giving the app's state (the open build's name, saved or not, the engine running or not);
  - what the tools return;
  - the app's own instructions to Claude, which include the build's section and field labels.

  The key goes only in that request's header. Nothing else leaves the computer.

## When something fails: what to send

- **Claude Desktop says the app is not open, or lists no tools:** send `~/Library/Logs/Claude/mcp*.log`.
  - Its "tee-mcp" lines say which folder the extension looked in ("the app is not reachable at
    .../TheEngineEngine-mcp/app.sock: ...").
  - `tee-mcp --where`, run in a Terminal from the extension's folder, prints the folder it uses.
  - The app's console says "Claude Desktop: allowed" at start, or why not.
- **The chat:** the notice in the chat says what the API answered.
- Never send your key: no log or message contains it.

## To watch for

These are known or suspected, and nobody has been able to check them on a Mac yet:

- **A minimised window.**
  - Until the next update, the app does not answer Claude Desktop while its window is minimised: each call fails
    after 2 seconds.
  - A window that is covered, or on another Space, does answer.
  - Keep it open, even behind Claude Desktop.
- **The Keychain's question.**
  - The first time the chat reads the saved key, macOS may ask whether TheEngineEngine may use it.
  - An app you build yourself is signed afresh by each build, and macOS may then ask again after every rebuild.
  - Choose "Always Allow".
  - The key is read away from the window: the window keeps drawing, and the chat says "reading your saved key" until
    the question is answered.
  - If you refuse, the chat says the key could not be read and does not ask again. Save the key again with **Key** to
    start over.
- **Long calls.**
  - Claude Desktop stops waiting for a tool after 60 seconds by default.
  - Every tool here ends within 55 seconds, and says why in words when it could not finish. A dyno sweep that cannot
    finish in time says so; ask for a narrower range.
- **The chat's first conversation with tools.**
  - The chat sends back what Claude wrote, exactly as it was written. The API checks that earlier messages are
    unchanged when Claude's thinking is kept between turns, and nobody here can run that check.
  - If the check ever finds a difference, you would see it on the message after Claude used a tool: a
    notice "the API answered 400: ..." about earlier messages or thinking. Please send that notice's words if you
    see it.
- **An idle link.**
  - The link's folder is in your user's temporary folder. macOS may remove files there that nobody has used for three
    days.
  - If Claude Desktop says the app is not open after the app has been running for days, turn Allow off and on in
    SETTINGS.
- **Claude Desktop's sandbox.**
  - Whether Claude Desktop runs its extensions inside a macOS sandbox is not documented anywhere we could read.
  - If it does, the extension's temporary folder would differ from the app's, and the log line above would show a
    folder under `~/Library/Containers`. Please send that line if you see it.
