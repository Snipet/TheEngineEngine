# tests/sim/vehicle/: card I's tests, as tee-tests-vehicle (tests/CMakeLists.txt's glob of tests/sim/*/CMakeLists.txt
# reaches this file; until it does, hook.cmake does). The target is made in targets.cmake, which both use.
if(TARGET tee-tests-vehicle)
  return()
endif()
include("${CMAKE_CURRENT_LIST_DIR}/targets.cmake")
