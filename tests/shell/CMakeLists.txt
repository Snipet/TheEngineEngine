# tests/shell/: the shell's and the application's tests (docs/plan/w1-b-shell.md), run by ctest.
#
#   tee-shell-tests   Catch2: the controls, the files, the script, the reference as a Player, the audio device on SDL's
#                     dummy driver. Every .cpp here is a source (adding a file edits no CMake).
#   shell.headless.*  tee-headless itself, run on a script: the reference through a start, a rev and a stop to a WAV
#                     and a PNG under out/shell/; the stand-in tone; a program that does not compile, shown in the Panel.
include(Catch)
file(GLOB _tee_shell_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-shell-tests ${_tee_shell_tests})
tee_target_defaults(tee-shell-tests)
target_link_libraries(tee-shell-tests PRIVATE tee_app Tee::shell_core Tee::shell_ref Tee::shell_sdl Catch2::Catch2WithMain)
target_compile_definitions(tee-shell-tests PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim\"")
catch_discover_tests(tee-shell-tests DISCOVERY_MODE PRE_TEST
  PROPERTIES TIMEOUT 600 ENVIRONMENT "SDL_AUDIODRIVER=dummy")

set(_out "${PROJECT_SOURCE_DIR}/out/shell")
set(_scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

add_test(NAME shell.headless.ref
  COMMAND tee-headless --sim "${PROJECT_SOURCE_DIR}/sim" --script "${_scripts}/start-rev-stop.txt"
          --wav "${_out}/ref-start-rev-stop.wav" --shot "${_out}/ref-end.png"
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
add_test(NAME shell.headless.drive
  COMMAND tee-headless --sim "${PROJECT_SOURCE_DIR}/sim" --script "${_scripts}/drive.txt" --wav "${_out}/ref-drive.wav"
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
set_tests_properties(shell.headless.ref shell.headless.drive shell.headless.tone shell.headless.error shell.headless.refuses
  PROPERTIES TIMEOUT 900 LABELS shell)

# The window itself, where there is one to test and a headless desktop to open it on (Linux, scripts/xvfb-run.sh: Xvfb,
# openbox and Mesa's software Vulkan): five seconds and at least 60 frames of the application with the reference started and revved by a
# script, on SDL's dummy audio driver, and the last frame read back from the GPU path into out/shell/window-gpu.png.
# The picture is the software Vulkan driver's, not a GPU's.
if(TARGET TheEngineEngine AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_program(TEE_XVFB Xvfb)
  if(TEE_XVFB)
    add_test(NAME shell.window.gpu
      COMMAND "${PROJECT_SOURCE_DIR}/scripts/xvfb-run.sh" $<TARGET_FILE:TheEngineEngine>
              --script "${_scripts}/window.txt" --frames 60 --shot "${_out}/window-gpu.png"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    add_test(NAME shell.window.plain
      COMMAND "${PROJECT_SOURCE_DIR}/scripts/xvfb-run.sh" $<TARGET_FILE:TheEngineEngine> --panel plain
              --script "${_scripts}/window.txt" --frames 60 --shot "${_out}/window-plain-gpu.png"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    add_test(NAME shell.window.info
      COMMAND "${PROJECT_SOURCE_DIR}/scripts/xvfb-run.sh" $<TARGET_FILE:TheEngineEngine> --info
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    set_tests_properties(shell.window.gpu shell.window.plain shell.window.info PROPERTIES
      TIMEOUT 600 LABELS "shell;window" ENVIRONMENT "SDL_AUDIODRIVER=dummy" RUN_SERIAL TRUE)
    # (a test with a pattern passes by the pattern alone: each of these lines is printed only when all went well)
    set_tests_properties(shell.window.gpu shell.window.plain PROPERTIES PASS_REGULAR_EXPRESSION "tee-app: ok")
    set_tests_properties(shell.window.info PROPERTIES PASS_REGULAR_EXPRESSION "renderer: Vulkan")
  endif()
endif()
