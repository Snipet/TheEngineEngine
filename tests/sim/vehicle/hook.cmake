# tests/sim/vehicle/hook.cmake: builds the vehicle's tests in a tree whose tests/CMakeLists.txt does not reach
# tests/sim/vehicle/ yet. Configure with
#   -DCMAKE_PROJECT_TheEngineEngine_INCLUDE=<checkout>/tests/sim/vehicle/hook.cmake
# and targets.cmake is read once the root CMakeLists.txt has been (after Tee::host and Catch2 exist).
cmake_language(EVAL CODE "cmake_language(DEFER DIRECTORY \"${CMAKE_SOURCE_DIR}\" CALL include \"${CMAKE_CURRENT_LIST_DIR}/targets.cmake\")")
