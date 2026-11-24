# tests/sim/boost/: card F's tests (the forced induction, sim/boost/), tee-tests-boost, reached by tests/CMakeLists.txt's
# glob of tests/sim/*/. targets.cmake makes the target.
if(TARGET tee-tests-boost)
  return()
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/targets.cmake")
