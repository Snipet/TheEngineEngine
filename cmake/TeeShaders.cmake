# cmake/TeeShaders.cmake: a panel's shaders, built when the application is built (card J, item 3).
#
#   tee_add_panel_shaders(<target> GUI <file.gui> SHADERS <a.sc> ... [HEADERS <x.sh> ...] [NAME <panel>])
#
# <target> gets a source that defines
#
#   std::span<const tee::ui::shaders::Built> tee::ui::shaders::<panel>Shaders();
#
# (declared in the generated header <tee/ui/shaders/<panel>.h>), the table ui/kit/shaders/Registry.h describes: each
# listed shader's file name and its binary for the renderer of the platform being built for. NAME defaults to the
# target's name; it is what a GuiPanel is told its shaders were built under (GuiPanel::Options::name).
#
# How a binary is made, in two steps:
#   1. tee-shadergen (ui/kit/shaders/shadergen.cpp) writes each fragment source as TestingPlayground's shaderSource
#      did: FunkGui's prelude (shaders/fs_custom_prelude.sc, the file funkgui::shaders::fragmentPrelude() returns), the
#      names u_time, u_size, u_px, in0..in15 and in_<name> from the shader element's inputs= in the .gui file, the
#      file's string literals turned into glyph codes, and beside it the files it includes (funk/text.sh,
#      funk/shapes.sh, the HEADERS).
#   2. FunkGui's shaderc (bgfx's, at FunkGui's pin) compiles it for Metal and SPIR-V on every host and for Direct3D 11
#      on a Windows host, with FunkGui's varyings (shaders/varying_custom.def.sc), embedded by --bin2c: the rules of
#      FunkGui's own cmake/FunkGuiTargets.cmake. Every profile is built on every host so that a shader that one
#      renderer's compiler refuses fails the build everywhere it can; the table holds the platform's own.
#
# When shaders are built: when a GPU layer of FunkGui's is in the build (TEE_UI_GPU, set by ui/CMakeLists.txt) or
# TEE_UI_BUILD_SHADERS is ON (to prove the shaders compile where there is no GPU layer: it needs TEE_SHADERC and
# TEE_BGFX_SRC then). Otherwise nothing is compiled, the table is empty, and the rasteriser draws its placeholder.
#
# Where the tools come from:
#   shaderc        TEE_SHADERC (a path), else FUNKGUI_SHADERC (FunkGui's own prebuilt one), else the target `shaderc`
#                  that FunkGui's bgfx.cmake defines
#   bgfx_shader.sh TEE_BGFX_SRC/bgfx/src, else the bgfx sources FunkGui fetched (bgfx_SOURCE_DIR)
#   the prelude    TEE_FUNKGUI_SHADERS, else FunkGui's own shaders/ (FunkGui_SOURCE_DIR)
include_guard(GLOBAL)

option(TEE_UI_BUILD_SHADERS "Build panel shaders although no GPU layer is in the build (needs TEE_SHADERC, TEE_BGFX_SRC)" OFF)
set(TEE_SHADERC "" CACHE FILEPATH "bgfx's shaderc, prebuilt, for panel shaders (else FunkGui's)")
set(TEE_BGFX_SRC "" CACHE PATH "bgfx.cmake's sources (bgfx/src/bgfx_shader.sh under it), for panel shaders (else FunkGui's)")
set(TEE_FUNKGUI_SHADERS "" CACHE PATH "FunkGui's shaders/ folder, for panel shaders (else FunkGui_SOURCE_DIR/shaders)")

# `content` written to `path`, only when it differs from what is there (so nothing is rebuilt for a configure).
function(tee_write_if_changed path content)
  if(EXISTS "${path}")
    file(READ "${path}" _had)
    if(_had STREQUAL content)
      return()
    endif()
  endif()
  file(WRITE "${path}" "${content}")
endfunction()

