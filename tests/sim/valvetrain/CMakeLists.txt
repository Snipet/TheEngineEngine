# tests/sim/valvetrain/: card E's tests of sim/valvetrain/ (docs/plan/w1-e-valvetrain-intake.md), run by ctest as tee-tests-valvetrain.
# Every .cpp here is a source; tests/sim/valvetrain/Lab.h is what both of card E's test directories share.
if(TARGET tee-tests-valvetrain)
  return()
endif()
include(Catch)
file(GLOB _tee_valvetrain_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-valvetrain ${_tee_valvetrain_test_sources})
tee_target_defaults(tee-tests-valvetrain)
target_include_directories(tee-tests-valvetrain PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-valvetrain PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-valvetrain PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
  "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
catch_discover_tests(tee-tests-valvetrain DISCOVERY_MODE PRE_TEST TEST_PREFIX "valvetrain." PROPERTIES TIMEOUT 900)
