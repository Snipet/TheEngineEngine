# tests/sim/calibration/na/: card N1's tests and probes (the engines that breathe by themselves), run by ctest as
# tee-tests-calibration-na. The probe ([.probe], hidden: run by hand) prints a cylinder's gas exchange on the engine
# dyno; the tests hold what the calibration fixed.
if(TARGET tee-tests-calibration-na)
  return()
endif()
include(Catch)
file(GLOB _tee_n1_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-calibration-na ${_tee_n1_sources})
tee_target_defaults(tee-tests-calibration-na)
target_include_directories(tee-tests-calibration-na PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-calibration-na PRIVATE Tee::host Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_compile_definitions(tee-tests-calibration-na PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-calibration-na DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "calibration-na."
                     PROPERTIES TIMEOUT 900)
