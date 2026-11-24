# tests/sim/intake/: card E's tests of sim/intake/ (docs/plan/w1-e-valvetrain-intake.md), run by ctest as tee-tests-intake.
# Every .cpp here is a source; tests/sim/valvetrain/Lab.h is what both of card E's test directories share.
if(TARGET tee-tests-intake)
  return()
endif()
include(Catch)
file(GLOB _tee_intake_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-intake ${_tee_intake_test_sources})
tee_target_defaults(tee-tests-intake)
target_include_directories(tee-tests-intake PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-intake PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-intake PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-tests-intake DISCOVERY_MODE PRE_TEST TEST_PREFIX "intake." PROPERTIES TIMEOUT 900)
