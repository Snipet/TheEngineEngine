# cmake/TeeDeps.cmake: the dependency cache, the compiler cache, and the dependencies themselves, each at its pin
# (cmake/TeePins.cmake).
#
#   TEE_DEPS_DIR      the dependency cache, shared by every build tree and every checkout on a machine:
#                       src/<name>-<version>/   each dependency's source, cloned by git at its pinned commit
#                       install/llvm-<...>/     FMinus's prebuilt LLVM (scripts/fetch-llvm.sh unpacks it; never built here)
#                       ccache/                 the compiler cache, unless CCACHE_DIR says otherwise
#                     It is FMinus's deps cache too (FMINUS_DEPS_DIR), so an FMinus checkout's own .deps serves as well.
#                     Default: the environment's TEE_DEPS_DIR; else /home/user/.tee-deps where scripts/bootstrap-cloud.sh
#                     made it (a cloud container); else .tee-deps/ in this checkout.
#
#   tee_source(<name> <out>)   the directory holding dependency <name>'s source at its pin, cloned on first use.
#                              -DFETCHCONTENT_SOURCE_DIR_<NAME>=<dir> builds a checkout of your own instead, as it is:
#                              the commit is not checked then.
#
# Gives the targets FMinus::FMinus, FunkGui::core and, on request, Catch2::Catch2WithMain and nlohmann_json::nlohmann_json.
# Dependencies keep their own flags: nothing of cmake/TeeFlags.cmake reaches them.

include_guard(GLOBAL)

# ---- the cache -------------------------------------------------------------------------------------------------------
if(DEFINED ENV{TEE_DEPS_DIR} AND NOT "$ENV{TEE_DEPS_DIR}" STREQUAL "")
  set(_tee_deps_default "$ENV{TEE_DEPS_DIR}")
elseif(IS_DIRECTORY "/home/user/.tee-deps")
  set(_tee_deps_default "/home/user/.tee-deps")
elseif(DEFINED TEE_SOURCE_DIR)
  set(_tee_deps_default "${TEE_SOURCE_DIR}/.tee-deps")
else()
  set(_tee_deps_default "${CMAKE_CURRENT_LIST_DIR}/../.tee-deps")
endif()
set(TEE_DEPS_DIR "${_tee_deps_default}" CACHE PATH "The dependency cache: sources, FMinus's LLVM, the compiler cache")
cmake_path(ABSOLUTE_PATH TEE_DEPS_DIR NORMALIZE)
file(MAKE_DIRECTORY "${TEE_DEPS_DIR}/src" "${TEE_DEPS_DIR}/install")

# ---- sources by git --------------------------------------------------------------------------------------------------
# One shallow fetch of the pinned commit, checked out and compared with the pin, then marked (.tee-commit) so that
# every later configure of any build tree uses the directory as it is. A lock keeps two configures from cloning into
# one directory at once.
function(tee_source name out)
  string(TOUPPER "${name}" _upper)
  if(NOT DEFINED TEE_${_upper}_COMMIT)
    message(FATAL_ERROR "TheEngineEngine: no pin named '${name}' in cmake/TeePins.cmake")
  endif()
  if(FETCHCONTENT_SOURCE_DIR_${_upper})
    if(NOT IS_DIRECTORY "${FETCHCONTENT_SOURCE_DIR_${_upper}}")
      message(FATAL_ERROR "TheEngineEngine: FETCHCONTENT_SOURCE_DIR_${_upper} is '${FETCHCONTENT_SOURCE_DIR_${_upper}}', "
                          "which is no directory")
    endif()
    message(STATUS "TheEngineEngine: ${name} from ${FETCHCONTENT_SOURCE_DIR_${_upper}} as it is, not the pin")
    set(${out} "${FETCHCONTENT_SOURCE_DIR_${_upper}}" PARENT_SCOPE)
    return()
  endif()

  set(_commit "${TEE_${_upper}_COMMIT}")
  set(_dir "${TEE_DEPS_DIR}/src/${TEE_${_upper}_SOURCE_NAME}")
  set(_marker "${_dir}/.tee-commit")
  set(${out} "${_dir}" PARENT_SCOPE)
  if(EXISTS "${_marker}")
    file(READ "${_marker}" _have)
    if(_have STREQUAL _commit)
      return()
    endif()
  endif()

  file(LOCK "${_dir}.tee-lock" GUARD FUNCTION)
  if(EXISTS "${_marker}")                       # another configure cloned it while this one waited
    file(READ "${_marker}" _have)
    if(_have STREQUAL _commit)
      return()
    endif()
  endif()
  find_package(Git REQUIRED)
  message(STATUS "TheEngineEngine: cloning ${name} ${TEE_${_upper}_VERSION} (${_commit}) into ${_dir}")
  file(REMOVE_RECURSE "${_dir}")
  file(MAKE_DIRECTORY "${_dir}")
  foreach(_step
      "init;--quiet"
      "remote;add;origin;${TEE_${_upper}_REPOSITORY}"
      "fetch;--quiet;--depth;1;origin;${_commit}"
      "-c;advice.detachedHead=false;checkout;--quiet;FETCH_HEAD")
    execute_process(COMMAND "${GIT_EXECUTABLE}" ${_step} WORKING_DIRECTORY "${_dir}"
                    RESULT_VARIABLE _rc ERROR_VARIABLE _err)
    if(NOT _rc EQUAL 0)
      message(FATAL_ERROR "TheEngineEngine: cloning ${name}: git ${_step} failed (${_rc}) in ${_dir}:\n${_err}")
    endif()
  endforeach()
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD WORKING_DIRECTORY "${_dir}"
                  OUTPUT_VARIABLE _head OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _rc)
  if(NOT _rc EQUAL 0 OR NOT _head STREQUAL _commit)
    message(FATAL_ERROR "TheEngineEngine: ${name}: ${_dir} is at '${_head}', and the pin is ${_commit}")
  endif()
  file(WRITE "${_marker}" "${_commit}")
