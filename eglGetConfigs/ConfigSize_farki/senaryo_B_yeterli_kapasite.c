#include "../common/common_utils.h"

static int config_derinlikli_gorsel_icin_uygun(EGLDisplay dpy, EGLConfig config) {
    EGLint surface_type = 0;
    EGLint renderable_type = 0;
    EGLint depth = 0;
    return eglGetConfigAttrib(dpy, config, EGL_SURFACE_TYPE, &surface_type) &&
           eglGetConfigAttrib(dpy, config, EGL_RENDERABLE_TYPE, &renderable_type) &&
           eglGetConfigAttrib(dpy, config, EGL_DEPTH_SIZE, &depth) &&
           (surface_type & EGL_WINDOW_BIT) &&
           (renderable_type & EGL_OPENGL_ES2_BIT) &&
           depth >= 16;
}

int main(void) {
    printf("--- SENARYO B: ConfigSize - Yeterli Kapasite ---\n\n");

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

    EGLint toplam_config = 0;
    if (!eglGetConfigs(egl_dpy, NULL, 0, &toplam_config)) {
        log_egl_error("eglGetConfigs sayim");
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    if (toplam_config <= 0) {
        printf("Sistemde toplam %d config var.\n", toplam_config);
        printf("GORSEL SONUC: Config olmadigi icin yeterli kapasite verilse bile context/surface kurulamaz.\n");
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 0;
    }

    EGLConfig *configs = (EGLConfig*)malloc((size_t)toplam_config * sizeof(EGLConfig));
    if (!configs) {
        printf("Hata: Config listesi icin bellek ayrilamadi.\n");
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLint aktarilan_sayi = 0;
    if (!eglGetConfigs(egl_dpy, configs, toplam_config, &aktarilan_sayi)) {
        log_egl_error("eglGetConfigs tam okuma");
        free(configs);
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 1;
    }

    if (aktarilan_sayi <= 0) {
        printf("Hata: Toplam config sayisi %d gorunmesine ragmen hic config kopyalanmadi.\n", toplam_config);
        free(configs);
        eglTerminate(egl_dpy);
        destroy_native_display(native_ctx);
        return 0;
    }

    EGLConfig secilen_config = NULL;
    for (int i = 0; i < aktarilan_sayi; i++) {
        if (config_derinlikli_gorsel_icin_uygun(egl_dpy, configs[i])) {
            secilen_config = configs[i];
            break;
        }
    }
    free(configs);

    if (!secilen_config) {
        printf("Hata: Tam config havuzunda EGL_WINDOW_BIT + EGL_OPENGL_ES2_BIT + depth destekli config bulunamadi.\n");
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
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.16f, 0.16f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    reset_common_gl_objects();
    draw_triangle(0.5f, 1.0f, 0.0f, 0.0f);
    draw_triangle(0.0f, 0.0f, 1.0f, 0.0f);

    printf("BASARILI: Yeterli kapasite ile %d/%d config okundu ve derinlikli uygun config secildi.\n",
           aktarilan_sayi,
           toplam_config);
    printf("GORSEL SONUC: Depth buffer aktif; yesil ucgen onde, kirmizi ucgen arkada kalir.\n");

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
