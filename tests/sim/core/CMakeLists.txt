# tests/sim/core/: the mechanics' tests (docs/plan/w1-d-mechanics-layouts.md), run by ctest as tee-tests-core.
#
# They compile sim/core/*.fm with rig.fm (a bench of the mechanics' own) through the host's tee::Program, and read the
# layout presets of content/builds/engines/. Every .cpp here is a source.
if(TARGET tee-tests-core)
  return()
endif()
include(Catch)
file(GLOB _tee_core_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-core ${_tee_core_test_sources})
tee_target_defaults(tee-tests-core)
target_include_directories(tee-tests-core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-core PRIVATE Tee::host Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_compile_definitions(tee-tests-core PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-core DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "core: " PROPERTIES TIMEOUT 900 LABELS core)
