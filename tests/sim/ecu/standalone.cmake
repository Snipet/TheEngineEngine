# tests/sim/ecu/standalone.cmake: card H's standalone tests (tests/sim/combustion/*.cpp, tests/sim/ecu/*.cpp) as their
# own executable, tee-tests-h, until tests/CMakeLists.txt globs tests/sim/ (a change asked of the lead in
# docs/handoffs/h-combustion-ecu.md). Nothing includes this file: a build tree that wants these tests names it at
# configure time, and it adds the executable once the root has made every target:
#
#   cmake --preset dev -DCMAKE_PROJECT_TheEngineEngine_INCLUDE=$PWD/tests/sim/ecu/standalone.cmake
#   cmake --build build-dev --target tee-tests-h && ctest --test-dir build-dev -R '^h\.'
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _tee_h_standalone_tests)

function(_tee_h_standalone_tests)
  if(NOT TEE_BUILD_TESTS OR TARGET tee-tests-h)
    return()
  endif()
  file(GLOB _h_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/sim/combustion/*.cpp"
                                         "${PROJECT_SOURCE_DIR}/tests/sim/ecu/*.cpp")
  add_executable(tee-tests-h ${_h_sources})
  tee_target_defaults(tee-tests-h)
  target_include_directories(tee-tests-h PRIVATE "${PROJECT_SOURCE_DIR}/tests")
  target_link_libraries(tee-tests-h PRIVATE Tee::host Catch2::Catch2WithMain)
  target_compile_definitions(tee-tests-h PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
  include(Catch)
  catch_discover_tests(tee-tests-h DISCOVERY_MODE PRE_TEST TEST_PREFIX "h." PROPERTIES TIMEOUT 900)
endfunction()
