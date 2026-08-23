#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: pNumConfig NULL Verilmesi (Gorsel Sonuclu) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay egl_dpy = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    EGLint major, minor;
    eglInitialize(egl_dpy, &major, &minor);

    EGLConfig dizi[10];

    // EGL 1.0 Standartlarina gore pNumConfig parametresi ASLA NULL OLAMAZ!
    // Kasten NULL gonderiyoruz.
    EGLBoolean basari = eglGetConfigs(egl_dpy, dizi, 10, NULL);
    EGLint hata_kodu = eglGetError();

    if (basari == EGL_FALSE) {
        printf("=================================================================\n");
        printf("HATA: pNumConfig parametresine NULL verdik!\n");
        printf("eglGetConfigs fonksiyonu kural geregi coktu ve EGL_FALSE dondu.\n");
        printf("Hata Kodu: 0x%X (EGL_BAD_PARAMETER)\n", hata_kodu);
        printf("Gorsel Sonuc: Config dizisi dolmadigi icin hicbir pencere,\n");
        printf("EGL surface veya cizim alani OLUSTURULAMAZ!\n");
        printf("=================================================================\n\n");
    }

    printf("Ekrana cizim yapilamadi. Program sonlandiriliyor.\n");

    eglTerminate(egl_dpy);
    XCloseDisplay(x_dpy);
    return -1;
}
