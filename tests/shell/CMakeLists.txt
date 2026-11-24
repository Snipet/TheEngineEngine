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
add_test(NAME shell.headless.tone
  COMMAND tee-headless --engine tone --script "${_scripts}/tone.txt" --wav "${_out}/tone.wav" --shot "${_out}/tone.png"
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
