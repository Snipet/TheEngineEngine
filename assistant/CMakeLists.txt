# assistant/: Claude in the app (docs/plan/w3-a-assistant.md, card A). One static library, by what it needs:
#
#   tee_assistant   tee_host, tee_shell_files and nlohmann/json: the inbox between the assistant's worker and the frame
#                   (Inbox.h), and the tool table both ways in share (Tools.h). No SDL, no FunkGui, no network.
#
# Its tests (tests/assistant/) are added here, so the root names this directory once.
add_library(tee_assistant STATIC
  src/Inbox.cpp
  src/Tools.cpp)
add_library(Tee::assistant ALIAS tee_assistant)
tee_target_defaults(tee_assistant)
target_include_directories(tee_assistant PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
find_package(Threads REQUIRED)
target_link_libraries(tee_assistant PUBLIC Tee::host Tee::shell_files nlohmann_json::nlohmann_json Threads::Threads)

if(TEE_BUILD_TESTS)
  add_subdirectory("${PROJECT_SOURCE_DIR}/tests/assistant" "${CMAKE_CURRENT_BINARY_DIR}/tests")
endif()
