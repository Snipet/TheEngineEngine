# tests/sim/calibration/: card N's (N1 na/, N2 boost/). Reached by tests/CMakeLists.txt's glob of tests/sim/*/; each
# subdirectory with a CMakeLists.txt is added here (adding one edits no CMake).
file(GLOB _tee_calibration_lists CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")
foreach(_list IN LISTS _tee_calibration_lists)
  cmake_path(GET _list PARENT_PATH _dir)
  add_subdirectory("${_dir}")
endforeach()
