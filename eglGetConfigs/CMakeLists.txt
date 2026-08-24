cmake_minimum_required(VERSION 3.10)
project(EGL1_0_GetConfigs_Incelemesi C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(GBM REQUIRED gbm)

# Ortak kütüphane
add_library(common_utils common/common_utils.c common/common_utils.h)
target_include_directories(common_utils PUBLIC common ${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS})

set(COMMON_LIBS common_utils ${DRM_LIBRARIES} ${GBM_LIBRARIES} EGL GLESv2)

# 1. Parametre: pDpyID
add_executable(senaryo_A_gecerli_display pDpyID_farki/senaryo_A_gecerli_display.c)
target_link_libraries(senaryo_A_gecerli_display ${COMMON_LIBS})

add_executable(senaryo_B_gecersiz_display pDpyID_farki/senaryo_B_gecersiz_display.c)
target_link_libraries(senaryo_B_gecersiz_display ${COMMON_LIBS})

# 2. Parametre: pConfigs
add_executable(senaryo_A_sadece_sayim pConfigs_farki/senaryo_A_sadece_sayim.c)
target_link_libraries(senaryo_A_sadece_sayim ${COMMON_LIBS})

add_executable(senaryo_B_veri_okuma pConfigs_farki/senaryo_B_veri_okuma.c)
target_link_libraries(senaryo_B_veri_okuma ${COMMON_LIBS})

# 3. Parametre: ConfigSize
add_executable(senaryo_A_yetersiz_kapasite ConfigSize_farki/senaryo_A_yetersiz_kapasite.c)
target_link_libraries(senaryo_A_yetersiz_kapasite ${COMMON_LIBS})

add_executable(senaryo_B_yeterli_kapasite ConfigSize_farki/senaryo_B_yeterli_kapasite.c)
target_link_libraries(senaryo_B_yeterli_kapasite ${COMMON_LIBS})

# 4. Parametre: pNumConfig
add_executable(senaryo_A_gecerli_isaretci pNumConfig_farki/senaryo_A_gecerli_isaretci.c)
target_link_libraries(senaryo_A_gecerli_isaretci ${COMMON_LIBS})

add_executable(senaryo_B_null_verilmesi pNumConfig_farki/senaryo_B_null_verilmesi.c)
target_link_libraries(senaryo_B_null_verilmesi ${COMMON_LIBS})

# 5. Harici Standart Senaryo
add_executable(profesyonel_2_adimli_sorgu harici_standart_senaryo/profesyonel_2_adimli_sorgu.c)
target_link_libraries(profesyonel_2_adimli_sorgu ${COMMON_LIBS})
