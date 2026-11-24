# tests/sim/ecu/: card H's tests of sim/ecu/ (docs/plan/w1-h-combustion-ecu.md), run by ctest as tee-tests-ecu.
#
# They compile a test program of this directory (a .fm file) with every file of sim/ beside it through the host's
# tee::Program (tests/sim/ecu/FmHarness.h), and, integrated, the whole simulator through tee::EnginePlayer.
if(TARGET tee-tests-ecu)
  return()
endif()
include(Catch)
file(GLOB _tee_ecu_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-ecu ${_tee_ecu_test_sources})
tee_target_defaults(tee-tests-ecu)
target_include_directories(tee-tests-ecu PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-ecu PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-ecu PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\"" "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\"")
# The light cases one a process; the whole-simulator cases ([integrated], tests/sim/ecu/Sim.h) in one process an entry,
# one compile, the kept player reset between them (docs/plan/01-rules.md section 3): traction control's apart (the two
# together pass 90 s on a slow runner); their full forms ([nightly]) one entry of their own.
catch_discover_tests(tee-tests-ecu DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "~[integrated]" TEST_PREFIX "ecu."
                     PROPERTIES TIMEOUT 900)
add_test(NAME "ecu.integrated" COMMAND tee-tests-ecu "[integrated]~[traction]~[nightly]" --durations yes)
add_test(NAME "ecu.integrated.traction" COMMAND tee-tests-ecu "[integrated][traction]~[nightly]" --durations yes)
add_test(NAME "ecu.integrated.nightly" COMMAND tee-tests-ecu "[integrated][nightly]" --durations yes)
set_tests_properties("ecu.integrated" "ecu.integrated.traction" PROPERTIES TIMEOUT 900 LABELS "ecu;integrated")
set_tests_properties("ecu.integrated.nightly" PROPERTIES TIMEOUT 3600 LABELS "ecu;integrated;nightly")
