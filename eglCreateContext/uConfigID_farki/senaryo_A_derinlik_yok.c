#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: uConfigID - Derinlik Tamponu Olmayan Config (EGL_DEPTH_SIZE = 0) ---\n\n");

    NativeDisplayContext* native_ctx = init_native_display();
    if (!native_ctx) {
        printf("Hata: Native Display (DRM/GBM) baslatilamadi.\n");
        return -1;
    }
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)native_ctx->gbm_dev);
    eglInitialize(display, NULL, NULL);

    // DİKKAT: EGL_DEPTH_SIZE 0 isteniyor! (Derinlik tamponu yok)
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_DEPTH_SIZE, 0,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(display, attribs, &config, 1, &num_config);

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT) {
        printf("Hata: EGL_NO_CONTEXT (Context olusturulamadi).\n");
        destroy_native_display(native_ctx);
        return -1;
    }

    struct gbm_surface *win = create_gbm_surface(native_ctx->gbm_dev, 400, 300);
    if (!win) {
        printf("Hata: GBM yuzeyi olusturulamadi.\n");
        eglDestroyContext(display, ctx);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        printf("Hata: EGL_NO_SURFACE (Yuzey olusturulamadi).\n");
        eglDestroyContext(display, ctx);
        destroy_native_display(native_ctx);
        return -1;
    }

    eglMakeCurrent(display, surface, surface, ctx);

    glEnable(GL_DEPTH_TEST); // Derinlik testi istiyoruz ama donanımda tampon YOK!
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Yakındaki uçağı (Kırmızı, Z=0.1) ÖNCE çiziyoruz
    draw_triangle(0.1f, 1.0f, 0.0f, 0.0f); // Uçak

    // Uzaktaki dağı (Mavi, Z=0.5) SONRA çiziyoruz
    draw_triangle(0.5f, 0.0f, 0.0f, 1.0f); // Dağ

    printf("DEGER A: EGL_DEPTH_SIZE = 0 Config kullanildi.\n");
    printf(" -> SONUC: Z-ekseni hesaplanamaz. \n");
    printf("    Uzaktaki dag (Mavi) sonra cizildigi icin yakindaki ucagin (Kirmizi) ustune biner!\n\n");

    eglSwapBuffers(display, surface);
    present_native_display(native_ctx, win);
    sleep(5);

    eglDestroySurface(display, surface);
    eglDestroyContext(display, ctx);
    destroy_native_display(native_ctx);
    return 0;
}
