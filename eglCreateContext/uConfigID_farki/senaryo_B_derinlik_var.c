#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: uConfigID - Derinlik Tamponu Olan Config (EGL_DEPTH_SIZE = 16) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    eglInitialize(display, NULL, NULL);

    // DİKKAT: EGL_DEPTH_SIZE 16 isteniyor! (Donanım destekli derinlik tamponu var)
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(display, attribs, &config, 1, &num_config);

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);

    Window win = create_x11_window(x_dpy, 400, 300, "Derinlik VAR (Ucak Dag'in Onunde)");
    EGLSurface surface = eglCreateWindowSurface(display, config, win, NULL);

    eglMakeCurrent(display, surface, surface, ctx);

    glEnable(GL_DEPTH_TEST); // Derinlik testi donanım tamponu sayesinde çalışacak
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Yakındaki uçağı (Kırmızı, Z=0.1) ÖNCE çiziyoruz
    draw_triangle(0.1f, 1.0f, 0.0f, 0.0f); // Uçak

    // Uzaktaki dağı (Mavi, Z=0.5) SONRA çiziyoruz
    draw_triangle(0.5f, 0.0f, 0.0f, 1.0f); // Dağ

    printf("DEGER B: EGL_DEPTH_SIZE = 16 Config kullanildi.\n");
    printf(" -> SONUC: Donanim gercek derinlik testi (Depth Test) yapar.\n");
    printf("    Yakindaki ucak (Kirmizi) onde kalir, mavi dag arkada kalir (3D basarisi).\n\n");

    eglSwapBuffers(display, surface);
    sleep(5);

    eglDestroySurface(display, surface);
    eglDestroyContext(display, ctx);
    XCloseDisplay(x_dpy);
    return 0;
}
