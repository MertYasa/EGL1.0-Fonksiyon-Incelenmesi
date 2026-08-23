#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: uConfigID - Derinlik Tamponu Olmayan Config (EGL_DEPTH_SIZE = 0) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)x_dpy);
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

    Window win = create_x11_window(x_dpy, 400, 300, "Derinlik YOK (Dag, Ucagi Ezer)");
    EGLSurface surface = eglCreateWindowSurface(display, config, win, NULL);

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
    sleep(5);

    eglDestroySurface(display, surface);
    eglDestroyContext(display, ctx);
    XCloseDisplay(x_dpy);
    return 0;
}
