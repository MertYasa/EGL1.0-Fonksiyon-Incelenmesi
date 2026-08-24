#include "../common/common_utils.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("=================================================================\n");
    printf("Senaryo A: eglTerminate(pDpyID) Gecerli (Valid) Parametre Kullanimi\n");
    printf("=================================================================\n");

    NativeWindow nw;
    if (!create_drm_window(&nw, 800, 600, "EGLTerminate - Senaryo A (Gecerli Display)")) {
        return -1;
    }

    // 1. Gecerli bir EGL Display al
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)nw.gbm_device);
    if (display == EGL_NO_DISPLAY) {
        printf("Hata: EGL Display alinamadi.\n");
        return -1;
    }

    // 2. EGL'yi baslat
    if (!eglInitialize(display, NULL, NULL)) {
        printf("Hata: EGL baslatilamadi.\n");
        return -1;
    }

    // 3. EGL Konfigurasyonu, Surface ve Context olustur
    EGLint attribList[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribList, &config, 1, &numConfigs);

    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)nw.gbm_surface, NULL);

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

    // 4. Context'i current yap ve cizim gerceklestir
    eglMakeCurrent(display, surface, surface, context);

    printf("-> Gecerli bir pDpyID kullanildigi icin EGL Context basariyla olusturuldu.\n");
    printf("-> Ekrana renkli bir ucgen ciziliyor...\n");

    draw_colorful_triangle();
    drm_swap_buffers(display, surface, &nw);

    // Cizimi gorebilmek icin biraz bekle
    sleep(3);

    // 5. eglTerminate'i gecerli display ile cagir
    printf("\nSimdi eglTerminate(display) cagriliyor...\n");
    EGLBoolean result = eglTerminate(display);

    if (result == EGL_TRUE) {
        printf("-> eglTerminate BASARILI (EGL_TRUE dondu).\n");
        printf("-> Gorsel Kanit: Ucgen basariyla cizildi ve ardindan EGL baglantisi temiz bir sekilde sonlandirildi.\n");
    } else {
        printf("-> Hata: eglTerminate basarisiz oldu. Hata Kodu: %s\n", get_egl_error_str(eglGetError()));
    }

    destroy_drm_window(&nw);
    return 0;
}
