#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: Gecerli Display (Gorsel Sonuclu) ---\n\n");

    AppState state;
    init_drm_and_gbm(&state, 400, 400);
    // GECERLI (DOGRU) EGL DISPLAY ALINIYOR
    EGLDisplay egl_dpy = eglGetDisplay((EGLNativeDisplayType)state.gbm_device);

    EGLint major, minor;
    eglInitialize(egl_dpy, &major, &minor);

    EGLint aktarilan_sayi = 0;
    EGLConfig dizi[10];

    // Gecerli display verildigi icin fonksiyon BASARILI olacak
    EGLBoolean basari = eglGetConfigs(egl_dpy, dizi, 10, &aktarilan_sayi);

    if (basari == EGL_TRUE && aktarilan_sayi > 0) {
        printf("=================================================================\n");
        printf("BASARILI: Gecerli EGLDisplay verildigi icin configler okunabildi.\n");
        printf("Gorsel Sonuc: Pencere basariyla acilacak ve cizim yapilacaktir.\n");
        printf("=================================================================\n\n");

        EGLConfig secilen_config = dizi[0];
        eglBindAPI(EGL_OPENGL_ES_API);
        EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

        EGLSurface surf = eglCreateWindowSurface(egl_dpy, secilen_config, (EGLNativeWindowType)state.gbm_surface, NULL);
        EGLContext ctx = eglCreateContext(egl_dpy, secilen_config, EGL_NO_CONTEXT, ctx_attribs);

        eglMakeCurrent(egl_dpy, surf, surf, ctx);

        printf("Pencere acildi. Cikmak icin terminalde Ctrl+C yapin.\n");

        while(1) {
            glClearColor(0.2f, 0.5f, 0.2f, 1.0f); // Yesil arka plan
            glClear(GL_COLOR_BUFFER_BIT);
            draw_triangle(0.0f, 1.0f, 1.0f, 1.0f); // Beyaz ucgen
            eglSwapBuffers(egl_dpy, surf);
            usleep(16000);
        }
    } else {
        printf("HATA: Bilinmeyen bir sorun olustu.\n");
    }

    cleanup_drm_and_gbm(&state);
    return 0;
}
