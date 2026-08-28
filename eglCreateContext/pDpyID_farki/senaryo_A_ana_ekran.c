#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: pDpyID - Kokpit Ana Ekrani (PFD) ---\n\n");

    NativeDisplayContext* native_ctx = init_native_display_at(0);
    if (!native_ctx) {
        printf("Hata: Native Display (DRM/GBM) acilamadi.\n");
        return 1;
    }

    EGLDisplay main_display = eglGetDisplay((EGLNativeDisplayType)native_ctx->gbm_dev);
    if (main_display == EGL_NO_DISPLAY || !eglInitialize(main_display, NULL, NULL)) {
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
    EGLConfig main_config;
    EGLint num_config = 0;
    if (!eglChooseConfig(main_display, attribs, &main_config, 1, &num_config) || num_config == 0) {
        log_egl_error("eglChooseConfig");
        eglTerminate(main_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext main_ctx = eglCreateContext(main_display, main_config, EGL_NO_CONTEXT, ctx_attribs);
    if (main_ctx == EGL_NO_CONTEXT) {
        log_egl_error("eglCreateContext");
        eglTerminate(main_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    struct gbm_surface *win = create_egl_compatible_gbm_surface(main_display,
                                                                main_config,
                                                                native_ctx->gbm_dev,
                                                                native_ctx->mode_info.hdisplay,
                                                                native_ctx->mode_info.vdisplay);
    if (!win) {
        eglDestroyContext(main_display, main_ctx);
        eglTerminate(main_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    EGLSurface surface = eglCreateWindowSurface(main_display, main_config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        log_egl_error("eglCreateWindowSurface");
        eglDestroyContext(main_display, main_ctx);
        gbm_surface_destroy(win);
        eglTerminate(main_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    if (!eglMakeCurrent(main_display, surface, surface, main_ctx)) {
        log_egl_error("eglMakeCurrent");
        eglDestroySurface(main_display, surface);
        eglDestroyContext(main_display, main_ctx);
        gbm_surface_destroy(win);
        eglTerminate(main_display);
        destroy_native_display(native_ctx);
        return 1;
    }

    glViewport(0, 0, native_ctx->mode_info.hdisplay, native_ctx->mode_info.vdisplay);
    glClearColor(0.02f, 0.08f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    reset_common_gl_objects();
    draw_quad(-1.0f, -1.0f, 1.0f, -0.65f, 0.0f, 0.0f, 0.55f, 0.95f);
    draw_triangle(0.0f, 0.0f, 0.95f, 0.35f);

    printf("DEGER A: dpy = ana ekranin EGLDisplay handle'i.\n");
    printf(" -> GORSEL SONUC: Ana ekran senaryosu koyu mavi zemin, camgobegi alt bant ve yesil ucgen cizer.\n");
    printf("    Context bu display/config uzerinde olusturuldu ve eglMakeCurrent ile bu yuzeye baglandi.\n\n");

    eglSwapBuffers(main_display, surface);
    present_native_display_for(native_ctx, win, 5);

    eglDestroySurface(main_display, surface);
    eglDestroyContext(main_display, main_ctx);
    gbm_surface_destroy(win);
    eglTerminate(main_display);
    destroy_native_display(native_ctx);
    return 0;
}
