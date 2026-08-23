cmake_minimum_required(VERSION 3.10)
project(eglGetCurrentContext_tutorial)

set(CMAKE_C_STANDARD 99)

# Find necessary libraries
find_package(X11 REQUIRED)

# Common utility library
add_library(common_utils common/common_utils.c)
target_include_directories(common_utils PUBLIC common)
target_link_libraries(common_utils ${X11_LIBRARIES} EGL GLESv2)

# Scenario A: Valid Context
add_executable(scenario_a void_param/scenario_a.c)
target_link_libraries(scenario_a common_utils)

# Scenario B: No Context
add_executable(scenario_b void_param/scenario_b.c)
target_link_libraries(scenario_b common_utils)
