# tests/sim/valvetrain/standalone.cmake: card E's standalone tests (tests/sim/valvetrain/*.cpp, tests/sim/intake/*.cpp)
# as their own executable, tee-tests-e, until tests/CMakeLists.txt globs tests/sim/ (a change asked of the lead in
# docs/handoffs/e-valvetrain-intake.md). Nothing includes this file: a build tree that wants these tests names it at
# configure time, and it adds the executable once the root has made every target:
#
#   cmake --preset dev -DCMAKE_PROJECT_TheEngineEngine_INCLUDE=$PWD/tests/sim/valvetrain/standalone.cmake
#   cmake --build build-dev --target tee-tests-e && ctest --test-dir build-dev -R '^e\.'
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _tee_e_standalone_tests)

function(_tee_e_standalone_tests)
  if(NOT TEE_BUILD_TESTS OR TARGET tee-tests-e)
    return()
  endif()
  file(GLOB _e_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/sim/valvetrain/*.cpp"
                                         "${PROJECT_SOURCE_DIR}/tests/sim/intake/*.cpp")
  add_executable(tee-tests-e ${_e_sources})
  tee_target_defaults(tee-tests-e)
  target_include_directories(tee-tests-e PRIVATE "${PROJECT_SOURCE_DIR}/tests")
  target_link_libraries(tee-tests-e PRIVATE Tee::host Catch2::Catch2WithMain)
  target_compile_definitions(tee-tests-e PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
  include(Catch)
  catch_discover_tests(tee-tests-e DISCOVERY_MODE PRE_TEST TEST_PREFIX "e." PROPERTIES TIMEOUT 900)
endfunction()
