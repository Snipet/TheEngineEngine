# shell/: everything between the operating system and the simulator's host and the interface's Panels
# (docs/plan/w1-b-shell.md). Five static libraries, by what each needs:
#
#   tee_shell_core   nothing but the standard library: the interfaces (include/tee/app/), the controls (input/Keys,
#                    Driver, Controls), the WAV and PNG writers, the recorder, the stand-in tone and the silent player
#   tee_shell_settings  nlohmann/json: the settings file (settings/Settings)
#   tee_shell_files  tee_host and nlohmann/json: builds on disk, recent files, the library (files/BuildFiles)
#   tee_shell_ref    tee_host: the reference engine and the simulator as Players (player/RefAdapter, SimAdapter)
#   tee_shell_sdl    SDL3: the audio device (audio/AudioDevice) and SDL's events into the controls (input/SdlInput)
#   tee_shell_wheel  the racing wheel's transport (input/g29/RacingWheel.h): macOS's IOKit HID manager
#                    (RacingWheelMac.cpp, IOKit and CoreFoundation); Linux's evdev and sysfs over the kernel's hid-lg4ff
#                    (RacingWheelLinux.cpp, G29Evdev.cpp); Windows: SDL's joystick API, read only (RacingWheelWin.cpp,
#                    SDL3); elsewhere none (RacingWheelNone.cpp). The G29's map and decisions (g29/G29, G29Driver,
#                    G29Input, G29Joystick) are the core's: plain C++, tested with fakes.
#
# FunkGui is in none of them: FunkGui::core is an INTERFACE library whose sources compile into the target that links
# it, so what needs it (panel/PlaceholderPanel.cpp, app/Offline.cpp) is compiled by each executable (app/CMakeLists.txt).
add_library(tee_shell_core STATIC
  input/Keys.cpp
  input/Driver.cpp
  input/Controls.cpp
  input/WheelMap.cpp
  input/g29/G29.cpp
  input/g29/G29Driver.cpp
  input/g29/G29Input.cpp input/g29/G29Joystick.cpp
  audio/Wav.cpp
  audio/Recorder.cpp
  files/Png.cpp
  player/TonePlayer.cpp)
add_library(Tee::shell_core ALIAS tee_shell_core)
tee_target_defaults(tee_shell_core)
target_include_directories(tee_shell_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include")
find_package(Threads REQUIRED)                      # the recorder's worker
target_link_libraries(tee_shell_core PUBLIC Threads::Threads)

# The settings file reads and writes JSON (nlohmann/json, header-only): a library of its own, so that the core needs
# nothing but the standard library.
add_library(tee_shell_settings STATIC settings/Settings.cpp)
add_library(Tee::shell_settings ALIAS tee_shell_settings)
tee_target_defaults(tee_shell_settings)
target_link_libraries(tee_shell_settings PUBLIC Tee::shell_core PRIVATE nlohmann_json::nlohmann_json)

add_library(tee_shell_ref STATIC player/RefAdapter.cpp player/SimAdapter.cpp)
add_library(Tee::shell_ref ALIAS tee_shell_ref)
tee_target_defaults(tee_shell_ref)
target_link_libraries(tee_shell_ref PUBLIC Tee::shell_core PRIVATE Tee::host)

# Builds on disk (files/BuildFiles): card C's tee::Build, and the head of a build's file read as JSON for a summary.
add_library(tee_shell_files STATIC files/BuildFiles.cpp)
add_library(Tee::shell_files ALIAS tee_shell_files)
tee_target_defaults(tee_shell_files)
target_link_libraries(tee_shell_files PUBLIC Tee::shell_core Tee::host PRIVATE nlohmann_json::nlohmann_json)

add_library(tee_shell_sdl STATIC audio/AudioDevice.cpp input/SdlInput.cpp)
add_library(Tee::shell_sdl ALIAS tee_shell_sdl)
tee_target_defaults(tee_shell_sdl)
target_link_libraries(tee_shell_sdl PUBLIC Tee::shell_core SDL3::SDL3)

if(APPLE)
  add_library(tee_shell_wheel STATIC input/g29/RacingWheelMac.cpp)
  target_link_libraries(tee_shell_wheel PUBLIC Tee::shell_core "-framework IOKit" "-framework CoreFoundation")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # the kernel's hid-lg4ff, read and commanded through evdev and sysfs (g29/G29Evdev.h)
  add_library(tee_shell_wheel STATIC input/g29/RacingWheelLinux.cpp input/g29/G29Evdev.cpp
              input/g29/G29EvdevOpen.cpp)
  target_link_libraries(tee_shell_wheel PUBLIC Tee::shell_core)
elseif(WIN32)
  # a plain joystick through SDL, read only (g29/G29Joystick.h: on Windows nothing can be sent to the wheel)
  add_library(tee_shell_wheel STATIC input/g29/RacingWheelWin.cpp)
  target_link_libraries(tee_shell_wheel PUBLIC Tee::shell_core PRIVATE SDL3::SDL3)
else()
  add_library(tee_shell_wheel STATIC input/g29/RacingWheelNone.cpp)
  target_link_libraries(tee_shell_wheel PUBLIC Tee::shell_core)
endif()
add_library(Tee::shell_wheel ALIAS tee_shell_wheel)
tee_target_defaults(tee_shell_wheel)

# The shell's own plain Panel, for app/CMakeLists.txt: every executable that shows a Panel compiles the first; the
# second, which makes it tee::ui::makeMainPanel, only where no interface package (a target tee_ui) defines that.
set(TEE_SHELL_PANEL_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/panel/PlaceholderPanel.cpp" CACHE INTERNAL "")
set(TEE_SHELL_PANEL_MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/panel/PlaceholderMain.cpp" CACHE INTERNAL "")
