# cmake/TeeFetch.cmake: clones every pinned dependency into the dependency cache, without configuring anything.
# scripts/bootstrap-cloud.sh runs it, so that a configure afterwards needs no network:
#
#   cmake [-DTEE_DEPS_DIR=<dir>] [-DTEE_FETCH="fminus;catch2"] -P cmake/TeeFetch.cmake
#
# TEE_FETCH names the pins to clone; without it, all of cmake/TeePins.cmake's: TheEngineEngine's own and the four that
# FMinus would otherwise fetch as archives by URL (which a cloud container refuses).
cmake_minimum_required(VERSION 3.30)
set(TEE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/TeePins.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/TeeDeps.cmake")
if(NOT TEE_FETCH)
  set(TEE_FETCH ${TEE_PINS})
endif()
foreach(_name IN LISTS TEE_FETCH)
  tee_source(${_name} _dir)
  string(TOUPPER "${_name}" _upper)
  message(STATUS "${_name} ${TEE_${_upper}_VERSION}: ${_dir}")
endforeach()
