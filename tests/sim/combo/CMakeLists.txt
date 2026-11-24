# tests/sim/combo/: package X's tests (the whole program under test), as tee-tests-combo; ctest -L combo runs them.
# Every .cpp here is a source. The nightly tests (the triples, the long fuzz, the soak) are registered too, labelled
# nightly, and skip themselves unless TEE_NIGHTLY is set: TEE_NIGHTLY=1 ctest --preset dev -L nightly.
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
catch_discover_tests(tee-tests-combo DISCOVERY_MODE PRE_TEST TEST_PREFIX "combo."
  PROPERTIES TIMEOUT 1800 LABELS combo)
foreach(_n triples fuzz-long soak)
  add_test(NAME combo.nightly.${_n} COMMAND tee-tests-combo "[nightly][${_n}]")
  set_tests_properties(combo.nightly.${_n} PROPERTIES LABELS "combo;nightly" TIMEOUT 7200 SKIP_RETURN_CODE 4)
endforeach()

if(EXISTS "${PROJECT_SOURCE_DIR}/tools/tee-fuzz/main.cc")
  add_executable(tee-fuzz "${PROJECT_SOURCE_DIR}/tools/tee-fuzz/main.cc")
  tee_target_defaults(tee-fuzz)
  target_link_libraries(tee-fuzz PRIVATE tee-combo-harness nlohmann_json::nlohmann_json)
endif()
