cmake_minimum_required(VERSION 3.10)
project(eglGetCurrentContext_tutorial)

set(CMAKE_C_STANDARD 99)

# Find necessary libraries using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(GBM REQUIRED gbm)

# Common utility library
add_library(common_utils common/common_utils.c)
target_include_directories(common_utils PUBLIC common ${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS})
target_link_libraries(common_utils ${DRM_LIBRARIES} ${GBM_LIBRARIES} EGL GLESv2)

# Scenario A: Valid Context
add_executable(scenario_a void_param/scenario_a.c)
target_link_libraries(scenario_a common_utils)

# Scenario B: No Context
add_executable(scenario_b void_param/scenario_b.c)
target_link_libraries(scenario_b common_utils)
