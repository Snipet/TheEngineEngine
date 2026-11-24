# tests/sim/combustion/: card H's tests of sim/combustion/ (docs/plan/w1-h-combustion-ecu.md), run by ctest as tee-tests-combustion.
#
# They compile a test program of this directory (a .fm file) with every file of sim/ beside it through the host's
# tee::Program (tests/sim/ecu/FmHarness.h), and, integrated, the whole simulator through tee::EnginePlayer.
if(TARGET tee-tests-combustion)
  return()
endif()
include(Catch)
file(GLOB _tee_combustion_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-combustion ${_tee_combustion_test_sources})
tee_target_defaults(tee-tests-combustion)
target_include_directories(tee-tests-combustion PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-combustion PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-combustion PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\"" "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
# The light cases one a process; the whole-simulator cases ([integrated], tests/sim/ecu/Sim.h) in one process, one
# compile, the kept player reset between them (docs/plan/01-rules.md section 3); their full forms one entry of their own.
catch_discover_tests(tee-tests-combustion DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "~[integrated]"
                     TEST_PREFIX "combustion." PROPERTIES TIMEOUT 900)
add_test(NAME "combustion.integrated" COMMAND tee-tests-combustion "[integrated]~[nightly]" --durations yes)
add_test(NAME "combustion.integrated.nightly" COMMAND tee-tests-combustion "[integrated][nightly]" --durations yes)
set_tests_properties("combustion.integrated" PROPERTIES TIMEOUT 900 LABELS "combustion;integrated")
set_tests_properties("combustion.integrated.nightly" PROPERTIES TIMEOUT 3600 LABELS "combustion;integrated;nightly")
