#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: pAttribList - EGL 1.0 Standart Kullanimi ---\n\n");

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

    EGLint egl10_attribs[] = { EGL_NONE };
    EGLContext ctx_egl10 = eglCreateContext(display, config, EGL_NO_CONTEXT, egl10_attribs);
    if (ctx_egl10 == EGL_NO_CONTEXT) {
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
        eglDestroyContext(display, ctx_egl10);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        eglDestroyContext(display, ctx_egl10);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    if (!eglMakeCurrent(display, surface, surface, ctx_egl10)) {
        log_egl_error("eglMakeCurrent");
        eglDestroySurface(display, surface);
        eglDestroyContext(display, ctx_egl10);
        gbm_surface_destroy(win);
        eglTerminate(display);
        destroy_native_display(native_ctx);
        return -1;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    glViewport(0, 0, native_ctx->mode_info.hdisplay, native_ctx->mode_info.vdisplay);
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    printf("DEGER A: pAttribList = { EGL_NONE }\n");
    printf(" -> SONUC: EGL 1.0 standart bicimde ek client-version istegi vermeden context olusturur.\n");
    printf(" -> GORSEL SONUC: Duz gri ekran. Shader/modern pipeline talebi yapilmadi.\n");
    printf("    Aktif OpenGL ES versiyonu: %s\n\n", version ? (const char*)version : "Bilinmiyor");

    eglSwapBuffers(display, surface);
    present_native_display_for(native_ctx, win, 5);

    eglDestroySurface(display, surface);
    eglDestroyContext(display, ctx_egl10);
    gbm_surface_destroy(win);
    eglTerminate(display);
    destroy_native_display(native_ctx);
    return 0;
}
