# tests/sim/calibration/: card N's tests, one directory a session (na/: N1, boost/: N2), each with its own
# CMakeLists.txt by the rule in tests/CMakeLists.txt. This file only adds them.
file(GLOB _tee_calibration_lists CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")
list(SORT _tee_calibration_lists)
foreach(_list IN LISTS _tee_calibration_lists)
  cmake_path(GET _list PARENT_PATH _dir)
  add_subdirectory("${_dir}")
endforeach()
