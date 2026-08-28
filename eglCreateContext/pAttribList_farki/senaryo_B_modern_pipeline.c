#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: pAttribList - Modern Pipeline (EGL_CONTEXT_CLIENT_VERSION) ---\n\n");

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

    EGLint modern_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx_modern = eglCreateContext(display, config, EGL_NO_CONTEXT, modern_attribs);
    if (ctx_modern == EGL_NO_CONTEXT) {
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
        eglDestroyContext(display, ctx_modern);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        eglDestroyContext(display, ctx_modern);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    if (!eglMakeCurrent(display, surface, surface, ctx_modern)) {
        log_egl_error("eglMakeCurrent");
        eglDestroySurface(display, surface);
        eglDestroyContext(display, ctx_modern);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    glViewport(0, 0, native_ctx->mode_info.hdisplay, native_ctx->mode_info.vdisplay);
    glClearColor(0.03f, 0.03f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    reset_common_gl_objects();
    draw_quad(-1.0f, -1.0f, 1.0f, -0.72f, 0.0f, 0.0f, 0.35f, 1.0f);
    draw_triangle(0.0f, 0.95f, 0.15f, 0.85f);

    printf("DEGER B: pAttribList = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE }\n");
    printf(" -> SONUC: GLES2/SC2.0 icin programmable shader pipeline talep edildi.\n");
    printf(" -> GORSEL SONUC: Koyu zemin uzerinde mavi bant ve shader ile cizilen mor ucgen gorulur.\n");
    printf("    Aktif OpenGL ES versiyonu: %s\n\n", version ? (const char*)version : "Bilinmiyor");

    eglSwapBuffers(display, surface);
    present_native_display_for(native_ctx, win, 5);

    eglDestroySurface(display, surface);
    eglDestroyContext(display, ctx_modern);
    gbm_surface_destroy(win);
    eglTerminate(display);
    destroy_native_display(native_ctx);
    return 0;
}
