# tests/sim/calibration/boost/: card N2's (the boosted engines). tee-calib-boost: the measuring harness (not a test);
# tee-tests-calib-boost: the tests (ctest calibration.boost.*), on the engine dyno through Bench.h.
if(TARGET tee-calib-boost)
  return()
endif()
add_executable(tee-calib-boost "${CMAKE_CURRENT_SOURCE_DIR}/harness/main.cpp")
tee_target_defaults(tee-calib-boost)
target_include_directories(tee-calib-boost PRIVATE "${PROJECT_SOURCE_DIR}/tools")
target_link_libraries(tee-calib-boost PRIVATE Tee::host)
target_compile_definitions(tee-calib-boost PRIVATE "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\"")

include(Catch)
file(GLOB _tee_calib_boost_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-calib-boost ${_tee_calib_boost_sources})
tee_target_defaults(tee-tests-calib-boost)
target_include_directories(tee-tests-calib-boost PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-calib-boost PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-calib-boost PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-calib-boost DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "calibration.boost." PROPERTIES TIMEOUT 300)
