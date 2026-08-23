#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: Gecersiz Display (Gorsel Sonuclu) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);

    // BILEREK GECERSIZ DISPLAY VERIYORUZ (EGL_NO_DISPLAY = 0)
    EGLDisplay hatali_dpy = EGL_NO_DISPLAY;

    EGLint aktarilan_sayi = 0;
    EGLConfig dizi[10];

    // Gecersiz display ile cagirildiginda ne olacak?
    EGLBoolean basari = eglGetConfigs(hatali_dpy, dizi, 10, &aktarilan_sayi);
    EGLint hata_kodu = eglGetError();

    if (basari == EGL_FALSE) {
        printf("=================================================================\n");
        printf("HATA: EGL_NO_DISPLAY (Gecersiz Display) verildigi icin \n");
        printf("eglGetConfigs EGL_FALSE dondu! Hata Kodu: 0x%X (EGL_BAD_DISPLAY)\n", hata_kodu);
        printf("Gorsel Sonuc: Config listesi okunamadigi icin EGLContext ve \n");
        printf("EGLSurface OLUSTURULAMAZ. Ekrana HICBIR SEY CIZILEMEZ!\n");
        printf("=================================================================\n\n");
    }

    printf("Ekrana cizim yapilamadi. Program sonlandiriliyor.\n");

    XCloseDisplay(x_dpy);
    return -1;
}
