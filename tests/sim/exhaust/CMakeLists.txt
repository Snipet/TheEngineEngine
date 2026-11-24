# tests/sim/exhaust/: card G's tests (targets.cmake). Until the tests' own CMakeLists adds this directory, a build
# adds them with -DCMAKE_PROJECT_TheEngineEngine_INCLUDE=<this directory>/hook.cmake.
include("${CMAKE_CURRENT_LIST_DIR}/targets.cmake")
