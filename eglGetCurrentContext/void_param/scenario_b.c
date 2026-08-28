#include "../common/common_utils.h"
#include <stdio.h>

int main(void) {
    printf("==================================================\n");
    printf("SENARYO B: Aktif bir context YOKKEN eglGetCurrentContext cagirimi\n");
    printf("==================================================\n");

    AppState state;
    if (!init_egl_and_drm(&state, 800, 600, "Senaryo B - Hata Durumu (Bosanmis Context)")) {
        return -1;
    }

    // Ilk olarak context'i bagla ve ekrani kirmiziya boya
    if (!make_current_checked(&state, state.egl_surface, state.egl_surface, state.egl_context,
                              "Context aktif edilemedi; senaryo temiz kapatiliyor.")) {
        cleanup(&state);
        return -1;
    }
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // Kirmizi arka plan
    glClear(GL_COLOR_BUFFER_BIT);

    // Bilerek context'i detach (ayirma) yapiyoruz
    printf("1. eglMakeCurrent ile aktif context kapatiliyor (EGL_NO_CONTEXT geciliyor).\n");
    if (!make_current_checked(&state, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT,
                              "Context detach edilemedi; senaryo temiz kapatiliyor.")) {
        cleanup(&state);
        return -1;
    }

    // Context olmayan durumda eglGetCurrentContext'i cagir
    EGLContext current = eglGetCurrentContext();

    printf("2. eglGetCurrentContext cagirildi.\n");
    if (current == EGL_NO_CONTEXT) {
        printf("-> SONUC: Beklendigi gibi eglGetCurrentContext() EGL_NO_CONTEXT dondurdu.\n");
    } else {
        printf("-> HATA: Beklenmeyen bir context donduruldu!\n");
    }

    printf("\n>>> Bu senaryoda aktif bir context olmadigi (eglGetCurrentContext() EGL_NO_CONTEXT dondurdugu) icin ucgen cizilemedi ve ekrana HICBIR SEY CIZILEMEDI. <<<\n\n");

    printf("3. Gorsel ispat icin pencere 3 saniye acik tutuluyor. (Sadece kirmizi arka plan goreceksiniz, ucgen yok!)\n");

    // Ekrani guncellemek icin context'i tekrar gecici olarak bagliyoruz (sadece eglSwapBuffers yapabilmek icin, cizim YAPMIYORUZ)
    if (!make_current_checked(&state, state.egl_surface, state.egl_surface, state.egl_context,
                              "Context tekrar baglanamadi; senaryo temiz kapatiliyor.")) {
        cleanup(&state);
        return -1;
    }
    if (!swap_buffers_checked(&state)) {
        cleanup(&state);
        return -1;
    }

    sleep_ms(3000);

    cleanup(&state);
    return 0;
}
