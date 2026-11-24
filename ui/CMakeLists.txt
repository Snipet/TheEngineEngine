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
#   tee_ui        (interface) both: what the application links for makeMainPanel
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

# Our warnings, on our code (docs/plan/01-rules.md section 4): the root's own rule (cmake/TeeFlags.cmake) when ui/ is
# built by the root, the same flags when it is built alone.
function(tee_ui_warnings target)
  if(COMMAND tee_target_defaults)
    tee_target_defaults(${target})
    return()
  endif()
  target_compile_features(${target} PUBLIC cxx_std_20)
  if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(${target} PRIVATE /W4 /WX $<$<CXX_COMPILER_ID:Clang>:/clang:-ffp-contract=off>)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror -ffp-contract=off)
  endif()
endfunction()

# FunkGui's headers and definitions without its sources (above).
add_library(tee_ui_funkgui_headers INTERFACE)
target_include_directories(tee_ui_funkgui_headers SYSTEM INTERFACE
  $<TARGET_PROPERTY:FunkGuiCore,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(tee_ui_funkgui_headers INTERFACE
  $<TARGET_PROPERTY:FunkGuiCore,INTERFACE_COMPILE_DEFINITIONS>)

# Card B's interfaces (Player, ParamSource, TelemetrySource, AppServices; makeMainPanel's declaration): the shell's
# headers, which need nothing of the shell's but themselves.
add_library(tee_ui_sources_header INTERFACE)
target_include_directories(tee_ui_sources_header INTERFACE "${TEE_UI_REPO}/shell/include")

# Whether a GPU layer of FunkGui's is in this build (FunkGui::gpu under JUCE, or card A's FunkGui::desktop): with one,
# funkgui::shaders is FunkGui's and panel shaders are built (cmake/TeeShaders.cmake); with none, ShaderLibraryNone.cpp
# stands in and the rasteriser draws each shader's placeholder.
set(TEE_UI_GPU OFF)
if(TARGET FunkGui::desktop OR TARGET FunkGui::gpu)
  set(TEE_UI_GPU ON)
endif()

file(GLOB _tee_ui_kit_sources CONFIGURE_DEPENDS "${TEE_UI_DIR}/kit/gui/*.cpp" "${TEE_UI_DIR}/kit/gui/*.h"
                                                "${TEE_UI_DIR}/kit/shaders/*.h"
                                                # J2: the widgets and the binding to a Spec's fields
                                                "${TEE_UI_DIR}/kit/widgets/*.cpp" "${TEE_UI_DIR}/kit/widgets/*.h"
                                                "${TEE_UI_DIR}/kit/bind/*.cpp" "${TEE_UI_DIR}/kit/bind/*.h")
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

# J2: the application's frame of screens (ui/app/), over a FieldDoc (ui/kit/bind/Fields.h).
file(GLOB _tee_ui_app_sources CONFIGURE_DEPENDS "${TEE_UI_DIR}/app/*.cpp" "${TEE_UI_DIR}/app/*.h")
# U1: the workspaces (docs/plan/ui-design.md section 5). Every directory ui/views/<id>/ whose name does not start with
# '_' is compiled into tee_ui_app, and its register.h (one line: TEE_UI_WORKSPACE(<id>, <factory>)) is listed in a
# generated source, so a package adds a workspace without editing a shared file (ui/app/Workspace.h).
file(GLOB _tee_ui_view_sources CONFIGURE_DEPENDS "${TEE_UI_DIR}/views/*/*.cpp" "${TEE_UI_DIR}/views/*/*.h")
file(GLOB _tee_ui_view_registers CONFIGURE_DEPENDS "${TEE_UI_DIR}/views/*/register.h")
list(FILTER _tee_ui_view_sources EXCLUDE REGEX "/views/_[^/]*/")
list(FILTER _tee_ui_view_registers EXCLUDE REGEX "/views/_[^/]*/")
list(SORT _tee_ui_view_registers)
set(_tee_ui_view_includes "")
foreach(_r IN LISTS _tee_ui_view_registers)
  file(RELATIVE_PATH _rel "${TEE_UI_DIR}" "${_r}")
  string(APPEND _tee_ui_view_includes "#include \"${_rel}\"\n")
endforeach()
set(_tee_ui_registry "${CMAKE_CURRENT_BINARY_DIR}/generated/WorkspaceRegistry.cpp")
file(GENERATE OUTPUT "${_tee_ui_registry}" CONTENT
"// Generated by ui/CMakeLists.txt from ui/views/*/register.h: the workspaces this build has.
#include \"app/Workspace.h\"
namespace tee::ui
{
#define TEE_UI_WORKSPACE(id, factory) std::unique_ptr<Workspace> factory(WorkspaceContext&);
${_tee_ui_view_includes}#undef TEE_UI_WORKSPACE
    const std::vector<WorkspaceEntry>& registeredWorkspaces()
    {
        static const std::vector<WorkspaceEntry> all = {
#define TEE_UI_WORKSPACE(id, factory) { #id, &factory },
${_tee_ui_view_includes}#undef TEE_UI_WORKSPACE
        };
        return all;
    }
}
")
add_library(tee_ui_app STATIC ${_tee_ui_app_sources} ${_tee_ui_view_sources} "${_tee_ui_registry}")
target_link_libraries(tee_ui_app PUBLIC tee_ui_kit)
tee_ui_warnings(tee_ui_app)

# J2: the binding to card C's tee::Build (ui/kit/bind/host/), where the host library is (the root's build).
if(TARGET tee_host)
  add_library(tee_ui_build STATIC "${TEE_UI_DIR}/kit/bind/host/BuildDoc.cpp" "${TEE_UI_DIR}/kit/bind/host/BuildDoc.h")
  target_link_libraries(tee_ui_build PUBLIC tee_ui_app tee_host)
  tee_ui_warnings(tee_ui_build)
endif()

# What the application links for its Panel (shell/include/tee/app/MainPanel.h: while no target tee_ui exists the shell
# uses its placeholder).
add_library(tee_ui INTERFACE)
target_link_libraries(tee_ui INTERFACE tee_ui_ref tee_ui_kit)

if(TEE_UI_BUILD_TESTS)
  add_subdirectory("${TEE_UI_REPO}/tests/ui" "${CMAKE_CURRENT_BINARY_DIR}/tests")
endif()
