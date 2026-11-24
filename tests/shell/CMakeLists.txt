# tests/shell/: the shell's and the application's tests (docs/plan/w1-b-shell.md), run by ctest.
#
#   tee-shell-tests   Catch2: the controls, the files, the script, the reference as a Player, the audio device on SDL's
#                     dummy driver. Every .cpp here is a source (adding a file edits no CMake).
#   shell.headless.*  tee-headless itself, run on a script: the reference through a start, a rev and a stop to a WAV
#                     and a PNG under out/shell/; the simulator (sim/engine.fm) the same way, in the application's
#                     frame; the stand-in tone; a program that does not compile, shown in the Panel.
include(Catch)
file(GLOB _tee_shell_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-shell-tests ${_tee_shell_tests})
tee_target_defaults(tee-shell-tests)
target_link_libraries(tee-shell-tests PRIVATE tee_app Tee::shell_core Tee::shell_settings Tee::shell_files Tee::shell_ref Tee::shell_sdl
                                              Catch2::Catch2WithMain)
target_compile_definitions(tee-shell-tests PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim\"")
catch_discover_tests(tee-shell-tests DISCOVERY_MODE PRE_TEST
  PROPERTIES TIMEOUT 600 ENVIRONMENT "SDL_AUDIODRIVER=dummy")

