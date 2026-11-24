# ui/CMakeLists.txt: the interface kit (card J). Two ways in:
#
#   added by the root:      add_subdirectory(ui). FunkGui is already there (the root's pin); the application links it.
#   alone:                  cmake -S ui -B build-ui. Nothing but FunkGui's core (JUCE off, no GPU) and a made-up
#                           Player; no LLVM, no SDL. FunkGui comes from TEE_FUNKGUI_DIR (a checkout) or by git at the
#                           pin below.
#
# Targets:
#   tee_ui_kit    (static)  ui/kit/gui and ui/kit/shaders: the .gui language, its layout, controls, displays and view
#   tee_ui_ref    (static)  ui/ref: RefPanel, the reference engine's own panel, and tee::ui::makeMainPanel
#   tee-shadergen (a tool)  writes what shaderc is handed for a panel's shaders (cmake/TeeShaders.cmake runs it)
#   tests/ui                when TEE_UI_BUILD_TESTS is on (the default alone; the root's TEE_BUILD_TESTS otherwise)
#
# FunkGui::core is an INTERFACE library that carries its sources, as a JUCE module does: whoever links it compiles
# FunkGui. So our libraries take only its headers and definitions (tee_ui_funkgui_headers), and the one thing that is
# finally linked (the application, a test) links FunkGui itself, once.
cmake_minimum_required(VERSION 3.30)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(TeeUi LANGUAGES C CXX)
  set(TEE_UI_ALONE ON)
else()
  set(TEE_UI_ALONE OFF)
endif()

get_filename_component(TEE_UI_REPO "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(TEE_UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(TEE_UI_ALONE)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  option(TEE_UI_BUILD_TESTS "The interface kit's tests (tests/ui)" ON)

  # FunkGui v0.27.0. The root's pin (cmake/TeePins.cmake) is the one the application is built with; this one is only
  # for building ui/ alone and must be moved with it.
  set(TEE_UI_FUNKGUI_REPOSITORY "https://github.com/Snipet/FunkGui" CACHE STRING "FunkGui's repository (ui/ alone)")
  set(TEE_UI_FUNKGUI_COMMIT "b7bc1cd" CACHE STRING "FunkGui's commit (ui/ alone)")
  set(TEE_FUNKGUI_DIR "" CACHE PATH "A FunkGui checkout to use in place of fetching one (ui/ alone)")
  set(FUNKGUI_WITH_JUCE OFF CACHE BOOL "" FORCE)
  set(FUNKGUI_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(FUNKGUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  if(TEE_FUNKGUI_DIR)
    add_subdirectory("${TEE_FUNKGUI_DIR}" "${CMAKE_BINARY_DIR}/funkgui" EXCLUDE_FROM_ALL)
  else()
    include(FetchContent)
    FetchContent_Declare(FunkGui GIT_REPOSITORY "${TEE_UI_FUNKGUI_REPOSITORY}" GIT_TAG "${TEE_UI_FUNKGUI_COMMIT}"
                         EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(FunkGui)
  endif()
  enable_testing()
else()
  if(NOT DEFINED TEE_UI_BUILD_TESTS)
    if(DEFINED TEE_BUILD_TESTS)
      set(TEE_UI_BUILD_TESTS ${TEE_BUILD_TESTS})
    else()
      set(TEE_UI_BUILD_TESTS ON)
    endif()
  endif()
endif()

if(NOT TARGET FunkGui::core)
  message(FATAL_ERROR "ui/: FunkGui::core is not defined (the root adds FunkGui before ui/; alone, ui/ fetches it)")
endif()

# Our warnings, on our code (docs/plan/01-rules.md §4).
function(tee_ui_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror -ffp-contract=off)
  endif()
  target_compile_features(${target} PUBLIC cxx_std_20)
endfunction()

# FunkGui's headers and definitions without its sources (above).
add_library(tee_ui_funkgui_headers INTERFACE)
target_include_directories(tee_ui_funkgui_headers SYSTEM INTERFACE
  $<TARGET_PROPERTY:FunkGuiCore,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(tee_ui_funkgui_headers INTERFACE
  $<TARGET_PROPERTY:FunkGuiCore,INTERFACE_COMPILE_DEFINITIONS>)

# Card B's interfaces (Player, ParamSource, TelemetrySource): the shell's header when it is in the tree, else the
# stand-in that holds the card's text (tests/ui/standin/tee/app/Sources.h says why).
add_library(tee_ui_sources_header INTERFACE)
if(EXISTS "${TEE_UI_REPO}/shell/include/tee/app/Sources.h")
  target_include_directories(tee_ui_sources_header INTERFACE "${TEE_UI_REPO}/shell/include")
else()
  message(STATUS "ui/: shell/include/tee/app/Sources.h is not in the tree: using tests/ui/standin")
  target_include_directories(tee_ui_sources_header INTERFACE "${TEE_UI_REPO}/tests/ui/standin")
endif()

# Whether a GPU layer of FunkGui's is in this build (FunkGui::gpu under JUCE, or card A's FunkGui::desktop): with one,
# funkgui::shaders is FunkGui's and panel shaders are built (cmake/TeeShaders.cmake); with none, ShaderLibraryNone.cpp
# stands in and the rasteriser draws each shader's placeholder.
set(TEE_UI_GPU OFF)
if(TARGET FunkGui::desktop OR TARGET FunkGui::gpu)
  set(TEE_UI_GPU ON)
endif()

file(GLOB _tee_ui_kit_sources CONFIGURE_DEPENDS "${TEE_UI_DIR}/kit/gui/*.cpp" "${TEE_UI_DIR}/kit/gui/*.h"
                                                "${TEE_UI_DIR}/kit/shaders/*.h")
add_library(tee_ui_kit STATIC ${_tee_ui_kit_sources}
  "${TEE_UI_DIR}/kit/shaders/ShaderText.cpp" "${TEE_UI_DIR}/kit/shaders/ShaderLib.cpp"
  "${TEE_UI_DIR}/kit/shaders/ShaderFont.cpp" "${TEE_UI_DIR}/kit/shaders/Registry.cpp")
if(NOT TEE_UI_GPU)
  target_sources(tee_ui_kit PRIVATE "${TEE_UI_DIR}/kit/shaders/ShaderLibraryNone.cpp")
endif()
target_include_directories(tee_ui_kit PUBLIC "${TEE_UI_DIR}")
target_link_libraries(tee_ui_kit PUBLIC tee_ui_funkgui_headers tee_ui_sources_header)
target_compile_definitions(tee_ui_kit PUBLIC TEE_UI_GPU=$<BOOL:${TEE_UI_GPU}>)
tee_ui_warnings(tee_ui_kit)

# The build's tool: FunkGui-free (the .gui reader and the shader text alone).
add_executable(tee-shadergen "${TEE_UI_DIR}/kit/shaders/shadergen.cpp"
  "${TEE_UI_DIR}/kit/shaders/ShaderText.cpp" "${TEE_UI_DIR}/kit/shaders/ShaderLib.cpp"
  "${TEE_UI_DIR}/kit/gui/GuiSpec.cpp" "${TEE_UI_DIR}/kit/gui/PanelSpec.cpp")
target_include_directories(tee-shadergen PRIVATE "${TEE_UI_DIR}")
tee_ui_warnings(tee-shadergen)

include("${TEE_UI_REPO}/cmake/TeeShaders.cmake")

add_subdirectory(ref)

if(TEE_UI_BUILD_TESTS)
  add_subdirectory("${TEE_UI_REPO}/tests/ui" "${CMAKE_CURRENT_BINARY_DIR}/tests")
endif()
