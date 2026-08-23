cmake_minimum_required(VERSION 3.10)
project(EGL1_0_Fonksiyon_Incelemesi C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Gerekli kütüphaneleri bul (Ubuntu'da libx11-dev, libegl1-mesa-dev, libgles2-mesa-dev paketleri gereklidir)
find_package(X11 REQUIRED)

# Ortak kütüphane
add_library(common_utils common/common_utils.c common/common_utils.h)
target_include_directories(common_utils PUBLIC common)
target_include_directories(common_utils PUBLIC ${X11_INCLUDE_DIR})

# Tüm senaryolara bağlanacak ortak link kütüphaneleri
set(COMMON_LIBS common_utils ${X11_LIBRARIES} EGL GLESv2)

# 1. Parametre: pDpyID
add_executable(senaryo_A_ana_ekran pDpyID_farki/senaryo_A_ana_ekran.c)
target_link_libraries(senaryo_A_ana_ekran ${COMMON_LIBS})

add_executable(senaryo_B_yedek_ekran pDpyID_farki/senaryo_B_yedek_ekran.c)
target_link_libraries(senaryo_B_yedek_ekran ${COMMON_LIBS})

# 2. Parametre: uConfigID
add_executable(senaryo_A_derinlik_yok uConfigID_farki/senaryo_A_derinlik_yok.c)
target_link_libraries(senaryo_A_derinlik_yok ${COMMON_LIBS})

add_executable(senaryo_B_derinlik_var uConfigID_farki/senaryo_B_derinlik_var.c)
target_link_libraries(senaryo_B_derinlik_var ${COMMON_LIBS})

# 3. Parametre: uShareContext
add_executable(senaryo_A_paylasim_yok uShareContext_farki/senaryo_A_paylasim_yok.c)
target_link_libraries(senaryo_A_paylasim_yok ${COMMON_LIBS})

add_executable(senaryo_B_ortak_context uShareContext_farki/senaryo_B_ortak_context.c)
target_link_libraries(senaryo_B_ortak_context ${COMMON_LIBS})

# 4. Parametre: pAttribList
add_executable(senaryo_A_egl10_standart pAttribList_farki/senaryo_A_egl10_standart.c)
target_link_libraries(senaryo_A_egl10_standart ${COMMON_LIBS})

add_executable(senaryo_B_modern_pipeline pAttribList_farki/senaryo_B_modern_pipeline.c)
target_link_libraries(senaryo_B_modern_pipeline ${COMMON_LIBS})
