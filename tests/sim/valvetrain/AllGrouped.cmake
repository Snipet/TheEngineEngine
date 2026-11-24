# tests/sim/valvetrain/AllGrouped.cmake: fails if a fast whole-simulator case ([integrated]~[nightly]) is in none of
# CMakeLists.txt's three grouped entries (tags [part-1] to [part-3]) (a new case would otherwise run nowhere). cmake -DEXE=<tee-tests-valvetrain> -P
execute_process(COMMAND "${EXE}" "[integrated]~[nightly]" --list-tests --verbosity quiet OUTPUT_VARIABLE all RESULT_VARIABLE rc1)
execute_process(COMMAND "${EXE}" "[part-1]~[nightly],[part-2]~[nightly],[part-3]~[nightly]"
                --list-tests --verbosity quiet OUTPUT_VARIABLE grouped RESULT_VARIABLE rc2)
if(NOT rc1 EQUAL 0 OR NOT rc2 EQUAL 0 OR all STREQUAL "")
  message(FATAL_ERROR "valvetrain: listing the cases failed (${rc1}, ${rc2}):\n${all}${grouped}")
endif()
# the two listings as sorted lists (a name's own ";" kept out of CMake's list separator)
foreach(v all grouped)
  string(REPLACE ";" "<semicolon>" ${v} "${${v}}")
  string(REPLACE "\n" ";" ${v} "${${v}}")
  list(SORT ${v})
endforeach()
if(NOT all STREQUAL grouped)
  message(FATAL_ERROR "valvetrain: a fast [integrated] case is in no grouped entry (tests/sim/valvetrain/CMakeLists.txt):\n"
                      "all:\n${all}\ngrouped:\n${grouped}")
endif()