set(_out "${PROJECT_SOURCE_DIR}/out/shell")
set(_scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

add_test(NAME shell.headless.ref
  COMMAND tee-headless --engine ref --sim "${PROJECT_SOURCE_DIR}/sim" --script "${_scripts}/start-rev-stop.txt"
          --wav "${_out}/ref-start-rev-stop.wav" --shot "${_out}/ref-end.png"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
add_test(NAME shell.headless.sim
  COMMAND tee-headless --engine sim --sim "${PROJECT_SOURCE_DIR}/sim" --script "${_scripts}/sim-start-rev-stop.txt"
          --wav "${_out}/sim-start-rev-stop.wav" --shot "${_out}/sim-end.png"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
# The window's wait for the engine's compile, in tee-headless: the compile held back a second (--hold-compile), the frame
# drawn meanwhile with START doing nothing and the status line counting, then the engine started (scripts/hold-compile.txt)
add_test(NAME shell.headless.hold
  COMMAND tee-headless --engine sim --sim "${PROJECT_SOURCE_DIR}/sim" --hold-compile 1.0
          --script "${_scripts}/hold-compile.txt"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(shell.headless.hold PROPERTIES TIMEOUT 900 LABELS shell)
# An engine the simulator finds diverged (FLAG_FAULT: put at rest and restarted) fails an offline run (exit 1), unless
# the run says it expected it (--expect-fault): made to diverge by holding the crank's speed absurd before every call.
add_test(NAME shell.headless.fault
  COMMAND tee-headless --engine sim --sim "${PROJECT_SOURCE_DIR}/sim" --hold-value crank.omega=1e7 --seconds 0.5
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
add_test(NAME shell.headless.fault.expected
  COMMAND tee-headless --engine sim --sim "${PROJECT_SOURCE_DIR}/sim" --hold-value crank.omega=1e7 --seconds 0.5
          --expect-fault
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(shell.headless.fault PROPERTIES WILL_FAIL TRUE)
set_tests_properties(shell.headless.fault.expected PROPERTIES PASS_REGULAR_EXPRESSION "tee-offline: ok .* faults=[1-9]")
set_tests_properties(shell.headless.fault shell.headless.fault.expected PROPERTIES TIMEOUT 900 LABELS shell)
# QUALITY economy (card C's SimHost::setQuality: the engine at 48 kHz inside): the same start, rev and stop pass, and
# the summary says the inner rate it ran at
add_test(NAME shell.headless.economy
  COMMAND tee-headless --engine sim --sim "${PROJECT_SOURCE_DIR}/sim" --quality economy
          --script "${_scripts}/sim-start-rev-stop.txt"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(shell.headless.economy PROPERTIES PASS_REGULAR_EXPRESSION "tee-offline: ok .* inner=48000" TIMEOUT 900
                     LABELS shell)
add_test(NAME shell.headless.drive
  COMMAND tee-headless --engine ref --sim "${PROJECT_SOURCE_DIR}/sim" --script "${_scripts}/drive.txt"
          --wav "${_out}/ref-drive.wav"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
# (--panel plain: this script's pointer presses are at the plain Panel's start button and throttle strip, wherever
# the interface package's own Panel has its controls)
add_test(NAME shell.headless.tone
  COMMAND tee-headless --engine tone --panel plain --script "${_scripts}/tone.txt" --wav "${_out}/tone.wav"
          --shot "${_out}/tone.png"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
add_test(NAME shell.headless.error
  COMMAND tee-headless --sim "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/broken" --keep-going --frames 6
          --shot "${_out}/compile-error.png"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
add_test(NAME shell.headless.refuses
  COMMAND tee-headless --sim "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/broken" --frames 6
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(shell.headless.refuses PROPERTIES WILL_FAIL TRUE)
set_tests_properties(shell.headless.ref shell.headless.sim shell.headless.drive shell.headless.tone shell.headless.error
                     shell.headless.refuses
  PROPERTIES TIMEOUT 900 LABELS shell)

# The garage's Library as the shell implements it (app/GarageLibrary.h): an executable of its own, where the frame is.
if(TARGET tee_ui_build AND TARGET tee_ui_test_funkgui AND NOT TEE_PLACEHOLDER_PANEL)
  add_executable(tee-shell-frame-tests "${CMAKE_CURRENT_SOURCE_DIR}/frame/GarageLibraryTest.cpp"
                                       "${PROJECT_SOURCE_DIR}/app/GarageLibrary.cpp")
  tee_target_defaults(tee-shell-frame-tests)
  target_include_directories(tee-shell-frame-tests PRIVATE "${PROJECT_SOURCE_DIR}/app")
  target_compile_definitions(tee-shell-frame-tests PRIVATE TEE_HAS_UI_FRAME=1
    "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"" "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim\"")
  # (FunkGui's sources compiled in, as the interface package's own tests have them)
  target_link_libraries(tee-shell-frame-tests PRIVATE tee_ui_build tee_ui_test_funkgui Tee::shell_files Tee::shell_ref Tee::host
                                                      Catch2::Catch2WithMain)
  catch_discover_tests(tee-shell-frame-tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 600)
endif()

# Each workspace of the frame, opened by the application over the simulator (tee-headless --workspace): it opens,
# runs 120 frames (2 s) with the engine started, draws, writes its picture (out/shell/workspace-<id>.png) and exits 0. A
# workspace the frame does not have is an error (exit 2), so a renamed one fails here. shell.workspace.none: that.
if(TARGET tee_ui_build AND NOT TEE_PLACEHOLDER_PANEL)
  set(_tee_workspaces garage engine valves air exhaust tune car dyno drive scope)
  set(_tee_workspace_tests "")
  foreach(_ws IN LISTS _tee_workspaces)
    add_test(NAME shell.workspace.${_ws}
      COMMAND tee-headless --engine sim --sim "${PROJECT_SOURCE_DIR}/sim" --workspace ${_ws} --frames 120
              --script "${_scripts}/workspace.txt" --shot "${_out}/workspace-${_ws}.png"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    list(APPEND _tee_workspace_tests shell.workspace.${_ws})
  endforeach()
  add_test(NAME shell.workspace.none
    COMMAND tee-headless --engine sim --sim "${PROJECT_SOURCE_DIR}/sim" --workspace nowhere --frames 2
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  set_tests_properties(shell.workspace.none PROPERTIES WILL_FAIL TRUE)
  list(APPEND _tee_workspace_tests shell.workspace.none)
  set_tests_properties(${_tee_workspace_tests} PROPERTIES TIMEOUT 600 LABELS "shell;workspace")
endif()

# The window itself, where there is one to test and a headless desktop to open it on (Linux, scripts/xvfb-run.sh: Xvfb,
# openbox and Mesa's software Vulkan): five seconds and at least 60 frames of the application with the engine started
# and revved by a script, on SDL's dummy audio driver, and the last frame read back from the GPU path: the simulator
# in the application's frame (out/shell/window-gpu.png), the reference in its own panel with its three shaders
# (window-ref-panel-gpu.png), and the plain Panel. The pictures are the software Vulkan driver's, not a GPU's.
if(TARGET TheEngineEngine AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_program(TEE_XVFB Xvfb)
  if(TEE_XVFB)
    add_test(NAME shell.window.gpu
      COMMAND "${PROJECT_SOURCE_DIR}/scripts/xvfb-run.sh" $<TARGET_FILE:TheEngineEngine>
              --script "${_scripts}/window.txt" --frames 60 --shot "${_out}/window-gpu.png"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    add_test(NAME shell.window.ref
      COMMAND "${PROJECT_SOURCE_DIR}/scripts/xvfb-run.sh" $<TARGET_FILE:TheEngineEngine> --ref-panel
              --script "${_scripts}/window.txt" --frames 60 --shot "${_out}/window-ref-panel-gpu.png"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    add_test(NAME shell.window.plain
      COMMAND "${PROJECT_SOURCE_DIR}/scripts/xvfb-run.sh" $<TARGET_FILE:TheEngineEngine> --panel plain
              --script "${_scripts}/window.txt" --frames 60 --shot "${_out}/window-plain-gpu.png"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    add_test(NAME shell.window.info
      COMMAND "${PROJECT_SOURCE_DIR}/scripts/xvfb-run.sh" $<TARGET_FILE:TheEngineEngine> --info
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    set_tests_properties(shell.window.gpu shell.window.ref shell.window.plain shell.window.info PROPERTIES
      TIMEOUT 600 LABELS "shell;window" ENVIRONMENT "SDL_AUDIODRIVER=dummy" RUN_SERIAL TRUE)
    # (a test with a pattern passes by the pattern alone: each of these lines is printed only when all went well)
    set_tests_properties(shell.window.gpu shell.window.ref shell.window.plain PROPERTIES
      PASS_REGULAR_EXPRESSION "tee-app: ok")
    set_tests_properties(shell.window.info PROPERTIES PASS_REGULAR_EXPRESSION "renderer: Vulkan")
  endif()
endif()
