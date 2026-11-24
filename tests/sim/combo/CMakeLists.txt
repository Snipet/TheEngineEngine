# tests/sim/combo/: package X's tests (the whole program under test), as tee-tests-combo; ctest -L combo runs them.
# Every .cpp here is a source. The fast tier's cases carry [fast] and run as one entry (below). The nightly tests (the
# triples, the fuzz, the soak) carry the Catch tag [nightly], which
# ADD_TAGS_AS_LABELS makes their ctest label: the dev and release presets leave them out; ctest --preset nightly runs
# them (tests/CMakeLists.txt).
# tools/tee-fuzz (package X's) is built here as well, from the same harness.
if(TARGET tee-tests-combo)
  return()
endif()
file(GLOB _tee_combo_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(REMOVE_ITEM _tee_combo_sources "${CMAKE_CURRENT_SOURCE_DIR}/Combo.cpp")
add_library(tee-combo-harness STATIC "${CMAKE_CURRENT_SOURCE_DIR}/Combo.cpp")
tee_target_defaults(tee-combo-harness)
target_include_directories(tee-combo-harness PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-combo-harness PUBLIC Tee::host Tee::test)

add_executable(tee-tests-combo ${_tee_combo_sources})
tee_target_defaults(tee-tests-combo)
target_include_directories(tee-tests-combo PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-combo PRIVATE tee-combo-harness Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_compile_definitions(tee-tests-combo PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
# The fast tier's cases run as four ctest entries, combo.fast-1 to combo.fast-4 (the Catch tags [fast-1] to [fast-4]),
# so the program compiles once a process (about 6.6 CPU-s each otherwise; 01-rules.md section 3) and each entry stays
# under section 3's 90 s for a grouped entry (the tier took 160 s on CI's slower Linux runner as one entry and 219 s on
# Windows; about 45 to 50 s each here now). Every run makes a fresh host from tee::test's cached Sim, so no state passes
# from one case to the next: shown by running them in reverse and in random order (the commit says so). Every [fast]
# case carries one of the four tags: combo.fast-tags fails if one does not. The rest (the nightly tier) are discovered
# one by one.
catch_discover_tests(tee-tests-combo DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "combo."
  TEST_SPEC "~[fast]" PROPERTIES TIMEOUT 7200)
foreach(_part 1 2 3 4)
  add_test(NAME combo.fast-${_part} COMMAND tee-tests-combo "[fast-${_part}]")
  set_tests_properties(combo.fast-${_part} PROPERTIES LABELS "combo;fast" TIMEOUT 900)
endforeach()
add_test(NAME combo.fast-tags COMMAND tee-tests-combo --list-tests "[fast]~[fast-1]~[fast-2]~[fast-3]~[fast-4]")
set_tests_properties(combo.fast-tags PROPERTIES LABELS "combo;fast" TIMEOUT 60
  PASS_REGULAR_EXPRESSION "[^0-9]0 matching test cases")

if(EXISTS "${PROJECT_SOURCE_DIR}/tools/tee-fuzz/main.cc")
  add_executable(tee-fuzz "${PROJECT_SOURCE_DIR}/tools/tee-fuzz/main.cc")
  tee_target_defaults(tee-fuzz)
  target_link_libraries(tee-fuzz PRIVATE tee-combo-harness nlohmann_json::nlohmann_json)
endif()
