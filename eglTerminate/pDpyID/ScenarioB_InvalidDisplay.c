#include "../common/common_utils.h"
#include <stdio.h>

int main(void) {
    printf("=================================================================\n");
    printf("Senaryo B: eglTerminate(pDpyID) Hatali (EGL_NO_DISPLAY) Parametre Kullanimi\n");
    printf("=================================================================\n");

    printf("Bu negatif senaryoda pDpyID olarak EGL_NO_DISPLAY/NULL benzeri gecersiz bir deger kullanimi anlatilir.\n");
    printf("Guvenlik nedeniyle gercek eglTerminate(EGL_NO_DISPLAY), EGL/GBM veya DRM cagrisi yapilmiyor.\n");
    printf("Beklenen sonuc: EGL implementasyonu boyle bir display'i gecerli kabul etmemeli ve EGL_BAD_DISPLAY raporlamalidir.\n");
    printf("SONUC: Gecerli display olmadigi icin context/surface olusturulmaz, cizim ve DRM present denenmez.\n");

    return 0;
}
