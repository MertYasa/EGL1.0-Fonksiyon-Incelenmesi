#include "../common/common_utils.h"

int main(void) {
    printf("--- SENARYO A: pConfigs - Sadece Sayim Yapma ---\n\n");

    NativeDisplayContext* native_ctx = init_native_display();
    if (!native_ctx) {
        printf("Hata: Native Display (DRM/GBM) acilamadi.\n");
        return 1;
    }

    EGLDisplay egl_dpy = get_egl_display_for_gbm(native_ctx->gbm_dev);
    if (egl_dpy == EGL_NO_DISPLAY || !eglInitialize(egl_dpy, NULL, NULL)) {
        log_egl_error("eglInitialize");
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLint toplam_sayi = 0;
    if (!eglGetConfigs(egl_dpy, NULL, 0, &toplam_sayi)) {
        log_egl_error("eglGetConfigs");
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    printf("BASARILI: pConfigs=NULL ve config_size=0 ile sadece config sayisi sorgulandi.\n");
    printf("Sistemde %d adet EGLConfig var.\n", toplam_sayi);
    printf("GORSEL SONUC: Bu senaryo bilerek cizim yapmaz; elde config handle olmadigi icin surface/context kurulmaz.\n");

    eglTerminate(egl_dpy);
    destroy_native_display(native_ctx);
    return 0;
}
