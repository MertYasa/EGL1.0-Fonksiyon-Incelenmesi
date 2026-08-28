#include "../common/common_utils.h"

int main(void) {
    printf("--- SENARYO B: pDpyID - Gecersiz Display ---\n\n");

    EGLDisplay hatali_dpy = EGL_NO_DISPLAY;
    EGLint aktarilan_sayi = 0;
    EGLConfig configs[10];

    EGLBoolean basari = eglGetConfigs(hatali_dpy, configs, 10, &aktarilan_sayi);

    if (basari == EGL_FALSE) {
        EGLint hata_kodu = eglGetError();
        printf("BEKLENEN HATA: EGL_NO_DISPLAY ile eglGetConfigs basarisiz oldu.\n");
        printf("EGL hata kodu: %s (0x%04x)\n", egl_error_name(hata_kodu), hata_kodu);
        printf("GORSEL SONUC: Config alinmadigi icin context/surface olusturulmaz ve cizim yapilmaz.\n");
        return 0;
    }

    printf("Hata: Gecersiz display beklenmedik sekilde kabul edildi. Aktarilan config sayisi: %d\n", aktarilan_sayi);
    return 1;
}
