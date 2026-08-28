#include "../common/common_utils.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("=================================================================\n");
    printf("Senaryo C: eglTerminate(pDpyID) Initialize Edilmemis Parametre Kullanimi\n");
    printf("=================================================================\n");

    NativeWindow nw;
    init_native_window(&nw);
    EGLDisplay display = EGL_NO_DISPLAY;
    int exit_code = -1;

    if (!create_drm_window(&nw, 800, 600, "EGLTerminate - Senaryo C (Uninitialized Display)")) {
        goto cleanup;
    }

    // Gecerli bir EGL Display al, fakat senaryo geregi eglInitialize cagirma.
    display = get_egl_display_for_gbm(nw.gbm_device);
    if (display == EGL_NO_DISPLAY) {
        printf("Hata: EGL Display alinamadi: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }

    printf("Display gecerli alindi fakat INITIALIZE EDILMEDI.\n");
    printf("Simdi eglTerminate(display) cagriliyor...\n");

    EGLBoolean result = eglTerminate(display);
    if (result == EGL_TRUE) {
        printf("\n-> eglTerminate BASARILI (EGL_TRUE dondu).\n");
        printf("-> Not: EGL spesifikasyonuna gore baslatilmamis display uzerinde terminate cagirmak serbesttir ve etkisi yoktur.\n");
        exit_code = 0;
    } else {
        printf("\n-> Hata: eglTerminate basarisiz oldu. Hata Kodu: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }

    printf("\nBu display eglInitialize ile baslatilmadigi icin eglGetConfigs/eglChooseConfig, context, surface, cizim ve DRM present denenmeyecek.\n");
    printf("SONUC: EGL Context olusturulamaz ve ekrana yeni bir gorsel basilmamalidir.\n");

    sleep(3);

    cleanup:
        destroy_drm_window(&nw);
    return exit_code;
}
