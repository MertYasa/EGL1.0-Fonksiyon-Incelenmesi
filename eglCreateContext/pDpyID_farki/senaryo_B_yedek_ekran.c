#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: pDpyID - Yedek Ekran (Standby/EICAS) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    if (!x_dpy) {
        printf("Hata: X11 Display acilamadi.\n");
        return 1;
    }

    EGLDisplay backup_display = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    eglInitialize(backup_display, NULL, NULL);

    EGLint attribs[] = { EGL_NONE };
    EGLConfig backup_config;
    EGLint num_config;
    eglChooseConfig(backup_display, attribs, &backup_config, 1, &num_config);

    EGLint ctx_attribs[] = { EGL_NONE };
    EGLContext backup_ctx = eglCreateContext(backup_display, backup_config, EGL_NO_CONTEXT, ctx_attribs);

    if (backup_ctx != EGL_NO_CONTEXT) {
        printf("DEGER B: Yedek Ekran Display ID secildi.\n");
        printf(" -> SONUC: Baglam (Context) basariyla 2. ekrana (Yedek Ekran) baglandi.\n");
        printf("    (Gorsel bir ciktiya gerek yok, eglCreateContext'in Display parametresi anlasildi)\n\n");
    } else {
        printf("Hata: Context olusturulamadi.\n");
    }

    eglDestroyContext(backup_display, backup_ctx);
    XCloseDisplay(x_dpy);
    return 0;
}