endfunction()

# Script mode (cmake/TeeFetch.cmake) stops here: it wants the clones and nothing else.
if(CMAKE_SCRIPT_MODE_FILE)
  return()
endif()

# ---- the compiler cache ----------------------------------------------------------------------------------------------
# ccache (sccache where that is what there is), unless a launcher was chosen. Its folder is the dependency cache's,
# unless the environment names one (CCACHE_DIR, as CI does), so that every checkout on a machine shares one cache; and
# paths under this checkout are hashed relative to it (CCACHE_BASEDIR), so that a second checkout elsewhere hits what
# the first one compiled. TEE_COMPILER_CACHE=OFF builds without.
option(TEE_COMPILER_CACHE "Compile through ccache or sccache when one is installed" ON)
if(TEE_COMPILER_CACHE AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
  find_program(TEE_CCACHE NAMES ccache sccache)
  if(TEE_CCACHE)
    set(_tee_launcher "${TEE_CCACHE}")
    cmake_path(GET TEE_CCACHE STEM _tee_ccache_name)
    if(_tee_ccache_name STREQUAL "ccache" AND NOT DEFINED ENV{CCACHE_DIR})
      file(MAKE_DIRECTORY "${TEE_DEPS_DIR}/ccache")
      set(_tee_launcher "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${TEE_DEPS_DIR}/ccache"
                        "CCACHE_BASEDIR=${PROJECT_SOURCE_DIR}" CCACHE_NOHASHDIR=true "${TEE_CCACHE}")
    endif()
    set(CMAKE_C_COMPILER_LAUNCHER "${_tee_launcher}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${_tee_launcher}")
    message(STATUS "TheEngineEngine: compiling through ${TEE_CCACHE}")
  endif()
endif()

# ---- FMinus ----------------------------------------------------------------------------------------------------------
# A subproject: its library and runtime only (it leaves its tests, tools and examples out when it is not the top
# level), its warnings not errors here, its JSON and XML as it defaults them (on).
#
# Its LLVM is found, never built: FMinus's default would build LLVM into the cache at the first configure (10 to 30
# minutes), so without an install the configure stops here and says what to run.
set(TEE_LLVM_DIR "" CACHE PATH
    "The LLVM install FMinus links (its prefix, holding lib/cmake/llvm); empty: chosen from TEE_DEPS_DIR/install")
set(_tee_llvm "${TEE_LLVM_DIR}")
if(NOT _tee_llvm)
  set(_tee_llvm_roots "${TEE_DEPS_DIR}")
  if(DEFINED ENV{FMINUS_DEPS_DIR} AND NOT "$ENV{FMINUS_DEPS_DIR}" STREQUAL "")
    list(APPEND _tee_llvm_roots "$ENV{FMINUS_DEPS_DIR}")
  endif()
  set(_tee_llvm_plain "")
  set(_tee_llvm_assert "")
  foreach(_root IN LISTS _tee_llvm_roots)
    file(GLOB _found LIST_DIRECTORIES true "${_root}/install/llvm-${TEE_LLVM_VERSION}-*")
    foreach(_d IN LISTS _found)
      if(NOT EXISTS "${_d}/lib/cmake/llvm/LLVMConfig.cmake")
        continue()                                   # a lock file, or an install that is not whole
      endif()
      if(_d MATCHES "-assert-")
        list(APPEND _tee_llvm_assert "${_d}")
      else()
        list(APPEND _tee_llvm_plain "${_d}")
      endif()
    endforeach()
  endforeach()
  # A Release build takes an LLVM without assertions when there is one; any other build one with them (it catches
  # malformed IR at once), when there is one.
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(_tee_llvm_order ${_tee_llvm_plain} ${_tee_llvm_assert})
  else()
    set(_tee_llvm_order ${_tee_llvm_assert} ${_tee_llvm_plain})
  endif()
  if(_tee_llvm_order)
    list(GET _tee_llvm_order 0 _tee_llvm)
  endif()
endif()
if(NOT _tee_llvm OR NOT EXISTS "${_tee_llvm}/lib/cmake/llvm/LLVMConfig.cmake")
  message(FATAL_ERROR
    "TheEngineEngine: no LLVM ${TEE_LLVM_VERSION} for FMinus under ${TEE_DEPS_DIR}/install, and none is built here "
    "(it takes 10 to 30 minutes). Do one of:\n"
    "  scripts/bootstrap-cloud.sh        a cloud container (Ubuntu 24.04): everything, the prebuilt LLVM included\n"
    "  scripts/fetch-llvm.sh             any system: FMinus's prebuilt LLVM alone, into TEE_DEPS_DIR/install\n"
    "  -DTEE_DEPS_DIR=<FMinus>/.deps     the cache of an FMinus checkout that has built its own\n"
    "  -DTEE_LLVM_DIR=<prefix>           an LLVM ${TEE_LLVM_VERSION} install of your own\n"
    "then configure again.")
endif()
message(STATUS "TheEngineEngine: FMinus's LLVM is ${_tee_llvm}")

# FMinus's four runtime dependencies come as archives by URL, by its own pins. Where scripts/bootstrap-cloud.sh has
# cloned them by git instead (a machine that refuses the archives), FMinus is pointed at the clones, in the directories
# it would have unpacked into; everywhere else its pins are used untouched.
foreach(_dep pffft fast_float yyjson pugixml)
  string(TOUPPER "${_dep}" _upper)
  set(_dir "${TEE_DEPS_DIR}/src/${TEE_${_upper}_SOURCE_NAME}")
  if(EXISTS "${_dir}/.tee-commit" AND NOT FETCHCONTENT_SOURCE_DIR_FMINUS_${_upper})
    file(READ "${_dir}/.tee-commit" _have)
    if(_have STREQUAL TEE_${_upper}_COMMIT)
      set(FETCHCONTENT_SOURCE_DIR_FMINUS_${_upper} "${_dir}")
    endif()
  endif()
endforeach()

tee_source(fminus _tee_fminus_dir)
# The four pins above must be FMinus's own at this commit: its file is read, and a difference stops the configure.
include("${_tee_fminus_dir}/cmake/FMinusPins.cmake")
foreach(_check "LLVM_VERSION;${TEE_LLVM_VERSION}" "PFFFT_COMMIT;${TEE_PFFFT_COMMIT}"
               "FAST_FLOAT_VERSION;${TEE_FAST_FLOAT_VERSION}" "YYJSON_VERSION;${TEE_YYJSON_VERSION}"
               "PUGIXML_VERSION;${TEE_PUGIXML_VERSION}")
  list(GET _check 0 _key)
  list(GET _check 1 _ours)
  if(NOT "${FMINUS_${_key}}" STREQUAL "${_ours}")
    message(FATAL_ERROR "TheEngineEngine: FMinus at ${_tee_fminus_dir} pins FMINUS_${_key} '${FMINUS_${_key}}', and "
                        "cmake/TeePins.cmake says '${_ours}': move the pin there with FMinus's")
  endif()
endforeach()

# Its options, as normal variables (CMP0077, and CMP0126 for the cache variables it declares).
set(FMINUS_LLVM_MODE system)
set(LLVM_DIR "${_tee_llvm}/lib/cmake/llvm")
set(FMINUS_DEPS_DIR "${TEE_DEPS_DIR}")
# Bison and Flex. On Linux the system's (Ubuntu's Bison 3.8.2 and Flex 2.6.4 are the versions FMinus pins; the ones in
# FMinus's LLVM archive want a library an Ubuntu container lacks). Elsewhere FMinus's own way: the installs the
# archive brought into the cache (WinFlexBison on Windows), which it finds by their stamps and builds nothing.
# A cache variable of ours, set into FMinus's on every configure: FMinus's own cache entry, which its first configure
# creates as "fetch", must not be what a second configure reads.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  set(_tee_parser_tools_default system)
else()
  set(_tee_parser_tools_default fetch)
endif()
set(TEE_PARSER_TOOLS_MODE "${_tee_parser_tools_default}" CACHE STRING
    "Where FMinus's Bison and Flex come from: system (the PATH's) or fetch (FMinus's installs in the dependency cache)")
set_property(CACHE TEE_PARSER_TOOLS_MODE PROPERTY STRINGS system fetch)
set(FMINUS_PARSER_TOOLS_MODE "${TEE_PARSER_TOOLS_MODE}")
if(TEE_PARSER_TOOLS_MODE STREQUAL "system")
  # A tree whose earlier configure ran in fetch mode cached the cache's own tools: forget them, so the PATH's are found.
  foreach(_tool BISON_EXECUTABLE FLEX_EXECUTABLE FLEX_INCLUDE_DIR)
    if(DEFINED CACHE{${_tool}})
      cmake_path(IS_PREFIX TEE_DEPS_DIR "$CACHE{${_tool}}" NORMALIZE _tee_in_cache)
      if(_tee_in_cache)
        unset(${_tool} CACHE)
      endif()
    endif()
  endforeach()
endif()
set(FMINUS_BUILD_TESTS OFF)
set(FMINUS_BUILD_TOOLS OFF)
set(FMINUS_BUILD_EXAMPLES OFF)
set(FMINUS_WERROR OFF)                       # third-party code never gets -Werror
set(FMINUS_JSON ON)
add_subdirectory("${_tee_fminus_dir}" "${PROJECT_BINARY_DIR}/_deps/fminus" EXCLUDE_FROM_ALL SYSTEM)

# ---- SDL3 (the shell's: shell/cmake/TeeSdl.cmake) --------------------------------------------------------------------
# Before FunkGui, whose desktop host takes the SDL3::SDL3 it finds defined.
include("${TEE_SOURCE_DIR}/shell/cmake/TeeSdl.cmake")

# ---- FunkGui ---------------------------------------------------------------------------------------------------------
# Its JUCE-free core for now: no JUCE, no bgfx, no tools, no tests, and nothing fetched. FunkGui::core is an INTERFACE
# library: its sources compile into the target that links it, which also calls funkgui_configure_product(). When the pin
# moves to the release with the desktop host, TEE_BUILD_APP turns FunkGui's GPU layer on here.
set(FUNKGUI_WITH_JUCE OFF)
set(FUNKGUI_BUILD_TOOLS OFF)
set(FUNKGUI_BUILD_TESTS OFF)
tee_source(funkgui _tee_funkgui_dir)
add_subdirectory("${_tee_funkgui_dir}" "${PROJECT_BINARY_DIR}/_deps/funkgui" EXCLUDE_FROM_ALL SYSTEM)

# ---- nlohmann/json ---------------------------------------------------------------------------------------------------
# Header-only: its include directory, as a system one, and none of its own CMake.
tee_source(nlohmann_json _tee_json_dir)
if(NOT TARGET nlohmann_json::nlohmann_json)
  add_library(tee_nlohmann_json INTERFACE)
  target_include_directories(tee_nlohmann_json SYSTEM INTERFACE "${_tee_json_dir}/single_include")
  add_library(nlohmann_json::nlohmann_json ALIAS tee_nlohmann_json)
endif()

# ---- Catch2 (tests only) ---------------------------------------------------------------------------------------------
# Catch2's library is compiled as C++20, as the tests are: it asks only for C++14, and at the compiler's default (C++14
# or older with Apple's clang and with clang-cl) it leaves out what a C++17 test then links against, such as its
# printer of std::string_view. A function, so that the standard is set for Catch2's directory alone.
function(_tee_add_catch2 dir)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CATCH_INSTALL_DOCS OFF)
  set(CATCH_INSTALL_EXTRAS OFF)
  add_subdirectory("${dir}" "${PROJECT_BINARY_DIR}/_deps/catch2" EXCLUDE_FROM_ALL SYSTEM)
endfunction()
if(TEE_BUILD_TESTS)
  tee_source(catch2 _tee_catch2_dir)
  _tee_add_catch2("${_tee_catch2_dir}")
  list(APPEND CMAKE_MODULE_PATH "${_tee_catch2_dir}/extras")       # Catch.cmake: catch_discover_tests()
endif()
