#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: pDpyID - Yedek Ekran (Standby/EICAS) ---\n\n");

    NativeDisplayContext* native_ctx = init_native_display_at(1);
    if (!native_ctx) {
        printf("Hata: Native Display (DRM/GBM) acilamadi.\n");
        return 1;
    }

    EGLDisplay backup_display = eglGetDisplay((EGLNativeDisplayType)native_ctx->gbm_dev);
    if (backup_display == EGL_NO_DISPLAY || !eglInitialize(backup_display, NULL, NULL)) {
        log_egl_error("eglInitialize");
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE
    };
    EGLConfig backup_config;
    EGLint num_config = 0;
    if (!eglChooseConfig(backup_display, attribs, &backup_config, 1, &num_config) || num_config == 0) {
        log_egl_error("eglChooseConfig");
        eglTerminate(backup_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext backup_ctx = eglCreateContext(backup_display, backup_config, EGL_NO_CONTEXT, ctx_attribs);
    if (backup_ctx == EGL_NO_CONTEXT) {
        log_egl_error("eglCreateContext");
        eglTerminate(backup_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    struct gbm_surface *win = create_egl_compatible_gbm_surface(backup_display,
                                                                backup_config,
                                                                native_ctx->gbm_dev,
                                                                native_ctx->mode_info.hdisplay,
                                                                native_ctx->mode_info.vdisplay);
    if (!win) {
        eglDestroyContext(backup_display, backup_ctx);
        eglTerminate(backup_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLSurface surface = eglCreateWindowSurface(backup_display, backup_config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        eglDestroyContext(backup_display, backup_ctx);
        gbm_surface_destroy(win);
        eglTerminate(backup_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    if (!eglMakeCurrent(backup_display, surface, surface, backup_ctx)) {
        log_egl_error("eglMakeCurrent");
        eglDestroySurface(backup_display, surface);
        eglDestroyContext(backup_display, backup_ctx);
        gbm_surface_destroy(win);
        eglTerminate(backup_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    glViewport(0, 0, native_ctx->mode_info.hdisplay, native_ctx->mode_info.vdisplay);
    glClearColor(0.16f, 0.10f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    reset_common_gl_objects();
    draw_quad(-1.0f, 0.65f, 1.0f, 1.0f, 0.0f, 1.0f, 0.55f, 0.0f);
    draw_triangle(0.0f, 1.0f, 0.95f, 0.0f);

    printf("DEGER B: dpy = yedek ekran icin secilen EGLDisplay/native display yolu.\n");
    printf(" -> GORSEL SONUC: Yedek ekran senaryosu kahverengi zemin, turuncu ust bant ve sari ucgen cizer.\n");
    printf("    VM'de ikinci connector yoksa kod 0 numarali ekrana duser ama farkli desenle durumu somutlastirir.\n\n");

    eglSwapBuffers(backup_display, surface);
    present_native_display_for(native_ctx, win, 5);

    eglDestroySurface(backup_display, surface);
    eglDestroyContext(backup_display, backup_ctx);
    gbm_surface_destroy(win);
    eglTerminate(backup_display);
    destroy_native_display(native_ctx);
    return 0;
}
