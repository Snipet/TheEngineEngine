# TheEngineEngine: a car engine simulated as physics and heard as the physics' own sound. The simulator is an FMinus
# program (sim/), compiled when the application starts by the FMinus JIT it carries; host/ is the C++ that compiles
# and plays it, tools/ the command-line programs on top (tee-render, tee-bench), tests/ their proof.
#
# Every dependency is pinned in cmake/TeePins.cmake and fetched by git at its commit (cmake/TeeDeps.cmake); FMinus's
# LLVM comes prebuilt (scripts/fetch-llvm.sh) and is never built here.
#
# Use the presets: `cmake --workflow --preset dev-verify` configures, builds and runs every test (README.md).
#
# Options:
#   TEE_BUILD_TOOLS    tools/: tee-render, tee-bench                                     (ON)
#   TEE_BUILD_TESTS    tests/, run by ctest                                              (ON)
#   TEE_BUILD_APP      the application (shell/, ui/): OFF until FunkGui's desktop host is pinned
#   TEE_WERROR         warnings in our own sources are errors (cmake/TeeFlags.cmake)     (ON)
#   TEE_DEPS_DIR       the dependency cache (cmake/TeeDeps.cmake)
#   TEE_LLVM_DIR       an LLVM install for FMinus, when none is in the cache
#   TEE_COMPILER_CACHE compile through ccache or sccache when installed                  (ON)
cmake_minimum_required(VERSION 3.30)

set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS")   # FMinus's and FunkGui's own

# The compiler, unless one was chosen (CC, CXX, -DCMAKE_<LANG>_COMPILER): clang on Linux, as FMinus's prebuilt LLVM
# was built with, and clang-cl on Windows, where FMinus refuses MSVC's cl (run from an x64 developer prompt, which
# puts Visual Studio's clang-cl on the PATH). macOS takes Xcode's.
if(NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CC} AND NOT DEFINED ENV{CXX})
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
  elseif(CMAKE_HOST_WIN32 AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_C_COMPILER clang-cl)
    set(CMAKE_CXX_COMPILER clang-cl)
  endif()
endif()

project(TheEngineEngine
  VERSION 0.1.0
  DESCRIPTION "A car engine simulator whose sound is the simulation's own"
  LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)   # no C++20 modules anywhere: without this Ninja scans every source twice
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND NOT DEFINED CMAKE_LINKER_TYPE)
  find_program(TEE_LLD ld.lld)
  if(TEE_LLD)
    set(CMAKE_LINKER_TYPE LLD)        # LLVM's static libraries link several times faster
  endif()
endif()
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC" AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  # The release C runtime (/MD) in every configuration: LLVM's libraries are built against it (FMinus sets the same).
  set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
endif()

option(TEE_BUILD_TOOLS "Build the tools under tools/ (tee-render, tee-bench)" ON)
option(TEE_BUILD_TESTS "Build the tests under tests/, which ctest runs" ON)
option(TEE_BUILD_APP "Build the application (shell/, ui/): OFF until FunkGui's desktop host is pinned" OFF)

set(TEE_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(TeePins)
include(TeeFlags)       # tee_target_defaults()
include(TeeDeps)        # the cache, ccache, FMinus::FMinus, FunkGui::core, nlohmann_json, Catch2

# Our own outputs only (set after the dependencies, which place their own).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

if(TEE_BUILD_TESTS)
  enable_testing()
endif()
add_subdirectory(host)
if(TEE_BUILD_TOOLS)
  add_subdirectory(tools)
endif()
if(TEE_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(TEE_BUILD_APP)
  if(EXISTS "${PROJECT_SOURCE_DIR}/shell/CMakeLists.txt")
    add_subdirectory(shell)
  else()
    message(WARNING "TheEngineEngine: TEE_BUILD_APP is ON, and there is no shell/ yet: the application arrives with "
                    "FunkGui's desktop host (docs/plan/w1-a-funkgui-desktop.md)")
  endif()
endif()

message(STATUS "TheEngineEngine ${PROJECT_VERSION}: ${CMAKE_BUILD_TYPE}, tools ${TEE_BUILD_TOOLS}, tests "
               "${TEE_BUILD_TESTS}, app ${TEE_BUILD_APP}, deps cache ${TEE_DEPS_DIR}")
