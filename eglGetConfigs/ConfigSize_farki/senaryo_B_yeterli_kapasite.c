#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO B: Yeterli Kapasite Verilmesi (Gorsel Sonuclu) ---\n\n");

    AppState state;
    init_drm_and_gbm(&state, 400, 400);
    EGLDisplay egl_dpy = eglGetDisplay((EGLNativeDisplayType)state.gbm_device);

    EGLint major, minor;
    eglInitialize(egl_dpy, &major, &minor);

    EGLint toplam_gercek_sayi = 0;
    eglGetConfigs(egl_dpy, NULL, 0, &toplam_gercek_sayi);
    printf("Sistemde normalde %d adet konfigürasyon var.\n\n", toplam_gercek_sayi);

    EGLint aktarilan_sayi = 0;
    EGLConfig dizi[500];

    // Genis bir dizi verdigimiz icin sistemdeki tum configleri alabilecegiz
    eglGetConfigs(egl_dpy, dizi, 500, &aktarilan_sayi);
    printf("ConfigSize = 500 olarak belirtildigi icin EGL var olan %d verinin tamamini kopyaladi.\n\n", aktarilan_sayi);

    // Bize verilen bu genis havuzun icinden Derinlik Tamponu (Z-Buffer) destegi olan bir tane rahatca bulabiliriz
    EGLConfig secilen_config = 0;
    int derinlik_bulundu = 0;

    for(int i = 0; i < aktarilan_sayi; i++) {
        EGLint depth, renderable, surface;
        eglGetConfigAttrib(egl_dpy, dizi[i], EGL_DEPTH_SIZE, &depth);
        eglGetConfigAttrib(egl_dpy, dizi[i], EGL_RENDERABLE_TYPE, &renderable);
        eglGetConfigAttrib(egl_dpy, dizi[i], EGL_SURFACE_TYPE, &surface);

        if(depth >= 16 && (renderable & EGL_OPENGL_ES2_BIT) && (surface & EGL_WINDOW_BIT)) {
            secilen_config = dizi[i];
            derinlik_bulundu = 1;
            break;
        }
    }

    if(derinlik_bulundu) {
        printf("=================================================================\n");
        printf("BASARILI: Kapasitemiz genis oldugu icin %d adet config icinden\n", aktarilan_sayi);
        printf("Derinlik Tamponu (Z-Buffer) olan bir config BULMAYI BASARDIK!\n");
        printf("Z-Buffer aktif oldugu icin 3 Boyut derinlik testi calisacak.\n");
        printf("Gorsel Sonuc: Onde (Z=0.0) olmasi gereken yesil ucgen,\n");
        printf("arkadaki (Z=0.5) kirmizi ucgenin onunde (DOGRU) duracaktir.\n");
        printf("=================================================================\n\n");
    } else {
        printf("HATA: Sisteminizde derinlik tamponu destekleyen config yok!\n");
        cleanup_drm_and_gbm(&state);
        return -1;
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    EGLSurface surf = eglCreateWindowSurface(egl_dpy, secilen_config, (EGLNativeWindowType)state.gbm_surface, NULL);
    EGLContext ctx = eglCreateContext(egl_dpy, secilen_config, EGL_NO_CONTEXT, ctx_attribs);

    eglMakeCurrent(egl_dpy, surf, surf, ctx);

    printf("Pencere acildi. Cikmak icin terminalde Ctrl+C yapin.\n");

    while(1) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Z=0.0 (Yakin) olan Yesil Ucgen Cizimi
        draw_triangle(0.0f, 0.0f, 1.0f, 0.0f);

        // Z=0.5 (Uzak) olan Kirmizi Ucgen Cizimi
        draw_triangle(0.5f, 1.0f, 0.0f, 0.0f);

        eglSwapBuffers(egl_dpy, surf);
        usleep(16000); // ~60 FPS
    }

    cleanup_drm_and_gbm(&state);
    return 0;
}
