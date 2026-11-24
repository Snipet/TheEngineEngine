# tests/: Catch2 tests, run by ctest. Every .cpp here is a source of tee-tests (adding a file edits no CMake);
# tests/sim/*.cpp are sources too (the simulator's own). tests/support/ holds what tests share
# (tee::test::nullDepth and the rest), headers only.
#
# A simulator package's tests are its own directory, tests/sim/<package>/, with a CMakeLists.txt of its own, which the
# glob at the end of this file adds: nobody edits this file to add one. The rule for that CMakeLists.txt:
#   - it begins with a guard, so that a second read does nothing:
#       if(TARGET tee-tests-<package>)
#         return()
#       endif()
#   - it makes one executable, tee-tests-<package>, from the directory's own sources, with tee_target_defaults(),
#     linking Tee::host and Catch2::Catch2WithMain;
#   - it registers its tests with catch_discover_tests(tee-tests-<package> DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS
#     TEST_PREFIX "<package>." PROPERTIES TIMEOUT 900), so that ctest -R '^<package>\.' runs the package's alone;
#   - a long soak (minutes, not seconds) carries the Catch tag [nightly]: ADD_TAGS_AS_LABELS makes it the ctest label
#     "nightly", which the dev and release test presets (and so CI) leave out; ctest --preset nightly runs those alone.
#
# The tests find the simulator and the content in this checkout (compiled in), and write what they keep under out/.
include(Catch)
file(GLOB _tee_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/sim/*.cpp")
add_executable(tee-tests ${_tee_test_sources})
tee_target_defaults(tee-tests)
target_include_directories(tee-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-tests DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS PROPERTIES TIMEOUT 900)
add_subdirectory(host)   # card C: the host's and the tools' tests (tests/host/CMakeLists.txt)

# ---- the simulator packages' tests (the rule above) ------------------------------------------------------------------
file(GLOB _tee_sim_test_lists CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sim/*/CMakeLists.txt")
list(SORT _tee_sim_test_lists)
foreach(_list IN LISTS _tee_sim_test_lists)
  cmake_path(GET _list PARENT_PATH _dir)
  add_subdirectory("${_dir}")
endforeach()
