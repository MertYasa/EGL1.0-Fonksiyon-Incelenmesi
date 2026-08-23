#include "../common/common_utils.h"

int main() {
    printf("--- PROFESYONEL 2 ADIMLI SORGU (Gorsel Sonuclu) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay egl_dpy = eglGetDisplay((EGLNativeDisplayType)x_dpy);
    EGLint major, minor;
    eglInitialize(egl_dpy, &major, &minor);

    // 1. ADIM: Sadece kac tane config oldugunu ogren (pConfigs = NULL)
    EGLint toplam_config = 0;
    eglGetConfigs(egl_dpy, NULL, 0, &toplam_config);

    printf("Adim 1: Sistemde %d adet konfigürasyon tespit edildi.\n", toplam_config);

    if (toplam_config == 0) {
        printf("Sistem EGL desteklemiyor veya bosta.\n");
        return -1;
    }

    // 2. ADIM: Tam ihtiyacimiz kadar bellek ayir ve gercek verileri cek
    EGLConfig* tum_configler = (EGLConfig*) malloc(toplam_config * sizeof(EGLConfig));
    EGLint aktarilan_sayi = 0;

    EGLBoolean basari = eglGetConfigs(egl_dpy, tum_configler, toplam_config, &aktarilan_sayi);

    if (basari == EGL_TRUE) {
        printf("Adim 2: %d adet konfigürasyon basariyla bellege alindi.\n\n", aktarilan_sayi);

        // Profesyonel islem: Havuzdaki tum configlerden en iyisini (Derinlikli) bul
        EGLConfig secilen_config = tum_configler[0];
        for(int i = 0; i < aktarilan_sayi; i++) {
            EGLint depth;
            eglGetConfigAttrib(egl_dpy, tum_configler[i], EGL_DEPTH_SIZE, &depth);
            if(depth >= 16) {
                secilen_config = tum_configler[i];
                break;
            }
        }

        free(tum_configler); // Bellek temizligi

        printf("=================================================================\n");
        printf("BASARILI: Profesyonel 2-adimli yontem ile EGL kurulumu kusursuz!\n");
        printf("Gorsel Sonuc: Olasi en dogru konfigürasyon bulundu, Z-Buffer acik.\n");
        printf("Ondeki yesil ucgen, arkadaki kirmizi ucgenin ustunu (dogru sekilde) kapatacak.\n");
        printf("=================================================================\n\n");

        eglBindAPI(EGL_OPENGL_ES_API);
        EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

        Window win = create_x11_window(x_dpy, 400, 400, "Profesyonel Sorgu (Kusursuz Cizim)");
        EGLSurface surf = eglCreateWindowSurface(egl_dpy, secilen_config, win, NULL);
        EGLContext ctx = eglCreateContext(egl_dpy, secilen_config, EGL_NO_CONTEXT, ctx_attribs);

        eglMakeCurrent(egl_dpy, surf, surf, ctx);

        printf("Pencere acildi. Cikmak icin terminalde Ctrl+C yapin.\n");

        while(1) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Koyu gri
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            draw_triangle(0.0f, 0.0f, 1.0f, 0.0f); // Z=0.0 Yesil
            draw_triangle(0.5f, 1.0f, 0.0f, 0.0f); // Z=0.5 Kirmizi (Dogru sekilde arkada kalacak)

            eglSwapBuffers(egl_dpy, surf);
            usleep(16000);
        }
    } else {
        free(tum_configler);
    }

    return 0;
}
