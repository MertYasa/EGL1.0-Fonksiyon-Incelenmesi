#include "../common/common_utils.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("=================================================================\n");
    printf("Senaryo B: eglTerminate(pDpyID) Hatali (EGL_NO_DISPLAY) Parametre Kullanimi\n");
    printf("=================================================================\n");

    NativeWindow nw;
    if (!create_x11_window(&nw, 800, 600, "EGLTerminate - Senaryo B (Hatali Display)")) {
        return -1;
    }

    printf("X11 Penceresi acildi. Simdi hatali parametre (EGL_NO_DISPLAY) kullanilarak eglTerminate cagrilmaya calisiliyor...\n");

    // Hatali pDpyID parametresi (EGL_NO_DISPLAY) ile eglTerminate cagiriyoruz
    EGLBoolean result = eglTerminate(EGL_NO_DISPLAY);

    if (result == EGL_FALSE) {
        EGLint err = eglGetError();
        printf("\n-> Beklendigi gibi eglTerminate BASARISIZ oldu (EGL_FALSE dondu).\n");
        printf("-> Alinan EGL Hatasi: %s\n", get_egl_error_str(err));

        if (err == EGL_BAD_DISPLAY) {
            printf("\nSONUC:\n");
            printf("Bu parametreyi hatalı/eksik (NULL/EGL_NO_DISPLAY) verdiğimiz için gecerli bir EGL Display baglantisi kurulamadi.\n");
            printf("Bu yuzden EGL Context oluşturulamadı ve ekrana HİÇBİR ŞEY ÇİZİLEMEDİ.\n");
            printf("Gorsel Kanit: Ekranda sadece bos, siyah bir X11 penceresi gorunmektedir (3 saniye sonra kapanacak).\n");
        }
    } else {
        printf("-> Beklenmeyen durum: eglTerminate basarili oldu!\n");
    }

    // Bos pencereyi gormek icin bekle
    sleep(3);

    destroy_x11_window(&nw);
    return 0;
}