function(_tee_shader_tools out_shaderc out_dep out_bgfx out_funkgui)
  if(TEE_SHADERC)
    set(_shaderc "${TEE_SHADERC}")
    set(_dep "${TEE_SHADERC}")
  elseif(FUNKGUI_SHADERC)
    set(_shaderc "${FUNKGUI_SHADERC}")
    set(_dep "${FUNKGUI_SHADERC}")
  elseif(TARGET shaderc)
    set(_shaderc "$<TARGET_FILE:shaderc>")
    set(_dep shaderc)
  else()
    message(FATAL_ERROR "tee_add_panel_shaders: no shaderc (set TEE_SHADERC, or build FunkGui with its GPU layer)")
  endif()
  if(TEE_BGFX_SRC)
    set(_bgfx "${TEE_BGFX_SRC}")
  else()
    set(_bgfx "${bgfx_SOURCE_DIR}")
    if(NOT _bgfx)
      include(FetchContent)
      FetchContent_GetProperties(bgfx SOURCE_DIR _bgfx)
    endif()
  endif()
  if(NOT EXISTS "${_bgfx}/bgfx/src/bgfx_shader.sh")
    message(FATAL_ERROR "tee_add_panel_shaders: cannot find bgfx/src/bgfx_shader.sh under '${_bgfx}' (set TEE_BGFX_SRC)")
  endif()
  if(TEE_FUNKGUI_SHADERS)
    set(_fg "${TEE_FUNKGUI_SHADERS}")
  else()
    set(_fg "${FunkGui_SOURCE_DIR}/shaders")
    if(NOT EXISTS "${_fg}/fs_custom_prelude.sc")
      set(_fg "${funkgui_SOURCE_DIR}/shaders")
    endif()
  endif()
  if(NOT EXISTS "${_fg}/fs_custom_prelude.sc" OR NOT EXISTS "${_fg}/varying_custom.def.sc")
    message(FATAL_ERROR "tee_add_panel_shaders: cannot find FunkGui's shaders/ ('${_fg}'; set TEE_FUNKGUI_SHADERS)")
  endif()
  set(${out_shaderc} "${_shaderc}" PARENT_SCOPE)
  set(${out_dep} "${_dep}" PARENT_SCOPE)
  set(${out_bgfx} "${_bgfx}" PARENT_SCOPE)
  set(${out_funkgui} "${_fg}" PARENT_SCOPE)
endfunction()

