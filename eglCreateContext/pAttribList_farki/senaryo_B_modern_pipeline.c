#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: pAttribList - Modern Pipeline (EGL_CONTEXT_CLIENT_VERSION) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    eglInitialize(display, NULL, NULL);

    EGLint attribs[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE };
    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(display, attribs, &config, 1, &num_config);

    // EGL 1.4 / GLES2 yapısında modern shader boru hattını istiyoruz
    EGLint modern_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx_modern = eglCreateContext(display, config, EGL_NO_CONTEXT, modern_attribs);

    if (ctx_modern != EGL_NO_CONTEXT) {
        EGLint pbuffer_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
        EGLSurface pbuffer_surf = eglCreatePbufferSurface(display, config, pbuffer_attribs);

        eglMakeCurrent(display, pbuffer_surf, pbuffer_surf, ctx_modern);

        const GLubyte* version = glGetString(GL_VERSION);

        printf("DEGER: pAttribList = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE }\n");
        printf(" -> SONUC: Programmable Shader (GLES 2.0 / SC 2.0) boru hattina girildi.\n");
        printf("    Aktif OpenGL ES Versiyonu: %s\n", version ? (const char*)version : "Bilinmiyor");
        printf("    (Gorsel ekrana gerek yok, versiyon kontrolu ile pAttribList'in etkisi goruldu)\n\n");

        eglDestroySurface(display, pbuffer_surf);
    }

    eglDestroyContext(display, ctx_modern);
    XCloseDisplay(x_dpy);
    return 0;
}
