# app/: the executables (docs/plan/w1-b-shell.md).
#
#   tee-headless      the application with no window and no audio device, in every build: what runs in a cloud
#                     container and in ctest (app/Offline.h)
#   TheEngineEngine   the application in a window, with TEE_BUILD_APP (it needs FunkGui's desktop host)
#
# tee_app is what the two share and that needs no FunkGui: the command line, the script, the engine's loader. What
# needs FunkGui (Offline.cpp, the Panel) is compiled by each executable, with FunkGui's own sources: FunkGui's
# targets are INTERFACE libraries whose sources compile into the target that links them.
add_library(tee_app STATIC Options.cpp Script.cpp Engine.cpp)
tee_target_defaults(tee_app)
target_include_directories(tee_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee_app PUBLIC Tee::shell_core Tee::shell_ref Tee::shell_files PRIVATE Tee::host)

option(TEE_PLACEHOLDER_PANEL "The shell's own plain Panel as the application's, even with the interface package built" OFF)

# tee_app_gui(<target>): the sources and settings of an executable that runs the Panel.
function(tee_app_gui target)
  target_sources(${target} PRIVATE "${PROJECT_SOURCE_DIR}/app/Offline.cpp" "${PROJECT_SOURCE_DIR}/app/FrameSources.cpp"
                                   "${PROJECT_SOURCE_DIR}/app/GarageLibrary.cpp"
                                   "${TEE_SHELL_PANEL_SOURCE}")
  target_link_libraries(${target} PRIVATE tee_app FMinus::FMinus)
  target_compile_definitions(${target} PRIVATE "TEE_VERSION=\"${PROJECT_VERSION}\"")
  funkgui_configure_product(${target} PRODUCT TheEngineEngine OBJC_PREFIX Tee ENV_PREFIX TEE_
                            PREFS_FOLDER TheEngineEngine)
  tee_target_defaults(${target})
  # FunkGui's own sources compile in this target (its libraries are INTERFACE ones that carry their sources), and so
  # under this target's flags. They are a dependency's: warnings there are not errors (docs/plan/01-rules.md,
  # section 4), whatever TEE_WERROR says of ours.
  set(_funkgui_sources "")
  foreach(_library FunkGuiCore FunkGuiGpu FunkGuiDesktop)
    if(TARGET ${_library})
      get_target_property(_sources ${_library} INTERFACE_SOURCES)
      if(_sources)
        list(APPEND _funkgui_sources ${_sources})
      endif()
    endif()
  endforeach()
  list(FILTER _funkgui_sources EXCLUDE REGEX "\\$<")
  if(_funkgui_sources)
    if(TEE_MSVC_FRONTEND)
      set(_no_werror "/WX-")
    else()
      set(_no_werror "-Wno-error")
    endif()
    set_property(SOURCE ${_funkgui_sources} TARGET_DIRECTORY ${target} APPEND PROPERTY COMPILE_OPTIONS "${_no_werror}")
  endif()
  if(TEE_MSVC_FRONTEND)
    # as FunkGui's consumers build it on Windows: large objects, and windows.h without its rarely used parts
    target_compile_options(${target} PRIVATE /bigobj /utf-8)
    target_compile_definitions(${target} PRIVATE WIN32_LEAN_AND_MEAN)
  endif()
  set_property(GLOBAL APPEND PROPERTY TEE_APP_GUI_TARGETS ${target})
endfunction()

# Where tee::ui::makeMainPanel comes from is settled once every directory has been read (the root may add ui/ before
# or after app/): the interface package's target when there is one, else the shell's plain Panel.
function(tee_app_panels)
  get_property(_targets GLOBAL PROPERTY TEE_APP_GUI_TARGETS)
  set(_ui "")
  foreach(_candidate tee_ui tee_ui_ref)
    if(NOT _ui AND TARGET ${_candidate} AND NOT TEE_PLACEHOLDER_PANEL)
      set(_ui ${_candidate})
    endif()
  endforeach()
  foreach(_target IN LISTS _targets)
    if(_ui)
      target_link_libraries(${_target} PRIVATE ${_ui})
      target_compile_definitions(${_target} PRIVATE TEE_HAS_UI=1)
      # the application's frame over a tee::Build, where the interface package built its binding to the host
      if(TARGET tee_ui_build)
        target_link_libraries(${_target} PRIVATE tee_ui_build)
        target_compile_definitions(${_target} PRIVATE TEE_HAS_UI_FRAME=1)
      endif()
    else()
      target_sources(${_target} PRIVATE "${TEE_SHELL_PANEL_MAIN_SOURCE}")
    endif()
  endforeach()
  if(_ui)
    message(STATUS "TheEngineEngine: the application's Panel is ${_ui}'s (--panel plain shows the shell's own)")
  else()
    message(STATUS "TheEngineEngine: the application's Panel is the shell's plain one")
  endif()
endfunction()
cmake_language(DEFER DIRECTORY "${PROJECT_SOURCE_DIR}" CALL tee_app_panels)

add_executable(tee-headless Headless.cpp)
target_link_libraries(tee-headless PRIVATE FunkGui::core)
tee_app_gui(tee-headless)
if(TEE_BUILD_APP)
  # In a tree that builds the window, the interface package is built for the GPU (its shader registry calls FunkGui's
  # GPU layer), so tee-headless links that layer too, and the desktop host under it (which defines the three
  # functions the layer's context asks its host for). It opens no window and starts no GPU all the same.
  target_link_libraries(tee-headless PRIVATE FunkGui::gpu FunkGui::desktop)
  funkgui_compile_shaders(tee-headless)
endif()

# The simulator's source and the content beside the executables, where they look for them (app/Engine.h): copied at
# every build, so an edited .fm file is the one the next run compiles.
add_custom_target(tee_app_data ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/sim" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sim"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/content"
          "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/content"
  COMMENT "sim/ and content/ beside the executables"
  VERBATIM)
add_dependencies(tee-headless tee_app_data)

if(TEE_BUILD_APP)
  include("${CMAKE_CURRENT_SOURCE_DIR}/Window.cmake")
endif()

if(TEE_BUILD_TESTS)
  add_subdirectory("${PROJECT_SOURCE_DIR}/tests/shell" "${PROJECT_BINARY_DIR}/tests/shell")
endif()
