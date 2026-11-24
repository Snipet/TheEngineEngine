# cmake/TeeShaderc.cmake: one shaderc run for an OPTIONAL panel shader (cmake/TeeShaders.cmake), by cmake -P.
#
# shaderc compiles SRC for one back end into OUT (a --bin2c header whose array is SYM). When it refuses, the build goes
# on: a warning names the panel, the shader and the back end with shaderc's own message, and OUT becomes a one-byte
# array with SYM_refused defined, which the generated table gives the size 0, so the kit leaves that program
# unregistered on that back end (and the application's --info names the effect that does not draw).
#
# -DSHADERC= -DSRC= -DOUT= -DSYM= -DPLATFORM= -DPROFILE= -DVARYING= -DINC1= -DINC2= -DWHAT=<panel/file>
foreach(_v SHADERC SRC OUT SYM PLATFORM PROFILE VARYING INC1 INC2 WHAT)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "TeeShaderc.cmake: -D${_v}= is missing")
  endif()
endforeach()
execute_process(
  COMMAND "${SHADERC}" -f "${SRC}" -o "${OUT}" --type fragment --platform ${PLATFORM} -p ${PROFILE}
          --varyingdef "${VARYING}" -i "${INC1}" -i "${INC2}" --bin2c ${SYM}
  RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(_rc EQUAL 0 AND EXISTS "${OUT}")
  return()
endif()
string(STRIP "${_out}\n${_err}" _said)
message(WARNING "shaderc refused the optional shader ${WHAT} for ${PROFILE} (${PLATFORM}): it is left out on that "
                "back end, and the build goes on. shaderc said:\n${_said}")
# a line CI's log can be searched for
message(STATUS "TEE_SHADER_REFUSED ${WHAT} ${PROFILE}")
file(WRITE "${OUT}" "// ${WHAT}: refused by shaderc for ${PROFILE} (cmake/TeeShaderc.cmake): left out on this back end\n"
                    "#define ${SYM}_refused 1\n"
                    "static const uint8_t ${SYM}[1] = { 0 };\n")
