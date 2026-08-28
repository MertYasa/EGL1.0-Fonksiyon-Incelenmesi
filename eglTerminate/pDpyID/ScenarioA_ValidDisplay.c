#include "../common/common_utils.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("=================================================================\n");
    printf("Senaryo A: eglTerminate(pDpyID) Gecerli (Valid) Parametre Kullanimi\n");
    printf("=================================================================\n");

    NativeWindow nw;
    init_native_window(&nw);
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int initialized = 0;
    int exit_code = -1;

    if (!create_drm_window(&nw, 800, 600, "EGLTerminate - Senaryo A (Gecerli Display)")) {
        goto cleanup;
    }

    // 1. Gecerli bir EGL Display al
    display = get_egl_display_for_gbm(nw.gbm_device);
    if (display == EGL_NO_DISPLAY) {
        printf("Hata: EGL Display alinamadi: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }

    // 2. EGL'yi baslat
    if (!eglInitialize(display, NULL, NULL)) {
        printf("Hata: eglInitialize basarisiz: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }
    initialized = 1;

    // 3. EGL Konfigurasyonu, Surface ve Context olustur
    EGLint attribList[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLConfig config = NULL;
    EGLint numConfigs = 0;

    if (!eglGetConfigs(display, NULL, 0, &numConfigs)) {
        printf("Hata: eglGetConfigs basarisiz: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }
    if (numConfigs == 0) {
        printf("Bilgi: eglGetConfigs cagrisi basarili oldu ama surucu hic EGL config dondurmedi; cizim yapilmayacak.\n");
        goto cleanup;
    }

    numConfigs = 0;
    if (!eglChooseConfig(display, attribList, &config, 1, &numConfigs)) {
        printf("Hata: eglChooseConfig basarisiz: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }
    if (numConfigs == 0) {
        printf("Bilgi: eglChooseConfig cagrisi basarili oldu ama GBM window surface ve GLES2 icin uygun config bulunamadi; cizim yapilmayacak.\n");
        goto cleanup;
    }

    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)nw.gbm_surface, NULL);
    if (surface == EGL_NO_SURFACE) {
        printf("Hata: eglCreateWindowSurface basarisiz: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        printf("Hata: eglCreateContext basarisiz: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }

    // 4. Context'i current yap ve cizim gerceklestir
    if (!eglMakeCurrent(display, surface, surface, context)) {
        printf("Hata: eglMakeCurrent basarisiz: %s\n", get_egl_error_str(eglGetError()));
        goto cleanup;
    }

    printf("-> Gecerli bir pDpyID kullanildigi icin EGL Context basariyla olusturuldu.\n");
    printf("-> Ekrana renkli bir ucgen ciziliyor...\n");

    if (!draw_colorful_triangle()) {
        printf("Hata: GLES2 ucgen cizimi hazirlanamadi; swap/present denenmeyecek.\n");
        goto cleanup;
    }
    if (!drm_swap_buffers(display, surface, &nw)) {
        printf("Hata: eglSwapBuffers veya DRM present basarisiz oldu; goruntu kaniti uretilemedi.\n");
        goto cleanup;
    }

    // Cizimi gorebilmek icin biraz bekle
    sleep(3);

    // 5. eglTerminate'i gecerli display ile cagir
    printf("\nSimdi eglTerminate(display) cagriliyor...\n");
    EGLBoolean result = eglTerminate(display);

    if (result == EGL_TRUE) {
        printf("-> eglTerminate BASARILI (EGL_TRUE dondu).\n");
        printf("-> Gorsel Kanit: Ucgen basariyla cizildi ve ardindan EGL baglantisi temiz bir sekilde sonlandirildi.\n");
        initialized = 0;
        context = EGL_NO_CONTEXT;
        surface = EGL_NO_SURFACE;
        exit_code = 0;
    } else {
        printf("-> Hata: eglTerminate basarisiz oldu. Hata Kodu: %s\n", get_egl_error_str(eglGetError()));
    }

cleanup:
    if (display != EGL_NO_DISPLAY) {
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        if (initialized) {
            eglTerminate(display);
        }
    }
    destroy_drm_window(&nw);
    return exit_code;
}
