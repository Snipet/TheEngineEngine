# tests/assistant/: card A's tests (docs/plan/w3-a-detailed.md, "Proof"), added from assistant/CMakeLists.txt.
#
#   tee-tests-assistant   Catch2. The inbox, the names and the drivers alone (discovered, a process each: no simulator);
#                         the tools on a real simulator, document and library, on a simulated clock of 60 frames a
#                         second as tee-headless runs (tagged [sharedsim]: one ctest entry, one process, one compile,
#                         tests/shell/support/SharedSim.h).
if(TARGET tee-tests-assistant)
  return()
endif()
include(Catch)
file(GLOB _tee_assistant_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(tee-tests-assistant ${_tee_assistant_tests})
tee_target_defaults(tee-tests-assistant)
target_include_directories(tee-tests-assistant PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tests/shell")
target_link_libraries(tee-tests-assistant PRIVATE Tee::assistant Tee::shell_ref Tee::shell_files Tee::host Catch2::Catch2WithMain)
target_compile_definitions(tee-tests-assistant PRIVATE
  "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\""
  "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim\"")
if(TARGET tee-mcp)   # (macOS and Linux) the bridge, run by McpTest.cpp as Claude Desktop runs it
  target_compile_definitions(tee-tests-assistant PRIVATE "TEE_MCP_EXE=\"$<TARGET_FILE:tee-mcp>\""
                             "TEE_MCPB_MANIFEST=\"${CMAKE_BINARY_DIR}/mcpb/manifest.json\"")
  add_dependencies(tee-tests-assistant tee-mcp)
endif()
if(TARGET CURL::libcurl AND NOT WIN32)   # the real transport, tested against a loopback server
  target_compile_definitions(tee-tests-assistant PRIVATE TEE_TEST_CURL=1)
endif()
add_test(NAME assistant.sim COMMAND tee-tests-assistant "[sharedsim]~[nightly]" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(assistant.sim PROPERTIES TIMEOUT 600 LABELS assistant)
catch_discover_tests(tee-tests-assistant DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_SPEC "~[sharedsim]"
  TEST_PREFIX "assistant." PROPERTIES TIMEOUT 120)

# The chat's panel, drawn by FunkGui's software rasteriser (pictures under out/assistant/), its own executable: it needs
# FunkGui's sources (tee_ui_test_funkgui, tests/ui), which the rest does not.
if(TARGET tee_assistant_view AND TARGET tee_ui_test_funkgui)
  add_executable(tee-tests-assistant-view view/ChatViewTest.cpp)
  tee_target_defaults(tee-tests-assistant-view)
  target_link_libraries(tee-tests-assistant-view PRIVATE Tee::assistant_view tee_ui_test_funkgui Catch2::Catch2WithMain)
  target_compile_definitions(tee-tests-assistant-view PRIVATE "TEE_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")
  catch_discover_tests(tee-tests-assistant-view DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS TEST_PREFIX "assistant.view."
    PROPERTIES TIMEOUT 120)
endif()
