# tests/sim/boost/targets.cmake: tee-boost-tests, card F's tests (the forced induction, sim/boost/, on the made-up
# engine of rig.fm), run by ctest. Every .cpp beside this file is a source (adding one edits no CMake). Included by
# this directory's CMakeLists.txt, or by hook.cmake.
if(TARGET tee-boost-tests OR NOT TARGET Tee::host)
  return()
endif()
set(_tee_boost_dir "${CMAKE_CURRENT_LIST_DIR}")
include(Catch)
file(GLOB _tee_boost_sources CONFIGURE_DEPENDS "${_tee_boost_dir}/*.cpp")
add_executable(tee-boost-tests ${_tee_boost_sources})
tee_target_defaults(tee-boost-tests)
target_include_directories(tee-boost-tests PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${_tee_boost_dir}")
target_link_libraries(tee-boost-tests PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-boost-tests PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-boost-tests DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 900)
