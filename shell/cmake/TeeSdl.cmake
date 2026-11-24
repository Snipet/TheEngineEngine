# shell/cmake/TeeSdl.cmake: SDL3 in our tree, at its pin (cmake/TeePins.cmake: the release FunkGui pins too), fetched
# by git into the dependency cache, built as a static library with the subsystems the shell and FunkGui's desktop host
# use (video, events, audio, joystick, gamepad, haptic, dialogs) and no others. Included by cmake/TeeDeps.cmake before
# FunkGui is added, so that FunkGui::desktop uses this SDL3::SDL3 and fetches none of its own.
#
# On Linux only SDL's X11 video driver is built (FunkGui's Vulkan surface is an X11 window's; a Wayland desktop runs it
# through XWayland), and SDL's audio drivers are those whose development packages the machine has: ALSA, PulseAudio
# and PipeWire where libasound2-dev, libpulse-dev and libpipewire-0.3-dev are installed (scripts/bootstrap-cloud.sh
# and CI install them), and always its dummy driver, which is the one the tests run on.
include_guard(GLOBAL)

if(TARGET SDL3::SDL3)
  message(STATUS "TheEngineEngine: SDL3::SDL3 is already defined: using it as it is")
  return()
endif()

tee_source(sdl3 _tee_sdl_dir)

# SDL's options, as normal variables (its option() calls honour them: CMP0077).
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_INSTALL OFF)
set(SDL_UNINSTALL OFF)
foreach(_tee_sdl_off GPU RENDER CAMERA SENSOR POWER TRAY)
  set(SDL_${_tee_sdl_off} OFF)
endforeach()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(SDL_X11 ON)
  foreach(_tee_sdl_off WAYLAND KMSDRM OPENGL OPENGLES VULKAN)
    set(SDL_${_tee_sdl_off} OFF)
  endforeach()
endif()
add_subdirectory("${_tee_sdl_dir}" "${PROJECT_BINARY_DIR}/_deps/sdl3" EXCLUDE_FROM_ALL SYSTEM)
if(NOT TARGET SDL3::SDL3)
  message(FATAL_ERROR "TheEngineEngine: SDL ${TEE_SDL3_VERSION} at ${_tee_sdl_dir} defined no SDL3::SDL3")
endif()
# Third-party code: its warnings are not ours.
if(TARGET SDL3-static)
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(SDL3-static PRIVATE /w)
  else()
    target_compile_options(SDL3-static PRIVATE -w)
  endif()
endif()
set(TEE_SDL3_SOURCE_DIR "${_tee_sdl_dir}")
