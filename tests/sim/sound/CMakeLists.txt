# tests/sim/sound/: card M's tests (the mechanical voices, sim/sound/), as tee-tests-sound; ctest -R '^sound\.' runs
# them. Every .cpp here is a source (adding one edits no CMake).
if(TARGET tee-tests-sound OR NOT TARGET Tee::host)
  return()
endif()
include(Catch)
file(GLOB _tee_sound_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-sound ${_tee_sound_sources})
tee_target_defaults(tee-tests-sound)
target_include_directories(tee-tests-sound PRIVATE "${PROJECT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(tee-tests-sound PRIVATE Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-sound PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
catch_discover_tests(tee-tests-sound DISCOVERY_MODE PRE_TEST TEST_PREFIX "sound." PROPERTIES TIMEOUT 900)
