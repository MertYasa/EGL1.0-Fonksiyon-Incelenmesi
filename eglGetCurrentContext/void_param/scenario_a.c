#include "../common/common_utils.h"
#include <stdio.h>

int main(void) {
    printf("==================================================\n");
    printf("SENARYO A: Aktif bir context varken eglGetCurrentContext cagirimi\n");
    printf("==================================================\n");

    AppState state;
    if (!init_egl_and_drm(&state, 800, 600, "Senaryo A - Aktif Context")) {
        return -1;
    }

    // Context'i aktif hale getir (Make current)
    if (!make_current_checked(&state, state.egl_surface, state.egl_surface, state.egl_context,
                              "Context aktif edilemedi; senaryo temiz kapatiliyor.")) {
        cleanup(&state);
        return -1;
                              }

    printf("1. eglMakeCurrent basariyla cagirildi ve context aktif edildi.\n");

    // eglGetCurrentContext'i cagir
    EGLContext current = eglGetCurrentContext();

    if (current == EGL_NO_CONTEXT) {
        printf("2. HATA: eglGetCurrentContext EGL_NO_CONTEXT dondurdu!\n");
    } else if (current == state.egl_context) {
        printf("2. BASARILI: eglGetCurrentContext aktif olan context'i (%p) dogru sekilde dondurdu.\n", (void*)current);
    } else {
        printf("2. HATA: eglGetCurrentContext farkli bir context dondurdu!\n");
    }

    // Basarili durumda ekrana cizim yap (Gorsel kanit)
    printf("3. Cizim islemi baslatiliyor (Ekranda renkli bir ucgen gormelisiniz)...\n");

    setup_triangle_drawing();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    draw_triangle();

    if (!swap_buffers_checked(&state)) {
        cleanup(&state);
        return -1;
    }

    printf("4. Cizim tamamlandi, pencere 3 saniye acik kalacak.\n");
    sleep_ms(3000);

    cleanup(&state);
    return 0;
}
