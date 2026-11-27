# assistant/: Claude in the app (docs/plan/w3-a-assistant.md, card A). One static library, by what it needs:
#
#   tee_assistant   tee_host, tee_shell_files and nlohmann/json: the inbox between the assistant's worker and the frame
#                   (Inbox.h), the tool table both ways in share (Tools.h), the cards of what Claude changed (Cards.h)
#                   and the app's end of Claude Desktop's link (McpLink.h). No SDL, no FunkGui, no network.
#   tee-mcp         the MCP server Claude Desktop starts (mcp/Main.cpp): nlohmann/json and McpWire.h only, nothing of
#                   the host's. macOS and Linux.
#   TheEngineEngine.mcpb   the Claude Desktop extension: manifest.json and tee-mcp, zipped (target tee-mcpb).
#
# Its tests (tests/assistant/) are added here, so the root names this directory once.
add_library(tee_assistant STATIC
  src/Inbox.cpp
  src/Tools.cpp
  src/Cards.cpp
  src/McpLink.cpp
  src/Session.cpp
  src/Transport.cpp
  src/Chat.cpp
  src/KeyStore.cpp)
add_library(Tee::assistant ALIAS tee_assistant)
tee_target_defaults(tee_assistant)
target_include_directories(tee_assistant PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
find_package(Threads REQUIRED)
target_link_libraries(tee_assistant PUBLIC Tee::host Tee::shell_files nlohmann_json::nlohmann_json Threads::Threads)

# The key chat's HTTP (src/Transport.cpp): libcurl from the macOS SDK, from the system on Linux (the package
# libcurl4-openssl-dev). Where its headers are missing the build goes on without it: the chat then says so in words.
# Windows has none in this version.
option(TEE_ASSISTANT_CURL "The key chat's HTTP through libcurl, where its headers are found" ON)
# The key's store (src/KeyStore.cpp): the Keychain on macOS (the Security framework); the Secret Service on Linux through
# libsecret (the package libsecret-1-dev), where its headers are found; else none, and the key is refused, never kept in
# a file instead.
if(APPLE)
  target_link_libraries(tee_assistant PRIVATE "-framework Security" "-framework CoreFoundation")
elseif(NOT WIN32)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(TEE_LIBSECRET QUIET IMPORTED_TARGET libsecret-1)
  endif()
  if(TEE_LIBSECRET_FOUND)
    target_link_libraries(tee_assistant PRIVATE PkgConfig::TEE_LIBSECRET)
    target_compile_definitions(tee_assistant PRIVATE TEE_HAS_LIBSECRET=1)
    message(STATUS "TheEngineEngine: the assistant's key store is libsecret ${TEE_LIBSECRET_VERSION}")
  else()
    message(STATUS "TheEngineEngine: the assistant's key store is off (no libsecret headers: libsecret-1-dev on Linux)")
  endif()
endif()

if(TEE_ASSISTANT_CURL AND NOT WIN32)
  find_package(CURL QUIET)
  if(CURL_FOUND)
    target_link_libraries(tee_assistant PRIVATE CURL::libcurl)
    target_compile_definitions(tee_assistant PRIVATE TEE_HAS_CURL=1)
    message(STATUS "TheEngineEngine: the assistant's HTTP is libcurl ${CURL_VERSION_STRING}")
  else()
    message(STATUS "TheEngineEngine: the assistant's HTTP is off (no libcurl headers: libcurl4-openssl-dev on Linux)")
  endif()
endif()

if(NOT WIN32)
  add_executable(tee-mcp mcp/Main.cpp)
  tee_target_defaults(tee-mcp)
  target_include_directories(tee-mcp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(tee-mcp PRIVATE nlohmann_json::nlohmann_json)

  # The extension: Claude Desktop installs it with a double click and starts server/tee-mcp from it. Its version goes
  # up with every change to the bridge or its manifest (Claude Desktop updates an installed extension only to a newer
  # one). The manifest's shape is TestingPlayground's at 47bac40 (manifest_version 0.3).
  set(TEE_MCPB_VERSION "0.2.3")
  if(APPLE)
    set(_tee_mcpb_platform "darwin")
  else()
    set(_tee_mcpb_platform "linux")
  endif()
  set(_tee_mcpb "${CMAKE_BINARY_DIR}/mcpb")
  file(CONFIGURE OUTPUT "${_tee_mcpb}/manifest.json" @ONLY CONTENT [=[{
  "manifest_version": "0.3",
  "name": "theengineengine",
  "display_name": "TheEngineEngine",
  "version": "@TEE_MCPB_VERSION@",
  "description": "Ask Claude about the engine open in TheEngineEngine: it reads the build, changes it a step at a time and measures its torque curve.",
  "long_description": "Open TheEngineEngine and turn on 'Allow Claude Desktop' in it, then talk to Claude about the engine on the bench. Claude reads the build, explains its fields, changes them (each batch of changes one step of the app's own Undo; the app's status line counts them), loads a build from the library or saves the open one under a new name, and measures its torque curve on a private copy. Loading over unsaved changes asks you in the app first. Nothing leaves your computer except your conversation with Claude: the extension talks to the app through a private local connection.",
  "author": { "name": "TheEngineEngine" },
  "server": {
    "type": "binary",
    "entry_point": "server/tee-mcp",
    "mcp_config": { "command": "${__dirname}/server/tee-mcp", "args": [], "env": {} }
  },
  "tools": [
    { "name": "get_status", "description": "The open build and the engine's gauges" },
    { "name": "get_build", "description": "Read the build" },
    { "name": "set_fields", "description": "Change fields, one undo step" },
    { "name": "predict_curve", "description": "Predict the torque curve" }
  ],
  "tools_generated": true,
  "compatibility": { "platforms": ["@_tee_mcpb_platform@"] }
}
]=])
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/TheEngineEngine.mcpb"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_tee_mcpb}/server"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:tee-mcp> "${_tee_mcpb}/server/tee-mcp"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_BINARY_DIR}/TheEngineEngine.mcpb"
    COMMAND ${CMAKE_COMMAND} -E chdir "${_tee_mcpb}" ${CMAKE_COMMAND} -E tar cf "${CMAKE_BINARY_DIR}/TheEngineEngine.mcpb"
            --format=zip manifest.json server/tee-mcp
    DEPENDS tee-mcp "${_tee_mcpb}/manifest.json"
    VERBATIM)
  add_custom_target(tee-mcpb ALL DEPENDS "${CMAKE_BINARY_DIR}/TheEngineEngine.mcpb")
endif()

# The chat's panel (view/): one FunkGui item over ChatPort, on the interface kit (ui/kit/gui: Items, Scroll, Style),
# with TestingPlayground's chat markdown ported (view/SEED.tsv). Built where the kit is; the frame mounts it (U1's).
if(TARGET tee_ui_kit)
  add_library(tee_assistant_view STATIC view/ChatView.cpp view/ChatView.h view/Markdown.cpp view/Markdown.h)
  add_library(Tee::assistant_view ALIAS tee_assistant_view)
  tee_target_defaults(tee_assistant_view)
  target_include_directories(tee_assistant_view PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/view")
  target_link_libraries(tee_assistant_view PUBLIC Tee::assistant tee_ui_kit)
endif()

if(TEE_BUILD_TESTS)
  add_subdirectory("${PROJECT_SOURCE_DIR}/tests/assistant" "${CMAKE_CURRENT_BINARY_DIR}/tests")
endif()
