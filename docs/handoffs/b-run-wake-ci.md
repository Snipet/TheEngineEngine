# B: the wake's run in CI (scripts/run-wake.sh), the block for build.yml

For the steward to copy as bytes, once the lead has read the script: it goes in build.yml's "Run the application" step,
just before its `ls -l "$out" || true`. The step is continue-on-error off Linux already, so macOS and Windows report
and do not fail the job. `limited`, `run` (Xvfb on Linux) and `rc` are the step's own.

```bash
          echo "---- the assistant's wake: the window minimised, one call through tee-mcp answered while it does not tick"
          if [ "$RUNNER_OS" = Windows ]; then
            bash scripts/run-wake.sh "$app" none || rc=$?
          elif [ -n "${{ steps.mcpb.outputs.path }}" ]; then
            wake="$RUNNER_TEMP/mcpb-wake"; rm -rf "$wake"; mkdir -p "$wake"
            unzip -q "${{ steps.mcpb.outputs.path }}" -d "$wake" \
              && limited "${run[@]}" scripts/run-wake.sh "$app" "$wake/server/tee-mcp" || rc=$?
          else
            echo "::warning::no Claude Desktop extension was built: the wake's run was not made"
          fi
```

What a passing run prints, one line (Linux, Xvfb with openbox, 27 Nov):

    run-wake: answered in 0.11 s; window: minimised=1 hidden=0 occluded=1; ticks 30 before the call, 30 after the answer; simulator compiling at the call

What the script waits on: the app's first `minimise:` report (SDL says the window is down; printed on the main thread
whether it ticks or not) and the link's socket (opened at launch, before the simulator is ready), then one second more.
It does not wait for the simulator: `get_build` reads only the document, and the call is answered while the simulator
is still compiling (measured warm, and cold with `TEE_CACHE_DIR` empty and eight busy loops on four cores: both 0.1 s).

Not run: macOS's runner (whether it honours a minimise: the app hides the window if not, and the line says so) and
Windows (no link in this version: the script says so and passes). Proof that it bites: today's tree with only
`shown.assistant->setWake(...)` in app/Main.cpp made a no-op: the call answered "timed out before the app took it:
nothing was done", exit 1 (the commit before the wake has no `--minimise-after` to run it with).

Found on the way, for the lead: while the window does not tick, the simulator is never adopted
(`SimAdapter::adopt()` runs only from `SimAdapter::frame()`, the frame's), so it stays "compiling" however long the
compile took, and the document's edits are not flushed to the host (`build_->flush()`, the same path) until the window
ticks again. The wake drains the assistant's jobs; it does not run the engine's frame.
