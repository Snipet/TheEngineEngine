# app/Window.cmake: TheEngineEngine, the application in a window (TEE_BUILD_APP; included by app/CMakeLists.txt).
# FunkGui's desktop host over SDL3's window and bgfx, the shell's audio device and input, and the same Panel and
# offline renderer tee-headless has.
#
# What it needs at run time goes with it: sim/ and content/ beside the executable (in a macOS bundle: its Resources),
# and the licences of what it holds (FunkGui's font and bgfx, by funkgui_add_font; the rest by the packaging step of
# .github/workflows/build.yml). On macOS it is an application bundle, signed ad hoc at the end of its build with the
# entitlement FMinus's JIT needs under the hardened runtime (macos/TheEngineEngine.entitlements).
add_executable(TheEngineEngine MACOSX_BUNDLE "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
                                             "${CMAKE_CURRENT_LIST_DIR}/WindowCommands.cpp")
target_link_libraries(TheEngineEngine PRIVATE FunkGui::core FunkGui::gpu FunkGui::desktop Tee::shell_sdl
                                              Tee::shell_settings)
tee_app_gui(TheEngineEngine)
funkgui_compile_shaders(TheEngineEngine)
funkgui_add_font(TheEngineEngine)
add_dependencies(TheEngineEngine tee_app_data)

if(APPLE)
  set_target_properties(TheEngineEngine PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "TheEngineEngine"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.snipet.theengineengine"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_STRING "TheEngineEngine ${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "GPL-3.0-only")
  # The simulator's source and the content into the bundle, then the signature over all of it (a file added after
  # the signature would break its seal). A POST_BUILD step runs only when the executable is linked, so the link
  # depends on every file of sim/ and content/: an edited .fm or build file links again, and the copy and the
  # signature follow it. Without this the bundle kept the engine of its last link (tee_app_data's copy beside the
  # executables is not the one a bundle reads). A file deleted from sim/ or content/ stays in an old bundle: the
  # copy only adds and replaces.
  file(GLOB_RECURSE _tee_bundle_data CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/sim/*" "${PROJECT_SOURCE_DIR}/content/*")
  set_property(TARGET TheEngineEngine APPEND PROPERTY LINK_DEPENDS ${_tee_bundle_data})
  add_custom_command(TARGET TheEngineEngine POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/sim"
            "$<TARGET_BUNDLE_CONTENT_DIR:TheEngineEngine>/Resources/sim"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/content"
            "$<TARGET_BUNDLE_CONTENT_DIR:TheEngineEngine>/Resources/content"
    COMMAND codesign --force --sign - --entitlements "${CMAKE_CURRENT_LIST_DIR}/macos/TheEngineEngine.entitlements"
            "$<TARGET_BUNDLE_DIR:TheEngineEngine>"
    COMMENT "sim/ and content/ into TheEngineEngine.app, and its ad-hoc signature"
    VERBATIM)
endif()
