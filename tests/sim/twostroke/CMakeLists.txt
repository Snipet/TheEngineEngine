# tests/sim/twostroke/: the two-stroke's tests (card T, docs/plan/w2-cards.md), run by ctest as tee-tests-twostroke.
# They compile sim/core/twostroke/*.fm with rig2.fm and the reference's pipes (read-only) through tee::Program.
if(TARGET tee-tests-twostroke)
  return()
endif()
include(Catch)
file(GLOB _tee_ts_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-twostroke ${_tee_ts_test_sources})
tee_target_defaults(tee-tests-twostroke)
target_include_directories(tee-tests-twostroke PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-twostroke PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-twostroke PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-twostroke DISCOVERY_MODE PRE_TEST TEST_PREFIX "twostroke: " PROPERTIES TIMEOUT 1800 LABELS twostroke)
