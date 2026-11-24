# tests/sim/calibration/balance/: card N3's air and energy balance (tee-tests-calibration-balance). The probe
# ([.balance], hidden) prints an engine's books for scripts/balance.py; the tests hold what the balance fixed.
if(TARGET tee-tests-calibration-balance)
  return()
endif()
include(Catch)
file(GLOB _tee_n3_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-calibration-balance ${_tee_n3_sources})
tee_target_defaults(tee-tests-calibration-balance)
target_include_directories(tee-tests-calibration-balance PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-calibration-balance PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-calibration-balance PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-calibration-balance DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS
                     TEST_PREFIX "calibration-balance." PROPERTIES TIMEOUT 900)
