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

What a passing run prints (Linux, Xvfb with openbox, 27 Nov): the first line is required, the second reported:

    run-wake: answered in 0.11 s; window: minimised=1 hidden=0 occluded=1; ticks 30 before the call, 30 after the answer; simulator compiling at the call
    run-wake: simulator ready while minimised, 6.5 s after the minimise; ticks 30 (reported)

(cold, `TEE_CACHE_DIR` empty, eight busy loops on four cores: 0.13 s, and ready 12.1 s after the minimise, ticks 30;
the second line waits 15 s at most and says so if the simulator is not ready by then, without failing.)

What the script waits on: the app's first `minimise:` report (SDL says the window is down; printed on the main thread
whether it ticks or not) and the link's socket (opened at launch, before the simulator is ready), then one second more.
It does not wait for the simulator: `get_build` reads only the document, and the call is answered while the simulator
is still compiling (measured warm, and cold with `TEE_CACHE_DIR` empty and eight busy loops on four cores: both 0.1 s).

Not run: macOS's runner (whether it honours a minimise: the app hides the window if not, and the line says so) and
Windows (no link in this version: the script says so and passes). Proof that it bites: today's tree with only
`shown.assistant->setWake(...)` in app/Main.cpp made a no-op: the call answered "timed out before the app took it:
nothing was done", exit 1 (the commit before the wake has no `--minimise-after` to run it with).

X-066, found on the way and cured in 48a26516: while the window did not tick the simulator was never adopted and a
tool's change never reached it (`SimAdapter::adopt()` and `build_->flush()` run only from `SimAdapter::frame()`, the
frame's). The wake's drain now runs the engine's frame before and after the jobs (`Shown::drainAssistant`); app/Main.cpp
says, at the wake's branch, what of the frame still does not run while minimised (input, the bake's tick, the frames
clock, the status line, the script's checks) and why.
