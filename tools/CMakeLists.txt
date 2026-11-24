# tools/: one executable a directory (tools/<name>/*.cpp is the target <name>; adding a tool edits no CMake), each on
# tee_host. tools/common/ holds what they share, headers only.
#
# A tool finds the simulator's files and the content by itself: --sim and --content, else TEE_SIM_DIR and
# TEE_CONTENT_DIR, else this checkout's (compiled in below) while it is there, else sim/ref and content/ref beside the
# executable (an unpacked CI artefact).
file(GLOB _tee_tool_dirs LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(_dir IN LISTS _tee_tool_dirs)
  cmake_path(GET _dir FILENAME _name)
  if(NOT IS_DIRECTORY "${_dir}" OR _name STREQUAL "common")
    continue()
  endif()
  file(GLOB _sources CONFIGURE_DEPENDS "${_dir}/*.cpp")
  if(NOT _sources)
    continue()
  endif()
  add_executable(${_name} ${_sources})
  tee_target_defaults(${_name})
  target_include_directories(${_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(${_name} PRIVATE Tee::host)
  target_compile_definitions(${_name} PRIVATE
    "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\""
    "TEE_SOURCE_CONTENT_DIR=\"${PROJECT_SOURCE_DIR}/content/ref\""
    "TEE_VERSION=\"${PROJECT_VERSION}\"")
endforeach()
