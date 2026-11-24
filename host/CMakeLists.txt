# host/: tee_host, the C++ that compiles and plays the simulator. It is written on FMinus's public header alone
# (fminus/FMinus.h): no LLVM header, none of FMinus's internals. Every .cpp under src/ is a source (adding a file edits
# no CMake); include/tee/ is what the tools, the tests and later the shell include.
file(GLOB_RECURSE _tee_host_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
add_library(tee_host STATIC ${_tee_host_sources})
add_library(Tee::host ALIAS tee_host)
tee_target_defaults(tee_host)
target_include_directories(tee_host PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tee_host PUBLIC FMinus::FMinus PRIVATE nlohmann_json::nlohmann_json)
add_subdirectory(test)   # card C: Tee::test, the scenario and measures library the tests use (host/test/)
