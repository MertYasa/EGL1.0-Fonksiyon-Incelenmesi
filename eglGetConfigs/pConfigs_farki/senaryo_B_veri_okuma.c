#include "../common/common_utils.h"

static int config_gorsel_icin_uygun(EGLDisplay dpy, EGLConfig config) {
    EGLint surface_type = 0;
    EGLint renderable_type = 0;
    return eglGetConfigAttrib(dpy, config, EGL_SURFACE_TYPE, &surface_type) &&
           eglGetConfigAttrib(dpy, config, EGL_RENDERABLE_TYPE, &renderable_type) &&
           (surface_type & EGL_WINDOW_BIT) &&
           (renderable_type & EGL_OPENGL_ES2_BIT);
}

int main(void) {
    printf("--- SENARYO B: pConfigs - Veri Okuma ---\n\n");

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

    EGLConfig configs[64];
    EGLint aktarilan_sayi = 0;
    if (!eglGetConfigs(egl_dpy, configs, 64, &aktarilan_sayi) || aktarilan_sayi <= 0) {
        log_egl_error("eglGetConfigs");
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLConfig secilen_config = NULL;
    for (int i = 0; i < aktarilan_sayi; i++) {
        if (config_gorsel_icin_uygun(egl_dpy, configs[i])) {
            secilen_config = configs[i];
            break;
        }
    }

    if (!secilen_config) {
        printf("Hata: Okunan configler icinde gorsel cikti icin uygun config yok.\n");
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        log_egl_error("eglBindAPI");
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    struct gbm_surface *win = create_egl_compatible_gbm_surface(egl_dpy,
                                                                secilen_config,
                                                                native_ctx->gbm_dev,
                                                                native_ctx->mode_info.hdisplay,
                                                                native_ctx->mode_info.vdisplay);
    if (!win) {
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLSurface surface = eglCreateWindowSurface(egl_dpy, secilen_config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        gbm_surface_destroy(win);
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(egl_dpy, secilen_config, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT) {
        log_egl_error("eglCreateContext");
        eglDestroySurface(egl_dpy, surface);
        gbm_surface_destroy(win);
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    if (!eglMakeCurrent(egl_dpy, surface, surface, ctx)) {
        log_egl_error("eglMakeCurrent");
        eglDestroyContext(egl_dpy, ctx);
        eglDestroySurface(egl_dpy, surface);
        gbm_surface_destroy(win);
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    glViewport(0, 0, native_ctx->mode_info.hdisplay, native_ctx->mode_info.vdisplay);
    glClearColor(0.08f, 0.10f, 0.28f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    reset_common_gl_objects();
    draw_triangle(0.0f, 1.0f, 0.9f, 0.0f);

    printf("BASARILI: pConfigs gecerli dizi oldugu icin %d config bellege kopyalandi.\n", aktarilan_sayi);
    printf("GORSEL SONUC: Okunan configlerden uygun olanla lacivert zemin uzerine sari ucgen cizildi.\n");

    if (!swap_and_present_native_display(egl_dpy, surface, native_ctx, win, 5)) {
        eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(egl_dpy, ctx);
        eglDestroySurface(egl_dpy, surface);
        gbm_surface_destroy(win);
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    eglMakeCurrent(egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl_dpy, ctx);
    eglDestroySurface(egl_dpy, surface);
    gbm_surface_destroy(win);
    eglTerminate(egl_dpy);
    destroy_native_display(native_ctx);
    return 0;
}
