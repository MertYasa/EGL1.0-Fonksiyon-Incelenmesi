#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: uConfigID - Derinlik Tamponu Olan Config (EGL_DEPTH_SIZE = 16) ---\n\n");

    NativeDisplayContext* native_ctx = init_native_display();
    if (!native_ctx) {
        printf("Hata: Native Display (DRM/GBM) baslatilamadi.\n");
        return -1;
    }

    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)native_ctx->gbm_dev);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, NULL, NULL)) {
        log_egl_error("eglInitialize");
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_config = 0;
    if (!eglChooseConfig(display, attribs, &config, 1, &num_config) || num_config == 0) {
        log_egl_error("eglChooseConfig");
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT) {
        log_egl_error("eglCreateContext");
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    struct gbm_surface *win = create_egl_compatible_gbm_surface(display,
                                                                config,
                                                                native_ctx->gbm_dev,
                                                                native_ctx->mode_info.hdisplay,
                                                                native_ctx->mode_info.vdisplay);
    if (!win) {
        eglDestroyContext(display, ctx);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        eglDestroyContext(display, ctx);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    if (!eglMakeCurrent(display, surface, surface, ctx)) {
        log_egl_error("eglMakeCurrent");
        eglDestroySurface(display, surface);
        eglDestroyContext(display, ctx);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    glViewport(0, 0, native_ctx->mode_info.hdisplay, native_ctx->mode_info.vdisplay);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    reset_common_gl_objects();

    draw_triangle(0.1f, 1.0f, 0.0f, 0.0f);
    draw_triangle(0.5f, 0.0f, 0.0f, 1.0f);

    printf("DEGER B: config = EGL_DEPTH_SIZE 16.\n");
    printf(" -> GORSEL SONUC: Derinlik tamponu var. Kirmizi ucgen onde kalir, mavi ucgen arkada elenir.\n");
    printf("    Cizim sirasi ayni olmasina ragmen z bilgisi sonucu degistirir.\n\n");

    eglSwapBuffers(display, surface);
    present_native_display_for(native_ctx, win, 5);

    eglDestroySurface(display, surface);
    eglDestroyContext(display, ctx);
    gbm_surface_destroy(win);
    eglTerminate(display);
    destroy_native_display(native_ctx);
    return 0;
}
