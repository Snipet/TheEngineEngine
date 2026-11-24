# cmake/TeeFlags.cmake: tee_target_defaults(<target>), what every target of TheEngineEngine's own gets
# (docs/plan/01-rules.md, section 4), and never a dependency:
#
#   C++20 without extensions;
#   -Wall -Wextra -Wshadow -Wpedantic, as errors unless TEE_WERROR=OFF;
#   -ffp-contract=off: a multiply and an add are fused nowhere, so the host's arithmetic is the same on arm64 and
#   x86-64 (as FMinus's own code is compiled); never -ffast-math.
#
# clang-cl (Windows) takes the same through its own spellings.
include_guard(GLOBAL)

option(TEE_WERROR "Treat warnings in TheEngineEngine's own sources as errors" ON)

set(TEE_MSVC_FRONTEND OFF)
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  set(TEE_MSVC_FRONTEND ON)
endif()

function(tee_target_defaults target)
  get_target_property(_type ${target} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY")
    target_compile_features(${target} INTERFACE cxx_std_20)
    return()
  endif()
  target_compile_features(${target} PUBLIC cxx_std_20)
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
  if(TEE_MSVC_FRONTEND)
    # clang-cl reads -Wall as cl's /Wall, which is clang's -Weverything; its /W4 is clang's -Wall -Wextra.
    target_compile_options(${target} PRIVATE /W4 -Wshadow -Wpedantic /clang:-ffp-contract=off
                                             $<$<BOOL:${TEE_WERROR}>:/WX>)
    # The Windows C runtime's deprecations of standard C (fopen, getenv): warnings, then errors.
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wshadow -Wpedantic -ffp-contract=off
                                             $<$<BOOL:${TEE_WERROR}>:-Werror>)
  endif()
endfunction()
