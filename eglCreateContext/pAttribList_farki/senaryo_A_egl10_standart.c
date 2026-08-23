#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: pAttribList - EGL 1.0 Standart Kullanimi ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    eglInitialize(display, NULL, NULL);

    EGLint attribs[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE };
    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(display, attribs, &config, 1, &num_config);

    // EGL 1.0'da zorunlu olduğu gibi { EGL_NONE } veya NULL veriyoruz.
    EGLint egl10_attribs[] = { EGL_NONE };
    EGLContext ctx_egl10 = eglCreateContext(display, config, EGL_NO_CONTEXT, egl10_attribs);

    if (ctx_egl10 != EGL_NO_CONTEXT) {
        EGLint pbuffer_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
        EGLSurface pbuffer_surf = eglCreatePbufferSurface(display, config, pbuffer_attribs);

        eglMakeCurrent(display, pbuffer_surf, pbuffer_surf, ctx_egl10);

        const GLubyte* version = glGetString(GL_VERSION);

        printf("DEGER: pAttribList = { EGL_NONE }\n");
        printf(" -> SONUC: Sabit (Fixed-Function) boru hattina (Orn: GLES 1.x) girilir.\n");
        printf("    Aktif OpenGL ES Versiyonu: %s\n", version ? (const char*)version : "Bilinmiyor");
        printf("    (Gorsel ekrana gerek yok, versiyon kontrolu ile pAttribList'in etkisi goruldu)\n\n");

        eglDestroySurface(display, pbuffer_surf);
    }

    eglDestroyContext(display, ctx_egl10);
    XCloseDisplay(x_dpy);
    return 0;
}
