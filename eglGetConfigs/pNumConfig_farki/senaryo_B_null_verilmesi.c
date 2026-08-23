#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: pNumConfig'e KASITLI OLARAK NULL Verilmesi ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay egl_dpy = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    eglInitialize(egl_dpy, NULL, NULL);

    // EGL Standardına göre pNumConfig ASLA NULL olamaz!
    printf("eglGetConfigs(egl_dpy, NULL, 0, NULL) cagiriliyor...\n");
    EGLBoolean result = eglGetConfigs(egl_dpy, NULL, 0, NULL);

    if (result == EGL_FALSE) {
        EGLint error = eglGetError();
        printf("[BEKLENEN HATA] EGL islem yapmayi reddetti.\n");
        if (error == EGL_BAD_PARAMETER) {
            printf("Hata Kodu: EGL_BAD_PARAMETER (0x%x) - pNumConfig NULL olamaz!\n", error);
        } else {
            printf("Hata Kodu: 0x%x\n", error);
        }
    }

    eglTerminate(egl_dpy);
    XCloseDisplay(x_dpy);
    return 0;
}
