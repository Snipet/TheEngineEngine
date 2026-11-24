# shell/cmake/TeeGpu.cmake: what FunkGui is told about its GPU layer, before it is added (cmake/TeeDeps.cmake).
#
# Without TEE_BUILD_APP: FunkGui's core alone (no bgfx, no desktop host), as tee-headless and the tests need it.
#
# With TEE_BUILD_APP: FunkGui::gpu and FunkGui::desktop, on
#   bgfx     bgfx.cmake with bgfx, bx and bimg inside it, each cloned by git at its pin (cmake/TeePins.cmake) into the
#            dependency cache; FunkGui is pointed at the clone (FETCHCONTENT_SOURCE_DIR_BGFX) and fetches nothing.
#   shaderc  bgfx's shader compiler, which FunkGui's and our own shaders are built with. A prebuilt one is used when
#            there is one: TEE_SHADERC, else ${TEE_DEPS_DIR}/tools/shaderc-<bgfx.cmake's version>/shaderc with a
#            shaderc.stamp beside it that names the pinned bgfx.cmake commit (FunkGui's own convention; CI keeps one a
#            platform in a cache). With none, bgfx's tools are built in this tree: about 2,000 CPU-seconds, once.
include_guard(GLOBAL)

set(FUNKGUI_WITH_BGFX OFF)
set(FUNKGUI_WITH_DESKTOP OFF)
if(NOT TEE_BUILD_APP)
  return()
endif()
set(FUNKGUI_WITH_BGFX ON)
set(FUNKGUI_WITH_DESKTOP ON)

# clang-cl: large objects and UTF-8 sources, for what is added from here on (bgfx and its tools, FunkGui, and our own
# targets, which lose nothing by it), as FunkGui's consumers build it on Windows. windows.h is left whole here: bgfx's
# shader compiler counts on the parts WIN32_LEAN_AND_MEAN leaves out (the targets that compile FunkGui define that
# for themselves: app/CMakeLists.txt).
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX,C>:/bigobj> $<$<COMPILE_LANGUAGE:CXX,C>:/utf-8>)
endif()

if(NOT FETCHCONTENT_SOURCE_DIR_BGFX)
  tee_source(bgfx_cmake _tee_bgfx_dir)      # first: the three below are cloned into its directory
  tee_source(bgfx _tee_bgfx_sub)
  tee_source(bx _tee_bgfx_sub)
  tee_source(bimg _tee_bgfx_sub)
  set(FETCHCONTENT_SOURCE_DIR_BGFX "${_tee_bgfx_dir}")
endif()

set(TEE_SHADERC "" CACHE FILEPATH
    "A prebuilt shaderc of the pinned bgfx, its shaderc.stamp beside it; empty: the dependency cache's, else built here")
if(TEE_SHADERC)
  set(FUNKGUI_SHADERC "${TEE_SHADERC}")
elseif(NOT FUNKGUI_SHADERC)
  set(_tee_shaderc "${TEE_DEPS_DIR}/tools/shaderc-${TEE_BGFX_CMAKE_VERSION}/shaderc")
  if(CMAKE_HOST_WIN32)
    string(APPEND _tee_shaderc ".exe")
  endif()
  set(_tee_stamp "")
  if(EXISTS "${TEE_DEPS_DIR}/tools/shaderc-${TEE_BGFX_CMAKE_VERSION}/shaderc.stamp")
    file(STRINGS "${TEE_DEPS_DIR}/tools/shaderc-${TEE_BGFX_CMAKE_VERSION}/shaderc.stamp" _tee_stamp LIMIT_COUNT 1)
  endif()
  if(EXISTS "${_tee_shaderc}" AND _tee_stamp STREQUAL "bgfx.cmake ${TEE_BGFX_CMAKE_COMMIT}")
    set(FUNKGUI_SHADERC "${_tee_shaderc}")
    message(STATUS "TheEngineEngine: the prebuilt shader compiler ${_tee_shaderc}")
  else()
    message(STATUS "TheEngineEngine: no prebuilt shaderc stamped for bgfx.cmake ${TEE_BGFX_CMAKE_COMMIT} in "
                   "${TEE_DEPS_DIR}/tools: bgfx's shader compiler is built in this tree (about 2,000 CPU-seconds, once)")
  endif()
endif()
