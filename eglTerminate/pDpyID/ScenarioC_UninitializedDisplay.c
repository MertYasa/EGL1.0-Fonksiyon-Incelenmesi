#include "../common/common_utils.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("=================================================================\n");
    printf("Senaryo C: eglTerminate(pDpyID) Initialize Edilmemis Parametre Kullanimi\n");
    printf("=================================================================\n");

    NativeWindow nw;
    if (!create_drm_window(&nw, 800, 600, "EGLTerminate - Senaryo C (Uninitialized Display)")) {
        return -1;
    }

    // Gecerli bir EGL Display al
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)nw.gbm_device);
    if (display == EGL_NO_DISPLAY) {
        printf("Hata: EGL Display alinamadi.\n");
        return -1;
    }

    // DİKKAT: eglInitialize CAGIRMIYORUZ! (Senaryo Geregi)

    printf("Display gecerli alindi fakat INITIALIZE EDILMEDI.\n");
    printf("Simdi eglTerminate cagriliyor...\n");

    EGLBoolean result = eglTerminate(display);

    if (result == EGL_TRUE) {
        printf("\n-> eglTerminate BASARILI (EGL_TRUE dondu).\n");
        printf("-> Not: EGL spesifikasyonuna gore, baslatilmamis (uninitialized) bir display uzerinde terminate cagirmak serbesttir ve etkisi yoktur.\n");
    } else {
        printf("\n-> Hata: eglTerminate basarisiz oldu. Hata Kodu: %s\n", get_egl_error_str(eglGetError()));
    }

    // Cizim yapmaya calisalim
    printf("\nSimdi bu display ile EGL Context olusturmaya ve cizim yapmaya calisiyoruz...\n");

    EGLint attribList[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE };
    EGLConfig config;
    EGLint numConfigs;

    EGLBoolean chooseRes = eglChooseConfig(display, attribList, &config, 1, &numConfigs);
    if (!chooseRes) {
        EGLint err = eglGetError();
        printf("\n-> eglChooseConfig BASARISIZ! Hata: %s\n", get_egl_error_str(err));

        if (err == EGL_NOT_INITIALIZED) {
            printf("\nSONUC:\n");
            printf("Bu parametreyi (pDpyID) eglTerminate'e basariyla vermis olsak da, en basta eglInitialize ile baslatilmadigi icin (NOT_INITIALIZED) EGL Context oluşturulamadı ve ekrana HİÇBİR ŞEY ÇİZİLEMEDİ.\n");
            printf("Gorsel Kanit: Ekranda sadece bos DRM/KMS ekrani kaldi (3 saniye sonra kapanacak).\n");
        }
    }

    // Bos pencereyi gormek icin bekle
    sleep(3);

    destroy_drm_window(&nw);
    return 0;
}
