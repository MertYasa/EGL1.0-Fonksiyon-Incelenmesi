#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: pDpyID - Kokpit Ana Ekrani (PFD) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    if (!x_dpy) {
        printf("Hata: X11 Display acilamadi.\n");
        return 1;
    }

    EGLDisplay main_display = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    eglInitialize(main_display, NULL, NULL);

    EGLint attribs[] = { EGL_NONE };
    EGLConfig main_config;
    EGLint num_config;
    eglChooseConfig(main_display, attribs, &main_config, 1, &num_config);

    EGLint ctx_attribs[] = { EGL_NONE };
    EGLContext main_ctx = eglCreateContext(main_display, main_config, EGL_NO_CONTEXT, ctx_attribs);

    if (main_ctx != EGL_NO_CONTEXT) {
        printf("DEGER A: Ana Ekran Display ID secildi.\n");
        printf(" -> SONUC: Baglam (Context) basariyla 1. ekrana (Ana Ekran) baglandi.\n");
        printf("    (Gorsel bir ciktiya gerek yok, eglCreateContext'in Display parametresi anlasildi)\n\n");
    } else {
        printf("Hata: Context olusturulamadi.\n");
    }

    eglDestroyContext(main_display, main_ctx);
    XCloseDisplay(x_dpy);
    return 0;
}
