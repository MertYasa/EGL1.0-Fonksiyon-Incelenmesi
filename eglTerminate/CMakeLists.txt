cmake_minimum_required(VERSION 3.10)
project(EGL_Terminate_Tutorial)

set(CMAKE_C_STANDARD 99)

# Include directories
include_directories(common)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(GBM REQUIRED gbm)

# DRM, GBM, EGL and GLESv2 libraries
set(LIBS ${DRM_LIBRARIES} ${GBM_LIBRARIES} EGL GLESv2)
include_directories(${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS})
link_directories(${DRM_LIBRARY_DIRS} ${GBM_LIBRARY_DIRS})

# Common utilities
add_library(common_utils common/common_utils.c)

# Scenario A: Valid pDpyID
add_executable(ScenarioA_ValidDisplay pDpyID/ScenarioA_ValidDisplay.c)
target_link_libraries(ScenarioA_ValidDisplay common_utils ${LIBS})

# Scenario B: Invalid pDpyID (EGL_NO_DISPLAY)
add_executable(ScenarioB_InvalidDisplay pDpyID/ScenarioB_InvalidDisplay.c)
target_link_libraries(ScenarioB_InvalidDisplay common_utils ${LIBS})

# Scenario C: Uninitialized pDpyID
add_executable(ScenarioC_UninitializedDisplay pDpyID/ScenarioC_UninitializedDisplay.c)
target_link_libraries(ScenarioC_UninitializedDisplay common_utils ${LIBS})
