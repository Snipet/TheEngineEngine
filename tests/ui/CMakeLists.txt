# tests/ui/CMakeLists.txt: the interface kit's headless tests (card J, item 5). One program, tee-ui-tests, a test a
# name; each is a CTest test "ui.<name>". Pictures (PNGs from FunkGui's software rasteriser) go under out/ui/ in the
# repository's root, which git ignores; goldens are the text files under tests/ui/golden/, written only by
#
#     tee-ui-tests <name> --bless        (or: tee-ui-tests all --bless)
#
# and reviewed from the PNGs.
# FunkGui's core compiled once, apart from our code: its sources come with FunkGui::core (an INTERFACE library), and
# our warnings-as-errors are for our files, not for a dependency's.
add_library(tee_ui_test_funkgui STATIC)
target_link_libraries(tee_ui_test_funkgui PRIVATE FunkGui::core)
if(TARGET FunkGui::desktop)
  target_link_libraries(tee_ui_test_funkgui PRIVATE FunkGui::desktop)
endif()
funkgui_configure_product(tee_ui_test_funkgui PRODUCT TheEngineEngine OBJC_PREFIX TeeUiTests ENV_PREFIX TEE_
                          PREFS_FOLDER TheEngineEngine)
funkgui_compile_shaders(tee_ui_test_funkgui)
target_link_libraries(tee_ui_test_funkgui PUBLIC tee_ui_funkgui_headers
                      $<TARGET_PROPERTY:FunkGuiCore,INTERFACE_LINK_LIBRARIES>)

add_executable(tee-ui-tests Tests.cpp MockPlayer.h MockSpec.h RefTests.inc WidgetTests.inc BindTests.inc AppTests.inc)
target_link_libraries(tee-ui-tests PRIVATE tee_ui_app tee_ui_ref tee_ui_kit tee_ui_test_funkgui)
tee_ui_warnings(tee-ui-tests)
target_compile_definitions(tee-ui-tests PRIVATE
  "TEE_UI_TEST_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\""
  "TEE_UI_REPO_DIR=\"${TEE_UI_REPO}\""
  "TEE_UI_OUT_DIR=\"${TEE_UI_REPO}/out/ui\"")

# The test program's own list of names is the truth (tee-ui-tests --list); this list must match it, which the test
# ui.names checks.
set(TEE_UI_TESTS
  names seed gui.parse gui.notes ports
  kinds.golden kinds.small kinds.large
  knob.drag knob.wheel knob.keys knob.reset slider.drag switch.click buttons.click menu.pick xy.drag pointer.area
  tabs.switch resize.layout scroll.wheel display.telemetry keyboard.hidden a11y
  shader.text shader.placeholder
  ref.strip ref.fallback ref.panel
  # J2
  units button.click toggle.click stepper.click textfield.type list.select instruments plot.zoom curve.drag map.edit
  widgets.gallery
  fieldport fieldparams page.generated gui.auto gui.widgets page.split
  app.frame app.set app.start app.garage app.resize app.sheet)
foreach(_t IN LISTS TEE_UI_TESTS)
  add_test(NAME ui.${_t} COMMAND tee-ui-tests ${_t})
endforeach()
string(REPLACE ";" "," _tee_ui_names "${TEE_UI_TESTS}")
target_compile_definitions(tee-ui-tests PRIVATE "TEE_UI_TEST_NAMES=\"${_tee_ui_names}\"")
set_tests_properties(ui.ref.panel PROPERTIES SKIP_RETURN_CODE 77)

# J2: the binding over card C's tee::Build with the real Spec (sim/), where the host is (the root's build).
if(TARGET tee_ui_build)
  add_executable(tee-ui-build-tests BuildTests.cpp)
  target_link_libraries(tee-ui-build-tests PRIVATE tee_ui_build tee_ui_test_funkgui)
  tee_ui_warnings(tee-ui-build-tests)
  target_compile_definitions(tee-ui-build-tests PRIVATE
    "TEE_UI_SIM_DIR=\"${TEE_UI_REPO}/sim\""
    "TEE_UI_BUILDS_DIR=\"${TEE_UI_REPO}/content/builds\""
    "TEE_UI_OUT_DIR=\"${TEE_UI_REPO}/out/ui\"")
  foreach(_t build.frame build.set build.files)
    add_test(NAME ui.${_t} COMMAND tee-ui-build-tests ${_t})
  endforeach()
endif()
