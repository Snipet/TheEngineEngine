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
# The light cases one a process; the whole-simulator cases ([integrated], Engine.h's Rig) a few to a process, one compile
# each, the kept player reset between them (docs/plan/01-rules.md section 3: an entry under 90 s on a slow runner); their
# full forms ([nightly]) one entry of their own.
catch_discover_tests(tee-tests-valvetrain DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "~[integrated]"
                     TEST_PREFIX "valvetrain." PROPERTIES TIMEOUT 900)
add_test(NAME "valvetrain.integrated-1" COMMAND tee-tests-valvetrain "[part-1]~[nightly]" --durations yes)
add_test(NAME "valvetrain.integrated-2" COMMAND tee-tests-valvetrain "[part-2]~[nightly]" --durations yes)
add_test(NAME "valvetrain.integrated-3" COMMAND tee-tests-valvetrain "[part-3]~[nightly]" --durations yes)
add_test(NAME "valvetrain.integrated.nightly" COMMAND tee-tests-valvetrain "[integrated][nightly]" --durations yes)
set_tests_properties("valvetrain.integrated-1" "valvetrain.integrated-2" "valvetrain.integrated-3" PROPERTIES TIMEOUT 900
                     LABELS "valvetrain;integrated")
set_tests_properties("valvetrain.integrated.nightly" PROPERTIES TIMEOUT 3600 LABELS "valvetrain;integrated;nightly")
# every fast whole-simulator case is in one of the three entries above: this lists those that are not, and fails on any
add_test(NAME "valvetrain.integrated-all" COMMAND "${CMAKE_COMMAND}" -DEXE=$<TARGET_FILE:tee-tests-valvetrain>
         -P "${CMAKE_CURRENT_SOURCE_DIR}/AllGrouped.cmake")
set_tests_properties("valvetrain.integrated-all" PROPERTIES TIMEOUT 60 LABELS "valvetrain")
