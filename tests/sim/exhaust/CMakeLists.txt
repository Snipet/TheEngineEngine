# tests/sim/exhaust/: card G's tests of the exhaust builder (sim/exhaust/), on its own harness (harness.fm, with the
# reference's exhaust copied alone in refexhaust.fm) through the host's JIT. ctest -R '^exhaust\.' runs them alone.
if(TARGET tee-tests-exhaust)
  return()
endif()
file(GLOB _tee_exhaust_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-exhaust ${_tee_exhaust_sources})
tee_target_defaults(tee-tests-exhaust)
target_include_directories(tee-tests-exhaust PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-exhaust PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-exhaust PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-tests-exhaust DISCOVERY_MODE PRE_TEST TEST_PREFIX "exhaust." PROPERTIES TIMEOUT 900)