function(tee_add_panel_shaders target)
  cmake_parse_arguments(PARSE_ARGV 1 _tp "" "GUI;NAME" "SHADERS;HEADERS")
  if(NOT TARGET ${target})
    message(FATAL_ERROR "tee_add_panel_shaders: '${target}' is not a target")
  endif()
  if(_tp_UNPARSED_ARGUMENTS OR NOT _tp_GUI)
    message(FATAL_ERROR "tee_add_panel_shaders(${target} GUI <file.gui> SHADERS <a.sc> ... [HEADERS <x.sh> ...] [NAME <panel>])")
  endif()
  if(NOT _tp_NAME)
    set(_tp_NAME "${target}")
  endif()
  if(NOT _tp_NAME MATCHES "^[A-Za-z][A-Za-z0-9]*$")
    message(FATAL_ERROR "tee_add_panel_shaders(${target}): NAME '${_tp_NAME}' must be letters and digits (it names a function)")
  endif()
  set(_name "${_tp_NAME}")
  set(_out "${CMAKE_CURRENT_BINARY_DIR}/tee-shaders/${_name}")
  set(_inc "${_out}/include")
  file(MAKE_DIRECTORY "${_inc}/tee/ui/shaders")

  set(_build OFF)
  if((TEE_UI_GPU OR TEE_UI_BUILD_SHADERS) AND _tp_SHADERS)
    set(_build ON)
  endif()

  # The header, the same in every build.
  set(_h "#pragma once\n\n// Generated by cmake/TeeShaders.cmake for the panel \"${_name}\": do not edit.\n")
  string(APPEND _h "#include \"kit/shaders/Registry.h\"\n\n#include <span>\n\n")
  string(APPEND _h "namespace tee::ui::shaders\n{\n    std::span<const Built> ${_name}Shaders();\n}\n")
  tee_write_if_changed("${_inc}/tee/ui/shaders/${_name}.h" "${_h}")

  set(_cpp "// Generated by cmake/TeeShaders.cmake for the panel \"${_name}\": do not edit.\n")
  string(APPEND _cpp "#include \"tee/ui/shaders/${_name}.h\"\n\n#include <cstdint>\n\n")
  set(_headers "")
  set(_rows "")
  if(_build)
    _tee_shader_tools(_shaderc _shaderc_dep _bgfx _fg)
    set(_profiles mtl=osx:metal spv=linux:spirv)
    if(CMAKE_HOST_WIN32)
      list(APPEND _profiles dx11=windows:s_5_0)
    endif()
    set(_gen_args "")
    set(_gen_outputs "")
    set(_gen_inputs "${_tp_GUI}")
    foreach(_h_file IN LISTS _tp_HEADERS)
      get_filename_component(_h_abs "${_h_file}" ABSOLUTE)
      list(APPEND _gen_args --header "${_h_abs}")
      list(APPEND _gen_inputs "${_h_abs}")
    endforeach()
    foreach(_sc IN LISTS _tp_SHADERS)
      get_filename_component(_sc_abs "${_sc}" ABSOLUTE)
      get_filename_component(_stem "${_sc}" NAME_WE)
      list(APPEND _gen_args "${_sc_abs}")
      list(APPEND _gen_inputs "${_sc_abs}")
      list(APPEND _gen_outputs "${_out}/${_stem}/fs.sc")
    endforeach()
    get_filename_component(_gui_abs "${_tp_GUI}" ABSOLUTE)
    add_custom_command(OUTPUT ${_gen_outputs}
      COMMAND tee-shadergen --gui "${_gui_abs}" --prelude "${_fg}/fs_custom_prelude.sc" --out "${_out}" ${_gen_args}
      DEPENDS tee-shadergen ${_gen_inputs} "${_fg}/fs_custom_prelude.sc"
      COMMENT "tee-shadergen: the panel ${_name}'s fragment sources"
      VERBATIM)
    foreach(_sc IN LISTS _tp_SHADERS)
      get_filename_component(_stem "${_sc}" NAME_WE)
      get_filename_component(_file "${_sc}" NAME)
      string(MAKE_C_IDENTIFIER "${_name}_${_stem}" _sym)
      foreach(_p IN LISTS _profiles)
        string(REGEX MATCH "^([a-z0-9]+)=([a-z]+):([a-z0-9_]+)$" _ok "${_p}")
        set(_ext ${CMAKE_MATCH_1})
        set(_platform ${CMAKE_MATCH_2})
        set(_profile ${CMAKE_MATCH_3})
        add_custom_command(OUTPUT "${_out}/${_sym}.${_ext}.h"
          COMMAND "${_shaderc}" -f "${_out}/${_stem}/fs.sc" -o "${_out}/${_sym}.${_ext}.h"
                  --type fragment --platform ${_platform} -p ${_profile}
                  --varyingdef "${_fg}/varying_custom.def.sc"
                  -i "${_out}/${_stem}" -i "${_bgfx}/bgfx/src" --bin2c ${_sym}_${_ext}
          DEPENDS ${_shaderc_dep} "${_out}/${_stem}/fs.sc" "${_fg}/varying_custom.def.sc"
          COMMENT "shaderc: ${_name}/${_file} -> ${_profile}"
          VERBATIM)
        list(APPEND _headers "${_out}/${_sym}.${_ext}.h")
      endforeach()
      # The platform's own: Direct3D 11 on Windows, Metal on macOS, SPIR-V (Vulkan) elsewhere, as FunkGui's renderer.
      string(APPEND _cpp "#if defined(_WIN32)\n#include \"${_sym}.dx11.h\"\n#define TEE_SHADER_${_sym} ${_sym}_dx11\n")
      string(APPEND _cpp "#elif defined(__APPLE__)\n#include \"${_sym}.mtl.h\"\n#define TEE_SHADER_${_sym} ${_sym}_mtl\n")
      string(APPEND _cpp "#else\n#include \"${_sym}.spv.h\"\n#define TEE_SHADER_${_sym} ${_sym}_spv\n#endif\n")
      string(APPEND _rows "            { \"${_name}\", \"${_file}\", TEE_SHADER_${_sym}, static_cast<uint32_t>(sizeof TEE_SHADER_${_sym}) },\n")
    endforeach()
  endif()
  string(APPEND _cpp "\nnamespace tee::ui::shaders\n{\n    std::span<const Built> ${_name}Shaders()\n    {\n")
  if(_rows)
    string(APPEND _cpp "        static const Built kTable[] = {\n${_rows}        };\n        return kTable;\n")
  else()
    string(APPEND _cpp "        return {};                                   // no shaders in this build (no GPU layer)\n")
  endif()
  string(APPEND _cpp "    }\n}\n")
  tee_write_if_changed("${_out}/${_name}_shaders.cpp" "${_cpp}")

  target_sources(${target} PRIVATE "${_out}/${_name}_shaders.cpp" ${_headers})
  target_include_directories(${target} PUBLIC "${_inc}")
  target_include_directories(${target} PRIVATE "${_out}")
  if(_headers)
    set_source_files_properties("${_out}/${_name}_shaders.cpp" PROPERTIES OBJECT_DEPENDS "${_headers}")
  endif()
endfunction()
