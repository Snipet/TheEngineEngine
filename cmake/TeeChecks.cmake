# cmake/TeeChecks.cmake: tee_check_targets(), called at the end of the root CMakeLists.txt: every target that compiles
# sources of ours must have had tee_target_defaults() (cmake/TeeFlags.cmake), directly or through a package's own
# helper. Without it a target builds on Linux and macOS and stops on Windows at the first M_PI (no _USE_MATH_DEFINES),
# or a min/max (no NOMINMAX), or builds without the warnings and -ffp-contract=off everywhere: the configure says so
# on every platform instead.
#
# A target that compiles a dependency's sources alone (with none of our flags on purpose) is named in
# TEE_TARGETS_WITHOUT_DEFAULTS below, with the reason.
include_guard(GLOBAL)

set(TEE_TARGETS_WITHOUT_DEFAULTS
  tee_ui_test_funkgui   # tests/ui/CMakeLists.txt: FunkGui's core compiled once, with FunkGui's flags, not ours
)

function(_tee_check_dir dir out)
  set(_bad "")
  get_property(_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_t IN LISTS _targets)
    get_target_property(_type ${_t} TYPE)
    if(NOT _type MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
      continue()
    endif()
    if(_t IN_LIST TEE_TARGETS_WITHOUT_DEFAULTS)
      continue()
    endif()
    get_target_property(_done ${_t} TEE_TARGET_DEFAULTS)
    if(NOT _done)
      file(RELATIVE_PATH _rel "${PROJECT_SOURCE_DIR}" "${dir}")
      list(APPEND _bad "${_t} (${_rel}/)")
    endif()
  endforeach()
  get_property(_subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
  foreach(_sub IN LISTS _subdirs)
    # ours only: a dependency's directory is outside the checkout (the dependency cache) or in the build tree
    cmake_path(IS_PREFIX PROJECT_SOURCE_DIR "${_sub}" NORMALIZE _inside)
    cmake_path(IS_PREFIX PROJECT_BINARY_DIR "${_sub}" NORMALIZE _built)
    cmake_path(IS_PREFIX TEE_DEPS_DIR "${_sub}" NORMALIZE _dep)
    if(_inside AND NOT _built AND NOT _dep)
      _tee_check_dir("${_sub}" _more)
      list(APPEND _bad ${_more})
    endif()
  endforeach()
  set(${out} "${_bad}" PARENT_SCOPE)
endfunction()

function(tee_check_targets)
  _tee_check_dir("${PROJECT_SOURCE_DIR}" _bad)
  if(_bad)
    list(JOIN _bad "\n  " _list)
    message(FATAL_ERROR "These targets compile our sources without tee_target_defaults() (cmake/TeeFlags.cmake): on "
                        "Windows they have no M_PI and no NOMINMAX, and nowhere our warnings or -ffp-contract=off. "
                        "Call it on each (or, for a target that compiles only a dependency's sources, name it in "
                        "cmake/TeeChecks.cmake):\n  ${_list}")
  endif()
endfunction()
