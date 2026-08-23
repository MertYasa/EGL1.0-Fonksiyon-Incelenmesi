#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: Sadece Sayim Yapma (Gorsel Sonuclu) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay egl_dpy = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    EGLint major, minor;
    eglInitialize(egl_dpy, &major, &minor);

    EGLint toplam_sayi = 0;

    // Config dizisi yerine NULL veriyoruz (Sadece saymak icin)
    EGLBoolean basari = eglGetConfigs(egl_dpy, NULL, 0, &toplam_sayi);

    printf("=================================================================\n");
    printf("DURUM: eglGetConfigs basariyla (EGL_TRUE) calisti.\n");
    printf("Sistemde %d adet config oldugu sayildi.\n", toplam_sayi);
    printf("Ancak parametre olarak NULL (dizi yok) verdigimiz icin elimizde \n");
    printf("EGLContext olusturacak HICBIR CONFIG HANDLE'I YOK.\n");
    printf("Gorsel Sonuc: Config Handle'i olmadan OpenGL ES baglami acilamaz.\n");
    printf("Ekrana cizim YAPILAMAZ! Bu kullanim sadece bilgi almak icindir.\n");
    printf("=================================================================\n\n");

    printf("Ekrana cizim yapilamadi. Program sonlandiriliyor.\n");

    eglTerminate(egl_dpy);
    XCloseDisplay(x_dpy);
    return 0;
}
