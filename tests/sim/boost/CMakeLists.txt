# tests/sim/boost/: card F's tests (the forced induction, sim/boost/): tee-tests-boost, run by ctest as boost.*.
# Reached by tests/CMakeLists.txt's glob of tests/sim/*/. Every .cpp here is a source (adding one edits no CMake).
if(TARGET tee-tests-boost)
  return()
endif()
include(Catch)
file(GLOB _tee_boost_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-boost ${_tee_boost_sources})
tee_target_defaults(tee-tests-boost)
target_include_directories(tee-tests-boost PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-boost PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-boost PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-tests-boost DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "boost." PROPERTIES TIMEOUT 900)
