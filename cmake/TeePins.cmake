# cmake/TeePins.cmake: every dependency of TheEngineEngine, in one place: its name, its repository, the commit it is
# pinned to, its version and its licence. Everything is fetched by git at that commit (git names a commit by the hash
# of its contents, so a clone is checked as it is made); nothing is taken from the system but the compiler, Ninja and,
# on Linux, Bison and Flex.
#
# Read by cmake/TeeDeps.cmake (the build), by cmake/TeeFetch.cmake (scripts/bootstrap-cloud.sh's clones) and by
# .github/workflows/build.yml (this file's hash keys the caches; TEE_LLVM_VERSION names the LLVM archive).
#
# Moving a pin: change its COMMIT and VERSION together, here and nowhere else. A tag is written for the reader only:
# the commit is what is fetched and checked.
#
#   tee_pin(<name> REPOSITORY <url> COMMIT <40 hex> VERSION <text> LICENSE <SPDX> [SOURCE_NAME <directory>])
# defines TEE_<NAME>_REPOSITORY, _COMMIT, _VERSION, _LICENSE and _SOURCE_NAME (the directory's name under
# ${TEE_DEPS_DIR}/src; <name>-<version> unless given) and adds <name> to TEE_PINS.

set(TEE_PINS "")
macro(tee_pin name)
  cmake_parse_arguments(_pin "" "REPOSITORY;COMMIT;VERSION;LICENSE;SOURCE_NAME" "" ${ARGN})
  string(TOUPPER "${name}" _pin_upper)
  if(NOT _pin_COMMIT MATCHES "^[0-9a-f]+$" OR NOT _pin_REPOSITORY OR NOT _pin_VERSION OR NOT _pin_LICENSE)
    message(FATAL_ERROR "TheEngineEngine: the pin of ${name} wants REPOSITORY, COMMIT (40 hex digits), VERSION and LICENSE")
  endif()
  string(LENGTH "${_pin_COMMIT}" _pin_length)
  if(NOT _pin_length EQUAL 40)
    message(FATAL_ERROR "TheEngineEngine: the pin of ${name}: COMMIT is ${_pin_length} characters, not a full SHA-1")
  endif()
  if(NOT _pin_SOURCE_NAME)
    set(_pin_SOURCE_NAME "${name}-${_pin_VERSION}")
  endif()
  foreach(_pin_key REPOSITORY COMMIT VERSION LICENSE SOURCE_NAME)
    set(TEE_${_pin_upper}_${_pin_key} "${_pin_${_pin_key}}")
  endforeach()
  list(APPEND TEE_PINS "${name}")
endmacro()

# ---- what TheEngineEngine links --------------------------------------------------------------------------------------

# FMinus: the compiler and JIT the simulator is written for (include/fminus/FMinus.h is all the host includes).
tee_pin(fminus
  REPOSITORY "https://github.com/Snipet/FMinus.git"
  COMMIT 5b665d746e48dcea6914dbc04f815c5376215b52      # main, 2026-11-23
  VERSION 0.1.0-5b665d7
  LICENSE GPL-3.0-only)

# FunkGui: the interface library. v0.27.0 for now, its JUCE-free core only; the pin moves to the release that has the
# desktop host (docs/plan/w1-a-funkgui-desktop.md), and TEE_BUILD_APP turns on with it.
tee_pin(funkgui
  REPOSITORY "https://github.com/Snipet/FunkGui.git"
  COMMIT b7bc1cd30be28f09a97fe1bff9fd6b66b5709bad      # v0.27.0
  VERSION 0.27.0
  LICENSE GPL-3.0-only)

# Catch2 v3: the tests (tests/).
tee_pin(catch2
  REPOSITORY "https://github.com/catchorg/Catch2.git"
  COMMIT 08092139210881f01a34e60c5f2fd6afa0ea4024      # v3.16.1
  VERSION 3.16.1
  LICENSE BSL-1.0)

# nlohmann/json: presets, scenarios and the files of the tools (header-only).
tee_pin(nlohmann_json
  REPOSITORY "https://github.com/nlohmann/json.git"
  COMMIT 55f93686c01528224f448c19128836e7df245f72      # v3.12.0
  VERSION 3.12.0
  LICENSE MIT)

# ---- FMinus's own, which it pins by archive (its cmake/FMinusPins.cmake) ---------------------------------------------
# FMinus fetches these four as release archives by URL, checked by SHA-256. Where archives by URL are refused (the cloud
# containers), scripts/bootstrap-cloud.sh clones each here at the commit the archive was made from, into the directory
# FMinus's CMake looks in, and cmake/TeeDeps.cmake points FMinus at the clone. On a machine that has no such clone,
# FMinus's own pins are used untouched. The versions and commits below must be those of FMinus's pins at TEE_FMINUS_COMMIT:
# cmake/TeeDeps.cmake checks the versions against FMinus's file at configure time.

# PFFFT: the FFT inside FMinus's runtime. FMinus pins a commit of its master; the directory is named by it.
tee_pin(pffft
  REPOSITORY "https://github.com/marton78/pffft.git"
  COMMIT aa16fd3db58de4ba5dae8b0438440bb9da46b6fa      # master, 2026-09-29 (FMINUS_PFFFT_COMMIT)
  VERSION aa16fd3db58de4ba5dae8b0438440bb9da46b6fa
  LICENSE BSD-3-Clause)                                # FFTPACK's notice and Pommier's, BSD-like

# fast_float: numbers read from text, in FMinus's runtime (header-only).
tee_pin(fast_float
  REPOSITORY "https://github.com/fastfloat/fast_float.git"
  COMMIT dbeabeeb3e6791cb96994c2a7a084c36e62ccfb2      # v8.3.3 (FMINUS_FAST_FLOAT_VERSION)
  VERSION 8.3.3
  LICENSE "Apache-2.0 OR MIT OR BSL-1.0")

# yyjson: the JSON parser inside FMinus's runtime (FMINUS_JSON, on).
tee_pin(yyjson
  REPOSITORY "https://github.com/ibireme/yyjson.git"
  COMMIT 6447536015f3d600f3d65323b10976103b337ca7      # 0.13.0 (FMINUS_YYJSON_VERSION)
  VERSION 0.13.0
  LICENSE MIT)

# pugixml: the XML parser inside FMinus's runtime (FMINUS_XML, on by FMinus's default).
tee_pin(pugixml
  REPOSITORY "https://github.com/zeux/pugixml.git"
  COMMIT c8033ce9d039e7f9d134877c363397b3cfe20816      # v1.16 (FMINUS_PUGIXML_VERSION)
  VERSION 1.16
  LICENSE MIT)

# ---- not fetched by git ----------------------------------------------------------------------------------------------

# LLVM, which FMinus links statically: never built here. It comes prebuilt, as the archive for the platform from the
# release deps-llvm-<version> of Snipet/FMinus (FMinus's Deps workflow makes them; each has a .sha256 beside it),
# unpacked under ${TEE_DEPS_DIR}/install (scripts/bootstrap-cloud.sh, the CI workflow), or from an FMinus checkout's own
# deps cache (TEE_DEPS_DIR or LLVM_DIR). Licence: Apache-2.0 WITH LLVM-exception.
set(TEE_LLVM_VERSION "23.1.3")
set(TEE_LLVM_RELEASE_REPOSITORY "Snipet/FMinus")
set(TEE_LLVM_RELEASE_TAG "deps-llvm-${TEE_LLVM_VERSION}")
