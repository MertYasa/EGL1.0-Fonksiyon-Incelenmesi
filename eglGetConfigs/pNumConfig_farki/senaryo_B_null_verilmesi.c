#include "../common/common_utils.h"

int main(void) {
    printf("--- SENARYO B: pNumConfig - NULL Verilmesi ---\n\n");

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

    printf("BEKLENEN HATA: pNumConfig=NULL EGL 1.0 icin gecersiz parametredir.\n");
    printf("GUVENLI TEST: Bu ornek kasitli olarak eglGetConfigs(..., NULL) cagirmiyor.\n");
    printf("NEDEN: Bazi EGL suruculeri gecersiz output pointer'i icin EGL_FALSE yerine process crash uretebilir.\n");
    printf("DOGRU DAVRANIS: Uretim kodu pNumConfig NULL ise EGL cagrisindan once reddetmelidir.\n");
    printf("GORSEL SONUC: Config sayisi guvenli sekilde alinamadigi icin cizim kurulmaz.\n");

    eglTerminate(egl_dpy);
    destroy_native_display(native_ctx);
    return 0;
}
