# host/test/: tee_test (Tee::test), the test library every simulator package's tests are written with
# (include/tee/test/Scenario.h; card C, section 7). It finds the simulator and the content in this checkout unless
# told otherwise (TEE_SIM_DIR, TEE_CONTENT_DIR, or setSimDirectory and setContentDirectory). Every .cpp here is a source.
file(GLOB _tee_test_lib_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(tee_test STATIC ${_tee_test_lib_sources})
add_library(Tee::test ALIAS tee_test)
tee_target_defaults(tee_test)
target_link_libraries(tee_test PUBLIC Tee::host)
target_compile_definitions(tee_test PRIVATE "TEE_TEST_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
