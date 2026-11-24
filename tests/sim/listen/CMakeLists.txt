# tests/sim/listen/: package L's tests of the listener (sim/listen/): standalone on its rig (rig.fm) through the host's
# JIT, and in the whole simulator through tee::EnginePlayer. ctest -R '^listen\.' runs them alone.
if(TARGET tee-tests-listen)
  return()
endif()
include(Catch)
file(GLOB _tee_listen_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-listen ${_tee_listen_sources})
tee_target_defaults(tee-tests-listen)
target_include_directories(tee-tests-listen PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tee-tests-listen PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-listen PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-listen DISCOVERY_MODE PRE_TEST TEST_PREFIX "listen." PROPERTIES TIMEOUT 900 LABELS listen)
