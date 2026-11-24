# tests/sim/calibration/boost/: card N2's (the boosted engines). tee-calib-boost: the measuring harness (not a test):
# a build on the engine dyno at steps, with the telemetry's boosted lines and the Scope's port pressures averaged.
if(TARGET tee-calib-boost)
  return()
endif()
add_executable(tee-calib-boost "${CMAKE_CURRENT_SOURCE_DIR}/harness/main.cpp")
tee_target_defaults(tee-calib-boost)
target_include_directories(tee-calib-boost PRIVATE "${PROJECT_SOURCE_DIR}/tools")
target_link_libraries(tee-calib-boost PRIVATE Tee::host)
target_compile_definitions(tee-calib-boost PRIVATE "TEE_SOURCE_SIM_DIR=\"${PROJECT_SOURCE_DIR}/sim/ref\"")
