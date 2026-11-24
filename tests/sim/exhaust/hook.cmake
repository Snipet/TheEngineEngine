# For a build that wants card G's tests before tests/sim/ is added by the tests' own CMakeLists: targets.cmake,
# included once the top-level CMakeLists has made its targets (cmake_language(DEFER) at the end of the top directory;
# the path baked in now, as a deferred call's arguments are read when it runs).
cmake_language(EVAL CODE
  "cmake_language(DEFER DIRECTORY \"${CMAKE_SOURCE_DIR}\" CALL include [[${CMAKE_CURRENT_LIST_DIR}/targets.cmake]])")
