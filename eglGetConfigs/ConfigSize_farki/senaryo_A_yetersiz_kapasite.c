#include "../common/common_utils.h"

int main() {
    printf("--- SENARYO A: Yetersiz Kapasite Verilmesi (Gorsel Sonuclu) ---\n\n");

    Display* x_dpy = XOpenDisplay(NULL);
    EGLDisplay egl_dpy = eglGetDisplay((EGLNativeDisplayType)x_dpy);

    EGLint major, minor;
    eglInitialize(egl_dpy, &major, &minor);

    EGLint toplam_gercek_sayi = 0;
    eglGetConfigs(egl_dpy, NULL, 0, &toplam_gercek_sayi);
    printf("Sistemde normalde %d adet konfigürasyon var.\n\n", toplam_gercek_sayi);

    EGLint aktarilan_sayi = 0;
    EGLConfig dizi[2];

    // Sistemde çok olmasına rağmen biz ConfigSize = 2 diyoruz
    eglGetConfigs(egl_dpy, dizi, 2, &aktarilan_sayi);
    printf("ConfigSize = 2 olarak belirtildigi icin EGL sadece %d adet veriyi kopyaladi.\n\n", aktarilan_sayi);

    // Bize verilen bu 2 config icinden Derinlik Tamponu (Z-Buffer) destegi olan bir tane arayalim
    EGLConfig secilen_config = dizi[0]; // Varsayilan olarak ilkini (muhtemelen derinlik desteksiz) alalim
    int derinlik_bulundu = 0;

    for(int i = 0; i < aktarilan_sayi; i++) {
        EGLint depth;
        eglGetConfigAttrib(egl_dpy, dizi[i], EGL_DEPTH_SIZE, &depth);
        if(depth >= 16) {
            secilen_config = dizi[i];
            derinlik_bulundu = 1;
            break;
        }
    }

    if(!derinlik_bulundu) {
        printf("=================================================================\n");
        printf("UYARI: Sadece 2 konfigurasyon okuyabildigimiz icin aralarinda\n");
        printf("Derinlik Tamponu (Z-Buffer) olan bir config BULAMADIK!\n");
        printf("Z-Buffer olmadigi icin 3 Boyut derinlik testi calismayacak.\n");
        printf("Gorsel Sonuc: Arkada (Z=0.5) olmasi gereken kirmizi ucgen, \n");
        printf("ondeki (Z=0.0) yesil ucgenin ustune binecektir (HATALI GORUNTU).\n");
        printf("=================================================================\n\n");
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    Window win = create_x11_window(x_dpy, 400, 400, "Senaryo A: Kapasite Az (Derinlik Yok, Goruntu Hatali)");
    EGLSurface surf = eglCreateWindowSurface(egl_dpy, secilen_config, win, NULL);
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

        // Z=0.5 (Uzak) olan Kirmizi Ucgen Cizimi (Ters cizelim ki ust uste gelsinler)
        // Common utils'teki cizimi kullanacagiz ama o ayni sekli ciziyor. Z degerleri farkli oldugundan yeterli.
        draw_triangle(0.5f, 1.0f, 0.0f, 0.0f);

        eglSwapBuffers(egl_dpy, surf);
        usleep(16000); // ~60 FPS
    }

    return 0;
}
