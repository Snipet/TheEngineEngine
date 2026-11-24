# shell/: everything between the operating system and the simulator's host and the interface's Panels
# (docs/plan/w1-b-shell.md). Three static libraries, by what each needs:
#
#   tee_shell_core   nothing but the standard library: the interfaces (include/tee/app/), the controls (input/Keys,
#                    Driver, Controls), the WAV and PNG writers, the stand-in tone and the silent player
#   tee_shell_ref    tee_host: the reference engine as a Player (player/RefAdapter)
#   tee_shell_sdl    SDL3: the audio device (audio/AudioDevice) and SDL's events into the controls (input/SdlInput)
#
# FunkGui is in none of them: FunkGui::core is an INTERFACE library whose sources compile into the target that links
# it, so what needs it (panel/PlaceholderPanel.cpp, app/Offline.cpp) is compiled by each executable (app/CMakeLists.txt).
add_library(tee_shell_core STATIC
  input/Keys.cpp
  input/Driver.cpp
  input/Controls.cpp
  audio/Wav.cpp
  files/Png.cpp
  player/TonePlayer.cpp)
add_library(Tee::shell_core ALIAS tee_shell_core)
tee_target_defaults(tee_shell_core)
target_include_directories(tee_shell_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(tee_shell_ref STATIC player/RefAdapter.cpp)
add_library(Tee::shell_ref ALIAS tee_shell_ref)
tee_target_defaults(tee_shell_ref)
target_link_libraries(tee_shell_ref PUBLIC Tee::shell_core PRIVATE Tee::host)

add_library(tee_shell_sdl STATIC audio/AudioDevice.cpp input/SdlInput.cpp)
add_library(Tee::shell_sdl ALIAS tee_shell_sdl)
tee_target_defaults(tee_shell_sdl)
target_link_libraries(tee_shell_sdl PUBLIC Tee::shell_core SDL3::SDL3)

# The shell's own plain Panel, for app/CMakeLists.txt: every executable that shows a Panel compiles the first; the
# second, which makes it tee::ui::makeMainPanel, only where no interface package (a target tee_ui) defines that.
set(TEE_SHELL_PANEL_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/panel/PlaceholderPanel.cpp" CACHE INTERNAL "")
set(TEE_SHELL_PANEL_MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/panel/PlaceholderMain.cpp" CACHE INTERNAL "")
