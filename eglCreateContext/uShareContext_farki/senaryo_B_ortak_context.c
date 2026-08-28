#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: uShareContext - Ortak Context (Paylasimli) ---\n\n");

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

    struct gbm_surface *win = create_egl_compatible_gbm_surface(display,
                                                                config,
                                                                native_ctx->gbm_dev,
                                                                native_ctx->mode_info.hdisplay,
                                                                native_ctx->mode_info.vdisplay);
    if (!win) {
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        if (win) gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext main_ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (main_ctx == EGL_NO_CONTEXT) {
        log_egl_error("eglCreateContext(main)");
        eglDestroySurface(display, surface);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLContext shared_ctx = eglCreateContext(display, config, main_ctx, ctx_attribs);
    if (shared_ctx == EGL_NO_CONTEXT) {
        log_egl_error("eglCreateContext(shared)");
        eglDestroyContext(display, main_ctx);
        eglDestroySurface(display, surface);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    if (!eglMakeCurrent(display, surface, surface, main_ctx)) {
        log_egl_error("eglMakeCurrent(main_ctx)");
        eglDestroyContext(display, shared_ctx);
        eglDestroyContext(display, main_ctx);
        eglDestroySurface(display, surface);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }
    GLuint texture = create_checkerboard_texture();
    glFinish();
    printf("[VRAM] 1. baglamda sari-siyah doku yaratildi. Texture ID: %u\n", texture);

    if (!eglMakeCurrent(display, surface, surface, shared_ctx)) {
        log_egl_error("eglMakeCurrent(shared_ctx)");
        eglDestroyContext(display, shared_ctx);
        eglDestroyContext(display, main_ctx);
        eglDestroySurface(display, surface);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }
    reset_common_gl_objects();
    GLboolean is_valid = glIsTexture(texture);

    glViewport(0, 0, native_ctx->mode_info.hdisplay, native_ctx->mode_info.vdisplay);
    glClearColor(0.02f, 0.16f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (is_valid) {
        draw_textured_quad(texture);
    } else {
        draw_quad(-0.9f, -0.9f, 0.9f, 0.9f, 0.0f, 0.9f, 0.0f, 0.0f);
    }

    printf("\nDEGER B: share_context = main_ctx\n");
    printf(" -> SONUC: 2. baglam 1. baglamin dokusunu %s.\n", is_valid ? "TANIYOR" : "TANIMIYOR");
    printf(" -> GORSEL SONUC: Paylasim calisinca 1. context'te uretilen sari-siyah doku 2. context'te gorunur.\n\n");

    eglSwapBuffers(display, surface);
    present_native_display_for(native_ctx, win, 5);

    eglDestroyContext(display, shared_ctx);
    eglDestroyContext(display, main_ctx);
    eglDestroySurface(display, surface);
    gbm_surface_destroy(win);
    eglTerminate(display);
    destroy_native_display(native_ctx);
    return 0;
}
